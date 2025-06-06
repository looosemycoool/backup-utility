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
    if (!file_exists(source)) {
        return -1;
    }
    
    if (options->preserve_metadata) {
        return copy_file_with_metadata(source, destination);
    } else {
        return copy_file(source, destination);
    }
}

int backup_directory(const char *source_dir, const char *backup_dir, const BackupOptions *options) {
    if (create_directory(backup_dir, 0755) != 0) {
        return -1;
    }
    return 0;
}

int perform_backup(const char *source, const char *destination, const BackupOptions *options, BackupStatus *status) {
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
        return -1;
    }
    
    fprintf(index_file, "backup_time=%ld\n", status->end_time);
    fprintf(index_file, "total_files=%d\n", status->total_files);
    fprintf(index_file, "copied_files=%d\n", status->copied_files);
    fprintf(index_file, "total_size=%ld\n", status->total_size);
    
    fclose(index_file);
    return 0;
}

int load_backup_index(const char *backup_dir, time_t *last_backup_time) {
    char index_path[MAX_PATH_LENGTH];
    snprintf(index_path, sizeof(index_path), "%s/.backup_index", backup_dir);
    
    FILE *index_file = fopen(index_path, "r");
    if (!index_file) {
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
    return 0;
}

void print_backup_status(const BackupStatus *status) {
    double duration = difftime(status->end_time, status->start_time);
    
    printf("=== 백업 완료 ===\n");
    printf("전체 파일: %d개\n", status->total_files);
    printf("복사된 파일: %d개\n", status->copied_files);
    printf("건너뛴 파일: %d개\n", status->skipped_files);
    printf("오류 파일: %d개\n", status->error_files);
    printf("전체 크기: %ld바이트\n", status->total_size);
    printf("복사된 크기: %ld바이트\n", status->copied_size);
    printf("소요 시간: %.2f초\n", duration);
}
