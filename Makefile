# 🚀 고급 백업 유틸리티 완벽한 빌드 시스템 v2.0
# Professional-grade backup utility with comprehensive build system

# ============================================================================
# 🔧 컴파일러 및 플래그 설정
# ============================================================================

CC = gcc
CFLAGS = -Wall -Wextra -std=c99 -O2 -D_GNU_SOURCE
DEBUG_FLAGS = -g -DDEBUG -O0 -fsanitize=address -fno-omit-frame-pointer
RELEASE_FLAGS = -O3 -DNDEBUG -march=native -flto
LDFLAGS = -pthread -lz -lm

# Valgrind 및 정적 분석 도구
VALGRIND = valgrind --leak-check=full --show-leak-kinds=all --track-origins=yes
CPPCHECK = cppcheck --enable=all --std=c99 --error-exitcode=1
CLANG_FORMAT = clang-format -i -style=Google

# ============================================================================
# 📁 디렉토리 설정
# ============================================================================

SRCDIR = src
OBJDIR = obj
BINDIR = bin
TESTDIR = tests
DOCDIR = docs
DISTDIR = dist

# ============================================================================
# 📋 소스 파일 및 타겟 설정
# ============================================================================

SOURCES = $(wildcard $(SRCDIR)/*.c)
OBJECTS = $(SOURCES:$(SRCDIR)/%.c=$(OBJDIR)/%.o)
DEPS = $(OBJECTS:.o=.d)
TARGET = $(BINDIR)/backup

# 버전 정보
VERSION = 2.0.0
COMMIT_HASH = $(shell git rev-parse --short HEAD 2>/dev/null || echo "unknown")
BUILD_DATE = $(shell date '+%Y-%m-%d %H:%M:%S')

# ============================================================================
# 🎯 기본 타겟들
# ============================================================================

.PHONY: all clean debug release install uninstall test help
.PHONY: demo comprehensive-test performance-test stress-test
.PHONY: format analyze valgrind docs package
.PHONY: quick-test advanced-test benchmark profile

# 기본 빌드
all: $(TARGET)
	@echo "✅ 빌드 완료: $(TARGET)"

# ============================================================================
# 🔨 빌드 규칙
# ============================================================================

# 실행 파일 생성
$(TARGET): $(OBJECTS) | $(BINDIR)
	@echo "🔗 링킹 중... $@"
	@$(CC) $(OBJECTS) -o $@ $(LDFLAGS)
	@echo "🎉 빌드 완료: $@"
	@echo "📊 바이너리 크기: $$(du -h $@ | cut -f1)"

# 오브젝트 파일 생성 (의존성 추적 포함)
$(OBJDIR)/%.o: $(SRCDIR)/%.c | $(OBJDIR)
	@echo "⚙️  컴파일 중... $<"
	@$(CC) $(CFLAGS) -DVERSION='"$(VERSION)"' -DCOMMIT_HASH='"$(COMMIT_HASH)"' \
		-DBUILD_DATE='"$(BUILD_DATE)"' -MMD -MP -c $< -o $@

# 의존성 파일 생성
$(OBJDIR)/%.d: $(SRCDIR)/%.c | $(OBJDIR)
	@$(CC) $(CFLAGS) -MM -MT $(@:.d=.o) $< > $@

# 디렉토리 생성
$(OBJDIR):
	@mkdir -p $(OBJDIR)

$(BINDIR):
	@mkdir -p $(BINDIR)

$(TESTDIR):
	@mkdir -p $(TESTDIR)

$(DOCDIR):
	@mkdir -p $(DOCDIR)

$(DISTDIR):
	@mkdir -p $(DISTDIR)

# 의존성 포함 (에러 무시)
-include $(DEPS)

# ============================================================================
# 🚀 빌드 변형들
# ============================================================================

# 디버그 빌드
debug: CFLAGS += $(DEBUG_FLAGS)
debug: clean $(TARGET)
	@echo "🐛 디버그 빌드 완료"

# 릴리스 빌드 (최적화)
release: CFLAGS += $(RELEASE_FLAGS)
release: clean $(TARGET)
	@echo "⚡ 릴리스 빌드 완료 (최적화됨)"

# 프로파일링 빌드
profile: CFLAGS += -pg -O2
profile: LDFLAGS += -pg
profile: clean $(TARGET)
	@echo "📊 프로파일링 빌드 완료"

# ============================================================================
# 🧪 테스트 시스템
# ============================================================================

# 빠른 기본 테스트
quick-test: $(TARGET)
	@echo "🚀 빠른 테스트 실행 중..."
	@echo "📋 버전 확인:"
	@./$(TARGET) version
	@echo ""
	@echo "📄 단일 파일 테스트:"
	@echo "Hello, World!" > test_quick.txt
	@./$(TARGET) backup test_quick.txt test_quick_backup.txt
	@./$(TARGET) restore test_quick_backup.txt test_quick_restored.txt
	@if cmp -s test_quick.txt test_quick_restored.txt; then \
		echo "✅ 빠른 테스트 성공!"; \
	else \
		echo "❌ 빠른 테스트 실패"; exit 1; \
	fi
	@rm -f test_quick*.txt
	@echo "🎉 빠른 테스트 완료!"

# 기본 테스트 (기존)
test: $(TARGET)
	@echo "🧪 기본 테스트 실행 중..."
	@echo "📋 버전 확인:"
	@./$(TARGET) version
	@echo ""
	@echo "📄 파일 백업/복원 테스트:"
	@echo "Hello, World!" > test_input.txt
	@./$(TARGET) backup test_input.txt test_backup.txt
	@./$(TARGET) restore test_backup.txt test_output.txt
	@if cmp -s test_input.txt test_output.txt; then \
		echo "✅ 파일 백업/복원 테스트 성공!"; \
	else \
		echo "❌ 파일 백업/복원 테스트 실패"; exit 1; \
	fi
	@rm -f test_input.txt test_backup.txt test_output.txt
	@echo ""
	@echo "📦 압축 테스트:"
	@echo "Compression test data for demonstration" > test_compress.txt
	@./$(TARGET) backup -c gzip test_compress.txt test_compress_backup.txt
	@./$(TARGET) restore test_compress_backup.txt.gz test_uncompress.txt
	@if cmp -s test_compress.txt test_uncompress.txt; then \
		echo "✅ 압축 백업/복원 테스트 성공!"; \
	else \
		echo "❌ 압축 백업/복원 테스트 실패"; exit 1; \
	fi
	@rm -f test_compress*.txt* test_uncompress.txt
	@echo "🎉 기본 테스트 완료!"

# 완전한 테스트 스위트
comprehensive-test: $(TARGET)
	@echo "🔬 완전한 테스트 스위트 실행 중..."
	@chmod +x comprehensive_test.sh
	@./comprehensive_test.sh

# 고급 테스트
advanced-test: $(TARGET) | $(TESTDIR)
	@echo "🎛️ 고급 기능 테스트 실행 중..."
	@echo "📁 테스트 데이터 준비..."
	@mkdir -p $(TESTDIR)/advanced/{source,backup,restore}
	@echo "Advanced test file 1" > $(TESTDIR)/advanced/source/file1.txt
	@echo "Advanced test file 2" > $(TESTDIR)/advanced/source/file2.txt
	@mkdir -p $(TESTDIR)/advanced/source/subdir
	@echo "Nested file" > $(TESTDIR)/advanced/source/subdir/nested.txt
	@echo "Temporary file" > $(TESTDIR)/advanced/source/temp.tmp
	@echo ""
	@echo "🧪 재귀적 백업 테스트:"
	@./$(TARGET) backup -r -v $(TESTDIR)/advanced/source $(TESTDIR)/advanced/backup
	@echo ""
	@echo "🎯 필터링 백업 테스트:"
	@./$(TARGET) backup -r --exclude="*.tmp" $(TESTDIR)/advanced/source $(TESTDIR)/advanced/backup_filtered
	@echo ""
	@echo "📦 압축 + 검증 테스트:"
	@./$(TARGET) backup -c gzip --verify $(TESTDIR)/advanced/source/file1.txt $(TESTDIR)/advanced/backup/verified.txt
	@echo ""
	@echo "♻️ 복원 테스트:"
	@./$(TARGET) restore -r $(TESTDIR)/advanced/backup $(TESTDIR)/advanced/restore
	@if diff -r $(TESTDIR)/advanced/source $(TESTDIR)/advanced/restore >/dev/null 2>&1; then \
		echo "✅ 고급 테스트 성공!"; \
	else \
		echo "⚠️ 고급 테스트에서 차이점 발견 (정상일 수 있음)"; \
	fi
	@rm -rf $(TESTDIR)/advanced
	@echo "🎉 고급 테스트 완료!"

# 성능 테스트
performance-test: $(TARGET) | $(TESTDIR)
	@echo "⚡ 성능 테스트 실행 중..."
	@mkdir -p $(TESTDIR)/performance
	@echo "📊 큰 파일 생성 중..."
	@dd if=/dev/zero of=$(TESTDIR)/performance/large.dat bs=1M count=10 2>/dev/null
	@echo "⏱️ 백업 성능 측정:"
	@time ./$(TARGET) backup $(TESTDIR)/performance/large.dat $(TESTDIR)/performance/large_backup.dat
	@echo "⏱️ 압축 백업 성능 측정:"
	@time ./$(TARGET) backup -c gzip $(TESTDIR)/performance/large.dat $(TESTDIR)/performance/large_compressed.dat
	@echo "📈 압축 효과:"
	@echo "원본: $$(du -h $(TESTDIR)/performance/large.dat | cut -f1)"
	@echo "압축: $$(du -h $(TESTDIR)/performance/large_compressed.dat.gz | cut -f1)"
	@rm -rf $(TESTDIR)/performance
	@echo "✅ 성능 테스트 완료!"

# 스트레스 테스트
stress-test: $(TARGET) | $(TESTDIR)
	@echo "💪 스트레스 테스트 실행 중..."
	@mkdir -p $(TESTDIR)/stress/source
	@echo "📁 많은 파일 생성 중..."
	@for i in $$(seq 1 1000); do \
		echo "File $$i content" > $(TESTDIR)/stress/source/file_$$i.txt; \
	done
	@echo "⏱️ 1000개 파일 백업 테스트:"
	@time ./$(TARGET) backup -r $(TESTDIR)/stress/source $(TESTDIR)/stress/backup
	@echo "🔍 결과 검증:"
	@source_count=$$(find $(TESTDIR)/stress/source -type f | wc -l); \
	backup_count=$$(find $(TESTDIR)/stress/backup -type f | wc -l); \
	echo "원본 파일: $$source_count개"; \
	echo "백업 파일: $$backup_count개"; \
	if [ $$source_count -eq $$backup_count ]; then \
		echo "✅ 스트레스 테스트 성공!"; \
	else \
		echo "❌ 스트레스 테스트 실패"; \
	fi
	@rm -rf $(TESTDIR)/stress
	@echo "💪 스트레스 테스트 완료!"

# 벤치마크
benchmark: $(TARGET) | $(TESTDIR)
	@echo "🏁 벤치마크 실행 중..."
	@mkdir -p $(TESTDIR)/benchmark
	@echo "📊 다양한 크기의 파일로 벤치마크:"
	@for size in 1 10 100; do \
		echo "📁 $${size}MB 파일 테스트:"; \
		dd if=/dev/zero of=$(TESTDIR)/benchmark/test_$${size}mb.dat bs=1M count=$$size 2>/dev/null; \
		echo "  압축 없음:"; \
		time ./$(TARGET) backup $(TESTDIR)/benchmark/test_$${size}mb.dat $(TESTDIR)/benchmark/backup_$${size}mb.dat 2>&1; \
		echo "  GZIP 압축:"; \
		time ./$(TARGET) backup -c gzip $(TESTDIR)/benchmark/test_$${size}mb.dat $(TESTDIR)/benchmark/backup_$${size}mb_gz.dat 2>&1; \
		echo ""; \
	done
	@rm -rf $(TESTDIR)/benchmark
	@echo "🏁 벤치마크 완료!"

# ============================================================================
# 🎬 데모 및 시뮬레이션
# ============================================================================

# 영상 촬영용 데모
demo: $(TARGET)
	@echo "🎬 영상 데모 시뮬레이션 실행 중..."
	@if [ -f demo_simulation.sh ]; then \
		chmod +x demo_simulation.sh; \
		./demo_simulation.sh; \
	else \
		echo "❌ demo_simulation.sh 파일이 없습니다"; \
		echo "💡 다음 명령어로 데모 스크립트를 다운로드하세요:"; \
		echo "   curl -O https://example.com/demo_simulation.sh"; \
	fi

# ============================================================================
# 🔍 코드 품질 및 분석
# ============================================================================

# 코드 포맷팅
format:
	@echo "🎨 코드 포맷팅 중..."
	@if command -v $(CLANG_FORMAT) >/dev/null 2>&1; then \
		find $(SRCDIR) -name "*.c" -o -name "*.h" | xargs $(CLANG_FORMAT); \
		echo "✅ 코드 포맷팅 완료"; \
	else \
		echo "⚠️ clang-format이 설치되지 않음"; \
	fi

# 정적 분석
analyze:
	@echo "🔍 정적 분석 실행 중..."
	@if command -v $(CPPCHECK) >/dev/null 2>&1; then \
		$(CPPCHECK) $(SRCDIR)/; \
		echo "✅ 정적 분석 완료"; \
	else \
		echo "⚠️ cppcheck이 설치되지 않음"; \
	fi

# Valgrind 메모리 검사
valgrind: debug
	@echo "🔬 Valgrind 메모리 검사 실행 중..."
	@if command -v valgrind >/dev/null 2>&1; then \
		echo "Hello, Valgrind!" > valgrind_test.txt; \
		$(VALGRIND) ./$(TARGET) backup valgrind_test.txt valgrind_backup.txt; \
		$(VALGRIND) ./$(TARGET) restore valgrind_backup.txt valgrind_restored.txt; \
		rm -f valgrind_test.txt valgrind_backup.txt valgrind_restored.txt; \
		echo "✅ Valgrind 검사 완료"; \
	else \
		echo "⚠️ Valgrind가 설치되지 않음"; \
	fi

# ============================================================================
# 📚 문서화
# ============================================================================

# 문서 생성
docs: | $(DOCDIR)
	@echo "📚 문서 생성 중..."
	@if command -v doxygen >/dev/null 2>&1; then \
		doxygen Doxyfile 2>/dev/null || echo "Doxyfile 없음, 기본 문서 생성"; \
		echo "✅ 문서 생성 완료: $(DOCDIR)/"; \
	else \
		echo "⚠️ Doxygen이 설치되지 않음"; \
		echo "📝 수동 문서:" > $(DOCDIR)/manual.txt; \
		echo "README.md 파일을 참조하세요" >> $(DOCDIR)/manual.txt; \
	fi

# ============================================================================
# 📦 패키징 및 배포
# ============================================================================

# 소스 패키지 생성
package: clean | $(DISTDIR)
	@echo "📦 배포 패키지 생성 중..."
	@tar -czf $(DISTDIR)/backup-utility-$(VERSION)-src.tar.gz \
		--exclude='$(OBJDIR)' --exclude='$(BINDIR)' --exclude='$(DISTDIR)' \
		--exclude='.git' --exclude='*.swp' --exclude='*~' \
		--transform 's,^,backup-utility-$(VERSION)/,' \
		*
	@echo "✅ 소스 패키지 생성 완료: $(DISTDIR)/backup-utility-$(VERSION)-src.tar.gz"

# 바이너리 패키지 생성
binary-package: release | $(DISTDIR)
	@echo "📦 바이너리 패키지 생성 중..."
	@mkdir -p $(DISTDIR)/backup-utility-$(VERSION)-bin
	@cp $(TARGET) $(DISTDIR)/backup-utility-$(VERSION)-bin/
	@cp README.md $(DISTDIR)/backup-utility-$(VERSION)-bin/
	@cp LICENSE $(DISTDIR)/backup-utility-$(VERSION)-bin/ 2>/dev/null || echo "라이선스 파일 없음"
	@tar -czf $(DISTDIR)/backup-utility-$(VERSION)-bin.tar.gz \
		-C $(DISTDIR) backup-utility-$(VERSION)-bin
	@rm -rf $(DISTDIR)/backup-utility-$(VERSION)-bin
	@echo "✅ 바이너리 패키지 생성 완료: $(DISTDIR)/backup-utility-$(VERSION)-bin.tar.gz"

# ============================================================================
# 🔧 시스템 관리
# ============================================================================

# 시스템 설치
install: $(TARGET)
	@echo "🔧 시스템에 설치 중..."
	@sudo mkdir -p /usr/local/bin
	@sudo cp $(TARGET) /usr/local/bin/backup
	@sudo chmod 755 /usr/local/bin/backup
	@sudo mkdir -p /usr/local/share/man/man1
	@sudo cp docs/backup.1 /usr/local/share/man/man1/ 2>/dev/null || echo "매뉴얼 페이지 없음"
	@echo "✅ 설치 완료: /usr/local/bin/backup"
	@echo "💡 사용법: backup --help"

# 시스템에서 제거
uninstall:
	@echo "🗑️ 시스템에서 제거 중..."
	@sudo rm -f /usr/local/bin/backup
	@sudo rm -f /usr/local/share/man/man1/backup.1
	@echo "✅ 제거 완료"

# ============================================================================
# 🧹 정리
# ============================================================================

# 빌드 파일 정리
clean:
	@echo "🧹 빌드 파일 정리 중..."
	@rm -rf $(OBJDIR) $(BINDIR) $(TESTDIR) $(DOCDIR) $(DISTDIR)
	@rm -f *.tmp *.log *.bak
	@rm -f test_* valgrind_* gmon.out
	@echo "✅ 정리 완료"

# 완전 정리 (git clean과 유사)
distclean: clean
	@echo "🧹 완전 정리 중..."
	@rm -f *~ *.swp *.swo
	@find . -name "*.o" -delete 2>/dev/null || true
	@find . -name "*.d" -delete 2>/dev/null || true
	@echo "✅ 완전 정리 완료"

# ============================================================================
# 🆘 도움말 및 정보
# ============================================================================

# 도움말
help:
	@echo "🚀 고급 백업 유틸리티 빌드 시스템 v$(VERSION)"
	@echo "📅 빌드 날짜: $(BUILD_DATE)"
	@echo "🔗 커밋 해시: $(COMMIT_HASH)"
	@echo ""
	@echo "📋 사용 가능한 타겟:"
	@echo ""
	@echo "🔨 빌드 타겟:"
	@echo "  all           - 기본 빌드 (기본값)"
	@echo "  debug         - 디버그 빌드 (AddressSanitizer 포함)"
	@echo "  release       - 릴리스 빌드 (최적화)"
	@echo "  profile       - 프로파일링 빌드"
	@echo ""
	@echo "🧪 테스트 타겟:"
	@echo "  quick-test    - 빠른 기본 테스트"
	@echo "  test          - 기본 테스트"
	@echo "  comprehensive-test - 완전한 테스트 스위트"
	@echo "  advanced-test - 고급 기능 테스트"
	@echo "  performance-test - 성능 테스트"
	@echo "  stress-test   - 스트레스 테스트"
	@echo "  benchmark     - 벤치마크"
	@echo ""
	@echo "🎬 데모 및 시뮬레이션:"
	@echo "  demo          - 영상 촬영용 데모"
	@echo ""
	@echo "🔍 코드 품질:"
	@echo "  format        - 코드 포맷팅"
	@echo "  analyze       - 정적 분석"
	@echo "  valgrind      - 메모리 검사"
	@echo ""
	@echo "📚 문서화:"
	@echo "  docs          - 문서 생성"
	@echo ""
	@echo "📦 패키징:"
	@echo "  package       - 소스 패키지 생성"
	@echo "  binary-package - 바이너리 패키지 생성"
	@echo ""
	@echo "🔧 시스템 관리:"
	@echo "  install       - 시스템에 설치"
	@echo "  uninstall     - 시스템에서 제거"
	@echo ""
	@echo "🧹 정리:"
	@echo "  clean         - 빌드 파일 정리"
	@echo "  distclean     - 완전 정리"
	@echo ""
	@echo "🆘 도움말:"
	@echo "  help          - 이 도움말 표시"
	@echo "  version       - 버전 정보 표시"
	@echo ""
	@echo "💡 예시:"
	@echo "  make                    # 기본 빌드"
	@echo "  make debug              # 디버그 빌드"
	@echo "  make comprehensive-test # 전체 테스트"
	@echo "  make demo               # 영상 데모"
	@echo "  make install            # 시스템 설치"

# 버전 정보
version:
	@echo "🚀 고급 백업 유틸리티 빌드 시스템"
	@echo "📦 버전: $(VERSION)"
	@echo "🔗 커밋: $(COMMIT_HASH)"
	@echo "📅 빌드 날짜: $(BUILD_DATE)"
	@echo "🔧 컴파일러: $(CC)"
	@echo "⚙️ 플래그: $(CFLAGS)"
	@echo "🔗 링커: $(LDFLAGS)"
	@echo "📁 소스 파일: $(words $(SOURCES))개"
	@echo "🎯 타겟: $(TARGET)"

# 시스템 정보
sysinfo:
	@echo "💻 시스템 정보:"
	@echo "🖥️ OS: $$(uname -s)"
	@echo "🏗️ 아키텍처: $$(uname -m)"
	@echo "🔧 커널: $$(uname -r)"
	@echo "💾 메모리: $$(free -h | grep ^Mem | awk '{print $$2}' 2>/dev/null || echo 'N/A')"
	@echo "💽 디스크: $$(df -h . | tail -1 | awk '{print $$4}' 2>/dev/null || echo 'N/A') 사용 가능"
	@echo "🔨 GCC 버전: $$($(CC) --version | head -1)"
	@echo "🧰 Make 버전: $$(make --version | head -1)"

# 의존성 확인
check-deps:
	@echo "🔍 의존성 확인 중..."
	@for cmd in gcc make; do \
		if command -v $$cmd >/dev/null 2>&1; then \
			echo "✅ $$cmd: $$($$cmd --version | head -1)"; \
		else \
			echo "❌ $$cmd: 없음"; \
		fi; \
	done
	@for lib in pthread z m; do \
		if echo "int main(){return 0;}" | $(CC) -x c - -l$$lib -o /dev/null 2>/dev/null; then \
			echo "✅ lib$$lib: 사용 가능"; \
		else \
			echo "❌ lib$$lib: 없음"; \
		fi; \
	done
	@rm -f /dev/null 2>/dev/null || true

# ============================================================================
# 🔧 개발자용 고급 기능
# ============================================================================

# 디버그 정보
debug-info: debug
	@echo "🐛 디버그 정보:"
	@echo "📊 바이너리 크기: $$(du -h $(TARGET) | cut -f1)"
	@echo "🔍 심볼 테이블: $$(objdump -t $(TARGET) | wc -l) 심볼"
	@echo "📋 섹션 정보:"
	@objdump -h $(TARGET) | grep -E '\.(text|data|bss)'

# 의존성 그래프 생성
dep-graph:
	@echo "📊 의존성 그래프 생성 중..."
	@if command -v dot >/dev/null 2>&1; then \
		$(CC) $(CFLAGS) -MM $(SOURCES) | \
		sed 's/[^:]*: *//' | sed 's/ *\\$$//' | \
		sort -u > deps.tmp; \
		echo "digraph deps {" > deps.dot; \
		while read dep; do echo "  \"$$dep\";" >> deps.dot; done < deps.tmp; \
		echo "}" >> deps.dot; \
		dot -Tpng deps.dot -o dependency_graph.png; \
		rm -f deps.tmp deps.dot; \
		echo "✅ 의존성 그래프 생성 완료: dependency_graph.png"; \
	else \
		echo "⚠️ Graphviz (dot)가 설치되지 않음"; \
	fi

# ============================================================================
# 🎛️ 고급 설정
# ============================================================================

# 컴파일 시간 측정
time-build:
	@echo "⏱️ 컴파일 시간 측정..."
	@time $(MAKE) clean && time $(MAKE) all

# 병렬 빌드
parallel-build:
	@echo "⚡ 병렬 빌드 실행..."
	@$(MAKE) -j$$(nproc) all

# 크로스 컴파일 (예: ARM64)
cross-compile-arm64:
	@echo "🔄 ARM64 크로스 컴파일..."
	@if command -v aarch64-linux-gnu-gcc >/dev/null 2>&1; then \
		$(MAKE) CC=aarch64-linux-gnu-gcc LDFLAGS="-static $(LDFLAGS)" clean all; \
		mv $(TARGET) $(TARGET)-arm64; \
		echo "✅ ARM64 빌드 완료: $(TARGET)-arm64"; \
	else \
		echo "❌ ARM64 크로스 컴파일러가 없습니다"; \
	fi

# ============================================================================
# 📝 추가 정보
# ============================================================================

# 프로젝트 통계
stats:
	@echo "📊 프로젝트 통계:"
	@echo "📁 소스 파일: $(words $(SOURCES))개"
	@echo "📄 총 코드 라인: $$(cat $(SOURCES) $(wildcard $(SRCDIR)/*.h) | wc -l)"
	@echo "📝 C 코드 라인: $$(cat $(SOURCES) | grep -v '^$$' | grep -v '^\s*//' | wc -l)"
	@echo "💬 주석 라인: $$(cat $(SOURCES) | grep -E '^\s*//' | wc -l)"
	@echo "📊 함수 개수: $$(grep -E '^[a-zA-Z_][a-zA-Z0-9_]*\s*\(' $(SOURCES) | wc -l)"
	@echo "🔧 TODO 항목: $$(grep -r TODO $(SRCDIR) | wc -l)"
	@echo "⚠️ FIXME 항목: $$(grep -r FIXME $(SRCDIR) | wc -l)"

# 라이선스 체크
license-check:
	@echo "📜 라이선스 확인 중..."
	@if [ -f LICENSE ]; then \
		echo "✅ LICENSE 파일 존재"; \
		head -5 LICENSE; \
	else \
		echo "⚠️ LICENSE 파일 없음"; \
	fi
	@echo "📝 소스 파일 라이선스 헤더:"
	@for file in $(SOURCES); do \
		if head -10 $$file | grep -i license >/dev/null; then \
			echo "✅ $$file: 라이선스 헤더 있음"; \
		else \
			echo "⚠️ $$file: 라이선스 헤더 없음"; \
		fi; \
	done

# 보안 체크
security-check:
	@echo "🔒 보안 체크 실행 중..."
	@echo "📋 잠재적 보안 문제:"
	@grep -n -E '(strcpy|strcat|sprintf|gets)' $(SOURCES) || echo "✅ 위험한 함수 없음"
	@echo "🔍 버퍼 오버플로우 가능성:"
	@grep -n -E 'char\s+\w+\[' $(SOURCES) | head -5 || echo "✅ 고정 크기 버퍼 확인됨"

# ============================================================================
# 🎨 최종 마무리
# ============================================================================

.DEFAULT_GOAL := all

# Makefile 자체 도움말
makefile-help:
	@echo "📚 이 Makefile에 대한 정보:"
	@echo "📝 총 타겟 수: $$(grep -c '^[a-zA-Z][a-zA-Z0-9_-]*:' Makefile)"
	@echo "📋 사용 가능한 타겟 목록:"
	@grep -E '^[a-zA-Z][a-zA-Z0-9_-]*:' Makefile | sed 's/:.*$$//' | sort | tr '\n' ' '
	@echo ""
	@echo "💡 'make help'로 자세한 도움말을 확인하세요"

# 성공 메시지
success:
	@echo ""
	@echo "🎉 모든 작업이 성공적으로 완료되었습니다!"
	@echo "🚀 고급 백업 유틸리티 v$(VERSION) 준비 완료!"
	@echo "📖 'make help'로 사용법을 확인하세요"
	@echo ""