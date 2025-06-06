#define _GNU_SOURCE  // strdup을 위해 필요
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
    if (file_exists(path) && is_directory(path)) {
        return 0;
    }
    
    // strdup 대신 수동으로 문자열 복사
    size_t len = strlen(path);
    char *path_copy = malloc(len + 1);
    if (!path_copy) {
        return -1;
    }
    strcpy(path_copy, path);
    
    char *parent_dir = dirname(path_copy);
    
    if (strcmp(parent_dir, ".") != 0 && strcmp(parent_dir, "/") != 0) {
        if (create_directory(parent_dir, mode) != 0) {
            free(path_copy);
            return -1;
        }
    }
    
    free(path_copy);
    
    if (mkdir(path, mode) != 0) {
        return -1;
    }
    
    return 0;
}

int copy_file(const char *source, const char *destination) {
    int src_fd, dest_fd;
    char buffer[BUFFER_SIZE];
    ssize_t bytes_read, bytes_written;
    
    // 소스 파일 열기
    src_fd = open(source, O_RDONLY);
    if (src_fd == -1) {
        return -1;
    }
    
    // 목적지 파일 열기
    dest_fd = open(destination, O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (dest_fd == -1) {
        close(src_fd);
        return -1;
    }
    
    // 파일 복사
    while ((bytes_read = read(src_fd, buffer, BUFFER_SIZE)) > 0) {
        bytes_written = write(dest_fd, buffer, bytes_read);
        if (bytes_written != bytes_read) {
            close(src_fd);
            close(dest_fd);
            return -1;
        }
    }
    
    if (bytes_read == -1) {
        close(src_fd);
        close(dest_fd);
        return -1;
    }
    
    close(src_fd);
    close(dest_fd);
    return 0;
}

int copy_file_with_metadata(const char *source, const char *destination) {
    struct stat st;
    struct utimbuf times;
    
    // 먼저 파일 복사
    if (copy_file(source, destination) != 0) {
        return -1;
    }
    
    // 메타데이터 복사
    if (stat(source, &st) != 0) {
        return -1;
    }
    
    // 권한 설정
    chmod(destination, st.st_mode);
    
    // 시간 설정
    times.actime = st.st_atime;
    times.modtime = st.st_mtime;
    utime(destination, &times);
    
    return 0;
}

int get_file_info(const char *path, FileInfo *info) {
    struct stat st;
    
    if (stat(path, &st) != 0) {
        return -1;
    }
    
    // strdup 대신 수동으로 문자열 복사
    size_t len = strlen(path);
    char *path_copy = malloc(len + 1);
    if (!path_copy) {
        return -1;
    }
    strcpy(path_copy, path);
    
    char *filename = basename(path_copy);
    strncpy(info->name, filename, sizeof(info->name) - 1);
    info->name[sizeof(info->name) - 1] = '\0';
    free(path_copy);
    
    strncpy(info->full_path, path, sizeof(info->full_path) - 1);
    info->full_path[sizeof(info->full_path) - 1] = '\0';
    
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
        return 1;
    }
    
    return st.st_mtime > last_backup_time;
}
