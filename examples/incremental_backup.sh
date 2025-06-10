#!/bin/bash
# 증분 백업 스크립트 예제

BACKUP_BASE="/backup/incremental"
SOURCE_DIR="/home/user/work"

# 주간 전체 백업 (일요일)
if [ $(date +%u) -eq 7 ]; then
    BACKUP_DIR="$BACKUP_BASE/full_$(date +%Y%m%d)"
    ./bin/backup backup -r -c gzip -l 6 -p -v "$SOURCE_DIR" "$BACKUP_DIR"
else
    # 일일 증분 백업
    BACKUP_DIR="$BACKUP_BASE/incremental_$(date +%Y%m%d)"
    ./bin/backup backup -r -i -c gzip -l 6 -p -v "$SOURCE_DIR" "$BACKUP_DIR"
fi
