# Atlas Layer 4 Benchmarks Makefile
# (c) 2024-2025 UOR Foundation - MIT License
#
# Builds and runs Layer 4 manifold benchmarks with conservation verification

# Directories
LAYER4_DIR := ..
PROJECT_ROOT := ../../..
RESULTS_DIR := results

CC = gcc
CFLAGS = -std=c11 -Wall -Wextra -O3 -march=native -fPIC
DEBUG_FLAGS = -g -O0 -DDEBUG
INCLUDES = -I$(LAYER4_DIR)/include \
           -I$(PROJECT_ROOT)/include \
           -I$(PROJECT_ROOT)/layers/layer0-atlas/include \
           -I$(PROJECT_ROOT)/layers/layer1-boundary/include \
           -I$(PROJECT_ROOT)/layers/layer2-conservation/include \
           -I$(PROJECT_ROOT)/layers/layer3-resonance/include \
           -Iharness
           
LIBS = -L$(PROJECT_ROOT)/lib -latlas-manifold -latlas-manifold-rs \
       -L$(PROJECT_ROOT)/lib -latlas-resonance \
       -L$(PROJECT_ROOT)/lib -latlas-conservation \
       -L$(PROJECT_ROOT)/lib -latlas-boundary \
       -L$(PROJECT_ROOT)/lib -latlas-core \
       -lm -lpthread -ldl

BUILD_DIR = build
STANDALONE_DIR = standalone

# Benchmark executables
STANDALONE_BENCHMARKS = run_layer4_benchmarks test_conservation
CORE_BENCHMARKS = projection_benchmarks shard_benchmarks
GEOMETRIC_BENCHMARKS = transform_benchmarks manifold_benchmarks
SIGNAL_BENCHMARKS = resonance_benchmarks
APPLICATION_BENCHMARKS = computational_benchmarks
TRADITIONAL_BENCHMARKS = algorithm_benchmarks

# All benchmarks
ALL_BENCHMARKS = $(STANDALONE_BENCHMARKS) $(CORE_BENCHMARKS) $(GEOMETRIC_BENCHMARKS) \
                 $(SIGNAL_BENCHMARKS) $(APPLICATION_BENCHMARKS) $(TRADITIONAL_BENCHMARKS)

# Source files
HARNESS_SRC = harness/conservation_verify.c harness/benchmark_stubs.c
STANDALONE_SRC = $(addprefix $(STANDALONE_DIR)/, $(addsuffix .c, $(STANDALONE_BENCHMARKS)))
CORE_SRC = $(addprefix core/, $(addsuffix .c, $(CORE_BENCHMARKS)))
GEOMETRIC_SRC = $(addprefix geometric/, $(addsuffix .c, $(GEOMETRIC_BENCHMARKS)))
SIGNAL_SRC = $(addprefix signal/, $(addsuffix .c, $(SIGNAL_BENCHMARKS)))
APPLICATION_SRC = $(addprefix applications/, $(addsuffix .c, $(APPLICATION_BENCHMARKS)))
TRADITIONAL_SRC = $(addprefix traditional/, $(addsuffix .c, $(TRADITIONAL_BENCHMARKS)))

# Build targets
STANDALONE_TARGETS = $(addprefix $(BUILD_DIR)/, $(STANDALONE_BENCHMARKS))
CORE_TARGETS = $(addprefix $(BUILD_DIR)/, $(CORE_BENCHMARKS))
GEOMETRIC_TARGETS = $(addprefix $(BUILD_DIR)/, $(GEOMETRIC_BENCHMARKS))
SIGNAL_TARGETS = $(addprefix $(BUILD_DIR)/, $(SIGNAL_BENCHMARKS))
APPLICATION_TARGETS = $(addprefix $(BUILD_DIR)/, $(APPLICATION_BENCHMARKS))
TRADITIONAL_TARGETS = $(addprefix $(BUILD_DIR)/, $(TRADITIONAL_BENCHMARKS))

ALL_TARGETS = $(STANDALONE_TARGETS) $(CORE_TARGETS) $(GEOMETRIC_TARGETS) \
              $(SIGNAL_TARGETS) $(APPLICATION_TARGETS) $(TRADITIONAL_TARGETS)

.PHONY: all clean run run-standalone run-core run-all help dirs results

# Default target
all: dirs $(ALL_TARGETS)
	@echo "✓ All benchmarks built successfully"

# Create necessary directories
dirs:
	@mkdir -p $(BUILD_DIR)
	@mkdir -p $(RESULTS_DIR)
	@mkdir -p $(RESULTS_DIR)/standalone
	@mkdir -p $(RESULTS_DIR)/core
	@mkdir -p $(RESULTS_DIR)/geometric
	@mkdir -p $(RESULTS_DIR)/signal
	@mkdir -p $(RESULTS_DIR)/applications
	@mkdir -p $(RESULTS_DIR)/traditional

# Help target
help:
	@echo "Atlas Layer 4 Benchmarks"
	@echo "========================="
	@echo ""
	@echo "Build targets:"
	@echo "  all              - Build all benchmarks"
	@echo "  standalone       - Build standalone benchmarks"
	@echo "  core             - Build core operation benchmarks"
	@echo "  geometric        - Build geometric transformation benchmarks"
	@echo "  signal           - Build signal processing benchmarks"
	@echo "  applications     - Build application-level benchmarks"
	@echo "  traditional      - Build traditional comparison benchmarks"
	@echo ""
	@echo "Run targets:"
	@echo "  run              - Run main benchmark suite"
	@echo "  run-standalone   - Run standalone benchmarks"
	@echo "  run-core         - Run core benchmarks"
	@echo "  run-all          - Run all benchmarks"
	@echo "  results          - Display latest results"
	@echo ""
	@echo "Other targets:"
	@echo "  clean            - Clean build artifacts"
	@echo "  help             - Show this help message"

# Build harness object file
$(BUILD_DIR)/conservation_verify.o: $(HARNESS_SRC) | dirs
	@echo "Building conservation harness..."
	@$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@

# Build standalone benchmarks
$(BUILD_DIR)/run_layer4_benchmarks: $(STANDALONE_DIR)/run_layer4_benchmarks.c | dirs
	@echo "Building $@..."
	@$(CC) $(CFLAGS) $(INCLUDES) $< -o $@ $(LIBS)

$(BUILD_DIR)/test_conservation: $(STANDALONE_DIR)/test_conservation.c | dirs
	@echo "Building $@..."
	@$(CC) $(CFLAGS) $(INCLUDES) $< -o $@ $(LIBS)

standalone: dirs $(STANDALONE_TARGETS)
	@echo "✓ Standalone benchmarks built"

# Build core benchmarks
$(BUILD_DIR)/%: core/%.c $(BUILD_DIR)/conservation_verify.o | dirs
	@echo "Building $@..."
	@$(CC) $(CFLAGS) $(INCLUDES) $< $(BUILD_DIR)/conservation_verify.o -o $@ $(LIBS)

core: dirs $(BUILD_DIR)/conservation_verify.o $(CORE_TARGETS)
	@echo "✓ Core benchmarks built"

# Build geometric benchmarks
$(BUILD_DIR)/%: geometric/%.c $(BUILD_DIR)/conservation_verify.o | dirs
	@echo "Building $@..."
	@$(CC) $(CFLAGS) $(INCLUDES) $< $(BUILD_DIR)/conservation_verify.o -o $@ $(LIBS)

geometric: dirs $(BUILD_DIR)/conservation_verify.o $(GEOMETRIC_TARGETS)
	@echo "✓ Geometric benchmarks built"

# Build signal benchmarks
$(BUILD_DIR)/%: signal/%.c $(BUILD_DIR)/conservation_verify.o | dirs
	@echo "Building $@..."
	@$(CC) $(CFLAGS) $(INCLUDES) $< $(BUILD_DIR)/conservation_verify.o -o $@ $(LIBS)

signal: dirs $(BUILD_DIR)/conservation_verify.o $(SIGNAL_TARGETS)
	@echo "✓ Signal benchmarks built"

# Build application benchmarks
$(BUILD_DIR)/%: applications/%.c $(BUILD_DIR)/conservation_verify.o | dirs
	@echo "Building $@..."
	@$(CC) $(CFLAGS) $(INCLUDES) $< $(BUILD_DIR)/conservation_verify.o -o $@ $(LIBS)

applications: dirs $(BUILD_DIR)/conservation_verify.o $(APPLICATION_TARGETS)
	@echo "✓ Application benchmarks built"

# Build traditional benchmarks
$(BUILD_DIR)/%: traditional/%.c $(BUILD_DIR)/conservation_verify.o | dirs
	@echo "Building $@..."
	@$(CC) $(CFLAGS) $(INCLUDES) $< $(BUILD_DIR)/conservation_verify.o -o $@ $(LIBS)

traditional: dirs $(BUILD_DIR)/conservation_verify.o $(TRADITIONAL_TARGETS)
	@echo "✓ Traditional benchmarks built"

# Run main benchmark suite
run: $(BUILD_DIR)/run_layer4_benchmarks
	@echo ""
	@echo "========================================="
	@echo "    Running Atlas Layer 4 Benchmarks    "
	@echo "========================================="
	@echo ""
	@$(BUILD_DIR)/run_layer4_benchmarks | tee $(RESULTS_DIR)/standalone/benchmark_main_$(shell date +%Y%m%d_%H%M%S).txt
	@echo ""
	@echo "Results saved to $(RESULTS_DIR)/standalone/"

# Run standalone benchmarks
run-standalone: $(STANDALONE_TARGETS)
	@echo "Running standalone benchmarks..."
	@for bench in $(STANDALONE_BENCHMARKS); do \
		echo ""; \
		echo "Running $$bench..."; \
		$(BUILD_DIR)/$$bench | tee $(RESULTS_DIR)/standalone/$$bench_$(shell date +%Y%m%d_%H%M%S).txt; \
	done

# Run core benchmarks
run-core: $(CORE_TARGETS)
	@echo "Running core benchmarks..."
	@for bench in $(CORE_BENCHMARKS); do \
		if [ -f $(BUILD_DIR)/$$bench ]; then \
			echo ""; \
			echo "Running $$bench..."; \
			$(BUILD_DIR)/$$bench | tee $(RESULTS_DIR)/core/$$bench_$(shell date +%Y%m%d_%H%M%S).txt; \
		fi; \
	done

# Run all benchmarks
run-all: all
	@echo ""
	@echo "========================================="
	@echo "  Running Complete Benchmark Suite      "
	@echo "========================================="
	@$(MAKE) run-standalone
	@$(MAKE) run-core
	@echo ""
	@echo "✓ All benchmarks completed. Results in $(RESULTS_DIR)/"

# Display latest results
results:
	@echo ""
	@echo "Latest Benchmark Results:"
	@echo "========================"
	@if [ -d $(RESULTS_DIR)/standalone ]; then \
		latest=$$(ls -t $(RESULTS_DIR)/standalone/benchmark_main_*.txt 2>/dev/null | head -1); \
		if [ -n "$$latest" ]; then \
			cat "$$latest"; \
		else \
			echo "No results found. Run 'make run' first."; \
		fi; \
	else \
		echo "No results directory found. Run benchmarks first."; \
	fi

# Clean build artifacts
clean:
	@echo "Cleaning build artifacts..."
	@rm -rf $(BUILD_DIR)
	@rm -f $(ALL_TARGETS)
	@echo "✓ Clean complete"

# Clean everything including results
distclean: clean
	@echo "Cleaning results..."
	@rm -rf $(RESULTS_DIR)
	@echo "✓ Deep clean complete"

.PHONY: standalone

# Debug build
debug: CFLAGS += $(DEBUG_FLAGS)
debug: all
	@echo "✓ Debug build complete"

# =============================================================================
# Build Rules for Benchmark Executables
# =============================================================================

# Core benchmarks
$(BUILD_DIR)/shard_benchmarks: $(CORE_SRC) $(HARNESS_SRC) | dirs
	@echo "Building build/shard_benchmarks..."
	$(CC) $(CFLAGS) $(INCLUDES) -o $@ core/shard_benchmarks.c $(HARNESS_SRC) $(LIBS)

$(BUILD_DIR)/projection_benchmarks: $(CORE_SRC) $(HARNESS_SRC) | dirs
	@echo "Building build/projection_benchmarks..."
	$(CC) $(CFLAGS) $(INCLUDES) -o $@ core/projection_benchmarks.c $(HARNESS_SRC) $(LIBS)

# Geometric benchmarks
$(BUILD_DIR)/transform_benchmarks: $(GEOMETRIC_SRC) $(HARNESS_SRC) | dirs
	@echo "Building build/transform_benchmarks..."
	$(CC) $(CFLAGS) $(INCLUDES) -o $@ geometric/transform_benchmarks.c $(HARNESS_SRC) $(LIBS)

$(BUILD_DIR)/manifold_benchmarks: $(GEOMETRIC_SRC) $(HARNESS_SRC) | dirs
	@echo "Building build/manifold_benchmarks..."
	$(CC) $(CFLAGS) $(INCLUDES) -o $@ geometric/manifold_benchmarks.c $(HARNESS_SRC) $(LIBS)

# Signal benchmarks
$(BUILD_DIR)/resonance_benchmarks: $(SIGNAL_SRC) $(HARNESS_SRC) | dirs
	@echo "Building build/resonance_benchmarks..."
	$(CC) $(CFLAGS) $(INCLUDES) -o $@ signal/resonance_benchmarks.c $(HARNESS_SRC) $(LIBS)

# Application benchmarks
$(BUILD_DIR)/computational_benchmarks: $(APPLICATION_SRC) $(HARNESS_SRC) | dirs
	@echo "Building build/computational_benchmarks..."
	$(CC) $(CFLAGS) $(INCLUDES) -o $@ applications/computational_benchmarks.c $(HARNESS_SRC) $(LIBS)

# Traditional benchmarks
$(BUILD_DIR)/algorithm_benchmarks: $(TRADITIONAL_SRC) $(HARNESS_SRC) | dirs
	@echo "Building build/algorithm_benchmarks..."
	$(CC) $(CFLAGS) $(INCLUDES) -o $@ traditional/algorithm_benchmarks.c $(HARNESS_SRC) $(LIBS)

# Standalone benchmarks
$(BUILD_DIR)/run_layer4_benchmarks: $(STANDALONE_SRC) $(HARNESS_SRC) | dirs
	@echo "Building build/run_layer4_benchmarks..."
	$(CC) $(CFLAGS) $(INCLUDES) -o $@ standalone/run_layer4_benchmarks.c $(HARNESS_SRC) $(LIBS)

$(BUILD_DIR)/test_conservation: $(STANDALONE_SRC) $(HARNESS_SRC) | dirs
	@echo "Building build/test_conservation..."
	$(CC) $(CFLAGS) $(INCLUDES) -o $@ standalone/test_conservation.c $(HARNESS_SRC) $(LIBS)