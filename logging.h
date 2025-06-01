#ifndef LOGGING_H
#define LOGGING_H

#include <stdio.h>

// 로그 레벨 정의
typedef enum {
    LOG_DEBUG = 0,    // 디버그 정보
    LOG_INFO = 1,     // 일반 정보
    LOG_WARNING = 2,  // 경고
    LOG_ERROR = 3     // 오류
} LogLevel;

// 함수 선언
void init_logging(int verbose);
void log_message(LogLevel level, const char *format, ...);
void close_logging(void);

#endif // LOGGING_H
