# Makefile for Atlas-12288 Layer 2 Benchmark Suite
# (c) 2024-2025 UOR Foundation - MIT License

# Project configuration
PROJECT_NAME := atlas-layer2-bench
ATLAS_ROOT := ../llvm
BUILD_DIR := build
SRC_DIR := .

# Compiler configuration
CC := gcc
CXX := g++
CLANG := clang
CLANG_CXX := clang++

# Architecture detection
ARCH := $(shell uname -m)
OS := $(shell uname -s)

# Base flags
CFLAGS_BASE := -std=c11 -Wall -Wextra -Werror -pedantic
CXXFLAGS_BASE := -std=c++17 -Wall -Wextra -Werror -pedantic
LDFLAGS_BASE := -L$(ATLAS_ROOT)/lib -latlas -lm -lrt -lpthread

# Include paths
INCLUDES := -I$(ATLAS_ROOT)/include -I.

# Optimization levels
OPT_DEBUG := -O0 -g3 -DDEBUG
OPT_RELEASE := -O3 -DNDEBUG -flto
OPT_NATIVE := -O3 -DNDEBUG -march=native -mtune=native -flto

# SIMD instruction sets
SIMD_BASELINE := -msse2
SIMD_AVX2 := -mavx2 -mfma
SIMD_AVX512 := -mavx512f -mavx512bw -mavx512dq -mavx512vl
SIMD_NEON := -mfpu=neon # ARM NEON

# Architecture-specific flags
ifeq ($(ARCH),x86_64)
    ARCH_FLAGS := -m64
    # Always use AVX2 for compatibility
    SIMD_FLAGS := $(SIMD_AVX2)
else ifeq ($(ARCH),aarch64)
    ARCH_FLAGS := 
    SIMD_FLAGS := $(SIMD_NEON)
else ifeq ($(ARCH),arm64)
    ARCH_FLAGS := 
    SIMD_FLAGS := $(SIMD_NEON)
else
    ARCH_FLAGS := 
    SIMD_FLAGS := $(SIMD_BASELINE)
endif

# Compiler-specific optimizations
GCC_OPTS := -ffast-math -funroll-loops -fprefetch-loop-arrays
CLANG_OPTS := -ffast-math -funroll-loops

# Build variants
VARIANTS := debug release native baseline avx2 avx512

# Default target
.PHONY: all
all: $(BUILD_DIR)/$(PROJECT_NAME)-release $(BUILD_DIR)/$(PROJECT_NAME)-simple-release

# Create build directory
$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

# Debug build
.PHONY: debug
debug: $(BUILD_DIR)/$(PROJECT_NAME)-debug

$(BUILD_DIR)/$(PROJECT_NAME)-debug: layer2-bench.c | $(BUILD_DIR)
	$(CC) $(CFLAGS_BASE) $(OPT_DEBUG) $(INCLUDES) $(ARCH_FLAGS) \
		-o $@ $< $(LDFLAGS_BASE)

# Release build
.PHONY: release
release: $(BUILD_DIR)/$(PROJECT_NAME)-release

$(BUILD_DIR)/$(PROJECT_NAME)-release: layer2-bench.c | $(BUILD_DIR)
	$(CC) $(CFLAGS_BASE) $(OPT_RELEASE) $(INCLUDES) $(ARCH_FLAGS) $(SIMD_FLAGS) $(GCC_OPTS) \
		-o $@ $< $(LDFLAGS_BASE)

# Simple release build (self-contained benchmarks)
.PHONY: simple-release
simple-release: $(BUILD_DIR)/$(PROJECT_NAME)-simple-release

$(BUILD_DIR)/$(PROJECT_NAME)-simple-release: layer2-bench-simple.c | $(BUILD_DIR)
	$(CC) $(CFLAGS_BASE) $(OPT_RELEASE) $(INCLUDES) $(ARCH_FLAGS) $(SIMD_FLAGS) $(GCC_OPTS) \
		-o $@ $< -lm -lrt -lpthread

# Native optimized build
.PHONY: native
native: $(BUILD_DIR)/$(PROJECT_NAME)-native

$(BUILD_DIR)/$(PROJECT_NAME)-native: layer2-bench.c | $(BUILD_DIR)
	$(CC) $(CFLAGS_BASE) $(OPT_NATIVE) $(INCLUDES) $(ARCH_FLAGS) $(GCC_OPTS) \
		-o $@ $< $(LDFLAGS_BASE)

# Baseline SIMD build (SSE2 only)
.PHONY: baseline
baseline: $(BUILD_DIR)/$(PROJECT_NAME)-baseline

$(BUILD_DIR)/$(PROJECT_NAME)-baseline: layer2-bench.c | $(BUILD_DIR)
	$(CC) $(CFLAGS_BASE) $(OPT_RELEASE) $(INCLUDES) $(ARCH_FLAGS) $(SIMD_BASELINE) \
		-UAVX2 -o $@ $< $(LDFLAGS_BASE)

# AVX2 build
.PHONY: avx2
avx2: $(BUILD_DIR)/$(PROJECT_NAME)-avx2

$(BUILD_DIR)/$(PROJECT_NAME)-avx2: layer2-bench.c | $(BUILD_DIR)
	$(CC) $(CFLAGS_BASE) $(OPT_RELEASE) $(INCLUDES) $(ARCH_FLAGS) $(SIMD_AVX2) \
		-D__AVX2__ -o $@ $< $(LDFLAGS_BASE)

# AVX-512 build
.PHONY: avx512
avx512: $(BUILD_DIR)/$(PROJECT_NAME)-avx512

$(BUILD_DIR)/$(PROJECT_NAME)-avx512: layer2-bench.c | $(BUILD_DIR)
	$(CC) $(CFLAGS_BASE) $(OPT_RELEASE) $(INCLUDES) $(ARCH_FLAGS) $(SIMD_AVX512) \
		-D__AVX512F__ -D__AVX2__ -o $@ $< $(LDFLAGS_BASE)

# Clang builds
.PHONY: clang-release
clang-release: $(BUILD_DIR)/$(PROJECT_NAME)-clang-release

$(BUILD_DIR)/$(PROJECT_NAME)-clang-release: layer2-bench.c | $(BUILD_DIR)
	$(CLANG) $(CFLAGS_BASE) $(OPT_RELEASE) $(INCLUDES) $(ARCH_FLAGS) $(SIMD_FLAGS) $(CLANG_OPTS) \
		-o $@ $< $(LDFLAGS_BASE)

.PHONY: clang-native
clang-native: $(BUILD_DIR)/$(PROJECT_NAME)-clang-native

$(BUILD_DIR)/$(PROJECT_NAME)-clang-native: layer2-bench.c | $(BUILD_DIR)
	$(CLANG) $(CFLAGS_BASE) $(OPT_NATIVE) $(INCLUDES) $(ARCH_FLAGS) $(CLANG_OPTS) \
		-o $@ $< $(LDFLAGS_BASE)

# WASM build (requires Emscripten)
.PHONY: wasm
wasm: $(BUILD_DIR)/$(PROJECT_NAME).wasm

$(BUILD_DIR)/$(PROJECT_NAME).wasm: layer2-bench.c | $(BUILD_DIR)
	@if command -v emcc >/dev/null 2>&1; then \
		emcc $(CFLAGS_BASE) -O3 $(INCLUDES) -s WASM=1 -s EXPORTED_FUNCTIONS='["_main"]' \
			-o $@ $< $(ATLAS_ROOT)/lib/libatlas.a; \
	else \
		echo "Error: Emscripten (emcc) not found. Install Emscripten SDK for WASM builds."; \
		exit 1; \
	fi

# Profile-guided optimization build
.PHONY: pgo
pgo: $(BUILD_DIR)/$(PROJECT_NAME)-pgo

$(BUILD_DIR)/$(PROJECT_NAME)-pgo: layer2-bench.c | $(BUILD_DIR)
	# Generate profile
	$(CC) $(CFLAGS_BASE) $(OPT_RELEASE) $(INCLUDES) $(ARCH_FLAGS) $(SIMD_FLAGS) \
		-fprofile-generate -o $(BUILD_DIR)/$(PROJECT_NAME)-profile $< $(LDFLAGS_BASE)
	cd $(BUILD_DIR) && ./$(PROJECT_NAME)-profile > /dev/null
	# Use profile
	$(CC) $(CFLAGS_BASE) $(OPT_RELEASE) $(INCLUDES) $(ARCH_FLAGS) $(SIMD_FLAGS) $(GCC_OPTS) \
		-fprofile-use -o $@ $< $(LDFLAGS_BASE)
	rm -f $(BUILD_DIR)/$(PROJECT_NAME)-profile *.gcda

# Cross-compilation targets
.PHONY: cross-arm64
cross-arm64: $(BUILD_DIR)/$(PROJECT_NAME)-arm64

$(BUILD_DIR)/$(PROJECT_NAME)-arm64: layer2-bench.c | $(BUILD_DIR)
	aarch64-linux-gnu-gcc $(CFLAGS_BASE) $(OPT_RELEASE) $(INCLUDES) $(SIMD_NEON) \
		-o $@ $< -L$(ATLAS_ROOT)/lib/arm64 -latlas -lm -lrt -lpthread

# Test runners
.PHONY: test
test: $(BUILD_DIR)/$(PROJECT_NAME)-simple-release
	@echo "Running Layer 2 benchmark suite (simplified)..."
	cd $(BUILD_DIR) && ./$(PROJECT_NAME)-simple-release

.PHONY: test-full
test-full: $(BUILD_DIR)/$(PROJECT_NAME)-release
	@echo "Running Layer 2 benchmark suite (full)..."
	cd $(BUILD_DIR) && ./$(PROJECT_NAME)-release

.PHONY: test-all
test-all: release baseline avx2
	@echo "Running comprehensive benchmark comparison..."
	@echo "=== Baseline (SSE2) ==="
	cd $(BUILD_DIR) && ./$(PROJECT_NAME)-baseline
	@echo ""
	@echo "=== AVX2 Optimized ==="
	cd $(BUILD_DIR) && ./$(PROJECT_NAME)-avx2
	@echo ""
	@echo "=== Release (Auto-detect) ==="
	cd $(BUILD_DIR) && ./$(PROJECT_NAME)-release

.PHONY: benchmark
benchmark: native
	@echo "Running performance benchmark with native optimizations..."
	cd $(BUILD_DIR) && ./$(PROJECT_NAME)-native > benchmark-results-$(shell date +%Y%m%d-%H%M%S).txt
	@echo "Results saved to build/benchmark-results-*.txt"

# Performance comparison
.PHONY: compare
compare: baseline avx2
	@echo "Performance comparison: Baseline vs AVX2"
	@echo "========================================"
	@echo "Running baseline (SSE2)..."
	cd $(BUILD_DIR) && timeout 60 ./$(PROJECT_NAME)-baseline | grep -E "(memcpy|memset|Witness|Delta)" > baseline.tmp
	@echo "Running AVX2 optimized..."
	cd $(BUILD_DIR) && timeout 60 ./$(PROJECT_NAME)-avx2 | grep -E "(memcpy|memset|Witness|Delta)" > avx2.tmp
	@echo ""
	@echo "Results comparison:"
	@paste $(BUILD_DIR)/baseline.tmp $(BUILD_DIR)/avx2.tmp
	@rm -f $(BUILD_DIR)/baseline.tmp $(BUILD_DIR)/avx2.tmp

# Static analysis
.PHONY: analyze
analyze: layer2-bench.c
	@echo "Running static analysis..."
	@if command -v clang-tidy >/dev/null 2>&1; then \
		clang-tidy $< -- $(CFLAGS_BASE) $(INCLUDES); \
	fi
	@if command -v cppcheck >/dev/null 2>&1; then \
		cppcheck --enable=all --suppress=missingIncludeSystem $<; \
	fi

# Memory analysis
.PHONY: memcheck
memcheck: $(BUILD_DIR)/$(PROJECT_NAME)-debug
	@if command -v valgrind >/dev/null 2>&1; then \
		cd $(BUILD_DIR) && valgrind --tool=memcheck --leak-check=full --show-leak-kinds=all \
			./$(PROJECT_NAME)-debug; \
	else \
		echo "Valgrind not available for memory checking"; \
	fi

# Performance profiling
.PHONY: profile
profile: $(BUILD_DIR)/$(PROJECT_NAME)-release
	@if command -v perf >/dev/null 2>&1; then \
		cd $(BUILD_DIR) && perf record -g ./$(PROJECT_NAME)-release; \
		perf report; \
	else \
		echo "perf not available for profiling"; \
	fi

# Clean targets
.PHONY: clean
clean:
	rm -rf $(BUILD_DIR)

.PHONY: distclean
distclean: clean
	rm -f *.gcda *.gcno perf.data*

# Help
.PHONY: help
help:
	@echo "Atlas-12288 Layer 2 Benchmark Suite Build System"
	@echo "================================================"
	@echo ""
	@echo "Build targets:"
	@echo "  all          - Build default release version"
	@echo "  debug        - Debug build with symbols"
	@echo "  release      - Optimized release build"
	@echo "  native       - Native architecture optimizations"
	@echo "  baseline     - SSE2-only build"
	@echo "  avx2         - AVX2 optimized build"
	@echo "  avx512       - AVX-512 optimized build"
	@echo "  clang-*      - Clang compiler variants"
	@echo "  wasm         - WebAssembly build (requires Emscripten)"
	@echo "  pgo          - Profile-guided optimization"
	@echo "  cross-arm64  - Cross-compile for ARM64"
	@echo ""
	@echo "Test targets:"
	@echo "  test         - Run basic benchmark suite"
	@echo "  test-all     - Compare all build variants"
	@echo "  benchmark    - Full performance benchmark"
	@echo "  compare      - Compare baseline vs optimized"
	@echo ""
	@echo "Analysis targets:"
	@echo "  analyze      - Static code analysis"
	@echo "  memcheck     - Memory leak detection"
	@echo "  profile      - Performance profiling"
	@echo ""
	@echo "Maintenance:"
	@echo "  clean        - Remove build artifacts"
	@echo "  distclean    - Remove all generated files"
	@echo "  help         - Show this help message"
	@echo ""
	@echo "Environment variables:"
	@echo "  CC           - C compiler (default: gcc)"
	@echo "  CLANG        - Clang compiler (default: clang)"
	@echo "  ATLAS_ROOT   - Atlas library root (default: ../llvm)"

# Build information
.PHONY: info
info:
	@echo "Build Configuration:"
	@echo "==================="
	@echo "Architecture: $(ARCH)"
	@echo "Operating System: $(OS)"
	@echo "C Compiler: $(CC)"
	@echo "SIMD Flags: $(SIMD_FLAGS)"
	@echo "Arch Flags: $(ARCH_FLAGS)"
	@echo "Atlas Root: $(ATLAS_ROOT)"
	@echo ""