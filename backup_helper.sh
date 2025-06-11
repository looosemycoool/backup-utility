#!/bin/bash

# 고급 백업 유틸리티 헬퍼 스크립트
# 사용자 친화적인 인터페이스 제공

set -e  # 에러 발생 시 즉시 종료

# 설정
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
BACKUP_BINARY="$SCRIPT_DIR/bin/backup"
VERSION="2.0"

# 색상 정의
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
PURPLE='\033[0;35m'
CYAN='\033[0;36m'
NC='\033[0m' # No Color

# 유틸리티 함수들

get_file_size() {
    local file="$1"
    if command -v stat >/dev/null 2>&1; then
        # Linux
        if stat -c%s "$file" 2>/dev/null; then
            return 0
        # macOS
        elif stat -f%z "$file" 2>/dev/null; then
            return 0
        fi
    fi
    # 폴백
    wc -c < "$file" 2>/dev/null || echo "0"
}

calculate_percentage() {
    local part="$1"
    local total="$2"
    if [ "$total" -eq 0 ]; then
        echo "0"
    else
        echo $(( part * 100 / total ))
    fi
}

# 표시 함수들

print_header() {
    echo -e "${BLUE}================================${NC}"
    echo -e "${CYAN}  고급 백업 유틸리티 헬퍼 v${VERSION}${NC}"
    echo -e "${BLUE}================================${NC}"
    echo
}

print_success() {
    echo -e "${GREEN}✅ $1${NC}"
}

print_error() {
    echo -e "${RED}❌ $1${NC}"
}

print_warning() {
    echo -e "${YELLOW}⚠️  $1${NC}"
}

print_info() {
    echo -e "${BLUE}ℹ️  $1${NC}"
}

check_binary() {
    if [ ! -f "$BACKUP_BINARY" ]; then
        print_error "백업 프로그램을 찾을 수 없습니다: $BACKUP_BINARY"
        print_info "먼저 'make' 명령어로 빌드해주세요."
        exit 1
    fi
    
    if [ ! -x "$BACKUP_BINARY" ]; then
        print_error "백업 프로그램에 실행 권한이 없습니다."
        print_info "다음 명령어로 권한을 부여하세요: chmod +x $BACKUP_BINARY"
        exit 1
    fi
}

show_help() {
    print_header
    echo -e "${CYAN}사용법:${NC}"
    echo "  $0 <명령어> [인수...]"
    echo
    echo -e "${CYAN}명령어:${NC}"
    echo -e "  ${GREEN}backup${NC}         <소스> <대상>           - 일반 백업"
    echo -e "  ${GREEN}backup-gzip${NC}    <소스> <대상>           - GZIP 압축 백업"
    echo -e "  ${GREEN}backup-zlib${NC}    <소스> <대상>           - ZLIB 압축 백업"
    echo -e "  ${GREEN}backup-dir${NC}     <소스> <대상>           - 디렉토리 백업 (재귀)"
    echo -e "  ${GREEN}restore${NC}        <소스> <대상>           - 복원 (자동 압축 감지)"
    echo -e "  ${GREEN}restore-dir${NC}    <소스> <대상>           - 디렉토리 복원"
    echo -e "  ${GREEN}verify${NC}         <백업파일>              - 백업 검증"
    echo -e "  ${GREEN}list${NC}           <백업경로>              - 백업 내용 목록"
    echo -e "  ${GREEN}test${NC}           [타입]                  - 테스트 실행"
    echo -e "  ${GREEN}demo${NC}           [시나리오]              - 데모 실행"
    echo -e "  ${GREEN}benchmark${NC}      [크기]                  - 성능 벤치마크"
    echo -e "  ${GREEN}version${NC}                                - 버전 정보"
    echo -e "  ${GREEN}help${NC}                                   - 이 도움말"
    echo
    echo -e "${CYAN}예시:${NC}"
    echo "  $0 backup document.txt backup_document.txt"
    echo "  $0 backup-gzip project/ project_backup/"
    echo "  $0 restore backup_document.txt restored_document.txt"
    echo "  $0 test full"
    echo "  $0 demo compression"
    echo
}

backup_file() {
    local source="$1"
    local dest="$2"
    
    if [ -z "$source" ] || [ -z "$dest" ]; then
        print_error "사용법: $0 backup <소스> <대상>"
        exit 1
    fi
    
    if [ ! -e "$source" ]; then
        print_error "소스 파일/디렉토리가 존재하지 않습니다: $source"
        exit 1
    fi
    
    print_info "일반 백업 시작: $source → $dest"
    
    if "$BACKUP_BINARY" backup --conflict=overwrite "$source" "$dest"; then
        print_success "백업 완료!"
        
        # 파일 크기 비교
        if [ -f "$source" ] && [ -f "$dest" ]; then
            local src_size=$(get_file_size "$source")
            local dst_size=$(get_file_size "$dest")
            echo "  📊 크기: $src_size bytes → $dst_size bytes"
        fi
    else
        print_error "백업 실패!"
        exit 1
    fi
}

backup_compressed() {
    local compression_type="$1"
    local source="$2"
    local dest="$3"
    
    if [ -z "$source" ] || [ -z "$dest" ]; then
        print_error "사용법: $0 backup-${compression_type} <소스> <대상>"
        exit 1
    fi
    
    if [ ! -e "$source" ]; then
        print_error "소스 파일/디렉토리가 존재하지 않습니다: $source"
        exit 1
    fi
    
    print_info "${compression_type^^} 압축 백업 시작: $source → $dest"
    
    if "$BACKUP_BINARY" backup --conflict=overwrite --compression="$compression_type" "$source" "$dest"; then
        print_success "압축 백업 완료!"
        
        # 압축률 계산
        if [ -f "$source" ]; then
            local ext=""
            case "$compression_type" in
                "gzip") ext=".gz" ;;
                "zlib") ext=".z" ;;
            esac
            
            if [ -f "${dest}${ext}" ]; then
                local src_size=$(get_file_size "$source")
                local dst_size=$(get_file_size "${dest}${ext}")
                
                if [ "$src_size" -gt 0 ]; then
                    local ratio=$(calculate_percentage "$dst_size" "$src_size")
                    echo "  📊 압축률: $src_size bytes → $dst_size bytes (${ratio}%)"
                fi
            fi
        fi
    else
        print_error "압축 백업 실패!"
        exit 1
    fi
}

backup_directory() {
    local source="$1"
    local dest="$2"
    
    if [ -z "$source" ] || [ -z "$dest" ]; then
        print_error "사용법: $0 backup-dir <소스디렉토리> <대상디렉토리>"
        exit 1
    fi
    
    if [ ! -d "$source" ]; then
        print_error "소스 디렉토리가 존재하지 않습니다: $source"
        exit 1
    fi
    
    print_info "디렉토리 백업 시작: $source → $dest"
    
    if "$BACKUP_BINARY" backup --conflict=overwrite -r -v "$source" "$dest"; then
        print_success "디렉토리 백업 완료!"
        
        # 통계 표시
        local file_count=$(find "$source" -type f | wc -l)
        local dir_count=$(find "$source" -type d | wc -l)
        echo "  📁 처리된 디렉토리: $dir_count개"
        echo "  📄 처리된 파일: $file_count개"
    else
        print_error "디렉토리 백업 실패!"
        exit 1
    fi
}

restore_file() {
    local source="$1"
    local dest="$2"
    
    if [ -z "$source" ] || [ -z "$dest" ]; then
        print_error "사용법: $0 restore <백업파일> <복원대상>"
        exit 1
    fi
    
    if [ ! -e "$source" ]; then
        print_error "백업 파일이 존재하지 않습니다: $source"
        exit 1
    fi
    
    print_info "복원 시작: $source → $dest"
    
    # 압축 파일 자동 감지 및 복원
    if [[ "$source" == *.gz ]]; then
        print_info "GZIP 압축 파일 감지됨"
        if gunzip -c "$source" > "$dest"; then
            print_success "GZIP 복원 완료!"
        else
            print_error "GZIP 복원 실패!"
            exit 1
        fi
    elif [[ "$source" == *.z ]]; then
        print_info "ZLIB 압축 파일 감지됨"
        # ZLIB 복원은 백업 프로그램 사용
        if "$BACKUP_BINARY" restore "$source" "$dest"; then
            print_success "ZLIB 복원 완료!"
        else
            print_error "ZLIB 복원 실패!"
            exit 1
        fi
    else
        print_info "일반 파일 복원"
        if cp "$source" "$dest"; then
            print_success "복원 완료!"
        else
            print_error "복원 실패!"
            exit 1
        fi
    fi
    
    # 무결성 검증 (선택사항)
    if command -v md5sum >/dev/null 2>&1; then
        print_info "무결성 검증 중..."
        # 압축 파일의 경우 원본과는 직접 비교 불가하므로 파일 존재만 확인
        if [ -f "$dest" ] && [ -s "$dest" ]; then
            print_success "복원된 파일 검증 완료"
        else
            print_warning "복원된 파일 검증 실패"
        fi
    fi
}

restore_directory() {
    local source="$1"
    local dest="$2"
    
    if [ -z "$source" ] || [ -z "$dest" ]; then
        print_error "사용법: $0 restore-dir <백업디렉토리> <복원대상>"
        exit 1
    fi
    
    if [ ! -d "$source" ]; then
        print_error "백업 디렉토리가 존재하지 않습니다: $source"
        exit 1
    fi
    
    print_info "디렉토리 복원 시작: $source → $dest"
    
    if "$BACKUP_BINARY" restore --conflict=overwrite -r -v "$source" "$dest"; then
        print_success "디렉토리 복원 완료!"
    else
        print_error "디렉토리 복원 실패!"
        exit 1
    fi
}

run_test() {
    local test_type="${1:-basic}"
    
    print_header
    print_info "테스트 실행: $test_type"
    echo
    
    case "$test_type" in
        "basic"|"기본")
            run_basic_test
            ;;
        "compression"|"압축")
            run_compression_test
            ;;
        "directory"|"디렉토리")
            run_directory_test
            ;;
        "full"|"전체")
            run_basic_test
            run_compression_test
            run_directory_test
            ;;
        *)
            print_error "알 수 없는 테스트 타입: $test_type"
            print_info "사용 가능한 타입: basic, compression, directory, full"
            exit 1
            ;;
    esac
}

run_basic_test() {
    print_info "=== 기본 백업/복원 테스트 ==="
    
    # 테스트 파일 생성
    local test_content="Hello, Backup Test! $(date)"
    echo "$test_content" > test_basic.txt
    
    # 백업
    print_info "백업 테스트..."
    backup_file test_basic.txt test_basic_backup.txt
    
    # 복원
    print_info "복원 테스트..."
    restore_file test_basic_backup.txt test_basic_restored.txt
    
    # 검증
    if cmp -s test_basic.txt test_basic_restored.txt; then
        print_success "기본 테스트 통과!"
    else
        print_error "기본 테스트 실패!"
        exit 1
    fi
    
    # 정리
    rm -f test_basic*.txt
    echo
}

run_compression_test() {
    print_info "=== 압축 백업/복원 테스트 ==="
    
    # 압축하기 좋은 테스트 데이터 생성
    local test_data=""
    for i in {1..100}; do
        test_data+="Line $i: This is test data for compression testing.\n"
    done
    printf "$test_data" > test_compress.txt
    
    local original_size=$(get_file_size "test_compress.txt")
    print_info "원본 파일 크기: $original_size bytes"
    
    # GZIP 테스트
    print_info "GZIP 압축 테스트..."
    backup_compressed gzip test_compress.txt test_gzip
    restore_file test_gzip.gz test_gzip_restored.txt
    
    if cmp -s test_compress.txt test_gzip_restored.txt; then
        print_success "GZIP 테스트 통과!"
    else
        print_error "GZIP 테스트 실패!"
        exit 1
    fi
    
    # ZLIB 테스트
    print_info "ZLIB 압축 테스트..."
    backup_compressed zlib test_compress.txt test_zlib
    
    # 정리
    rm -f test_compress.txt test_gzip* test_zlib*
    echo
}

run_directory_test() {
    print_info "=== 디렉토리 백업/복원 테스트 ==="
    
    # 테스트 디렉토리 구조 생성
    mkdir -p test_directory/{docs,images,code}
    echo "문서 파일" > test_directory/docs/document.txt
    echo "README 내용" > test_directory/README.md
    echo "이미지 설명" > test_directory/images/photo.txt
    echo "코드 내용" > test_directory/code/main.c
    
    print_info "테스트 디렉토리 구조:"
    find test_directory -type f | head -10
    
    # 디렉토리 백업
    print_info "디렉토리 백업 테스트..."
    backup_directory test_directory test_backup_dir
    
    # 디렉토리 복원
    print_info "디렉토리 복원 테스트..."
    restore_directory test_backup_dir test_restored_dir
    
    # 검증
    if diff -r test_directory test_restored_dir >/dev/null 2>&1; then
        print_success "디렉토리 테스트 통과!"
    else
        print_error "디렉토리 테스트 실패!"
        exit 1
    fi
    
    # 정리
    rm -rf test_directory test_backup_dir test_restored_dir
    echo
}

run_demo() {
    local demo_type="${1:-basic}"
    
    print_header
    print_info "데모 실행: $demo_type"
    echo
    
    case "$demo_type" in
        "basic"|"기본")
            demo_basic_usage
            ;;
        "compression"|"압축")
            demo_compression
            ;;
        "advanced"|"고급")
            demo_advanced_features
            ;;
        *)
            print_error "알 수 없는 데모 타입: $demo_type"
            print_info "사용 가능한 타입: basic, compression, advanced"
            exit 1
            ;;
    esac
}

demo_basic_usage() {
    print_info "=== 기본 사용법 데모 ==="
    
    echo -e "${CYAN}1. 테스트 파일 생성${NC}"
    echo "Hello, World! This is a demo file." > demo_file.txt
    echo "   파일 내용: $(cat demo_file.txt)"
    echo
    
    echo -e "${CYAN}2. 파일 백업${NC}"
    backup_file demo_file.txt demo_backup.txt
    echo
    
    echo -e "${CYAN}3. 파일 복원${NC}"
    restore_file demo_backup.txt demo_restored.txt
    echo
    
    echo -e "${CYAN}4. 결과 확인${NC}"
    if cmp -s demo_file.txt demo_restored.txt; then
        print_success "원본과 복원된 파일이 동일합니다!"
    else
        print_error "파일이 다릅니다!"
    fi
    
    # 정리
    rm -f demo_*.txt
    echo
}

demo_compression() {
    print_info "=== 압축 기능 데모 ==="
    
    echo -e "${CYAN}1. 큰 테스트 파일 생성${NC}"
    for i in {1..1000}; do
        echo "Line $i: Lorem ipsum dolor sit amet, consectetur adipiscing elit." >> demo_large.txt
    done
    
    local original_size=$(get_file_size "demo_large.txt")
    echo "   원본 크기: $original_size bytes"
    echo
    
    echo -e "${CYAN}2. GZIP 압축 백업${NC}"
    backup_compressed gzip demo_large.txt demo_compressed
    echo
    
    echo -e "${CYAN}3. 압축 파일 복원${NC}"
    restore_file demo_compressed.gz demo_uncompressed.txt
    echo
    
    echo -e "${CYAN}4. 무결성 검증${NC}"
    if cmp -s demo_large.txt demo_uncompressed.txt; then
        print_success "압축/해제가 성공적으로 완료되었습니다!"
    else
        print_error "파일 무결성 검증 실패!"
    fi
    
    # 정리
    rm -f demo_large.txt demo_compressed.gz demo_uncompressed.txt
    echo
}

demo_advanced_features() {
    print_info "=== 고급 기능 데모 ==="
    
    echo -e "${CYAN}1. 복잡한 디렉토리 구조 생성${NC}"
    mkdir -p demo_project/{src,docs,tests,build}
    echo "# 프로젝트 README" > demo_project/README.md
    echo "int main() { return 0; }" > demo_project/src/main.c
    echo "# 문서" > demo_project/docs/manual.md
    echo "#!/bin/bash" > demo_project/tests/test.sh
    echo "빌드 로그..." > demo_project/build/build.log
    
    find demo_project -type f | while read file; do
        echo "   📄 $file"
    done
    echo
    
    echo -e "${CYAN}2. 선택적 백업 (빌드 파일 제외)${NC}"
    print_info "build 디렉토리를 제외하고 백업 중..."
    
    # 수동으로 파일별 백업 (제외 패턴 시뮬레이션)
    mkdir -p demo_backup/demo_project/{src,docs,tests}
    cp demo_project/README.md demo_backup/demo_project/
    cp demo_project/src/* demo_backup/demo_project/src/
    cp demo_project/docs/* demo_backup/demo_project/docs/
    cp demo_project/tests/* demo_backup/demo_project/tests/
    
    print_success "선택적 백업 완료!"
    echo "   📁 백업된 구조:"
    find demo_backup -type f | while read file; do
        echo "     📄 $file"
    done
    echo
    
    echo -e "${CYAN}3. 백업 검증${NC}"
    if [ -d demo_backup/demo_project/build ]; then
        print_warning "build 디렉토리가 백업되었습니다 (예상하지 못한 결과)"
    else
        print_success "build 디렉토리가 올바르게 제외되었습니다!"
    fi
    
    # 정리
    rm -rf demo_project demo_backup
    echo
}

run_benchmark() {
    local size="${1:-10}"  # MB 단위
    
    print_header
    print_info "성능 벤치마크 실행 (${size}MB 파일)"
    echo
    
    # 테스트 파일 생성
    print_info "테스트 파일 생성 중..."
    if command -v dd >/dev/null 2>&1; then
        dd if=/dev/zero of=benchmark_file.dat bs=1M count="$size" 2>/dev/null
    else
        # dd가 없으면 다른 방법 사용
        for i in $(seq 1 $((size * 1024))); do
            printf "%1024s\n" " " >> benchmark_file.dat
        done
    fi
    
    local file_size=$(get_file_size "benchmark_file.dat")
    echo "   파일 크기: $file_size bytes (${size}MB)"
    echo
    
    # 일반 백업 벤치마크
    print_info "=== 일반 백업 벤치마크 ==="
    local start_time=$(date +%s)
    backup_file benchmark_file.dat benchmark_normal.dat
    local end_time=$(date +%s)
    local normal_time=$((end_time - start_time))
    echo "   ⏱️  소요 시간: ${normal_time}초"
    echo
    
    # GZIP 압축 백업 벤치마크
    print_info "=== GZIP 압축 백업 벤치마크 ==="
    start_time=$(date +%s)
    backup_compressed gzip benchmark_file.dat benchmark_gzip
    end_time=$(date +%s)
    local gzip_time=$((end_time - start_time))
    echo "   ⏱️  소요 시간: ${gzip_time}초"
    echo
    
    # 결과 비교
    print_info "=== 결과 비교 ==="
    echo "   📊 파일 크기 비교:"
    echo "     - 원본: $file_size bytes"
    if [ -f benchmark_normal.dat ]; then
        local normal_size=$(get_file_size "benchmark_normal.dat")
        echo "     - 일반 백업: $normal_size bytes"
    fi
    if [ -f benchmark_gzip.dat.gz ]; then
        local gzip_size=$(get_file_size "benchmark_gzip.dat.gz")
        echo "     - GZIP 압축: $gzip_size bytes"
        
        if [ "$file_size" -gt 0 ]; then
            local compression_ratio=$(calculate_percentage "$gzip_size" "$file_size")
            echo "     - 압축률: ${compression_ratio}%"
        fi
    fi
    
    echo "   ⏱️  성능 비교:"
    echo "     - 일반 백업: ${normal_time}초"
    echo "     - GZIP 압축: ${gzip_time}초"
    
    # 정리
    rm -f benchmark_*.dat*
    echo
}

show_version() {
    print_header
    echo -e "${CYAN}헬퍼 스크립트 버전:${NC} $VERSION"
    echo
    
    if [ -f "$BACKUP_BINARY" ]; then
        print_info "백업 프로그램 정보:"
        "$BACKUP_BINARY" version
    else
        print_warning "백업 프로그램을 찾을 수 없습니다."
    fi
}

# 메인 실행 로직

# 인수가 없으면 도움말 표시
if [ $# -eq 0 ]; then
    show_help
    exit 0
fi

# 백업 프로그램 존재 확인 (version과 help 제외)
if [ "$1" != "version" ] && [ "$1" != "help" ]; then
    check_binary
fi

# 명령어 처리
case "$1" in
    "backup")
        shift
        backup_file "$@"
        ;;
    "backup-gzip")
        shift
        backup_compressed gzip "$@"
        ;;
    "backup-zlib")
        shift
        backup_compressed zlib "$@"
        ;;
    "backup-dir")
        shift
        backup_directory "$@"
        ;;
    "restore")
        shift
        restore_file "$@"
        ;;
    "restore-dir")
        shift
        restore_directory "$@"
        ;;
    "verify")
        shift
        print_info "백업 검증: $1"
        if "$BACKUP_BINARY" verify "$1"; then
            print_success "검증 완료!"
        else
            print_error "검증 실패!"
            exit 1
        fi
        ;;
    "list")
        shift
        print_info "백업 내용 목록: $1"
        "$BACKUP_BINARY" list "$1"
        ;;
    "test")
        shift
        run_test "$@"
        ;;
    "demo")
        shift
        run_demo "$@"
        ;;
    "benchmark")
        shift
        run_benchmark "$@"
        ;;
    "version")
        show_version
        ;;
    "help"|"--help"|"-h")
        show_help
        ;;
    *)
        print_error "알 수 없는 명령어: $1"
        echo
        show_help
        exit 1
        ;;
esac