#ifndef BACKUP_H
#define BACKUP_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <dirent.h>
#include <time.h>
#include <errno.h>
#include <pthread.h>
#include <zlib.h>
#include <getopt.h>
#include <fcntl.h>
#include <utime.h>
#include <fnmatch.h>
#include <signal.h>
#include <sys/wait.h>
#include <limits.h>
#include <stdarg.h>
#include <stdint.h>
#include <sys/utsname.h>   // struct utsname용
#include <sys/statvfs.h>   // struct statvfs용

// 버전 정보
#define VERSION "2.0"
#define BUILD_DATE __DATE__

// 경로 및 버퍼 크기
#define MAX_PATH 4096
#define BUFFER_SIZE 8192
#define MAX_THREADS 16
#define MAX_PATTERNS 256
#define MAX_EXCLUDE_PATTERNS 256  // 추가된 상수

// 에러 코드
#define SUCCESS 0
#define ERROR_GENERAL 1
#define ERROR_FILE_NOT_FOUND 2
#define ERROR_FILE_OPEN 3
#define ERROR_FILE_READ 4
#define ERROR_FILE_WRITE 5
#define ERROR_COMPRESSION 6
#define ERROR_MEMORY 7
#define ERROR_THREAD 8
#define ERROR_INVALID_PARAMS 9
#define ERROR_CHECKSUM 10

// 압축 타입
typedef enum {
    COMPRESS_NONE = 0,
    COMPRESS_GZIP = 1,
    COMPRESS_ZLIB = 2,
    COMPRESS_LZ4 = 3
} compression_type_t;

// 백업 모드
typedef enum {
    BACKUP_FULL = 0,
    BACKUP_INCREMENTAL = 1,
    BACKUP_DIFFERENTIAL = 2
} backup_mode_t;

// 충돌 처리 모드
typedef enum {
    CONFLICT_ASK = 0,
    CONFLICT_OVERWRITE = 1,
    CONFLICT_SKIP = 2,
    CONFLICT_RENAME = 3
} conflict_mode_t;

// 로그 레벨
typedef enum {
    LOG_ERROR = 0,
    LOG_WARNING = 1,
    LOG_INFO = 2,
    LOG_DEBUG = 3
} log_level_t;

// 백업 옵션 구조체
typedef struct {
    int recursive;
    int verbose;
    int progress;
    int preserve_permissions;
    int preserve_timestamps;
    int dry_run;
    int verify;
    compression_type_t compression;
    backup_mode_t backup_mode;
    backup_mode_t mode;           // 추가된 멤버 (mode 호환성용)
    conflict_mode_t conflict_mode;
    int thread_count;
    int threads;                  // 추가된 멤버 (threads 호환성용)
    int logging;                  // 추가된 멤버
    char exclude_patterns[MAX_PATTERNS][256];
    int exclude_count;
    char config_file[MAX_PATH];
    char log_file[MAX_PATH];
    log_level_t log_level;
    size_t max_file_size;
} backup_options_t;

// 백업 통계 구조체
typedef struct {
    size_t files_processed;
    size_t files_skipped;
    size_t files_failed;
    size_t directories_processed;
    long dirs_processed;          // 추가된 멤버 (long 타입)
    size_t bytes_processed;
    size_t bytes_compressed;
    double compression_ratio;     // 추가된 멤버
    time_t start_time;
    time_t end_time;
} backup_stats_t;

// 진행률 정보 구조체
typedef struct {
    size_t total_files;
    size_t current_files;
    size_t total_bytes;
    size_t current_bytes;
    int percentage;
    int cancel_requested;         // 추가된 멤버
    pthread_mutex_t mutex;        // 추가된 멤버
} progress_info_t;

// 작업 큐 항목
typedef struct work_item {
    char source[MAX_PATH];
    char dest[MAX_PATH];
    struct work_item *next;
} work_item_t;

// 스레드 풀 구조체
typedef struct {
    pthread_t *threads;
    work_item_t *work_queue;
    pthread_mutex_t queue_mutex;
    pthread_cond_t queue_cond;
    int thread_count;
    int shutdown;
} thread_pool_t;

// 전역 변수 선언
extern backup_options_t g_options;
extern backup_stats_t g_stats;
extern progress_info_t g_progress;
extern pthread_mutex_t g_stats_mutex;
extern pthread_mutex_t g_log_mutex;

// 함수 선언

// main.c
void print_usage(const char *prog);
void print_version(void);
int parse_options(int argc, char **argv, backup_options_t *opts);
compression_type_t parse_compression_type(const char *str);
backup_mode_t parse_backup_mode(const char *str);
conflict_mode_t parse_conflict_mode(const char *str);
log_level_t parse_log_level(const char *str);

// backup.c
int backup_file(const char *source, const char *dest, const backup_options_t *opts);
int backup_directory(const char *source, const char *dest, const backup_options_t *opts);
int backup_directory_recursive(const char *source, const char *dest, const backup_options_t *opts);
int verify_backup_integrity(const char *source, const char *backup, const backup_options_t *opts);

// restore.c
int restore_file(const char *source, const char *dest, const backup_options_t *opts);
int restore_directory(const char *source, const char *dest, const backup_options_t *opts);
int restore_directory_recursive(const char *source, const char *dest, const backup_options_t *opts);

// file_utils.c
int file_exists(const char *path);
int is_directory(const char *path);
int is_regular_file(const char *path);
int create_directory(const char *path);
int create_directory_recursive(const char *path);
int copy_file_metadata(const char *source, const char *dest);
int should_include_file(const char *path, const backup_options_t *opts);
int compare_files(const char *file1, const char *file2);
size_t get_file_size(const char *path);
char *get_relative_path(const char *base, const char *path);
void normalize_path(char *path);

// compression.c
int compress_file(const char *source, const char *dest, compression_type_t type);
int decompress_file(const char *source, const char *dest, compression_type_t type);
const char *get_compression_extension(compression_type_t type);
compression_type_t get_compression_type(const char *filename);
int copy_file_simple(const char *source, const char *dest);

// logging.c
void log_message(log_level_t level, const char *format, ...);
void log_error(const char *format, ...);
void log_warning(const char *format, ...);
void log_info(const char *format, ...);
void log_debug(const char *format, ...);
void init_logging(const backup_options_t *opts);
void cleanup_logging(void);

// 스레드 관련
int init_thread_pool(thread_pool_t *pool, int thread_count);
void destroy_thread_pool(thread_pool_t *pool);
int add_work_item(thread_pool_t *pool, const char *source, const char *dest);
void *worker_thread(void *arg);

// 진행률 표시
void init_progress(size_t total_files, size_t total_bytes);
void update_progress(size_t files_done, size_t bytes_done);
void finish_progress(void);

// 유틸리티 매크로
#define MIN(a, b) ((a) < (b) ? (a) : (b))
#define MAX(a, b) ((a) > (b) ? (a) : (b))

#endif // BACKUP_H