# Layer 3 Resonance Benchmark Suite Makefile
# (c) 2024-2025 UOR Foundation - MIT License
# 
# Builds comprehensive performance benchmarks for Atlas-12288 Layer 3 operations

# Configuration
CC := clang
LAYER_ROOT := ..
PROJECT_ROOT := $(LAYER_ROOT)/../..
LIB_DIR := $(PROJECT_ROOT)/lib
INCLUDE_DIRS := -I$(LAYER_ROOT)/include -I$(PROJECT_ROOT)/include

# Compiler flags
CFLAGS_BASE := -std=c11 -Wall -Wextra -Wno-unused-parameter
CFLAGS_DEBUG := $(CFLAGS_BASE) -O0 -g -DDEBUG
CFLAGS_RELEASE := $(CFLAGS_BASE) -O2 -DNDEBUG
CFLAGS_PERFORMANCE := $(CFLAGS_BASE) -O3 -march=native -mtune=native -DNDEBUG -flto

# SIMD and optimization flags
SIMD_FLAGS := -mavx2 -mfma -mbmi -mbmi2
MATH_FLAGS := -lm
THREAD_FLAGS := -pthread

# Link flags
LDFLAGS := -L$(LIB_DIR)
LIBS := -latlas-resonance -latlas-conservation

# Source files
CLASSIFICATION_SRC := bench-classification.c
CLUSTERING_SRC := bench-clustering.c

# Output directory
BUILD_DIR := build

# Targets
CLASSIFICATION_TARGET := $(BUILD_DIR)/bench-classification
CLUSTERING_TARGET := $(BUILD_DIR)/bench-clustering

# Optimization level targets
CLASSIFICATION_DEBUG := $(BUILD_DIR)/bench-classification-debug
CLASSIFICATION_RELEASE := $(BUILD_DIR)/bench-classification-release  
CLASSIFICATION_PERF := $(BUILD_DIR)/bench-classification-perf

CLUSTERING_DEBUG := $(BUILD_DIR)/bench-clustering-debug
CLUSTERING_RELEASE := $(BUILD_DIR)/bench-clustering-release
CLUSTERING_PERF := $(BUILD_DIR)/bench-clustering-perf

# Default target - build release versions
.DEFAULT_GOAL := all

.PHONY: all clean debug release performance run-all run-classification run-clustering \
        install check-deps help test-suite profile

# Build all benchmarks (release configuration)
all: check-deps $(CLASSIFICATION_TARGET) $(CLUSTERING_TARGET)

# Build debug versions
debug: check-deps $(CLASSIFICATION_DEBUG) $(CLUSTERING_DEBUG)

# Build release versions  
release: check-deps $(CLASSIFICATION_RELEASE) $(CLUSTERING_RELEASE)

# Build performance optimized versions
performance: check-deps $(CLASSIFICATION_PERF) $(CLUSTERING_PERF)

# Create build directory
$(BUILD_DIR):
	@echo "  [MKDIR] Creating build directory..."
	@mkdir -p $(BUILD_DIR)

# Check dependencies
check-deps:
	@echo "  [CHECK] Verifying Layer 3 library..."
	@if [ ! -f "$(LIB_DIR)/libatlas-resonance.a" ]; then \
		echo "  [ERROR] Layer 3 library not found. Run 'make' in $(LAYER_ROOT) first"; \
		exit 1; \
	fi
	@if [ ! -f "$(LIB_DIR)/libatlas-conservation.a" ]; then \
		echo "  [ERROR] Layer 2 library not found. Build Layer 2 first"; \
		exit 1; \
	fi

# Classification benchmark targets

# Default (release) classification benchmark
$(CLASSIFICATION_TARGET): $(BUILD_DIR) $(CLASSIFICATION_SRC)
	@echo "  [CC] Building classification benchmark (release)..."
	@$(CC) $(CFLAGS_RELEASE) $(SIMD_FLAGS) $(INCLUDE_DIRS) \
		$(CLASSIFICATION_SRC) $(LDFLAGS) $(LIBS) $(MATH_FLAGS) \
		-o $(CLASSIFICATION_TARGET)

# Debug classification benchmark
$(CLASSIFICATION_DEBUG): $(BUILD_DIR) $(CLASSIFICATION_SRC)
	@echo "  [CC] Building classification benchmark (debug)..."
	@$(CC) $(CFLAGS_DEBUG) $(INCLUDE_DIRS) \
		$(CLASSIFICATION_SRC) $(LDFLAGS) $(LIBS) $(MATH_FLAGS) \
		-o $(CLASSIFICATION_DEBUG)

# Release classification benchmark
$(CLASSIFICATION_RELEASE): $(BUILD_DIR) $(CLASSIFICATION_SRC)
	@echo "  [CC] Building classification benchmark (release)..."
	@$(CC) $(CFLAGS_RELEASE) $(SIMD_FLAGS) $(INCLUDE_DIRS) \
		$(CLASSIFICATION_SRC) $(LDFLAGS) $(LIBS) $(MATH_FLAGS) \
		-o $(CLASSIFICATION_RELEASE)

# Performance classification benchmark  
$(CLASSIFICATION_PERF): $(BUILD_DIR) $(CLASSIFICATION_SRC)
	@echo "  [CC] Building classification benchmark (performance)..."
	@$(CC) $(CFLAGS_PERFORMANCE) $(SIMD_FLAGS) $(INCLUDE_DIRS) \
		$(CLASSIFICATION_SRC) $(LDFLAGS) $(LIBS) $(MATH_FLAGS) \
		-o $(CLASSIFICATION_PERF)

# Clustering benchmark targets

# Default (release) clustering benchmark
$(CLUSTERING_TARGET): $(BUILD_DIR) $(CLUSTERING_SRC)
	@echo "  [CC] Building clustering benchmark (release)..."
	@$(CC) $(CFLAGS_RELEASE) $(INCLUDE_DIRS) \
		$(CLUSTERING_SRC) $(LDFLAGS) $(LIBS) $(MATH_FLAGS) \
		-o $(CLUSTERING_TARGET)

# Debug clustering benchmark
$(CLUSTERING_DEBUG): $(BUILD_DIR) $(CLUSTERING_SRC)
	@echo "  [CC] Building clustering benchmark (debug)..."
	@$(CC) $(CFLAGS_DEBUG) $(INCLUDE_DIRS) \
		$(CLUSTERING_SRC) $(LDFLAGS) $(LIBS) $(MATH_FLAGS) \
		-o $(CLUSTERING_DEBUG)

# Release clustering benchmark
$(CLUSTERING_RELEASE): $(BUILD_DIR) $(CLUSTERING_SRC)
	@echo "  [CC] Building clustering benchmark (release)..."
	@$(CC) $(CFLAGS_RELEASE) $(INCLUDE_DIRS) \
		$(CLUSTERING_SRC) $(LDFLAGS) $(LIBS) $(MATH_FLAGS) \
		-o $(CLUSTERING_RELEASE)

# Performance clustering benchmark
$(CLUSTERING_PERF): $(BUILD_DIR) $(CLUSTERING_SRC)
	@echo "  [CC] Building clustering benchmark (performance)..."
	@$(CC) $(CFLAGS_PERFORMANCE) $(INCLUDE_DIRS) \
		$(CLUSTERING_SRC) $(LDFLAGS) $(LIBS) $(MATH_FLAGS) \
		-o $(CLUSTERING_PERF)

# Run targets

# Run all benchmarks (release versions)
run-all: $(CLASSIFICATION_TARGET) $(CLUSTERING_TARGET)
	@echo ""
	@echo "==== Running Layer 3 Benchmark Suite ===="
	@echo ""
	@echo "=== Classification Benchmarks ==="
	@$(CLASSIFICATION_TARGET)
	@echo ""
	@echo "=== Clustering Benchmarks ==="
	@$(CLUSTERING_TARGET)
	@echo ""
	@echo "==== Benchmark Suite Complete ===="

# Run classification benchmarks only
run-classification: $(CLASSIFICATION_TARGET)
	@echo ""
	@echo "=== Layer 3 Classification Benchmarks ==="
	@$(CLASSIFICATION_TARGET)

# Run clustering benchmarks only  
run-clustering: $(CLUSTERING_TARGET)
	@echo ""
	@echo "=== Layer 3 Clustering Benchmarks ==="
	@$(CLUSTERING_TARGET)

# Run performance optimized versions
run-performance: $(CLASSIFICATION_PERF) $(CLUSTERING_PERF)
	@echo ""
	@echo "==== Running Layer 3 Performance Benchmark Suite ===="
	@echo ""
	@echo "=== Classification Benchmarks (Performance Build) ==="
	@$(CLASSIFICATION_PERF)
	@echo ""
	@echo "=== Clustering Benchmarks (Performance Build) ==="
	@$(CLUSTERING_PERF)
	@echo ""
	@echo "==== Performance Benchmark Suite Complete ===="

# Run comparison between different optimization levels
run-comparison: debug release performance
	@echo ""
	@echo "==== Layer 3 Optimization Level Comparison ===="
	@echo ""
	@echo "=== Debug Build Performance ==="
	@$(CLASSIFICATION_DEBUG) | head -20
	@echo ""
	@echo "=== Release Build Performance ==="  
	@$(CLASSIFICATION_RELEASE) | head -20
	@echo ""
	@echo "=== Performance Build Performance ==="
	@$(CLASSIFICATION_PERF) | head -20
	@echo ""
	@echo "==== Comparison Complete ===="

# Profile classification benchmark
profile: $(CLASSIFICATION_PERF)
	@echo "  [PROFILE] Running classification benchmark with perf..."
	@if command -v perf >/dev/null 2>&1; then \
		perf record -g $(CLASSIFICATION_PERF); \
		perf report; \
	else \
		echo "  [SKIP] perf not available for profiling"; \
	fi

# Test suite - run all benchmarks and collect results
test-suite: $(CLASSIFICATION_TARGET) $(CLUSTERING_TARGET)
	@echo ""
	@echo "==== Layer 3 Benchmark Test Suite ===="
	@mkdir -p results
	@echo ""
	@echo "=== Classification Tests ==="
	@$(CLASSIFICATION_TARGET) | tee results/classification-results.txt
	@echo ""
	@echo "=== Clustering Tests ==="
	@$(CLUSTERING_TARGET) | tee results/clustering-results.txt
	@echo ""
	@echo "Results saved to results/ directory"
	@echo "==== Test Suite Complete ===="

# Install benchmarks to system location
install: all
	@echo "  [INSTALL] Installing benchmarks..."
	@mkdir -p $(PROJECT_ROOT)/bin/benchmarks
	@cp $(CLASSIFICATION_TARGET) $(PROJECT_ROOT)/bin/benchmarks/
	@cp $(CLUSTERING_TARGET) $(PROJECT_ROOT)/bin/benchmarks/
	@echo "  [OK] Benchmarks installed to $(PROJECT_ROOT)/bin/benchmarks/"

# Clean build artifacts
clean:
	@echo "  [CLEAN] Removing build artifacts..."
	@rm -rf $(BUILD_DIR)
	@rm -rf results
	@echo "  [OK] Benchmark build directory cleaned"

# Help target
help:
	@echo "Layer 3 Resonance Benchmark Suite"
	@echo ""
	@echo "Available targets:"
	@echo "  all              - Build all benchmarks (release config)"
	@echo "  debug            - Build debug versions"
	@echo "  release          - Build release versions"
	@echo "  performance      - Build performance optimized versions"
	@echo ""
	@echo "  run-all          - Run all benchmarks"
	@echo "  run-classification - Run classification benchmarks only"
	@echo "  run-clustering   - Run clustering benchmarks only"
	@echo "  run-performance  - Run performance optimized versions"
	@echo "  run-comparison   - Compare optimization levels"
	@echo ""
	@echo "  test-suite       - Run comprehensive test suite"
	@echo "  profile          - Profile classification benchmark"
	@echo "  install          - Install benchmarks system-wide"
	@echo "  clean            - Clean build artifacts"
	@echo "  help             - Show this help message"
	@echo ""
	@echo "Build configurations:"
	@echo "  Debug:      -O0 -g (no optimizations, debug info)"
	@echo "  Release:    -O2 -mavx2 (optimized, SIMD enabled)"
	@echo "  Performance: -O3 -march=native -flto (max optimization)"
	@echo ""
	@echo "Dependencies:"
	@echo "  Requires Layer 3 (resonance) and Layer 2 (conservation) libraries"
	@echo "  Build with 'make' in parent directory first"

# Additional utility targets

# Quick performance test
quick-test: $(CLASSIFICATION_PERF)
	@echo "  [QUICK] Running quick performance test..."
	@$(CLASSIFICATION_PERF) | grep -E "(GB/s|MB/s|ops/s|PASS|FAIL)"

# Validate benchmark correctness
validate: debug
	@echo "  [VALIDATE] Running benchmark validation..."
	@$(CLASSIFICATION_DEBUG) >/dev/null 2>&1 && echo "  [OK] Classification benchmark runs successfully"
	@$(CLUSTERING_DEBUG) >/dev/null 2>&1 && echo "  [OK] Clustering benchmark runs successfully"

# Show compiler and system info
info:
	@echo "=== Build Configuration ==="
	@echo "Compiler: $(CC)"
	@echo "Base flags: $(CFLAGS_BASE)"
	@echo "SIMD flags: $(SIMD_FLAGS)"  
	@echo "Include paths: $(INCLUDE_DIRS)"
	@echo "Library paths: $(LDFLAGS)"
	@echo "Libraries: $(LIBS)"
	@echo ""
	@echo "=== System Information ==="
	@uname -a
	@$(CC) --version | head -1

# Check for memory leaks (if valgrind available)
memcheck: debug
	@echo "  [MEMCHECK] Checking for memory leaks..."
	@if command -v valgrind >/dev/null 2>&1; then \
		valgrind --leak-check=full --show-leak-kinds=all \
			$(CLASSIFICATION_DEBUG) >/dev/null 2>memcheck.log; \
		if grep -q "All heap blocks were freed" memcheck.log; then \
			echo "  [OK] No memory leaks detected"; \
		else \
			echo "  [WARN] Potential memory leaks - see memcheck.log"; \
		fi; \
	else \
		echo "  [SKIP] valgrind not available"; \
	fi