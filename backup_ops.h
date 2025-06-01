#ifndef BACKUP_OPS_H
#define BACKUP_OPS_H

#include "file_utils.h"
#include <time.h>

// 백업 옵션 구조체
typedef struct {
    int incremental;         // 증분 백업 여부 (1: 증분, 0: 전체)
    int recursive;           // 재귀적 백업 여부
    int preserve_metadata;   // 메타데이터 보존 여부
    int verbose;            // 상세 출력 여부
    char exclude_patterns[10][256];  // 제외할 패턴들 (최대 10개)
    int exclude_count;      // 제외 패턴 개수
} BackupOptions;

// 백업 상태 구조체
typedef struct {
    int total_files;        // 전체 파일 수
    int copied_files;       // 복사된 파일 수
    int skipped_files;      // 건너뛴 파일 수
    int error_files;        // 오류 발생 파일 수
    long total_size;        // 전체 크기
    long copied_size;       // 복사된 크기
    time_t start_time;      // 시작 시간
    time_t end_time;        // 종료 시간
} BackupStatus;

// 함수 선언
int backup_file(const char *source, const char *destination, const BackupOptions *options);
int backup_directory(const char *source_dir, const char *backup_dir, const BackupOptions *options);
int perform_backup(const char *source, const char *destination, const BackupOptions *options, BackupStatus *status);
int create_backup_index(const char *backup_dir, const BackupStatus *status);
int load_backup_index(const char *backup_dir, time_t *last_backup_time);
void init_backup_options(BackupOptions *options);
void print_backup_status(const BackupStatus *status);

#endif // BACKUP_OPS_H
