#include "../include/score.h"
#include <ctype.h>
#include <math.h>
#include <string.h>

// Portable memmem replacement
static gboolean contains_bytes(const unsigned char *data, size_t len,
                               const char *pattern, size_t plen) {
    if (plen > len) return FALSE;
    for (size_t i = 0; i <= len - plen; i++) {
        if (memcmp(data + i, pattern, plen) == 0)
            return TRUE;
    }
    return FALSE;
}

double score_readability(const unsigned char *data, size_t len) {
    if (len == 0) return 0.0;

    // 1. Printable ratio
    size_t printable = 0;
    for (size_t i = 0; i < len; i++) {
        if (isprint(data[i])) printable++;
    }
    double printable_score = (double)printable / len;

    // 2. Common word detection (case-insensitive)
    static const char *common_words[] = {
        "the", "and", "for", "are", "but", "not", "you", "all",
        "any", "can", "her", "was", "one", "our", "out", "day",
        "get", "has", "him", "his", "how", "man", "new", "now",
        "old", "see", "two", "way", "who", "boy", "did", "its",
        "let", "put", "say", "she", "too", "use"
    };
    size_t common_count = sizeof(common_words) / sizeof(common_words[0]);

    size_t word_match = 0;
    size_t total_words = 0;
    for (size_t i = 0; i < len; i++) {
        if (isalpha(data[i])) {
            size_t start = i;
            while (i < len && isalpha(data[i])) i++;
            size_t wlen = i - start;
            total_words++;
            for (size_t w = 0; w < common_count; w++) {
                size_t cwlen = strlen(common_words[w]);
                if (wlen == cwlen &&
                    strncasecmp((const char*)data + start, common_words[w], cwlen) == 0) {
                    word_match++;
                    break;
                }
            }
            i--;
        }
    }
    double word_score = (total_words > 0) ? (double)word_match / total_words : 0.0;

    // 3. Space ratio
    size_t spaces = 0;
    for (size_t i = 0; i < len; i++) {
        if (data[i] == ' ') spaces++;
    }
    double space_score = (double)spaces / len;

    // 4. Flag pattern bonus
    double flag_bonus = 0.0;
    if (contains_bytes(data, len, "flag{", 5) ||
        contains_bytes(data, len, "ctf{", 4)) {
        flag_bonus = 1.0;
    }

    // 5. Entropy (low entropy = more readable)
    double entropy = 0.0;
    int freq[256] = {0};
    for (size_t i = 0; i < len; i++) freq[data[i]]++;
    for (int i = 0; i < 256; i++) {
        if (freq[i]) {
            double p = (double)freq[i] / len;
            entropy -= p * log2(p);
        }
    }
    double entropy_score = (entropy < 5.0) ? 0.2 : 0.0;

    // 6. Character class balance
    int letters = 0, digits = 0, others = 0;
    for (size_t i = 0; i < len; i++) {
        if (isalpha(data[i])) letters++;
        else if (isdigit(data[i])) digits++;
        else others++;
    }
    double balance = (letters > 0 && others > 0) ? 0.1 : 0.0;

    // 7. Punctuation bonuses (for flags/structured text)
    double punct_bonus = 0.0;
    if (contains_bytes(data, len, "_", 1)) punct_bonus += 0.1;
    if (contains_bytes(data, len, "{", 1)) punct_bonus += 0.2;

    // 8. Bonus for JSON-like structure
    if (len > 2 && data[0] == '{' && data[len-1] == '}')
        punct_bonus += 0.5;

    // 9. Bonus for high printable ratio
    if (printable_score > 0.9)
        punct_bonus += 0.3;

    double score = 0.5 * printable_score +
                   0.3 * word_score +
                   0.2 * space_score +
                   flag_bonus +
                   entropy_score +
                   balance +
                   punct_bonus;

    if (score > 2.0) score = 2.0;
    return score;
}