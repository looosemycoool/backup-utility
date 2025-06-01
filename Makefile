# Makefile for Backup Utility

CC = gcc
CFLAGS = -Wall -Wextra -std=c99 -O2 -g
TARGET = backup
SRCDIR = src
OBJDIR = obj

# 간단한 단일 파일 빌드
$(TARGET): $(SRCDIR)/backup.c
	$(CC) $(CFLAGS) -o $(TARGET) $(SRCDIR)/backup.c
	@echo "빌드 완료: ./$(TARGET)"

clean:
	rm -rf $(OBJDIR)
	rm -f ./$(TARGET)
	rm -f *.log
	rm -f test*.txt backup*.txt restored*.txt
	@echo "정리 완료"

test: $(TARGET)
	@echo "=== 기본 테스트 실행 ==="
	./$(TARGET) help
	@echo
	@echo "=== 테스트 파일 생성 ==="
	echo "Hello, World! 이것은 테스트 파일입니다." > test.txt
	echo "두 번째 줄입니다." >> test.txt
	@echo
	@echo "=== 백업 테스트 ==="
	./$(TARGET) backup -v test.txt backup_test.txt
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
	@echo "=== 테스트 완료! ==="

install: $(TARGET)
	cp $(TARGET) /usr/local/bin/
	@echo "설치 완료: /usr/local/bin/$(TARGET)"

.PHONY: clean test install
