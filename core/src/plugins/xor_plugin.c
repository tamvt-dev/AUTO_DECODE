#include <glib.h>
#include <string.h>
#include <ctype.h>
#include <math.h>
#include "../include/plugin.h"
#include "../include/score.h"
#include "../include/buffer.h"

// -------------------------------------------------------------------
// Core XOR functions
// -------------------------------------------------------------------
static Buffer xor_encode(Buffer in) {
    Buffer out = { NULL, 0 };
    if (!in.data || in.len == 0) return out;
    out.data = g_memdup(in.data, in.len);
    out.len = in.len;
    return out;
}

static Buffer xor_decode_single(Buffer in) {
    return xor_encode(in); // default key 0
}

// -------------------------------------------------------------------
// Candidate sorting helper (must be outside any function)
// -------------------------------------------------------------------
typedef struct {
    Buffer buf;
    double score;
    int key;
} XorCandidate;

static int cmp_candidate(const void *a, const void *b) {
    const XorCandidate *ca = (const XorCandidate*)a;
    const XorCandidate *cb = (const XorCandidate*)b;
    if (cb->score > ca->score) return 1;
    if (cb->score < ca->score) return -1;
    return 0;
}

// -------------------------------------------------------------------
// Multi-output: hybrid – fast keys first, then full scan if needed
// -------------------------------------------------------------------
static GList* xor_decode_multi(Buffer in) {
    GList *results = NULL;
    if (!in.data || in.len == 0) return NULL;

    const int fast_keys[] = {0x20, 0x41, 0x61, 0x00};
    const int num_fast = sizeof(fast_keys) / sizeof(fast_keys[0]);

    XorCandidate *cands = NULL;
    int cap = 0;
    int valid = 0;

    // Phase 1: fast keys
    for (int i = 0; i < num_fast; i++) {
        int key = fast_keys[i];
        Buffer *buf = g_new0(Buffer, 1);
        buf->len = in.len;
        buf->data = g_malloc(in.len);
        for (size_t j = 0; j < in.len; j++)
            buf->data[j] = in.data[j] ^ key;

        double score = score_readability(buf->data, buf->len);
        if (score > 0.3) {
            if (valid >= cap) {
                cap = cap ? cap * 2 : 16;
                cands = g_realloc(cands, sizeof(XorCandidate) * cap);
            }
            cands[valid].buf = *buf;
            cands[valid].score = score;
            cands[valid].key = key;
            valid++;
            g_free(buf); // only free wrapper, data now owned by cands
        } else {
            g_free(buf->data);
            g_free(buf);
        }
    }

    // Phase 2: fallback if nothing good found
    if (valid == 0) {
        for (int key = 0; key < 256; key++) {
            // skip keys already tested in fast phase
            gboolean already = FALSE;
            for (int i = 0; i < num_fast; i++) if (fast_keys[i] == key) { already = TRUE; break; }
            if (already) continue;

            Buffer *buf = g_new0(Buffer, 1);
            buf->len = in.len;
            buf->data = g_malloc(in.len);
            for (size_t j = 0; j < in.len; j++)
                buf->data[j] = in.data[j] ^ key;

            // Quick early prune: first byte should be printable
            if (!isprint(buf->data[0]) && !isprint(buf->data[1])) {
                g_free(buf->data);
                g_free(buf);
                continue;
            }

            double score = score_readability(buf->data, buf->len);
            if (score > 0.3) {
                if (valid >= cap) {
                    cap = cap ? cap * 2 : 256;
                    cands = g_realloc(cands, sizeof(XorCandidate) * cap);
                }
                cands[valid].buf = *buf;
                cands[valid].score = score;
                cands[valid].key = key;
                valid++;
                g_free(buf);
            } else {
                g_free(buf->data);
                g_free(buf);
            }
        }
    }

    if (valid == 0) {
        g_free(cands);
        return NULL;
    }

    // Sort by score descending
    qsort(cands, valid, sizeof(XorCandidate), cmp_candidate);

    // Keep top 5
    int keep = (valid < 5) ? valid : 5;
    for (int i = 0; i < keep; i++) {
        Buffer *buf = g_new0(Buffer, 1);
        buf->data = cands[i].buf.data;
        buf->len = cands[i].buf.len;
        results = g_list_append(results, buf);
    }

    // Free unused candidates
    for (int i = keep; i < valid; i++) {
        g_free(cands[i].buf.data);
    }
    g_free(cands);
    return results;
}

// -------------------------------------------------------------------
// Detection: try fast keys only
// -------------------------------------------------------------------
static gboolean xor_detect(Buffer in) {
    const int test_keys[] = {0x20, 0x41, 0x61, 0x00};
    for (int i = 0; i < 4; i++) {
        int key = test_keys[i];
        unsigned char *data = g_malloc(in.len);
        for (size_t j = 0; j < in.len; j++)
            data[j] = in.data[j] ^ key;
        double score = score_readability(data, in.len);
        g_free(data);
        if (score > 0.6) return TRUE;
    }
    return FALSE;
}

// -------------------------------------------------------------------
// Plugin registration
// -------------------------------------------------------------------
void xor_plugin_init(void) {
    g_plugin_registry->register_plugin("XOR",
                                       xor_decode_single,
                                       xor_decode_multi,
                                       xor_encode,
                                       xor_detect,
                                       70);
}