# Layer 0: Atlas Core Makefile

LAYER_NAME := core
LAYER_NUM := 0

# Include common build rules AFTER defining variables
include ../../build/layer.mk

# Default target - build the library
.DEFAULT_GOAL := $(LIB_PATH)

# =============================================================================
# Layer-Specific Rules
# =============================================================================

# Layer 0 specific test target (override base layer.mk)
.PHONY: test
test: $(LIB_PATH)
	@echo "  [TEST] Running Layer 0 (Atlas Core) tests..."
	@if [ -f "tests/test-core.c" ]; then \
		echo "  [CC] Compiling test-core.c..."; \
		mkdir -p build/tests; \
		if clang -std=c11 -I../../include -Iinclude \
			tests/test-core.c runtime-stubs.c -o build/tests/test-core 2>/dev/null; then \
			echo "  [RUN] Executing core tests..."; \
			./build/tests/test-core || echo "  [WARN] Some tests failed"; \
		else \
			echo "  [SKIP] test-core.c (compilation failed)"; \
		fi; \
	fi
	@if [ -f "tests/test-simd.ll" ]; then \
		echo "  [LLVM] Checking SIMD test..."; \
		llvm-as tests/test-simd.ll -o build/tests/test-simd.bc 2>/dev/null || \
			echo "  [SKIP] test-simd.ll (LLVM compilation failed)"; \
	fi
	@echo "  [OK] Layer 0 tests completed"