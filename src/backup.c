#include "backup.h"

extern int handle_file_conflict(const char *dest, conflict_mode_t mode);

int backup_file(const char *source, const char *dest, const backup_options_t *opts) {
    struct stat src_stat;
    char final_dest[MAX_PATH];
    int counter = 1;
    
    if (stat(source, &src_stat) != 0) {
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
        
        // 이미 올바른 확장자가 있는지 확인
        size_t dest_len = strlen(final_dest);
        size_t ext_len = strlen(ext);
        
        if (dest_len < ext_len || strcmp(final_dest + dest_len - ext_len, ext) != 0) {
            strncat(final_dest, ext, sizeof(final_dest) - strlen(final_dest) - 1);
        }
    }

    // 충돌 처리
    if (file_exists(final_dest)) {
        switch (opts->conflict_mode) {
            case CONFLICT_SKIP:
                log_info("파일 건너뛰기: %s", final_dest);
                pthread_mutex_lock(&g_stats_mutex);
                g_stats.files_skipped++;
                pthread_mutex_unlock(&g_stats_mutex);
                return SUCCESS;
            
            case CONFLICT_RENAME:
                // 자동 이름 변경
                while (file_exists(final_dest) && counter < 1000) {
                    char base_dest[MAX_PATH];
                    strncpy(base_dest, dest, sizeof(base_dest) - 1);
                    base_dest[sizeof(base_dest) - 1] = '\0';
                    
                    snprintf(final_dest, sizeof(final_dest), "%s.%d", base_dest, counter);
                    
                    if (opts->compression != COMPRESS_NONE) {
                        const char *ext = get_compression_extension(opts->compression);
                        strncat(final_dest, ext, sizeof(final_dest) - strlen(final_dest) - 1);
                    }
                    counter++;
                }
                break;
            
            case CONFLICT_ASK:
            case CONFLICT_OVERWRITE:
                if (!handle_file_conflict(final_dest, opts->conflict_mode)) {
                    pthread_mutex_lock(&g_stats_mutex);
                    g_stats.files_skipped++;
                    pthread_mutex_unlock(&g_stats_mutex);
                    return SUCCESS;
                }
                break;
        }
    }

    // DRY RUN 모드
    if (opts->dry_run) {
        printf("DRY RUN: %s -> %s\n", source, final_dest);
        pthread_mutex_lock(&g_stats_mutex);
        g_stats.files_processed++;
        g_stats.bytes_processed += src_stat.st_size;
        pthread_mutex_unlock(&g_stats_mutex);
        return SUCCESS;
    }

    // Verbose 모드 출력
    if (opts->verbose) {
        printf("백업: %s -> %s\n", source, final_dest);
    }

    // 실제 백업 수행
    int result;
    if (opts->compression != COMPRESS_NONE) {
        result = compress_file(source, final_dest, opts->compression);
    } else {
        result = copy_file_simple(source, final_dest);
    }

    if (result != SUCCESS) {
        log_error("파일 백업 실패: %s", source);
        pthread_mutex_lock(&g_stats_mutex);
        g_stats.files_failed++;
        pthread_mutex_unlock(&g_stats_mutex);
        return result;
    }

    // 메타데이터 복사
    if (opts->preserve_permissions || opts->preserve_timestamps) {
        copy_file_metadata(source, final_dest);
    }

    // 통계 업데이트
    pthread_mutex_lock(&g_stats_mutex);
    g_stats.files_processed++;
    g_stats.bytes_processed += src_stat.st_size;
    
    if (opts->compression != COMPRESS_NONE) {
        struct stat dest_stat;
        if (stat(final_dest, &dest_stat) == 0) {
            g_stats.bytes_compressed += dest_stat.st_size;
        }
    } else {
        g_stats.bytes_compressed += src_stat.st_size;
    }
    pthread_mutex_unlock(&g_stats_mutex);

    // 진행률 업데이트
    if (opts->progress) {
        update_progress(g_stats.files_processed, g_stats.bytes_processed);
    }

    log_debug("파일 백업 완료: %s -> %s", source, final_dest);
    return SUCCESS;
}

int backup_directory(const char *source, const char *dest, const backup_options_t *opts) {
    // 대상 디렉토리 생성
    if (!file_exists(dest)) {
        if (create_directory_recursive(dest) != SUCCESS) {
            log_error("디렉토리 생성 실패: %s", dest);
            return ERROR_FILE_WRITE;
        }
    }

    pthread_mutex_lock(&g_stats_mutex);
    g_stats.directories_processed++;
    pthread_mutex_unlock(&g_stats_mutex);

    return SUCCESS;
}

int backup_directory_recursive(const char *source, const char *dest, const backup_options_t *opts) {
    DIR *dir;
    struct dirent *entry;
    char src_path[MAX_PATH];
    char dest_path[MAX_PATH];
    int result = SUCCESS;
    size_t total_files = 0;
    size_t total_bytes = 0;

    // 먼저 전체 파일 수와 크기 계산 (진행률 표시용)
    if (opts->progress) {
        log_info("파일 스캔 중...");
        
        // 간단한 파일 카운팅
        dir = opendir(source);
        if (dir) {
            while ((entry = readdir(dir)) != NULL) {
                if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
                    continue;
                }
                
                snprintf(src_path, sizeof(src_path), "%s/%s", source, entry->d_name);
                
                if (should_include_file(src_path, opts)) {
                    struct stat st;
                    if (stat(src_path, &st) == 0 && S_ISREG(st.st_mode)) {
                        total_files++;
                        total_bytes += st.st_size;
                    }
                }
            }
            closedir(dir);
            
            log_info("총 %zu개 파일, %zu bytes", total_files, total_bytes);
            init_progress(total_files, total_bytes);
        }
    }

    // 대상 디렉토리 생성
    if (backup_directory(source, dest, opts) != SUCCESS) {
        return ERROR_FILE_WRITE;
    }

    // 소스 디렉토리 열기
    dir = opendir(source);
    if (!dir) {
        log_error("디렉토리 열기 실패: %s", source);
        return ERROR_FILE_OPEN;
    }

    size_t current_file = 0;
    
    // 디렉토리 내용 순회
    while ((entry = readdir(dir)) != NULL) {
        // . 및 .. 건너뛰기
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
            continue;
        }

        // 전체 경로 생성
        snprintf(src_path, sizeof(src_path), "%s/%s", source, entry->d_name);
        snprintf(dest_path, sizeof(dest_path), "%s/%s", dest, entry->d_name);

        // 제외 패턴 확인
        if (!should_include_file(src_path, opts)) {
            log_debug("파일 제외: %s", src_path);
            continue;
        }

        struct stat st;
        if (stat(src_path, &st) != 0) {
            log_warning("파일 정보 가져오기 실패: %s", src_path);
            continue;
        }

        if (S_ISDIR(st.st_mode)) {
            // 하위 디렉토리 재귀 처리
            log_debug("하위 디렉토리 처리: %s", src_path);
            int backup_result = backup_directory_recursive(src_path, dest_path, opts);
            if (backup_result != SUCCESS) {
                log_warning("하위 디렉토리 백업 실패: %s", src_path);
                result = backup_result;
            }
        } else if (S_ISREG(st.st_mode)) {
            // 일반 파일 백업
            current_file++;
            
            if (opts->progress && total_files > 0) {
                int percentage = (int)((current_file * 100) / total_files);
                printf("진행률: %zu/%zu 파일 (%d%%)\r", current_file, total_files, percentage);
                fflush(stdout);
            }
            
            int backup_result = backup_file(src_path, dest_path, opts);
            if (backup_result != SUCCESS) {
                log_warning("파일 백업 실패: %s", src_path);
                result = backup_result;
            }
        } else {
            log_debug("특수 파일 건너뛰기: %s", src_path);
        }
    }

    closedir(dir);

    if (opts->progress) {
        printf("\n"); // 진행률 출력 후 줄바꿈
        finish_progress();
    }

    return result;
}

int verify_backup_integrity(const char *source, const char *backup, const backup_options_t *opts) {
    if (!opts->verify) {
        return SUCCESS; // 검증 비활성화
    }
    
    if (opts->compression != COMPRESS_NONE) {
        // 압축 파일의 실제 경로 생성 (확장자 추가)
        char actual_backup[MAX_PATH];
        const char *ext = get_compression_extension(opts->compression);
        snprintf(actual_backup, sizeof(actual_backup), "%s%s", backup, ext);
        
        // 압축된 파일은 압축 해제 후 비교
        char temp_file[MAX_PATH];
        snprintf(temp_file, sizeof(temp_file), "/tmp/backup_verify_%d", getpid());
        
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