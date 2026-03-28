#ifndef CORE_H
#define CORE_H

#include <glib.h>
#include "errors.h"

typedef enum {
    CORE_MODE_AUTO,
    CORE_MODE_MORSE,
    CORE_MODE_BASE64,
    CORE_MODE_HEX,
    CORE_MODE_BINARY
} CoreMode;

typedef struct {
    char *output;
    char *format;
    char *input;
    CoreMode mode;
    ErrorCode error_code;
    char *error_message;
    double processing_time_ms;
    size_t input_size;
    size_t output_size;
    gboolean from_cache;
    int decode_depth;
} DecodeResult;

typedef struct {
    char *output;
    char *format;
    ErrorCode error_code;
    size_t input_size;
    size_t output_size;
} EncodeResult;

gboolean core_init(void);
void core_cleanup(void);

DecodeResult* core_decode(const char *input);
DecodeResult* core_decode_recursive(const char *input, int max_depth);

EncodeResult* core_encode(const char *input, CoreMode mode);

void core_cache_clear(void);
void core_cache_set_max_size(size_t size);

typedef struct {
    guint64 total_decodes;
    guint64 cache_hits;
    guint64 cache_misses;
    double avg_decode_time_ms;
    size_t total_input_bytes;
    size_t total_output_bytes;
    guint64 active_plugins;
} CoreStats;

CoreStats core_get_stats(void);
void core_reset_stats(void);

void decode_result_free(DecodeResult *result);
void encode_result_free(EncodeResult *result);

#endif /* CORE_H */