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

// Case-insensitive binary-safe search
static gboolean contains_bytes_nocase(const unsigned char *data, size_t len,
                                      const char *pattern, size_t plen) {
    if (plen > len) return FALSE;
    for (size_t i = 0; i <= len - plen; i++) {
        gboolean match = TRUE;
        for (size_t j = 0; j < plen; j++) {
            if (tolower(data[i + j]) != tolower((unsigned char)pattern[j])) {
                match = FALSE;
                break;
            }
        }
        if (match) return TRUE;
    }
    return FALSE;
}

double score_readability(const unsigned char *data, size_t len) {
    if (len == 0) return 0.0;

    // 1. Printable ratio and Run-length repetition penalty
    size_t printable = 0;
    size_t high_garbage = 0;
    size_t repeat_score_accum = 0;
    size_t current_run = 1;

    for (size_t i = 0; i < len; i++) {
        // ASCII Printable check
        if (data[i] >= 32 && data[i] <= 126) {
            printable++;
        } else if (data[i] > 126 || (data[i] < 32 && data[i] != '\n' && data[i] != '\r' && data[i] != '\t')) {
            high_garbage++;
        }
        
        // Advanced Run-length repetition penalty
        if (i > 0 && data[i] == data[i-1]) {
            current_run++;
            if (current_run >= 3) {
                // Exponential weight for longer runs (AAAAAA > AAA)
                repeat_score_accum += (current_run - 2); 
            }
        } else {
            current_run = 1;
        }
    }
    
    double printable_score = (double)printable / len;
    double garbage_penalty = (double)high_garbage / len;
    double repeat_penalty = (len > 0) ? (double)repeat_score_accum / len : 0.0;

    // 2. Common Word Detection
    static const char *common_words[] = {
        "the", "and", "for", "are", "but", "not", "you", "all",
        "any", "can", "her", "was", "one", "our", "out", "day",
        "get", "has", "him", "his", "how", "man", "new", "now",
        "old", "see", "two", "way", "who", "boy", "did", "its",
        "let", "put", "say", "she", "too", "use", "with", "have",
        "this", "will", "your", "from", "they", "been", "more",
        "when", "time", "very", "just", "know", "take", "into",
        "year", "some", "than", "them", "only", "come", "make",
        "over", "such", "back", "after", "work", "first", "well",
        "hello", "world", "secret", "flag", "root", "admin", "password",
        "user", "login", "data", "test", "null", "true", "false"
    };
    size_t word_match = 0;
    size_t total_words = 0;
    for (size_t i = 0; i < len; i++) {
        if (isalpha(data[i])) {
            size_t start = i;
            while (i < len && isalpha(data[i])) i++;
            size_t wlen = i - start;
            total_words++;
            for (size_t w = 0; w < sizeof(common_words)/sizeof(char*); w++) {
                if (wlen == strlen(common_words[w]) &&
                    strncasecmp((const char*)data + start, common_words[w], wlen) == 0) {
                    word_match++;
                    break;
                }
            }
            i--;
        }
    }
    double word_score = (total_words > 0) ? (double)word_match / total_words : 0.0;

    // 2b. Trigram Scoring (Top CTF/English patterns) - Normalized
    static const char *common_trigrams[] = { "the", "and", "ing", "her", "hat", "his", "tha", "ere", "for", "ent" };
    size_t trigram_matches = 0;
    for (size_t i = 0; i + 2 < len; i++) {
        char tg[4] = { tolower(data[i]), tolower(data[i+1]), tolower(data[i+2]), '\0' };
        for (size_t t = 0; t < 10; t++) {
            if (strcmp(tg, common_trigrams[t]) == 0) { trigram_matches++; break; }
        }
    }
    double trigram_score = (len > 5) ? (double)trigram_matches / (len - 2) : 0.0;
    if (trigram_score > 0.5) trigram_score = 0.5;

    // 2c. Optimized Bigram Scoring (Fast path via 2D Table)
    static char bigram_table[256][256] = {0};
    static gboolean bigram_init = FALSE;
    if (!bigram_init) {
        const char *bg[] = {"th", "he", "in", "er", "an", "re", "on", "at", "en", "nd", "ti", "es", "or", "te", "of", "ed", "is", "it", "al", "ar"};
        for (size_t b = 0; b < 20; b++) bigram_table[(unsigned char)bg[b][0]][(unsigned char)bg[b][1]] = 1;
        bigram_init = TRUE;
    }
    size_t bigram_matches = 0;
    size_t total_alpha_pairs = 0;
    for (size_t i = 0; i + 1 < len; i++) {
        if (isalpha(data[i]) && isalpha(data[i+1])) {
            total_alpha_pairs++;
            if (bigram_table[tolower(data[i])][tolower(data[i+1])]) bigram_matches++;
        }
    }
    double bigram_score = (total_alpha_pairs > 0) ? (double)bigram_matches / total_alpha_pairs : 0.0;

    // 3. Flags and Space
    double space_score = 0.0;
    size_t spaces = 0;
    for (size_t i = 0; i < len; i++) if (data[i] == ' ') spaces++;
    space_score = (double)spaces / len;

    double flag_bonus = 0.0;
    static const char *flags[] = { "flag{", "ctf{", "key{", "root{", "htbs{" };
    for (size_t f = 0; f < 5; f++) {
        if (contains_bytes_nocase(data, len, flags[f], strlen(flags[f]))) { flag_bonus = 1.0; break; }
    }

    // 4. Entropy "Sweet Spot" (English text ~4.5 bits)
    double entropy = 0.0;
    int freq[256] = {0};
    for (size_t i = 0; i < len; i++) freq[data[i]]++;
    for (int i = 0; i < 256; i++) {
        if (freq[i]) {
            double p = (double)freq[i] / len;
            entropy -= p * log2(p) / log2(2);
        }
    }
    // High score for entropy in [3.0, 5.0], penalized if > 5.5 (random) or < 2.5 (hex-like/binary)
    double entropy_score = 0.0;
    if (entropy >= 3.0 && entropy <= 5.0) entropy_score = 0.5;
    else if (entropy < 2.0 || entropy > 6.5) entropy_score = -0.5;

    // 5. Letter vs Digit Ratio
    size_t vc = 0, cc = 0, digits = 0;
    for (size_t i = 0; i < len; i++) {
        if (isdigit(data[i])) digits++;
        if (isalpha(data[i])) {
            char c = tolower(data[i]);
            if (strchr("aeiou", c)) vc++; else cc++;
        }
    }
    double letter_ratio = (len > 0) ? (double)(vc + cc) / len : 0.0;
    double digit_ratio = (len > 0) ? (double)digits / len : 0.0;

    double vowel_score = 0;
    if (vc + cc > 0) {
        double vr = (double)vc / (vc + cc);
        if (vr >= 0.3 && vr <= 0.5) vowel_score = 0.25;
    }

    // Final Aggregate
    double score = 0.30 * printable_score +
                   0.40 * word_score +
                   0.30 * bigram_score +
                   0.20 * trigram_score +
                   0.10 * space_score +
                   0.20 * entropy_score +
                   0.20 * letter_ratio -
                   0.20 * digit_ratio +
                   vowel_score +
                   flag_bonus -
                   (3.0 * garbage_penalty) -
                   (2.0 * repeat_penalty);

    // length normalization: naturally reward slightly longer text over noise like "A", "85c"
    if (len < 4) score *= 0.5;
    else if (len > 20) score += 0.1;

    if (score < 0.0) score = 0.0;
    if (score > 2.0) score = 2.0;
    return score;
}