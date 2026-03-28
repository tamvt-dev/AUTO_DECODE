#include <glib.h>
#include <string.h>
#include "../include/plugin.h"

static char* base64_decode(const char *input) {
    gsize out_len;
    guchar *decoded = g_base64_decode(input, &out_len);
    char *result = g_strndup((char*)decoded, out_len);
    g_free(decoded);
    return result;
}

static char* base64_encode(const char *input) {
    return g_base64_encode((const guchar*)input, strlen(input));
}

static gboolean base64_detect(const char *input) {
    for (const char *p = input; *p; p++) {
        if (*p != ' ' && *p != '\n' && *p != '\r') {
            if (!((*p >= 'A' && *p <= 'Z') ||
                  (*p >= 'a' && *p <= 'z') ||
                  (*p >= '0' && *p <= '9') ||
                  *p == '+' || *p == '/' || *p == '=')) {
                return FALSE;
            }
        }
    }
    return strlen(input) >= 4;
}

void plugin_init(void) {
    g_plugin_registry->register_plugin("base64", 
                                        base64_decode, 
                                        base64_encode, 
                                        base64_detect,
                                        100);
}

const char* plugin_version = "1.0.0";
const char* plugin_author = "Auto Decoder Team";