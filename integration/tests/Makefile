# Makefile for Atlas-12288 Integration Tests

CC = gcc
CFLAGS = -std=c99 -Wall -Wextra -O2 -I../include -I../runtime/include
LDFLAGS = -L../runtime/lib -L../llvm/lib
LIBS = -latlas -lm -lpthread

# Detect OS for shared library paths
UNAME_S := $(shell uname -s)
ifeq ($(UNAME_S),Linux)
    RPATH_FLAG = -Wl,-rpath,'$$ORIGIN/../runtime/lib:$$ORIGIN/../llvm/lib'
endif
ifeq ($(UNAME_S),Darwin)
    RPATH_FLAG = -Wl,-rpath,@loader_path/../runtime/lib:@loader_path/../llvm/lib
endif

# Test programs
TESTS = test-layer2-integration

# Build flags for different modes
DEBUG_FLAGS = -g -O0 -DDEBUG
SANITIZE_FLAGS = -fsanitize=address -fsanitize=undefined
THREAD_FLAGS = -fsanitize=thread

all: $(TESTS)

test-layer2-integration: test-layer2-integration.c atlas-stubs.c
	@echo "[CC] Building Layer 2 integration test..."
	@if [ -f ../llvm/lib/libatlas.a ] && [ -f ../runtime/lib/libatlas-runtime.a ]; then \
		echo "[LINK] Linking with Atlas libraries..."; \
		$(CC) $(CFLAGS) test-layer2-integration.c -o $@ $(LDFLAGS) $(RPATH_FLAG) $(LIBS); \
	else \
		echo "[STUB] Using stub implementations..."; \
		$(CC) $(CFLAGS) test-layer2-integration.c atlas-stubs.c -o $@ -lm -lpthread; \
	fi

# Debug build
debug: CFLAGS += $(DEBUG_FLAGS)
debug: $(TESTS)

# Build with AddressSanitizer
asan: CFLAGS += $(SANITIZE_FLAGS)
asan: $(TESTS)

# Build with ThreadSanitizer
tsan: CFLAGS += $(THREAD_FLAGS)
tsan: $(TESTS)

# Run tests
run: $(TESTS)
	@echo "========================================="
	@echo "Running Atlas-12288 Integration Tests"
	@echo "========================================="
	@for test in $(TESTS); do \
		echo "Running $$test..."; \
		./$$test || exit 1; \
		echo ""; \
	done
	@echo "========================================="
	@echo "✅ All integration tests passed!"
	@echo "========================================="

# Run specific test
run-layer2: test-layer2-integration
	@echo "Running Layer 2 integration tests..."
	@./test-layer2-integration

# Clean
clean:
	rm -f $(TESTS) *.o

# Install tests (optional)
install: $(TESTS)
	@echo "[INSTALL] Installing tests..."
	install -d $(PREFIX)/bin/tests
	install -m 755 $(TESTS) $(PREFIX)/bin/tests/

.PHONY: all run run-layer2 clean install debug asan tsan

help:
	@echo "Atlas-12288 Integration Tests"
	@echo "=============================="
	@echo ""
	@echo "Targets:"
	@echo "  make              - Build all tests"
	@echo "  make run          - Run all tests"
	@echo "  make run-layer2   - Run Layer 2 tests only"
	@echo "  make debug        - Build with debug symbols"
	@echo "  make asan         - Build with AddressSanitizer"
	@echo "  make tsan         - Build with ThreadSanitizer"
	@echo "  make clean        - Remove built files"
	@echo ""
	@echo "The tests validate:"
	@echo "  • Layer 2 domain lifecycle"
	@echo "  • Conservation laws"
	@echo "  • Witness generation/verification"
	@echo "  • Layer 2/3 integration"
	@echo "  • Error handling"
	@echo "  • Performance characteristics"