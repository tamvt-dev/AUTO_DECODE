#ifndef CONSTANTS_H
#define CONSTANTS_H
#ifdef __cplusplus
extern "C" {
#endif
/* Performance settings */
#define CACHE_SIZE_DEFAULT 1000
#define THREAD_POOL_SIZE 4
#define MAX_DECODE_DEPTH 5
#define STREAM_BUFFER_SIZE 8192

/* Limits */
#define MAX_INPUT_SIZE (1024 * 1024)      /* 1MB */
#define MAX_OUTPUT_SIZE (1024 * 1024 * 2) /* 2MB */
#define MAX_HISTORY_ITEMS 1000
#define MAX_PLUGINS 50

/* Timeouts (milliseconds) */
#define TASK_TIMEOUT_MS 30000
#define ANIMATION_INTERVAL_MS 50

/* UI defaults */
#define DEFAULT_WINDOW_WIDTH 900
#define DEFAULT_WINDOW_HEIGHT 700
#define DEFAULT_FONT_SIZE 11

/* Paths */
#ifdef _WIN32
    #define CONFIG_DIR ".\\config\\hyperdecode"
    #define DATA_DIR ".\\data\\hyperdecode"
    #define CACHE_DIR ".\\cache\\hyperdecode"
    #define PLUGINS_DIR ".\\plugins"
#else
    #define CONFIG_DIR "~/.config/hyperdecode"
    #define DATA_DIR "~/.local/share/hyperdecode"
    #define CACHE_DIR "~/.cache/hyperdecode"
    #define PLUGINS_DIR "/usr/lib/hyperdecode/plugins"
#endif
#ifdef __cplusplus
}
#endif
#endif /* CONSTANTS_H */
