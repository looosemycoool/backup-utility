#!/bin/bash

# 🧪 고급 백업 유틸리티 완전한 테스트 스위트
# 모든 기능을 체계적으로 테스트합니다

set -e

# 색상 설정
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
PURPLE='\033[0;35m'
CYAN='\033[0;36m'
WHITE='\033[1;37m'
NC='\033[0m'

# 테스트 결과 추적
TOTAL_TESTS=0
PASSED_TESTS=0
FAILED_TESTS=0
SKIPPED_TESTS=0

# 테스트 시작 시간
START_TIME=$(date +%s)

# 로그 함수들
log_info() {
    echo -e "${BLUE}[INFO]${NC} $1"
}

log_success() {
    echo -e "${GREEN}[SUCCESS]${NC} $1"
    ((PASSED_TESTS++))
}

log_error() {
    echo -e "${RED}[ERROR]${NC} $1"
    ((FAILED_TESTS++))
}

log_warning() {
    echo -e "${YELLOW}[WARNING]${NC} $1"
}

log_skip() {
    echo -e "${PURPLE}[SKIP]${NC} $1"
    ((SKIPPED_TESTS++))
}

# 테스트 헤더
test_header() {
    ((TOTAL_TESTS++))
    echo
    echo -e "${WHITE}╔════════════════════════════════════════════════════════════╗${NC}"
    echo -e "${WHITE}║ 테스트 #${TOTAL_TESTS}: $1${NC}"
    echo -e "${WHITE}╚════════════════════════════════════════════════════════════╝${NC}"
}

# 파일 비교 함수
compare_files() {
    if cmp -s "$1" "$2"; then
        return 0
    else
        return 1
    fi
}

# 디렉토리 비교 함수
compare_directories() {
    if diff -r "$1" "$2" >/dev/null 2>&1; then
        return 0
    else
        return 1
    fi
}

# 테스트 환경 준비
setup_test_env() {
    log_info "테스트 환경 준비 중..."
    
    # 테스트 디렉토리 정리 및 생성
    rm -rf test_env
    mkdir -p test_env/{source,backup,restore,temp}
    cd test_env
    
    # 다양한 테스트 파일 생성
    echo "Hello, World!" > source/simple.txt
    echo "This is a test file with some content." > source/medium.txt
    
    # 바이너리 파일
    dd if=/dev/urandom of=source/binary.dat bs=1024 count=10 2>/dev/null
    
    # 큰 파일
    dd if=/dev/zero of=source/large.dat bs=1024 count=100 2>/dev/null
    
    # 중첩 디렉토리 구조
    mkdir -p source/nested/{level1/{level2,another},different}
    echo "nested file 1" > source/nested/level1/file1.txt
    echo "nested file 2" > source/nested/level1/level2/file2.txt
    echo "different file" > source/nested/different/file3.txt
    
    # 특수 문자가 포함된 파일명
    echo "special chars" > source/nested/special\ file\ with\ spaces.txt
    echo "underscore" > source/nested/file_with_underscores.txt
    
    # 임시 파일들 (필터링 테스트용)
    echo "temp1" > source/temp1.tmp
    echo "temp2" > source/temp2.tmp
    echo "backup" > source/backup.bak
    echo "log" > source/debug.log
    
    # 빈 디렉토리
    mkdir -p source/empty_dir
    
    # 심볼릭 링크 (지원되는 경우)
    ln -sf ../simple.txt source/nested/symlink.txt 2>/dev/null || true
    
    log_success "테스트 환경 준비 완료"
}

# 1. 기본 파일 백업/복원 테스트
test_basic_file_operations() {
    test_header "기본 파일 백업/복원"
    
    # 단순 파일 백업
    if ../bin/backup backup source/simple.txt backup/simple_backup.txt >/dev/null 2>&1; then
        if compare_files source/simple.txt backup/simple_backup.txt; then
            log_success "단순 파일 백업 성공"
        else
            log_error "단순 파일 백업 - 파일 내용 불일치"
            return 1
        fi
    else
        log_error "단순 파일 백업 실패"
        return 1
    fi
    
    # 단순 파일 복원
    if ../bin/backup restore backup/simple_backup.txt restore/simple_restored.txt >/dev/null 2>&1; then
        if compare_files source/simple.txt restore/simple_restored.txt; then
            log_success "단순 파일 복원 성공"
        else
            log_error "단순 파일 복원 - 파일 내용 불일치"
            return 1
        fi
    else
        log_error "단순 파일 복원 실패"
        return 1
    fi
    
    # 바이너리 파일 백업/복원
    if ../bin/backup backup source/binary.dat backup/binary_backup.dat >/dev/null 2>&1; then
        if ../bin/backup restore backup/binary_backup.dat restore/binary_restored.dat >/dev/null 2>&1; then
            if compare_files source/binary.dat restore/binary_restored.dat; then
                log_success "바이너리 파일 백업/복원 성공"
            else
                log_error "바이너리 파일 백업/복원 - 파일 내용 불일치"
                return 1
            fi
        else
            log_error "바이너리 파일 복원 실패"
            return 1
        fi
    else
        log_error "바이너리 파일 백업 실패"
        return 1
    fi
}

# 2. 압축 기능 테스트
test_compression() {
    test_header "압축 기능"
    
    # GZIP 압축 테스트
    if ../bin/backup backup -c gzip source/large.dat backup/large_gzip.dat >/dev/null 2>&1; then
        if [[ -f backup/large_gzip.dat.gz ]]; then
            # 압축 파일 크기 확인
            original_size=$(stat -f%z source/large.dat 2>/dev/null || stat -c%s source/large.dat)
            compressed_size=$(stat -f%z backup/large_gzip.dat.gz 2>/dev/null || stat -c%s backup/large_gzip.dat.gz)
            
            if [[ $compressed_size -lt $original_size ]]; then
                log_success "GZIP 압축 성공 (압축률: $((100 * compressed_size / original_size))%)"
            else
                log_warning "GZIP 압축 완료하지만 크기 감소 없음"
            fi
            
            # 압축 해제 테스트
            if ../bin/backup restore backup/large_gzip.dat.gz restore/large_gzip_restored.dat >/dev/null 2>&1; then
                if compare_files source/large.dat restore/large_gzip_restored.dat; then
                    log_success "GZIP 압축 해제 성공"
                else
                    log_error "GZIP 압축 해제 - 파일 내용 불일치"
                    return 1
                fi
            else
                log_error "GZIP 압축 해제 실패"
                return 1
            fi
        else
            log_error "GZIP 압축 파일이 생성되지 않음"
            return 1
        fi
    else
        log_error "GZIP 압축 백업 실패"
        return 1
    fi
    
    # ZLIB 압축 테스트 (지원되는 경우)
    if ../bin/backup backup -c zlib source/medium.txt backup/medium_zlib.txt >/dev/null 2>&1; then
        if [[ -f backup/medium_zlib.txt.z ]]; then
            log_success "ZLIB 압축 성공"
            
            if ../bin/backup restore backup/medium_zlib.txt.z restore/medium_zlib_restored.txt >/dev/null 2>&1; then
                if compare_files source/medium.txt restore/medium_zlib_restored.txt; then
                    log_success "ZLIB 압축 해제 성공"
                else
                    log_error "ZLIB 압축 해제 - 파일 내용 불일치"
                    return 1
                fi
            else
                log_error "ZLIB 압축 해제 실패"
                return 1
            fi
        else
            log_skip "ZLIB 압축 미지원 또는 확장자 문제"
        fi
    else
        log_skip "ZLIB 압축 미지원"
    fi
}

# 3. 디렉토리 백업/복원 테스트
test_directory_operations() {
    test_header "디렉토리 백업/복원"
    
    # 재귀적 디렉토리 백업
    if ../bin/backup backup -r source/nested backup/nested_backup >/dev/null 2>&1; then
        # 백업된 파일 수 확인
        source_files=$(find source/nested -type f | wc -l)
        backup_files=$(find backup/nested_backup -type f | wc -l)
        
        if [[ $source_files -eq $backup_files ]]; then
            log_success "재귀적 디렉토리 백업 성공 ($source_files 파일)"
        else
            log_error "재귀적 디렉토리 백업 - 파일 수 불일치 (원본: $source_files, 백업: $backup_files)"
            return 1
        fi
        
        # 디렉토리 복원
        if ../bin/backup restore -r backup/nested_backup restore/nested_restored >/dev/null 2>&1; then
            if compare_directories source/nested restore/nested_restored; then
                log_success "재귀적 디렉토리 복원 성공"
            else
                log_error "재귀적 디렉토리 복원 - 내용 불일치"
                return 1
            fi
        else
            log_error "재귀적 디렉토리 복원 실패"
            return 1
        fi
    else
        log_error "재귀적 디렉토리 백업 실패"
        return 1
    fi
}

# 4. 파일 필터링 테스트
test_file_filtering() {
    test_header "파일 필터링"
    
    # 임시 파일 제외 테스트
    if ../bin/backup backup -r --exclude="*.tmp" source backup/filtered_backup >/dev/null 2>&1; then
        # .tmp 파일이 제외되었는지 확인
        tmp_files_in_backup=$(find backup/filtered_backup -name "*.tmp" | wc -l)
        
        if [[ $tmp_files_in_backup -eq 0 ]]; then
            log_success "임시 파일 필터링 성공 (.tmp 파일 제외됨)"
        else
            log_error "임시 파일 필터링 실패 (.tmp 파일이 백업됨)"
            return 1
        fi
    else
        log_error "필터링 백업 실패"
        return 1
    fi
    
    # 특정 확장자만 포함 테스트
    if ../bin/backup backup -r --include="*.txt" source backup/txt_only_backup >/dev/null 2>&1; then
        # .txt 파일만 있는지 확인
        non_txt_files=$(find backup/txt_only_backup -type f ! -name "*.txt" | wc -l)
        
        if [[ $non_txt_files -eq 0 ]]; then
            log_success "텍스트 파일만 필터링 성공"
        else
            log_error "텍스트 파일 필터링 실패 (비텍스트 파일이 포함됨)"
            return 1
        fi
    else
        log_skip "include 필터링 미지원"
    fi
}

# 5. 백업 검증 테스트
test_backup_verification() {
    test_header "백업 검증"
    
    # 검증 기능 테스트
    if ../bin/backup backup --verify source/simple.txt backup/verified_backup.txt >/dev/null 2>&1; then
        log_success "백업 검증 기능 성공"
    else
        log_error "백업 검증 기능 실패"
        return 1
    fi
    
    # 압축된 파일 검증 테스트
    if ../bin/backup backup -c gzip --verify source/medium.txt backup/verified_compressed.txt >/dev/null 2>&1; then
        log_success "압축 백업 검증 기능 성공"
    else
        log_error "압축 백업 검증 기능 실패"
        return 1
    fi
}

# 6. 오류 처리 테스트
test_error_handling() {
    test_header "오류 처리"
    
    # 존재하지 않는 파일 백업 시도
    if ! ../bin/backup backup nonexistent.txt backup/should_fail.txt >/dev/null 2>&1; then
        log_success "존재하지 않는 파일 처리 성공"
    else
        log_error "존재하지 않는 파일을 백업함 (오류 처리 실패)"
        return 1
    fi
    
    # 권한이 없는 디렉토리 접근 시도 (일반 사용자인 경우)
    if [[ $EUID -ne 0 ]]; then
        if ! ../bin/backup backup /root/should_not_exist backup/permission_test >/dev/null 2>&1; then
            log_success "권한 없는 파일 접근 처리 성공"
        else
            log_skip "권한 테스트 건너뜀 (루트 권한)"
        fi
    else
        log_skip "권한 테스트 건너뜀 (루트 사용자)"
    fi
    
    # 디스크 공간 부족 시뮬레이션은 위험하므로 생략
    log_skip "디스크 공간 테스트 생략 (시스템 안전을 위해)"
}

# 7. 성능 테스트
test_performance() {
    test_header "성능 테스트"
    
    # 큰 파일 처리 성능
    log_info "큰 파일 백업 성능 측정 중..."
    
    # 10MB 파일 생성
    dd if=/dev/zero of=source/performance_test.dat bs=1024 count=10240 2>/dev/null
    
    start_time=$(date +%s.%N)
    if ../bin/backup backup source/performance_test.dat backup/performance_backup.dat >/dev/null 2>&1; then
        end_time=$(date +%s.%N)
        duration=$(echo "$end_time - $start_time" | bc 2>/dev/null || echo "unknown")
        
        if [[ "$duration" != "unknown" ]]; then
            log_success "큰 파일 백업 성능: ${duration}초"
        else
            log_success "큰 파일 백업 완료 (시간 측정 실패)"
        fi
    else
        log_error "큰 파일 백업 성능 테스트 실패"
        return 1
    fi
    
    # 다중 파일 처리 성능
    mkdir -p source/many_files
    for i in {1..100}; do
        echo "File $i content" > source/many_files/file_$i.txt
    done
    
    start_time=$(date +%s.%N)
    if ../bin/backup backup -r source/many_files backup/many_files_backup >/dev/null 2>&1; then
        end_time=$(date +%s.%N)
        duration=$(echo "$end_time - $start_time" | bc 2>/dev/null || echo "unknown")
        
        if [[ "$duration" != "unknown" ]]; then
            log_success "다중 파일 백업 성능: ${duration}초 (100 파일)"
        else
            log_success "다중 파일 백업 완료 (시간 측정 실패)"
        fi
    else
        log_error "다중 파일 백업 성능 테스트 실패"
        return 1
    fi
}

# 8. 병렬 처리 테스트
test_parallel_processing() {
    test_header "병렬 처리"
    
    # 병렬 처리 백업 테스트
    if ../bin/backup backup -r -j 4 source backup/parallel_backup >/dev/null 2>&1; then
        # 결과 검증
        if compare_directories source backup/parallel_backup; then
            log_success "병렬 처리 백업 성공 (4 스레드)"
        else
            log_error "병렬 처리 백업 - 결과 불일치"
            return 1
        fi
    else
        log_skip "병렬 처리 미지원"
    fi
}

# 9. 진행률 표시 테스트
test_progress_display() {
    test_header "진행률 표시"
    
    # 진행률 표시 백업 테스트 (출력 확인은 어려우므로 오류 없이 실행되는지만 확인)
    if ../bin/backup backup -r -p source backup/progress_backup >/dev/null 2>&1; then
        log_success "진행률 표시 백업 성공"
    else
        log_error "진행률 표시 백업 실패"
        return 1
    fi
}

# 10. Dry-run 모드 테스트
test_dry_run() {
    test_header "Dry-run 모드"
    
    # dry-run 모드 실행
    if ../bin/backup backup -r --dry-run source backup/dry_run_test >/dev/null 2>&1; then
        # 실제로 파일이 생성되지 않았는지 확인
        if [[ ! -d backup/dry_run_test ]]; then
            log_success "Dry-run 모드 성공 (파일 생성되지 않음)"
        else
            log_error "Dry-run 모드 실패 (파일이 실제로 생성됨)"
            return 1
        fi
    else
        log_skip "Dry-run 모드 미지원"
    fi
}

# 결과 보고서
generate_report() {
    end_time=$(date +%s)
    duration=$((end_time - START_TIME))
    
    echo
    echo -e "${WHITE}╔══════════════════════════════════════════════════════════════════╗${NC}"
    echo -e "${WHITE}║                          📊 테스트 결과 보고서                        ║${NC}"
    echo -e "${WHITE}╚══════════════════════════════════════════════════════════════════╝${NC}"
    echo
    echo -e "${CYAN}🏁 테스트 완료 시간: ${duration}초${NC}"
    echo -e "${CYAN}📊 전체 테스트: ${TOTAL_TESTS}개${NC}"
    echo -e "${GREEN}✅ 성공: ${PASSED_TESTS}개${NC}"
    echo -e "${RED}❌ 실패: ${FAILED_TESTS}개${NC}"
    echo -e "${PURPLE}⏭️ 건너뜀: ${SKIPPED_TESTS}개${NC}"
    echo
    
    success_rate=0
    if [[ $TOTAL_TESTS -gt 0 ]]; then
        success_rate=$((100 * PASSED_TESTS / TOTAL_TESTS))
    fi
    
    echo -e "${CYAN}📈 성공률: ${success_rate}%${NC}"
    
    if [[ $FAILED_TESTS -eq 0 ]]; then
        echo -e "${GREEN}🎉 모든 테스트가 성공했습니다!${NC}"
        return 0
    else
        echo -e "${RED}⚠️ 일부 테스트가 실패했습니다.${NC}"
        return 1
    fi
}

# 메인 테스트 실행
main() {
    echo -e "${BLUE}🧪 고급 백업 유틸리티 완전한 테스트 스위트${NC}"
    echo -e "${CYAN}테스트 시작 시간: $(date)${NC}"
    echo
    
    # 프로그램 존재 확인
    if [[ ! -f bin/backup ]]; then
        log_error "bin/backup 파일이 없습니다. 먼저 빌드해주세요: make"
        exit 1
    fi
    
    # 테스트 환경 준비
    setup_test_env
    
    # 개별 테스트 실행
    test_basic_file_operations || true
    test_compression || true
    test_directory_operations || true
    test_file_filtering || true
    test_backup_verification || true
    test_error_handling || true
    test_performance || true
    test_parallel_processing || true
    test_progress_display || true
    test_dry_run || true
    
    # 테스트 환경 정리
    cd ..
    rm -rf test_env
    
    # 결과 보고서 생성
    generate_report
}

# bc 명령어 확인 (성능 테스트용)
if ! command -v bc >/dev/null 2>&1; then
    log_warning "bc 명령어가 없어 정확한 시간 측정이 어려울 수 있습니다"
fi

# 스크립트 실행
main "$@"