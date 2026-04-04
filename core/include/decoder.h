#ifndef DECODER_H
#define DECODER_H
#include <glib.h>
#ifdef __cplusplus
extern "C" {
#endif
gboolean is_morse(const char *text, size_t len);
gboolean is_base64(const char *text, size_t len);
gboolean is_hex(const char *text, size_t len);
gboolean is_binary(const char *text, size_t len);

char* decode_morse(const char *morse);
char* decode_base64(const char *input);
char* decode_hex(const char *input);
char* decode_binary(const char *input);
#ifdef __cplusplus
}
#endif
#endif /* DECODER_H */