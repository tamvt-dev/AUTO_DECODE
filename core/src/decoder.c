#include "../include/decoder.h"
#include "../include/logging.h"
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <glib.h>

/* Morse code table */
static const struct {
    const char *code;
    const char *character;
} morse_table[] = {
    {".-", "A"}, {"-...", "B"}, {"-.-.", "C"}, {"-..", "D"}, {".", "E"},
    {"..-.", "F"}, {"--.", "G"}, {"....", "H"}, {"..", "I"}, {".---", "J"},
    {"-.-", "K"}, {".-..", "L"}, {"--", "M"}, {"-.", "N"}, {"---", "O"},
    {".--.", "P"}, {"--.-", "Q"}, {".-.", "R"}, {"...", "S"}, {"-", "T"},
    {"..-", "U"}, {"...-", "V"}, {".--", "W"}, {"-..-", "X"}, {"-.--", "Y"},
    {"--..", "Z"},
    {"-----", "0"}, {".----", "1"}, {"..---", "2"}, {"...--", "3"}, {"....-", "4"},
    {".....", "5"}, {"-....", "6"}, {"--...", "7"}, {"---..", "8"}, {"----.", "9"},
    {".-.-.-", "."}, {"--..--", ","}, {"..--..", "?"}, {".----.", "'"}, {"-.-.--", "!"},
    {"-..-.", "/"}, {"-.--.", "("}, {"-.--.-", ")"}, {".-...", "&"}, {"---...", ":"},
    {"-.-.-.", ";"}, {"-...-", "="}, {".-.-.", "+"}, {"-....-", "-"}, {"..--.-", "_"},
    {".-..-.", "\""}, {"...-..-", "$"}, {".--.-.", "@"}, {NULL, NULL}
};

static GHashTable *morse_hash = NULL;

static void init_morse_hash(void) {
    if (!morse_hash) {
        morse_hash = g_hash_table_new_full(g_str_hash, g_str_equal, g_free, g_free);
        for (int i = 0; morse_table[i].code; i++) {
            g_hash_table_insert(morse_hash, 
                               g_strdup(morse_table[i].code),
                               g_strdup(morse_table[i].character));
        }
    }
}

gboolean is_morse(const char *text, size_t len) {
    if (!text || len == 0) return FALSE;
    
    const char *p = text;
    size_t valid = 0;
    size_t total = 0;
    
    while (*p && total < 100) {
        char c = *p;
        if (c == '.' || c == '-' || c == ' ' || c == '/') {
            valid++;
        } else if (c != '\n' && c != '\r') {
            return FALSE;
        }
        total++;
        p++;
    }
    
    return (valid > total * 0.7);
}

gboolean is_base64(const char *text, size_t len) {
    if (!text || len < 4) return FALSE;

    if (is_hex(text, len) || is_binary(text, len) || is_morse(text, len)) {
        return FALSE;
    }

    const char *p = text;
    size_t count = 0;
    size_t significant = 0;
    gboolean has_alpha = FALSE;
    gboolean has_padding_or_symbol = FALSE;

    while (*p && count < 256) {
        char c = *p;
        if (c != ' ' && c != '\n' && c != '\r') {
            if (!((c >= 'A' && c <= 'Z') ||
                  (c >= 'a' && c <= 'z') ||
                  (c >= '0' && c <= '9') ||
                  c == '+' || c == '/' || c == '=')) {
                return FALSE;
            }
            significant++;
            if ((c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z')) {
                has_alpha = TRUE;
            }
            if (c == '+' || c == '/' || c == '=') {
                has_padding_or_symbol = TRUE;
            }
        }
        p++;
        count++;
    }

    if (significant < 4) {
        return FALSE;
    }

    if (!has_alpha && !has_padding_or_symbol) {
        return FALSE;
    }

    return TRUE;
}

gboolean is_hex(const char *text, size_t len) {
    if (!text || len < 2) return FALSE;
    
    const char *p = text;
    if (p[0] == '0' && (p[1] == 'x' || p[1] == 'X')) {
        p += 2;
    }
    
    int hex_count = 0;
    while (*p && hex_count < 64) {
        char c = *p;
        if (c != ' ' && c != '\n' && c != '\r') {
            if (!((c >= '0' && c <= '9') ||
                  (c >= 'A' && c <= 'F') ||
                  (c >= 'a' && c <= 'f'))) {
                return FALSE;
            }
            hex_count++;
        }
        p++;
    }
    
    return (hex_count % 2 == 0 && hex_count > 0);
}

gboolean is_binary(const char *text, size_t len) {
    if (!text || len < 8) return FALSE;
    
    const char *p = text;
    int bit_count = 0;
    
    while (*p && bit_count < 128) {
        char c = *p;
        if (c != ' ' && c != '\n' && c != '\r') {
            if (c != '0' && c != '1') {
                return FALSE;
            }
            bit_count++;
        }
        p++;
    }
    
    return (bit_count % 8 == 0 && bit_count >= 8);
}

char* decode_morse(const char *morse) {
    if (!morse) return g_strdup("");
    
    init_morse_hash();
    
    GString *result = g_string_sized_new(strlen(morse) / 2);
    char *copy = g_alloca(strlen(morse) + 1);
    strcpy(copy, morse);
    
    char *saveptr1;
    char *word = strtok_r(copy, "/", &saveptr1);
    
    while (word) {
        while (*word == ' ') word++;
        
        char *saveptr2;
        char *code = strtok_r(word, " ", &saveptr2);
        
        while (code) {
            const char *character = g_hash_table_lookup(morse_hash, code);
            if (character) {
                g_string_append(result, character);
            } else {
                g_string_append_c(result, '?');
            }
            code = strtok_r(NULL, " ", &saveptr2);
        }
        
        g_string_append_c(result, ' ');
        word = strtok_r(NULL, "/", &saveptr1);
    }
    
    if (result->len > 0 && result->str[result->len - 1] == ' ') {
        g_string_truncate(result, result->len - 1);
    }
    
    return g_string_free(result, FALSE);
}

char* decode_base64(const char *input) {
    if (!input) return g_strdup("");
    
    GString *clean = g_string_new("");
    for (const char *p = input; *p; p++) {
        if (!isspace(*p)) {
            g_string_append_c(clean, *p);
        }
    }
    
    gsize out_len;
    guchar *decoded = g_base64_decode(clean->str, &out_len);
    g_string_free(clean, TRUE);
    
    if (!decoded) {
        return g_strdup("Error decoding Base64");
    }
    
    if (g_utf8_validate((const char*)decoded, out_len, NULL)) {
        char *result = g_strndup((char*)decoded, out_len);
        g_free(decoded);
        return result;
    }
    
    char *result = (char*)g_malloc(out_len * 2 + 1);
    for (size_t i = 0; i < out_len; i++) {
        sprintf(result + i * 2, "%02x", decoded[i]);
    }
    g_free(decoded);
    return result;
}

char* decode_hex(const char *input) {
    if (!input) return g_strdup("");
    
    const char *p = input;
    if (p[0] == '0' && (p[1] == 'x' || p[1] == 'X')) {
        p += 2;
    }
    
    size_t hex_count = 0;
    const char *q = p;
    while (*q) {
        if (!isspace(*q)) hex_count++;
        q++;
    }
    
    if (hex_count % 2 != 0) {
        return g_strdup("Error: Invalid hex length");
    }
    
    size_t out_len = hex_count / 2;
    guchar *decoded = (guchar*)g_malloc(out_len);
    
    size_t i = 0;
    while (*p && i < out_len) {
        while (*p && isspace(*p)) p++;
        if (!*p) break;
        
        char hex_byte[3] = {*p, *(p+1), 0};
        decoded[i] = (guchar)strtol(hex_byte, NULL, 16);
        i++;
        p += 2;
        
        while (*p && isspace(*p)) p++;
    }
    
    if (g_utf8_validate((const char*)decoded, out_len, NULL)) {
        char *result = g_strndup((char*)decoded, out_len);
        g_free(decoded);
        return result;
    }
    
    char *result = (char*)g_malloc(out_len * 2 + 1);
    for (size_t i = 0; i < out_len; i++) {
        sprintf(result + i * 2, "%02x", decoded[i]);
    }
    g_free(decoded);
    return result;
}

char* decode_binary(const char *input) {
    if (!input) return g_strdup("");
    
    size_t bit_count = 0;
    const char *p = input;
    while (*p) {
        if (*p == '0' || *p == '1') bit_count++;
        p++;
    }
    
    if (bit_count % 8 != 0) {
        return g_strdup("Error: Invalid binary length");
    }
    
    size_t out_len = bit_count / 8;
    guchar *decoded = (guchar*)g_malloc0(out_len);
    
    p = input;
    size_t byte_idx = 0;
    int bit_idx = 7;
    
    while (*p && byte_idx < out_len) {
        if (*p == '0' || *p == '1') {
            if (*p == '1') {
                decoded[byte_idx] |= (1 << bit_idx);
            }
            bit_idx--;
            
            if (bit_idx < 0) {
                bit_idx = 7;
                byte_idx++;
            }
        }
        p++;
    }
    
    if (g_utf8_validate((const char*)decoded, out_len, NULL)) {
        char *result = g_strndup((char*)decoded, out_len);
        g_free(decoded);
        return result;
    }
    
    char *result = (char*)g_malloc(out_len * 2 + 1);
    for (size_t i = 0; i < out_len; i++) {
        sprintf(result + i * 2, "%02x", decoded[i]);
    }
    g_free(decoded);
    return result;
}
