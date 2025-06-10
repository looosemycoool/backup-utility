# 고급 백업 유틸리티 Makefile

# 컴파일러 설정
CC = gcc
CFLAGS = -Wall -std=c99 -O2 -D_GNU_SOURCE -w
DEBUG_FLAGS = -g -DDEBUG -O0
RELEASE_FLAGS = -O3 -DNDEBUG
LDFLAGS = -pthread -lz -lm

# 디렉토리 설정
SRCDIR = src
OBJDIR = obj
BINDIR = bin

# 소스 파일
SOURCES = $(wildcard $(SRCDIR)/*.c)
OBJECTS = $(SOURCES:$(SRCDIR)/%.c=$(OBJDIR)/%.o)
DEPS = $(OBJECTS:.o=.d)
TARGET = $(BINDIR)/backup

# 기본 타겟
.PHONY: all clean debug release install uninstall test help quick-test advanced-test

all: $(TARGET)

# 실행 파일 생성
$(TARGET): $(OBJECTS) | $(BINDIR)
	@echo "링킹 중... $@"
	@$(CC) $(OBJECTS) -o $@ $(LDFLAGS)
	@echo "빌드 완료: $@"

# 오브젝트 파일 생성
$(OBJDIR)/%.o: $(SRCDIR)/%.c | $(OBJDIR)
	@echo "컴파일 중... $<"
	@$(CC) $(CFLAGS) -c $< -o $@

# 의존성 파일 생성
$(OBJDIR)/%.d: $(SRCDIR)/%.c | $(OBJDIR)
	@$(CC) $(CFLAGS) -MM -MT $(@:.d=.o) $< > $@

# 디렉토리 생성
$(OBJDIR):
	@mkdir -p $(OBJDIR)

$(BINDIR):
	@mkdir -p $(BINDIR)

# 의존성 포함
-include $(DEPS)

# 디버그 빌드
debug: CFLAGS += $(DEBUG_FLAGS)
debug: $(TARGET)

# 릴리스 빌드
release: CFLAGS += $(RELEASE_FLAGS)
release: $(TARGET)

# 정리
clean:
	@echo "빌드 파일 정리..."
	@rm -rf $(OBJDIR) $(BINDIR)
	@echo "정리 완료"

# 빠른 테스트
quick-test: $(TARGET)
	@echo "🚀 빠른 테스트 실행 중..."
	@./$(TARGET) version
	@echo "Hello, World!" > test_quick.txt
	@./$(TARGET) backup test_quick.txt test_quick_backup.txt
	@./$(TARGET) restore test_quick_backup.txt test_quick_restored.txt
	@if cmp -s test_quick.txt test_quick_restored.txt; then \
		echo "✅ 빠른 테스트 성공!"; \
	else \
		echo "❌ 빠른 테스트 실패"; \
	fi
	@rm -f test_quick*.txt

# 기본 테스트
test: $(TARGET)
	@echo "=== 기본 테스트 ==="
	@./$(TARGET) version
	@echo "Hello, World!" > test_input.txt
	@./$(TARGET) backup test_input.txt test_backup.txt
	@./$(TARGET) restore test_backup.txt test_output.txt
	@if cmp -s test_input.txt test_output.txt; then \
		echo "✅ 파일 백업/복원 테스트 성공!"; \
	else \
		echo "❌ 파일 백업/복원 테스트 실패"; \
	fi
	@rm -f test_input.txt test_backup.txt test_output.txt

# 도움말
help:
	@echo "고급 백업 유틸리티 빌드 시스템"
	@echo ""
	@echo "사용 가능한 타겟:"
	@echo "  all         - 기본 빌드"
	@echo "  debug       - 디버그 빌드"
	@echo "  release     - 릴리스 빌드"
	@echo "  clean       - 빌드 파일 정리"
	@echo "  test        - 기본 테스트"
	@echo "  quick-test  - 빠른 테스트"
	@echo "  help        - 이 도움말"
