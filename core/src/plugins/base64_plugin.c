#include <glib.h>
#include <string.h>
#include "../include/plugin.h"
#include "../include/score.h"
#include "../include/buffer.h"

// --- String-based helpers ---
static char* base64_decode_str(const char *input) {
    gsize out_len;
    guchar *decoded = g_base64_decode(input, &out_len);
    char *result = g_strndup((char*)decoded, out_len);
    g_free(decoded);
    return result;
}

static char* base64_encode_str(const char *input) {
    return g_base64_encode((const guchar*)input, strlen(input));
}

static gboolean base64_detect_str(const char *input) {
    size_t len = strlen(input);
    if (len % 4 != 0) return FALSE;
    for (const char *p = input; *p; p++) {
        if (*p != ' ' && *p != '\n' && *p != '\r') {
            if (!((*p >= 'A' && *p <= 'Z') ||
                  (*p >= 'a' && *p <= 'z') ||
                  (*p >= '0' && *p <= '9') ||
                  *p == '+' || *p == '/' || *p == '='))
                return FALSE;
        }
    }
    if (len >= 2 && input[len-1] == '=' && input[len-2] != '=' && input[len-2] != '=')
        return FALSE;
    return TRUE;
}

// --- Buffer-based wrappers ---
static Buffer base64_decode_buffer(Buffer in) {
    Buffer out = { NULL, 0 };
    if (!in.data || in.len == 0) return out;
    char *input_str = g_malloc(in.len + 1);
    memcpy(input_str, in.data, in.len);
    input_str[in.len] = '\0';
    char *result_str = base64_decode_str(input_str);
    g_free(input_str);
    if (result_str) {
        out.data = (unsigned char*)result_str;
        out.len = strlen(result_str);
    }
    return out;
}

static Buffer base64_encode_buffer(Buffer in) {
    Buffer out = { NULL, 0 };
    if (!in.data || in.len == 0) return out;
    char *input_str = g_malloc(in.len + 1);
    memcpy(input_str, in.data, in.len);
    input_str[in.len] = '\0';
    char *result_str = base64_encode_str(input_str);
    g_free(input_str);
    if (result_str) {
        out.data = (unsigned char*)result_str;
        out.len = strlen(result_str);
    }
    return out;
}

static gboolean base64_detect_buffer(Buffer in) {
    if (in.len == 0) return FALSE;
    char *input_str = g_malloc(in.len + 1);
    memcpy(input_str, in.data, in.len);
    input_str[in.len] = '\0';
    gboolean res = base64_detect_str(input_str);
    g_free(input_str);
    return res;
}

void base64_plugin_init(void) {
    g_plugin_registry->register_plugin("Base64",
                                        base64_decode_buffer,
                                        NULL,
                                        base64_encode_buffer,
                                        base64_detect_buffer,
                                        100);
}