#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "logging.h"
#include "file_utils.h"
#include "backup_ops.h"
#include "restore_ops.h"

void print_usage(const char *program_name) {
    printf("파일 백업 유틸리티 v1.0.0\n");
    printf("사용법: %s [명령어] [옵션] [인자...]\n\n", program_name);
    
    printf("명령어:\n");
    printf("  backup <소스> <목적지>    : 지정된 소스를 목적지에 백업\n");
    printf("  restore <백업> <목적지>   : 백업을 원래 위치로 복원\n");
    printf("  list <백업디렉토리>       : 백업 목록 표시\n");
    printf("  help                    : 도움말 표시\n\n");
    
    printf("옵션:\n");
    printf("  -v, --verbose           : 상세 정보 출력\n");
    printf("  -r, --recursive         : 디렉토리 재귀 처리\n");
    printf("  -i, --incremental       : 증분 백업\n");
    printf("  -m, --metadata          : 메타데이터 보존\n\n");
    
    printf("예시:\n");
    printf("  %s backup -v file.txt backup/\n", program_name);
    printf("  %s backup -r -m /home/user/documents backup/\n", program_name);
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
    int verbose = 0;
    for (int i = 2; i < argc; i++) {
        if (strcmp(argv[i], "-v") == 0 || strcmp(argv[i], "--verbose") == 0) {
            verbose = 1;
            break;
        }
    }
    
    // 로깅 초기화
    init_logging(verbose);
    
    // 명령어 확인
    const char *command = argv[1];
    log_message(LOG_INFO, "프로그램 시작: %s", command);
    
    int result = EXIT_SUCCESS;
    
    if (strcmp(command, "backup") == 0) {
        // backup 명령어 처리
        if (argc < 4) {
            log_message(LOG_ERROR, "백업 명령에는 소스와 목적지가 필요합니다.");
            printf("사용법: %s backup [옵션] <소스> <목적지>\n", argv[0]);
            result = EXIT_FAILURE;
        } else {
            // 마지막 두 인자가 소스와 목적지
            const char *source = argv[argc - 2];
            const char *destination = argv[argc - 1];
            
            log_message(LOG_INFO, "백업 시작: %s -> %s", source, destination);
            
            // DEBUG: 파일 존재 확인 전에 로그
            log_message(LOG_INFO, "DEBUG: 소스 파일 존재 확인 중: %s", source);
            
            // 소스 파일 존재 확인
            if (!file_exists(source)) {
                log_message(LOG_ERROR, "소스가 존재하지 않습니다: %s", source);
                result = EXIT_FAILURE;
            } else {
                log_message(LOG_INFO, "DEBUG: 소스 파일 존재 확인됨");
                
                // 파일 백업 수행
                if (is_directory(source)) {
                    log_message(LOG_INFO, "디렉토리 백업은 아직 구현 중입니다: %s", source);
                    result = EXIT_SUCCESS;
                } else {
                    log_message(LOG_INFO, "DEBUG: 파일 복사 시작");
                    
                    // 단순한 copy_file 함수부터 테스트
                    if (copy_file(source, destination) == 0) {
                        log_message(LOG_INFO, "백업 완료 (simple copy): %s -> %s", source, destination);
                        result = EXIT_SUCCESS;
                    } else {
                        log_message(LOG_ERROR, "백업 실패: %s -> %s", source, destination);
                        result = EXIT_FAILURE;
                    }
                }
            }
        }
        
    } else if (strcmp(command, "restore") == 0) {
        // restore 명령어 처리
        if (argc < 4) {
            log_message(LOG_ERROR, "복원 명령에는 백업 파일과 목적지가 필요합니다.");
            printf("사용법: %s restore [옵션] <백업파일> <목적지>\n", argv[0]);
            result = EXIT_FAILURE;
        } else {
            // 마지막 두 인자가 백업 파일과 목적지
            const char *backup_file = argv[argc - 2];
            const char *destination = argv[argc - 1];
            
            log_message(LOG_INFO, "복원 시작: %s -> %s", backup_file, destination);
            
            // 백업 파일 존재 확인
            if (!file_exists(backup_file)) {
                log_message(LOG_ERROR, "백업 파일이 존재하지 않습니다: %s", backup_file);
                result = EXIT_FAILURE;
            } else {
                // 파일 복원 수행 (단순 copy 사용)
                if (copy_file(backup_file, destination) == 0) {
                    log_message(LOG_INFO, "복원 완료: %s -> %s", backup_file, destination);
                    result = EXIT_SUCCESS;
                } else {
                    log_message(LOG_ERROR, "복원 실패: %s -> %s", backup_file, destination);
                    result = EXIT_FAILURE;
                }
            }
        }
        
    } else if (strcmp(command, "list") == 0) {
        // list 명령어 처리
        if (argc < 3) {
            log_message(LOG_ERROR, "목록 명령에는 백업 디렉토리가 필요합니다.");
            printf("사용법: %s list <백업디렉토리>\n", argv[0]);
            result = EXIT_FAILURE;
        } else {
            const char *backup_dir = argv[2];
            log_message(LOG_INFO, "백업 목록 표시: %s", backup_dir);
            
            if (list_available_backups(backup_dir) == 0) {
                result = EXIT_SUCCESS;
            } else {
                result = EXIT_FAILURE;
            }
        }
        
    } else {
        log_message(LOG_ERROR, "알 수 없는 명령어: %s", command);
        print_usage(argv[0]);
        result = EXIT_FAILURE;
    }
    
    log_message(LOG_INFO, "프로그램 종료 (종료 코드: %d)", result);
    close_logging();
    
    return result;
}
