#include "decoder_engine.h"
#include <QDebug>
#include <mutex>

#include "../core/include/core.h"
#include "../core/include/buffer.h"
#include "../core/include/pipeline.h"
#include "../core/include/plugin.h"
#include "../core/include/recipe.h"
#include <QJsonDocument>

static QString bufferToString(const Buffer &buf) {
    if (!buf.data || buf.len == 0) return QString();
    
    // Direct conversion without expensive debug per-byte logging
    return QString::fromLatin1(reinterpret_cast<const char*>(buf.data), buf.len);
}

DecoderEngine::RecipeResult DecoderEngine::executeRecipe(const QString &jsonStr, const QString &input)
{
    RecipeResult result;
    if (input.isEmpty()) {
        result.success = false;
        result.error = "Empty input";
        return result;
    }

    QByteArray jsonBytes = jsonStr.toUtf8();
    Recipe *recipe = recipe_parse_json(jsonBytes.constData());
    if (!recipe) {
        result.success = false;
        result.error = "Invalid recipe JSON";
        return result;
    }

    QByteArray inputBytes = input.toUtf8();
    Buffer inBuf = buffer_new(reinterpret_cast<const unsigned char*>(inputBytes.constData()), inputBytes.size());
    Buffer outBuf = recipe_execute(recipe, inBuf);
    recipe_free(recipe);
    buffer_free(&inBuf);

    if (outBuf.data) {
        result.output = bufferToString(outBuf);
        result.success = true;
        buffer_free(&outBuf);
    } else {
        result.success = false;
        result.error = "Recipe execution failed";
        buffer_free(&outBuf);
    }
    return result;
}

static Plugin* findPluginByName(const QString &name)
{
    PluginManager *pm = core_get_plugin_manager();
    if (!pm) return nullptr;

    GList *plugins = plugin_manager_list(pm);
    for (GList *iter = plugins; iter; iter = iter->next) {
        Plugin *p = (Plugin*)iter->data;
        if (name == p->name) {
            g_list_free(plugins);
            return p;
        }
    }
    g_list_free(plugins);
    return nullptr;
}

DecoderEngine& DecoderEngine::instance()
{
    static DecoderEngine engine;
    return engine;
}

void DecoderEngine::refreshFormats()
{
    rebuildFormatLists();
}

void DecoderEngine::rebuildFormatLists()
{
    m_decodeFormats.clear();
    m_decodeFormats << "Auto Detect" << "Base64" << "Hex" << "Binary" << "Morse";

    m_encodeFormats.clear();
    m_encodeFormats << "Base64" << "Hex" << "Binary" << "Morse";

    PluginManager *pm = core_get_plugin_manager();
    if (pm) {
        GList *plugins = plugin_manager_list(pm);
        for (GList *iter = plugins; iter; iter = iter->next) {
            Plugin *p = (Plugin*)iter->data;
            m_decodeFormats << p->name;
            if (p->encode) {
                m_encodeFormats << p->name;
            }
        }
        g_list_free(plugins);
    }
}

DecoderEngine::DecodeResult DecoderEngine::decode(const QString &input, int formatIndex)
{
    DecodeResult result;
    if (input.isEmpty()) {
        result.success = false;
        result.errorMessage = "Empty input";
        return result;
    }

    QByteArray inBytes = input.toUtf8();
    const char *cInput = inBytes.constData();

    if (formatIndex < 5) {
        CoreMode mode;
        switch (formatIndex) {
            case 0: mode = CORE_MODE_AUTO; break;
            case 1: mode = CORE_MODE_BASE64; break;
            case 2: mode = CORE_MODE_HEX; break;
            case 3: mode = CORE_MODE_BINARY; break;
            case 4: mode = CORE_MODE_MORSE; break;
            default: mode = CORE_MODE_AUTO;
        }
        ::DecodeResult *cRes = core_decode_with_mode(cInput, mode);
        if (cRes && cRes->error_code == ERROR_OK) {
            result.output = QString::fromUtf8(cRes->output);
            result.format = QString::fromUtf8(cRes->format);
            result.processingTimeMs = cRes->processing_time_ms;
            result.success = true;
        } else {
            result.success = false;
            result.errorMessage = cRes && cRes->error_message ? QString::fromUtf8(cRes->error_message) : "Decode failed";
        }
        if (cRes) decode_result_free(cRes);
    } else {
        QString pluginName = m_decodeFormats[formatIndex];
        Plugin *p = findPluginByName(pluginName);
        if (p && p->decode_single) {
            Buffer in = buffer_new(reinterpret_cast<const unsigned char*>(inBytes.constData()), inBytes.size());
            Buffer out = p->decode_single(in);
            buffer_free(&in);
            if (out.data) {
                result.output = bufferToString(out);
                result.format = pluginName;
                result.success = true;
                buffer_free(&out);
            } else {
                result.success = false;
                result.errorMessage = "Plugin decode failed";
            }
        } else {
            result.success = false;
            result.errorMessage = "Plugin decode not available";
        }
    }
    return result;
}

DecoderEngine::EncodeResult DecoderEngine::encode(const QString &input, int formatIndex)
{
    EncodeResult result;
    if (input.isEmpty()) {
        result.success = false;
        result.errorMessage = "Empty input";
        return result;
    }

    QByteArray inBytes = input.toUtf8();
    const char *cInput = inBytes.constData();

    if (formatIndex < 4) {
        CoreMode mode;
        switch (formatIndex) {
            case 0: mode = CORE_MODE_BASE64; break;
            case 1: mode = CORE_MODE_HEX; break;
            case 2: mode = CORE_MODE_BINARY; break;
            case 3: mode = CORE_MODE_MORSE; break;
            default: mode = CORE_MODE_BASE64;
        }
        ::EncodeResult *cRes = core_encode(cInput, mode);
        if (cRes && cRes->error_code == ERROR_OK) {
            result.output = QString::fromUtf8(cRes->output);
            result.success = true;
        } else {
            result.success = false;
            result.errorMessage = "Encode failed";
        }
        if (cRes) encode_result_free(cRes);
    } else {
        QString pluginName = m_encodeFormats[formatIndex];
        Plugin *p = findPluginByName(pluginName);
        if (p && p->encode) {
            Buffer in = buffer_new(reinterpret_cast<const unsigned char*>(inBytes.constData()), inBytes.size());
            Buffer out = p->encode(in);
            buffer_free(&in);
            if (out.data) {
                result.output = bufferToString(out);
                result.success = true;
                buffer_free(&out);
            } else {
                result.success = false;
                result.errorMessage = "Plugin encode failed";
            }
        } else {
            result.success = false;
            result.errorMessage = "Plugin encode not available";
        }
    }
    return result;
}

DecoderEngine::PipelineResult DecoderEngine::runPipeline(const QString &input, int maxDepth, int beamWidth)
{
    PipelineResult result;
    if (input.isEmpty()) {
        result.success = false;
        result.errorMessage = "Empty input data.";
        return result;
    }

    QByteArray inBytes = input.toUtf8();
    Buffer in = buffer_new(reinterpret_cast<const unsigned char*>(inBytes.constData()), inBytes.size());
    
    // Perform deep heuristic search
    GList *candidates = pipeline_smart_search(in, maxDepth, beamWidth);
    buffer_free(&in);

    if (candidates) {
        // Collect Top 5 candidates for UI visibility
        int count = 0;
        for (GList *iter = candidates; iter && count < 5; iter = iter->next, count++) {
            Candidate *cand = (Candidate*)iter->data;
            CandidateResult cr;
            cr.output = bufferToString(cand->buf);
            cr.score = cand->score;

            // Simple route string
            QString stepsStr;
            for (GList *si = cand->history; si; si = si->next) {
                StepInfo *info = (StepInfo*)si->data;
                if (!stepsStr.isEmpty()) stepsStr += " -> ";
                stepsStr += QString::fromUtf8(info->name);
                
                // Detailed history for step-by-step inspection
                cr.history.append({QString::fromUtf8(info->name), bufferToString(info->buf)});
            }
            cr.steps = stepsStr;
            result.candidates.append(cr);
        }
        result.success = true;
    } else {
        result.success = false;
        result.errorMessage = "No valid decoding path discovered.";
    }

    if (candidates) {
        candidate_list_free(candidates);
    }
    return result;
}

void DecoderEngine::clearCache()
{
    core_cache_clear();
}
