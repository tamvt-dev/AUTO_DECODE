#ifndef ENCODER_H
#define ENCODER_H
#include <glib.h>
#ifdef __cplusplus
extern "C" {
#endif
typedef enum {
    ENCODE_MORSE,
    ENCODE_BASE64,
    ENCODE_HEX,
    ENCODE_BINARY
} EncodeType;

char* encode_morse(const char *text);
char* encode_base64(const char *text);
char* encode_hex(const char *text);
char* encode_binary(const char *text);
char* encode_text(const char *text, EncodeType type);

#ifdef __cplusplus
}
#endif

#endif /* ENCODER_H */