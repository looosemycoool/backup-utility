#!/bin/bash

# 🚀 고급 백업 유틸리티 자동 설치 스크립트
# 의존성 확인부터 설치까지 완전 자동화

set -e  # 에러 발생 시 즉시 종료

# 설정
PROGRAM_NAME="고급 백업 유틸리티"
VERSION="2.0"
INSTALL_PREFIX="/usr/local"
BACKUP_BINARY="bin/backup"
CONFIG_DIR="$HOME/.backup-utility"

# 색상 정의
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
PURPLE='\033[0;35m'
CYAN='\033[0;36m'
WHITE='\033[1;37m'
NC='\033[0m' # No Color

BOLD='\033[1m'

# 시스템 정보
OS_TYPE=""
DISTRO=""
PACKAGE_MANAGER=""

# 함수들
print_banner() {
    clear
    echo -e "${CYAN}╔══════════════════════════════════════════════════════════════════╗${NC}"
    echo -e "${CYAN}║${WHITE}${BOLD}                   🚀 백업 유틸리티 설치기 🚀                     ${NC}${CYAN}║${NC}"
    echo -e "${CYAN}║${WHITE}                        버전 ${VERSION} 자동 설치                        ${NC}${CYAN}║${NC}"
    echo -e "${CYAN}╚══════════════════════════════════════════════════════════════════╝${NC}"
    echo
}

print_step() {
    echo -e "${GREEN}▶${NC} ${BOLD}$1${NC}"
}

print_info() {
    echo -e "${BLUE}ℹ${NC} $1"
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

print_progress() {
    local current=$1
    local total=$2
    local desc=$3
    local percent=$((current * 100 / total))
    local progress_bar=""
    
    for ((i=1; i<=50; i++)); do
        if [ $i -le $((percent/2)) ]; then
            progress_bar+="█"
        else
            progress_bar+="░"
        fi
    done
    
    echo -ne "\r${CYAN}[$progress_bar] ${percent}%${NC} $desc"
    
    if [ "$current" -eq "$total" ]; then
        echo
    fi
}

detect_system() {
    print_step "시스템 환경 감지 중..."
    
    # OS 타입 감지
    if [[ "$OSTYPE" == "linux-gnu"* ]]; then
        OS_TYPE="linux"
    elif [[ "$OSTYPE" == "darwin"* ]]; then
        OS_TYPE="macos"
    elif [[ "$OSTYPE" == "freebsd"* ]]; then
        OS_TYPE="freebsd"
    else
        print_error "지원되지 않는 운영 체제: $OSTYPE"
        exit 1
    fi
    
    # Linux 배포판 감지
    if [ "$OS_TYPE" = "linux" ]; then
        if [ -f /etc/os-release ]; then
            . /etc/os-release
            DISTRO=$ID
        elif [ -f /etc/redhat-release ]; then
            DISTRO="rhel"
        elif [ -f /etc/debian_version ]; then
            DISTRO="debian"
        else
            DISTRO="unknown"
        fi
        
        # 패키지 매니저 감지
        if command -v apt-get >/dev/null 2>&1; then
            PACKAGE_MANAGER="apt"
        elif command -v yum >/dev/null 2>&1; then
            PACKAGE_MANAGER="yum"
        elif command -v dnf >/dev/null 2>&1; then
            PACKAGE_MANAGER="dnf"
        elif command -v zypper >/dev/null 2>&1; then
            PACKAGE_MANAGER="zypper"
        elif command -v pacman >/dev/null 2>&1; then
            PACKAGE_MANAGER="pacman"
        else
            PACKAGE_MANAGER="unknown"
        fi
    elif [ "$OS_TYPE" = "macos" ]; then
        if command -v brew >/dev/null 2>&1; then
            PACKAGE_MANAGER="brew"
        else
            PACKAGE_MANAGER="none"
        fi
    fi
    
    print_success "시스템 감지 완료"
    print_info "OS: $OS_TYPE"
    print_info "배포판: $DISTRO"
    print_info "패키지 매니저: $PACKAGE_MANAGER"
    echo
}

check_root_privileges() {
    print_step "권한 확인 중..."
    
    if [ "$EUID" -eq 0 ]; then
        print_warning "root 권한으로 실행 중입니다."
        print_info "일반 사용자 권한으로 실행하는 것을 권장합니다."
        echo
        echo -e "${YELLOW}계속하시겠습니까? (y/N):${NC} "
        read -r continue_as_root
        if [[ ! $continue_as_root =~ ^[Yy] ]]; then
            print_info "설치가 취소되었습니다."
            exit 0
        fi
    else
        print_success "일반 사용자 권한으로 실행 중"
    fi
    echo
}

install_dependencies() {
    print_step "의존성 설치 중..."
    
    local deps_needed=()
    local build_deps=("build-essential" "gcc" "make" "libc6-dev")
    local lib_deps=("zlib1g-dev" "libz-dev")
    
    # 필수 도구 확인
    if ! command -v gcc >/dev/null 2>&1; then
        deps_needed+=("gcc")
    fi
    
    if ! command -v make >/dev/null 2>&1; then
        deps_needed+=("make")
    fi
    
    # zlib 개발 라이브러리 확인
    if ! ldconfig -p | grep -q libz.so; then
        deps_needed+=("zlib-dev")
    fi
    
    if [ ${#deps_needed[@]} -eq 0 ]; then
        print_success "모든 의존성이 이미 설치되어 있습니다!"
        return 0
    fi
    
    print_info "설치가 필요한 패키지: ${deps_needed[*]}"
    
    case "$PACKAGE_MANAGER" in
        "apt")
            print_info "APT 패키지 목록 업데이트 중..."
            sudo apt-get update -qq
            
            print_info "의존성 설치 중..."
            sudo apt-get install -y build-essential libz-dev
            ;;
        "yum")
            print_info "YUM으로 의존성 설치 중..."
            sudo yum groupinstall -y "Development Tools"
            sudo yum install -y zlib-devel
            ;;
        "dnf")
            print_info "DNF로 의존성 설치 중..."
            sudo dnf groupinstall -y "Development Tools"
            sudo dnf install -y zlib-devel
            ;;
        "zypper")
            print_info "Zypper로 의존성 설치 중..."
            sudo zypper install -y gcc make zlib-devel
            ;;
        "pacman")
            print_info "Pacman으로 의존성 설치 중..."
            sudo pacman -S --noconfirm base-devel zlib
            ;;
        "brew")
            print_info "Homebrew로 의존성 설치 중..."
            brew install gcc make zlib
            ;;
        *)
            print_warning "알 수 없는 패키지 매니저입니다."
            print_info "다음 패키지들을 수동으로 설치해주세요:"
            echo "  - GCC 컴파일러"
            echo "  - Make"
            echo "  - zlib 개발 라이브러리"
            echo
            echo -e "${YELLOW}의존성을 수동으로 설치했습니까? (y/N):${NC} "
            read -r manual_install
            if [[ ! $manual_install =~ ^[Yy] ]]; then
                print_error "의존성 설치가 필요합니다."
                exit 1
            fi
            ;;
    esac
    
    print_success "의존성 설치 완료!"
    echo
}

verify_dependencies() {
    print_step "의존성 검증 중..."
    
    local verification_failed=false
    
    # GCC 확인
    if command -v gcc >/dev/null 2>&1; then
        local gcc_version=$(gcc --version | head -n1)
        print_success "GCC: $gcc_version"
    else
        print_error "GCC를 찾을 수 없습니다"
        verification_failed=true
    fi
    
    # Make 확인
    if command -v make >/dev/null 2>&1; then
        local make_version=$(make --version | head -n1)
        print_success "Make: $make_version"
    else
        print_error "Make를 찾을 수 없습니다"
        verification_failed=true
    fi
    
    # zlib 확인
    if ldconfig -p 2>/dev/null | grep -q libz.so || [ -f /usr/lib/libz.a ] || [ -f /usr/local/lib/libz.a ]; then
        print_success "zlib: 설치됨"
    else
        print_error "zlib 개발 라이브러리를 찾을 수 없습니다"
        verification_failed=true
    fi
    
    if [ "$verification_failed" = true ]; then
        print_error "의존성 검증에 실패했습니다."
        exit 1
    fi
    
    print_success "모든 의존성이 확인되었습니다!"
    echo
}

build_program() {
    print_step "프로그램 빌드 중..."
    
    # 기존 빌드 정리
    print_info "이전 빌드 결과 정리 중..."
    make clean >/dev/null 2>&1 || true
    
    # 빌드 실행
    print_info "컴파일 시작..."
    
    # 진행률 표시를 위한 임시 파일
    local build_log=$(mktemp)
    
    # 백그라운드에서 빌드 실행
    make release > "$build_log" 2>&1 &
    local build_pid=$!
    
    # 진행률 시뮬레이션
    local progress=0
    while kill -0 $build_pid 2>/dev/null; do
        progress=$((progress + 1))
        if [ $progress -gt 100 ]; then progress=100; fi
        print_progress $progress 100 "컴파일 중..."
        sleep 0.1
    done
    
    # 빌드 완료 대기
    wait $build_pid
    local build_result=$?
    
    print_progress 100 100 "컴파일 완료"
    echo
    
    if [ $build_result -eq 0 ]; then
        print_success "빌드 완료!"
    else
        print_error "빌드 실패!"
        echo "빌드 로그:"
        cat "$build_log"
        rm -f "$build_log"
        exit 1
    fi
    
    rm -f "$build_log"
    
    # 빌드 결과 확인
    if [ -f "$BACKUP_BINARY" ]; then
        print_success "실행 파일 생성 확인: $BACKUP_BINARY"
        local file_size=$(stat -f%z "$BACKUP_BINARY" 2>/dev/null || stat -c%s "$BACKUP_BINARY")
        print_info "파일 크기: $file_size bytes"
    else
        print_error "실행 파일이 생성되지 않았습니다"
        exit 1
    fi
    
    echo
}

test_build() {
    print_step "빌드 테스트 중..."
    
    # 기본 실행 테스트
    print_info "프로그램 실행 테스트..."
    if ./"$BACKUP_BINARY" version >/dev/null 2>&1; then
        print_success "프로그램 실행 성공"
    else
        print_error "프로그램 실행 실패"
        exit 1
    fi
    
    # 간단한 기능 테스트
    print_info "기본 기능 테스트..."
    echo "테스트 데이터" > test_install.txt
    
    if ./"$BACKUP_BINARY" backup --conflict=overwrite test_install.txt test_backup.txt >/dev/null 2>&1; then
        if [ -f test_backup.txt ]; then
            print_success "백업 기능 테스트 성공"
            rm -f test_install.txt test_backup.txt
        else
            print_error "백업 파일이 생성되지 않음"
            rm -f test_install.txt test_backup.txt
            exit 1
        fi
    else
        print_error "백업 기능 테스트 실패"
        rm -f test_install.txt test_backup.txt
        exit 1
    fi
    
    print_success "모든 테스트 통과!"
    echo
}

install_to_system() {
    print_step "시스템 설치 중..."
    
    local install_type=""
    
    if [ "$EUID" -eq 0 ]; then
        install_type="system"
    else
        echo -e "${YELLOW}설치 위치를 선택하세요:${NC}"
        echo "  1) 시스템 전체 설치 (/usr/local/bin) - sudo 필요"
        echo "  2) 사용자 설치 (~/.local/bin)"
        echo "  3) 현재 디렉토리에서만 사용"
        echo
        echo -e "${CYAN}선택 (1-3, 기본값: 2):${NC} "
        read -r install_choice
        
        case "$install_choice" in
            1)
                install_type="system"
                ;;
            3)
                install_type="local"
                ;;
            *)
                install_type="user"
                ;;
        esac
    fi
    
    case "$install_type" in
        "system")
            print_info "시스템 전체 설치 중..."
            sudo cp "$BACKUP_BINARY" "$INSTALL_PREFIX/bin/"
            sudo chmod 755 "$INSTALL_PREFIX/bin/backup"
            print_success "시스템 설치 완료: $INSTALL_PREFIX/bin/backup"
            ;;
        "user")
            print_info "사용자 설치 중..."
            mkdir -p "$HOME/.local/bin"
            cp "$BACKUP_BINARY" "$HOME/.local/bin/"
            chmod 755 "$HOME/.local/bin/backup"
            print_success "사용자 설치 완료: $HOME/.local/bin/backup"
            
            # PATH 확인 및 안내
            if [[ ":$PATH:" != *":$HOME/.local/bin:"* ]]; then
                print_warning "$HOME/.local/bin이 PATH에 없습니다."
                print_info "다음 명령어를 실행하여 PATH에 추가하세요:"
                echo "  echo 'export PATH=\"\$HOME/.local/bin:\$PATH\"' >> ~/.bashrc"
                echo "  source ~/.bashrc"
            fi
            ;;
        "local")
            print_info "현재 디렉토리에서 사용합니다."
            print_success "설치 없음 - ./bin/backup으로 사용하세요"
            ;;
    esac
    
    echo
}

create_config_directory() {
    print_step "설정 디렉토리 생성 중..."
    
    mkdir -p "$CONFIG_DIR"
    
    # 기본 설정 파일 생성
    cat > "$CONFIG_DIR/config.conf" << EOF
# 고급 백업 유틸리티 설정 파일
# 이 파일을 편집하여 기본 동작을 사용자 정의할 수 있습니다.

# 기본 압축 타입 (none, gzip, zlib)
default_compression=none

# 기본 충돌 처리 (ask, overwrite, skip, rename)
default_conflict=ask

# 기본 스레드 수 (0 = 자동)
default_threads=0

# 기본 로그 레벨 (error, warning, info, debug)
default_log_level=info

# 백업 기록 보관 일수
backup_history_days=30

# 자동 검증 활성화 (true/false)
auto_verify=false
EOF
    
    # 예제 스크립트 생성
    cat > "$CONFIG_DIR/backup_scripts/daily_backup.sh" << 'EOF'
#!/bin/bash
# 일일 백업 스크립트 예제

BACKUP_DIR="$HOME/Backups/$(date +%Y%m%d)"
mkdir -p "$BACKUP_DIR"

# 문서 백업
backup backup -r -c gzip "$HOME/Documents" "$BACKUP_DIR/documents"

# 설정 파일 백업
backup backup -c gzip "$HOME/.bashrc" "$BACKUP_DIR/bashrc"
backup backup -c gzip "$HOME/.vimrc" "$BACKUP_DIR/vimrc"

echo "일일 백업 완료: $BACKUP_DIR"
EOF
    
    mkdir -p "$CONFIG_DIR/backup_scripts"
    chmod +x "$CONFIG_DIR/backup_scripts/daily_backup.sh"
    
    print_success "설정 디렉토리 생성 완료: $CONFIG_DIR"
    print_info "설정 파일: $CONFIG_DIR/config.conf"
    print_info "예제 스크립트: $CONFIG_DIR/backup_scripts/"
    echo
}

install_helper_scripts() {
    print_step "헬퍼 스크립트 설치 중..."
    
    # 헬퍼 스크립트가 있으면 설치
    if [ -f "backup_helper.sh" ]; then
        if [ "$install_type" = "system" ]; then
            sudo cp backup_helper.sh "$INSTALL_PREFIX/bin/"
            sudo chmod 755 "$INSTALL_PREFIX/bin/backup_helper.sh"
            print_success "헬퍼 스크립트 설치: $INSTALL_PREFIX/bin/backup_helper.sh"
        elif [ "$install_type" = "user" ]; then
            cp backup_helper.sh "$HOME/.local/bin/"
            chmod 755 "$HOME/.local/bin/backup_helper.sh"
            print_success "헬퍼 스크립트 설치: $HOME/.local/bin/backup_helper.sh"
        else
            print_info "헬퍼 스크립트: ./backup_helper.sh"
        fi
    else
        print_warning "헬퍼 스크립트를 찾을 수 없습니다: backup_helper.sh"
    fi
    
    echo
}

print_installation_summary() {
    print_step "설치 완료!"
    echo
    
    echo -e "${GREEN}🎉 $PROGRAM_NAME v$VERSION 설치가 완료되었습니다! 🎉${NC}"
    echo
    
    echo -e "${CYAN}설치된 구성 요소:${NC}"
    echo -e "  ${GREEN}✅${NC} 백업 프로그램"
    echo -e "  ${GREEN}✅${NC} 설정 디렉토리"
    echo -e "  ${GREEN}✅${NC} 예제 스크립트"
    
    if [ -f "$INSTALL_PREFIX/bin/backup_helper.sh" ] || [ -f "$HOME/.local/bin/backup_helper.sh" ]; then
        echo -e "  ${GREEN}✅${NC} 헬퍼 스크립트"
    fi
    
    echo
    
    echo -e "${CYAN}사용 방법:${NC}"
    case "$install_type" in
        "system")
            echo -e "  ${WHITE}backup version${NC}                    # 버전 확인"
            echo -e "  ${WHITE}backup help${NC}                       # 도움말"
            echo -e "  ${WHITE}backup backup file.txt backup.txt${NC} # 파일 백업"
            if [ -f "$INSTALL_PREFIX/bin/backup_helper.sh" ]; then
                echo -e "  ${WHITE}backup_helper.sh help${NC}             # 헬퍼 스크립트"
            fi
            ;;
        "user")
            echo -e "  ${WHITE}backup version${NC}                    # 버전 확인"
            echo -e "  ${WHITE}backup help${NC}                       # 도움말"
            echo -e "  ${WHITE}backup backup file.txt backup.txt${NC} # 파일 백업"
            if [ -f "$HOME/.local/bin/backup_helper.sh" ]; then
                echo -e "  ${WHITE}backup_helper.sh help${NC}             # 헬퍼 스크립트"
            fi
            
            if [[ ":$PATH:" != *":$HOME/.local/bin:"* ]]; then
                echo
                echo -e "${YELLOW}⚠️ PATH 설정이 필요합니다:${NC}"
                echo -e "  ${WHITE}echo 'export PATH=\"\$HOME/.local/bin:\$PATH\"' >> ~/.bashrc${NC}"
                echo -e "  ${WHITE}source ~/.bashrc${NC}"
            fi
            ;;
        "local")
            echo -e "  ${WHITE}./bin/backup version${NC}              # 버전 확인"
            echo -e "  ${WHITE}./bin/backup help${NC}                 # 도움말"
            echo -e "  ${WHITE}./bin/backup backup file.txt backup.txt${NC} # 파일 백업"
            if [ -f "backup_helper.sh" ]; then
                echo -e "  ${WHITE}./backup_helper.sh help${NC}           # 헬퍼 스크립트"
            fi
            ;;
    esac
    
    echo
    
    echo -e "${CYAN}추가 리소스:${NC}"
    echo -e "  ${WHITE}설정 파일:${NC}     $CONFIG_DIR/config.conf"
    echo -e "  ${WHITE}예제 스크립트:${NC} $CONFIG_DIR/backup_scripts/"
    echo -e "  ${WHITE}문서:${NC}         README.md"
    
    echo
    
    echo -e "${CYAN}다음 단계:${NC}"
    echo -e "  ${WHITE}1.${NC} make test                    # 테스트 실행"
    echo -e "  ${WHITE}2.${NC} 설정 파일 편집                # 기본값 변경"
    echo -e "  ${WHITE}3.${NC} 첫 번째 백업 시작!"
    
    echo
    echo -e "${PURPLE}${BOLD}안전하고 효율적인 백업을 위해 고급 백업 유틸리티를 선택해 주셔서 감사합니다! 🔒✨${NC}"
    echo
}

# 메인 실행 함수
main() {
    # 배너 출력
    print_banner
    
    # 인수 처리
    local auto_install=false
    local skip_deps=false
    
    while [[ $# -gt 0 ]]; do
        case $1 in
            --auto)
                auto_install=true
                shift
                ;;
            --skip-deps)
                skip_deps=true
                shift
                ;;
            --help)
                echo "사용법: $0 [옵션]"
                echo "옵션:"
                echo "  --auto       자동 설치 (사용자 입력 최소화)"
                echo "  --skip-deps  의존성 설치 건너뛰기"
                echo "  --help       이 도움말 표시"
                exit 0
                ;;
            *)
                print_error "알 수 없는 옵션: $1"
                echo "$0 --help 를 실행하여 도움말을 확인하세요."
                exit 1
                ;;
        esac
    done
    
    # 설치 확인
    if [ "$auto_install" = false ]; then
        echo -e "${CYAN}$PROGRAM_NAME v$VERSION을 설치하시겠습니까? (Y/n):${NC} "
        read -r confirm_install
        if [[ $confirm_install =~ ^[Nn] ]]; then
            print_info "설치가 취소되었습니다."
            exit 0
        fi
    fi
    
    # Ctrl+C 신호 처리
    trap 'echo -e "\n${YELLOW}설치가 중단되었습니다.${NC}"; exit 130' INT TERM
    
    # 설치 단계 실행
    detect_system
    check_root_privileges
    
    if [ "$skip_deps" = false ]; then
        install_dependencies
        verify_dependencies
    else
        print_warning "의존성 설치를 건너뜁니다."
    fi
    
    build_program
    test_build
    install_to_system
    create_config_directory
    install_helper_scripts
    print_installation_summary
    
    # 성공 완료
    exit 0
}

# 스크립트 실행
main "$@"