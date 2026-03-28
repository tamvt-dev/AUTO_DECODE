#ifndef ERRORS_H
#define ERRORS_H

#include <glib.h>

typedef enum {
    ERROR_OK = 0,
    ERROR_GENERIC = 1,
    ERROR_INVALID_INPUT = 100,
    ERROR_UNKNOWN_FORMAT = 101,
    ERROR_INVALID_ENCODING = 102,
    ERROR_DECODE_FAILED = 200,
    ERROR_ENCODE_FAILED = 201,
    ERROR_FILE_NOT_FOUND = 300,
    ERROR_FILE_READ = 301,
    ERROR_FILE_WRITE = 302,
    ERROR_OUT_OF_MEMORY = 400,
    ERROR_THREAD_CREATE = 500,
    ERROR_TIMEOUT = 600,
    ERROR_CANCELLED = 601,
    ERROR_PLUGIN_LOAD = 700,
    ERROR_PLUGIN_INIT = 701
} ErrorCode;

typedef struct Error {
    ErrorCode code;
    char *message;
    char *file;
    int line;
    char *function;
} Error;

Error* error_new(ErrorCode code, const char *format, ...) G_GNUC_PRINTF(2, 3);
void error_free(Error *error);
const char* error_get_message(ErrorCode code);

#endif /* ERRORS_H */