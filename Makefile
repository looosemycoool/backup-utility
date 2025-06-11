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
.PHONY: all clean debug release install uninstall test help quick-test advanced-test comprehensive-test demo

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

# 설치
install: $(TARGET)
	@echo "설치 중..."
	@sudo cp $(TARGET) /usr/local/bin/
	@sudo chmod 755 /usr/local/bin/backup
	@echo "설치 완료: /usr/local/bin/backup"

# 제거
uninstall:
	@echo "제거 중..."
	@sudo rm -f /usr/local/bin/backup
	@echo "제거 완료"

# 빠른 테스트
quick-test: $(TARGET)
	@echo "🚀 빠른 테스트 실행 중..."
	@./$(TARGET) version
	@echo "Hello, World!" > test_quick.txt
	@./$(TARGET) backup --conflict=overwrite test_quick.txt test_quick_backup.txt
	@./$(TARGET) restore test_quick_backup.txt test_quick_restored.txt
	@if cmp -s test_quick.txt test_quick_restored.txt; then \
		echo "✅ 빠른 테스트 성공!"; \
	else \
		echo "❌ 빠른 테스트 실패"; \
	fi
	@rm -f test_quick*.txt

# 고급 테스트
advanced-test: $(TARGET)
	@echo "🧪 고급 테스트 실행 중..."
	@echo ""
	@echo "=== 압축 테스트 ==="
	@echo "압축 테스트 데이터입니다." > test_compress.txt
	@./$(TARGET) backup --conflict=overwrite --compression=gzip test_compress.txt test_compress.txt
	@echo "압축 파일 크기:"
	@ls -lh test_compress.txt*
	@echo ""
	@echo "=== 디렉토리 테스트 ==="
	@mkdir -p test_dir/{subdir1,subdir2}
	@echo "파일 1" > test_dir/file1.txt
	@echo "파일 2" > test_dir/subdir1/file2.txt
	@echo "파일 3" > test_dir/subdir2/file3.txt
	@./$(TARGET) backup --conflict=overwrite -r test_dir backup_dir
	@echo "백업된 구조:"
	@find backup_dir -type f 2>/dev/null || echo "디렉토리 백업 확인 필요"
	@echo ""
	@echo "=== 정리 ==="
	@rm -rf test_compress.txt* test_dir backup_dir
	@echo "✅ 고급 테스트 완료!"

# 완전한 테스트
comprehensive-test: $(TARGET)
	@echo "🔬 완전한 테스트 실행 중..."
	@if [ -f ./comprehensive_test.sh ]; then \
		chmod +x ./comprehensive_test.sh && ./comprehensive_test.sh; \
	else \
		echo "comprehensive_test.sh 파일이 없습니다. 기본 테스트를 실행합니다."; \
		$(MAKE) advanced-test; \
	fi

# 데모 실행
demo: $(TARGET)
	@echo "🎬 데모 실행 중..."
	@if [ -f ./demo_simulation.sh ]; then \
		chmod +x ./demo_simulation.sh && ./demo_simulation.sh; \
	else \
		echo "demo_simulation.sh 파일이 없습니다. 기본 데모를 실행합니다."; \
		echo "=== 백업 유틸리티 데모 ==="; \
		./$(TARGET) version; \
		echo ""; \
		echo "데모 파일 생성..."; \
		echo "Hello, Backup Demo!" > demo_file.txt; \
		echo "데모 백업 실행..."; \
		./$(TARGET) backup --conflict=overwrite -v demo_file.txt demo_backup.txt; \
		echo "데모 복원 실행..."; \
		./$(TARGET) restore demo_backup.txt demo_restored.txt; \
		echo "결과 비교:"; \
		if cmp -s demo_file.txt demo_restored.txt; then \
			echo "✅ 데모 성공!"; \
		else \
			echo "❌ 데모 실패"; \
		fi; \
		rm -f demo_*.txt; \
	fi

# 기본 테스트
test: $(TARGET)
	@echo "=== 기본 테스트 ==="
	@./$(TARGET) version
	@echo ""
	@echo "Hello, World!" > test_input.txt
	@./$(TARGET) backup --conflict=overwrite test_input.txt test_backup.txt
	@./$(TARGET) restore test_backup.txt test_output.txt
	@if cmp -s test_input.txt test_output.txt; then \
		echo "✅ 파일 백업/복원 테스트 성공!"; \
	else \
		echo "❌ 파일 백업/복원 테스트 실패"; \
	fi
	@rm -f test_input.txt test_backup.txt test_output.txt
	@echo ""
	@echo "=== 압축 테스트 ==="
	@echo "Compression test data" > test_compress.txt
	@./$(TARGET) backup --conflict=overwrite --compression=gzip test_compress.txt test_compress.txt
	@./$(TARGET) restore test_compress.txt.gz test_uncompress.txt
	@if cmp -s test_compress.txt test_uncompress.txt; then \
		echo "✅ 압축 백업/복원 테스트 성공!"; \
	else \
		echo "❌ 압축 백업/복원 테스트 실패"; \
	fi
	@rm -f test_compress.txt test_compress.txt.gz test_uncompress.txt
	@echo "테스트 완료!"

# 벤치마크
benchmark: $(TARGET)
	@echo "⚡ 성능 벤치마크 실행 중..."
	@echo "큰 테스트 파일 생성 중..."
	@dd if=/dev/zero of=benchmark_file.txt bs=1M count=10 2>/dev/null
	@echo "파일 크기: $$(ls -lh benchmark_file.txt | awk '{print $$5}')"
	@echo ""
	@echo "=== 일반 백업 벤치마크 ==="
	@time ./$(TARGET) backup --conflict=overwrite benchmark_file.txt benchmark_normal.txt
	@echo ""
	@echo "=== GZIP 압축 백업 벤치마크 ==="
	@time ./$(TARGET) backup --conflict=overwrite --compression=gzip benchmark_file.txt benchmark_gzip.txt
	@echo ""
	@echo "=== 파일 크기 비교 ==="
	@ls -lh benchmark_*.txt*
	@echo ""
	@echo "=== 복원 벤치마크 ==="
	@time ./$(TARGET) restore benchmark_gzip.txt.gz benchmark_restored.txt
	@if cmp -s benchmark_file.txt benchmark_restored.txt; then \
		echo "✅ 벤치마크 성공!"; \
	else \
		echo "❌ 벤치마크 실패"; \
	fi
	@rm -f benchmark_*.txt*

# 코드 품질 검사
check: $(TARGET)
	@echo "🔍 코드 품질 검사 중..."
	@if command -v cppcheck >/dev/null 2>&1; then \
		echo "Cppcheck 실행 중..."; \
		cppcheck --enable=all --std=c99 $(SRCDIR)/; \
	else \
		echo "Cppcheck가 설치되지 않았습니다."; \
	fi
	@if command -v valgrind >/dev/null 2>&1; then \
		echo ""; \
		echo "Valgrind 메모리 검사 중..."; \
		echo "테스트 데이터" > valgrind_test.txt; \
		valgrind --tool=memcheck --leak-check=full --error-exitcode=1 \
			./$(TARGET) backup --conflict=overwrite valgrind_test.txt valgrind_backup.txt; \
		rm -f valgrind_test.txt valgrind_backup.txt; \
	else \
		echo "Valgrind가 설치되지 않았습니다."; \
	fi

# 문서 생성
docs:
	@echo "📚 문서 생성 중..."
	@if command -v doxygen >/dev/null 2>&1; then \
		doxygen Doxyfile; \
		echo "문서가 docs/ 디렉토리에 생성되었습니다."; \
	else \
		echo "Doxygen이 설치되지 않았습니다."; \
	fi

# 패키지 생성
package: release
	@echo "📦 패키지 생성 중..."
	@mkdir -p package/backup-utility-$(shell date +%Y%m%d)
	@cp -r $(SRCDIR) package/backup-utility-$(shell date +%Y%m%d)/
	@cp $(TARGET) package/backup-utility-$(shell date +%Y%m%d)/
	@cp Makefile package/backup-utility-$(shell date +%Y%m%d)/
	@cp README.md package/backup-utility-$(shell date +%Y%m%d)/ 2>/dev/null || echo "README.md 없음"
	@cd package && tar -czf backup-utility-$(shell date +%Y%m%d).tar.gz backup-utility-$(shell date +%Y%m%d)
	@echo "패키지 생성 완료: package/backup-utility-$(shell date +%Y%m%d).tar.gz"

# 전체 정리
distclean: clean
	@echo "전체 정리 중..."
	@rm -rf package docs *.tar.gz
	@echo "전체 정리 완료"

# 도움말
help:
	@echo "고급 백업 유틸리티 빌드 시스템"
	@echo ""
	@echo "사용 가능한 타겟:"
	@echo "  all               - 기본 빌드 (기본값)"
	@echo "  debug             - 디버그 빌드"
	@echo "  release           - 릴리스 빌드 (최적화)"
	@echo "  clean             - 빌드 파일 정리"
	@echo "  install           - 시스템에 설치"
	@echo "  uninstall         - 시스템에서 제거"
	@echo ""
	@echo "테스트 타겟:"
	@echo "  test              - 기본 테스트"
	@echo "  quick-test        - 빠른 테스트"
	@echo "  advanced-test     - 고급 테스트"
	@echo "  comprehensive-test- 완전한 테스트"
	@echo "  benchmark         - 성능 벤치마크"
	@echo ""
	@echo "기타 타겟:"
	@echo "  demo              - 데모 실행"
	@echo "  check             - 코드 품질 검사"
	@echo "  docs              - 문서 생성"
	@echo "  package           - 배포 패키지 생성"
	@echo "  distclean         - 전체 정리"
	@echo "  help              - 이 도움말 표시"
	@echo ""
	@echo "예시:"
	@echo "  make              # 기본 빌드"
	@echo "  make debug        # 디버그 빌드"
	@echo "  make test         # 테스트 실행"
	@echo "  make install      # 시스템 설치"

# 버전 정보
version:
	@echo "고급 백업 유틸리티 빌드 시스템"
	@echo "컴파일러: $(CC)"
	@echo "플래그: $(CFLAGS)"
	@echo "링커: $(LDFLAGS)"
	@if [ -f $(TARGET) ]; then \
		echo "실행 파일: 존재"; \
		$(TARGET) version; \
	else \
		echo "실행 파일: 없음 (make 실행 필요)"; \
	fi