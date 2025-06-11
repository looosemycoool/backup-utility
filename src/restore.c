#include "backup.h"

extern int handle_file_conflict(const char *dest, conflict_mode_t mode);

int restore_file(const char *source, const char *dest, const backup_options_t *opts) {
    struct stat src_stat;
    char temp_dest[MAX_PATH];
    compression_type_t comp_type;
    
    if (stat(source, &src_stat) != 0) {
        log_error("백업 파일 정보를 가져올 수 없습니다: %s", source);
        return ERROR_FILE_OPEN;
    }

    // 압축 타입 자동 감지
    comp_type = get_compression_type(source);
    
    strncpy(temp_dest, dest, sizeof(temp_dest) - 1);
    temp_dest[sizeof(temp_dest) - 1] = '\0';

    // 충돌 처리
    if (file_exists(temp_dest)) {
        switch (opts->conflict_mode) {
            case CONFLICT_SKIP:
                log_info("파일 건너뛰기: %s", temp_dest);
                pthread_mutex_lock(&g_stats_mutex);
                g_stats.files_skipped++;
                pthread_mutex_unlock(&g_stats_mutex);
                return SUCCESS;
            
            case CONFLICT_RENAME:
                // 자동 이름 변경
                int counter = 1;
                char base_dest[MAX_PATH];
                strncpy(base_dest, dest, sizeof(base_dest) - 1);
                base_dest[sizeof(base_dest) - 1] = '\0';
                
                while (file_exists(temp_dest) && counter < 1000) {
                    snprintf(temp_dest, sizeof(temp_dest), "%s.%d", base_dest, counter);
                    counter++;
                }
                break;
            
            case CONFLICT_ASK:
            case CONFLICT_OVERWRITE:
                if (!handle_file_conflict(temp_dest, opts->conflict_mode)) {
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
        printf("DRY RUN 복원: %s -> %s\n", source, temp_dest);
        pthread_mutex_lock(&g_stats_mutex);
        g_stats.files_processed++;
        g_stats.bytes_processed += src_stat.st_size;
        pthread_mutex_unlock(&g_stats_mutex);
        return SUCCESS;
    }

    // Verbose 모드 출력
    if (opts->verbose) {
        printf("복원: %s -> %s\n", source, temp_dest);
    }

    // 실제 복원 수행
    int result;
    if (comp_type != COMPRESS_NONE) {
        result = decompress_file(source, temp_dest, comp_type);
    } else {
        result = copy_file_simple(source, temp_dest);
    }

    if (result != SUCCESS) {
        log_error("파일 복원 실패: %s", source);
        pthread_mutex_lock(&g_stats_mutex);
        g_stats.files_failed++;
        pthread_mutex_unlock(&g_stats_mutex);
        return result;
    }

    // 메타데이터 복원
    if (opts->preserve_permissions || opts->preserve_timestamps) {
        copy_file_metadata(source, temp_dest);
    }

    // 통계 업데이트
    pthread_mutex_lock(&g_stats_mutex);
    g_stats.files_processed++;
    g_stats.bytes_processed += src_stat.st_size;
    
    struct stat dest_stat;
    if (stat(temp_dest, &dest_stat) == 0) {
        g_stats.bytes_compressed += dest_stat.st_size;
    }
    pthread_mutex_unlock(&g_stats_mutex);

    // 진행률 업데이트
    if (opts->progress) {
        update_progress(g_stats.files_processed, g_stats.bytes_processed);
    }

    log_debug("파일 복원 완료: %s -> %s", source, temp_dest);
    return SUCCESS;
}

int restore_directory(const char *source, const char *dest, const backup_options_t *opts) {
    // 대상 디렉토리 생성
    if (!file_exists(dest)) {
        if (create_directory_recursive(dest) != SUCCESS) {
            log_error("디렉토리 생성 실패: %s", dest);
            return ERROR_FILE_WRITE;
        }
    }

    // 메타데이터 복원
    if (opts->preserve_permissions || opts->preserve_timestamps) {
        copy_file_metadata(source, dest);
    }

    pthread_mutex_lock(&g_stats_mutex);
    g_stats.directories_processed++;
    pthread_mutex_unlock(&g_stats_mutex);

    return SUCCESS;
}

int restore_directory_recursive(const char *source, const char *dest, const backup_options_t *opts) {
    DIR *dir;
    struct dirent *entry;
    char src_path[MAX_PATH];
    char dest_path[MAX_PATH];
    int result = SUCCESS;
    size_t total_files = 0;
    size_t total_bytes = 0;

    // 먼저 전체 파일 수와 크기 계산 (진행률 표시용)
    if (opts->progress) {
        log_info("백업 파일 스캔 중...");
        
        dir = opendir(source);
        if (dir) {
            while ((entry = readdir(dir)) != NULL) {
                if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
                    continue;
                }
                
                snprintf(src_path, sizeof(src_path), "%s/%s", source, entry->d_name);
                
                struct stat st;
                if (stat(src_path, &st) == 0 && S_ISREG(st.st_mode)) {
                    total_files++;
                    total_bytes += st.st_size;
                }
            }
            closedir(dir);
            
            log_info("총 %zu개 백업 파일, %zu bytes", total_files, total_bytes);
            init_progress(total_files, total_bytes);
        }
    }

    // 대상 디렉토리 생성
    if (restore_directory(source, dest, opts) != SUCCESS) {
        return ERROR_FILE_WRITE;
    }

    // 소스 디렉토리 열기
    dir = opendir(source);
    if (!dir) {
        log_error("백업 디렉토리 열기 실패: %s", source);
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
        
        // 대상 경로 생성 (압축 확장자 제거)
        char dest_name[MAX_PATH];
        strncpy(dest_name, entry->d_name, sizeof(dest_name) - 1);
        dest_name[sizeof(dest_name) - 1] = '\0';
        
        // 압축 확장자 제거
        compression_type_t comp_type = get_compression_type(dest_name);
        if (comp_type != COMPRESS_NONE) {
            const char *ext = get_compression_extension(comp_type);
            size_t name_len = strlen(dest_name);
            size_t ext_len = strlen(ext);
            
            if (name_len > ext_len && strcmp(dest_name + name_len - ext_len, ext) == 0) {
                dest_name[name_len - ext_len] = '\0';
            }
        }
        
        snprintf(dest_path, sizeof(dest_path), "%s/%s", dest, dest_name);

        struct stat st;
        if (stat(src_path, &st) != 0) {
            log_warning("백업 파일 정보 가져오기 실패: %s", src_path);
            continue;
        }

        if (S_ISDIR(st.st_mode)) {
            // 하위 디렉토리 재귀 처리
            log_debug("하위 디렉토리 복원: %s", src_path);
            int restore_result = restore_directory_recursive(src_path, dest_path, opts);
            if (restore_result != SUCCESS) {
                log_warning("하위 디렉토리 복원 실패: %s", src_path);
                result = restore_result;
            }
        } else if (S_ISREG(st.st_mode)) {
            // 일반 파일 복원
            current_file++;
            
            if (opts->progress && total_files > 0) {
                int percentage = (int)((current_file * 100) / total_files);
                printf("복원 진행률: %zu/%zu 파일 (%d%%)\r", current_file, total_files, percentage);
                fflush(stdout);
            }
            
            int restore_result = restore_file(src_path, dest_path, opts);
            if (restore_result != SUCCESS) {
                log_warning("파일 복원 실패: %s", src_path);
                result = restore_result;
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

// 백업 메타데이터 읽기 함수
int read_backup_metadata(const char *backup_path, backup_stats_t *metadata) {
    char metadata_file[MAX_PATH];
    snprintf(metadata_file, sizeof(metadata_file), "%s/.backup_metadata", backup_path);
    
    FILE *file = fopen(metadata_file, "r");
    if (!file) {
        log_warning("백업 메타데이터 파일이 없습니다: %s", metadata_file);
        return ERROR_FILE_NOT_FOUND;
    }
    
    if (fread(metadata, sizeof(backup_stats_t), 1, file) != 1) {
        log_error("백업 메타데이터 읽기 실패: %s", metadata_file);
        fclose(file);
        return ERROR_FILE_READ;
    }
    
    fclose(file);
    log_debug("백업 메타데이터 읽기 완료: %s", metadata_file);
    return SUCCESS;
}

// 백업 메타데이터 쓰기 함수
int write_backup_metadata(const char *backup_path, const backup_stats_t *metadata) {
    char metadata_file[MAX_PATH];
    snprintf(metadata_file, sizeof(metadata_file), "%s/.backup_metadata", backup_path);
    
    FILE *file = fopen(metadata_file, "w");
    if (!file) {
        log_error("백업 메타데이터 파일 생성 실패: %s", metadata_file);
        return ERROR_FILE_WRITE;
    }
    
    if (fwrite(metadata, sizeof(backup_stats_t), 1, file) != 1) {
        log_error("백업 메타데이터 쓰기 실패: %s", metadata_file);
        fclose(file);
        return ERROR_FILE_WRITE;
    }
    
    fclose(file);
    log_debug("백업 메타데이터 쓰기 완료: %s", metadata_file);
    return SUCCESS;
}

// 백업 인덱스 생성 함수
int create_backup_index(const char *backup_path) {
    char index_file[MAX_PATH];
    snprintf(index_file, sizeof(index_file), "%s/.backup_index", backup_path);
    
    FILE *file = fopen(index_file, "w");
    if (!file) {
        log_error("백업 인덱스 파일 생성 실패: %s", index_file);
        return ERROR_FILE_WRITE;
    }
    
    // 백업 인덱스 헤더 작성
    fprintf(file, "# Backup Index File\n");
    fprintf(file, "# Generated: %s", ctime(&g_stats.start_time));
    fprintf(file, "# Format: <type>|<path>|<size>|<mtime>|<checksum>\n");
    
    fclose(file);
    log_debug("백업 인덱스 생성 완료: %s", index_file);
    return SUCCESS;
}

// 인크리멘털/차등 백업을 위한 변경 감지
int detect_file_changes(const char *source_path, const char *backup_path, 
                       backup_mode_t mode, int *needs_backup) {
    struct stat src_stat, backup_stat;
    char backup_file[MAX_PATH];
    
    *needs_backup = 1; // 기본적으로 백업 필요
    
    if (stat(source_path, &src_stat) != 0) {
        return ERROR_FILE_NOT_FOUND;
    }
    
    // 백업 파일 경로 생성
    const char *filename = strrchr(source_path, '/');
    filename = filename ? filename + 1 : source_path;
    snprintf(backup_file, sizeof(backup_file), "%s/%s", backup_path, filename);
    
    // 백업 파일이 존재하지 않으면 백업 필요
    if (stat(backup_file, &backup_stat) != 0) {
        *needs_backup = 1;
        return SUCCESS;
    }
    
    switch (mode) {
        case BACKUP_FULL:
            *needs_backup = 1; // 항상 백업
            break;
            
        case BACKUP_INCREMENTAL:
        case BACKUP_DIFFERENTIAL:
            // 수정 시간 비교
            if (src_stat.st_mtime > backup_stat.st_mtime) {
                *needs_backup = 1;
            } else {
                *needs_backup = 0;
            }
            break;
    }
    
    return SUCCESS;
}

// 백업 체인 검증 (인크리멘털 백업용)
int verify_backup_chain(const char *backup_base_path) {
    char full_backup_path[MAX_PATH];
    snprintf(full_backup_path, sizeof(full_backup_path), "%s/full", backup_base_path);
    
    if (!file_exists(full_backup_path)) {
        log_error("전체 백업이 없습니다. 인크리멘털 백업을 수행하려면 먼저 전체 백업을 생성하세요.");
        return ERROR_FILE_NOT_FOUND;
    }
    
    log_debug("백업 체인 검증 완료");
    return SUCCESS;
}