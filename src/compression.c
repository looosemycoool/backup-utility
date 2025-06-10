#include "backup.h"

int compress_file(const char *source, const char *dest, compression_type_t type) {
    if (type == COMPRESS_NONE) {
        return copy_file_simple(source, dest);
    }
    
    if (type != COMPRESS_GZIP && type != COMPRESS_ZLIB) {
        log_error("지원되지 않는 압축 타입: %d", type);
        return ERROR_COMPRESSION;
    }
    
    FILE *src_file = fopen(source, "rb");
    if (!src_file) {
        log_error("소스 파일 열기 실패: %s", source);
        return ERROR_FILE_OPEN;
    }
    
    gzFile dest_gz = gzopen(dest, "wb");
    if (!dest_gz) {
        log_error("압축 파일 생성 실패: %s", dest);
        fclose(src_file);
        return ERROR_FILE_OPEN;
    }
    
    char buffer[BUFFER_SIZE];
    size_t bytes_read;
    long total_read = 0, total_written = 0;
    
    log_debug("압축 시작: %s -> %s", source, dest);
    
    while ((bytes_read = fread(buffer, 1, sizeof(buffer), src_file)) > 0) {
        total_read += bytes_read;
        
        int bytes_written = gzwrite(dest_gz, buffer, bytes_read);
        if (bytes_written != (int)bytes_read) {
            log_error("압축 중 쓰기 오류");
            fclose(src_file);
            gzclose(dest_gz);
            return ERROR_COMPRESSION;
        }
        total_written += bytes_written;
    }
    
    fclose(src_file);
    gzclose(dest_gz);
    
    // 압축률 계산
    if (total_read > 0) {
        double ratio = (double)total_written / total_read * 100.0;
        log_debug("압축 완료: %ld -> %ld bytes (%.1f%%)", total_read, total_written, ratio);
        
        // 전역 통계 업데이트
        g_stats.bytes_compressed += total_written;
    }
    
    return SUCCESS;
}

int decompress_file(const char *source, const char *dest, compression_type_t type) {
    if (type == COMPRESS_NONE) {
        return copy_file_simple(source, dest);
    }
    
    if (type != COMPRESS_GZIP && type != COMPRESS_ZLIB) {
        log_error("지원되지 않는 압축 해제 타입: %d", type);
        return ERROR_COMPRESSION;
    }
    
    gzFile src_gz = gzopen(source, "rb");
    if (!src_gz) {
        log_error("압축 파일 열기 실패: %s", source);
        return ERROR_FILE_OPEN;
    }
    
    FILE *dest_file = fopen(dest, "wb");
    if (!dest_file) {
        log_error("대상 파일 생성 실패: %s", dest);
        gzclose(src_gz);
        return ERROR_FILE_OPEN;
    }
    
    char buffer[BUFFER_SIZE];
    int bytes_read;
    long total_written = 0;
    
    log_debug("압축 해제 시작: %s -> %s", source, dest);
    
    while ((bytes_read = gzread(src_gz, buffer, sizeof(buffer))) > 0) {
        if (fwrite(buffer, 1, bytes_read, dest_file) != (size_t)bytes_read) {
            log_error("압축 해제 중 쓰기 오류");
            gzclose(src_gz);
            fclose(dest_file);
            return ERROR_COMPRESSION;
        }
        total_written += bytes_read;
    }
    
    if (bytes_read < 0) {
        log_error("압축 해제 중 읽기 오류");
        gzclose(src_gz);
        fclose(dest_file);
        return ERROR_COMPRESSION;
    }
    
    gzclose(src_gz);
    fclose(dest_file);
    
    log_debug("압축 해제 완료: %ld bytes", total_written);
    
    return SUCCESS;
}

const char *get_compression_extension(compression_type_t type) {
    switch (type) {
        case COMPRESS_GZIP:
            return ".gz";
        case COMPRESS_ZLIB:
            return ".z";
        default:
            return "";
    }
}

compression_type_t get_compression_type(const char *filename) {
    const char *ext = strrchr(filename, '.');
    if (!ext) return COMPRESS_NONE;
    
    if (strcmp(ext, ".gz") == 0) return COMPRESS_GZIP;
    if (strcmp(ext, ".z") == 0) return COMPRESS_ZLIB;
    
    return COMPRESS_NONE;
}

// 간단한 파일 복사 (압축 없음)
int copy_file_simple(const char *source, const char *dest) {
    FILE *src = fopen(source, "rb");
    FILE *dst = fopen(dest, "wb");
    
    if (!src || !dst) {
        if (src) fclose(src);
        if (dst) fclose(dst);
        return ERROR_FILE_OPEN;
    }
    
    char buffer[BUFFER_SIZE];
    size_t bytes;
    long total_bytes = 0;
    
    while ((bytes = fread(buffer, 1, sizeof(buffer), src)) > 0) {
        if (fwrite(buffer, 1, bytes, dst) != bytes) {
            fclose(src);
            fclose(dst);
            return ERROR_GENERAL;
        }
        total_bytes += bytes;
    }
    
    fclose(src);
    fclose(dst);
    
    g_stats.bytes_processed += total_bytes;
    
    return SUCCESS;
}
