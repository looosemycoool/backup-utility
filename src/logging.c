#include "backup.h"

static FILE *g_log_file = NULL;
static log_level_t g_current_log_level = LOG_INFO;

void init_logging(const backup_options_t *opts) {
    g_current_log_level = opts->log_level;
    
    if (opts->logging && strlen(opts->log_file) > 0) {
        g_log_file = fopen(opts->log_file, "a");
        if (g_log_file) {
            log_info("=== 로깅 시작 ===");
            log_info("프로그램: 고급 백업 유틸리티 v%s", VERSION);
            log_info("시작 시간: %s", ctime(&g_stats.start_time));
        } else {
            printf("경고: 로그 파일을 열 수 없습니다: %s\n", opts->log_file);
        }
    }
}

void close_logging(void) {
    if (g_log_file) {
        log_info("=== 로깅 종료 ===");
        fclose(g_log_file);
        g_log_file = NULL;
    }
}

void log_message(int level, const char *format, ...) {
    if (level > g_current_log_level) {
        return;
    }
    
    const char *level_names[] = {"ERROR", "WARNING", "INFO", "DEBUG"};
    const char *level_name = (level >= 0 && level <= 3) ? level_names[level] : "UNKNOWN";
    
    char timestamp[64];
    time_t now = time(NULL);
    struct tm *tm_info = localtime(&now);
    strftime(timestamp, sizeof(timestamp), "%Y-%m-%d %H:%M:%S", tm_info);
    
    va_list args;
    va_start(args, format);
    
    // 콘솔 출력 (에러와 경고는 항상, 나머지는 verbose 모드에서)
    if (level <= LOG_WARNING || g_options.verbose) {
        printf("[%s] %s: ", timestamp, level_name);
        vprintf(format, args);
        printf("\n");
    }
    
    // 파일 출력
    if (g_log_file) {
        fprintf(g_log_file, "[%s] %s: ", timestamp, level_name);
        vfprintf(g_log_file, format, args);
        fprintf(g_log_file, "\n");
        fflush(g_log_file);
    }
    
    va_end(args);
}

void log_error(const char *format, ...) {
    va_list args;
    va_start(args, format);
    
    char timestamp[64];
    time_t now = time(NULL);
    struct tm *tm_info = localtime(&now);
    strftime(timestamp, sizeof(timestamp), "%Y-%m-%d %H:%M:%S", tm_info);
    
    printf("[%s] ERROR: ", timestamp);
    vprintf(format, args);
    printf("\n");
    
    if (g_log_file) {
        fprintf(g_log_file, "[%s] ERROR: ", timestamp);
        vfprintf(g_log_file, format, args);
        fprintf(g_log_file, "\n");
        fflush(g_log_file);
    }
    
    va_end(args);
}

void log_warning(const char *format, ...) {
    va_list args;
    va_start(args, format);
    log_message(LOG_WARNING, format, args);
    va_end(args);
}

void log_info(const char *format, ...) {
    va_list args;
    va_start(args, format);
    log_message(LOG_INFO, format, args);
    va_end(args);
}

void log_debug(const char *format, ...) {
    va_list args;
    va_start(args, format);
    log_message(LOG_DEBUG, format, args);
    va_end(args);
}

void rotate_log_if_needed(void) {
    const long MAX_LOG_SIZE = 10 * 1024 * 1024; // 10MB
    struct stat st;
    
    if (!g_log_file || !g_options.logging) {
        return;
    }
    
    if (stat(g_options.log_file, &st) == 0 && st.st_size > MAX_LOG_SIZE) {
        char backup_log[MAX_PATH];
        snprintf(backup_log, sizeof(backup_log), "%s.old", g_options.log_file);
        
        fclose(g_log_file);
        rename(g_options.log_file, backup_log);
        
        g_log_file = fopen(g_options.log_file, "a");
        if (g_log_file) {
            fprintf(g_log_file, "=== 로그 로테이션 완료 ===\n");
            fflush(g_log_file);
        }
        
        log_info("로그 파일 로테이션 완료: %s -> %s", g_options.log_file, backup_log);
    }
}

void log_memory_usage(void) {
    FILE *status_file;
    char line[256];
    long vm_size = 0, vm_rss = 0;
    
    status_file = fopen("/proc/self/status", "r");
    if (!status_file) {
        return;
    }
    
    while (fgets(line, sizeof(line), status_file)) {
        if (strncmp(line, "VmSize:", 7) == 0) {
            sscanf(line, "VmSize: %ld kB", &vm_size);
        } else if (strncmp(line, "VmRSS:", 6) == 0) {
            sscanf(line, "VmRSS: %ld kB", &vm_rss);
        }
    }
    
    fclose(status_file);
    
    if (vm_size > 0 && vm_rss > 0) {
        log_debug("메모리 사용량 - 가상: %ld KB, 물리: %ld KB", vm_size, vm_rss);
    }
}
