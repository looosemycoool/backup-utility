#include "backup.h"

int file_exists(const char *path) {
    return access(path, F_OK) == 0;
}

int is_directory(const char *path) {
    struct stat st;
    return stat(path, &st) == 0 && S_ISDIR(st.st_mode);
}

int is_regular_file(const char *path) {
    struct stat st;
    return stat(path, &st) == 0 && S_ISREG(st.st_mode);
}

int create_directory(const char *path) {
    return mkdir(path, 0755);
}

int create_directory_recursive(const char *path) {
    char tmp[MAX_PATH];
    char *p = NULL;
    size_t len;

    snprintf(tmp, sizeof(tmp), "%s", path);
    len = strlen(tmp);
    if (tmp[len - 1] == '/') {
        tmp[len - 1] = 0;
    }

    for (p = tmp + 1; *p; p++) {
        if (*p == '/') {
            *p = 0;
            mkdir(tmp, 0755);
            *p = '/';
        }
    }
    return mkdir(tmp, 0755);
}

char *get_relative_path(const char *base, const char *path) {
    static char relative[MAX_PATH];
    size_t base_len = strlen(base);
    
    if (strncmp(path, base, base_len) == 0) {
        const char *rel = path + base_len;
        while (*rel == '/') rel++;  // 시작 슬래시 제거
        strncpy(relative, rel, sizeof(relative) - 1);
        relative[sizeof(relative) - 1] = '\0';
        return relative;
    }
    
    strncpy(relative, path, sizeof(relative) - 1);
    relative[sizeof(relative) - 1] = '\0';
    return relative;
}

int match_pattern(const char *pattern, const char *string) {
    return fnmatch(pattern, string, FNM_PATHNAME) == 0;
}

int should_include_file(const char *path, const backup_options_t *opts) {
    const char *filename = strrchr(path, '/');
    filename = filename ? filename + 1 : path;
    
    // 제외 패턴 확인
    for (int i = 0; i < opts->exclude_count; i++) {
        if (match_pattern(opts->exclude_patterns[i], filename) ||
            match_pattern(opts->exclude_patterns[i], path)) {
            return 0;
        }
    }
    
    // 최대 파일 크기 확인
    if (opts->max_file_size > 0) {
        long size = get_file_size(path);
        if (size > opts->max_file_size) {
            return 0;
        }
    }
    
    return 1;
}

long get_file_size(const char *path) {
    struct stat st;
    if (stat(path, &st) == 0) {
        return st.st_size;
    }
    return -1;
}

// 간단한 체크섬 계산 (DJB2 해시)
char *calculate_checksum(const char *path) {
    FILE *file;
    unsigned char buffer[BUFFER_SIZE];
    size_t bytes_read;
    unsigned int hash = 5381;
    static char checksum[17];
    
    file = fopen(path, "rb");
    if (!file) {
        log_error("체크섬 계산을 위한 파일 열기 실패: %s", path);
        return NULL;
    }
    
    while ((bytes_read = fread(buffer, 1, sizeof(buffer), file)) > 0) {
        for (size_t i = 0; i < bytes_read; i++) {
            hash = ((hash << 5) + hash) + buffer[i];
        }
    }
    
    fclose(file);
    snprintf(checksum, sizeof(checksum), "%08x", hash);
    
    return checksum;
}

int compare_files(const char *file1, const char *file2) {
    FILE *f1, *f2;
    char buf1[BUFFER_SIZE], buf2[BUFFER_SIZE];
    size_t bytes1, bytes2;
    
    f1 = fopen(file1, "rb");
    f2 = fopen(file2, "rb");
    
    if (!f1 || !f2) {
        if (f1) fclose(f1);
        if (f2) fclose(f2);
        return 0;
    }
    
    do {
        bytes1 = fread(buf1, 1, sizeof(buf1), f1);
        bytes2 = fread(buf2, 1, sizeof(buf2), f2);
        
        if (bytes1 != bytes2 || memcmp(buf1, buf2, bytes1) != 0) {
            fclose(f1);
            fclose(f2);
            return 0;
        }
    } while (bytes1 > 0);
    
    fclose(f1);
    fclose(f2);
    return 1;
}

long calculate_directory_size(const char *path) {
    DIR *dir;
    struct dirent *entry;
    struct stat st;
    char full_path[MAX_PATH];
    long total_size = 0;
    
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
            if (S_ISDIR(st.st_mode)) {
                total_size += calculate_directory_size(full_path);
            } else if (S_ISREG(st.st_mode)) {
                total_size += st.st_size;
            }
        }
    }
    
    closedir(dir);
    return total_size;
}

long count_directory_files(const char *path) {
    DIR *dir;
    struct dirent *entry;
    char full_path[MAX_PATH];
    long file_count = 0;
    
    dir = opendir(path);
    if (!dir) {
        return 0;
    }
    
    while ((entry = readdir(dir)) != NULL) {
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
            continue;
        }
        
        snprintf(full_path, sizeof(full_path), "%s/%s", path, entry->d_name);
        
        if (is_directory(full_path)) {
            file_count += count_directory_files(full_path);
        } else if (is_regular_file(full_path)) {
            file_count++;
        }
    }
    
    closedir(dir);
    return file_count;
}
