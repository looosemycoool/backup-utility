#include "logging.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <time.h>
#include <string.h>

static int verbose_enabled = 0;

void init_logging(int verbose) {
    verbose_enabled = verbose;
    printf("DEBUG: 로깅 초기화 완료 (verbose=%d)\n", verbose);
}

void log_message(LogLevel level, const char *format, ...) {
    const char *level_strings[] = {"DEBUG", "INFO", "WARNING", "ERROR"};
    
    if (verbose_enabled || level >= LOG_WARNING) {
        printf("[%s] ", level_strings[level]);
        
        va_list args;
        va_start(args, format);
        vprintf(format, args);
        va_end(args);
        
        printf("\n");
        fflush(stdout);
    }
}

void close_logging(void) {
    printf("DEBUG: 로깅 종료\n");
}
