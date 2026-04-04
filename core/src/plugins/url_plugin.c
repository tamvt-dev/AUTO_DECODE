#include <glib.h>
#include <string.h>
#include <ctype.h>
#include "../include/plugin.h"
#include "../include/score.h"
#include "../include/buffer.h"

static char* url_decode_str(const char *input) {
    if (!input) return g_strdup("");
    GString *result = g_string_sized_new(strlen(input));
    for (size_t i = 0; i < strlen(input); i++) {
        if (input[i] == '%' && i + 2 < strlen(input)) {
            char hex[3] = {input[i+1], input[i+2], 0};
            int value = strtol(hex, NULL, 16);
            g_string_append_c(result, (char)value);
            i += 2;
        } else if (input[i] == '+') {
            g_string_append_c(result, ' ');
        } else {
            g_string_append_c(result, input[i]);
        }
    }
    return g_string_free(result, FALSE);
}

static char* url_encode_str(const char *input) {
    if (!input) return g_strdup("");
    GString *result = g_string_sized_new(strlen(input) * 3);
    for (size_t i = 0; i < strlen(input); i++) {
        char c = input[i];
        if (isalnum(c) || c == '-' || c == '_' || c == '.' || c == '~') {
            g_string_append_c(result, c);
        } else if (c == ' ') {
            g_string_append_c(result, '+');
        } else {
            g_string_append_printf(result, "%%%02X", (unsigned char)c);
        }
    }
    return g_string_free(result, FALSE);
}

static gboolean url_detect_str(const char *input) {
    return (strstr(input, "%20") != NULL ||
            strstr(input, "%2F") != NULL ||
            strstr(input, "%3A") != NULL);
}

static Buffer url_decode_buffer(Buffer in) {
    Buffer out = { NULL, 0 };
    if (!in.data || in.len == 0) return out;
    char *input_str = g_malloc(in.len + 1);
    memcpy(input_str, in.data, in.len);
    input_str[in.len] = '\0';
    char *result_str = url_decode_str(input_str);
    g_free(input_str);
    if (result_str) {
        out.data = (unsigned char*)result_str;
        out.len = strlen(result_str);
    }
    return out;
}

static Buffer url_encode_buffer(Buffer in) {
    Buffer out = { NULL, 0 };
    if (!in.data || in.len == 0) return out;
    char *input_str = g_malloc(in.len + 1);
    memcpy(input_str, in.data, in.len);
    input_str[in.len] = '\0';
    char *result_str = url_encode_str(input_str);
    g_free(input_str);
    if (result_str) {
        out.data = (unsigned char*)result_str;
        out.len = strlen(result_str);
    }
    return out;
}

static gboolean url_detect_buffer(Buffer in) {
    if (in.len == 0) return FALSE;
    char *input_str = g_malloc(in.len + 1);
    memcpy(input_str, in.data, in.len);
    input_str[in.len] = '\0';
    gboolean res = url_detect_str(input_str);
    g_free(input_str);
    return res;
}

void url_plugin_init(void) {
    g_plugin_registry->register_plugin("URL",
                                        url_decode_buffer,
                                        NULL,
                                        url_encode_buffer,
                                        url_detect_buffer,
                                        90);
}