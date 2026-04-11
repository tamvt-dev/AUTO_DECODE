#include "../include/buffer.h"
#include <glib.h>
#include <string.h>

Buffer buffer_new(const unsigned char *data, size_t len) {
    Buffer buf = { NULL, 0 };
    if (!data || len == 0) return buf;
    // g_memdup2 is the modern, safe version of g_memdup
    buf.data = (unsigned char*)g_memdup2(data, len);
    buf.len = len;
    return buf;
}

Buffer buffer_clone(const Buffer *src) {
    if (!src || !src->data) return (Buffer){ NULL, 0 };
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
    if (a->data == b->data) return TRUE; // Fast path: same memory block
    if (a->len != b->len) return FALSE;
    if (a->len == 0) return TRUE;
    return memcmp(a->data, b->data, a->len) == 0;
}

guint32 buffer_hash_fast(const Buffer *buf) {
    if (!buf || !buf->data || buf->len == 0) return 0;
    
    // DJB2 Hash implementation
    guint32 hash = 5381;
    for (size_t i = 0; i < buf->len; i++) {
        hash = ((hash << 5) + hash) + buf->data[i];
    }
    return hash;
}

char* buffer_key(const Buffer *buf) {
    if (!buf || !buf->data || buf->len == 0) return g_strdup("");

    static const char hex_table[] = "0123456789abcdef";
    size_t len = buf->len;
    unsigned char *data = buf->data;
    
    // Optimization: limit the key length for extremely large buffers?
    // Maybe not, but let's keep it robust for the pipeline.
    char *out = (char*)g_malloc(len * 2 + 1);
    for (size_t i = 0; i < len; i++) {
        out[i*2]     = hex_table[(data[i] >> 4) & 0xF];
        out[i*2 + 1] = hex_table[data[i] & 0xF];
    }
    out[len * 2] = '\0';
    return out;
}