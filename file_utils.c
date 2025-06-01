#include "file_utils.h"
#include "logging.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>
#include <errno.h>
#include <utime.h>
#include <libgen.h>

int file_exists(const char *path) {
    struct stat st;
    return (stat(path, &st) == 0);
}

int is_directory(const char *path) {
    struct stat st;
    if (stat(path, &st) != 0) {
        return 0;
    }
    return S_ISDIR(st.st_mode);
}

int create_directory(const char *path, mode_t mode) {
    // 디렉토리가 이미 존재하면 성공
    if (file_exists(path) && is_directory(path)) {
        return 0;
    }
    
    // 부모 디렉토리까지 재귀적으로 생성
    char *path_copy = strdup(path);
    char *parent_dir = dirname(path_copy);
    
    if (strcmp(parent_dir, ".") != 0 && strcmp(parent_dir, "/") != 0) {
        if (create_directory(parent_dir, mode) != 0) {
            free(path_copy);
            return -1;
        }
    }
    
    free(path_copy);
    
    // 디렉토리 생성
    if (mkdir(path, mode) != 0) {
        log_message(LOG_ERROR, "디렉토리 생성 실패: %s (%s)", path, strerror(errno));
        return -1;
    }
    
    log_message(LOG_DEBUG, "디렉토리 생성: %s", path);
    return 0;
}

int copy_file(const char *source, const char *destination) {
    int src_fd, dest_fd;
    char buffer[BUFFER_SIZE];
    ssize_t bytes_read, bytes_written;
    
    // 소스 파일 열기
    src_fd = open(source, O_RDONLY);
    if (src_fd == -1) {
        log_message(LOG_ERROR, "소스 파일 열기 실패: %s (%s)", source, strerror(errno));
        return -1;
    }
    
    // 목적지 파일 생성/열기
    dest_fd = open(destination, O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (dest_fd == -1) {
        log_message(LOG_ERROR, "목적지 파일 생성 실패: %s (%s)", destination, strerror(errno));
        close(src_fd);
        return -1;
    }
    
    // 파일 내용 복사
    while ((bytes_read = read(src_fd, buffer, BUFFER_SIZE)) > 0) {
        bytes_written = write(dest_fd, buffer, bytes_read);
        if (bytes_written != bytes_read) {
            log_message(LOG_ERROR, "파일 쓰기 오류: %s", destination);
            close(src_fd);
            close(dest_fd);
            return -1;
        }
    }
    
    if (bytes_read == -1) {
        log_message(LOG_ERROR, "파일 읽기 오류: %s", source);
        close(src_fd);
        close(dest_fd);
        return -1;
    }
    
    close(src_fd);
    close(dest_fd);
    
    log_message(LOG_DEBUG, "파일 복사 완료: %s -> %s", source, destination);
    return 0;
}

int copy_file_with_metadata(const char *source, const char *destination) {
    struct stat st;
    struct utimbuf times;
    
    // 기본 파일 복사
    if (copy_file(source, destination) != 0) {
        return -1;
    }
    
    // 소스 파일의 메타데이터 가져오기
    if (stat(source, &st) != 0) {
        log_message(LOG_ERROR, "소스 파일 정보 읽기 실패: %s", source);
        return -1;
    }
    
    // 권한 설정
    if (chmod(destination, st.st_mode) != 0) {
        log_message(LOG_WARNING, "권한 설정 실패: %s", destination);
    }
    
    // 수정 시간 설정
    times.actime = st.st_atime;
    times.modtime = st.st_mtime;
    if (utime(destination, &times) != 0) {
        log_message(LOG_WARNING, "시간 정보 설정 실패: %s", destination);
    }
    
    // 소유자 설정 (root 권한이 있을 때만)
    if (chown(destination, st.st_uid, st.st_gid) != 0) {
        log_message(LOG_DEBUG, "소유자 설정 건너뜀: %s (권한 부족)", destination);
    }
    
    return 0;
}

int get_file_info(const char *path, FileInfo *info) {
    struct stat st;
    
    if (stat(path, &st) != 0) {
        log_message(LOG_ERROR, "파일 정보 읽기 실패: %s", path);
        return -1;
    }
    
    // 파일명 추출
    char *path_copy = strdup(path);
    char *filename = basename(path_copy);
    strncpy(info->name, filename, sizeof(info->name) - 1);
    info->name[sizeof(info->name) - 1] = '\0';
    free(path_copy);
    
    // 경로 복사
    strncpy(info->full_path, path, sizeof(info->full_path) - 1);
    info->full_path[sizeof(info->full_path) - 1] = '\0';
    
    // 파일 정보 설정
    info->size = st.st_size;
    info->mtime = st.st_mtime;
    info->mode = st.st_mode;
    info->uid = st.st_uid;
    info->gid = st.st_gid;
    info->is_directory = S_ISDIR(st.st_mode) ? 1 : 0;
    
    return 0;
}

int compare_file_times(const char *file1, const char *file2) {
    struct stat st1, st2;
    
    if (stat(file1, &st1) != 0 || stat(file2, &st2) != 0) {
        return -1;
    }
    
    if (st1.st_mtime > st2.st_mtime) return 1;
    if (st1.st_mtime < st2.st_mtime) return -1;
    return 0;
}

long get_file_size(const char *path) {
    struct stat st;
    
    if (stat(path, &st) != 0) {
        return -1;
    }
    
    return st.st_size;
}

int has_file_changed(const char *filepath, time_t last_backup_time) {
    struct stat st;
    
    if (stat(filepath, &st) != 0) {
        return 1; // 파일이 없으면 변경된 것으로 간주
    }
    
    return st.st_mtime > last_backup_time;
}
