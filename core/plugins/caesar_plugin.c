#include <glib.h>
#include <string.h>
#include <ctype.h>
#include "../include/plugin.h"
#include "../include/score.h"
#include "../include/buffer.h"

#define DEFAULT_SHIFT 3
static int shift = DEFAULT_SHIFT;

static char* caesar_decode_str(const char *input) {
    if (!input) return g_strdup("");
    GString *result = g_string_sized_new(strlen(input));
    for (size_t i = 0; i < strlen(input); i++) {
        char c = input[i];
        if (c >= 'a' && c <= 'z')
            c = ((c - 'a' - shift + 26) % 26) + 'a';
        else if (c >= 'A' && c <= 'Z')
            c = ((c - 'A' - shift + 26) % 26) + 'A';
        else
            c = input[i];
        g_string_append_c(result, c);
    }
    return g_string_free(result, FALSE);
}

static char* caesar_encode_str(const char *input) {
    if (!input) return g_strdup("");
    GString *result = g_string_sized_new(strlen(input));
    for (size_t i = 0; i < strlen(input); i++) {
        char c = input[i];
        if (c >= 'a' && c <= 'z')
            c = ((c - 'a' + shift) % 26) + 'a';
        else if (c >= 'A' && c <= 'Z')
            c = ((c - 'A' + shift) % 26) + 'A';
        else
            c = input[i];
        g_string_append_c(result, c);
    }
    return g_string_free(result, FALSE);
}

static gboolean caesar_detect_str(const char *input) {
    if (!input || strlen(input) < 3) return FALSE;
    int original_shift = shift;
    for (int s = 1; s <= 25; s++) {
        shift = s;
        char *decoded = caesar_decode_str(input);
        int letter_count = 0;
        for (size_t i = 0; i < strlen(decoded) && i < 100; i++) {
            if (isalpha(decoded[i])) letter_count++;
        }
        g_free(decoded);
        if (letter_count > strlen(input) * 0.5) {
            shift = original_shift;
            return TRUE;
        }
    }
    shift = original_shift;
    return FALSE;
}

static Buffer caesar_decode_buffer(Buffer in) {
    Buffer out = { NULL, 0 };
    if (!in.data || in.len == 0) return out;
    char *input_str = g_malloc(in.len + 1);
    memcpy(input_str, in.data, in.len);
    input_str[in.len] = '\0';
    char *result_str = caesar_decode_str(input_str);
    g_free(input_str);
    if (result_str) {
        out.data = (unsigned char*)result_str;
        out.len = strlen(result_str);
    }
    return out;
}

static Buffer caesar_encode_buffer(Buffer in) {
    Buffer out = { NULL, 0 };
    if (!in.data || in.len == 0) return out;
    char *input_str = g_malloc(in.len + 1);
    memcpy(input_str, in.data, in.len);
    input_str[in.len] = '\0';
    char *result_str = caesar_encode_str(input_str);
    g_free(input_str);
    if (result_str) {
        out.data = (unsigned char*)result_str;
        out.len = strlen(result_str);
    }
    return out;
}

static gboolean caesar_detect_buffer(Buffer in) {
    if (in.len == 0) return FALSE;
    char *input_str = g_malloc(in.len + 1);
    memcpy(input_str, in.data, in.len);
    input_str[in.len] = '\0';
    gboolean res = caesar_detect_str(input_str);
    g_free(input_str);
    return res;
}

void caesar_plugin_init(void) {
    g_plugin_registry->register_plugin("Caesar",
                                        caesar_decode_buffer,
                                        NULL,
                                        caesar_encode_buffer,
                                        caesar_detect_buffer,
                                        60);
}