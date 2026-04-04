#ifndef LOGGING_H
#define LOGGING_H

#include <glib.h>
#ifdef __cplusplus
extern "C" {
#endif
typedef enum {
    LOG_LEVEL_DEBUG = 0,
    LOG_LEVEL_INFO,
    LOG_LEVEL_WARN,
    LOG_LEVEL_ERROR,
    LOG_LEVEL_FATAL
} LogLevel;

void log_init(LogLevel level, const char *filename, gboolean console);
void log_close(void);
void log_write(LogLevel level, const char *file, int line, 
               const char *func, const char *format, ...) G_GNUC_PRINTF(5, 6);

#define log_debug(...) log_write(LOG_LEVEL_DEBUG, __FILE__, __LINE__, __func__, __VA_ARGS__)
#define log_info(...)  log_write(LOG_LEVEL_INFO,  __FILE__, __LINE__, __func__, __VA_ARGS__)
#define log_warn(...)  log_write(LOG_LEVEL_WARN,  __FILE__, __LINE__, __func__, __VA_ARGS__)
#define log_error(...) log_write(LOG_LEVEL_ERROR, __FILE__, __LINE__, __func__, __VA_ARGS__)
#define log_fatal(...) log_write(LOG_LEVEL_FATAL, __FILE__, __LINE__, __func__, __VA_ARGS__)
#ifdef __cplusplus
}
#endif
#endif /* LOGGING_H */