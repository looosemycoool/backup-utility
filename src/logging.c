#include "backup.h"

static FILE *log_file = NULL;
static log_level_t current_log_level = LOG_INFO;
static int log_to_console = 1;
static int log_to_file = 0;

void init_logging(const backup_options_t *opts) {
    if (!opts) return;
    
    current_log_level = opts->log_level;
    
    // 로그 파일 설정
    if (strlen(opts->log_file) > 0) {
        log_file = fopen(opts->log_file, "a");
        if (log_file) {
            log_to_file = 1;
            log_info("로그 파일 초기화 완료: %s", opts->log_file);
        } else {
            fprintf(stderr, "경고: 로그 파일 열기 실패: %s\n", opts->log_file);
        }
    }
    
    // Verbose 모드가 아니면 콘솔 로그 최소화
    if (!opts->verbose) {
        log_to_console = 0;
    }
    
    log_debug("로깅 시스템 초기화 완료 (레벨: %d)", current_log_level);
}

void cleanup_logging(void) {
    if (log_file) {
        log_info("로깅 시스템 종료");
        fclose(log_file);
        log_file = NULL;
    }
    log_to_file = 0;
}

void log_message(log_level_t level, const char *format, ...) {
    va_list args;
    time_t now;
    struct tm *tm_info;
    char time_str[64];
    char message[1024];
    
    // 로그 레벨 확인
    if (level > current_log_level) {
        return;
    }
    
    // 시간 문자열 생성
    time(&now);
    tm_info = localtime(&now);
    strftime(time_str, sizeof(time_str), "%Y-%m-%d %H:%M:%S", tm_info);
    
    // 메시지 생성
    va_start(args, format);
    vsnprintf(message, sizeof(message), format, args);
    va_end(args);
    
    // 레벨별 문자열
    const char *level_names[] = {"ERROR", "WARNING", "INFO", "DEBUG"};
    const char *level_colors[] = {
        "\033[31m", // 빨간색 (ERROR)
        "\033[33m", // 노란색 (WARNING)
        "\033[32m", // 초록색 (INFO)
        "\033[36m"  // 청록색 (DEBUG)
    };
    const char *reset_color = "\033[0m";
    
    // 스레드 안전성을 위한 뮤텍스 잠금
    pthread_mutex_lock(&g_log_mutex);
    
    // 콘솔 출력
    if (log_to_console || level <= LOG_WARNING) {
        if (isatty(STDERR_FILENO)) {
            // 터미널인 경우 색상 출력
            fprintf(stderr, "[%s] %s%s%s: %s\n", 
                   time_str, level_colors[level], level_names[level], 
                   reset_color, message);
        } else {
            // 파이프나 리다이렉션인 경우 색상 없이
            fprintf(stderr, "[%s] %s: %s\n", 
                   time_str, level_names[level], message);
        }
        fflush(stderr);
    }
    
    // 파일 출력
    if (log_to_file && log_file) {
        fprintf(log_file, "[%s] %s: %s\n", 
               time_str, level_names[level], message);
        fflush(log_file);
    }
    
    pthread_mutex_unlock(&g_log_mutex);
}

void log_error(const char *format, ...) {
    va_list args;
    char message[1024];
    
    va_start(args, format);
    vsnprintf(message, sizeof(message), format, args);
    va_end(args);
    
    log_message(LOG_ERROR, "%s", message);
}

void log_warning(const char *format, ...) {
    va_list args;
    char message[1024];
    
    va_start(args, format);
    vsnprintf(message, sizeof(message), format, args);
    va_end(args);
    
    log_message(LOG_WARNING, "%s", message);
}

void log_info(const char *format, ...) {
    va_list args;
    char message[1024];
    
    va_start(args, format);
    vsnprintf(message, sizeof(message), format, args);
    va_end(args);
    
    log_message(LOG_INFO, "%s", message);
}

void log_debug(const char *format, ...) {
    va_list args;
    char message[1024];
    
    va_start(args, format);
    vsnprintf(message, sizeof(message), format, args);
    va_end(args);
    
    log_message(LOG_DEBUG, "%s", message);
}

// 진행률 관련 함수들
void init_progress(size_t total_files, size_t total_bytes) {
    pthread_mutex_lock(&g_stats_mutex);
    g_progress.total_files = total_files;
    g_progress.total_bytes = total_bytes;
    g_progress.current_files = 0;
    g_progress.current_bytes = 0;
    g_progress.percentage = 0;
    pthread_mutex_unlock(&g_stats_mutex);
    
    log_debug("진행률 초기화: %zu files, %zu bytes", total_files, total_bytes);
}

void update_progress(size_t files_done, size_t bytes_done) {
    pthread_mutex_lock(&g_stats_mutex);
    g_progress.current_files = files_done;
    g_progress.current_bytes = bytes_done;
    
    if (g_progress.total_files > 0) {
        g_progress.percentage = (int)((files_done * 100) / g_progress.total_files);
    }
    pthread_mutex_unlock(&g_stats_mutex);
    
    // 진행률 표시는 DEBUG 레벨이 아닌 경우에도 출력
    if (g_options.progress) {
        log_debug("진행률 업데이트: %zu/%zu files (%d%%)", 
                 files_done, g_progress.total_files, g_progress.percentage);
    }
}

void finish_progress(void) {
    pthread_mutex_lock(&g_stats_mutex);
    g_progress.percentage = 100;
    pthread_mutex_unlock(&g_stats_mutex);
    
    log_debug("진행률 완료: 100%%");
}

// 로그 로테이션 (파일 크기 관리)
void rotate_log_file(void) {
    if (!log_file || !log_to_file) return;
    
    // 현재 로그 파일 크기 확인
    long file_size = ftell(log_file);
    const long max_log_size = 10 * 1024 * 1024; // 10MB
    
    if (file_size > max_log_size) {
        char old_log_name[MAX_PATH];
        char current_log_name[MAX_PATH];
        
        // 현재 로그 파일명 저장
        if (g_options.log_file[0] != '\0') {
            strncpy(current_log_name, g_options.log_file, MAX_PATH - 1);
            current_log_name[MAX_PATH - 1] = '\0';
            
            snprintf(old_log_name, sizeof(old_log_name), "%s.old", current_log_name);
            
            // 현재 로그 파일 닫기
            fclose(log_file);
            
            // 기존 .old 파일 제거 후 현재 파일을 .old로 이동
            unlink(old_log_name);
            rename(current_log_name, old_log_name);
            
            // 새 로그 파일 열기
            log_file = fopen(current_log_name, "w");
            if (log_file) {
                log_info("로그 파일 로테이션 완료");
            } else {
                log_to_file = 0;
                fprintf(stderr, "경고: 새 로그 파일 생성 실패\n");
            }
        }
    }
}

// 메모리 사용량 로깅
void log_memory_usage(void) {
    FILE *status_file;
    char line[256];
    unsigned long vm_size = 0, vm_rss = 0;
    
    status_file = fopen("/proc/self/status", "r");
    if (!status_file) {
        return;
    }
    
    while (fgets(line, sizeof(line), status_file)) {
        if (strncmp(line, "VmSize:", 7) == 0) {
            sscanf(line, "VmSize: %lu kB", &vm_size);
        } else if (strncmp(line, "VmRSS:", 6) == 0) {
            sscanf(line, "VmRSS: %lu kB", &vm_rss);
        }
    }
    
    fclose(status_file);
    
    if (vm_size > 0 && vm_rss > 0) {
        log_debug("메모리 사용량 - 가상: %lu KB, 실제: %lu KB", vm_size, vm_rss);
    }
}

// 시스템 정보 로깅
void log_system_info(void) {
    struct utsname sys_info;
    
    if (uname(&sys_info) == 0) {
        log_info("시스템 정보 - OS: %s %s, 아키텍처: %s", 
                sys_info.sysname, sys_info.release, sys_info.machine);
    }
    
    // CPU 정보
    FILE *cpuinfo = fopen("/proc/cpuinfo", "r");
    if (cpuinfo) {
        char line[256];
        int cpu_count = 0;
        
        while (fgets(line, sizeof(line), cpuinfo)) {
            if (strncmp(line, "processor", 9) == 0) {
                cpu_count++;
            }
        }
        fclose(cpuinfo);
        
        if (cpu_count > 0) {
            log_info("CPU 정보 - 코어 수: %d", cpu_count);
        }
    }
    
    // 디스크 여유 공간
    struct statvfs disk_info;
    if (statvfs(".", &disk_info) == 0) {
        unsigned long long free_space = 
            (unsigned long long)disk_info.f_bavail * disk_info.f_frsize;
        
        log_info("디스크 여유 공간: %.2f GB", 
                (double)free_space / (1024 * 1024 * 1024));
    }
}

// 성능 측정 도구
typedef struct {
    char name[64];
    struct timespec start_time;
    struct timespec end_time;
} performance_timer_t;

static performance_timer_t perf_timers[16];
static int perf_timer_count = 0;

void start_performance_timer(const char *name) {
    if (perf_timer_count >= 16) return;
    
    performance_timer_t *timer = &perf_timers[perf_timer_count];
    strncpy(timer->name, name, sizeof(timer->name) - 1);
    timer->name[sizeof(timer->name) - 1] = '\0';
    
    clock_gettime(CLOCK_MONOTONIC, &timer->start_time);
    perf_timer_count++;
    
    log_debug("성능 측정 시작: %s", name);
}

void end_performance_timer(const char *name) {
    for (int i = perf_timer_count - 1; i >= 0; i--) {
        if (strcmp(perf_timers[i].name, name) == 0) {
            clock_gettime(CLOCK_MONOTONIC, &perf_timers[i].end_time);
            
            double elapsed = (perf_timers[i].end_time.tv_sec - perf_timers[i].start_time.tv_sec) +
                           (perf_timers[i].end_time.tv_nsec - perf_timers[i].start_time.tv_nsec) / 1000000000.0;
            
            log_info("성능 측정 완료: %s (%.3f초)", name, elapsed);
            
            // 타이머 제거 (배열에서 뒤의 항목들을 앞으로 이동)
            for (int j = i; j < perf_timer_count - 1; j++) {
                perf_timers[j] = perf_timers[j + 1];
            }
            perf_timer_count--;
            break;
        }
    }
}

// 에러 컨텍스트 로깅 (디버깅용)
void log_error_context(const char *function, const char *file, int line, const char *format, ...) {
    va_list args;
    char message[1024];
    
    va_start(args, format);
    vsnprintf(message, sizeof(message), format, args);
    va_end(args);
    
    log_error("%s [%s:%s:%d]", message, file, function, line);
}

// 매크로를 위한 헬퍼
#define LOG_ERROR_CTX(format, ...) \
    log_error_context(__func__, __FILE__, __LINE__, format, ##__VA_ARGS__)