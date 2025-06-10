#ifndef BACKUP_H
#define BACKUP_H

#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <dirent.h>
#include <time.h>
#include <errno.h>
#include <getopt.h>
#include <utime.h>
#include <fcntl.h>
#include <stdarg.h>
#include <signal.h>
#include <pthread.h>
#include <limits.h>
#include <stdint.h>
#include <fnmatch.h>
#include <zlib.h>

// 버전 정보
#define VERSION "2.0"
#define BUILD_DATE __DATE__

// 상수 정의
#define MAX_PATH 4096
#define MAX_THREADS 8
#define BUFFER_SIZE 8192
#define MAX_EXCLUDE_PATTERNS 100
#define METADATA_FILE ".backup_metadata"
#define METADATA_MAGIC 0xBAC1C0DE
#define METADATA_VERSION 1

// 오류 코드
typedef enum {
    SUCCESS = 0,
    ERROR_GENERAL = 1,
    ERROR_FILE_NOT_FOUND = 2,
    ERROR_FILE_OPEN = 3,
    ERROR_MEMORY = 4,
    ERROR_COMPRESSION = 5,
    ERROR_CHECKSUM = 6,
    ERROR_INTERRUPTED = 7
} error_code_t;

// 압축 타입
typedef enum {
    COMPRESS_NONE = 0,
    COMPRESS_GZIP,
    COMPRESS_ZLIB,
    COMPRESS_LZ4
} compression_type_t;

// 백업 모드
typedef enum {
    BACKUP_FULL = 0,
    BACKUP_INCREMENTAL,
    BACKUP_DIFFERENTIAL
} backup_mode_t;

// 충돌 처리 모드
typedef enum {
    CONFLICT_ASK = 0,
    CONFLICT_OVERWRITE,
    CONFLICT_SKIP,
    CONFLICT_RENAME
} conflict_mode_t;

// 로그 레벨
typedef enum {
    LOG_ERROR = 0,
    LOG_WARNING,
    LOG_INFO,
    LOG_DEBUG
} log_level_t;

// 백업 옵션
typedef struct {
    char source_path[MAX_PATH];
    char dest_path[MAX_PATH];
    char config_file[MAX_PATH];
    char log_file[MAX_PATH];
    char exclude_patterns[MAX_EXCLUDE_PATTERNS][MAX_PATH];
    int exclude_count;
    compression_type_t compression;
    backup_mode_t mode;
    conflict_mode_t conflict_mode;
    int recursive;
    int verbose;
    int progress;
    int verify;
    int preserve_permissions;
    int preserve_timestamps;
    int dry_run;
    int threads;
    long max_file_size;
    int logging;
    log_level_t log_level;
} backup_options_t;

// 백업 통계
typedef struct {
    long files_processed;
    long files_skipped;
    long files_failed;
    long dirs_processed;
    long bytes_processed;
    long bytes_compressed;
    double compression_ratio;
    time_t start_time;
    time_t end_time;
} backup_stats_t;

// 진행률 정보
typedef struct {
    long total_files;
    long current_files;
    long total_bytes;
    long current_bytes;
    int cancel_requested;
    pthread_mutex_t mutex;
} progress_info_t;

// 백업 메타데이터 엔트리
typedef struct {
    char path[MAX_PATH];
    time_t mtime;
    long size;
    char checksum[33];
    compression_type_t compression;
} backup_metadata_entry_t;

// 백업 메타데이터 헤더
typedef struct {
    uint32_t magic;
    uint32_t version;
    uint32_t entry_count;
    time_t backup_time;
    backup_mode_t mode;
    uint32_t checksum;
} backup_metadata_header_t;

// 전역 변수 선언
extern backup_options_t g_options;
extern backup_stats_t g_stats;
extern progress_info_t g_progress;

// main.c 함수들
void print_usage(const char *prog);
void print_version(void);
compression_type_t parse_compression_type(const char *str);
conflict_mode_t parse_conflict_mode(const char *str);
int load_config_file(const char *config_file, backup_options_t *opts);
int parse_options(int argc, char *argv[], backup_options_t *opts);
void signal_handler(int sig);

// backup.c 함수들
int backup_file(const char *source, const char *dest, const backup_options_t *opts);
int backup_directory(const char *source, const char *dest, const backup_options_t *opts);
int verify_backup_integrity(const char *source, const char *backup, const backup_options_t *opts);

// restore.c 함수들
int restore_file(const char *source, const char *dest, const backup_options_t *opts);
int restore_directory(const char *source, const char *dest, const backup_options_t *opts);
int verify_backup(const char *backup_path, const backup_options_t *opts);
int list_backup_contents(const char *backup_path, const backup_options_t *opts);

// file_utils.c 함수들
int file_exists(const char *path);
int is_directory(const char *path);
int is_regular_file(const char *path);
int create_directory(const char *path);
int create_directory_recursive(const char *path);
char *get_relative_path(const char *base, const char *path);
int match_pattern(const char *pattern, const char *string);
int should_include_file(const char *path, const backup_options_t *opts);
long get_file_size(const char *path);
char *calculate_checksum(const char *path);
int compare_files(const char *file1, const char *file2);
long calculate_directory_size(const char *path);
long count_directory_files(const char *path);

// logging.c 함수들
void init_logging(const backup_options_t *opts);
void close_logging(void);
void log_message(int level, const char *format, ...);
void log_error(const char *format, ...);
void log_warning(const char *format, ...);
void log_info(const char *format, ...);
void log_debug(const char *format, ...);
void rotate_log_if_needed(void);
void log_memory_usage(void);

// compression.c 함수들
int compress_file(const char *source, const char *dest, compression_type_t type);
int decompress_file(const char *source, const char *dest, compression_type_t type);
const char *get_compression_extension(compression_type_t type);
compression_type_t get_compression_type(const char *filename);

#endif /* BACKUP_H */
