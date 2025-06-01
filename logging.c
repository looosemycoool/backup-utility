#include "logging.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <time.h>
#include <string.h>

// 전역 변수
static int verbose_enabled = 0;
static FILE *log_file = NULL;

void init_logging(int verbose) {
    verbose_enabled = verbose;
    
    // 로그 파일 열기 (추가 모드)
    log_file = fopen("backup.log", "a");
    if (!log_file) {
        fprintf(stderr, "Warning: 로그 파일을 열 수 없습니다. 콘솔에만 출력됩니다.\n");
    }
    
    // 로그 시작 표시
    log_message(LOG_INFO, "=== 로깅 시스템 시작 ===");
}

void log_message(LogLevel level, const char *format, ...) {
    // 로그 레벨 문자열
    const char *level_strings[] = {
        "DEBUG",
        "INFO", 
        "WARNING",
        "ERROR"
    };
    
    // 현재 시간 가져오기
    time_t now;
    struct tm *local_time;
    char time_buffer[64];
    
    time(&now);
    local_time = localtime(&now);
    strftime(time_buffer, sizeof(time_buffer), "%Y-%m-%d %H:%M:%S", local_time);
    
    // 가변 인자 처리
    va_list args;
    va_start(args, format);
    
    // 콘솔 출력 (verbose 모드이거나 WARNING 이상일 때)
    if (verbose_enabled || level >= LOG_WARNING) {
        printf("[%s] %s: ", level_strings[level], time_buffer);
        vprintf(format, args);
        printf("\n");
        fflush(stdout);
    }
    
    // 로그 파일 출력
    if (log_file) {
        fprintf(log_file, "[%s] %s: ", level_strings[level], time_buffer);
        vfprintf(log_file, format, args);
        fprintf(log_file, "\n");
        fflush(log_file);
    }
    
    va_end(args);
}

void close_logging(void) {
    log_message(LOG_INFO, "=== 로깅 시스템 종료 ===");
    
    if (log_file) {
        fclose(log_file);
        log_file = NULL;
    }
}
