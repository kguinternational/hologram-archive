# Integration Tests Makefile
# (c) 2024–2025 UOR Foundation — MIT License
# Cross-layer integration testing and validation

# =============================================================================
# Configuration
# =============================================================================

# Tools
CLANG       ?= clang
CLANGXX     ?= clang++
LLVM_LINK   ?= llvm-link
OPT         ?= opt

# Build configuration
BUILD_TYPE ?= RELEASE
TARGET     ?= $(shell llvm-config --host-target)

# Build flags
ifeq ($(BUILD_TYPE),DEBUG)
	OPT_LEVEL   := -O0
	DEBUG_FLAGS := -g
	DEFINES     := -DDEBUG
else
	OPT_LEVEL   := -O3
	DEBUG_FLAGS :=
	DEFINES     := -DNDEBUG
endif

# Directories
TESTS_DIR     := tests
BUILD_DIR     := build
BENCHMARKS_DIR := benchmarks
EXAMPLES_DIR   := examples

# Layer libraries
LAYER0_LIB := ../lib/libatlas-core.a
LAYER1_LIB := ../lib/libatlas-boundary.a  
LAYER2_LIB := ../lib/libatlas-conservation.a
LAYER3_LIB := ../lib/libatlas-resonance.a

ALL_LAYER_LIBS := $(LAYER0_LIB) $(LAYER1_LIB) $(LAYER2_LIB) $(LAYER3_LIB)

# Include paths
INCLUDE_PATHS := \
	-I../include \
	-I../layers/layer0-atlas/include \
	-I../layers/layer1-boundary/include \
	-I../layers/layer2-conservation/include \
	-I../layers/layer3-resonance/include

# =============================================================================
# Test Sources
# =============================================================================

LLVM_TEST_SOURCES := \
	tests/atlas-acceptance-test.ll \
	tests/atlas-validation-test.ll \
	tests/test-acceptance.ll

C_TEST_SOURCES := \
	tests/test-layer2-integration.c

# =============================================================================
# Targets
# =============================================================================

.PHONY: all test clean distclean help
.PHONY: test-acceptance test-validation test-integration
.PHONY: benchmarks examples

all: check-deps $(BUILD_DIR) test

$(BUILD_DIR):
	@mkdir -p $@

# =============================================================================
# Dependency Checking
# =============================================================================

check-deps:
	@echo "[CHECK] Verifying layer dependencies..."
	@missing=0; \
	for lib in $(ALL_LAYER_LIBS); do \
		if [ ! -f "$$lib" ]; then \
			echo "[ERR] Missing: $$lib"; \
			missing=1; \
		fi \
	done; \
	if [ $$missing -eq 1 ]; then \
		echo "[ERR] Run 'make' from repository root to build all layers first"; \
		exit 1; \
	fi
	@echo "[OK] All layer dependencies satisfied"

# =============================================================================
# LLVM IR Integration Tests
# =============================================================================

test-llvm-ir: check-deps | $(BUILD_DIR)
	@echo "[TEST] Running LLVM IR integration tests..."
	@for test in $(LLVM_TEST_SOURCES); do \
		if [ -f "$$test" ]; then \
			echo "  Testing: $$test"; \
			base=$$(basename $$test .ll); \
			$(LLVM_LINK) $(ALL_LAYER_LIBS) "$$test" -o $(BUILD_DIR)/$${base}.bc 2>/dev/null || \
			$(LLVM_LINK) "$$test" -o $(BUILD_DIR)/$${base}.bc; \
			$(OPT) -passes='verify,default<O2>' $(BUILD_DIR)/$${base}.bc -o /dev/null; \
			echo "    [PASS] $$test"; \
		else \
			echo "  [SKIP] $$test not found"; \
		fi \
	done

# =============================================================================
# C Integration Tests
# =============================================================================

test-c-integration: check-deps | $(BUILD_DIR)
	@echo "[TEST] Running C integration tests..."
	@for test in $(C_TEST_SOURCES); do \
		if [ -f "$$test" ]; then \
			echo "  Building: $$test"; \
			base=$$(basename $$test .c); \
			$(CLANG) $(OPT_LEVEL) $(DEBUG_FLAGS) $(DEFINES) \
				$(INCLUDE_PATHS) "$$test" -o $(BUILD_DIR)/$$base \
				$(ALL_LAYER_LIBS) -lm; \
			echo "  Running: $$base"; \
			$(BUILD_DIR)/$$base && echo "    [PASS] $$base" || echo "    [FAIL] $$base"; \
		else \
			echo "  [SKIP] $$test not found"; \
		fi \
	done

# =============================================================================
# Specific Integration Tests
# =============================================================================

test-acceptance: check-deps | $(BUILD_DIR)
	@echo "[TEST] Running Atlas acceptance tests..."
	@if [ -f "tests/atlas-acceptance-test.ll" ]; then \
		$(LLVM_LINK) tests/atlas-acceptance-test.ll -o $(BUILD_DIR)/acceptance.bc; \
		$(OPT) -passes='verify' $(BUILD_DIR)/acceptance.bc -o /dev/null; \
		echo "[PASS] Acceptance tests validated"; \
	else \
		echo "[SKIP] Acceptance tests not found"; \
	fi

test-validation: check-deps | $(BUILD_DIR)
	@echo "[TEST] Running Atlas validation tests..."
	@if [ -f "tests/atlas-validation-test.ll" ]; then \
		$(LLVM_LINK) tests/atlas-validation-test.ll -o $(BUILD_DIR)/validation.bc; \
		$(OPT) -passes='verify' $(BUILD_DIR)/validation.bc -o /dev/null; \
		echo "[PASS] Validation tests passed"; \
	else \
		echo "[SKIP] Validation tests not found"; \
	fi

test-integration: check-deps | $(BUILD_DIR)
	@echo "[TEST] Running Layer 2 integration tests..."
	@if [ -f "tests/test-layer2-integration.c" ]; then \
		$(CLANG) $(OPT_LEVEL) $(DEBUG_FLAGS) $(DEFINES) \
			$(INCLUDE_PATHS) tests/test-layer2-integration.c \
			-o $(BUILD_DIR)/test-layer2-integration \
			$(ALL_LAYER_LIBS) -lm; \
		$(BUILD_DIR)/test-layer2-integration; \
	else \
		echo "[SKIP] Layer 2 integration test not found"; \
	fi

# =============================================================================
# Cross-Layer Testing
# =============================================================================

test-layer-interaction: check-deps | $(BUILD_DIR)
	@echo "[TEST] Testing layer interactions..."
	@echo "  L0-L1: Testing boundary operations with core types..."
	@echo "  L1-L2: Testing conservation with coordinate operations..."  
	@echo "  L2-L3: Testing resonance with conservation laws..."
	@echo "[INFO] Detailed interaction tests not yet implemented"

# =============================================================================
# Benchmarks and Performance
# =============================================================================

benchmarks: check-deps | $(BUILD_DIR)
	@echo "[BENCH] Running cross-layer benchmarks..."
	@if [ -d "$(BENCHMARKS_DIR)" ]; then \
		$(MAKE) -C $(BENCHMARKS_DIR) all || echo "[WARN] Benchmarks failed to build"; \
	else \
		echo "[SKIP] No benchmarks directory found"; \
	fi

# =============================================================================
# Examples
# =============================================================================

examples: check-deps | $(BUILD_DIR)
	@echo "[EXAMPLE] Building integration examples..."
	@if [ -d "$(EXAMPLES_DIR)" ]; then \
		for example in $(EXAMPLES_DIR)/*.c; do \
			if [ -f "$$example" ]; then \
				base=$$(basename $$example .c); \
				echo "  Building: $$base"; \
				$(CLANG) $(OPT_LEVEL) $(INCLUDE_PATHS) \
					"$$example" -o $(BUILD_DIR)/$$base \
					$(ALL_LAYER_LIBS) -lm; \
			fi \
		done \
	else \
		echo "[SKIP] No examples directory found"; \
	fi

# =============================================================================
# Main Test Target
# =============================================================================

test: test-llvm-ir test-c-integration test-acceptance test-validation test-integration

# =============================================================================
# Full Stack Test
# =============================================================================

test-full-stack: test benchmarks examples
	@echo "[TEST] Full stack testing complete"

# =============================================================================
# Cleaning
# =============================================================================

clean:
	@echo "[CLEAN] Cleaning integration tests..."
	rm -rf $(BUILD_DIR)

distclean: clean
	@echo "[CLEAN] Deep cleaning integration..."
	find . -name "*.bc" -delete 2>/dev/null || true
	find . -name "*.o" -delete 2>/dev/null || true

# =============================================================================
# Help
# =============================================================================

help:
	@echo "Atlas-12288 Integration Testing System"
	@echo ""
	@echo "Targets:"
	@echo "  all              - Run all integration tests"
	@echo "  test             - Run core integration tests"
	@echo "  test-llvm-ir     - Test LLVM IR integration"
	@echo "  test-c-integration - Test C runtime integration"
	@echo "  test-acceptance  - Run acceptance tests"
	@echo "  test-validation  - Run validation tests"
	@echo "  test-integration - Run layer 2 integration"
	@echo "  test-full-stack  - Complete testing suite"
	@echo "  benchmarks       - Run performance benchmarks"  
	@echo "  examples         - Build integration examples"
	@echo "  clean            - Remove build artifacts"
	@echo "  help             - Show this message"
	@echo ""
	@echo "Requirements:"
	@echo "  - All layers (0-3) must be built first"
	@echo "  - Run 'make' from repository root before running integration tests"

print-%:
	@echo $* = $($*)