#include "../include/logging.h"
#include <stdio.h>
#include <time.h>
#include <glib.h>

static FILE *log_file = NULL;
static LogLevel current_level = LOG_LEVEL_INFO;
static GMutex log_mutex;

static const char* level_names[] = {
    "DEBUG", "INFO", "WARN", "ERROR", "FATAL"
};

void log_init(LogLevel level, const char *filename, gboolean console) {
    current_level = level;
    g_mutex_init(&log_mutex);
    
    if (filename && !console) {
        log_file = fopen(filename, "a");
        if (!log_file) {
            fprintf(stderr, "Failed to open log file: %s\n", filename);
        }
    }
    
    if (console) {
        log_file = stdout;
    }
    
    log_info("Logging initialized (level: %s)", level_names[level]);
}

void log_close(void) {
    if (log_file && log_file != stdout) {
        fclose(log_file);
    }
    g_mutex_clear(&log_mutex);
}

void log_write(LogLevel level, const char *file, int line, 
               const char *func, const char *format, ...) {
    if (level < current_level) return;
    if (!log_file) return;
    
    time_t now = time(NULL);
    struct tm *tm_info = localtime(&now);
    char time_str[20];
    strftime(time_str, sizeof(time_str), "%Y-%m-%d %H:%M:%S", tm_info);
    
    g_mutex_lock(&log_mutex);
    
    fprintf(log_file, "[%s] [%s] %s:%d:%s - ", 
            time_str, level_names[level], file, line, func);
    
    va_list args;
    va_start(args, format);
    vfprintf(log_file, format, args);
    va_end(args);
    
    fprintf(log_file, "\n");
    fflush(log_file);
    
    g_mutex_unlock(&log_mutex);
}