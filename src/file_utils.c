#include "backup.h"

int file_exists(const char *path) {
    if (!path) return 0;
    return access(path, F_OK) == 0;
}

int is_directory(const char *path) {
    struct stat st;
    if (!path || stat(path, &st) != 0) {
        return 0;
    }
    return S_ISDIR(st.st_mode);
}

int is_regular_file(const char *path) {
    struct stat st;
    if (!path || stat(path, &st) != 0) {
        return 0;
    }
    return S_ISREG(st.st_mode);
}

int create_directory(const char *path) {
    if (!path) return ERROR_INVALID_PARAMS;
    
    if (mkdir(path, 0755) == 0) {
        return SUCCESS;
    }
    
    if (errno == EEXIST && is_directory(path)) {
        return SUCCESS; // 이미 존재하는 디렉토리
    }
    
    log_error("디렉토리 생성 실패: %s (%s)", path, strerror(errno));
    return ERROR_FILE_WRITE;
}

int create_directory_recursive(const char *path) {
    char tmp_path[MAX_PATH];
    char *p = NULL;
    size_t len;
    
    if (!path) return ERROR_INVALID_PARAMS;
    
    snprintf(tmp_path, sizeof(tmp_path), "%s", path);
    len = strlen(tmp_path);
    
    // 마지막 슬래시 제거
    if (tmp_path[len - 1] == '/') {
        tmp_path[len - 1] = '\0';
    }
    
    // 루트부터 시작하여 각 디렉토리 생성
    for (p = tmp_path + 1; *p; p++) {
        if (*p == '/') {
            *p = '\0';
            
            if (!file_exists(tmp_path)) {
                if (create_directory(tmp_path) != SUCCESS) {
                    return ERROR_FILE_WRITE;
                }
            }
            
            *p = '/';
        }
    }
    
    // 마지막 디렉토리 생성
    if (!file_exists(tmp_path)) {
        return create_directory(tmp_path);
    }
    
    return SUCCESS;
}

int copy_file_metadata(const char *source, const char *dest) {
    struct stat st;
    struct utimbuf times;
    
    if (!source || !dest) return ERROR_INVALID_PARAMS;
    
    if (stat(source, &st) != 0) {
        log_error("소스 파일 정보 가져오기 실패: %s", source);
        return ERROR_FILE_READ;
    }
    
    // 파일 권한 복사
    if (chmod(dest, st.st_mode & 07777) != 0) {
        log_warning("파일 권한 설정 실패: %s", dest);
    }
    
    // 파일 시간 정보 복사
    times.actime = st.st_atime;
    times.modtime = st.st_mtime;
    
    if (utime(dest, &times) != 0) {
        log_warning("파일 시간 정보 설정 실패: %s", dest);
    }
    
    // 소유자 정보 복사 (root 권한이 있을 때만)
    if (getuid() == 0) {
        if (chown(dest, st.st_uid, st.st_gid) != 0) {
            log_warning("파일 소유자 설정 실패: %s", dest);
        }
    }
    
    return SUCCESS;
}

int should_include_file(const char *path, const backup_options_t *opts) {
    const char *filename;
    struct stat st;
    
    if (!path || !opts) return 0;
    
    // 파일 정보 가져오기
    if (stat(path, &st) != 0) {
        return 0; // 접근할 수 없는 파일은 제외
    }
    
    // 파일 크기 제한 확인
    if (opts->max_file_size != SIZE_MAX && st.st_size > opts->max_file_size) {
        log_debug("파일 크기 제한으로 제외: %s (%zu bytes)", path, st.st_size);
        return 0;
    }
    
    // 파일명 추출
    filename = strrchr(path, '/');
    if (!filename) {
        filename = path;
    } else {
        filename++; // '/' 다음 문자부터
    }
    
    // 제외 패턴 확인
    for (int i = 0; i < opts->exclude_count; i++) {
        if (fnmatch(opts->exclude_patterns[i], filename, FNM_CASEFOLD) == 0) {
            log_debug("제외 패턴 일치로 제외: %s (패턴: %s)", 
                     filename, opts->exclude_patterns[i]);
            return 0;
        }
        
        // 전체 경로에 대해서도 패턴 확인
        if (fnmatch(opts->exclude_patterns[i], path, FNM_CASEFOLD) == 0) {
            log_debug("제외 패턴 일치로 제외: %s (패턴: %s)", 
                     path, opts->exclude_patterns[i]);
            return 0;
        }
    }
    
    // 숨김 파일 처리 (일반적으로 포함하지만 로그는 debug 레벨로)
    if (filename[0] == '.' && strlen(filename) > 1) {
        log_debug("숨김 파일 포함: %s", filename);
    }
    
    return 1; // 포함
}

int compare_files(const char *file1, const char *file2) {
    FILE *f1, *f2;
    char buf1[BUFFER_SIZE], buf2[BUFFER_SIZE];
    size_t n1, n2;
    int result = 1; // 기본적으로 동일하다고 가정
    
    if (!file1 || !file2) return 0;
    
    f1 = fopen(file1, "rb");
    if (!f1) {
        log_error("파일 열기 실패: %s", file1);
        return 0;
    }
    
    f2 = fopen(file2, "rb");
    if (!f2) {
        log_error("파일 열기 실패: %s", file2);
        fclose(f1);
        return 0;
    }
    
    // 파일 크기 먼저 비교
    struct stat st1, st2;
    if (fstat(fileno(f1), &st1) == 0 && fstat(fileno(f2), &st2) == 0) {
        if (st1.st_size != st2.st_size) {
            log_debug("파일 크기 다름: %s (%zu) vs %s (%zu)", 
                     file1, st1.st_size, file2, st2.st_size);
            result = 0;
            goto cleanup;
        }
    }
    
    // 바이트 단위로 비교
    while ((n1 = fread(buf1, 1, BUFFER_SIZE, f1)) > 0) {
        n2 = fread(buf2, 1, BUFFER_SIZE, f2);
        
        if (n1 != n2 || memcmp(buf1, buf2, n1) != 0) {
            log_debug("파일 내용 다름: %s vs %s", file1, file2);
            result = 0;
            break;
        }
    }
    
    // f2에 더 많은 데이터가 있는지 확인
    if (result && fread(buf2, 1, 1, f2) > 0) {
        log_debug("파일 크기 다름 (f2가 더 큼): %s vs %s", file1, file2);
        result = 0;
    }

cleanup:
    fclose(f1);
    fclose(f2);
    
    return result;
}

size_t get_file_size(const char *path) {
    struct stat st;
    
    if (!path || stat(path, &st) != 0) {
        return 0;
    }
    
    return st.st_size;
}

char *get_relative_path(const char *base, const char *path) {
    static char relative[MAX_PATH];
    size_t base_len, path_len;
    
    if (!base || !path) return NULL;
    
    base_len = strlen(base);
    path_len = strlen(path);
    
    // base가 path의 접두사인지 확인
    if (path_len < base_len || strncmp(base, path, base_len) != 0) {
        // 상대 경로를 만들 수 없음, 전체 경로 반환
        strncpy(relative, path, MAX_PATH - 1);
        relative[MAX_PATH - 1] = '\0';
        return relative;
    }
    
    // base 이후의 경로 추출
    const char *rel_start = path + base_len;
    
    // 시작 슬래시 건너뛰기
    while (*rel_start == '/') {
        rel_start++;
    }
    
    strncpy(relative, rel_start, MAX_PATH - 1);
    relative[MAX_PATH - 1] = '\0';
    
    return relative;
}

void normalize_path(char *path) {
    char *src, *dst;
    char *start;
    
    if (!path) return;
    
    src = dst = path;
    start = path;
    
    // 경로 정규화: 중복 슬래시 제거, ./ 및 ../ 처리
    while (*src) {
        if (*src == '/') {
            // 연속된 슬래시 처리
            while (*src == '/') src++;
            
            if (dst > start) {
                *dst++ = '/';
            }
        } else if (*src == '.' && (src == start || *(src-1) == '/')) {
            if (*(src+1) == '/' || *(src+1) == '\0') {
                // ./ 건너뛰기
                src++;
                if (*src == '/') src++;
            } else if (*(src+1) == '.' && (*(src+2) == '/' || *(src+2) == '\0')) {
                // ../ 처리 - 이전 디렉토리 제거
                src += 2;
                if (*src == '/') src++;
                
                // 이전 슬래시까지 되돌아가기
                if (dst > start) {
                    dst--;
                    while (dst > start && *dst != '/') {
                        dst--;
                    }
                }
            } else {
                *dst++ = *src++;
            }
        } else {
            *dst++ = *src++;
        }
    }
    
    // 마지막 슬래시 제거 (루트 디렉토리가 아닌 경우)
    if (dst > start + 1 && *(dst-1) == '/') {
        dst--;
    }
    
    *dst = '\0';
}

// 파일 타입 감지
const char *get_file_type_string(const char *path) {
    struct stat st;
    
    if (!path || stat(path, &st) != 0) {
        return "unknown";
    }
    
    if (S_ISREG(st.st_mode)) {
        // 확장자로 파일 타입 판단
        const char *ext = strrchr(path, '.');
        if (ext) {
            ext++; // '.' 다음으로 이동
            
            if (strcasecmp(ext, "gz") == 0) return "gzip";
            if (strcasecmp(ext, "z") == 0) return "zlib";
            if (strcasecmp(ext, "txt") == 0) return "text";
            if (strcasecmp(ext, "log") == 0) return "log";
            if (strcasecmp(ext, "c") == 0 || strcasecmp(ext, "h") == 0) return "source";
        }
        return "file";
    } else if (S_ISDIR(st.st_mode)) {
        return "directory";
    } else if (S_ISLNK(st.st_mode)) {
        return "symlink";
    } else if (S_ISBLK(st.st_mode)) {
        return "block";
    } else if (S_ISCHR(st.st_mode)) {
        return "character";
    } else if (S_ISFIFO(st.st_mode)) {
        return "fifo";
    } else if (S_ISSOCK(st.st_mode)) {
        return "socket";
    }
    
    return "special";
}

// 디렉토리 크기 계산 (재귀적)
size_t calculate_directory_size(const char *path) {
    DIR *dir;
    struct dirent *entry;
    struct stat st;
    char full_path[MAX_PATH];
    size_t total_size = 0;
    
    if (!path) return 0;
    
    dir = opendir(path);
    if (!dir) {
        return 0;
    }
    
    while ((entry = readdir(dir)) != NULL) {
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
            continue;
        }
        
        snprintf(full_path, sizeof(full_path), "%s/%s", path, entry->d_name);
        
        if (stat(full_path, &st) == 0) {
            if (S_ISREG(st.st_mode)) {
                total_size += st.st_size;
            } else if (S_ISDIR(st.st_mode)) {
                total_size += calculate_directory_size(full_path);
            }
        }
    }
    
    closedir(dir);
    return total_size;
}

// 파일 개수 계산 (재귀적)
size_t count_files_in_directory(const char *path) {
    DIR *dir;
    struct dirent *entry;
    char full_path[MAX_PATH];
    size_t file_count = 0;
    
    if (!path) return 0;
    
    dir = opendir(path);
    if (!dir) {
        return 0;
    }
    
    while ((entry = readdir(dir)) != NULL) {
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
            continue;
        }
        
        snprintf(full_path, sizeof(full_path), "%s/%s", path, entry->d_name);
        
        if (is_regular_file(full_path)) {
            file_count++;
        } else if (is_directory(full_path)) {
            file_count += count_files_in_directory(full_path);
        }
    }
    
    closedir(dir);
    return file_count;
}

// 안전한 파일 이름 생성 (특수 문자 제거/변환)
void sanitize_filename(char *filename) {
    char *p;
    
    if (!filename) return;
    
    for (p = filename; *p; p++) {
        // 위험한 문자들을 안전한 문자로 변환
        switch (*p) {
            case '/':
            case '\\':
            case ':':
            case '*':
            case '?':
            case '"':
            case '<':
            case '>':
            case '|':
                *p = '_';
                break;
            case '\0':
                return;
        }
    }
}

// 파일 잠금 (백업 중 동시 접근 방지)
int lock_file(const char *path) {
    char lock_file[MAX_PATH];
    FILE *lock_fp;
    
    if (!path) return ERROR_INVALID_PARAMS;
    
    snprintf(lock_file, sizeof(lock_file), "%s.lock", path);
    
    lock_fp = fopen(lock_file, "w");
    if (!lock_fp) {
        log_error("파일 잠금 생성 실패: %s", lock_file);
        return ERROR_FILE_WRITE;
    }
    
    fprintf(lock_fp, "%d\n", getpid());
    fclose(lock_fp);
    
    log_debug("파일 잠금 생성: %s", lock_file);
    return SUCCESS;
}

// 파일 잠금 해제
int unlock_file(const char *path) {
    char lock_file[MAX_PATH];
    
    if (!path) return ERROR_INVALID_PARAMS;
    
    snprintf(lock_file, sizeof(lock_file), "%s.lock", path);
    
    if (unlink(lock_file) != 0) {
        log_warning("파일 잠금 제거 실패: %s", lock_file);
        return ERROR_FILE_WRITE;
    }
    
    log_debug("파일 잠금 해제: %s", lock_file);
    return SUCCESS;
}