#include <glib.h>
#include <string.h>
#include <ctype.h>
#include "../include/plugin.h"

static char* url_decode(const char *input) {
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

static char* url_encode(const char *input) {
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

static gboolean url_detect(const char *input) {
    return (strstr(input, "%20") != NULL ||
            strstr(input, "%2F") != NULL ||
            strstr(input, "%3A") != NULL);
}


void url_plugin_init(void) {
    g_plugin_registry->register_plugin("URL", url_decode, url_encode, url_detect, 90);
}