#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdarg.h>
#include <time.h>
#include <errno.h>
#include <dirent.h>
#include <sys/time.h>
#include <utime.h>

#define BUFFER_SIZE 8192
#define MAX_PATH 4096
#define VERSION "2.0.0"

// 로그 레벨
typedef enum {
    LOG_DEBUG = 0,
    LOG_INFO = 1,
    LOG_WARNING = 2,
    LOG_ERROR = 3
} LogLevel;

// 백업 옵션
typedef struct {
    int verbose;
    int recursive;
    int preserve_metadata;
    int verify_backup;
    int incremental;
    char log_file[MAX_PATH];
} BackupOptions;

// 전역 설정
static BackupOptions g_options = {0};
static FILE *g_log_file = NULL;

// === 로깅 시스템 ===
void init_logging(const BackupOptions *options) {
    g_options = *options;
    
    if (strlen(options->log_file) > 0) {
        g_log_file = fopen(options->log_file, "a");
        if (!g_log_file) {
            fprintf(stderr, "경고: 로그 파일을 열 수 없습니다: %s\n", options->log_file);
        }
    }
}

void log_message(LogLevel level, const char *format, ...) {
    const char *level_strings[] = {"DEBUG", "INFO", "WARNING", "ERROR"};
    char timestamp[64];
    time_t now = time(NULL);
    struct tm *tm_info = localtime(&now);
    strftime(timestamp, sizeof(timestamp), "%Y-%m-%d %H:%M:%S", tm_info);
    
    // 콘솔 출력 (verbose 모드 또는 경고/오류)
    if (g_options.verbose || level >= LOG_WARNING) {
        printf("[%s] %s: ", level_strings[level], timestamp);
        va_list args;
        va_start(args, format);
        vprintf(format, args);
        va_end(args);
        printf("\n");
        fflush(stdout);
    }
    
    // 파일 로그
    if (g_log_file) {
        fprintf(g_log_file, "[%s] %s: ", level_strings[level], timestamp);
        va_list args;
        va_start(args, format);
        vfprintf(g_log_file, format, args);
        va_end(args);
        fprintf(g_log_file, "\n");
        fflush(g_log_file);
    }
}

void close_logging(void) {
    if (g_log_file) {
        fclose(g_log_file);
        g_log_file = NULL;
    }
}

// === 파일 유틸리티 ===
int file_exists(const char *path) {
    struct stat st;
    return (stat(path, &st) == 0);
}

int is_directory(const char *path) {
    struct stat st;
    return (stat(path, &st) == 0 && S_ISDIR(st.st_mode));
}

int is_regular_file(const char *path) {
    struct stat st;
    return (stat(path, &st) == 0 && S_ISREG(st.st_mode));
}

void get_file_size_str(off_t size, char *buffer, size_t buf_size) {
    if (size < 1024) {
        snprintf(buffer, buf_size, "%ld B", (long)size);
    } else if (size < 1024 * 1024) {
        snprintf(buffer, buf_size, "%.1f KB", (double)size / 1024);
    } else if (size < 1024 * 1024 * 1024) {
        snprintf(buffer, buf_size, "%.1f MB", (double)size / (1024 * 1024));
    } else {
        snprintf(buffer, buf_size, "%.1f GB", (double)size / (1024 * 1024 * 1024));
    }
}

int create_directory_recursive(const char *path) {
    char temp_path[MAX_PATH];
    char *p = NULL;
    size_t len;

    snprintf(temp_path, sizeof(temp_path), "%s", path);
    len = strlen(temp_path);
    if (temp_path[len - 1] == '/') {
        temp_path[len - 1] = 0;
    }

    for (p = temp_path + 1; *p; p++) {
        if (*p == '/') {
            *p = 0;
            if (mkdir(temp_path, 0755) != 0 && errno != EEXIST) {
                return -1;
            }
            *p = '/';
        }
    }
    
    if (mkdir(temp_path, 0755) != 0 && errno != EEXIST) {
        return -1;
    }
    
    return 0;
}

// === 파일 복사 및 백업 ===
int copy_file_with_metadata(const char *source, const char *destination) {
    int src_fd, dest_fd;
    char buffer[BUFFER_SIZE];
    ssize_t bytes_read, bytes_written;
    struct stat src_stat;
    off_t total_bytes = 0;
    
    // 소스 파일 정보 얻기
    if (stat(source, &src_stat) != 0) {
        log_message(LOG_ERROR, "소스 파일 정보를 읽을 수 없습니다: %s (%s)", source, strerror(errno));
        return -1;
    }
    
    // 소스 파일 열기
    src_fd = open(source, O_RDONLY);
    if (src_fd == -1) {
        log_message(LOG_ERROR, "소스 파일을 열 수 없습니다: %s (%s)", source, strerror(errno));
        return -1;
    }
    
    // 목적지 파일 생성
    dest_fd = open(destination, O_WRONLY | O_CREAT | O_TRUNC, src_stat.st_mode);
    if (dest_fd == -1) {
        log_message(LOG_ERROR, "목적지 파일을 생성할 수 없습니다: %s (%s)", destination, strerror(errno));
        close(src_fd);
        return -1;
    }
    
    // 파일 복사
    char size_str[32];
    get_file_size_str(src_stat.st_size, size_str, sizeof(size_str));
    log_message(LOG_DEBUG, "파일 복사 시작: %s (%s)", source, size_str);
    
    while ((bytes_read = read(src_fd, buffer, BUFFER_SIZE)) > 0) {
        bytes_written = write(dest_fd, buffer, bytes_read);
        if (bytes_written != bytes_read) {
            log_message(LOG_ERROR, "파일 쓰기 오류: %s (%s)", destination, strerror(errno));
            close(src_fd);
            close(dest_fd);
            unlink(destination);
            return -1;
        }
        total_bytes += bytes_written;
        
        if (g_options.verbose && src_stat.st_size > 0) {
            int progress = (int)((total_bytes * 100) / src_stat.st_size);
            if (progress % 10 == 0) {
                printf("\r진행률: %d%% (%ld/%ld bytes)", progress, (long)total_bytes, (long)src_stat.st_size);
                fflush(stdout);
            }
        }
    }
    
    if (g_options.verbose && src_stat.st_size > 0) {
        printf("\r진행률: 100%% (%ld/%ld bytes)\n", (long)total_bytes, (long)src_stat.st_size);
    }
    
    if (bytes_read == -1) {
        log_message(LOG_ERROR, "파일 읽기 오류: %s (%s)", source, strerror(errno));
        close(src_fd);
        close(dest_fd);
        unlink(destination);
        return -1;
    }
    
    close(src_fd);
    close(dest_fd);
    
    // 메타데이터 보존
    if (g_options.preserve_metadata) {
        // 권한 설정
        if (chmod(destination, src_stat.st_mode) != 0) {
            log_message(LOG_WARNING, "권한 설정 실패: %s (%s)", destination, strerror(errno));
        }
        
        // 타임스탬프 설정
        struct utimbuf times;
        times.actime = src_stat.st_atime;
        times.modtime = src_stat.st_mtime;
        if (utime(destination, &times) != 0) {
            log_message(LOG_WARNING, "타임스탬프 설정 실패: %s (%s)", destination, strerror(errno));
        }
        
        log_message(LOG_DEBUG, "메타데이터 보존 완료: %s", destination);
    }
    
    log_message(LOG_INFO, "파일 복사 완료: %s -> %s (%ld bytes)", source, destination, (long)total_bytes);
    return 0;
}

int verify_backup(const char *source, const char *backup) {
    struct stat src_stat, backup_stat;
    
    if (stat(source, &src_stat) != 0 || stat(backup, &backup_stat) != 0) {
        log_message(LOG_ERROR, "백업 검증 실패: 파일 정보를 읽을 수 없습니다");
        return -1;
    }
    
    if (src_stat.st_size != backup_stat.st_size) {
        log_message(LOG_ERROR, "백업 검증 실패: 파일 크기가 다릅니다 (%ld vs %ld)", 
                   (long)src_stat.st_size, (long)backup_stat.st_size);
        return -1;
    }
    
    // 내용 비교 (간단한 버전)
    FILE *src_file = fopen(source, "rb");
    FILE *backup_file = fopen(backup, "rb");
    
    if (!src_file || !backup_file) {
        if (src_file) fclose(src_file);
        if (backup_file) fclose(backup_file);
        log_message(LOG_ERROR, "백업 검증 실패: 파일을 열 수 없습니다");
        return -1;
    }
    
    char src_buffer[BUFFER_SIZE], backup_buffer[BUFFER_SIZE];
    size_t src_read, backup_read;
    
    while ((src_read = fread(src_buffer, 1, BUFFER_SIZE, src_file)) > 0) {
        backup_read = fread(backup_buffer, 1, BUFFER_SIZE, backup_file);
        
        if (src_read != backup_read || memcmp(src_buffer, backup_buffer, src_read) != 0) {
            fclose(src_file);
            fclose(backup_file);
            log_message(LOG_ERROR, "백업 검증 실패: 파일 내용이 다릅니다");
            return -1;
        }
    }
    
    fclose(src_file);
    fclose(backup_file);
    
    log_message(LOG_INFO, "백업 검증 성공: %s", backup);
    return 0;
}

int backup_directory_recursive(const char *source_dir, const char *backup_dir) {
    DIR *dir;
    struct dirent *entry;
    char source_path[MAX_PATH], backup_path[MAX_PATH];
    int file_count = 0, error_count = 0;
    
    dir = opendir(source_dir);
    if (!dir) {
        log_message(LOG_ERROR, "디렉토리를 열 수 없습니다: %s (%s)", source_dir, strerror(errno));
        return -1;
    }
    
    // 백업 디렉토리 생성
    if (create_directory_recursive(backup_dir) != 0) {
        log_message(LOG_ERROR, "백업 디렉토리 생성 실패: %s (%s)", backup_dir, strerror(errno));
        closedir(dir);
        return -1;
    }
    
    log_message(LOG_INFO, "디렉토리 백업 시작: %s -> %s", source_dir, backup_dir);
    
    while ((entry = readdir(dir)) != NULL) {
        // . 과 .. 건너뛰기
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
            continue;
        }
        
        snprintf(source_path, sizeof(source_path), "%s/%s", source_dir, entry->d_name);
        snprintf(backup_path, sizeof(backup_path), "%s/%s", backup_dir, entry->d_name);
        
        if (is_directory(source_path)) {
            // 하위 디렉토리 재귀 백업
            if (backup_directory_recursive(source_path, backup_path) != 0) {
                error_count++;
            }
        } else if (is_regular_file(source_path)) {
            // 파일 백업
            if (copy_file_with_metadata(source_path, backup_path) == 0) {
                if (g_options.verify_backup) {
                    if (verify_backup(source_path, backup_path) != 0) {
                        error_count++;
                    }
                }
                file_count++;
            } else {
                error_count++;
            }
        }
    }
    
    closedir(dir);
    
    log_message(LOG_INFO, "디렉토리 백업 완료: %s (%d 파일, %d 오류)", 
               source_dir, file_count, error_count);
    
    return error_count > 0 ? -1 : 0;
}

// === 백업 목록 관리 ===
void list_backups(const char *backup_dir) {
    DIR *dir;
    struct dirent *entry;
    struct stat st;
    char full_path[MAX_PATH];
    char size_str[32], time_str[64];
    int count = 0;
    
    if (!is_directory(backup_dir)) {
        log_message(LOG_ERROR, "백업 디렉토리가 존재하지 않습니다: %s", backup_dir);
        return;
    }
    
    dir = opendir(backup_dir);
    if (!dir) {
        log_message(LOG_ERROR, "디렉토리를 열 수 없습니다: %s (%s)", backup_dir, strerror(errno));
        return;
    }
    
    printf("\n=== 백업 목록: %s ===\n", backup_dir);
    printf("%-40s %12s %20s %s\n", "이름", "크기", "수정일시", "타입");
    printf("%-40s %12s %20s %s\n", "----", "----", "--------", "----");
    
    while ((entry = readdir(dir)) != NULL) {
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
            continue;
        }
        
        snprintf(full_path, sizeof(full_path), "%s/%s", backup_dir, entry->d_name);
        
        if (stat(full_path, &st) == 0) {
            get_file_size_str(st.st_size, size_str, sizeof(size_str));
            strftime(time_str, sizeof(time_str), "%Y-%m-%d %H:%M:%S", localtime(&st.st_mtime));
            
            const char *type = S_ISDIR(st.st_mode) ? "디렉토리" : "파일";
            
            printf("%-40s %12s %20s %s\n", entry->d_name, 
                   S_ISDIR(st.st_mode) ? "-" : size_str, time_str, type);
            count++;
        }
    }
    
    closedir(dir);
    printf("\n총 %d 항목\n\n", count);
}

// === 명령어 처리 ===
void print_usage(const char *program_name) {
    printf("고급 파일 백업 유틸리티 v%s\n", VERSION);
    printf("사용법: %s [명령어] [옵션] [인자...]\n\n", program_name);
    
    printf("명령어:\n");
    printf("  backup <소스> <목적지>      : 지정된 소스를 목적지에 백업\n");
    printf("  restore <백업> <목적지>     : 백업을 원래 위치로 복원\n");
    printf("  list <백업디렉토리>         : 백업 목록 표시\n");
    printf("  help                      : 도움말 표시\n");
    printf("  version                   : 버전 정보 표시\n\n");
    
    printf("옵션:\n");
    printf("  -v, --verbose             : 상세 정보 출력\n");
    printf("  -r, --recursive           : 디렉토리 재귀 처리\n");
    printf("  -m, --metadata            : 메타데이터 보존 (권한, 타임스탬프)\n");
    printf("  -c, --verify              : 백업 후 검증\n");
    printf("  -l, --log <파일>          : 로그를 파일에 저장\n\n");
    
    printf("예시:\n");
    printf("  %s backup -v -m file.txt backup/\n", program_name);
    printf("  %s backup -r -c -l backup.log /home/user/docs backup/\n", program_name);
    printf("  %s restore backup/file.txt /home/user/\n", program_name);
    printf("  %s list backup/\n", program_name);
}

void print_version(void) {
    printf("파일 백업 유틸리티 v%s\n", VERSION);
    printf("Copyright (c) 2025. MIT License.\n");
    printf("고급 기능: 재귀 백업, 메타데이터 보존, 백업 검증, 로깅\n");
}

int parse_options(int argc, char *argv[], BackupOptions *options, int *arg_start) {
    // 기본값 설정
    memset(options, 0, sizeof(BackupOptions));
    *arg_start = 2; // 기본적으로 명령어 다음부터
    
    for (int i = 2; i < argc; i++) {
        if (strcmp(argv[i], "-v") == 0 || strcmp(argv[i], "--verbose") == 0) {
            options->verbose = 1;
        } else if (strcmp(argv[i], "-r") == 0 || strcmp(argv[i], "--recursive") == 0) {
            options->recursive = 1;
        } else if (strcmp(argv[i], "-m") == 0 || strcmp(argv[i], "--metadata") == 0) {
            options->preserve_metadata = 1;
        } else if (strcmp(argv[i], "-c") == 0 || strcmp(argv[i], "--verify") == 0) {
            options->verify_backup = 1;
        } else if (strcmp(argv[i], "-l") == 0 || strcmp(argv[i], "--log") == 0) {
            if (i + 1 < argc) {
                strncpy(options->log_file, argv[i + 1], MAX_PATH - 1);
                i++; // 다음 인자 건너뛰기
            } else {
                log_message(LOG_ERROR, "-l 옵션에는 로그 파일명이 필요합니다");
                return -1;
            }
        } else {
            // 옵션이 아닌 첫 번째 인자를 찾았음
            *arg_start = i;
            break;
        }
    }
    
    return 0;
}

int main(int argc, char *argv[]) {
    if (argc < 2) {
        print_usage(argv[0]);
        return EXIT_FAILURE;
    }
    
    const char *command = argv[1];
    
    // 도움말과 버전은 바로 처리
    if (strcmp(command, "help") == 0) {
        print_usage(argv[0]);
        return EXIT_SUCCESS;
    }
    
    if (strcmp(command, "version") == 0) {
        print_version();
        return EXIT_SUCCESS;
    }
    
    // 옵션 파싱
    BackupOptions options;
    int arg_start;
    if (parse_options(argc, argv, &options, &arg_start) != 0) {
        return EXIT_FAILURE;
    }
    
    // 로깅 초기화
    init_logging(&options);
    
    log_message(LOG_INFO, "프로그램 시작: %s v%s", command, VERSION);
    
    int result = EXIT_SUCCESS;
    
    if (strcmp(command, "backup") == 0) {
        if (argc - arg_start < 2) {
            log_message(LOG_ERROR, "백업 명령에는 소스와 목적지가 필요합니다");
            printf("사용법: %s backup [옵션] <소스> <목적지>\n", argv[0]);
            result = EXIT_FAILURE;
        } else {
            const char *source = argv[argc - 2];
            const char *destination = argv[argc - 1];
            
            log_message(LOG_INFO, "백업 시작: %s -> %s", source, destination);
            
            if (!file_exists(source)) {
                log_message(LOG_ERROR, "소스가 존재하지 않습니다: %s", source);
                result = EXIT_FAILURE;
            } else {
                if (is_directory(source) && options.recursive) {
                    // 디렉토리 재귀 백업
                    if (backup_directory_recursive(source, destination) != 0) {
                        result = EXIT_FAILURE;
                    }
                } else if (is_regular_file(source)) {
                    // 단일 파일 백업
                    if (copy_file_with_metadata(source, destination) != 0) {
                        result = EXIT_FAILURE;
                    } else if (options.verify_backup) {
                        if (verify_backup(source, destination) != 0) {
                            result = EXIT_FAILURE;
                        }
                    }
                } else {
                    log_message(LOG_ERROR, "소스가 일반 파일이 아닙니다 (디렉토리인 경우 -r 옵션 사용): %s", source);
                    result = EXIT_FAILURE;
                }
            }
        }
        
    } else if (strcmp(command, "restore") == 0) {
        if (argc - arg_start < 2) {
            log_message(LOG_ERROR, "복원 명령에는 백업 파일과 목적지가 필요합니다");
            printf("사용법: %s restore [옵션] <백업파일> <목적지>\n", argv[0]);
            result = EXIT_FAILURE;
        } else {
            const char *backup_file = argv[argc - 2];
            const char *destination = argv[argc - 1];
            
            log_message(LOG_INFO, "복원 시작: %s -> %s", backup_file, destination);
            
            if (!file_exists(backup_file)) {
                log_message(LOG_ERROR, "백업 파일이 존재하지 않습니다: %s", backup_file);
                result = EXIT_FAILURE;
            } else {
                if (copy_file_with_metadata(backup_file, destination) != 0) {
                    result = EXIT_FAILURE;
                }
            }
        }
        
    } else if (strcmp(command, "list") == 0) {
        if (argc - arg_start < 1) {
            log_message(LOG_ERROR, "목록 명령에는 백업 디렉토리가 필요합니다");
            printf("사용법: %s list [옵션] <백업디렉토리>\n", argv[0]);
            result = EXIT_FAILURE;
        } else {
            const char *backup_dir = argv[argc - 1];
            list_backups(backup_dir);
        }
        
    } else {
        log_message(LOG_ERROR, "알 수 없는 명령어: %s", command);
        print_usage(argv[0]);
        result = EXIT_FAILURE;
    }
    
    log_message(LOG_INFO, "프로그램 종료 (종료 코드: %d)", result);
    close_logging();
    
    return result;
}
