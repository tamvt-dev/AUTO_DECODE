#include "decoder_engine.h"
#include <QDebug>
#include <mutex>

#include "../core/include/core.h"
#include "../core/include/buffer.h"
#include "../core/include/pipeline.h"
#include "../core/include/plugin.h"

static QString bufferToString(const Buffer &buf) {
    return QString::fromUtf8(reinterpret_cast<const char*>(buf.data), buf.len);
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
        return result;
    }

    QByteArray inBytes = input.toUtf8();
    Buffer in = buffer_new(reinterpret_cast<const unsigned char*>(inBytes.constData()), inBytes.size());
    GList *candidates = pipeline_smart_search(in, maxDepth, beamWidth);
    buffer_free(&in);

    if (candidates) {
        Candidate *best = (Candidate*)candidates->data;
        result.output = bufferToString(best->buf);
        result.score = best->score;

        QString steps;
        for (GList *s = best->steps; s; s = s->next) {
            if (!steps.isEmpty()) {
                steps += " -> ";
            }
            steps += reinterpret_cast<const char*>(s->data);
        }

        result.steps =
            "Input -> Fast Heuristic Engine -> AI Strategy Planner -> "
            "Multi-Pipeline Executor -> Scoring Engine -> Auto Retry + Mutation -> Best Result"
            "\nRoute: " + steps;
        result.success = true;
    } else {
        result.success = false;
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
