#include "../include/errors.h"
#include <stdio.h>
#include <stdarg.h>
#include <string.h>

Error* error_new(ErrorCode code, const char *format, ...) {
    Error *error = g_new0(Error, 1);
    error->code = code;
    
    va_list args;
    va_start(args, format);
    error->message = g_strdup_vprintf(format, args);
    va_end(args);
    
    return error;
}

void error_free(Error *error) {
    if (error) {
        g_free(error->message);
        g_free(error->file);
        g_free(error->function);
        g_free(error);
    }
}

const char* error_get_message(ErrorCode code) {
    switch (code) {
        case ERROR_OK: return "Success";
        case ERROR_INVALID_INPUT: return "Invalid input";
        case ERROR_UNKNOWN_FORMAT: return "Unknown format";
        case ERROR_DECODE_FAILED: return "Decode failed";
        case ERROR_ENCODE_FAILED: return "Encode failed";
        case ERROR_FILE_NOT_FOUND: return "File not found";
        case ERROR_OUT_OF_MEMORY: return "Out of memory";
        case ERROR_TIMEOUT: return "Operation timeout";
        case ERROR_CANCELLED: return "Cancelled by user";
        default: return "Unknown error";
    }
}