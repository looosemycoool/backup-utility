#!/bin/bash

# 🚀 고급 백업 유틸리티 자동 설치 스크립트
# 시스템 의존성 검사, 빌드, 설치를 자동으로 수행합니다

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

# 설치 설정
INSTALL_PREFIX="/usr/local"
BUILD_TYPE="release"
ENABLE_TESTS="yes"
SUDO_REQUIRED="no"

# 시스템 정보
OS=""
DISTRO=""
PACKAGE_MANAGER=""

# 로그 함수들
log_info() {
    echo -e "${BLUE}[INFO]${NC} $1"
}

log_success() {
    echo -e "${GREEN}[SUCCESS]${NC} $1"
}

log_error() {
    echo -e "${RED}[ERROR]${NC} $1"
}

log_warning() {
    echo -e "${YELLOW}[WARNING]${NC} $1"
}

log_step() {
    echo -e "${CYAN}[STEP]${NC} $1"
}

# 시스템 감지
detect_system() {
    log_step "시스템 환경 감지 중..."
    
    OS=$(uname -s)
    
    case "$OS" in
        Linux)
            if [ -f /etc/os-release ]; then
                . /etc/os-release
                DISTRO=$ID
                
                case "$DISTRO" in
                    ubuntu|debian)
                        PACKAGE_MANAGER="apt"
                        ;;
                    centos|rhel|fedora)
                        PACKAGE_MANAGER="yum"
                        ;;
                    arch)
                        PACKAGE_MANAGER="pacman"
                        ;;
                    *)
                        PACKAGE_MANAGER="unknown"
                        ;;
                esac
            fi
            ;;
        Darwin)
            DISTRO="macos"
            if command -v brew >/dev/null 2>&1; then
                PACKAGE_MANAGER="brew"
            else
                PACKAGE_MANAGER="none"
            fi
            ;;
        *)
            DISTRO="unknown"
            PACKAGE_MANAGER="unknown"
            ;;
    esac
    
    log_info "운영체제: $OS"
    log_info "배포판: $DISTRO"
    log_info "패키지 매니저: $PACKAGE_MANAGER"
}

# 권한 확인
check_permissions() {
    log_step "권한 확인 중..."
    
    if [[ $EUID -eq 0 ]]; then
        log_warning "루트 권한으로 실행 중입니다"
        SUDO_REQUIRED="no"
    else
        if sudo -n true 2>/dev/null; then
            log_info "sudo 권한 사용 가능"
            SUDO_REQUIRED="yes"
        else
            log_warning "sudo 권한이 필요할 수 있습니다"
            SUDO_REQUIRED="yes"
        fi
    fi
}

# 의존성 설치
install_dependencies() {
    log_step "의존성 설치 중..."
    
    case "$PACKAGE_MANAGER" in
        apt)
            log_info "APT를 사용하여 의존성 설치..."
            if [[ $SUDO_REQUIRED == "yes" ]]; then
                sudo apt-get update
                sudo apt-get install -y build-essential libssl-dev zlib1g-dev \
                    libpthread-stubs0-dev git make gcc
            else
                apt-get update
                apt-get install -y build-essential libssl-dev zlib1g-dev \
                    libpthread-stubs0-dev git make gcc
            fi
            ;;
        yum)
            log_info "YUM을 사용하여 의존성 설치..."
            if [[ $SUDO_REQUIRED == "yes" ]]; then
                sudo yum install -y gcc openssl-devel zlib-devel \
                    glibc-devel git make
            else
                yum install -y gcc openssl-devel zlib-devel \
                    glibc-devel git make
            fi
            ;;
        brew)
            log_info "Homebrew를 사용하여 의존성 설치..."
            brew install openssl zlib
            ;;
        pacman)
            log_info "Pacman을 사용하여 의존성 설치..."
            if [[ $SUDO_REQUIRED == "yes" ]]; then
                sudo pacman -S --noconfirm gcc openssl zlib make git
            else
                pacman -S --noconfirm gcc openssl zlib make git
            fi
            ;;
        *)
            log_warning "자동 의존성 설치를 지원하지 않는 시스템입니다"
            log_info "다음 패키지들을 수동으로 설치해주세요:"
            echo "  - gcc (또는 clang)"
            echo "  - make"
            echo "  - openssl-dev"
            echo "  - zlib-dev"
            echo "  - pthread"
            read -p "의존성이 설치되었나요? (y/N): " deps_ready
            if [[ ! $deps_ready =~ ^[Yy]$ ]]; then
                log_error "의존성 설치가 필요합니다"
                exit 1
            fi
            ;;
    esac
    
    log_success "의존성 설치 완료"
}

# 선택적 도구 설치
install_optional_tools() {
    log_step "선택적 개발 도구 설치 여부 확인..."
    
    echo "다음 선택적 도구들을 설치하시겠습니까?"
    echo "  - valgrind (메모리 검사)"
    echo "  - cppcheck (정적 분석)"
    echo "  - clang-format (코드 포맷팅)"
    echo "  - doxygen (문서 생성)"
    
    read -p "선택적 도구 설치 (y/N): " install_optional
    
    if [[ $install_optional =~ ^[Yy]$ ]]; then
        case "$PACKAGE_MANAGER" in
            apt)
                if [[ $SUDO_REQUIRED == "yes" ]]; then
                    sudo apt-get install -y valgrind cppcheck clang-format doxygen graphviz
                else
                    apt-get install -y valgrind cppcheck clang-format doxygen graphviz
                fi
                ;;
            yum)
                if [[ $SUDO_REQUIRED == "yes" ]]; then
                    sudo yum install -y valgrind cppcheck doxygen graphviz
                else
                    yum install -y valgrind cppcheck doxygen graphviz
                fi
                ;;
            brew)
                brew install valgrind cppcheck doxygen graphviz
                ;;
            pacman)
                if [[ $SUDO_REQUIRED == "yes" ]]; then
                    sudo pacman -S --noconfirm valgrind cppcheck doxygen graphviz
                else
                    pacman -S --noconfirm valgrind cppcheck doxygen graphviz
                fi
                ;;
        esac
        log_success "선택적 도구 설치 완료"
    else
        log_info "선택적 도구 설치 건너뛰기"
    fi
}

# 의존성 검증
verify_dependencies() {
    log_step "의존성 검증 중..."
    
    local missing_deps=()
    
    # 필수 도구 확인
    for tool in gcc make; do
        if ! command -v $tool >/dev/null 2>&1; then
            missing_deps+=($tool)
        fi
    done
    
    # 라이브러리 확인
    for lib in pthread z m; do
        if ! echo "int main(){return 0;}" | gcc -x c - -l$lib -o /dev/null 2>/dev/null; then
            missing_deps+=(lib$lib)
        fi
    done
    
    if [ ${#missing_deps[@]} -eq 0 ]; then
        log_success "모든 의존성이 만족되었습니다"
    else
        log_error "다음 의존성이 누락되었습니다: ${missing_deps[*]}"
        exit 1
    fi
}

# 빌드 설정
configure_build() {
    log_step "빌드 설정 중..."
    
    echo "빌드 타입을 선택하세요:"
    echo "  1) release - 최적화된 릴리스 빌드 (권장)"
    echo "  2) debug   - 디버그 빌드"
    echo "  3) profile - 프로파일링 빌드"
    
    read -p "선택 (1-3, 기본값: 1): " build_choice
    
    case "$build_choice" in
        2)
            BUILD_TYPE="debug"
            log_info "디버그 빌드로 설정"
            ;;
        3)
            BUILD_TYPE="profile"
            log_info "프로파일링 빌드로 설정"
            ;;
        *)
            BUILD_TYPE="release"
            log_info "릴리스 빌드로 설정"
            ;;
    esac
    
    echo "설치 경로를 선택하세요:"
    echo "  1) /usr/local (시스템 전체, sudo 필요)"
    echo "  2) ~/.local (사용자만)"
    echo "  3) 사용자 정의"
    
    read -p "선택 (1-3, 기본값: 1): " install_choice
    
    case "$install_choice" in
        2)
            INSTALL_PREFIX="$HOME/.local"
            log_info "사용자 디렉토리에 설치: $INSTALL_PREFIX"
            mkdir -p "$INSTALL_PREFIX/bin"
            ;;
        3)
            read -p "설치 경로 입력: " custom_prefix
            INSTALL_PREFIX="$custom_prefix"
            log_info "사용자 정의 경로에 설치: $INSTALL_PREFIX"
            mkdir -p "$INSTALL_PREFIX/bin"
            ;;
        *)
            INSTALL_PREFIX="/usr/local"
            log_info "시스템 디렉토리에 설치: $INSTALL_PREFIX"
            ;;
    esac
}

# 빌드 실행
build_project() {
    log_step "프로젝트 빌드 중..."
    
    # 이전 빌드 정리
    make clean >/dev/null 2>&1 || true
    
    # 빌드 실행
    log_info "빌드 타입: $BUILD_TYPE"
    if make $BUILD_TYPE; then
        log_success "빌드 완료"
    else
        log_error "빌드 실패"
        exit 1
    fi
}

# 테스트 실행
run_tests() {
    if [[ $ENABLE_TESTS == "yes" ]]; then
        log_step "테스트 실행 중..."
        
        echo "테스트를 실행하시겠습니까? (권장)"
        read -p "테스트 실행 (Y/n): " run_test_choice
        
        if [[ ! $run_test_choice =~ ^[Nn]$ ]]; then
            log_info "빠른 테스트 실행 중..."
            if make quick-test; then
                log_success "기본 테스트 통과"
                
                echo "고급 테스트도 실행하시겠습니까? (시간이 더 오래 걸립니다)"
                read -p "고급 테스트 실행 (y/N): " advanced_test_choice
                
                if [[ $advanced_test_choice =~ ^[Yy]$ ]]; then
                    log_info "고급 테스트 실행 중..."
                    if make advanced-test; then
                        log_success "고급 테스트 통과"
                    else
                        log_warning "고급 테스트 실패 (설치는 계속됩니다)"
                    fi
                fi
            else
                log_warning "기본 테스트 실패 (설치는 계속됩니다)"
            fi
        else
            log_info "테스트 건너뛰기"
        fi
    fi
}

# 설치 실행
install_project() {
    log_step "프로젝트 설치 중..."
    
    if [[ "$INSTALL_PREFIX" == "/usr/local" ]]; then
        if [[ $SUDO_REQUIRED == "yes" ]]; then
            if sudo make install; then
                log_success "설치 완료: $INSTALL_PREFIX/bin/backup"
            else
                log_error "설치 실패"
                exit 1
            fi
        else
            if make install; then
                log_success "설치 완료: $INSTALL_PREFIX/bin/backup"
            else
                log_error "설치 실패"
                exit 1
            fi
        fi
    else
        # 사용자 정의 경로에 설치
        if cp bin/backup "$INSTALL_PREFIX/bin/"; then
            chmod +x "$INSTALL_PREFIX/bin/backup"
            log_success "설치 완료: $INSTALL_PREFIX/bin/backup"
        else
            log_error "설치 실패"
            exit 1
        fi
    fi
}

# PATH 설정
setup_path() {
    log_step "PATH 설정 확인 중..."
    
    if [[ "$INSTALL_PREFIX" != "/usr/local" ]]; then
        if ! echo "$PATH" | grep -q "$INSTALL_PREFIX/bin"; then
            log_info "PATH에 $INSTALL_PREFIX/bin 추가가 필요합니다"
            
            echo "다음 중 하나를 선택하세요:"
            echo "  1) 자동으로 ~/.bashrc에 추가"
            echo "  2) 수동으로 설정"
            echo "  3) 건너뛰기"
            
            read -p "선택 (1-3): " path_choice
            
            case "$path_choice" in
                1)
                    echo "export PATH=\"$INSTALL_PREFIX/bin:\$PATH\"" >> ~/.bashrc
                    log_success "~/.bashrc에 PATH 추가됨"
                    log_info "새 터미널을 열거나 'source ~/.bashrc' 실행"
                    ;;
                2)
                    log_info "다음 라인을 셸 설정 파일에 추가하세요:"
                    echo "export PATH=\"$INSTALL_PREFIX/bin:\$PATH\""
                    ;;
                3)
                    log_info "PATH 설정 건너뛰기"
                    log_warning "프로그램 실행 시 전체 경로를 사용하세요: $INSTALL_PREFIX/bin/backup"
                    ;;
            esac
        else
            log_success "PATH가 이미 올바르게 설정되어 있습니다"
        fi
    fi
}

# 설치 후 검증
verify_installation() {
    log_step "설치 검증 중..."
    
    local backup_path
    if [[ "$INSTALL_PREFIX" == "/usr/local" ]]; then
        backup_path="/usr/local/bin/backup"
    else
        backup_path="$INSTALL_PREFIX/bin/backup"
    fi
    
    if [[ -x "$backup_path" ]]; then
        log_success "실행 파일 확인됨: $backup_path"
        
        # 버전 확인
        if "$backup_path" version >/dev/null 2>&1; then
            log_success "프로그램이 정상적으로 실행됩니다"
            
            # 버전 정보 표시
            log_info "설치된 버전:"
            "$backup_path" version
        else
            log_warning "프로그램 실행에 문제가 있을 수 있습니다"
        fi
    else
        log_error "설치 검증 실패: 실행 파일을 찾을 수 없습니다"
        exit 1
    fi
}

# 사용법 안내
show_usage_guide() {
    log_step "사용법 안내"
    
    local backup_cmd
    if command -v backup >/dev/null 2>&1; then
        backup_cmd="backup"
    else
        backup_cmd="$INSTALL_PREFIX/bin/backup"
    fi
    
    echo
    echo -e "${GREEN}╔══════════════════════════════════════════════════════════════════╗${NC}"
    echo -e "${GREEN}║                    🎉 설치 완료!                                    ║${NC}"
    echo -e "${GREEN}╚══════════════════════════════════════════════════════════════════╝${NC}"
    echo
    echo -e "${CYAN}💡 빠른 시작 가이드:${NC}"
    echo
    echo "📋 도움말 보기:"
    echo "  $backup_cmd help"
    echo
    echo "📄 기본 파일 백업:"
    echo "  $backup_cmd backup myfile.txt backup_myfile.txt"
    echo
    echo "📦 압축 백업:"
    echo "  $backup_cmd backup -c gzip largefile.dat compressed_backup.dat"
    echo
    echo "🗂️ 디렉토리 백업:"
    echo "  $backup_cmd backup -r my_folder/ backup_folder/"
    echo
    echo "♻️ 복원:"
    echo "  $backup_cmd restore backup_file.txt restored_file.txt"
    echo
    echo -e "${CYAN}📚 더 많은 정보:${NC}"
    echo "  - README.md 파일을 참조하세요"
    echo "  - make demo 명령어로 영상 데모를 시청하세요"
    echo "  - make comprehensive-test로 전체 테스트를 실행하세요"
    echo
}

# 정리
cleanup() {
    log_step "임시 파일 정리 중..."
    # 필요시 임시 파일 정리
    true
}

# 메인 설치 프로세스
main() {
    echo -e "${BLUE}🚀 고급 백업 유틸리티 자동 설치 스크립트${NC}"
    echo -e "${CYAN}이 스크립트는 시스템을 분석하고 필요한 의존성을 설치한 후 백업 유틸리티를 빌드하고 설치합니다.${NC}"
    echo
    
    # 설치 확인
    read -p "설치를 시작하시겠습니까? (y/N): " start_install
    if [[ ! $start_install =~ ^[Yy]$ ]]; then
        log_info "설치가 취소되었습니다"
        exit 0
    fi
    
    # 현재 디렉토리 확인
    if [[ ! -f "Makefile" ]] || [[ ! -d "src" ]]; then
        log_error "백업 유틸리티 소스 디렉토리에서 실행해주세요"
        exit 1
    fi
    
    # 설치 과정 실행
    detect_system
    check_permissions
    configure_build
    
    # 의존성 설치 여부 확인
    echo "시스템 의존성을 자동으로 설치하시겠습니까?"
    echo "(거부하면 수동으로 의존성을 설치해야 합니다)"
    read -p "의존성 자동 설치 (Y/n): " auto_deps
    
    if [[ ! $auto_deps =~ ^[Nn]$ ]]; then
        install_dependencies
        install_optional_tools
    fi
    
    verify_dependencies
    build_project
    run_tests
    install_project
    setup_path
    verify_installation
    show_usage_guide
    cleanup
    
    log_success "🎉 설치가 성공적으로 완료되었습니다!"
}

# 에러 처리
trap 'log_error "설치 중 오류가 발생했습니다. 라인 $LINENO에서 스크립트가 중단되었습니다."; exit 1' ERR

# 스크립트 실행
main "$@"