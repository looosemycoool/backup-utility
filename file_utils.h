#ifndef FILE_UTILS_H
#define FILE_UTILS_H

#include <sys/stat.h>
#include <time.h>

// 상수 정의
#define MAX_PATH_LENGTH 4096
#define BUFFER_SIZE 8192

// 파일 정보 구조체
typedef struct {
    char name[256];           // 파일명
    char full_path[MAX_PATH_LENGTH];  // 전체 경로
    off_t size;              // 파일 크기
    time_t mtime;            // 수정 시간
    mode_t mode;             // 파일 권한
    uid_t uid;               // 소유자 ID
    gid_t gid;               // 그룹 ID
    int is_directory;        // 디렉토리 여부 (1: 디렉토리, 0: 파일)
} FileInfo;

// 함수 선언
int file_exists(const char *path);
int is_directory(const char *path);
int create_directory(const char *path, mode_t mode);
int copy_file(const char *source, const char *destination);
int copy_file_with_metadata(const char *source, const char *destination);
int get_file_info(const char *path, FileInfo *info);
int compare_file_times(const char *file1, const char *file2);
long get_file_size(const char *path);
int has_file_changed(const char *filepath, time_t last_backup_time);

#endif // FILE_UTILS_H
