#include "backup_ops.h"
#include "logging.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

void init_backup_options(BackupOptions *options) {
    options->incremental = 0;
    options->recursive = 0;
    options->preserve_metadata = 1;
    options->verbose = 0;
    options->exclude_count = 0;
}

int backup_file(const char *source, const char *destination, const BackupOptions *options) {
    log_message(LOG_DEBUG, "파일 백업 시작: %s -> %s", source, destination);
    
    if (!file_exists(source)) {
        log_message(LOG_ERROR, "소스 파일이 존재하지 않습니다: %s", source);
        return -1;
    }
    
    // 메타데이터 보존 옵션에 따라 복사 방법 결정
    if (options->preserve_metadata) {
        return copy_file_with_metadata(source, destination);
    } else {
        return copy_file(source, destination);
    }
}

int backup_directory(const char *source_dir, const char *backup_dir, const BackupOptions *options) {
    log_message(LOG_INFO, "디렉토리 백업 기능은 아직 구현 중입니다: %s -> %s", source_dir, backup_dir);
    
    // 기본적으로 목적지 디렉토리 생성
    if (create_directory(backup_dir, 0755) != 0) {
        log_message(LOG_ERROR, "백업 디렉토리 생성 실패: %s", backup_dir);
        return -1;
    }
    
    return 0;
}

int perform_backup(const char *source, const char *destination, const BackupOptions *options, BackupStatus *status) {
    log_message(LOG_INFO, "백업 수행: %s -> %s", source, destination);
    
    // 백업 상태 초기화
    status->total_files = 0;
    status->copied_files = 0;
    status->skipped_files = 0;
    status->error_files = 0;
    status->total_size = 0;
    status->copied_size = 0;
    time(&status->start_time);
    
    int result;
    
    if (is_directory(source)) {
        if (!options->recursive) {
            log_message(LOG_WARNING, "디렉토리 백업에는 -r 옵션이 필요합니다");
            return -1;
        }
        result = backup_directory(source, destination, options);
    } else {
        result = backup_file(source, destination, options);
        if (result == 0) {
            status->total_files = 1;
            status->copied_files = 1;
            status->total_size = get_file_size(source);
            status->copied_size = status->total_size;
        } else {
            status->total_files = 1;
            status->error_files = 1;
        }
    }
    
    time(&status->end_time);
    return result;
}

int create_backup_index(const char *backup_dir, const BackupStatus *status) {
    char index_path[MAX_PATH_LENGTH];
    snprintf(index_path, sizeof(index_path), "%s/.backup_index", backup_dir);
    
    FILE *index_file = fopen(index_path, "w");
    if (!index_file) {
        log_message(LOG_ERROR, "백업 인덱스 파일 생성 실패: %s", index_path);
        return -1;
    }
    
    fprintf(index_file, "backup_time=%ld\n", status->end_time);
    fprintf(index_file, "total_files=%d\n", status->total_files);
    fprintf(index_file, "copied_files=%d\n", status->copied_files);
    fprintf(index_file, "total_size=%ld\n", status->total_size);
    
    fclose(index_file);
    log_message(LOG_DEBUG, "백업 인덱스 생성 완료: %s", index_path);
    return 0;
}

int load_backup_index(const char *backup_dir, time_t *last_backup_time) {
    char index_path[MAX_PATH_LENGTH];
    snprintf(index_path, sizeof(index_path), "%s/.backup_index", backup_dir);
    
    FILE *index_file = fopen(index_path, "r");
    if (!index_file) {
        log_message(LOG_DEBUG, "백업 인덱스 파일이 없습니다: %s", index_path);
        *last_backup_time = 0;
        return 0;
    }
    
    char line[256];
    while (fgets(line, sizeof(line), index_file)) {
        if (strncmp(line, "backup_time=", 12) == 0) {
            *last_backup_time = (time_t)atol(line + 12);
            break;
        }
    }
    
    fclose(index_file);
    log_message(LOG_DEBUG, "백업 인덱스 로드 완료: %s", index_path);
    return 0;
}

void print_backup_status(const BackupStatus *status) {
    double duration = difftime(status->end_time, status->start_time);
    
    log_message(LOG_INFO, "=== 백업 완료 ===");
    log_message(LOG_INFO, "전체 파일: %d개", status->total_files);
    log_message(LOG_INFO, "복사된 파일: %d개", status->copied_files);
    log_message(LOG_INFO, "건너뛴 파일: %d개", status->skipped_files);
    log_message(LOG_INFO, "오류 파일: %d개", status->error_files);
    log_message(LOG_INFO, "전체 크기: %ld바이트", status->total_size);
    log_message(LOG_INFO, "복사된 크기: %ld바이트", status->copied_size);
    log_message(LOG_INFO, "소요 시간: %.2f초", duration);
}
