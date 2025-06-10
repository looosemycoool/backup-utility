# 🚀 빠른 시작 가이드

이 가이드를 따라하면 5분 안에 고급 백업 유틸리티를 설치하고 사용할 수 있습니다.

## 📋 필요사항

### 최소 요구사항
- **운영체제**: Linux, macOS, 또는 POSIX 호환 시스템
- **컴파일러**: GCC 또는 Clang
- **빌드 도구**: Make
- **라이브러리**: zlib, OpenSSL

### 권장 사양
- **RAM**: 최소 1GB (큰 파일 백업 시 더 많이 필요)
- **디스크**: 백업할 데이터의 2-3배 여유 공간
- **CPU**: 멀티코어 권장 (병렬 처리용)

## ⚡ 1분 설치

### 방법 1: 자동 설정 스크립트 사용 (권장)

```bash
# 1. 저장소 클론 또는 파일 다운로드
git clone https://github.com/yourname/backup-utility.git
cd backup-utility

# 2. 자동 설정 실행 (모든 것을 자동으로 설정)
chmod +x setup.sh
./setup.sh
```

### 방법 2: 수동 설치

```bash
# 1. 의존성 설치
# Ubuntu/Debian:
sudo apt-get install build-essential libssl-dev zlib1g-dev

# CentOS/RHEL:
sudo yum groupinstall "Development Tools"
sudo yum install openssl-devel zlib-devel

# macOS:
brew install gcc openssl zlib

# 2. 프로젝트 구조 생성
mkdir -p {src,obj,bin,tests,docs,config,examples}

# 3. 소스 파일들을 src/ 디렉토리에 배치
# - backup.h, main.c, backup.c, restore.c, 
#   file_utils.c, logging.c, compression.c

# 4. 빌드
make clean && make

# 5. 테스트
make test
```

## 🎯 첫 번째 백업

### 단일 파일 백업

```bash
# 간단한 파일 백업
./bin/backup backup README.md backup_readme.md

# 압축 백업
./bin/backup backup -c gzip README.md backup_readme.md.gz

# 메타데이터 보존하며 백업
./bin/backup backup -m README.md backup_readme.md
```

### 디렉토리 백업

```bash
# 전체 디렉토리 백업 (재귀적)
./bin/backup backup -r -v documents/ backup/documents/

# 압축과 진행률 표시를 포함한 백업
./bin/backup backup -r -c gzip -p -v documents/ backup/documents_compressed/

# 메타데이터 보존 + 병렬 처리
./bin/backup backup -r -m -j 4 -p -v documents/ backup/documents_full/
```

## 🔄 복원하기

### 기본 복원

```bash
# 단일 파일 복원
./bin/backup restore backup_readme.md restored_readme.md

# 압축된 파일 복원
./bin/backup restore backup_readme.md.gz restored_readme.md

# 디렉토리 복원
./bin/backup restore -r backup/documents/ restored/documents/
```

## 🔍 백업 검증

```bash
# 백업 무결성 검증
./bin/backup verify backup/documents/

# 백업 내용 목록 보기
./bin/backup list backup/documents/
```

## ⚙️ 설정 사용

### 기본 설정으로 실행

```bash
# 설정 파일 지정
./bin/backup backup --config=config/backup.conf -r documents/ backup/

# 개발용 설정 (디버그 모드)
./bin/backup backup --config=config/backup-dev.conf -r documents/ backup/

# 성능 최적화 설정
./bin/backup backup --config=config/backup-performance.conf -r documents/ backup/
```

## 🎨 고급 기능 맛보기

### 1. 필터링 백업

```bash
# 텍스트 파일만 백업
./bin/backup backup -r --include="*.txt" documents/ backup/text_only/

# 임시 파일 제외하고 백업
./bin/backup backup -r --exclude="*.tmp" --exclude="temp*" documents/ backup/clean/

# 큰 파일 제외 (10MB 이상)
./bin/backup backup -r --max-size=10485760 documents/ backup/small_files/
```

### 2. 증분 백업

```bash
# 첫 번째 전체 백업
./bin/backup backup -r -v documents/ backup/full/

# 변경된 파일만 증분 백업
./bin/backup backup -r -i -v documents/ backup/incremental/
```

### 3. 병렬 처리

```bash
# 4개 스레드로 빠른 백업
./bin/backup backup -r -j 4 -p -v large_directory/ backup/parallel/
```

## 🧪 테스트 실행

### 기본 테스트

```bash
# Make 테스트 (권장)
make test

# 자동화된 테스트 스크립트
./tests/run_tests.sh

# 성능 테스트
make performance-test

# 고급 기능 테스트
make advanced-test
```

### 수동 테스트

```bash
# 테스트 데이터 생성
mkdir -p test_data
echo "Hello, World!" > test_data/hello.txt
echo "Test file 2" > test_data/test2.txt
mkdir -p test_data/subdir
echo "Subdirectory file" > test_data/subdir/sub.txt

# 백업 테스트
./bin/backup backup -r -v test_data/ test_backup/

# 복원 테스트  
./bin/backup restore -r -v test_backup/ test_restore/

# 결과 확인
diff -r test_data/ test_restore/
```

## ⭐ 유용한 팁

### 1. 자주 사용하는 명령어 별칭 설정

```bash
# ~/.bashrc 또는 ~/.zshrc에 추가
alias backup='./bin/backup backup -r -m -p -v'
alias restore='./bin/backup restore -r -m -v'
alias backup-quick='./bin/backup backup -r -c gzip -l 1 -j 4 -p'
alias backup-best='./bin/backup backup -r -c gzip -l 9 -m -V'
```

### 2. 환경 변수 설정

```bash
# ~/.bashrc에 추가
export BACKUP_CONFIG="$HOME/.backup.conf"
export BACKUP_LOG_DIR="$HOME/.backup/logs"
export BACKUP_TMP_DIR="/tmp/backup_$$"
```

### 3. 크론탭으로 자동 백업 설정

```bash
# 크론탭 편집
crontab -e

# 매일 새벽 2시에 백업 실행
0 2 * * * /path/to/backup-utility/examples/daily_backup.sh

# 매주 일요일 전체 백업, 평일 증분 백업
0 2 * * 0 /path/to/backup-utility/examples/weekly_full_backup.sh
0 2 * * 1-6 /path/to/backup-utility/examples/daily_incremental.sh
```

## 🎯 일반적인 사용 사례

### 사례 1: 개발 프로젝트 백업

```bash
# 소스 코드 백업 (압축, 빠른 처리)
./bin/backup backup -r -c gzip -l 6 --exclude="node_modules" \
    --exclude="*.log" --exclude=".git" \
    ~/projects/ /backup/projects/

# 복원
./bin/backup restore -r /backup/projects/ ~/restored_projects/
```

### 사례 2: 시스템 설정 백업

```bash
# 중요한 설정 파일들 백업
sudo ./bin/backup backup -r -m -v \
    /etc /home/user/.config /home/user/.bashrc \
    /backup/system_config/

# 권한 포함해서 복원
sudo ./bin/backup restore -r -m /backup/system_config/ /
```

### 사례 3: 미디어 파일 백업

```bash
# 큰 미디어 파일들 (압축 효과 낮으니 압축 안함)
./bin/backup backup -r -j 4 -p --exclude="*.tmp" \
    /home/user/Pictures/ /backup/pictures/

./bin/backup backup -r -j 4 -p \
    /home/user/Videos/ /backup/videos/
```

## ❓ 자주 묻는 질문

### Q: 백업이 너무 느려요
```bash
# 성능 최적화 옵션 사용
./bin/backup backup -r -j 4 -c lz4 --config=config/backup-performance.conf
```

### Q: 특정 파일 타입만 백업하고 싶어요
```bash
# 이미지 파일만 백업
./bin/backup backup -r --include="*.jpg" --include="*.png" --include="*.gif"
```

### Q: 네트워크 스토리지에 백업할 때 최적화는?
```bash
# 압축률 높이고 검증 활성화
./bin/backup backup -r -c gzip -l 9 -V
```

### Q: 백업이 중단됐는데 이어서 할 수 있나요?
```bash
# 증분 백업으로 변경된 부분만 처리
./bin/backup backup -r -i
```

## 🆘 문제 해결

### 빌드 오류
```bash
# 의존성 다시 설치
sudo apt-get install --reinstall build-essential libssl-dev zlib1g-dev

# 깨끗하게 다시 빌드
make distclean && make debug
```

### 권한 오류
```bash
# sudo 없이 실행하거나 권한 부여
chmod +x bin/backup

# 메타데이터 보존 옵션 제거
./bin/backup backup -r documents/ backup/  # -m 옵션 제거
```

### 메모리 부족
```bash
# 스레드 수 줄이기
./bin/backup backup -r -j 1

# 작은 버퍼 사용
./bin/backup backup -r --config=config/backup-minimal.conf
```
