#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdarg.h>

#define BUFFER_SIZE 8192

// 간단한 로깅
static int verbose_mode = 0;

void simple_log(const char *level, const char *format, ...) {
    if (verbose_mode || strcmp(level, "ERROR") == 0) {
        printf("[%s] ", level);
        va_list args;
        va_start(args, format);
        vprintf(format, args);
        va_end(args);
        printf("\n");
        fflush(stdout);
    }
}

// 간단한 파일 함수들
int my_file_exists(const char *path) {
    struct stat st;
    return (stat(path, &st) == 0);
}

int my_copy_file(const char *source, const char *destination) {
    int src_fd, dest_fd;
    char buffer[BUFFER_SIZE];
    ssize_t bytes_read, bytes_written;
    
    src_fd = open(source, O_RDONLY);
    if (src_fd == -1) {
        return -1;
    }
    
    dest_fd = open(destination, O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (dest_fd == -1) {
        close(src_fd);
        return -1;
    }
    
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

void print_usage(const char *program_name) {
    printf("파일 백업 유틸리티 v1.0.0\n");
    printf("사용법: %s [명령어] [옵션] [인자...]\n\n", program_name);
    
    printf("명령어:\n");
    printf("  backup <소스> <목적지>    : 지정된 소스를 목적지에 백업\n");
    printf("  restore <백업> <목적지>   : 백업을 원래 위치로 복원\n");
    printf("  help                    : 도움말 표시\n\n");
    
    printf("옵션:\n");
    printf("  -v, --verbose           : 상세 정보 출력\n\n");
    
    printf("예시:\n");
    printf("  %s backup -v file.txt backup/\n", program_name);
    printf("  %s restore backup/file.txt /home/user/\n", program_name);
}

int main(int argc, char *argv[]) {
    // 최소 인자 수 확인
    if (argc < 2) {
        print_usage(argv[0]);
        return EXIT_FAILURE;
    }
    
    // 명령어가 help인 경우 바로 처리
    if (strcmp(argv[1], "help") == 0) {
        print_usage(argv[0]);
        return EXIT_SUCCESS;
    }
    
    // verbose 옵션 확인
    for (int i = 2; i < argc; i++) {
        if (strcmp(argv[i], "-v") == 0 || strcmp(argv[i], "--verbose") == 0) {
            verbose_mode = 1;
            break;
        }
    }
    
    // 명령어 확인
    const char *command = argv[1];
    simple_log("INFO", "프로그램 시작: %s", command);
    
    int result = EXIT_SUCCESS;
    
    if (strcmp(command, "backup") == 0) {
        // backup 명령어 처리
        if (argc < 4) {
            simple_log("ERROR", "백업 명령에는 소스와 목적지가 필요합니다.");
            printf("사용법: %s backup [옵션] <소스> <목적지>\n", argv[0]);
            result = EXIT_FAILURE;
        } else {
            // 마지막 두 인자가 소스와 목적지
            const char *source = argv[argc - 2];
            const char *destination = argv[argc - 1];
            
            simple_log("INFO", "백업 시작: %s -> %s", source, destination);
            
            // 소스 파일 존재 확인
            if (!my_file_exists(source)) {
                simple_log("ERROR", "소스가 존재하지 않습니다: %s", source);
                result = EXIT_FAILURE;
            } else {
                simple_log("INFO", "소스 파일 확인됨, 복사 시작");
                
                // 파일 복사
                if (my_copy_file(source, destination) == 0) {
                    simple_log("INFO", "백업 완료: %s -> %s", source, destination);
                    result = EXIT_SUCCESS;
                } else {
                    simple_log("ERROR", "백업 실패: %s -> %s", source, destination);
                    result = EXIT_FAILURE;
                }
            }
        }
        
    } else if (strcmp(command, "restore") == 0) {
        // restore 명령어 처리
        if (argc < 4) {
            simple_log("ERROR", "복원 명령에는 백업 파일과 목적지가 필요합니다.");
            printf("사용법: %s restore [옵션] <백업파일> <목적지>\n", argv[0]);
            result = EXIT_FAILURE;
        } else {
            // 마지막 두 인자가 백업 파일과 목적지
            const char *backup_file = argv[argc - 2];
            const char *destination = argv[argc - 1];
            
            simple_log("INFO", "복원 시작: %s -> %s", backup_file, destination);
            
            // 백업 파일 존재 확인
            if (!my_file_exists(backup_file)) {
                simple_log("ERROR", "백업 파일이 존재하지 않습니다: %s", backup_file);
                result = EXIT_FAILURE;
            } else {
                // 파일 복원 수행
                if (my_copy_file(backup_file, destination) == 0) {
                    simple_log("INFO", "복원 완료: %s -> %s", backup_file, destination);
                    result = EXIT_SUCCESS;
                } else {
                    simple_log("ERROR", "복원 실패: %s -> %s", backup_file, destination);
                    result = EXIT_FAILURE;
                }
            }
        }
        
    } else {
        simple_log("ERROR", "알 수 없는 명령어: %s", command);
        print_usage(argv[0]);
        result = EXIT_FAILURE;
    }
    
    simple_log("INFO", "프로그램 종료 (종료 코드: %d)", result);
    
    return result;
}
