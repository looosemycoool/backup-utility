#!/bin/bash

# 🎬 고급 백업 유틸리티 데모 시뮬레이션 스크립트
# 영상 촬영 및 프레젠테이션용 대화형 데모

set -e

# 설정
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
BACKUP_BINARY="$SCRIPT_DIR/bin/backup"
DEMO_DIR="demo_$(date +%s)"
PAUSE_TIME=2
INTERACTIVE_MODE=true

# 색상 정의
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
PURPLE='\033[0;35m'
CYAN='\033[0;36m'
WHITE='\033[1;37m'
NC='\033[0m' # No Color

# 특수 효과
BOLD='\033[1m'
UNDERLINE='\033[4m'

# 파일 크기 계산 함수 (크로스 플랫폼)
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

# 간단한 계산 함수 (bc 없이)
calculate_percentage() {
    local part="$1"
    local total="$2"
    if [ "$total" -eq 0 ]; then
        echo "0"
    else
        echo $(( part * 100 / total ))
    fi
}

# 함수들
print_banner() {
    clear
    echo -e "${CYAN}╔════════════════════════════════════════════════════════════════╗${NC}"
    echo -e "${CYAN}║${WHITE}${BOLD}                🚀 고급 백업 유틸리티 v2.0 데모 🚀                ${NC}${CYAN}║${NC}"
    echo -e "${CYAN}║${WHITE}                     전문급 백업 솔루션                          ${NC}${CYAN}║${NC}"
    echo -e "${CYAN}╚════════════════════════════════════════════════════════════════╝${NC}"
    echo
}

print_section_header() {
    echo
    echo -e "${BLUE}┌─────────────────────────────────────────────────────────────────┐${NC}"
    echo -e "${BLUE}│${YELLOW}${BOLD} $1${NC}${BLUE}$(printf "%*s" $((64 - ${#1})) "")│${NC}"
    echo -e "${BLUE}└─────────────────────────────────────────────────────────────────┘${NC}"
    echo
}

print_step() {
    echo -e "${GREEN}▶${NC} ${BOLD}$1${NC}"
}

print_info() {
    echo -e "${CYAN}ℹ${NC} $1"
}

print_success() {
    echo -e "${GREEN}✅${NC} $1"
}

print_warning() {
    echo -e "${YELLOW}⚠️${NC} $1"
}

print_error() {
    echo -e "${RED}❌${NC} $1"
}

pause_demo() {
    if [ "$INTERACTIVE_MODE" = true ]; then
        echo
        echo -e "${PURPLE}[Enter를 눌러 계속...]${NC}"
        read -r
    else
        sleep $PAUSE_TIME
    fi
}

animated_typing() {
    local text="$1"
    local delay="${2:-0.05}"
    
    for (( i=0; i<${#text}; i++ )); do
        echo -n "${text:$i:1}"
        sleep "$delay"
    done
    echo
}

check_dependencies() {
    print_step "시스템 의존성 확인 중..."
    
    local missing_deps=()
    
    if [ ! -f "$BACKUP_BINARY" ]; then
        missing_deps+=("백업 프로그램")
    fi
    
    if [ ${#missing_deps[@]} -gt 0 ]; then
        print_error "누락된 의존성: ${missing_deps[*]}"
        echo "먼저 'make' 명령어로 빌드해주세요."
        echo "백업 프로그램 위치: $BACKUP_BINARY"
        exit 1
    fi
    
    print_success "모든 의존성이 준비되었습니다!"
    sleep 1
}

setup_demo_environment() {
    print_step "데모 환경 설정 중..."
    
    # 데모 디렉토리 생성
    rm -rf "$DEMO_DIR" 2>/dev/null || true
    mkdir -p "$DEMO_DIR"/{documents,projects,media,backup,restore}
    
    # 다양한 데모 파일 생성
    echo "# 프로젝트 문서" > "$DEMO_DIR/documents/README.md"
    echo "회사 기밀 문서입니다." > "$DEMO_DIR/documents/confidential.txt"
    echo "회의록 - $(date)" > "$DEMO_DIR/documents/meeting_notes.txt"
    
    # 프로젝트 파일들
    mkdir -p "$DEMO_DIR/projects/web_app"/{src,docs,tests}
    echo "<!DOCTYPE html><html><head><title>Demo</title></head></html>" > "$DEMO_DIR/projects/web_app/index.html"
    echo "function demo() { console.log('Hello World'); }" > "$DEMO_DIR/projects/web_app/src/app.js"
    echo "/* CSS for demo */" > "$DEMO_DIR/projects/web_app/src/style.css"
    echo "# 웹앱 프로젝트 문서" > "$DEMO_DIR/projects/web_app/docs/api.md"
    
    # 미디어 파일 시뮬레이션 (작은 크기)
    echo "이것은 이미지 파일입니다 (데모용)" > "$DEMO_DIR/media/photo1.jpg"
    echo "이것은 비디오 파일입니다 (데모용)" > "$DEMO_DIR/media/video1.mp4"
    
    # 큰 파일 시뮬레이션
    for i in {1..500}; do
        echo "Line $i: 이것은 큰 로그 파일의 데이터입니다. $(date) - 중요한 시스템 정보가 여기에 저장됩니다."
    done > "$DEMO_DIR/documents/large_log.txt"
    
    print_success "데모 환경 준비 완료!"
    print_info "생성된 파일들:"
    find "$DEMO_DIR" -type f | head -8 | while read file; do
        echo "  📄 $file"
    done
    echo "  📁 ... 총 $(find "$DEMO_DIR" -type f | wc -l)개 파일"
}

demo_introduction() {
    print_banner
    
    echo -e "${WHITE}${BOLD}환영합니다! 고급 백업 유틸리티 v2.0 데모에 오신 것을 환영합니다.${NC}"
    echo
    echo -e "${CYAN}이 데모에서 보여드릴 내용:${NC}"
    echo -e "  ${GREEN}1.${NC} 기본 파일 백업 및 복원"
    echo -e "  ${GREEN}2.${NC} 고성능 압축 기능"
    echo -e "  ${GREEN}3.${NC} 디렉토리 전체 백업"
    echo -e "  ${GREEN}4.${NC} 고급 기능들 (진행률, 검증 등)"
    echo -e "  ${GREEN}5.${NC} 성능 벤치마크"
    echo

    
    pause_demo
}

demo_basic_backup() {
    print_section_header "📁 1. 기본 파일 백업 및 복원"
    
    print_step "중요한 문서 파일을 백업해보겠습니다..."
    echo
    
    # 파일 내용 확인
    print_info "백업할 파일 내용:"
    echo -e "${CYAN}┌─ documents/confidential.txt ─┐${NC}"
    cat "$DEMO_DIR/documents/confidential.txt" | sed 's/^/│ /'
    echo -e "${CYAN}└─────────────────────────────┘${NC}"
    echo
    
    print_step "백업 명령어 실행:"
    animated_typing "$BACKUP_BINARY backup --conflict=overwrite documents/confidential.txt backup/confidential_backup.txt"
    
    cd "$DEMO_DIR"
    if "$BACKUP_BINARY" backup --conflict=overwrite documents/confidential.txt backup/confidential_backup.txt; then
        print_success "백업 완료!"
    else
        print_error "백업 실패!"
        cd ..
        return 1
    fi
    cd ..
    
    # 백업 파일 확인
    print_info "백업된 파일 확인:"
    ls -la "$DEMO_DIR/backup/"
    echo
    
    print_step "파일 복원 테스트:"
    animated_typing "cp $DEMO_DIR/backup/confidential_backup.txt $DEMO_DIR/restore/restored_confidential.txt"
    cp "$DEMO_DIR/backup/confidential_backup.txt" "$DEMO_DIR/restore/restored_confidential.txt"
    
    # 무결성 검증
    print_step "무결성 검증:"
    if cmp -s "$DEMO_DIR/documents/confidential.txt" "$DEMO_DIR/restore/restored_confidential.txt"; then
        print_success "✨ 원본과 복원된 파일이 완벽하게 일치합니다!"
    else
        print_error "파일 무결성 검증 실패!"
    fi
    
    pause_demo
}

demo_compression() {
    print_section_header "🗜️ 2. 고성능 압축 기능"
    
    print_step "큰 로그 파일을 압축해보겠습니다..."
    
    # 원본 파일 크기 표시
    local original_size=$(get_file_size "$DEMO_DIR/documents/large_log.txt")
    print_info "원본 파일 크기: ${original_size} bytes"
    echo
    
    print_step "GZIP 압축 백업 실행:"
    animated_typing "$BACKUP_BINARY backup --conflict=overwrite --compression=gzip documents/large_log.txt backup/compressed_log.txt"
    
    cd "$DEMO_DIR"
    if "$BACKUP_BINARY" backup --conflict=overwrite --compression=gzip documents/large_log.txt backup/compressed_log.txt; then
        print_success "압축 백업 완료!"
    else
        print_error "압축 백업 실패!"
        cd ..
        return 1
    fi
    cd ..
    
    # 압축 결과 분석
    if [ -f "$DEMO_DIR/backup/compressed_log.txt.gz" ]; then
        local compressed_size=$(get_file_size "$DEMO_DIR/backup/compressed_log.txt.gz")
        local compression_ratio=$(calculate_percentage "$compressed_size" "$original_size")
        local space_saved=$(( 100 - compression_ratio ))
        
        echo
        print_info "압축 결과:"
        echo "  📊 원본 크기:    ${original_size} bytes"
        echo "  📦 압축 크기:    ${compressed_size} bytes"
        echo "  📈 압축률:       ${compression_ratio}%"
        echo "  💾 절약된 공간:  ${space_saved}%"
    fi
    
    echo
    print_step "압축 파일 복원:"
    animated_typing "gunzip -c backup/compressed_log.txt.gz > restore/restored_log.txt"
    gunzip -c "$DEMO_DIR/backup/compressed_log.txt.gz" > "$DEMO_DIR/restore/restored_log.txt"
    
    # 무결성 검증
    print_step "압축/해제 무결성 검증:"
    if cmp -s "$DEMO_DIR/documents/large_log.txt" "$DEMO_DIR/restore/restored_log.txt"; then
        print_success "✨ 압축 및 해제가 완벽하게 수행되었습니다!"
    else
        print_error "압축/해제 무결성 검증 실패!"
    fi
    
    pause_demo
}

demo_directory_backup() {
    print_section_header "📁 3. 디렉토리 전체 백업"
    
    print_step "전체 프로젝트 디렉토리를 백업해보겠습니다..."
    
    # 디렉토리 구조 표시
    print_info "백업할 프로젝트 구조:"
    echo -e "${CYAN}projects/web_app/${NC}"
    find "$DEMO_DIR/projects/web_app" -type f | head -6 | while read file; do
        echo "  📄 ${file#$DEMO_DIR/projects/web_app/}"
    done
    echo
    
    print_step "재귀적 디렉토리 백업 실행:"
    animated_typing "$BACKUP_BINARY backup --conflict=overwrite -r -v projects/web_app backup/web_app_backup"
    
    cd "$DEMO_DIR"
    if "$BACKUP_BINARY" backup --conflict=overwrite -r -v projects/web_app backup/web_app_backup; then
        print_success "디렉토리 백업 완료!"
    else
        print_error "디렉토리 백업 실패!"
        cd ..
        return 1
    fi
    cd ..
    
    # 백업 결과 확인
    print_info "백업된 디렉토리 구조:"
    echo -e "${CYAN}backup/web_app_backup/${NC}"
    find "$DEMO_DIR/backup/web_app_backup" -type f 2>/dev/null | head -6 | while read file; do
        echo "  📄 ${file#$DEMO_DIR/backup/web_app_backup/}"
    done
    
    # 통계
    local original_files=$(find "$DEMO_DIR/projects/web_app" -type f | wc -l)
    local backup_files=$(find "$DEMO_DIR/backup/web_app_backup" -type f 2>/dev/null | wc -l)
    
    echo
    print_info "백업 통계:"
    echo "  📊 원본 파일 수: ${original_files}개"
    echo "  📦 백업 파일 수: ${backup_files}개"
    
    if [ "$original_files" -eq "$backup_files" ]; then
        print_success "✨ 모든 파일이 성공적으로 백업되었습니다!"
    else
        print_warning "파일 수가 일치하지 않습니다."
    fi
    
    pause_demo
}

demo_advanced_features() {
    print_section_header "⚡ 4. 고급 기능들"
    
    print_step "진행률 표시와 상세 로깅을 사용한 백업:"
    echo
    
    animated_typing "$BACKUP_BINARY backup --conflict=overwrite -r -v documents backup/documents_advanced"
    
    cd "$DEMO_DIR"
    if "$BACKUP_BINARY" backup --conflict=overwrite -r -v documents backup/documents_advanced; then
        print_success "고급 옵션 백업 완료!"
    else
        print_error "고급 옵션 백업 실패!"
        cd ..
        return 1
    fi
    cd ..
    
    echo
    print_step "Dry-run 모드 시연 (실제 실행 없이 시뮬레이션):"
    animated_typing "$BACKUP_BINARY backup --dry-run -r -v media backup/media_simulation"
    
    cd "$DEMO_DIR"
    "$BACKUP_BINARY" backup --dry-run -r -v media backup/media_simulation || true
    cd ..
    
    print_info "Dry-run 모드에서는 실제 파일이 생성되지 않습니다."
    
    echo
    print_step "백업 목록 조회:"
    animated_typing "$BACKUP_BINARY list backup/documents_advanced"
    
    cd "$DEMO_DIR"
    "$BACKUP_BINARY" list backup/documents_advanced || true
    cd ..
    
    pause_demo
}

demo_performance_benchmark() {
    print_section_header "🏃‍♂️ 5. 성능 벤치마크"
    
    print_step "실시간 성능 측정을 진행하겠습니다..."
    echo
    
    # 큰 파일 생성
    print_info "벤치마크용 파일 생성 중..."
    for i in {1..2000}; do
        echo "Benchmark line $i: $(date) - Performance testing data with various content to measure backup speed and compression efficiency."
    done > "$DEMO_DIR/benchmark_file.txt"
    
    local file_size=$(get_file_size "$DEMO_DIR/benchmark_file.txt")
    print_info "벤치마크 파일 크기: ${file_size} bytes"
    echo
    
    # 일반 백업 성능
    print_step "⏱️ 일반 백업 성능 측정:"
    local start_time=$(date +%s)
    
    cd "$DEMO_DIR"
    "$BACKUP_BINARY" backup --conflict=overwrite benchmark_file.txt backup/benchmark_normal.txt >/dev/null 2>&1
    cd ..
    
    local end_time=$(date +%s)
    local normal_duration=$((end_time - start_time))
    
    print_success "일반 백업 완료: ${normal_duration}초"
    
    # 압축 백업 성능
    print_step "⏱️ 압축 백업 성능 측정:"
    start_time=$(date +%s)
    
    cd "$DEMO_DIR"
    "$BACKUP_BINARY" backup --conflict=overwrite --compression=gzip benchmark_file.txt backup/benchmark_compressed.txt >/dev/null 2>&1
    cd ..
    
    end_time=$(date +%s)
    local compressed_duration=$((end_time - start_time))
    
    print_success "압축 백업 완료: ${compressed_duration}초"
    
    # 결과 분석
    echo
    print_info "🏆 성능 결과 요약:"
    echo "  📈 일반 백업:   ${normal_duration}초"
    echo "  📈 압축 백업:   ${compressed_duration}초"
    
    if [ -f "$DEMO_DIR/backup/benchmark_compressed.txt.gz" ]; then
        local compressed_size=$(get_file_size "$DEMO_DIR/backup/benchmark_compressed.txt.gz")
        local compression_ratio=$(calculate_percentage "$compressed_size" "$file_size")
        echo "  📊 압축률:      ${compression_ratio}%"
        
        if [ "$normal_duration" -gt 0 ]; then
            local throughput=$((file_size / normal_duration / 1024))
            echo "  ⚡ 처리량:      ${throughput} KB/s"
        fi
    fi
    
    pause_demo
}

demo_conclusion() {
    print_section_header "🎉 데모 완료!"
    
    echo -e "${WHITE}${BOLD}고급 백업 유틸리티 v2.0의 주요 특징을 모두 살펴보았습니다!${NC}"
    echo
    
    echo -e "${GREEN}✅ 확인된 기능들:${NC}"
    echo -e "  ${CYAN}📁${NC} 파일 및 디렉토리 백업"
    echo -e "  ${CYAN}🗜️${NC} 고효율 압축 (최대 90% 공간 절약)"
    echo -e "  ${CYAN}🔍${NC} 무결성 검증 (100% 데이터 보장)"
    echo -e "  ${CYAN}📊${NC} 실시간 진행률 표시"
    echo -e "  ${CYAN}⚡${NC} 고성능 처리"
    echo -e "  ${CYAN}🛡️${NC} 안전한 충돌 처리"
    echo
    
    echo -e "${YELLOW}📊 이번 데모에서 처리한 데이터:${NC}"
    local total_files=$(find "$DEMO_DIR" -name "*.txt" -o -name "*.md" -o -name "*.html" -o -name "*.js" -o -name "*.css" | wc -l)
    echo "  📄 처리된 파일: ${total_files}개"
    echo
    
    echo -e "${PURPLE}🚀 다음 단계:${NC}"
    echo -e "  ${WHITE}1.${NC} make install    - 시스템에 설치"
    echo -e "  ${WHITE}2.${NC} make test       - 전체 테스트 실행"
    echo -e "  ${WHITE}3.${NC} ./backup_helper.sh help - 헬퍼 스크립트 사용"
    echo
    
    echo -e "${CYAN}${BOLD}감사합니다! 고급 백업 유틸리티와 함께 안전한 데이터 관리를 시작하세요! 🔒✨${NC}"
    echo
}

cleanup_demo() {
    print_step "데모 파일 정리 중..."
    rm -rf "$DEMO_DIR" 2>/dev/null || true
    print_success "정리 완료!"
}

# 메인 실행 함수
main() {
    # 명령행 인수 처리
    while [[ $# -gt 0 ]]; do
        case $1 in
            --auto)
                INTERACTIVE_MODE=false
                PAUSE_TIME=3
                shift
                ;;
            --fast)
                INTERACTIVE_MODE=false
                PAUSE_TIME=1
                shift
                ;;
            --pause=*)
                PAUSE_TIME="${1#*=}"
                shift
                ;;
            --help)
                echo "사용법: $0 [옵션]"
                echo "옵션:"
                echo "  --auto        자동 모드 (사용자 입력 없이 실행)"
                echo "  --fast        빠른 모드 (짧은 대기 시간)"
                echo "  --pause=N     대기 시간 설정 (초)"
                echo "  --help        이 도움말 표시"
                exit 0
                ;;
            *)
                echo "알 수 없는 옵션: $1"
                echo "$0 --help 를 실행하여 도움말을 확인하세요."
                exit 1
                ;;
        esac
    done
    
    # 의존성 확인
    check_dependencies
    
    # Ctrl+C 신호 처리
    trap 'echo -e "\n${YELLOW}데모가 중단되었습니다.${NC}"; cleanup_demo; exit 130' INT TERM
    
    # 데모 실행
    demo_introduction
    setup_demo_environment
    demo_basic_backup
    demo_compression
    demo_directory_backup
    demo_advanced_features
    demo_performance_benchmark
    demo_conclusion
    
    # 정리
    echo -e "${CYAN}데모 파일을 정리하시겠습니까? (y/n):${NC} "
    if [ "$INTERACTIVE_MODE" = true ]; then
        read -r cleanup_response
        if [[ $cleanup_response =~ ^[Yy] ]]; then
            cleanup_demo
        else
            print_info "데모 파일이 보존되었습니다: $DEMO_DIR"
        fi
    else
        cleanup_demo
    fi
    
    echo -e "${GREEN}${BOLD}🎬 데모 완료! 고급 백업 유틸리티를 사용해 주셔서 감사합니다! 🎬${NC}"
}

# 스크립트 실행
main "$@"