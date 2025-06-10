#!/bin/bash
# 시스템 설정 백업 스크립트

BACKUP_DIR="/backup/system/$(date +%Y%m%d)"
mkdir -p "$BACKUP_DIR"

# 중요한 시스템 설정 백업
./bin/backup backup -r -m -v /etc "$BACKUP_DIR/etc"
./bin/backup backup -r -m -v /home/*/.bashrc /home/*/.profile "$BACKUP_DIR/user_configs"
./bin/backup backup -r -m -v /var/log "$BACKUP_DIR/logs"

# 패키지 목록 백업
if command -v dpkg &> /dev/null; then
    dpkg --get-selections > "$BACKUP_DIR/package_list.txt"
elif command -v rpm &> /dev/null; then
    rpm -qa > "$BACKUP_DIR/package_list.txt"
fi

# 백업 검증
./bin/backup verify "$BACKUP_DIR"
