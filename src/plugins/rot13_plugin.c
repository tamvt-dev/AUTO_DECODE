#include <glib.h>
#include <string.h>
#include <ctype.h>
#include "../include/plugin.h"

static char* rot13_decode(const char *input) {
    if (!input) return g_strdup("");
    GString *result = g_string_sized_new(strlen(input));
    for (size_t i = 0; i < strlen(input); i++) {
        char c = input[i];
        if (c >= 'a' && c <= 'z')
            c = ((c - 'a' + 13) % 26) + 'a';
        else if (c >= 'A' && c <= 'Z')
            c = ((c - 'A' + 13) % 26) + 'A';
        g_string_append_c(result, c);
    }
    return g_string_free(result, FALSE);
}

static char* rot13_encode(const char *input) {
    return rot13_decode(input);
}

static gboolean rot13_detect(const char *input) {
    if (!input || strlen(input) < 3) return FALSE;
    char *decoded = rot13_decode(input);
    int letter_count = 0;
    for (size_t i = 0; i < strlen(decoded) && i < 100; i++) {
        if (isalpha(decoded[i])) letter_count++;
    }
    gboolean result = (letter_count > strlen(decoded) * 0.3);
    g_free(decoded);
    return result;
}

void rot13_plugin_init(void) {
    g_plugin_registry->register_plugin("ROT13", rot13_decode, rot13_encode, rot13_detect, 80);
}
