#include "../include/plugin.h"
#include "../include/logging.h"
#include <zlib.h>
#include <glib.h>

static gboolean gzip_detect(Buffer in) {
    return in.len > 10 && in.data[0] == 0x1f && in.data[1] == 0x8b;
}

static Buffer gzip_decode(Buffer in) {
    z_stream strm;
    strm.zalloc = Z_NULL;
    strm.zfree = Z_NULL;
    strm.opaque = Z_NULL;
    strm.avail_in = 0;
    strm.next_in = Z_NULL;
    inflateInit2(&strm, 15 + 32); // auto-detect window

    strm.avail_in = in.len;
    strm.next_in = in.data;
    unsigned char outbuf[8192];
    unsigned long total_out = 0;

    int ret;
    do {
        strm.avail_out = sizeof(outbuf);
        strm.next_out = outbuf;
        ret = inflate(&strm, Z_NO_FLUSH);
        if (ret != Z_OK && ret != Z_STREAM_END) break;

        // Copy output (stub: first chunk only)
        total_out += strm.total_out;
    } while (ret != Z_STREAM_END);

    inflateEnd(&strm);

    Buffer out;
    out.len = MIN(total_out, 4096); // cap
    out.data = g_malloc(out.len);
    memcpy(out.data, outbuf, out.len);
    return out;
}

void gzip_plugin_init(void) {
    g_plugin_registry->register_plugin("GZip",
                                       gzip_decode,
                                       NULL,
                                       NULL,
                                       gzip_detect,
                                       90);
    log_info("GZip plugin registered");
}
