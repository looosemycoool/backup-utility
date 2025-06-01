#ifndef RESTORE_OPS_H
#define RESTORE_OPS_H

#include "file_utils.h"
#include <time.h>

// 복원 시 충돌 처리 방법
typedef enum {
    CONFLICT_OVERWRITE = 0,  // 덮어쓰기
    CONFLICT_SKIP = 1,       // 건너뛰기
    CONFLICT_RENAME = 2,     // 이름 변경
    CONFLICT_ASK = 3         // 사용자에게 묻기
} ConflictResolution;

// 복원 옵션 구조체
typedef struct {
    ConflictResolution conflict_resolution;  // 충돌 처리 방법
    int restore_metadata;    // 메타데이터 복원 여부
    int recursive;          // 재귀적 복원 여부
    int verify_after_restore;  // 복원 후 검증 여부
    int verbose;            // 상세 출력 여부
    time_t restore_time;    // 특정 시점으로 복원 (0이면 최신)
} RestoreOptions;

// 복원 상태 구조체
typedef struct {
    int total_files;        // 전체 파일 수
    int restored_files;     // 복원된 파일 수
    int skipped_files;      // 건너뛴 파일 수
    int error_files;        // 오류 발생 파일 수
    long total_size;        // 전체 크기
    long restored_size;     // 복원된 크기
    time_t start_time;      // 시작 시간
    time_t end_time;        // 종료 시간
} RestoreStatus;

// 함수 선언
int restore_file(const char *backup_path, const char *destination, const RestoreOptions *options);
int restore_directory(const char *backup_dir, const char *destination_dir, const RestoreOptions *options);
int perform_restore(const char *backup_path, const char *destination, const RestoreOptions *options, RestoreStatus *status);
int verify_restored_file(const char *original, const char *restored);
int handle_file_conflict(const char *destination, ConflictResolution resolution);
void init_restore_options(RestoreOptions *options);
void print_restore_status(const RestoreStatus *status);
int list_available_backups(const char *backup_dir);

#endif // RESTORE_OPS_H
