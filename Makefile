# 고급 파일 백업 유틸리티 Makefile
# Version 2.0.0

CC = gcc
CFLAGS = -Wall -Wextra -std=c99 -O2 -g
TARGET = backup
SRCDIR = src
TESTDIR = test

# 기본 빌드
$(TARGET): $(SRCDIR)/backup.c
	$(CC) $(CFLAGS) -o $(TARGET) $(SRCDIR)/backup.c
	@echo "✅ 빌드 완료: ./$(TARGET)"

# 디버그 빌드
debug: CFLAGS += -DDEBUG -O0
debug: $(TARGET)
	@echo "🐛 디버그 빌드 완료"

# 릴리즈 빌드
release: CFLAGS += -DNDEBUG -O3 -s
release: $(TARGET)
	@echo "🚀 릴리즈 빌드 완료"

# 정리
clean:
	rm -f ./$(TARGET)
	rm -f *.log
	rm -rf $(TESTDIR)
	rm -f test*.txt backup*.txt restored*.txt
	rm -f core dump*
	@echo "🧹 정리 완료"

# 기본 테스트
test: $(TARGET)
	@echo "🧪 기본 테스트 실행..."
	@echo
	@echo "=== 버전 정보 ==="
	./$(TARGET) version
	@echo
	@echo "=== 도움말 ==="
	./$(TARGET) help
	@echo
	@echo "=== 단일 파일 백업 테스트 ==="
	echo "Hello, World! 이것은 테스트 파일입니다." > test.txt
	echo "두 번째 줄입니다." >> test.txt
	echo "세 번째 줄입니다." >> test.txt
	./$(TARGET) backup -v -m -c test.txt backup_test.txt
	@echo
	@echo "=== 백업된 파일 확인 ==="
	cat backup_test.txt
	@echo
	@echo "=== 복원 테스트 ==="
	rm test.txt
	./$(TARGET) restore -v backup_test.txt restored_test.txt
	@echo
	@echo "=== 복원된 파일 확인 ==="
	cat restored_test.txt
	@echo
	@echo "✅ 기본 테스트 완료!"

# 고급 테스트
test-advanced: $(TARGET)
	@echo "🚀 고급 테스트 실행..."
	@echo
	@echo "=== 테스트 환경 준비 ==="
	mkdir -p $(TESTDIR)/source/subdir1/subdir2
	mkdir -p $(TESTDIR)/backup
	echo "메인 파일입니다." > $(TESTDIR)/source/main.txt
	echo "하위 디렉토리 파일1" > $(TESTDIR)/source/subdir1/file1.txt
	echo "하위 디렉토리 파일2" > $(TESTDIR)/source/subdir1/file2.txt
	echo "깊은 하위 디렉토리 파일" > $(TESTDIR)/source/subdir1/subdir2/deep.txt
	@echo
	@echo "=== 재귀 디렉토리 백업 테스트 ==="
	./$(TARGET) backup -v -r -m -c -l backup.log $(TESTDIR)/source $(TESTDIR)/backup
	@echo
	@echo "=== 백업 목록 확인 ==="
	./$(TARGET) list $(TESTDIR)/backup
	@echo
	@echo "=== 로그 파일 확인 ==="
	if [ -f backup.log ]; then echo "로그 파일 마지막 5줄:"; tail -5 backup.log; fi
	@echo
	@echo "✅ 고급 테스트 완료!"

# 성능 테스트
test-performance: $(TARGET)
	@echo "⚡ 성능 테스트 실행..."
	@echo
	@echo "=== 대용량 파일 생성 ==="
	dd if=/dev/zero of=large_test.txt bs=1M count=10 2>/dev/null
	@echo
	@echo "=== 대용량 파일 백업 테스트 ==="
	time ./$(TARGET) backup -v -c large_test.txt large_backup.txt
	@echo
	@echo "=== 파일 크기 확인 ==="
	ls -lh large_test.txt large_backup.txt
	@echo
	@echo "✅ 성능 테스트 완료!"

# 오류 처리 테스트
test-error: $(TARGET)
	@echo "🔥 오류 처리 테스트 실행..."
	@echo
	@echo "=== 존재하지 않는 파일 백업 시도 ==="
	./$(TARGET) backup -v nonexistent.txt backup.txt || echo "예상된 오류입니다."
	@echo
	@echo "=== 권한 없는 디렉토리 백업 시도 ==="
	./$(TARGET) backup -v /root/. backup/ || echo "예상된 오류입니다."
	@echo
	@echo "=== 잘못된 명령어 테스트 ==="
	./$(TARGET) invalid_command || echo "예상된 오류입니다."
	@echo
	@echo "✅ 오류 처리 테스트 완료!"

# 전체 테스트 스위트
test-all: test test-advanced test-performance test-error
	@echo
	@echo "🎉 모든 테스트 완료!"

# 코드 스타일 검사 (cppcheck가 설치된 경우)
lint:
	@if command -v cppcheck >/dev/null 2>&1; then \
		echo "🔍 코드 스타일 검사..."; \
		cppcheck --enable=all --std=c99 $(SRCDIR)/backup.c; \
	else \
		echo "⚠️  cppcheck가 설치되지 않았습니다."; \
	fi

# 설치
install: $(TARGET)
	@echo "📦 시스템에 설치 중..."
	sudo cp $(TARGET) /usr/local/bin/
	sudo chmod +x /usr/local/bin/$(TARGET)
	@echo "✅ 설치 완료: /usr/local/bin/$(TARGET)"

# 제거
uninstall:
	@echo "🗑️  시스템에서 제거 중..."
	sudo rm -f /usr/local/bin/$(TARGET)
	@echo "✅ 제거 완료"

# 배포 패키지 생성
package: clean $(TARGET)
	@echo "📦 배포 패키지 생성 중..."
	mkdir -p backup-utility-2.0.0
	cp $(TARGET) backup-utility-2.0.0/
	cp Makefile backup-utility-2.0.0/
	cp README.md backup-utility-2.0.0/
	cp -r $(SRCDIR) backup-utility-2.0.0/
	tar -czf backup-utility-2.0.0.tar.gz backup-utility-2.0.0/
	rm -rf backup-utility-2.0.0/
	@echo "✅ 패키지 생성 완료: backup-utility-2.0.0.tar.gz"

# 도움말
help:
	@echo "사용 가능한 Make 타겟:"
	@echo "  make          - 기본 빌드"
	@echo "  make debug    - 디버그 빌드"
	@echo "  make release  - 릴리즈 빌드"
	@echo "  make test     - 기본 테스트"
	@echo "  make test-advanced - 고급 테스트"
	@echo "  make test-performance - 성능 테스트"
	@echo "  make test-error - 오류 처리 테스트"
	@echo "  make test-all - 모든 테스트"
	@echo "  make lint     - 코드 스타일 검사"
	@echo "  make install  - 시스템에 설치"
	@echo "  make uninstall - 시스템에서 제거"
	@echo "  make package  - 배포 패키지 생성"
	@echo "  make clean    - 정리"
	@echo "  make help     - 이 도움말"

.PHONY: debug release clean test test-advanced test-performance test-error test-all lint install uninstall package help
