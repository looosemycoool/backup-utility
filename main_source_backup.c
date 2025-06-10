#include "backup.h"

// 전역 변수 정의
backup_options_t g_options = {0};
backup_stats_t g_stats = {0};
progress_info_t g_progress = {0};

void print_usage(const char *prog) {
    printf("고급 파일 백업 유틸리티 v%s\n", VERSION);
    printf("빌드: %s\n\n", BUILD_DATE);
    printf("사용법:\n");
    printf("  %s <명령어> [옵션] <소스> <대상>\n\n", prog);
    printf("명령어:\n");
    printf("  backup                      파일/디렉토리 백업\n");
    printf("  restore                     파일/디렉토리 복원\n");
    printf("  verify                      백업 검증\n");
    printf("  list                        백업 내용 목록\n");
    printf("  version                     버전 정보\n");
    printf("  help                        도움말\n\n");
    printf("옵션:\n");
    printf("  -r, --recursive             재귀적 처리\n");
    printf("  -v, --verbose               상세 출력\n");
    printf("  -p, --progress              진행률 표시\n");
    printf("  -c, --compression=TYPE      압축 (none, gzip, zlib)\n");
    printf("  -m, --mode=MODE             백업 모드 (full, incremental, differential)\n");
    printf("  -x, --exclude=PATTERN       제외 패턴\n");
    printf("  -j, --jobs=N                병렬 처리 스레드 수 (기본: %d)\n", MAX_THREADS);
    printf("  --verify                    백업 후 검증\n");
    printf("  --preserve-permissions      권한 보존\n");
    printf("  --preserve-timestamps       시간 정보 보존\n");
    printf("  --dry-run                   실제 실행하지 않고 시뮬레이션\n");
    printf("  --conflict=MODE             충돌 처리 (ask, overwrite, skip, rename)\n");
    printf("  --config=FILE               설정 파일\n");
    printf("  --log=FILE                  로그 파일\n");
    printf("  --log-level=LEVEL           로그 레벨 (error, warning, info, debug)\n");
    printf("  --max-size=SIZE             최대 파일 크기 (바이트)\n\n");
    printf("예시:\n");
    printf("  %s backup -rv /home/user /backup/user\n", prog);
    printf("  %s backup -c gzip --verify file.txt backup.txt.gz\n", prog);
    printf("  %s restore /backup/user /home/user\n", prog);
    printf("  %s verify /backup/user\n", prog);
    printf("  %s list /backup/user\n", prog);
}

void print_version(void) {
    printf("고급 백업 유틸리티 버전 %s\n", VERSION);
    printf("빌드 날짜: %s\n", BUILD_DATE);
    printf("컴파일러: GCC %s\n", __VERSION__);
    printf("최대 병렬 스레드: %d\n", MAX_THREADS);
    printf("지원 압축: gzip, zlib\n");
    printf("버퍼 크기: %d bytes\n", BUFFER_SIZE);
}

compression_type_t parse_compression_type(const char *str) {
    if (!str || strcmp(str, "none") == 0) return COMPRESS_NONE;
    if (strcmp(str, "gzip") == 0) return COMPRESS_GZIP;
    if (strcmp(str, "zlib") == 0) return COMPRESS_ZLIB;
    return COMPRESS_NONE;
}

conflict_mode_t parse_conflict_mode(const char *str) {
    if (!str || strcmp(str, "ask") == 0) return CONFLICT_ASK;
    if (strcmp(str, "overwrite") == 0) return CONFLICT_OVERWRITE;
    if (strcmp(str, "skip") == 0) return CONFLICT_SKIP;
    if (strcmp(str, "rename") == 0) return CONFLICT_RENAME;
    return CONFLICT_ASK;
}

log_level_t parse_log_level(const char *str) {
    if (!str || strcmp(str, "error") == 0) return LOG_ERROR;
    if (strcmp(str, "warning") == 0) return LOG_WARNING;
    if (strcmp(str, "info") == 0) return LOG_INFO;
    if (strcmp(str, "debug") == 0) return LOG_DEBUG;
    return LOG_INFO;
}

int load_config_file(const char *config_file, backup_options_t *opts) {
    FILE *file;
    char line[1024];
    char key[256], value[256];
    
    file = fopen(config_file, "r");
    if (!file) {
        return -1;
    }
    
    while (fgets(line, sizeof(line), file)) {
        // 주석과 빈 줄 건너뛰기
        if (line[0] == '#' || line[0] == '\n' || line[0] == '\0') {
            continue;
        }
        
        // key=value 파싱
        if (sscanf(line, "%255[^=]=%255[^\n]", key, value) == 2) {
            if (strcmp(key, "compression") == 0) {
                opts->compression = parse_compression_type(value);
            } else if (strcmp(key, "recursive") == 0) {
                opts->recursive = (strcmp(value, "true") == 0 || strcmp(value, "1") == 0);
            } else if (strcmp(key, "verbose") == 0) {
                opts->verbose = (strcmp(value, "true") == 0 || strcmp(value, "1") == 0);
            } else if (strcmp(key, "verify") == 0) {
                opts->verify = (strcmp(value, "true") == 0 || strcmp(value, "1") == 0);
            } else if (strcmp(key, "threads") == 0) {
                opts->threads = atoi(value);
                if (opts->threads < 1) opts->threads = 1;
                if (opts->threads > MAX_THREADS) opts->threads = MAX_THREADS;
            } else if (strcmp(key, "log_file") == 0) {
                strncpy(opts->log_file, value, sizeof(opts->log_file) - 1);
            } else if (strcmp(key, "log_level") == 0) {
                opts->log_level = parse_log_level(value);
            } else if (strcmp(key, "exclude") == 0 && opts->exclude_count < MAX_EXCLUDE_PATTERNS) {
                strncpy(opts->exclude_patterns[opts->exclude_count], value, MAX_PATH - 1);
                opts->exclude_count++;
            }
        }
    }
    
    fclose(file);
    return 0;
}

int parse_options(int argc, char *argv[], backup_options_t *opts) {
    int opt;
    static struct option long_options[] = {
        {"recursive", no_argument, 0, 'r'},
        {"verbose", no_argument, 0, 'v'},
        {"progress", no_argument, 0, 'p'},
        {"compression", required_argument, 0, 'c'},
        {"mode", required_argument, 0, 'm'},
        {"exclude", required_argument, 0, 'x'},
        {"jobs", required_argument, 0, 'j'},
        {"verify", no_argument, 0, 1001},
        {"preserve-permissions", no_argument, 0, 1002},
        {"preserve-timestamps", no_argument, 0, 1003},
        {"dry-run", no_argument, 0, 1004},
        {"conflict", required_argument, 0, 1005},
        {"config", required_argument, 0, 1006},
        {"log", required_argument, 0, 1007},
        {"log-level", required_argument, 0, 1008},
        {"max-size", required_argument, 0, 1009},
        {"help", no_argument, 0, 'h'},
        {0, 0, 0, 0}
    };
    
    // 기본값 설정
    opts->compression = COMPRESS_NONE;
    opts->mode = BACKUP_FULL;
    opts->conflict_mode = CONFLICT_ASK;
    opts->threads = MAX_THREADS;
    opts->max_file_size = LONG_MAX;
    opts->preserve_permissions = 1;
    opts->preserve_timestamps = 1;
    opts->log_level = LOG_INFO;
    
    while ((opt = getopt_long(argc, argv, "rvpc:m:x:j:h", long_options, NULL)) != -1) {
        switch (opt) {
            case 'r':
                opts->recursive = 1;
                break;
            case 'v':
                opts->verbose = 1;
                break;
            case 'p':
                opts->progress = 1;
                break;
            case 'c':
                opts->compression = parse_compression_type(optarg);
                break;
            case 'm':
                if (strcmp(optarg, "full") == 0) opts->mode = BACKUP_FULL;
                else if (strcmp(optarg, "incremental") == 0) opts->mode = BACKUP_INCREMENTAL;
                else if (strcmp(optarg, "differential") == 0) opts->mode = BACKUP_DIFFERENTIAL;
                break;
            case 'x':
                if (opts->exclude_count < MAX_EXCLUDE_PATTERNS) {
                    strncpy(opts->exclude_patterns[opts->exclude_count], optarg, MAX_PATH - 1);
                    opts->exclude_count++;
                }
                break;
            case 'j':
                opts->threads = atoi(optarg);
                if (opts->threads < 1) opts->threads = 1;
                if (opts->threads > MAX_THREADS) opts->threads = MAX_THREADS;
                break;
            case 1001:
                opts->verify = 1;
                break;
            case 1002:
                opts->preserve_permissions = 1;
                break;
            case 1003:
                opts->preserve_timestamps = 1;
                break;
            case 1004:
                opts->dry_run = 1;
                break;
            case 1005:
                opts->conflict_mode = parse_conflict_mode(optarg);
                break;
            case 1006:
                strncpy(opts->config_file, optarg, sizeof(opts->config_file) - 1);
                break;
            case 1007:
                strncpy(opts->log_file, optarg, sizeof(opts->log_file) - 1);
                opts->logging = 1;
                break;
            case 1008:
                opts->log_level = parse_log_level(optarg);
                break;
            case 1009:
                opts->max_file_size = atol(optarg);
                break;
            case 'h':
                print_usage(argv[0]);
                exit(0);
                break;
            default:
                print_usage(argv[0]);
                return -1;
        }
    }
    
    return 0;
}

void signal_handler(int sig) {
    switch (sig) {
        case SIGINT:
        case SIGTERM:
            printf("\n중단 요청을 받았습니다...\n");
            g_progress.cancel_requested = 1;
            break;
        case SIGUSR1:
            printf("\n현재 진행률: %ld/%ld 파일 (%ld%%)\n", 
                   g_progress.current_files, g_progress.total_files,
                   g_progress.total_files > 0 ? (g_progress.current_files * 100 / g_progress.total_files) : 0);
            break;
    }
}

int main(int argc, char *argv[]) {
    int result = SUCCESS;
    const char *command;
    const char *source, *dest;
    
    if (argc < 2) {
        print_usage(argv[0]);
        return 1;
    }
    
    command = argv[1];
    
    if (strcmp(command, "help") == 0) {
        print_usage(argv[0]);
        return 0;
    }
    
    if (strcmp(command, "version") == 0) {
        print_version();
        return 0;
    }
    
    // ⭐ 수정: command(argv[1]) 다음부터 전달
    if (parse_options(argc - 2, argv + 2, &g_options) != 0) {
        return 1;
    }
    
    // 설정 파일 로드
    if (strlen(g_options.config_file) > 0) {
        load_config_file(g_options.config_file, &g_options);
    }
    
    // 로깅 초기화
    init_logging(&g_options);
    
    // 신호 처리기 등록
    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);
    signal(SIGUSR1, signal_handler);
    
    // 통계 초기화
    memset(&g_stats, 0, sizeof(backup_stats_t));
    g_stats.start_time = time(NULL);
    
    // 진행률 정보 초기화
    memset(&g_progress, 0, sizeof(progress_info_t));
    pthread_mutex_init(&g_progress.mutex, NULL);
    
    // 명령어별 처리
    if (strcmp(command, "backup") == 0) {
        // ⭐ 수정: optind는 argv+2 기준이므로 +2 필요
        int remaining_args = argc - optind - 2;
        if (remaining_args < 2) {
            printf("사용법: %s backup [옵션] <소스> <대상>\n", argv[0]);
            return 1;
        }
        
        // ⭐ 수정: argv+2 기준 optind이므로 실제로는 +2 필요
        source = argv[optind + 2];
        dest = argv[optind + 3];
        
        // 디버그 정보 (verbose 모드에서만)
        if (g_options.verbose) {
            printf("백업 작업: %s -> %s\n", source, dest);
        }
        
        // ⭐ 수정: 정상적인 로그 기록 (한 번만)
        log_info("백업 시작: %s -> %s", source, dest);
        
        // 소스 파일/디렉토리 존재 확인
        if (!file_exists(source)) {
            printf("오류: 소스가 존재하지 않습니다: %s\n", source);
            result = ERROR_FILE_NOT_FOUND;
        } else {
            // 디렉토리 vs 파일 처리
            if (is_directory(source)) {
                if (!g_options.recursive) {
                    printf("오류: 디렉토리 백업에는 -r 옵션이 필요합니다.\n");
                    result = ERROR_GENERAL;
                } else {
                    result = backup_directory(source, dest, &g_options);
                }
            } else {
                result = backup_file(source, dest, &g_options);
            }
        }
        
        // 백업 후 검증 (옵션이 설정된 경우)
        if (result == SUCCESS && g_options.verify) {
            log_info("백업 검증 중...");
            result = verify_backup_integrity(source, dest, &g_options);
        }
        
    } else if (strcmp(command, "restore") == 0) {
        int remaining_args = argc - optind - 2;
        if (remaining_args < 2) {
            printf("사용법: %s restore [옵션] <소스> <대상>\n", argv[0]);
            return 1;
        }
        
        source = argv[optind + 2];
        dest = argv[optind + 3];
        
        log_info("복원 시작: %s -> %s", source, dest);
        
        if (!file_exists(source)) {
            printf("오류: 백업 소스가 존재하지 않습니다: %s\n", source);
            result = ERROR_FILE_NOT_FOUND;
        } else {
            if (is_directory(source)) {
                if (!g_options.recursive) {
                    printf("오류: 디렉토리 복원에는 -r 옵션이 필요합니다.\n");
                    result = ERROR_GENERAL;
                } else {
                    result = restore_directory(source, dest, &g_options);
                }
            } else {
                result = restore_file(source, dest, &g_options);
            }
        }
        
    } else if (strcmp(command, "verify") == 0) {
        int remaining_args = argc - optind - 2;
        if (remaining_args < 1) {
            printf("사용법: %s verify [옵션] <백업경로>\n", argv[0]);
            return 1;
        }
        
        source = argv[optind + 2];
        result = verify_backup(source, &g_options);
        
    } else if (strcmp(command, "list") == 0) {
        int remaining_args = argc - optind - 2;
        if (remaining_args < 1) {
            printf("사용법: %s list [옵션] <백업경로>\n", argv[0]);
            return 1;
        }
        
        source = argv[optind + 2];
        result = list_backup_contents(source, &g_options);
        
    } else {
        printf("오류: 알 수 없는 명령어: %s\n", command);
        print_usage(argv[0]);
        return 1;
    }
    
    // 통계 출력
    g_stats.end_time = time(NULL);
    
    if (g_options.verbose || g_options.progress) {
        printf("\n=== 작업 완료 ===\n");
        printf("처리된 파일: %ld\n", g_stats.files_processed);
        printf("건너뛴 파일: %ld\n", g_stats.files_skipped);
        printf("실패한 파일: %ld\n", g_stats.files_failed);
        printf("처리된 디렉토리: %ld\n", g_stats.dirs_processed);
        printf("처리된 바이트: %ld\n", g_stats.bytes_processed);
        
        if (g_stats.bytes_compressed > 0) {
            g_stats.compression_ratio = (double)g_stats.bytes_compressed / g_stats.bytes_processed * 100.0;
            printf("압축된 바이트: %ld (%.1f%%)\n", g_stats.bytes_compressed, g_stats.compression_ratio);
        }
        
        double elapsed = difftime(g_stats.end_time, g_stats.start_time);
        printf("소요 시간: %.2f초\n", elapsed);
        
        if (elapsed > 0) {
            printf("처리 속도: %.2f MB/s\n", 
                   (double)g_stats.bytes_processed / (1024 * 1024) / elapsed);
        }
    }
    
    // 정리
    if (result != SUCCESS) {
        log_error("작업이 실패했습니다. (오류 코드: %d)", result);
    }
    
    close_logging();
    pthread_mutex_destroy(&g_progress.mutex);
    
    return result;
}