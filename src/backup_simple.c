#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>

#define BUFFER_SIZE 8192

int simple_file_exists(const char *path) {
    struct stat st;
    return (stat(path, &st) == 0);
}

int simple_copy_file(const char *source, const char *destination) {
    int src_fd, dest_fd;
    char buffer[BUFFER_SIZE];
    ssize_t bytes_read, bytes_written;
    
    printf("DEBUG: 소스 파일 열기: %s\n", source);
    src_fd = open(source, O_RDONLY);
    if (src_fd == -1) {
        printf("DEBUG: 소스 파일 열기 실패\n");
        return -1;
    }
    
    printf("DEBUG: 목적지 파일 생성: %s\n", destination);
    dest_fd = open(destination, O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (dest_fd == -1) {
        printf("DEBUG: 목적지 파일 생성 실패\n");
        close(src_fd);
        return -1;
    }
    
    printf("DEBUG: 파일 복사 시작\n");
    while ((bytes_read = read(src_fd, buffer, BUFFER_SIZE)) > 0) {
        bytes_written = write(dest_fd, buffer, bytes_read);
        if (bytes_written != bytes_read) {
            printf("DEBUG: 쓰기 실패\n");
            close(src_fd);
            close(dest_fd);
            return -1;
        }
        printf("DEBUG: %ld 바이트 복사됨\n", bytes_read);
    }
    
    if (bytes_read == -1) {
        printf("DEBUG: 읽기 실패\n");
        close(src_fd);
        close(dest_fd);
        return -1;
    }
    
    printf("DEBUG: 파일 복사 완료\n");
    close(src_fd);
    close(dest_fd);
    return 0;
}

int main(int argc, char *argv[]) {
    if (argc != 4) {
        printf("사용법: %s backup <소스> <목적지>\n", argv[0]);
        return 1;
    }
    
    if (strcmp(argv[1], "backup") != 0) {
        printf("첫 번째 인자는 'backup'이어야 합니다.\n");
        return 1;
    }
    
    const char *source = argv[2];
    const char *destination = argv[3];
    
    printf("DEBUG: 프로그램 시작\n");
    printf("DEBUG: 소스: %s\n", source);
    printf("DEBUG: 목적지: %s\n", destination);
    
    if (!simple_file_exists(source)) {
        printf("ERROR: 소스 파일이 존재하지 않습니다: %s\n", source);
        return 1;
    }
    
    printf("DEBUG: 소스 파일 확인됨\n");
    
    if (simple_copy_file(source, destination) == 0) {
        printf("SUCCESS: 백업 완료\n");
        return 0;
    } else {
        printf("ERROR: 백업 실패\n");
        return 1;
    }
}
