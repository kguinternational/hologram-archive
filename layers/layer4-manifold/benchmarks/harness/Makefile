# Makefile for Atlas Layer 4 Benchmark Harness
# (c) 2024-2025 UOR Foundation - MIT License

# =============================================================================
# Configuration
# =============================================================================

# Build configuration
BUILD_TYPE ?= DEBUG
CC ?= clang
CFLAGS_DEBUG = -O0 -g3 -DDEBUG
CFLAGS_RELEASE = -O3 -DNDEBUG
CFLAGS_COMMON = -std=c11 -Wall -Wextra -Wpedantic -Wno-format-truncation -fPIC

# Select build-specific flags
ifeq ($(BUILD_TYPE),RELEASE)
    CFLAGS = $(CFLAGS_COMMON) $(CFLAGS_RELEASE)
else
    CFLAGS = $(CFLAGS_COMMON) $(CFLAGS_DEBUG)
endif

# Directories
HARNESS_DIR = .
BUILD_DIR = $(HARNESS_DIR)/build
OBJ_DIR = $(BUILD_DIR)/obj
BIN_DIR = $(BUILD_DIR)/bin
LAYER4_DIR = ../..
PROJECT_ROOT = $(LAYER4_DIR)/../..
LIB_DIR = $(PROJECT_ROOT)/lib

# Include paths
INCLUDES = -I$(HARNESS_DIR) \
           -I$(LAYER4_DIR)/include \
           -I$(PROJECT_ROOT)/include \
           -I$(PROJECT_ROOT)/layers/layer0-atlas/include \
           -I$(PROJECT_ROOT)/layers/layer1-boundary/include \
           -I$(PROJECT_ROOT)/layers/layer2-conservation/include \
           -I$(PROJECT_ROOT)/layers/layer3-resonance/include

# Libraries and linking
LIBS = -L$(LIB_DIR) -latlas-manifold -latlas-manifold-rs \
       -L$(LIB_DIR) -latlas-resonance \
       -L$(LIB_DIR) -latlas-conservation \
       -L$(LIB_DIR) -latlas-boundary \
       -L$(LIB_DIR) -latlas-core \
       -lm -lpthread -ldl

# Source files
FRAMEWORK_SOURCES = benchmark_framework.c
UTILS_SOURCES = benchmark_utils.c  
RUNNER_SOURCES = benchmark_runner.c

# Object files
FRAMEWORK_OBJECTS = $(FRAMEWORK_SOURCES:%.c=$(OBJ_DIR)/%.o)
UTILS_OBJECTS = $(UTILS_SOURCES:%.c=$(OBJ_DIR)/%.o)
RUNNER_OBJECTS = $(RUNNER_SOURCES:%.c=$(OBJ_DIR)/%.o)

ALL_OBJECTS = $(FRAMEWORK_OBJECTS) $(UTILS_OBJECTS) $(RUNNER_OBJECTS)

# Target executables
BENCHMARK_RUNNER = $(BIN_DIR)/benchmark_runner

# Library targets
BENCHMARK_LIB = $(BUILD_DIR)/libbenchmark-harness.a

# =============================================================================
# Default Target
# =============================================================================

.PHONY: all
all: $(BENCHMARK_RUNNER) $(BENCHMARK_LIB)

# =============================================================================
# Directory Creation
# =============================================================================

$(BUILD_DIR):
	@mkdir -p $(BUILD_DIR)

$(OBJ_DIR): | $(BUILD_DIR)
	@mkdir -p $(OBJ_DIR)

$(BIN_DIR): | $(BUILD_DIR)
	@mkdir -p $(BIN_DIR)

# =============================================================================
# Object File Rules
# =============================================================================

$(OBJ_DIR)/%.o: %.c | $(OBJ_DIR)
	@echo "CC $<"
	@$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@

# =============================================================================
# Library Rules
# =============================================================================

$(BENCHMARK_LIB): $(FRAMEWORK_OBJECTS) $(UTILS_OBJECTS) | $(BUILD_DIR)
	@echo "AR $@"
	@ar rcs $@ $^
	@echo "RANLIB $@"
	@ranlib $@

# =============================================================================
# Executable Rules
# =============================================================================

# Build Layer 4 dependencies first
$(LAYER4_LIB):
	@echo "Building Layer 4 dependencies..."
	@$(MAKE) -C $(LAYER4_DIR) BUILD_TYPE=$(BUILD_TYPE)

$(LAYER2_LIB):
	@echo "Building Layer 2 dependencies..."
	@$(MAKE) -C $(LAYER2_DIR) BUILD_TYPE=$(BUILD_TYPE)

$(BENCHMARK_RUNNER): $(RUNNER_OBJECTS) $(BENCHMARK_LIB) $(LAYER4_LIB) $(LAYER2_LIB) | $(BIN_DIR)
	@echo "LINK $@"
	@$(CC) $(CFLAGS) -o $@ $^ $(LIBS)

# =============================================================================
# Build Targets
# =============================================================================

.PHONY: debug release
debug:
	@$(MAKE) BUILD_TYPE=DEBUG

release:
	@$(MAKE) BUILD_TYPE=RELEASE

# =============================================================================
# Test Targets  
# =============================================================================

.PHONY: test test-quick test-verbose
test: $(BENCHMARK_RUNNER)
	@echo "Running benchmark harness tests..."
	@$(BENCHMARK_RUNNER) --iterations 100 --warmup 10 --csv --json --text

test-quick: $(BENCHMARK_RUNNER)
	@echo "Running quick benchmark tests..."
	@$(BENCHMARK_RUNNER) --iterations 10 --warmup 2

test-verbose: $(BENCHMARK_RUNNER)
	@echo "Running verbose benchmark tests..."
	@$(BENCHMARK_RUNNER) --iterations 100 --warmup 10 --verbose --csv --json --text

# =============================================================================
# Benchmark Execution Targets
# =============================================================================

.PHONY: bench bench-release bench-debug bench-full
bench: $(BENCHMARK_RUNNER)
	@echo "Running standard benchmarks..."
	@$(BENCHMARK_RUNNER) --iterations 1000 --warmup 100 --csv --json

bench-release:
	@$(MAKE) BUILD_TYPE=RELEASE bench

bench-debug:
	@$(MAKE) BUILD_TYPE=DEBUG bench

bench-full: $(BENCHMARK_RUNNER)
	@echo "Running comprehensive benchmarks..."
	@$(BENCHMARK_RUNNER) --iterations 10000 --warmup 1000 --verbose --csv --json --text

# =============================================================================
# Analysis Targets
# =============================================================================

.PHONY: benchmark-report generate-baseline compare-baseline
benchmark-report: $(BENCHMARK_RUNNER)
	@echo "Generating benchmark report..."
	@$(BENCHMARK_RUNNER) --verbose --csv --json --text --output-dir benchmark_results

generate-baseline: $(BENCHMARK_RUNNER)
	@echo "Generating performance baseline..."
	@$(BENCHMARK_RUNNER) --generate-baseline --iterations 5000 --warmup 500 --json

compare-baseline: $(BENCHMARK_RUNNER)
	@echo "Comparing against baseline..."
	@$(BENCHMARK_RUNNER) --compare --iterations 1000 --warmup 100 --threshold 1.05

# =============================================================================
# Installation Targets
# =============================================================================

.PHONY: install install-bin install-lib
PREFIX ?= /usr/local
INSTALL_BIN_DIR = $(PREFIX)/bin
INSTALL_LIB_DIR = $(PREFIX)/lib  
INSTALL_INC_DIR = $(PREFIX)/include/atlas

install: install-bin install-lib

install-bin: $(BENCHMARK_RUNNER)
	@echo "Installing benchmark runner..."
	@mkdir -p $(INSTALL_BIN_DIR)
	@cp $(BENCHMARK_RUNNER) $(INSTALL_BIN_DIR)/atlas-benchmark-runner
	@chmod +x $(INSTALL_BIN_DIR)/atlas-benchmark-runner

install-lib: $(BENCHMARK_LIB)
	@echo "Installing benchmark library..."
	@mkdir -p $(INSTALL_LIB_DIR)
	@mkdir -p $(INSTALL_INC_DIR)
	@cp $(BENCHMARK_LIB) $(INSTALL_LIB_DIR)/
	@cp benchmark_framework.h $(INSTALL_INC_DIR)/
	@cp benchmark_utils.h $(INSTALL_INC_DIR)/

# =============================================================================
# Quality Assurance Targets
# =============================================================================

.PHONY: lint format check-format
lint: $(ALL_OBJECTS)
	@echo "Linting benchmark harness code..."
	@if command -v clang-tidy >/dev/null 2>&1; then \
		clang-tidy *.c -- $(CFLAGS) $(INCLUDES); \
	else \
		echo "clang-tidy not found, skipping lint check"; \
	fi

format:
	@echo "Formatting benchmark harness code..."
	@if command -v clang-format >/dev/null 2>&1; then \
		clang-format -i *.c *.h; \
	else \
		echo "clang-format not found, skipping format"; \
	fi

check-format:
	@echo "Checking code format..."
	@if command -v clang-format >/dev/null 2>&1; then \
		if ! clang-format --dry-run --Werror *.c *.h >/dev/null 2>&1; then \
			echo "Code formatting issues found. Run 'make format' to fix."; \
			exit 1; \
		else \
			echo "Code formatting is correct."; \
		fi; \
	else \
		echo "clang-format not found, skipping format check"; \
	fi

# =============================================================================
# Memory and Performance Analysis
# =============================================================================

.PHONY: valgrind benchmark-profile
valgrind: $(BENCHMARK_RUNNER)
	@echo "Running benchmark runner under Valgrind..."
	@valgrind --tool=memcheck --leak-check=full --show-leak-kinds=all \
		--track-origins=yes --verbose \
		$(BENCHMARK_RUNNER) --iterations 10 --warmup 2

benchmark-profile: $(BENCHMARK_RUNNER)
	@echo "Profiling benchmark runner..."
	@if command -v perf >/dev/null 2>&1; then \
		perf record -g $(BENCHMARK_RUNNER) --iterations 1000 --warmup 100; \
		perf report; \
	else \
		echo "perf not found, skipping profiling"; \
	fi

# =============================================================================
# Documentation Targets
# =============================================================================

.PHONY: docs clean-docs
docs:
	@echo "Generating documentation..."
	@if command -v doxygen >/dev/null 2>&1; then \
		doxygen Doxyfile 2>/dev/null || echo "Doxygen config not found"; \
	else \
		echo "Doxygen not found, skipping documentation"; \
	fi

clean-docs:
	@echo "Cleaning documentation..."
	@rm -rf docs/

# =============================================================================
# Clean Targets
# =============================================================================

.PHONY: clean clean-build clean-results distclean
clean:
	@echo "Cleaning benchmark harness build files..."
	@rm -rf $(BUILD_DIR)

clean-build: clean

clean-results:
	@echo "Cleaning benchmark results..."
	@rm -rf benchmark_results/
	@rm -f *.csv *.json *.txt
	@rm -f baseline.json

distclean: clean clean-results clean-docs
	@echo "Deep cleaning benchmark harness..."
	@rm -f core core.* *.core
	@rm -f perf.data perf.data.old

# =============================================================================
# Help Target
# =============================================================================

.PHONY: help
help:
	@echo "Atlas Layer 4 Benchmark Harness Makefile"
	@echo "========================================"
	@echo ""
	@echo "Build Targets:"
	@echo "  all                 Build benchmark runner and library (default)"
	@echo "  debug               Build with debug configuration"
	@echo "  release             Build with release configuration"
	@echo ""
	@echo "Test Targets:"
	@echo "  test                Run standard tests"
	@echo "  test-quick          Run quick tests (few iterations)"
	@echo "  test-verbose        Run tests with verbose output"
	@echo ""
	@echo "Benchmark Targets:"
	@echo "  bench               Run standard benchmarks"
	@echo "  bench-release       Run benchmarks with release build"
	@echo "  bench-debug         Run benchmarks with debug build" 
	@echo "  bench-full          Run comprehensive benchmarks"
	@echo ""
	@echo "Analysis Targets:"
	@echo "  benchmark-report    Generate complete benchmark report"
	@echo "  generate-baseline   Generate performance baseline"
	@echo "  compare-baseline    Compare against baseline"
	@echo ""
	@echo "Quality Assurance:"
	@echo "  lint                Run static analysis"
	@echo "  format              Format source code"
	@echo "  check-format        Check code formatting"
	@echo "  valgrind            Run memory analysis"
	@echo "  benchmark-profile   Profile performance"
	@echo ""
	@echo "Installation:"
	@echo "  install             Install benchmark tools"
	@echo "  install-bin         Install benchmark runner only"
	@echo "  install-lib         Install benchmark library only"
	@echo ""
	@echo "Clean Targets:"
	@echo "  clean               Clean build files"
	@echo "  clean-results       Clean benchmark results"
	@echo "  distclean           Deep clean everything"
	@echo ""
	@echo "Configuration:"
	@echo "  BUILD_TYPE=DEBUG|RELEASE  Set build configuration"
	@echo "  CC=compiler               Set C compiler"
	@echo "  PREFIX=path               Set install prefix"

# =============================================================================
# Dependency Handling
# =============================================================================

# Include dependency files if they exist
-include $(ALL_OBJECTS:.o=.d)

# Generate dependencies
$(OBJ_DIR)/%.d: %.c | $(OBJ_DIR)
	@$(CC) $(CFLAGS) $(INCLUDES) -MM -MT $(OBJ_DIR)/$*.o $< -MF $@