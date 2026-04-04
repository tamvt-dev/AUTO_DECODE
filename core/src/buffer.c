#include "../include/buffer.h"
#include <glib.h>
#include <string.h>

Buffer buffer_new(const unsigned char *data, size_t len) {
    Buffer buf = { NULL, 0 };
    if (!data || len == 0) return buf;
    buf.data = g_memdup(data, len);
    buf.len = len;
    return buf;
}

Buffer buffer_clone(const Buffer *src) {
    return buffer_new(src->data, src->len);
}

void buffer_free(Buffer *buf) {
    if (buf) {
        g_free(buf->data);
        buf->data = NULL;
        buf->len = 0;
    }
}

gboolean buffer_equal(const Buffer *a, const Buffer *b) {
    if (!a || !b) return FALSE;
    if (a->len != b->len) return FALSE;
    if (a->len == 0) return TRUE;
    return memcmp(a->data, b->data, a->len) == 0;
}

char* buffer_key(const Buffer *buf) {
    static const char hex[] = "0123456789abcdef";
    char *out = g_malloc(buf->len * 2 + 1);
    for (size_t i = 0; i < buf->len; i++) {
        out[i*2]     = hex[(buf->data[i] >> 4) & 0xF];
        out[i*2 + 1] = hex[buf->data[i] & 0xF];
    }
    out[buf->len * 2] = '\0';
    return out;
}