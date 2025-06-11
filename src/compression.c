#include "backup.h"

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
    FILE *src_file, *dest_file;
    char buffer[BUFFER_SIZE];
    size_t bytes_read, bytes_written;
    
    if (!source || !dest) {
        return ERROR_INVALID_PARAMS;
    }
    
    src_file = fopen(source, "rb");
    if (!src_file) {
        log_error("소스 파일 열기 실패: %s", source);
        return ERROR_FILE_OPEN;
    }
    
    dest_file = fopen(dest, "wb");
    if (!dest_file) {
        log_error("대상 파일 생성 실패: %s", dest);
        fclose(src_file);
        return ERROR_FILE_OPEN;
    }
    
    while ((bytes_read = fread(buffer, 1, BUFFER_SIZE, src_file)) > 0) {
        bytes_written = fwrite(buffer, 1, bytes_read, dest_file);
        if (bytes_written != bytes_read) {
            log_error("파일 쓰기 실패: %s", dest);
            fclose(src_file);
            fclose(dest_file);
            unlink(dest); // 불완전한 파일 제거
            return ERROR_FILE_WRITE;
        }
    }
    
    if (ferror(src_file)) {
        log_error("파일 읽기 실패: %s", source);
        fclose(src_file);
        fclose(dest_file);
        unlink(dest);
        return ERROR_FILE_READ;
    }
    
    fclose(src_file);
    fclose(dest_file);
    
    return SUCCESS;
}

// GZIP 압축
int compress_file_gzip(const char *source, const char *dest) {
    FILE *src_file;
    gzFile dest_gz;
    char buffer[BUFFER_SIZE];
    int bytes_read;
    
    src_file = fopen(source, "rb");
    if (!src_file) {
        log_error("소스 파일 열기 실패: %s", source);
        return ERROR_FILE_OPEN;
    }
    
    dest_gz = gzopen(dest, "wb9"); // 최대 압축 레벨
    if (!dest_gz) {
        log_error("GZIP 파일 생성 실패: %s", dest);
        fclose(src_file);
        return ERROR_FILE_OPEN;
    }
    
    while ((bytes_read = fread(buffer, 1, BUFFER_SIZE, src_file)) > 0) {
        if (gzwrite(dest_gz, buffer, bytes_read) != bytes_read) {
            log_error("GZIP 쓰기 실패: %s", dest);
            fclose(src_file);
            gzclose(dest_gz);
            unlink(dest);
            return ERROR_FILE_WRITE;
        }
    }
    
    if (ferror(src_file)) {
        log_error("파일 읽기 실패: %s", source);
        fclose(src_file);
        gzclose(dest_gz);
        unlink(dest);
        return ERROR_FILE_READ;
    }
    
    fclose(src_file);
    
    if (gzclose(dest_gz) != Z_OK) {
        log_error("GZIP 파일 닫기 실패: %s", dest);
        unlink(dest);
        return ERROR_FILE_WRITE;
    }
    
    return SUCCESS;
}

// GZIP 해제
int decompress_file_gzip(const char *source, const char *dest) {
    gzFile src_gz;
    FILE *dest_file;
    char buffer[BUFFER_SIZE];
    int bytes_read;
    
    src_gz = gzopen(source, "rb");
    if (!src_gz) {
        log_error("GZIP 파일 열기 실패: %s", source);
        return ERROR_FILE_OPEN;
    }
    
    dest_file = fopen(dest, "wb");
    if (!dest_file) {
        log_error("대상 파일 생성 실패: %s", dest);
        gzclose(src_gz);
        return ERROR_FILE_OPEN;
    }
    
    while ((bytes_read = gzread(src_gz, buffer, BUFFER_SIZE)) > 0) {
        if (fwrite(buffer, 1, bytes_read, dest_file) != (size_t)bytes_read) {
            log_error("파일 쓰기 실패: %s", dest);
            gzclose(src_gz);
            fclose(dest_file);
            unlink(dest);
            return ERROR_FILE_WRITE;
        }
    }
    
    if (bytes_read < 0) {
        log_error("GZIP 읽기 실패: %s", source);
        gzclose(src_gz);
        fclose(dest_file);
        unlink(dest);
        return ERROR_FILE_READ;
    }
    
    gzclose(src_gz);
    fclose(dest_file);
    
    return SUCCESS;
}

// ZLIB 압축
int compress_file_zlib(const char *source, const char *dest) {
    FILE *src_file, *dest_file;
    z_stream strm;
    unsigned char in[BUFFER_SIZE];
    unsigned char out[BUFFER_SIZE];
    int ret, flush;
    unsigned have;
    
    src_file = fopen(source, "rb");
    if (!src_file) {
        log_error("소스 파일 열기 실패: %s", source);
        return ERROR_FILE_OPEN;
    }
    
    dest_file = fopen(dest, "wb");
    if (!dest_file) {
        log_error("대상 파일 생성 실패: %s", dest);
        fclose(src_file);
        return ERROR_FILE_OPEN;
    }
    
    // zlib 초기화
    strm.zalloc = Z_NULL;
    strm.zfree = Z_NULL;
    strm.opaque = Z_NULL;
    ret = deflateInit(&strm, Z_BEST_COMPRESSION);
    if (ret != Z_OK) {
        log_error("ZLIB 초기화 실패");
        fclose(src_file);
        fclose(dest_file);
        unlink(dest);
        return ERROR_COMPRESSION;
    }
    
    // 압축 수행
    do {
        strm.avail_in = fread(in, 1, BUFFER_SIZE, src_file);
        if (ferror(src_file)) {
            deflateEnd(&strm);
            fclose(src_file);
            fclose(dest_file);
            unlink(dest);
            return ERROR_FILE_READ;
        }
        
        flush = feof(src_file) ? Z_FINISH : Z_NO_FLUSH;
        strm.next_in = in;
        
        do {
            strm.avail_out = BUFFER_SIZE;
            strm.next_out = out;
            
            ret = deflate(&strm, flush);
            if (ret == Z_STREAM_ERROR) {
                deflateEnd(&strm);
                fclose(src_file);
                fclose(dest_file);
                unlink(dest);
                return ERROR_COMPRESSION;
            }
            
            have = BUFFER_SIZE - strm.avail_out;
            if (fwrite(out, 1, have, dest_file) != have || ferror(dest_file)) {
                deflateEnd(&strm);
                fclose(src_file);
                fclose(dest_file);
                unlink(dest);
                return ERROR_FILE_WRITE;
            }
        } while (strm.avail_out == 0);
        
    } while (flush != Z_FINISH);
    
    deflateEnd(&strm);
    fclose(src_file);
    fclose(dest_file);
    
    return SUCCESS;
}

// ZLIB 해제
int decompress_file_zlib(const char *source, const char *dest) {
    FILE *src_file, *dest_file;
    z_stream strm;
    unsigned char in[BUFFER_SIZE];
    unsigned char out[BUFFER_SIZE];
    int ret;
    unsigned have;
    
    src_file = fopen(source, "rb");
    if (!src_file) {
        log_error("ZLIB 파일 열기 실패: %s", source);
        return ERROR_FILE_OPEN;
    }
    
    dest_file = fopen(dest, "wb");
    if (!dest_file) {
        log_error("대상 파일 생성 실패: %s", dest);
        fclose(src_file);
        return ERROR_FILE_OPEN;
    }
    
    // zlib 초기화
    strm.zalloc = Z_NULL;
    strm.zfree = Z_NULL;
    strm.opaque = Z_NULL;
    strm.avail_in = 0;
    strm.next_in = Z_NULL;
    ret = inflateInit(&strm);
    if (ret != Z_OK) {
        log_error("ZLIB 초기화 실패");
        fclose(src_file);
        fclose(dest_file);
        unlink(dest);
        return ERROR_COMPRESSION;
    }
    
    // 압축 해제 수행
    do {
        strm.avail_in = fread(in, 1, BUFFER_SIZE, src_file);
        if (ferror(src_file)) {
            inflateEnd(&strm);
            fclose(src_file);
            fclose(dest_file);
            unlink(dest);
            return ERROR_FILE_READ;
        }
        
        if (strm.avail_in == 0) break;
        strm.next_in = in;
        
        do {
            strm.avail_out = BUFFER_SIZE;
            strm.next_out = out;
            
            ret = inflate(&strm, Z_NO_FLUSH);
            switch (ret) {
                case Z_NEED_DICT:
                case Z_DATA_ERROR:
                case Z_MEM_ERROR:
                    inflateEnd(&strm);
                    fclose(src_file);
                    fclose(dest_file);
                    unlink(dest);
                    return ERROR_COMPRESSION;
            }
            
            have = BUFFER_SIZE - strm.avail_out;
            if (fwrite(out, 1, have, dest_file) != have || ferror(dest_file)) {
                inflateEnd(&strm);
                fclose(src_file);
                fclose(dest_file);
                unlink(dest);
                return ERROR_FILE_WRITE;
            }
        } while (strm.avail_out == 0);
        
    } while (ret != Z_STREAM_END);
    
    inflateEnd(&strm);
    fclose(src_file);
    fclose(dest_file);
    
    return SUCCESS;
}

// 메인 압축 함수
int compress_file(const char *source, const char *dest, compression_type_t type) {
    if (type == COMPRESS_NONE) {
        return copy_file_simple(source, dest);
    }
    
    if (type != COMPRESS_GZIP && type != COMPRESS_ZLIB) {
        log_error("지원되지 않는 압축 타입: %d", type);
        return ERROR_COMPRESSION;
    }
    
    switch (type) {
        case COMPRESS_GZIP:
            return compress_file_gzip(source, dest);
        case COMPRESS_ZLIB:
            return compress_file_zlib(source, dest);
        default:
            return ERROR_COMPRESSION;
    }
}

// 메인 압축 해제 함수
int decompress_file(const char *source, const char *dest, compression_type_t type) {
    if (type == COMPRESS_NONE) {
        return copy_file_simple(source, dest);
    }
    
    if (type != COMPRESS_GZIP && type != COMPRESS_ZLIB) {
        log_error("지원되지 않는 압축 타입: %d", type);
        return ERROR_COMPRESSION;
    }
    
    switch (type) {
        case COMPRESS_GZIP:
            return decompress_file_gzip(source, dest);
        case COMPRESS_ZLIB:
            return decompress_file_zlib(source, dest);
        default:
            return ERROR_COMPRESSION;
    }
}

// 압축률 계산
double calculate_compression_ratio(const char *original, const char *compressed) {
    size_t orig_size = get_file_size(original);
    size_t comp_size = get_file_size(compressed);
    
    if (orig_size == 0) return 0.0;
    
    return ((double)comp_size / orig_size) * 100.0;
}

// 압축 레벨 최적화 (향후 확장용)
int get_optimal_compression_level(const char *file_path) {
    size_t file_size = get_file_size(file_path);
    const char *ext = strrchr(file_path, '.');
    
    // 파일 크기에 따른 압축 레벨 조정
    if (file_size < 1024) {
        return 1; // 작은 파일은 빠른 압축
    } else if (file_size < 1024 * 1024) {
        return 6; // 중간 크기 파일은 균형
    } else {
        return 9; // 큰 파일은 최대 압축
    }
    
    // 파일 타입에 따른 압축 레벨 조정 (향후 구현)
    if (ext) {
        if (strcmp(ext, ".txt") == 0 || strcmp(ext, ".log") == 0) {
            return 9; // 텍스트 파일은 압축률이 좋음
        } else if (strcmp(ext, ".jpg") == 0 || strcmp(ext, ".png") == 0) {
            return 1; // 이미 압축된 파일은 빠른 압축
        }
    }
    
    return 6; // 기본값
}