#include "backup.h"

static pthread_mutex_t g_stats_mutex = PTHREAD_MUTEX_INITIALIZER;

// 바이트 크기를 읽기 쉬운 형태로 변환
static char *format_bytes(long bytes) {
    static char buffer[32];
    const char *units[] = {"B", "KB", "MB", "GB", "TB"};
    int unit = 0;
    double size = bytes;
    
    while (size >= 1024.0 && unit < 4) {
        size /= 1024.0;
        unit++;
    }
    
    if (unit == 0) {
        snprintf(buffer, sizeof(buffer), "%ld %s", bytes, units[unit]);
    } else {
        snprintf(buffer, sizeof(buffer), "%.1f %s", size, units[unit]);
    }
    
    return buffer;
}

// 압축 타입을 문자열로 변환
static const char *get_compression_type_string(compression_type_t type) {
    switch (type) {
        case COMPRESS_NONE:
            return "none";
        case COMPRESS_GZIP:
            return "gzip";
        case COMPRESS_ZLIB:
            return "zlib";
        case COMPRESS_LZ4:
            return "lz4";
        default:
            return "unknown";
    }
}

int restore_file(const char *source, const char *dest, const backup_options_t *opts) {
    struct stat src_stat;
    char final_dest[MAX_PATH];
    char clean_filename[MAX_PATH];
    compression_type_t compression;
    int counter = 1;
    
    if (stat(source, &src_stat) != 0) {
        log_error("소스 파일 정보 확인 실패: %s", source);
        return ERROR_FILE_NOT_FOUND;
    }
    
    // 압축 타입 자동 감지
    compression = get_compression_type(source);
    
    // 대상 파일명에서 압축 확장자 제거
    strncpy(clean_filename, dest, sizeof(clean_filename) - 1);
    clean_filename[sizeof(clean_filename) - 1] = '\0';
    
    if (compression != COMPRESS_NONE) {
        const char *ext = get_compression_extension(compression);
        char *ext_pos = strstr(clean_filename, ext);
        if (ext_pos) {
            *ext_pos = '\0';
        }
    }
    
    // 대상 디렉토리가 있으면 파일명만 사용
    if (is_directory(dest)) {
        const char *filename = strrchr(clean_filename, '/');
        filename = filename ? filename + 1 : clean_filename;
        snprintf(final_dest, sizeof(final_dest), "%s/%s", dest, filename);
    } else {
        strncpy(final_dest, clean_filename, sizeof(final_dest) - 1);
        final_dest[sizeof(final_dest) - 1] = '\0';
    }
    
    // 충돌 처리
    if (file_exists(final_dest)) {
        switch (opts->conflict_mode) {
            case CONFLICT_SKIP:
                log_info("파일 건너뛰기: %s (이미 존재함)", final_dest);
                pthread_mutex_lock(&g_stats_mutex);
                g_stats.files_skipped++;
                pthread_mutex_unlock(&g_stats_mutex);
                return SUCCESS;
                
            case CONFLICT_ASK:
                printf("파일이 이미 존재합니다: %s\n", final_dest);
                printf("덮어쓰시겠습니까? (y/n): ");
                char response;
                scanf(" %c", &response);
                if (response != 'y' && response != 'Y') {
                    pthread_mutex_lock(&g_stats_mutex);
                    g_stats.files_skipped++;
                    pthread_mutex_unlock(&g_stats_mutex);
                    return SUCCESS;
                }
                break;
                
            case CONFLICT_RENAME:
                {
                    char new_dest[MAX_PATH];
                    do {
                        snprintf(new_dest, sizeof(new_dest), "%s.restored.%d", final_dest, counter++);
                    } while (file_exists(new_dest) && counter < 1000);
                    
                    if (counter >= 1000) {
                        log_error("고유한 파일명을 생성할 수 없습니다: %s", final_dest);
                        return ERROR_GENERAL;
                    }
                    
                    strncpy(final_dest, new_dest, sizeof(final_dest) - 1);
                    final_dest[sizeof(final_dest) - 1] = '\0';
                }
                break;
                
            case CONFLICT_OVERWRITE:
            default:
                break;
        }
    }
    
    // 대상 디렉토리 생성
    char dest_dir[MAX_PATH];
    strncpy(dest_dir, final_dest, sizeof(dest_dir) - 1);
    dest_dir[sizeof(dest_dir) - 1] = '\0';
    
    char *last_slash = strrchr(dest_dir, '/');
    if (last_slash) {
        *last_slash = '\0';
        create_directory_recursive(dest_dir);
    }
    
    if (opts->dry_run) {
        printf("DRY RUN: %s -> %s\n", source, final_dest);
        return SUCCESS;
    }
    
    if (opts->verbose) {
        printf("복원: %s -> %s", source, final_dest);
        if (compression != COMPRESS_NONE) {
            printf(" (압축 해제: %s)", get_compression_type_string(compression));
        }
        printf("\n");
    }
    
    // 파일 복원 (압축 해제 포함)
    int result = decompress_file(source, final_dest, compression);
    
    if (result != SUCCESS) {
        log_error("파일 복원 실패: %s", source);
        pthread_mutex_lock(&g_stats_mutex);
        g_stats.files_failed++;
        pthread_mutex_unlock(&g_stats_mutex);
        return result;
    }
    
    // 권한 및 타임스탬프 복원
    if (opts->preserve_permissions || opts->preserve_timestamps) {
        if (opts->preserve_permissions) {
            chmod(final_dest, src_stat.st_mode);
        }
        
        if (opts->preserve_timestamps) {
            struct utimbuf times;
            times.actime = src_stat.st_atime;
            times.modtime = src_stat.st_mtime;
            utime(final_dest, &times);
        }
    }
    
    pthread_mutex_lock(&g_stats_mutex);
    g_stats.files_processed++;
    g_stats.bytes_processed += src_stat.st_size;
    pthread_mutex_unlock(&g_stats_mutex);
    
    log_debug("파일 복원 완료: %s (%ld bytes)", final_dest, src_stat.st_size);
    
    return SUCCESS;
}

int restore_directory(const char *source, const char *dest, const backup_options_t *opts) {
    DIR *dir;
    struct dirent *entry;
    char src_path[MAX_PATH], dest_path[MAX_PATH];
    int result = SUCCESS;
    long total_files = 0;
    
    if (!opts->recursive) {
        log_error("디렉토리 복원에는 재귀 옵션이 필요합니다");
        return ERROR_GENERAL;
    }
    
    log_info("디렉토리 복원 시작: %s -> %s", source, dest);
    
    // 진행률 정보 업데이트
    if (opts->progress) {
        total_files = count_directory_files(source);
        
        pthread_mutex_lock(&g_progress.mutex);
        g_progress.total_files = total_files;
        pthread_mutex_unlock(&g_progress.mutex);
        
        log_info("총 %ld개 파일", total_files);
    }
    
    // 대상 디렉토리 생성
    if (create_directory_recursive(dest) != 0 && errno != EEXIST) {
        log_error("대상 디렉토리 생성 실패: %s", dest);
        return ERROR_GENERAL;
    }
    
    dir = opendir(source);
    if (!dir) {
        log_error("소스 디렉토리를 열 수 없습니다: %s (%s)", source, strerror(errno));
        return ERROR_GENERAL;
    }
    
    while ((entry = readdir(dir)) != NULL && result == SUCCESS) {
        // . 과 .. 건너뛰기
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
            continue;
        }
        
        // 메타데이터 파일 건너뛰기
        if (strcmp(entry->d_name, METADATA_FILE) == 0) {
            continue;
        }
        
        // 중단 요청 확인
        if (g_progress.cancel_requested) {
            result = ERROR_INTERRUPTED;
            break;
        }
        
        snprintf(src_path, sizeof(src_path), "%s/%s", source, entry->d_name);
        snprintf(dest_path, sizeof(dest_path), "%s/%s", dest, entry->d_name);
        
        if (is_directory(src_path)) {
            // 하위 디렉토리 처리
            result = restore_directory(src_path, dest_path, opts);
            if (result == SUCCESS) {
                pthread_mutex_lock(&g_stats_mutex);
                g_stats.dirs_processed++;
                pthread_mutex_unlock(&g_stats_mutex);
            }
        } else if (is_regular_file(src_path)) {
            // 파일 복원
            int restore_result = restore_file(src_path, dest_path, opts);
            if (restore_result != SUCCESS && restore_result != ERROR_INTERRUPTED) {
                log_warning("파일 복원 실패: %s", src_path);
                pthread_mutex_lock(&g_stats_mutex);
                g_stats.files_failed++;
                pthread_mutex_unlock(&g_stats_mutex);
                
                // 개별 파일 실패는 전체 복원을 중단하지 않음
                if (result == SUCCESS) {
                    result = restore_result;
                }
            }
            
            // 진행률 업데이트
            if (opts->progress) {
                pthread_mutex_lock(&g_progress.mutex);
                g_progress.current_files++;
                printf("\r진행률: %ld/%ld 파일 (%ld%%)", 
                       g_progress.current_files, g_progress.total_files,
                       g_progress.total_files > 0 ? (g_progress.current_files * 100 / g_progress.total_files) : 0);
                fflush(stdout);
                pthread_mutex_unlock(&g_progress.mutex);
            }
        }
    }
    
    closedir(dir);
    
    if (opts->progress && result != ERROR_INTERRUPTED) {
        printf("\n");
    }
    
    if (result == SUCCESS) {
        log_info("디렉토리 복원 완료: %s", dest);
    }
    
    return result;
}

int verify_backup(const char *backup_path, const backup_options_t *opts) {
    log_info("백업 검증 시작: %s", backup_path);
    
    if (is_directory(backup_path)) {
        return verify_backup_filesystem(backup_path, opts);
    } else {
        // 단일 파일 검증
        if (!file_exists(backup_path)) {
            log_error("백업 파일이 존재하지 않습니다: %s", backup_path);
            return ERROR_FILE_NOT_FOUND;
        }
        
        log_info("백업 파일 검증 완료: %s", backup_path);
        return SUCCESS;
    }
}

int list_backup_contents(const char *backup_path, const backup_options_t *opts) {
    if (!file_exists(backup_path)) {
        log_error("백업 경로가 존재하지 않습니다: %s", backup_path);
        return ERROR_FILE_NOT_FOUND;
    }
    
    if (is_directory(backup_path)) {
        return list_backup_filesystem(backup_path, opts);
    } else {
        // 단일 파일 정보
        struct stat st;
        if (stat(backup_path, &st) == 0) {
            compression_type_t comp_type = get_compression_type(backup_path);
            
            printf("백업 파일: %s\n", backup_path);
            printf("크기: %s\n", format_bytes(st.st_size));
            printf("수정 시간: %s", ctime(&st.st_mtime));
            printf("압축 타입: %s\n", get_compression_type_string(comp_type));
        }
        return SUCCESS;
    }
}

// 백업 검증 (파일시스템 기반)
int verify_backup_filesystem(const char *backup_dir, const backup_options_t *opts) {
    DIR *dir;
    struct dirent *entry;
    char backup_path[MAX_PATH];
    int verified_count = 0, error_count = 0;
    
    log_info("백업 디렉토리 검증 시작: %s", backup_dir);
    
    dir = opendir(backup_dir);
    if (!dir) {
        log_error("백업 디렉토리를 열 수 없습니다: %s", backup_dir);
        return ERROR_GENERAL;
    }
    
    while ((entry = readdir(dir)) != NULL) {
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0 ||
            strcmp(entry->d_name, METADATA_FILE) == 0) {
            continue;
        }
        
        snprintf(backup_path, sizeof(backup_path), "%s/%s", backup_dir, entry->d_name);
        
        if (is_regular_file(backup_path)) {
            if (access(backup_path, R_OK) == 0) {
                verified_count++;
                if (opts->verbose) {
                    printf("✓ %s\n", entry->d_name);
                }
            } else {
                error_count++;
                log_error("접근 불가능한 파일: %s", backup_path);
            }
        } else if (is_directory(backup_path)) {
            // 재귀적으로 하위 디렉토리 검증
            int sub_result = verify_backup_filesystem(backup_path, opts);
            if (sub_result != SUCCESS) {
                error_count++;
            }
        }
    }
    
    closedir(dir);
    
    if (error_count == 0) {
        log_info("백업 검증 완료: %d개 파일 확인", verified_count);
        return SUCCESS;
    } else {
        log_error("백업 검증 실패: %d개 오류 발견", error_count);
        return ERROR_GENERAL;
    }
}

// 백업 내용 목록 출력 (파일시스템 기반)
int list_backup_filesystem(const char *backup_dir, const backup_options_t *opts) {
    DIR *dir;
    struct dirent *entry;
    struct stat st;
    char backup_path[MAX_PATH];
    int file_count = 0;
    long total_size = 0;
    const char *type_str;
    compression_type_t comp_type;
    
    printf("\n백업 내용:\n");
    printf("%-50s %-15s %-20s %-10s\n", "파일명", "크기", "수정시간", "타입");
    printf("--------------------------------------------------------------------------------\n");
    
    dir = opendir(backup_dir);
    if (!dir) {
        log_error("백업 디렉토리를 열 수 없습니다: %s", backup_dir);
        return ERROR_GENERAL;
    }
    
    while ((entry = readdir(dir)) != NULL) {
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0 ||
            strcmp(entry->d_name, METADATA_FILE) == 0) {
            continue;
        }
        
        snprintf(backup_path, sizeof(backup_path), "%s/%s", backup_dir, entry->d_name);
        
        if (stat(backup_path, &st) == 0) {
            file_count++;
            
            if (S_ISREG(st.st_mode)) {
                total_size += st.st_size;
                
                // 압축 타입 확인
                comp_type = get_compression_type(entry->d_name);
                type_str = get_compression_type_string(comp_type);
                
                char time_str[20];
                struct tm *tm_info = localtime(&st.st_mtime);
                strftime(time_str, sizeof(time_str), "%Y-%m-%d %H:%M", tm_info);
                
                printf("%-50s %-15s %-20s %-10s\n",
                       entry->d_name,
                       format_bytes(st.st_size),
                       time_str,
                       type_str);
            } else if (S_ISDIR(st.st_mode)) {
                char time_str[20];
                struct tm *tm_info = localtime(&st.st_mtime);
                strftime(time_str, sizeof(time_str), "%Y-%m-%d %H:%M", tm_info);
                
                printf("%-50s %-15s %-20s %-10s\n",
                       entry->d_name,
                       "-",
                       time_str,
                       "디렉토리");
            }
        }
    }
    
    closedir(dir);
    
    printf("--------------------------------------------------------------------------------\n");
    printf("총 %d개 파일, %s\n", file_count, format_bytes(total_size));
    
    return SUCCESS;
}
