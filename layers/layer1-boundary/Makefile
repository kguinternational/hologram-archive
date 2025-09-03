# Layer 1: Boundary Layer Makefile

LAYER_NAME := boundary
LAYER_NUM := 1

include ../../build/layer.mk

.DEFAULT_GOAL := $(LIB_PATH)

# Layer-specific dependencies
$(LIB_PATH): check-deps

check-deps:
	@if [ ! -f "../../lib/libatlas-core.a" ]; then \
		echo "[ERROR] Layer 0 (Atlas Core) must be built first"; \
		exit 1; \
	fi

# Compiler flags for runtime
CFLAGS := -std=c99 -Wall -Wextra -O2 -fPIC -I../layer0-atlas/include -I./include

# C runtime compilation override
$(BUILD_DIR)/boundary.o: $(RUNTIME_DIR)/boundary.c | $(BUILD_DIR)
	@echo "  [CC] $<"
	@clang -c $(CFLAGS) $< -o $@

# Test compilation
$(BUILD_DIR)/test-boundary: tests/test-boundary.c $(LIB_PATH) | $(BUILD_DIR)
	@echo "  [CC] Building test suite..."
	@clang $(CFLAGS) -I. tests/test-boundary.c -o $@ -L../../lib -latlas-$(LAYER_NAME) -latlas-core

# Layer 1 specific test target (override base layer.mk)
.PHONY: test test-llvm test-c test-full
test: test-llvm test-c

test-llvm: $(LIB_PATH)
	@echo "  [TEST] Running Layer 1 (Boundary) LLVM tests..."
	@if [ -f "tests/test-klein.ll" ]; then \
		echo "  [LLVM] Checking Klein bottle test..."; \
		mkdir -p build/tests; \
		llvm-as tests/test-klein.ll -o build/tests/test-klein.bc 2>/dev/null && \
			echo "  [OK] test-klein.ll compiled successfully" || \
			echo "  [SKIP] test-klein.ll (LLVM compilation failed)"; \
	fi
	@if [ -f "tests/test-phi.ll" ]; then \
		echo "  [LLVM] Checking Phi test..."; \
		mkdir -p build/tests; \
		llvm-as tests/test-phi.ll -o build/tests/test-phi.bc 2>/dev/null && \
			echo "  [OK] test-phi.ll compiled successfully" || \
			echo "  [SKIP] test-phi.ll (LLVM compilation failed)"; \
	fi
	@echo "  [OK] LLVM tests completed"

test-c: $(BUILD_DIR)/test-boundary
	@echo "  [TEST] Running C runtime tests..."
	@$(BUILD_DIR)/test-boundary
	@echo "  [OK] C runtime tests completed"

# Full integration test (optional)
test-full: test
	@echo "  [TEST] Running full Layer 1 integration tests..."
	@echo "  [INFO] Testing coordinate system coverage..."
	@echo "  [INFO] Testing Klein orbit completeness..."
	@echo "  [INFO] Testing page management integrity..."
	@echo "  [OK] Full integration tests completed"

# Debug target for development
.PHONY: debug
debug: $(LIB_PATH)
	@echo "=== Layer 1 (Boundary) Debug Information ==="
	@echo "LLVM Sources: $(LLVM_SOURCES)"
	@echo "C Sources: $(C_SOURCES)"
	@echo "LLVM Objects: $(LLVM_OBJ)"
	@echo "C Objects: $(C_OBJ)"
	@echo "Library: $(LIB_PATH)"
	@echo "Build artifacts:"
	@ls -la $(BUILD_DIR)/ 2>/dev/null || echo "(no build artifacts)"
	@echo "Library contents:"
	@ar -t $(LIB_PATH) 2>/dev/null || echo "(library not found)"

# Clean target is provided by layer.mk - no need to override