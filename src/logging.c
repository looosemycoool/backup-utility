#include "logging.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <time.h>
#include <string.h>

static int verbose_enabled = 0;
static FILE *log_file = NULL;

void init_logging(int verbose) {
    verbose_enabled = verbose;
    log_file = fopen("backup.log", "a");
    if (!log_file) {
        fprintf(stderr, "Warning: 로그 파일을 열 수 없습니다.\n");
    }
}

void log_message(LogLevel level, const char *format, ...) {
    const char *level_strings[] = {"DEBUG", "INFO", "WARNING", "ERROR"};
    
    time_t now;
    struct tm *local_time;
    char time_buffer[64];
    
    time(&now);
    local_time = localtime(&now);
    strftime(time_buffer, sizeof(time_buffer), "%Y-%m-%d %H:%M:%S", local_time);
    
    va_list args;
    va_start(args, format);
    
    if (verbose_enabled || level >= LOG_WARNING) {
        printf("[%s] %s: ", level_strings[level], time_buffer);
        vprintf(format, args);
        printf("\n");
        fflush(stdout);
    }
    
    if (log_file) {
        fprintf(log_file, "[%s] %s: ", level_strings[level], time_buffer);
        vfprintf(log_file, format, args);
        fprintf(log_file, "\n");
        fflush(log_file);
    }
    
    va_end(args);
}

void close_logging(void) {
    if (log_file) {
        fclose(log_file);
        log_file = NULL;
    }
}
