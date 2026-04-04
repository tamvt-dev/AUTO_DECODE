#include "../include/plugin.h"
#include "../include/buffer.h"

// Khai báo các hàm decode của từng plugin (Buffer version)
extern Buffer base64_decode_buffer(Buffer in);
extern Buffer rot13_decode_buffer(Buffer in);
extern Buffer url_decode_buffer(Buffer in);
extern Buffer atbash_decode_buffer(Buffer in);
extern Buffer caesar_decode_buffer(Buffer in);
extern Buffer xor_decode_single(Buffer in);
extern Buffer scramble_decode_buffer(Buffer in);

// Hàm detect (Buffer version)
extern gboolean base64_detect_buffer(Buffer in);
extern gboolean rot13_detect_buffer(Buffer in);
extern gboolean url_detect_buffer(Buffer in);
extern gboolean atbash_detect_buffer(Buffer in);
extern gboolean caesar_detect_buffer(Buffer in);
extern gboolean xor_detect(Buffer in);
extern gboolean scramble_detect_buffer(Buffer in);

// Metadata (tùy chọn)
static char *xor_meta = NULL;
static char *scramble_meta = NULL;

// Mảng decoders (phải kết thúc bằng {NULL})
DecoderInfo decoders[] = {
    {"Base64", base64_decode_buffer, NULL, NULL, base64_detect_buffer, NULL, 100},
    {"ROT13",  rot13_decode_buffer,  NULL, NULL, rot13_detect_buffer,  NULL, 80},
    {"URL",    url_decode_buffer,    NULL, NULL, url_detect_buffer,    NULL, 90},
    {"Atbash", atbash_decode_buffer, NULL, NULL, atbash_detect_buffer, NULL, 70},
    {"Caesar", caesar_decode_buffer, NULL, NULL, caesar_detect_buffer, NULL, 60},
    {"XOR",    xor_decode_single,    NULL, NULL, xor_detect,           xor_meta, 70},
    {"Scramble", scramble_decode_buffer, NULL, NULL, scramble_detect_buffer, scramble_meta, 60},
    {NULL, NULL, NULL, NULL, NULL, NULL, 0}
};