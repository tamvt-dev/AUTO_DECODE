#include "../include/encoder.h"
#include <string.h>
#include <ctype.h>
#include <glib.h>

static const struct {
    char character;
    const char *code;
} reverse_morse[] = {
    {'A', ".-"}, {'B', "-..."}, {'C', "-.-."}, {'D', "-.."}, {'E', "."},
    {'F', "..-."}, {'G', "--."}, {'H', "...."}, {'I', ".."}, {'J', ".---"},
    {'K', "-.-"}, {'L', ".-.."}, {'M', "--"}, {'N', "-."}, {'O', "---"},
    {'P', ".--."}, {'Q', "--.-"}, {'R', ".-."}, {'S', "..."}, {'T', "-"},
    {'U', "..-"}, {'V', "...-"}, {'W', ".--"}, {'X', "-..-"}, {'Y', "-.--"},
    {'Z', "--.."},
    {'0', "-----"}, {'1', ".----"}, {'2', "..---"}, {'3', "...--"}, {'4', "....-"},
    {'5', "....."}, {'6', "-...."}, {'7', "--..."}, {'8', "---.."}, {'9', "----."},
    {'.', ".-.-.-"}, {',', "--..--"}, {'?', "..--.."}, {'\'', ".----."}, {'!', "-.-.--"},
    {'/', "-..-."}, {'(', "-.--."}, {')', "-.--.-"}, {'&', ".-..."}, {':', "---..."},
    {';', "-.-.-."}, {'=', "-...-"}, {'+', ".-.-."}, {'-', "-....-"}, {'_', "..--.-"},
    {'"', ".-..-."}, {'$', "...-..-"}, {'@', ".--.-."}, {' ', "/"}
};

char* encode_morse(const char *text) {
    if (!text) return g_strdup("");
    
    GString *result = g_string_new("");
    GString *word = g_string_new("");
    
    for (size_t i = 0; i <= strlen(text); i++) {
        char c = toupper(text[i]);
        
        if (c == ' ' || c == '\0') {
            if (word->len > 0) {
                for (size_t j = 0; j < word->len; j++) {
                    char ch = word->str[j];
                    for (size_t k = 0; k < G_N_ELEMENTS(reverse_morse); k++) {
                        if (ch == reverse_morse[k].character) {
                            if (j > 0) g_string_append_c(result, ' ');
                            g_string_append(result, reverse_morse[k].code);
                            break;
                        }
                    }
                }
                g_string_append(result, " / ");
                g_string_truncate(word, 0);
            }
        } else {
            g_string_append_c(word, c);
        }
    }
    
    if (result->len >= 3 && 
        result->str[result->len-3] == ' ' &&
        result->str[result->len-2] == '/' &&
        result->str[result->len-1] == ' ') {
        g_string_truncate(result, result->len - 3);
    }
    
    g_string_free(word, TRUE);
    return g_string_free(result, FALSE);
}

char* encode_base64(const char *text) {
    if (!text) return g_strdup("");
    return g_base64_encode((const guchar*)text, strlen(text));
}

char* encode_hex(const char *text) {
    if (!text) return g_strdup("");
    
    GString *result = g_string_sized_new(strlen(text) * 2);
    for (size_t i = 0; i < strlen(text); i++) {
        g_string_append_printf(result, "%02x", (unsigned char)text[i]);
    }
    return g_string_free(result, FALSE);
}

char* encode_binary(const char *text) {
    if (!text) return g_strdup("");
    
    GString *result = g_string_sized_new(strlen(text) * 9);
    for (size_t i = 0; i < strlen(text); i++) {
        for (int j = 7; j >= 0; j--) {
            g_string_append_c(result, ((text[i] >> j) & 1) ? '1' : '0');
        }
        if (i < strlen(text) - 1) {
            g_string_append_c(result, ' ');
        }
    }
    return g_string_free(result, FALSE);
}

char* encode_text(const char *text, EncodeType type) {
    switch (type) {
        case ENCODE_MORSE: return encode_morse(text);
        case ENCODE_BASE64: return encode_base64(text);
        case ENCODE_HEX: return encode_hex(text);
        case ENCODE_BINARY: return encode_binary(text);
        default: return g_strdup("");
    }
}