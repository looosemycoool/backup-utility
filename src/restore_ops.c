#include "restore_ops.h"
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
    if (!file_exists(backup_path)) {
        return -1;
    }
    
    if (file_exists(destination)) {
        if (handle_file_conflict(destination, options->conflict_resolution) != 0) {
            return 1;
        }
    }
    
    int result;
    if (options->restore_metadata) {
        result = copy_file_with_metadata(backup_path, destination);
    } else {
        result = copy_file(backup_path, destination);
    }
    
    return result;
}

int restore_directory(const char *backup_dir, const char *destination_dir, const RestoreOptions *options) {
    if (create_directory(destination_dir, 0755) != 0) {
        return -1;
    }
    return 0;
}

int perform_restore(const char *backup_path, const char *destination, const RestoreOptions *options, RestoreStatus *status) {
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
    long orig_size = get_file_size(original);
    long rest_size = get_file_size(restored);
    
    if (orig_size != rest_size) {
        return -1;
    }
    
    return 0;
}

int handle_file_conflict(const char *destination, ConflictResolution resolution) {
    switch (resolution) {
        case CONFLICT_OVERWRITE:
            return 0;
        case CONFLICT_SKIP:
            return -1;
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
    
    printf("=== 복원 완료 ===\n");
    printf("전체 파일: %d개\n", status->total_files);
    printf("복원된 파일: %d개\n", status->restored_files);
    printf("건너뛴 파일: %d개\n", status->skipped_files);
    printf("오류 파일: %d개\n", status->error_files);
    printf("전체 크기: %ld바이트\n", status->total_size);
    printf("복원된 크기: %ld바이트\n", status->restored_size);
    printf("소요 시간: %.2f초\n", duration);
}

int list_available_backups(const char *backup_dir) {
    if (!is_directory(backup_dir)) {
        return -1;
    }
    
    printf("백업 목록 기능은 아직 구현 중입니다: %s\n", backup_dir);
    return 0;
}
