#include <glib.h>
#include <string.h>
#include <ctype.h>
#include "../include/plugin.h"

static char* atbash_decode(const char *input) {
    if (!input) return g_strdup("");
    GString *res = g_string_sized_new(strlen(input));
    for (size_t i = 0; i < strlen(input); i++) {
        char c = input[i];
        if (c >= 'a' && c <= 'z') c = 'z' - (c - 'a');
        else if (c >= 'A' && c <= 'Z') c = 'Z' - (c - 'A');
        g_string_append_c(res, c);
    }
    return g_string_free(res, FALSE);
}

static char* atbash_encode(const char *input) {
    return atbash_decode(input);
}

static gboolean atbash_detect(const char *input) {
    if (!input || strlen(input) < 3) return FALSE;
    char *decoded = atbash_decode(input);
    int letters = 0;
    for (size_t i = 0; i < strlen(decoded) && i < 100; i++) if (isalpha(decoded[i])) letters++;
    gboolean ok = (letters > strlen(decoded) * 0.3);
    g_free(decoded);
    return ok;
}

void atbash_plugin_init(void) {
    g_plugin_registry->register_plugin("Atbash", atbash_decode, atbash_encode, atbash_detect, 70);
}
