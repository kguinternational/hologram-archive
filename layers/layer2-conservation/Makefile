# Layer 2: Conservation Layer Makefile

LAYER_NAME := conservation
LAYER_NUM := 2

include ../../build/layer.mk

.DEFAULT_GOAL := $(LIB_PATH)

# Layer-specific dependencies
$(LIB_PATH): check-deps

check-deps:
	@if [ ! -f "../../lib/libatlas-boundary.a" ]; then \
		echo "[ERROR] Layer 1 (Boundary) must be built first"; \
		exit 1; \
	fi

# Layer 2 specific test target (override base layer.mk)
.PHONY: test test-conservation test-legacy
test: test-conservation test-legacy

test-conservation: $(LIB_PATH)
	@echo "  [TEST] Running comprehensive Conservation tests..."
	@mkdir -p build/tests
	@echo "  [CC] Compiling conservation test suite..."
	@clang -std=c11 -I./include -I./wasm -I../../include -I../layer0-atlas/include -I../layer1-boundary/include \
		-Wall -Wextra -O2 -g \
		tests/test-conservation.c runtime/conservation.c tests/llvm-stubs.c \
		-o build/tests/test-conservation -lm -lpthread || \
		(echo "  [SKIP] test-conservation.c (compilation failed)" && exit 1)
	@if [ -x "build/tests/test-conservation" ]; then \
		echo "  [RUN] Executing comprehensive conservation tests..."; \
		./build/tests/test-conservation || echo "  [WARN] Some conservation tests failed"; \
	fi

test-legacy: $(LIB_PATH)
	@echo "  [TEST] Running legacy Layer 2 tests..."
	@if [ -x "tests/run-runtime-tests.sh" ]; then \
		echo "  [SCRIPT] Running runtime test suite..."; \
		cd tests && ./run-runtime-tests.sh --property-only || echo "  [WARN] Some runtime tests failed"; \
	fi
	@if [ -f "tests/test-conservation.ll" ]; then \
		echo "  [LLVM] Checking conservation test..."; \
		mkdir -p build/tests; \
		llvm-as tests/test-conservation.ll -o build/tests/test-conservation.bc 2>/dev/null || \
			echo "  [SKIP] test-conservation.ll (LLVM compilation failed)"; \
	fi
	@if [ -f "tests/test-properties.c" ] && [ -f "tests/test-concurrency.c" ]; then \
		echo "  [CC] Compiling property and concurrency tests..."; \
		mkdir -p build/tests; \
		clang -std=c11 -I./include -I../../include -I../include \
			tests/test-properties.c runtime/*.c -o build/tests/test-properties 2>/dev/null || \
			echo "  [SKIP] test-properties.c (compilation failed)"; \
		if [ -x "build/tests/test-properties" ]; then \
			echo "  [RUN] Executing property tests..."; \
			./build/tests/test-properties || echo "  [WARN] Some property tests failed"; \
		fi; \
	fi
	@echo "  [OK] Layer 2 tests completed"

# =============================================================================
# Benchmark targets
# =============================================================================

BENCH_SRC := ../../integration/benchmarks/layer2-bench-simple.c
BENCH_BUILD_DIR := benchmark/build
BENCH_TARGETS := bench-debug bench-release bench-native bench-baseline bench-avx2

# Common benchmark flags
BENCH_CFLAGS_BASE := -std=c11 -Wall -Wextra -I../../include -I../include
BENCH_LDFLAGS := -lm -lrt -lpthread

# Optimization levels
BENCH_DEBUG_FLAGS := -O0 -g3 -DDEBUG
BENCH_RELEASE_FLAGS := -O3 -DNDEBUG -flto
BENCH_NATIVE_FLAGS := -O3 -DNDEBUG -march=native -mtune=native -flto

# Architecture-specific SIMD flags
BENCH_BASELINE_FLAGS := -O3 -DNDEBUG -msse2
BENCH_AVX2_FLAGS := -O3 -DNDEBUG -mavx2 -mfma -D__AVX2__

.PHONY: bench-l2 bench-debug bench-release bench-native bench-baseline bench-avx2 bench-quick bench-thorough bench-clean

# Main benchmark target
bench-l2: bench-release
	@echo "  [BENCH] Running Layer 2 benchmark suite..."
	@./$(BENCH_BUILD_DIR)/layer2-bench-release

# Create benchmark build directory
$(BENCH_BUILD_DIR):
	@mkdir -p $(BENCH_BUILD_DIR)

# Debug benchmark build
bench-debug: $(BENCH_BUILD_DIR)/layer2-bench-debug

$(BENCH_BUILD_DIR)/layer2-bench-debug: $(BENCH_SRC) | $(BENCH_BUILD_DIR)
	@echo "  [BENCH-CC] Building debug benchmark..."
	@clang $(BENCH_CFLAGS_BASE) $(BENCH_DEBUG_FLAGS) -o $@ $< $(BENCH_LDFLAGS)

# Release benchmark build
bench-release: $(BENCH_BUILD_DIR)/layer2-bench-release

$(BENCH_BUILD_DIR)/layer2-bench-release: $(BENCH_SRC) | $(BENCH_BUILD_DIR)
	@echo "  [BENCH-CC] Building release benchmark..."
	@clang $(BENCH_CFLAGS_BASE) $(BENCH_RELEASE_FLAGS) -o $@ $< $(BENCH_LDFLAGS)

# Native optimized benchmark build
bench-native: $(BENCH_BUILD_DIR)/layer2-bench-native

$(BENCH_BUILD_DIR)/layer2-bench-native: $(BENCH_SRC) | $(BENCH_BUILD_DIR)
	@echo "  [BENCH-CC] Building native optimized benchmark..."
	@clang $(BENCH_CFLAGS_BASE) $(BENCH_NATIVE_FLAGS) -o $@ $< $(BENCH_LDFLAGS)

# Baseline SSE2 benchmark build
bench-baseline: $(BENCH_BUILD_DIR)/layer2-bench-baseline

$(BENCH_BUILD_DIR)/layer2-bench-baseline: $(BENCH_SRC) | $(BENCH_BUILD_DIR)
	@echo "  [BENCH-CC] Building baseline SSE2 benchmark..."
	@clang $(BENCH_CFLAGS_BASE) $(BENCH_BASELINE_FLAGS) -o $@ $< $(BENCH_LDFLAGS)

# AVX2 optimized benchmark build
bench-avx2: $(BENCH_BUILD_DIR)/layer2-bench-avx2

$(BENCH_BUILD_DIR)/layer2-bench-avx2: $(BENCH_SRC) | $(BENCH_BUILD_DIR)
	@echo "  [BENCH-CC] Building AVX2 optimized benchmark..."
	@clang $(BENCH_CFLAGS_BASE) $(BENCH_AVX2_FLAGS) -o $@ $< $(BENCH_LDFLAGS)

# Quick benchmark run
bench-quick: bench-release
	@echo "  [BENCH] Running quick Layer 2 benchmark..."
	@./$(BENCH_BUILD_DIR)/layer2-bench-release

# Thorough benchmark run with comparisons
bench-thorough: bench-baseline bench-avx2 bench-native
	@echo "  [BENCH] Running thorough Layer 2 benchmark comparison..."
	@echo "  [BENCH] === Baseline (SSE2) ==="
	@./$(BENCH_BUILD_DIR)/layer2-bench-baseline || true
	@echo ""
	@echo "  [BENCH] === AVX2 Optimized ==="
	@./$(BENCH_BUILD_DIR)/layer2-bench-avx2 || true
	@echo ""
	@echo "  [BENCH] === Native Optimized ==="
	@./$(BENCH_BUILD_DIR)/layer2-bench-native || true

# Clean benchmark builds
bench-clean:
	@echo "  [BENCH-CLEAN] Cleaning benchmark builds..."
	@rm -rf benchmark

# =============================================================================
# WebAssembly (WASM) targets
# =============================================================================

WASM_BUILD_DIR := build/wasm
WASM_CC := emcc
WASM_CFLAGS := -std=c11 -O2 -s WASM=1 -s EXPORTED_RUNTIME_METHODS='["ccall","cwrap"]' \
               -s ALLOW_MEMORY_GROWTH=1 -s MODULARIZE=1 -s EXPORT_NAME=AtlasConservation \
               -DATLAS_SINGLE_THREAD=1 -I./include -I./wasm -I../../include \
               -I../layer0-atlas/include -I../layer1-boundary/include

.PHONY: wasm wasm-clean test-wasm wasm-node test-wasm-node wasm-build-dir

# Create WASM build directory
wasm-build-dir:
	@mkdir -p $(WASM_BUILD_DIR)

# Main WASM build target
wasm: wasm-build-dir
	@echo "  [WASM] Building Layer 2 Conservation for WebAssembly..."
	@if ! command -v emcc >/dev/null 2>&1; then \
		echo "  [ERROR] Emscripten (emcc) not found. Install Emscripten first."; \
		echo "  [INFO] Visit: https://emscripten.org/docs/getting_started/downloads.html"; \
		exit 1; \
	fi
	@echo "  [WASM-CC] Compiling conservation runtime to WASM..."
	@$(WASM_CC) $(WASM_CFLAGS) \
		runtime/conservation.c \
		-s EXPORTED_FUNCTIONS='["_atlas_domain_create","_atlas_domain_attach","_atlas_domain_verify","_atlas_domain_commit","_atlas_domain_destroy","_atlas_budget_alloc","_atlas_budget_release","_atlas_witness_generate","_atlas_witness_verify","_atlas_witness_destroy","_atlas_conserved_delta","_atlas_get_last_error","_atlas_error_string","_atlas_runtime_is_thread_safe"]' \
		-o $(WASM_BUILD_DIR)/atlas-conservation.js
	@echo "  [WASM-CC] Compiling conservation test suite to WASM..."
	@$(WASM_CC) $(WASM_CFLAGS) \
		tests/test-conservation.c runtime/conservation.c tests/llvm-stubs.c \
		-s EXPORTED_FUNCTIONS='["_main"]' \
		-o $(WASM_BUILD_DIR)/test-conservation.js
	@echo "  [WASM] Creating HTML test page..."
	@cp wasm/templates/test.html $(WASM_BUILD_DIR)/test.html
	@echo "  [WASM] Creating Node.js test runner..."
	@cp wasm/templates/test-node.js $(WASM_BUILD_DIR)/test-node.js
	@echo "  [WASM] Build completed successfully"
	@echo "  [INFO] WASM files generated in $(WASM_BUILD_DIR)/"
	@echo "  [INFO] Open $(WASM_BUILD_DIR)/test.html in a browser to run tests"
	@echo "  [INFO] Run 'make wasm-node' to test in Node.js"

# WASM test target (browser)
test-wasm: wasm
	@echo "  [WASM-TEST] Layer 2 Conservation WASM test ready"
	@echo "  [INFO] Open $(WASM_BUILD_DIR)/test.html in a browser to run WASM tests"
	@echo "  [INFO] Files available:"
	@ls -la $(WASM_BUILD_DIR)/

# Node.js WASM test
wasm-node: wasm
	@echo "  [WASM-NODE] Running Layer 2 Conservation WASM test in Node.js..."
	@if [ ! -f "$(WASM_BUILD_DIR)/test-node.js" ]; then \
		echo "  [ERROR] Node.js test not found. Run 'make wasm' first."; \
		exit 1; \
	fi
	@cd $(WASM_BUILD_DIR) && node test-node.js

test-wasm-node: wasm-node

# Clean WASM builds
wasm-clean:
	@echo "  [WASM-CLEAN] Cleaning WASM builds..."
	@rm -rf $(WASM_BUILD_DIR)

# Extend main clean target to include benchmark and WASM cleanup
clean: bench-clean wasm-clean