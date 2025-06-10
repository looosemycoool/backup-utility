#include "backup.h"

static pthread_mutex_t g_stats_mutex = PTHREAD_MUTEX_INITIALIZER;

int backup_file(const char *source, const char *dest, const backup_options_t *opts) {
    struct stat src_stat;
    char final_dest[MAX_PATH];
    int counter = 1;
    
     if (stat(source, &src_stat) != 0) {  // ✅ 수정!
        log_error("파일 정보를 가져올 수 없습니다: %s", source);
        return ERROR_FILE_OPEN;
    }
    
    
    if (!should_include_file(source, opts)) {
        log_debug("파일 제외: %s", source);
        pthread_mutex_lock(&g_stats_mutex);
        g_stats.files_skipped++;
        pthread_mutex_unlock(&g_stats_mutex);
        return SUCCESS;
    }
    
    strncpy(final_dest, dest, sizeof(final_dest) - 1);
    final_dest[sizeof(final_dest) - 1] = '\0';
    
    // 압축 확장자 추가
    if (opts->compression != COMPRESS_NONE) {
        const char *ext = get_compression_extension(opts->compression);
        const char *last_dot = strrchr(final_dest, '.');
        if (!last_dot || strcmp(last_dot, ext) != 0) {
        strncat(final_dest, ext, sizeof(final_dest) - strlen(final_dest) - 1);
        }
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
                        snprintf(new_dest, sizeof(new_dest), "%s.%d", final_dest, counter++);
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
        printf("백업: %s -> %s\n", source, final_dest);
    }
    
    // 파일 백업 (압축 여부에 따라)
    int result = compress_file(source, final_dest, opts->compression);
    
    if (result != SUCCESS) {
        log_error("파일 백업 실패: %s", source);
        pthread_mutex_lock(&g_stats_mutex);
        g_stats.files_failed++;
        pthread_mutex_unlock(&g_stats_mutex);
        return result;
    }
    
    // 권한 및 타임스탬프 보존
    if (opts->preserve_permissions || opts->preserve_timestamps) {
        struct stat src_stat;
        if (stat(source, &src_stat) == 0) {
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
    }
    
    pthread_mutex_lock(&g_stats_mutex);
    g_stats.files_processed++;
    g_stats.bytes_processed += src_stat.st_size;
    pthread_mutex_unlock(&g_stats_mutex);
    
    log_debug("파일 백업 완료: %s (%ld bytes)", source, src_stat.st_size);
    
    return SUCCESS;
}

int backup_directory(const char *source, const char *dest, const backup_options_t *opts) {
    DIR *dir;
    struct dirent *entry;
    char src_path[MAX_PATH], dest_path[MAX_PATH];
    int result = SUCCESS;
    long total_files = 0, total_bytes = 0;
    
    if (!opts->recursive) {
        log_error("디렉토리 백업에는 재귀 옵션이 필요합니다");
        return ERROR_GENERAL;
    }
    
    log_info("디렉토리 백업 시작: %s -> %s", source, dest);
    
    // 진행률 정보 업데이트
    if (opts->progress) {
        total_files = count_directory_files(source);
        total_bytes = calculate_directory_size(source);
        
        pthread_mutex_lock(&g_progress.mutex);
        g_progress.total_files = total_files;
        g_progress.total_bytes = total_bytes;
        pthread_mutex_unlock(&g_progress.mutex);
        
        log_info("총 %ld개 파일, %ld bytes", total_files, total_bytes);
    }
    
    // 대상 디렉토리 생성
    if (create_directory_recursive(dest) != 0 && errno != EEXIST) {
        log_error("대상 디렉토리 생성 실패: %s", dest);
        return ERROR_GENERAL;
    }
    
    result = backup_directory_recursive(source, dest, opts);
    
    if (result == SUCCESS) {
        pthread_mutex_lock(&g_stats_mutex);
        g_stats.dirs_processed++;
        pthread_mutex_unlock(&g_stats_mutex);
        log_info("디렉토리 백업 완료: %s", source);
    }
    
    return result;
}

 int backup_directory_recursive(const char *source, const char *dest, const backup_options_t *opts) {
    DIR *dir;
    struct dirent *entry;
    char src_path[MAX_PATH], dest_path[MAX_PATH];
    int result = SUCCESS;
    
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
        
        // 중단 요청 확인
        if (g_progress.cancel_requested) {
            result = ERROR_INTERRUPTED;
            break;
        }
        
        snprintf(src_path, sizeof(src_path), "%s/%s", source, entry->d_name);
        snprintf(dest_path, sizeof(dest_path), "%s/%s", dest, entry->d_name);
        
        if (is_directory(src_path)) {
            // 하위 디렉토리 처리
            if (create_directory(dest_path) == SUCCESS || errno == EEXIST) {
                result = backup_directory_recursive(src_path, dest_path, opts);
                if (result == SUCCESS) {
                    pthread_mutex_lock(&g_stats_mutex);
                    g_stats.dirs_processed++;
                    pthread_mutex_unlock(&g_stats_mutex);
                }
            } else {
                log_error("디렉토리 생성 실패: %s", dest_path);
                result = ERROR_GENERAL;
            }
        } else if (is_regular_file(src_path)) {
            // 파일 백업
            int backup_result = backup_file(src_path, dest_path, opts);
            if (backup_result != SUCCESS && backup_result != ERROR_INTERRUPTED) {
                log_warning("파일 백업 실패: %s", src_path);
                pthread_mutex_lock(&g_stats_mutex);
                g_stats.files_failed++;
                pthread_mutex_unlock(&g_stats_mutex);
                
                // 개별 파일 실패는 전체 백업을 중단하지 않음
                if (result == SUCCESS) {
                    result = backup_result;
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
    
    return result;
}

int verify_backup_integrity(const char *source, const char *backup, const backup_options_t *opts) {
    if (!opts->verify) {
        return SUCCESS; // 검증 비활성화
    }
    
    if (opts->compression != COMPRESS_NONE) {
        // ✅ 압축 파일의 실제 경로 생성 (확장자 추가)
        char actual_backup[MAX_PATH];
        const char *ext = get_compression_extension(opts->compression);
        snprintf(actual_backup, sizeof(actual_backup), "%s%s", backup, ext);
        
        // 압축된 파일은 압축 해제 후 비교
        char temp_file[MAX_PATH];
        snprintf(temp_file, sizeof(temp_file), "/tmp/backup_verify_%d", getpid());
        
        // ✅ actual_backup 사용 (원래는 backup이었음)
        if (decompress_file(actual_backup, temp_file, opts->compression) != SUCCESS) {
            log_error("검증을 위한 압축 해제 실패: %s", actual_backup);
            return ERROR_COMPRESSION;
        }
        
        int result = compare_files(source, temp_file);
        unlink(temp_file);
        
        if (!result) {
            log_error("백업 검증 실패 (압축): %s", source);
            return ERROR_CHECKSUM;
        }
    } else {
        // 압축되지 않은 파일 직접 비교
        if (!compare_files(source, backup)) {
            log_error("백업 검증 실패: %s", source);
            return ERROR_CHECKSUM;
        }
    }
    
    log_debug("백업 검증 성공: %s", source);
    return SUCCESS;
}