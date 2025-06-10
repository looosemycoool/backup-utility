# 🚀 고급 파일 백업 유틸리티 v2.0.0

안전하고 효율적인 C 기반 파일 백업 도구입니다. 단순한 파일 복사부터 복잡한 디렉토리 재귀 백업까지 지원합니다.

## ✨ 주요 기능

### 🔄 백업 기능
- **단일 파일 백업**: 개별 파일을 안전하게 백업
- **재귀 디렉토리 백업**: 전체 디렉토리 구조를 유지하며 백업
- **메타데이터 보존**: 파일 권한, 타임스탬프 등 원본 정보 유지
- **백업 검증**: 복사 후 무결성 자동 확인
- **진행률 표시**: 대용량 파일 백업 시 실시간 진행 상황

### 🔄 복원 기능
- **완전 복원**: 백업된 파일을 원래 상태로 복원
- **메타데이터 복원**: 권한과 타임스탬프까지 완전히 복원

### 📊 관리 기능
- **백업 목록**: 백업 디렉토리의 모든 파일/폴더 목록 표시
- **상세 정보**: 파일 크기, 수정일, 타입 등 표시
- **로깅 시스템**: 콘솔 및 파일 로그 지원

### 🛡️ 안전성
- **오류 처리**: 파일 없음, 권한 부족 등 다양한 상황 처리
- **원자적 연산**: 백업 실패 시 부분 파일 자동 정리
- **검증 시스템**: 백업 완료 후 내용 검증 옵션

## 🛠️ 설치

### 필수 요구사항
- GCC 컴파일러
- POSIX 호환 시스템 (Linux, macOS, WSL)
- Make 빌드 도구

### 빌드 및 설치

```bash
# 저장소 클론
git clone https://github.com/yourusername/backup-utility.git
cd backup-utility

# 기본 빌드
make

# 또는 최적화된 릴리즈 빌드
make release

# 시스템에 설치 (선택사항)
make install
```

## 📖 사용법

### 기본 명령어 구조

```bash
./backup [명령어] [옵션] [인자...]
```

### 🔧 명령어

#### `backup` - 파일/디렉토리 백업
```bash
# 단일 파일 백업
./backup backup source.txt backup.txt

# 메타데이터 보존 + 검증
./backup backup -m -c important.doc backup/important.doc

# 디렉토리 재귀 백업 (모든 옵션)
./backup backup -r -v -m -c -l backup.log ~/documents backup/documents/
```

#### `restore` - 백업 복원
```bash
# 파일 복원
./backup restore backup.txt restored.txt

# 메타데이터와 함께 복원
./backup restore -m -v backup/important.doc ~/documents/important.doc
```

#### `list` - 백업 목록 보기
```bash
# 백업 디렉토리 내용 확인
./backup list backup/

# 상세 정보 포함
./backup list -v backup/
```

#### `help` - 도움말
```bash
./backup help
```

#### `version` - 버전 정보
```bash
./backup version
```

### ⚙️ 옵션

| 옵션 | 전체 이름 | 설명 |
|------|-----------|------|
| `-v` | `--verbose` | 상세 정보 출력 |
| `-r` | `--recursive` | 디렉토리 재귀 처리 |
| `-m` | `--metadata` | 메타데이터 보존 (권한, 타임스탬프) |
| `-c` | `--verify` | 백업 후 검증 |
| `-l` | `--log <파일>` | 로그를 파일에 저장 |

## 📚 사용 예시

### 1. 일반적인 파일 백업

```bash
# 중요한 문서 백업
./backup backup -v -m -c report.pdf backup/report_backup.pdf

# 결과:
# [INFO] 2025-06-01 20:30:15: 프로그램 시작: backup v2.0.0
# [INFO] 2025-06-01 20:30:15: 백업 시작: report.pdf -> backup/report_backup.pdf
# [DEBUG] 2025-06-01 20:30:15: 파일 복사 시작: report.pdf (2.1 MB)
# 진행률: 100% (2097152/2097152 bytes)
# [DEBUG] 2025-06-01 20:30:16: 메타데이터 보존 완료: backup/report_backup.pdf
# [INFO] 2025-06-01 20:30:16: 파일 복사 완료: report.pdf -> backup/report_backup.pdf (2097152 bytes)
# [INFO] 2025-06-01 20:30:16: 백업 검증 성공: backup/report_backup.pdf
# [INFO] 2025-06-01 20:30:16: 프로그램 종료 (종료 코드: 0)
```

### 2. 전체 디렉토리 백업

```bash
# 프로젝트 전체 백업
./backup backup -r -v -m -c -l project_backup.log ~/my_project backup/my_project/

# 로그 파일 확인
tail -f project_backup.log
```

### 3. 백업 복원

```bash
# 백업에서 파일 복원
./backup restore -v -m backup/report_backup.pdf ~/documents/report_restored.pdf

# 전체 디렉토리 복원 (수동으로 각 파일)
./backup list backup/my_project/
# 나열된 파일들을 하나씩 복원...
```

### 4. 백업 관리

```bash
# 백업 목록 확인
./backup list backup/

# 결과:
# === 백업 목록: backup/ ===
# 이름                                     크기        수정일시              타입
# ----                                     ----        --------              ----
# report_backup.pdf                        2.1 MB      2025-06-01 20:30:16   파일
# my_project                               -           2025-06-01 20:35:22   디렉토리
# important.doc                            145.2 KB    2025-06-01 19:45:10   파일
# 
# 총 3 항목
```

## 🧪 테스트

### 기본 테스트
```bash
make test
```

### 고급 테스트 (디렉토리, 로깅 등)
```bash
make test-advanced
```

### 성능 테스트 (대용량 파일)
```bash
make test-performance
```

### 오류 처리 테스트
```bash
make test-error
```

### 전체 테스트 스위트
```bash
make test-all
```

## 🔧 개발자 가이드

### 프로젝트 구조

```
backup-utility/
├── backup                    # 실행 파일
├── Makefile                 # 빌드 시스템
├── README.md                # 이 파일
├── src/
│   └── backup.c             # 메인 소스 코드
└── test/                    # 테스트 디렉토리 (테스트 시 생성)
```

### 빌드 옵션

```bash
make debug      # 디버그 빌드 (-g -O0)
make release    # 릴리즈 빌드 (-O3 -s)
make lint       # 코드 스타일 검사 (cppcheck 필요)
```

### 코드 품질

- **C99 표준** 준수
- **POSIX 호환성** 보장
- **메모리 안전성** 고려
- **오류 처리** 철저히 구현
- **로깅 시스템** 내장

## 🚀 고급 사용법

### 1. 자동화된 백업 스크립트

```bash
#!/bin/bash
# daily_backup.sh

LOG_DIR="/var/log/backup"
BACKUP_DIR="/backup/daily"
SOURCE_DIR="/home/user/important"

mkdir -p "$LOG_DIR" "$BACKUP_DIR"

./backup backup -r -v -m -c \
    -l "$LOG_DIR/backup_$(date +%Y%m%d).log" \
    "$SOURCE_DIR" \
    "$BACKUP_DIR/backup_$(date +%Y%m%d)"

echo "백업 완료: $(date)"
```

### 2. Cron을 이용한 정기 백업

```bash
# crontab -e에 추가
# 매일 오전 2시에 백업 실행
0 2 * * * /path/to/backup-utility/daily_backup.sh
```

### 3. 백업 복원 스크립트

```bash
#!/bin/bash
# restore_backup.sh

BACKUP_DATE="$1"
BACKUP_DIR="/backup/daily/backup_$BACKUP_DATE"
RESTORE_DIR="/home/user/restored"

if [ ! -d "$BACKUP_DIR" ]; then
    echo "백업이 존재하지 않습니다: $BACKUP_DATE"
    exit 1
fi

echo "백업 목록:"
./backup list "$BACKUP_DIR"

echo "복원을 계속하시겠습니까? (y/N)"
read -r response
if [[ "$response" =~ ^[Yy]$ ]]; then
    mkdir -p "$RESTORE_DIR"
    # 실제 복원 작업...
    echo "복원 완료"
fi
```

## 🐛 문제 해결

### 일반적인 문제들

#### 1. 권한 오류
```bash
# 오류: Permission denied
# 해결: 파일/디렉토리 권한 확인
ls -la source_file
chmod 644 source_file  # 필요시 권한 수정
```

#### 2. 디스크 공간 부족
```bash
# 오류: No space left on device
# 해결: 디스크 공간 확인
df -h
# 백업 대상 크기 확인
du -sh source_directory
```

#### 3. 메모리 부족 (대용량 파일)
```bash
# 대용량 파일은 청크 단위로 처리됨 (8KB)
# 시스템 메모리와 무관하게 안전하게 처리
```

### 로그 분석

```bash
# 로그 레벨별 확인
grep "ERROR" backup.log    # 오류만 확인
grep "WARNING" backup.log  # 경고만 확인
grep "INFO" backup.log     # 정보만 확인

# 실시간 로그 모니터링
tail -f backup.log
```

## 📊 성능

### 벤치마크 (참고용)

| 파일 크기 | 백업 시간 | 검증 시간 | 총 시간 |
|-----------|----------|----------|---------|
| 10 MB     | 0.1초    | 0.05초   | 0.15초  |
| 100 MB    | 1.2초    | 0.6초    | 1.8초   |
| 1 GB      | 12초     | 6초      | 18초    |
| 10 GB     | 2분      | 1분      | 3분     |

*결과는 시스템 사양에 따라 달라질 수 있습니다.*

### 최적화 팁

1. **SSD 사용**: 기계식 하드디스크보다 빠른 성능
2. **검증 옵션**: `-c` 옵션은 시간이 추가로 소요되지만 안전성 증대
3. **로그 레벨**: `-v` 옵션 없이 사용하면 더 빠른 성능
4. **네트워크 저장소**: 로컬 디스크 대비 느릴 수 있음

## 🤝 기여하기

1. **Fork** 저장소
2. **Feature branch** 생성 (`git checkout -b feature/amazing-feature`)
3. **변경사항 커밋** (`git commit -m 'Add amazing feature'`)
4. **Branch에 Push** (`git push origin feature/amazing-feature`)
5. **Pull Request** 생성

### 개발 가이드라인

- **C99 표준** 준수
- **함수는 60줄 이내**로 유지
- **의미있는 변수명** 사용
- **모든 함수에 오류 처리** 구현
- **테스트 케이스** 추가



## 🔄 변경 로그

### v2.0.0 (2025-06-02)
- ✨ 디렉토리 재귀 백업 기능 추가
- ✨ 메타데이터 보존 기능 추가  
- ✨ 백업 검증 시스템 구현
- ✨ 고급 로깅 시스템 (파일 로그 지원)
- ✨ 백업 목록 관리 기능
- 🐛 메모리 누수 수정
- 🚀 성능 최적화 (청크 기반 복사)

### v1.0.0 (2025-06-01)
- 🎉 초기 릴리즈
- ✨ 기본 파일 백업/복원 기능
- ✨ 명령줄 인터페이스
- ✨ 기본 로깅 시스템

## 🙏 감사의 말

이 프로젝트는 다음 기술들을 기반으로 합니다:
- **C 표준 라이브러리**
- **POSIX API**
- **GNU Make**

