#ifndef BUFFER_H
#define BUFFER_H
#include <glib.h>
#ifdef __cplusplus
extern "C" {
#endif
typedef struct {
    unsigned char *data;
    size_t len;
} Buffer;

Buffer buffer_new(const unsigned char *data, size_t len);
Buffer buffer_clone(const Buffer *src);
void buffer_free(Buffer *buf);
gboolean buffer_equal(const Buffer *a, const Buffer *b);
char* buffer_key(const Buffer *buf);
#ifdef __cplusplus
}
#endif
#endif // BUFFER_H