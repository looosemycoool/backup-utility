# 파일 백업 유틸리티

ELEC462 시스템 프로그래밍 팀 프로젝트 - 파일 및 디렉토리 백업/복원 시스템

## 프로젝트 개요

이 프로젝트는 C 언어로 구현된 명령줄 기반 파일 백업 유틸리티입니다. 파일과 디렉토리의 백업, 복원, 목록 관리 기능을 제공하며, 시스템 프로그래밍 개념을 활용하여 구현되었습니다.

## 주요 기능

- **파일 백업**: 개별 파일 또는 디렉토리 전체 백업
- **파일 복원**: 백업된 파일을 원본 위치 또는 지정된 위치로 복원
- **메타데이터 보존**: 파일 권한, 수정 시간 등 메타데이터 유지
- **증분 백업**: 변경된 파일만 백업하여 효율성 향상
- **로깅 시스템**: 상세한 로그 기록 및 오류 추적
- **충돌 처리**: 파일 덮어쓰기 시 사용자 선택 옵션

## 시스템 요구사항

- **운영체제**: Linux/Unix 계열
- **컴파일러**: GCC 4.8 이상
- **필수 라이브러리**: POSIX 표준 라이브러리

## 설치 및 빌드

### 1. 저장소 클론
```bash
git clone https://github.com/your-username/backup-utility.git
cd backup-utility
```

### 2. 컴파일
```bash
make
```

### 3. 테스트
```bash
make test
```

### 4. 설치 (선택사항)
```bash
sudo make install
```

## 사용법

### 기본 명령어

```bash
# 도움말 보기
./backup help

# 파일 백업
./backup backup <소스> <목적지>

# 파일 복원  
./backup restore <백업파일> <목적지>

# 백업 목록 보기
./backup list <백업디렉토리>
```

### 옵션

- `-v, --verbose`: 상세 정보 출력
- `-r, --recursive`: 디렉토리 재귀 처리
- `-i, --incremental`: 증분 백업
- `-m, --metadata`: 메타데이터 보존

### 사용 예시

```bash
# 단일 파일 백업 (상세 모드)
./backup backup -v document.txt backup/

# 디렉토리 재귀 백업 (메타데이터 포함)
./backup backup -r -m /home/user/documents backup/docs/

# 증분 백업
./backup backup -i -v /home/user/project backup/project/

# 파일 복원
./backup restore backup/document.txt /home/user/restored/
```

## 프로젝트 구조

```
backup-utility/
├── src/
│   ├── backup.c          # 메인 프로그램
│   ├── backup_ops.c      # 백업 기능 구현
│   ├── backup_ops.h      # 백업 기능 헤더
│   ├── restore_ops.c     # 복원 기능 구현
│   ├── restore_ops.h     # 복원 기능 헤더
│   ├── file_utils.c      # 파일 유틸리티 구현
│   ├── file_utils.h      # 파일 유틸리티 헤더
│   ├── logging.c         # 로깅 시스템 구현
│   └── logging.h         # 로깅 시스템 헤더
├── tests/                # 테스트 파일들
├── Makefile             # 빌드 스크립트
├── README.md            # 프로젝트 문서
└── .gitignore           # Git 무시 파일 목록
```

## 구현된 시스템 콜

이 프로젝트에서 사용된 주요 시스템 콜들:

- `open()`, `read()`, `write()`, `close()`: 파일 I/O
- `stat()`, `lstat()`: 파일 정보 조회
- `mkdir()`: 디렉토리 생성
- `opendir()`, `readdir()`, `closedir()`: 디렉토리 탐색
- `chmod()`, `chown()`: 권한 및 소유자 설정
- `utime()`: 파일 시간 정보 설정

## 개발 진행 상황

- [x] 프로젝트 구조 설정
- [x] 로깅 시스템 구현
- [x] 파일 유틸리티 기본 기능
- [x] 단일 파일 백업/복원
-
