k#include "restore_ops.h"
#include "logging.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

void init_restore_options(RestoreOptions *options) {
    options->conflict_resolution = CONFLICT_ASK;
    options->restore_metadata = 1;
    options->recursive = 0;
    options->verify_after_restore = 0;
    options->verbose = 0;
    options->restore_time = 0;
}

int restore_file(const char *backup_path, const char *destination, const RestoreOptions *options) {
    log_message(LOG_DEBUG, "파일 복원 시작: %s -> %s", backup_path, destination);
    
    if (!file_exists(backup_path)) {
        log_message(LOG_ERROR, "백업 파일이 존재하지 않습니다: %s", backup_path);
        return -1;
    }
    
    // 목적지 파일이 이미 존재하는 경우 충돌 처리
    if (file_exists(destination)) {
        if (handle_file_conflict(destination, options->conflict_resolution) != 0) {
            log_message(LOG_INFO, "파일 복원 건너뜀: %s", destination);
            return 1; // 건너뜀을 나타내는 특별한 반환값
        }
    }
    
    // 메타데이터 복원 옵션에 따라 복사 방법 결정
    int result;
    if (options->restore_metadata) {
        result = copy_file_with_metadata(backup_path, destination);
    } else {
        result = copy_file(backup_path, destination);
    }
    
    // 복원 후 검증
    if (result == 0 && options->verify_after_restore) {
        if (verify_restored_file(backup_path, destination) != 0) {
            log_message(LOG_WARNING, "복원된 파일 검증 실패: %s", destination);
        }
    }
    
    return result;
}

int restore_directory(const char *backup_dir, const char *destination_dir, const RestoreOptions *options) {
    log_message(LOG_INFO, "디렉토리 복원 기능은 아직 구현 중입니다: %s -> %s", backup_dir, destination_dir);
    
    // 기본적으로 목적지 디렉토리 생성
    if (create_directory(destination_dir, 0755) != 0) {
        log_message(LOG_ERROR, "복원 디렉토리 생성 실패: %s", destination_dir);
        return -1;
    }
    
    return 0;
}

int perform_restore(const char *backup_path, const char *destination, const RestoreOptions *options, RestoreStatus *status) {
    log_message(LOG_INFO, "복원 수행: %s -> %s", backup_path, destination);
    
    // 복원 상태 초기화
    status->total_files = 0;
    status->restored_files = 0;
    status->skipped_files = 0;
    status->error_files = 0;
    status->total_size = 0;
    status->restored_size = 0;
    time(&status->start_time);
    
    int result;
    
    if (is_directory(backup_path)) {
        if (!options->recursive) {
            log_message(LOG_WARNING, "디렉토리 복원에는 -r 옵션이 필요합니다");
            return -1;
        }
        result = restore_directory(backup_path, destination, options);
    } else {
        result = restore_file(backup_path, destination, options);
        status->total_files = 1;
        if (result == 0) {
            status->restored_files = 1;
            status->total_size = get_file_size(backup_path);
            status->restored_size = status->total_size;
        } else if (result == 1) {
            status->skipped_files = 1;
        } else {
            status->error_files = 1;
        }
    }
    
    time(&status->end_time);
    return result;
}

int verify_restored_file(const char *original, const char *restored) {
    log_message(LOG_DEBUG, "파일 검증: %s vs %s", original, restored);
    
    // 파일 크기 비교
    long orig_size = get_file_size(original);
    long rest_size = get_file_size(restored);
    
    if (orig_size != rest_size) {
        log_message(LOG_ERROR, "파일 크기 불일치: %ld vs %ld", orig_size, rest_size);
        return -1;
    }
    
    log_message(LOG_DEBUG, "파일 검증 성공");
    return 0;
}

int handle_file_conflict(const char *destination, ConflictResolution resolution) {
    switch (resolution) {
        case CONFLICT_OVERWRITE:
            log_message(LOG_DEBUG, "파일 덮어쓰기: %s", destination);
            return 0;
            
        case CONFLICT_SKIP:
            log_message(LOG_DEBUG, "파일 건너뛰기: %s", destination);
            return -1;
            
        case CONFLICT_RENAME:
            log_message(LOG_INFO, "파일 이름 변경 기능은 아직 구현 중입니다: %s", destination);
            return 0;
            
        case CONFLICT_ASK:
            printf("파일이 이미 존재합니다: %s\n", destination);
            printf("덮어쓰시겠습니까? (y/n): ");
            char response;
            scanf(" %c", &response);
            if (response == 'y' || response == 'Y') {
                return 0;
            } else {
                return -1;
            }
            
        default:
            return 0;
    }
}

void print_restore_status(const RestoreStatus *status) {
    double duration = difftime(status->end_time, status->start_time);
    
    log_message(LOG_INFO, "=== 복원 완료 ===");
    log_message(LOG_INFO, "전체 파일: %d개", status->total_files);
    log_message(LOG_INFO, "복원된 파일: %d개", status->restored_files);
    log_message(LOG_INFO, "건너뛴 파일: %d개", status->skipped_files);
    log_message(LOG_INFO, "오류 파일: %d개", status->error_files);
    log_message(LOG_INFO, "전체 크기: %ld바이트", status->total_size);
    log_message(LOG_INFO, "복원된 크기: %ld바이트", status->restored_size);
    log_message(LOG_INFO, "소요 시간: %.2f초", duration);
}

int list_available_backups(const char *backup_dir) {
    log_message(LOG_INFO, "백업 목록 표시: %s", backup_dir);
    
    if (!is_directory(backup_dir)) {
        log_message(LOG_ERROR, "백업 디렉토리가 존재하지 않습니다: %s", backup_dir);
        return -1;
    }
    
    log_message(LOG_INFO, "백업 목록 기능은 아직 구현 중입니다");
    return 0;
}
