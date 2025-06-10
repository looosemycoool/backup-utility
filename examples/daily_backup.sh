#!/bin/bash
# 일일 백업 스크립트 예제

BACKUP_DIR="/backup/daily/$(date +%Y%m%d)"
SOURCE_DIR="/home/user/documents"

# 디렉토리 생성
mkdir -p "$BACKUP_DIR"

# 백업 실행
./bin/backup backup -r -c gzip -l 6 -p -v -L "$BACKUP_DIR/backup.log" \
    "$SOURCE_DIR" "$BACKUP_DIR"

# 7일 이상 된 백업 정리
find /backup/daily -maxdepth 1 -type d -mtime +7 -exec rm -rf {} \;
