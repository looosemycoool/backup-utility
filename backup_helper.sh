#!/bin/bash
# 백업 헬퍼 스크립트

if [ "$1" = "backup" ]; then
    ./bin/backup backup --conflict=overwrite "$2" "$3"
elif [ "$1" = "backup-gzip" ]; then
    ./bin/backup backup --conflict=overwrite --compression=gzip "$2" "$3"
elif [ "$1" = "restore" ]; then
    if [[ "$2" == *.gz ]]; then
        echo "GZIP 파일 복원: $2 -> $3"
        gunzip -c "$2" > "$3"
    else
        echo "일반 파일 복원: $2 -> $3"
        cp "$2" "$3"
    fi
elif [ "$1" = "test" ]; then
    echo "=== 백업 유틸리티 테스트 ==="
    
    # 테스트 파일 생성
    echo "Test data $(date)" > test_backup.txt
    
    # 일반 백업
    ./bin/backup backup --conflict=overwrite test_backup.txt backup_normal.txt
    echo "✅ 일반 백업 완료"
    
    # GZIP 백업
    ./bin/backup backup --conflict=overwrite --compression=gzip test_backup.txt backup_gzip.txt
    echo "✅ GZIP 백업 완료"
    
    # 복원 테스트
    gunzip -c backup_gzip.txt.gz > restore_gzip.txt
    cp backup_normal.txt restore_normal.txt
    
    # 결과 확인
    if diff test_backup.txt restore_normal.txt > /dev/null; then
        echo "✅ 일반 복원 성공"
    else
        echo "❌ 일반 복원 실패"
    fi
    
    if diff test_backup.txt restore_gzip.txt > /dev/null; then
        echo "✅ GZIP 복원 성공"
    else
        echo "❌ GZIP 복원 실패"
    fi
    
    # 크기 비교
    echo "파일 크기 비교:"
    ls -lh test_backup.txt backup_normal.txt backup_gzip.txt.gz
else
    echo "사용법: $0 {backup|backup-gzip|restore|test} [소스] [대상]"
    echo "예시:"
    echo "  $0 backup file.txt backup.txt"
    echo "  $0 backup-gzip file.txt backup.txt"
    echo "  $0 restore backup.txt.gz restored.txt"
    echo "  $0 test"
fi
