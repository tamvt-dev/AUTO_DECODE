#include "../include/plugin.h"
#include "../include/pipeline.h"
#include "../include/score.h"
#include "../include/core.h"
#include "../include/decoder.h"
#include <glib.h>
#include <ctype.h>
#include <string.h>

static char* buffer_key_raw(const unsigned char *data, size_t len) {
    static const char hex[] = "0123456789abcdef";
    char *out = g_malloc(len * 2 + 1);
    for (size_t i = 0; i < len; i++) {
        out[i*2]     = hex[(data[i] >> 4) & 0xF];
        out[i*2 + 1] = hex[data[i] & 0xF];
    }
    out[len * 2] = '\0';
    return out;
}

static int compare_candidate(const void *a, const void *b) {
    const Candidate *ca = (const Candidate*)a;
    const Candidate *cb = (const Candidate*)b;
    if (cb->score > ca->score) return 1;
    if (cb->score < ca->score) return -1;
    return 0;
}

static void candidate_free(Candidate *c) {
    if (c) {
        g_free(c->buf.data);
        g_list_free_full(c->steps, g_free);
        g_free(c->meta);
        g_free(c);
    }
}

static Candidate* candidate_clone(const Candidate *src) {
    Candidate *dst = g_new0(Candidate, 1);
    dst->buf.data = g_malloc(src->buf.len);
    memcpy(dst->buf.data, src->buf.data, src->buf.len);
    dst->buf.len = src->buf.len;
    dst->score = src->score;
    dst->steps = g_list_copy_deep(src->steps, (GCopyFunc)g_strdup, NULL);
    dst->meta = g_strdup(src->meta);
    return dst;
}

typedef struct {
    Plugin *plugin;
    int priority;
} PlannedPlugin;

typedef struct {
    int max_depth;
    int beam_width;
    int retry_count;
    GList *plugins; // PlannedPlugin*
} PipelinePlan;

static double candidate_pipeline_score(const Candidate *cand);
static double step_confidence_boost(const char *step_name);
static gboolean should_stop_on_readable_text(const Candidate *cand);
static gboolean is_exploratory_transform(const char *step_name);
static gboolean should_skip_plugin_for_candidate(const Candidate *cand, const Plugin *plugin, int depth);
static const char* candidate_last_step(const Candidate *cand);
static gboolean has_structured_artifact(const Buffer *buf);
static gboolean is_structured_transform(const char *step_name);

static gboolean is_base64_like(const Buffer *buf) {
    if (!buf || !buf->data || buf->len < 8) return FALSE;
    size_t significant = 0;
    for (size_t i = 0; i < buf->len; i++) {
        unsigned char c = buf->data[i];
        if (g_ascii_isspace(c)) continue;
        significant++;
        if (!(g_ascii_isalnum(c) || c == '+' || c == '/' || c == '=' || c == '-' || c == '_')) {
            return FALSE;
        }
    }
    return significant >= 8;
}

static gboolean is_hex_like(const Buffer *buf) {
    if (!buf || !buf->data || buf->len < 4) return FALSE;
    size_t significant = 0;
    for (size_t i = 0; i < buf->len; i++) {
        unsigned char c = buf->data[i];
        if (g_ascii_isspace(c)) continue;
        significant++;
        if (!g_ascii_isxdigit(c)) return FALSE;
    }
    return significant >= 4 && (significant % 2 == 0);
}

static gboolean is_binary_like(const Buffer *buf) {
    if (!buf || !buf->data || buf->len < 8) return FALSE;
    size_t bits = 0;
    for (size_t i = 0; i < buf->len; i++) {
        unsigned char c = buf->data[i];
        if (g_ascii_isspace(c)) continue;
        if (c != '0' && c != '1') return FALSE;
        bits++;
    }
    return bits >= 8;
}

static gboolean is_morse_like(const Buffer *buf) {
    if (!buf || !buf->data || buf->len < 3) return FALSE;
    gboolean has_symbol = FALSE;
    for (size_t i = 0; i < buf->len; i++) {
        unsigned char c = buf->data[i];
        if (c == '.' || c == '-' || c == '/' || g_ascii_isspace(c)) {
            if (c == '.' || c == '-') has_symbol = TRUE;
            continue;
        }
        return FALSE;
    }
    return has_symbol;
}

static gboolean is_url_like(const Buffer *buf) {
    if (!buf || !buf->data || buf->len < 3) return FALSE;
    const char *text = (const char*)buf->data;
    return g_strstr_len(text, buf->len, "%") != NULL ||
           g_strstr_len(text, buf->len, "http") != NULL ||
           g_strstr_len(text, buf->len, "+") != NULL;
}

static gboolean is_alpha_text(const Buffer *buf) {
    if (!buf || !buf->data || buf->len == 0) return FALSE;
    size_t alpha = 0;
    size_t printable = 0;
    for (size_t i = 0; i < buf->len; i++) {
        unsigned char c = buf->data[i];
        if (isprint(c)) printable++;
        if (isalpha(c)) alpha++;
    }
    return printable > 0 && alpha >= printable / 2;
}

static int plugin_priority_for_input(Plugin *p, const Buffer *input) {
    int priority = p->priority * 10;
    if (p->detect && p->detect(*input)) priority += 120;

    if (g_strcmp0(p->name, "Base64") == 0 && is_base64_like(input)) priority += 70;
    if (g_strcmp0(p->name, "Hex") == 0 && is_hex_like(input)) priority += 70;
    if (g_strcmp0(p->name, "Binary") == 0 && is_binary_like(input)) priority += 70;
    if (g_strcmp0(p->name, "Morse") == 0 && is_morse_like(input)) priority += 70;
    if (g_strcmp0(p->name, "URL") == 0 && is_url_like(input)) priority += 60;
    if ((g_strcmp0(p->name, "ROT13") == 0 ||
         g_strcmp0(p->name, "Caesar") == 0 ||
         g_strcmp0(p->name, "Atbash") == 0) && is_alpha_text(input)) {
        priority += 35;
    }
    if (g_strcmp0(p->name, "XOR") == 0) priority += 20;
    if (g_strcmp0(p->name, "Scramble") == 0) priority += 10;

    return priority;
}

static int compare_planned_plugin(const void *a, const void *b) {
    const PlannedPlugin *pa = (const PlannedPlugin*)a;
    const PlannedPlugin *pb = (const PlannedPlugin*)b;
    return pb->priority - pa->priority;
}

static void planned_plugin_free(PlannedPlugin *pp) {
    g_free(pp);
}

static GList* build_planned_plugins(PluginManager *pm, const Buffer *input) {
    GList *plugins = plugin_manager_list(pm);
    GList *planned = NULL;

    for (GList *iter = plugins; iter; iter = iter->next) {
        Plugin *p = (Plugin*)iter->data;
        if (!p->enabled || (!p->decode_single && !p->decode_multi)) continue;

        PlannedPlugin *pp = g_new0(PlannedPlugin, 1);
        pp->plugin = p;
        pp->priority = plugin_priority_for_input(p, input);
        planned = g_list_append(planned, pp);
    }

    g_list_free(plugins);
    return g_list_sort(planned, (GCompareFunc)compare_planned_plugin);
}

static void pipeline_plan_free(PipelinePlan *plan) {
    if (!plan) return;
    g_list_free_full(plan->plugins, (GDestroyNotify)planned_plugin_free);
    g_free(plan);
}

static PipelinePlan* build_pipeline_plan(const Buffer *input, int max_depth, int beam_width) {
    PluginManager *pm = core_get_plugin_manager();
    if (!pm) return NULL;

    PipelinePlan *plan = g_new0(PipelinePlan, 1);
    plan->max_depth = max_depth;
    plan->beam_width = beam_width;
    plan->retry_count = 2;
    plan->plugins = build_planned_plugins(pm, input);

    if (is_base64_like(input) || is_hex_like(input) || is_binary_like(input) || is_morse_like(input)) {
        plan->max_depth = MIN(max_depth + 2, 6);
        plan->beam_width += 2;
    } else if (is_alpha_text(input)) {
        plan->max_depth = MIN(max_depth + 1, 5);
    } else {
        plan->retry_count = 3;
    }

    return plan;
}

static double candidate_pipeline_score(const Candidate *cand) {
    int step_count = g_list_length(cand->steps);
    double score = score_readability(cand->buf.data, cand->buf.len);
    double confidence = 0.0;
    gboolean structured_artifact = has_structured_artifact(&cand->buf);

    for (GList *iter = cand->steps; iter; iter = iter->next) {
        const char *step = (const char*)iter->data;
        if (g_strcmp0(step, "Input") == 0) continue;
        confidence += step_confidence_boost(step);
        if (structured_artifact && is_exploratory_transform(step)) confidence -= 0.35;
        if (!structured_artifact && is_structured_transform(step)) confidence += 0.05;
    }

    score -= (step_count > 1) ? 0.06 * (step_count - 1) : 0.0;
    if (structured_artifact) {
        score -= 0.22;
    } else if (should_stop_on_readable_text(cand)) {
        score += 0.18;
    }
    score += confidence;
    return score;
}

static double step_confidence_boost(const char *step_name) {
    if (!step_name) return 0.0;

    if (g_strcmp0(step_name, "Binary") == 0) return 0.38;
    if (g_strcmp0(step_name, "Hex") == 0) return 0.32;
    if (g_strcmp0(step_name, "Base64") == 0) return 0.28;
    if (g_strcmp0(step_name, "Morse") == 0) return 0.24;
    if (g_strcmp0(step_name, "URL") == 0) return 0.14;
    if (g_strcmp0(step_name, "ROT13") == 0) return 0.06;
    if (g_strcmp0(step_name, "Caesar") == 0) return 0.04;
    if (g_strcmp0(step_name, "Atbash") == 0) return 0.04;
    if (g_strcmp0(step_name, "XOR") == 0) return -0.10;
    if (g_strcmp0(step_name, "Scramble") == 0) return -0.18;
    return 0.0;
}

static gboolean is_exploratory_transform(const char *step_name) {
    if (!step_name) return FALSE;
    return g_strcmp0(step_name, "ROT13") == 0 ||
           g_strcmp0(step_name, "Caesar") == 0 ||
           g_strcmp0(step_name, "Atbash") == 0 ||
           g_strcmp0(step_name, "XOR") == 0 ||
           g_strcmp0(step_name, "Scramble") == 0;
}

static gboolean is_structured_transform(const char *step_name) {
    if (!step_name) return FALSE;
    return g_strcmp0(step_name, "Binary") == 0 ||
           g_strcmp0(step_name, "Hex") == 0 ||
           g_strcmp0(step_name, "Base64") == 0 ||
           g_strcmp0(step_name, "Morse") == 0 ||
           g_strcmp0(step_name, "URL") == 0;
}

static gboolean has_structured_artifact(const Buffer *buf) {
    if (!buf || !buf->data || buf->len == 0) return FALSE;
    return is_binary_like(buf) || is_hex_like(buf) || is_base64_like(buf) || is_morse_like(buf) || is_url_like(buf);
}

static gboolean should_stop_on_readable_text(const Candidate *cand) {
    if (!cand || !cand->buf.data || cand->buf.len == 0) return FALSE;
    if (!is_alpha_text(&cand->buf)) return FALSE;
    return score_readability(cand->buf.data, cand->buf.len) >= 1.05;
}

static gboolean should_skip_plugin_for_candidate(const Candidate *cand, const Plugin *plugin, int depth) {
    if (!cand || !plugin) return TRUE;

    const char *last_step = candidate_last_step(cand);
    gboolean structured_artifact = has_structured_artifact(&cand->buf);

    if (g_strcmp0(last_step, plugin->name) == 0) return TRUE;
    if (should_stop_on_readable_text(cand) && is_exploratory_transform(plugin->name)) return TRUE;
    if (structured_artifact && is_exploratory_transform(plugin->name)) return TRUE;

    if (depth == 0 && g_strcmp0(last_step, "Input") == 0) {
        if (is_binary_like(&cand->buf)) return TRUE;
        if (is_hex_like(&cand->buf) && g_strcmp0(plugin->name, "Hex") != 0) return TRUE;
        if (is_morse_like(&cand->buf) && g_strcmp0(plugin->name, "Morse") != 0) return TRUE;
    }

    return FALSE;
}

static GList* create_input_variants(const Buffer *input) {
    GList *variants = NULL;

    Buffer *original = g_new0(Buffer, 1);
    *original = buffer_clone(input);
    variants = g_list_append(variants, original);

    GString *compact = g_string_sized_new(input->len);
    for (size_t i = 0; i < input->len; i++) {
        unsigned char c = input->data[i];
        if (!g_ascii_isspace(c)) g_string_append_c(compact, (char)c);
    }
    if (compact->len > 0 && compact->len != input->len) {
        Buffer *trimmed = g_new0(Buffer, 1);
        *trimmed = buffer_new((const unsigned char*)compact->str, compact->len);
        variants = g_list_append(variants, trimmed);
    }
    g_string_free(compact, TRUE);

    if (is_hex_like(input)) {
        GString *lower = g_string_sized_new(input->len);
        for (size_t i = 0; i < input->len; i++) {
            unsigned char c = input->data[i];
            if (!g_ascii_isspace(c)) g_string_append_c(lower, (char)g_ascii_tolower(c));
        }
        if (lower->len > 0) {
            Buffer *hex_lower = g_new0(Buffer, 1);
            *hex_lower = buffer_new((const unsigned char*)lower->str, lower->len);
            variants = g_list_append(variants, hex_lower);
        }
        g_string_free(lower, TRUE);
    }

    return variants;
}

static void buffer_box_free(Buffer *buf) {
    if (!buf) return;
    buffer_free(buf);
    g_free(buf);
}

static const char* candidate_last_step(const Candidate *cand) {
    GList *last = g_list_last(cand->steps);
    return last ? (const char*)last->data : NULL;
}

static Candidate* build_decoded_candidate(const Candidate *cand, const char *step_name, char *decoded) {
    if (!decoded || !decoded[0]) {
        g_free(decoded);
        return NULL;
    }

    Buffer next_buf = buffer_new((const unsigned char*)decoded, strlen(decoded));
    g_free(decoded);
    if (!next_buf.data || buffer_equal(&cand->buf, &next_buf)) {
        buffer_free(&next_buf);
        return NULL;
    }

    Candidate *next = g_new0(Candidate, 1);
    next->buf = next_buf;
    next->steps = g_list_copy_deep(cand->steps, (GCopyFunc)g_strdup, NULL);
    next->steps = g_list_append(next->steps, g_strdup(step_name));
    next->meta = g_strdup("");
    next->score = candidate_pipeline_score(next);
    return next;
}

static GList* generate_builtin_candidates(const Candidate *cand) {
    GList *produced = NULL;
    const char *last_step = candidate_last_step(cand);
    char *input = g_strndup((const char*)cand->buf.data, cand->buf.len);
    if (!input) return NULL;

    gboolean input_stage = (g_strcmp0(last_step, "Input") == 0);
    gboolean prefer_binary = input_stage && is_binary_like(&cand->buf);
    gboolean prefer_hex = input_stage && !prefer_binary && is_hex_like(&cand->buf);
    gboolean prefer_morse = input_stage && !prefer_binary && !prefer_hex && is_morse_like(&cand->buf);
    gboolean prefer_base64 = input_stage && !prefer_binary && !prefer_hex && !prefer_morse && is_base64_like(&cand->buf);

    if (prefer_binary) {
        Candidate *next = build_decoded_candidate(cand, "Binary", decode_binary(input));
        if (next) produced = g_list_append(produced, next);
        g_free(input);
        return produced;
    }

    if (prefer_hex) {
        Candidate *next = build_decoded_candidate(cand, "Hex", decode_hex(input));
        if (next) produced = g_list_append(produced, next);
        g_free(input);
        return produced;
    }

    if (prefer_morse) {
        Candidate *next = build_decoded_candidate(cand, "Morse", decode_morse(input));
        if (next) produced = g_list_append(produced, next);
        g_free(input);
        return produced;
    }

    if (prefer_base64) {
        Candidate *next = build_decoded_candidate(cand, "Base64", decode_base64(input));
        if (next) produced = g_list_append(produced, next);
        g_free(input);
        return produced;
    }

    if (g_strcmp0(last_step, "Binary") != 0 && is_binary(input, cand->buf.len)) {
        Candidate *next = build_decoded_candidate(cand, "Binary", decode_binary(input));
        if (next) produced = g_list_append(produced, next);
    }
    if (g_strcmp0(last_step, "Hex") != 0 && is_hex(input, cand->buf.len)) {
        Candidate *next = build_decoded_candidate(cand, "Hex", decode_hex(input));
        if (next) produced = g_list_append(produced, next);
    }
    if (g_strcmp0(last_step, "Base64") != 0 && is_base64(input, cand->buf.len)) {
        Candidate *next = build_decoded_candidate(cand, "Base64", decode_base64(input));
        if (next) produced = g_list_append(produced, next);
    }
    if (g_strcmp0(last_step, "Morse") != 0 && is_morse(input, cand->buf.len)) {
        Candidate *next = build_decoded_candidate(cand, "Morse", decode_morse(input));
        if (next) produced = g_list_append(produced, next);
    }

    g_free(input);
    return produced;
}

typedef struct {
    const Candidate *cand;
    Plugin *plugin;
    GMutex *results_mutex;
    GList **results;
} PipelineTask;

static void pipeline_task_free(PipelineTask *task) {
    g_free(task);
}

static void pipeline_executor_worker(gpointer data, gpointer user_data) {
    (void)user_data;
    PipelineTask *task = (PipelineTask*)data;
    const Candidate *cand = task->cand;
    Plugin *p = task->plugin;
    GList *results = NULL;
    GList *produced = NULL;

    if (p->decode_multi) {
        results = p->decode_multi(cand->buf);
    } else if (p->decode_single) {
        Buffer out = p->decode_single(cand->buf);
        if (out.data) {
            Buffer *boxed = g_new0(Buffer, 1);
            *boxed = out;
            results = g_list_append(NULL, boxed);
        }
    }

    for (GList *r = results; r; r = r->next) {
        Buffer *res = (Buffer*)r->data;
        if (!res->data || res->len == 0) {
            buffer_box_free(res);
            continue;
        }
        if (buffer_equal(&cand->buf, res)) {
            buffer_box_free(res);
            continue;
        }

        Candidate *next = g_new0(Candidate, 1);
        next->buf = *res;
        g_free(res);
        next->steps = g_list_copy_deep(cand->steps, (GCopyFunc)g_strdup, NULL);
        next->steps = g_list_append(next->steps, g_strdup(p->name));
        next->meta = g_strdup(p->meta_info ? p->meta_info : "");
        next->score = candidate_pipeline_score(next);
        produced = g_list_append(produced, next);
    }
    g_list_free(results);

    if (produced) {
        g_mutex_lock(task->results_mutex);
        *task->results = g_list_concat(*task->results, produced);
        g_mutex_unlock(task->results_mutex);
    }

    pipeline_task_free(task);
}

static GList* dedupe_candidate_results(GList *candidates, GHashTable *visited) {
    GList *deduped = NULL;
    for (GList *iter = candidates; iter; iter = iter->next) {
        Candidate *cand = (Candidate*)iter->data;
        char *key = buffer_key(&cand->buf);
        if (g_hash_table_contains(visited, key)) {
            g_free(key);
            candidate_free(cand);
            continue;
        }
        g_hash_table_add(visited, key);
        deduped = g_list_append(deduped, cand);
    }
    g_list_free(candidates);
    return deduped;
}

static GList* execute_planned_pipeline(const Buffer *input, const PipelinePlan *plan) {
    if (!input || !input->data || input->len == 0 || !plan) return NULL;

    GHashTable *visited = g_hash_table_new_full(g_str_hash, g_str_equal, g_free, NULL);
    GList *beam = NULL;

    Candidate *init = g_new0(Candidate, 1);
    init->buf = buffer_clone(input);
    init->score = score_readability(init->buf.data, init->buf.len);
    init->steps = g_list_append(NULL, g_strdup("Input"));
    init->meta = g_strdup("");
    beam = g_list_append(beam, init);
    g_hash_table_add(visited, buffer_key(&init->buf));

    for (int depth = 0; depth < plan->max_depth; depth++) {
        GList *new_beam = NULL;
        GMutex results_mutex;
        g_mutex_init(&results_mutex);
        GError *pool_error = NULL;
        GThreadPool *pool = g_thread_pool_new(pipeline_executor_worker, NULL, -1, FALSE, &pool_error);
        if (!pool_error && pool) {
            for (GList *iter = beam; iter; iter = iter->next) {
                Candidate *cand = (Candidate*)iter->data;
                GList *builtin = generate_builtin_candidates(cand);
                if (builtin) {
                    g_mutex_lock(&results_mutex);
                    new_beam = g_list_concat(new_beam, builtin);
                    g_mutex_unlock(&results_mutex);
                }

                for (GList *piter = plan->plugins; piter; piter = piter->next) {
                    PlannedPlugin *pp = (PlannedPlugin*)piter->data;
                    Plugin *p = pp->plugin;
                    if (p->detect && !p->detect(cand->buf) && pp->priority < 40) continue;
                    if (should_skip_plugin_for_candidate(cand, p, depth)) continue;

                    PipelineTask *task = g_new0(PipelineTask, 1);
                    task->cand = cand;
                    task->plugin = p;
                    task->results_mutex = &results_mutex;
                    task->results = &new_beam;
                    g_thread_pool_push(pool, task, NULL);
                }
            }
            g_thread_pool_free(pool, FALSE, TRUE);
        } else {
            if (pool_error) g_error_free(pool_error);

            for (GList *iter = beam; iter; iter = iter->next) {
                Candidate *cand = (Candidate*)iter->data;
                GList *builtin = generate_builtin_candidates(cand);
                if (builtin) {
                    new_beam = g_list_concat(new_beam, builtin);
                }

                for (GList *piter = plan->plugins; piter; piter = piter->next) {
                    PlannedPlugin *pp = (PlannedPlugin*)piter->data;
                    Plugin *p = pp->plugin;
                    if (p->detect && !p->detect(cand->buf) && pp->priority < 40) continue;
                    if (should_skip_plugin_for_candidate(cand, p, depth)) continue;

                    GList *results = NULL;
                    if (p->decode_multi) {
                        results = p->decode_multi(cand->buf);
                    } else if (p->decode_single) {
                        Buffer out = p->decode_single(cand->buf);
                        if (out.data) {
                            Buffer *boxed = g_new0(Buffer, 1);
                            *boxed = out;
                            results = g_list_append(NULL, boxed);
                        }
                    }

                    for (GList *r = results; r; r = r->next) {
                        Buffer *res = (Buffer*)r->data;
                        if (!res->data || res->len == 0) {
                            buffer_box_free(res);
                            continue;
                        }
                        if (buffer_equal(&cand->buf, res)) {
                            buffer_box_free(res);
                            continue;
                        }

                        Candidate *next = g_new0(Candidate, 1);
                        next->buf = *res;
                        g_free(res);
                        next->steps = g_list_copy_deep(cand->steps, (GCopyFunc)g_strdup, NULL);
                        next->steps = g_list_append(next->steps, g_strdup(p->name));
                        next->meta = g_strdup(p->meta_info ? p->meta_info : "");
                        next->score = candidate_pipeline_score(next);
                        new_beam = g_list_append(new_beam, next);
                    }
                    g_list_free(results);
                }
            }
        }
        g_mutex_clear(&results_mutex);

        new_beam = dedupe_candidate_results(new_beam, visited);

        if (!new_beam) break;

        new_beam = g_list_sort(new_beam, (GCompareFunc)compare_candidate);
        while ((int)g_list_length(new_beam) > plan->beam_width) {
            GList *last = g_list_last(new_beam);
            candidate_free((Candidate*)last->data);
            new_beam = g_list_delete_link(new_beam, last);
        }

        for (GList *iter = beam; iter; iter = iter->next) {
            candidate_free((Candidate*)iter->data);
        }
        g_list_free(beam);
        beam = new_beam;
    }

    g_hash_table_destroy(visited);
    return beam;
}

GList* pipeline_beam_search(Buffer input, int max_depth, int beam_width) {
    if (!input.data || input.len == 0) return NULL;

    PluginManager *pm = core_get_plugin_manager();
    if (!pm) return NULL;

    GHashTable *visited = g_hash_table_new_full(g_str_hash, g_str_equal, g_free, NULL);
    GList *beam = NULL;

    // Initial candidate
    Candidate *init = g_new0(Candidate, 1);
    init->buf.data = g_malloc(input.len);
    memcpy(init->buf.data, input.data, input.len);
    init->buf.len = input.len;
    init->score = score_readability(init->buf.data, init->buf.len);
    init->steps = g_list_append(NULL, g_strdup("Input"));
    init->meta = g_strdup("");
    beam = g_list_append(beam, init);

    char *key = buffer_key_raw(init->buf.data, init->buf.len);
    g_hash_table_add(visited, key);

    for (int depth = 0; depth < max_depth; depth++) {
        GList *new_beam = NULL;
        GList *plugins = plugin_manager_list(pm);

        for (GList *iter = beam; iter; iter = iter->next) {
            Candidate *cand = iter->data;

            for (GList *piter = plugins; piter; piter = piter->next) {
                Plugin *p = (Plugin*)piter->data;
                if (!p->decode_single && !p->decode_multi) continue;
                if (p->detect && !p->detect(cand->buf)) continue;

                GList *results = NULL;
                if (p->decode_multi) {
                    results = p->decode_multi(cand->buf);
                } else if (p->decode_single) {
                    Buffer out = p->decode_single(cand->buf);
                    if (out.data) {
                        Buffer *buf = g_new0(Buffer, 1);
                        *buf = out; // copy struct
                        results = g_list_append(NULL, buf);
                    }
                }

                for (GList *r = results; r; r = r->next) {
                    Buffer *res = (Buffer*)r->data;
                    if (!res->data || res->len == 0) {
                        if (res) g_free(res);
                        continue;
                    }

                    // Skip if unchanged
                    if (res->len == cand->buf.len &&
                        memcmp(res->data, cand->buf.data, res->len) == 0) {
                        g_free(res->data);
                        g_free(res);
                        continue;
                    }

                    char *key = buffer_key_raw(res->data, res->len);
                    if (g_hash_table_contains(visited, key)) {
                        g_free(key);
                        g_free(res->data);
                        g_free(res);
                        continue;
                    }
                    g_hash_table_add(visited, key);

                    Candidate *new_cand = g_new0(Candidate, 1);
                    new_cand->buf = *res; // take ownership
                    g_free(res);
                    new_cand->score = score_readability(new_cand->buf.data, new_cand->buf.len);
                    new_cand->steps = g_list_copy_deep(cand->steps, (GCopyFunc)g_strdup, NULL);
                    char *step_name = g_strdup_printf("%s%s", p->name,
                                                       p->meta_info ? p->meta_info : "");
                    new_cand->steps = g_list_append(new_cand->steps, step_name);
                    new_cand->meta = g_strdup(p->meta_info ? p->meta_info : "");

                    new_beam = g_list_append(new_beam, new_cand);
                }
                g_list_free(results);
            }
        }
        g_list_free(plugins);

        if (!new_beam) break;

        new_beam = g_list_sort(new_beam, (GCompareFunc)compare_candidate);
        int count = g_list_length(new_beam);
        while (count > beam_width) {
            GList *last = g_list_last(new_beam);
            candidate_free(last->data);
            new_beam = g_list_delete_link(new_beam, last);
            count--;
        }

        for (GList *iter = beam; iter; iter = iter->next)
            candidate_free(iter->data);
        g_list_free(beam);
        beam = new_beam;
    }

    g_hash_table_destroy(visited);
    return beam;
}

void candidate_list_free(GList *list) {
    for (GList *iter = list; iter; iter = iter->next)
        candidate_free(iter->data);
    g_list_free(list);
}

GList* pipeline_smart_search(Buffer input, int max_depth, int beam_width) {
    if (!input.data || input.len == 0) return NULL;

    PipelinePlan *plan = build_pipeline_plan(&input, max_depth, beam_width);
    if (!plan) return NULL;

    GList *variants = create_input_variants(&input);
    GList *all_candidates = NULL;

    int retries = 0;
    for (GList *iter = variants; iter && retries < plan->retry_count; iter = iter->next, retries++) {
        Buffer *variant = (Buffer*)iter->data;
        GList *beam = execute_planned_pipeline(variant, plan);
        for (GList *b = beam; b; b = b->next) {
            Candidate *cand = candidate_clone((Candidate*)b->data);
            all_candidates = g_list_append(all_candidates, cand);
        }
        candidate_list_free(beam);
    }

    g_list_free_full(variants, (GDestroyNotify)buffer_box_free);
    pipeline_plan_free(plan);

    if (!all_candidates) return NULL;

    all_candidates = g_list_sort(all_candidates, (GCompareFunc)compare_candidate);
    while ((int)g_list_length(all_candidates) > beam_width) {
        GList *last = g_list_last(all_candidates);
        candidate_free((Candidate*)last->data);
        all_candidates = g_list_delete_link(all_candidates, last);
    }
    return all_candidates;
}
