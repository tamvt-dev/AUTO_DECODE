#include <glib.h>
#include <string.h>
#include <ctype.h>
#include "../include/plugin.h"

#define DEFAULT_SHIFT 3
static int shift = DEFAULT_SHIFT;

static char* caesar_decode(const char *input) {
    if (!input) return g_strdup("");
    GString *res = g_string_sized_new(strlen(input));
    for (size_t i = 0; i < strlen(input); i++) {
        char c = input[i];
        if (c >= 'a' && c <= 'z') c = ((c - 'a' - shift + 26) % 26) + 'a';
        else if (c >= 'A' && c <= 'Z') c = ((c - 'A' - shift + 26) % 26) + 'A';
        else c = input[i];
        g_string_append_c(res, c);
    }
    return g_string_free(res, FALSE);
}

static char* caesar_encode(const char *input) {
    if (!input) return g_strdup("");
    GString *res = g_string_sized_new(strlen(input));
    for (size_t i = 0; i < strlen(input); i++) {
        char c = input[i];
        if (c >= 'a' && c <= 'z') c = ((c - 'a' + shift) % 26) + 'a';
        else if (c >= 'A' && c <= 'Z') c = ((c - 'A' + shift) % 26) + 'A';
        else c = input[i];
        g_string_append_c(res, c);
    }
    return g_string_free(res, FALSE);
}

static gboolean caesar_detect(const char *input) {
    if (!input || strlen(input) < 3) return FALSE;
    int orig = shift;
    for (int s = 1; s <= 25; s++) {
        shift = s;
        char *decoded = caesar_decode(input);
        int letters = 0;
        for (size_t i = 0; i < strlen(decoded) && i < 100; i++) if (isalpha(decoded[i])) letters++;
        g_free(decoded);
        if (letters > strlen(input) * 0.5) {
            shift = orig;
            return TRUE;
        }
    }
    shift = orig;
    return FALSE;
}

void caesar_plugin_init(void) {
    g_plugin_registry->register_plugin("Caesar", caesar_decode, caesar_encode, caesar_detect, 60);
}
