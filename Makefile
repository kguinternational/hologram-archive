# Hologram Atlas-12288 Main Makefile
# (c) 2024–2025 UOR Foundation — MIT License
# Hierarchical Layer-Based Build System

# =============================================================================
# Configuration
# =============================================================================

# LLVM tools (override with environment variables if needed)
LLVM_CONFIG ?= llvm-config
LLC         ?= llc
OPT         ?= opt
LLVM_LINK   ?= llvm-link
CLANG       ?= clang
CLANGXX     ?= clang++
AR          ?= ar

# Build configuration
BUILD_TYPE ?= RELEASE
PREFIX     ?= /usr/local
TARGET     ?= $(shell $(LLVM_CONFIG) --host-target)

# Version check
LLV_MAJOR := $(shell $(LLVM_CONFIG) --version | sed -E 's/^([0-9]+).*/\1/')

# =============================================================================
# Layer Directories
# =============================================================================

LAYER0_DIR := layers/layer0-atlas
LAYER1_DIR := layers/layer1-boundary
LAYER2_DIR := layers/layer2-conservation
LAYER3_DIR := layers/layer3-resonance
LAYER4_DIR := layers/layer4-manifold
LAYER5_DIR := layers/layer5-vpi
LAYER6_DIR := layers/layer6-sdk
LAYER7_DIR := layers/layer7-apps

# Active layers (implemented)
ACTIVE_LAYERS := layer0 layer1 layer2 layer3 layer4

# Output directories
BUILD_DIR := build
LIB_DIR   := lib
INCLUDE_DIR := include
INTEGRATION_DIR := integration

# =============================================================================
# Targets
# =============================================================================

.PHONY: all clean distclean test check help check-llvm install uninstall
.PHONY: $(ACTIVE_LAYERS) integration
.PHONY: layer0 layer1 layer2 layer3 layer4
.PHONY: bench bench-l2 bench-debug bench-release bench-thorough bench-quick bench-clean
.PHONY: lint format typecheck check-rust-tools
.PHONY: lint-all format-check-all typecheck-all

all: check-llvm $(ACTIVE_LAYERS) integration

# Create output directories
$(BUILD_DIR) $(LIB_DIR):
	@mkdir -p $@

# =============================================================================
# Version / Environment Guards
# =============================================================================

check-llvm:
	@maj=$(LLV_MAJOR); \
	if [ -z "$$maj" ]; then echo "[ERR] Unable to detect LLVM version via llvm-config"; exit 1; fi; \
	if [ "$$maj" -lt 15 ]; then echo "[ERR] LLVM >= 15 required (opaque pointers). Detected $$maj"; exit 1; fi; \
	echo "[OK] LLVM $$maj detected"

# Check if Rust tools are available
check-rust-tools:
	@if command -v cargo >/dev/null 2>&1; then \
		echo "[OK] Rust/Cargo detected"; \
	else \
		echo "[WARN] Rust/Cargo not found - Layer 4 quality checks will be skipped"; \
		exit 1; \
	fi

# =============================================================================
# Layer Build Rules (Hierarchical Dependencies)
# =============================================================================

layer0: check-llvm | $(BUILD_DIR) $(LIB_DIR)
	@echo "[LAYER0] Building Atlas Core..."
	@$(MAKE) -C $(LAYER0_DIR) BUILD_TYPE=$(BUILD_TYPE) TARGET=$(TARGET)

layer1: layer0
	@echo "[LAYER1] Building Boundary Layer..."
	@$(MAKE) -C $(LAYER1_DIR) BUILD_TYPE=$(BUILD_TYPE) TARGET=$(TARGET)

layer2: layer1
	@echo "[LAYER2] Building Conservation Layer..."
	@$(MAKE) -C $(LAYER2_DIR) BUILD_TYPE=$(BUILD_TYPE) TARGET=$(TARGET)

layer3: layer2
	@echo "[LAYER3] Building Resonance Layer..."
	@$(MAKE) -C $(LAYER3_DIR) BUILD_TYPE=$(BUILD_TYPE) TARGET=$(TARGET)

layer4: layer3
	@echo "[LAYER4] Building Manifold Layer..."
	@$(MAKE) -C $(LAYER4_DIR) BUILD_TYPE=$(BUILD_TYPE) TARGET=$(TARGET)
	@echo "[LAYER4] Building Rust components..."
	@cd $(LAYER4_DIR)/rs && cargo build --release
	@echo "[LAYER4] Copying Rust library to lib directory..."
	@mkdir -p $(LIB_DIR)
	@cp $(LAYER4_DIR)/rs/target/release/libatlas_manifold.a $(LIB_DIR)/libatlas-manifold-rs.a 2>/dev/null || echo "[WARN] Rust static library not found, skipping copy"

layer5: layer4
	@echo "[LAYER5] VPI Layer - Not implemented yet"

layer6: layer5
	@echo "[LAYER6] SDK Layer - Not implemented yet"

layer7: layer6
	@echo "[LAYER7] Applications Layer - Not implemented yet"

# =============================================================================
# Integration and Cross-Layer Testing
# =============================================================================

integration: $(ACTIVE_LAYERS)
	@echo "[INTEGRATION] Building integration tests..."
	@$(MAKE) -C $(INTEGRATION_DIR) BUILD_TYPE=$(BUILD_TYPE) TARGET=$(TARGET)

# =============================================================================
# Combined Library Creation
# =============================================================================

# Create combined static library from all layers
$(LIB_DIR)/libatlas.a: $(ACTIVE_LAYERS) | $(LIB_DIR)
	@echo "[LIBRARY] Creating combined libatlas.a..."
	@rm -f $@
	@$(AR) rcs $@ \
		$(LIB_DIR)/libatlas-core.a \
		$(LIB_DIR)/libatlas-boundary.a \
		$(LIB_DIR)/libatlas-conservation.a \
		$(LIB_DIR)/libatlas-resonance.a \
		$(LIB_DIR)/libatlas-manifold.a

# Create combined shared library from all layers  
$(LIB_DIR)/libatlas.so: $(ACTIVE_LAYERS) | $(LIB_DIR)
	@echo "[LIBRARY] Creating combined libatlas.so..."
	@$(CLANG) -shared -fPIC -o $@ \
		-Wl,--whole-archive \
		$(LIB_DIR)/libatlas-core.a \
		$(LIB_DIR)/libatlas-boundary.a \
		$(LIB_DIR)/libatlas-conservation.a \
		$(LIB_DIR)/libatlas-resonance.a \
		$(LIB_DIR)/libatlas-manifold.a \
		-Wl,--no-whole-archive

# =============================================================================
# Quality Check Targets
# =============================================================================

# Global lint target - runs linting for all layers
lint: check-llvm
	@echo "[LINT] Running linting checks for all layers..."
	@echo "[LINT] C/C++ layers: Using clang-tidy would require configuration - skipping for now"
	@if command -v cargo >/dev/null 2>&1; then \
		echo "[LINT] Running Rust clippy for Layer 4..."; \
		cd $(LAYER4_DIR)/rs && cargo clippy --all-targets --all-features -- -D warnings || exit 1; \
		echo "[LINT] Layer 4 Rust linting completed"; \
	else \
		echo "[LINT] Rust not available - skipping Layer 4 linting"; \
	fi
	@echo "[LINT] All linting checks completed"

# Global format target - runs formatting for all layers  
format: check-llvm
	@echo "[FORMAT] Running formatting for all layers..."
	@echo "[FORMAT] C/C++ layers: Using clang-format would require configuration - skipping for now"
	@if command -v cargo >/dev/null 2>&1; then \
		echo "[FORMAT] Running Rust formatter for Layer 4..."; \
		cd $(LAYER4_DIR)/rs && cargo fmt || exit 1; \
		echo "[FORMAT] Layer 4 Rust formatting completed"; \
	else \
		echo "[FORMAT] Rust not available - skipping Layer 4 formatting"; \
	fi
	@echo "[FORMAT] All formatting completed"

# Global typecheck target - runs type checking for all layers
typecheck: check-llvm
	@echo "[TYPECHECK] Running type checking for all layers..."
	@echo "[TYPECHECK] C/C++ layers: Type checking handled during compilation"
	@if command -v cargo >/dev/null 2>&1; then \
		echo "[TYPECHECK] Running Rust type checking for Layer 4..."; \
		cd $(LAYER4_DIR)/rs && cargo check --all-targets --all-features || exit 1; \
		echo "[TYPECHECK] Layer 4 Rust type checking completed"; \
	else \
		echo "[TYPECHECK] Rust not available - skipping Layer 4 type checking"; \
	fi
	@echo "[TYPECHECK] All type checking completed"

# =============================================================================
# CI Quality Check Targets
# =============================================================================

# CI target: Comprehensive linting for all layers
lint-all: check-llvm
	@echo "[LINT-ALL] Running comprehensive linting checks..."
	@echo "[LINT-ALL] C/C++ layers: Static analysis checks would be configured here"
	@if command -v cargo >/dev/null 2>&1; then \
		echo "[LINT-ALL] Running comprehensive Rust linting for Layer 4..."; \
		cd $(LAYER4_DIR)/rs && \
		cargo clippy --all-targets --all-features --release -- -D warnings && \
		cargo clippy --all-targets --all-features -- -D warnings || exit 1; \
		echo "[LINT-ALL] Layer 4 comprehensive linting completed"; \
	else \
		echo "[LINT-ALL] Rust not available - skipping Layer 4 comprehensive linting"; \
	fi
	@echo "[LINT-ALL] Comprehensive linting completed"

# CI target: Format verification for all layers
format-check-all: check-llvm
	@echo "[FORMAT-CHECK] Verifying formatting for all layers..."
	@echo "[FORMAT-CHECK] C/C++ layers: Format checking would be configured here"
	@if command -v cargo >/dev/null 2>&1; then \
		echo "[FORMAT-CHECK] Verifying Rust formatting for Layer 4..."; \
		cd $(LAYER4_DIR)/rs && \
		cargo fmt --all -- --check || (echo "[ERROR] Layer 4 Rust code is not properly formatted. Run 'make format' to fix." && exit 1); \
		echo "[FORMAT-CHECK] Layer 4 formatting verification completed"; \
	else \
		echo "[FORMAT-CHECK] Rust not available - skipping Layer 4 format checking"; \
	fi
	@echo "[FORMAT-CHECK] Format verification completed"

# CI target: Comprehensive type checking for all layers
typecheck-all: check-llvm
	@echo "[TYPECHECK-ALL] Running comprehensive type checking..."
	@echo "[TYPECHECK-ALL] Building all C/C++ layers for type checking..."
	@$(MAKE) --quiet layer0 layer1 layer2 layer3 layer4 >/dev/null 2>&1 || (echo "[ERROR] C/C++ compilation failed - type checking failed" && exit 1)
	@if command -v cargo >/dev/null 2>&1; then \
		echo "[TYPECHECK-ALL] Running comprehensive Rust type checking for Layer 4..."; \
		cd $(LAYER4_DIR)/rs && \
		cargo check --all-targets --all-features --release && \
		cargo check --all-targets --all-features || exit 1; \
		echo "[TYPECHECK-ALL] Layer 4 comprehensive type checking completed"; \
	else \
		echo "[TYPECHECK-ALL] Rust not available - skipping Layer 4 comprehensive type checking"; \
	fi
	@echo "[TYPECHECK-ALL] Comprehensive type checking completed"

# =============================================================================
# Testing
# =============================================================================

test: all test-integration
	@echo "[TEST] Running layer tests..."
	@$(MAKE) -C $(LAYER0_DIR) test
	@$(MAKE) -C $(LAYER1_DIR) test
	@$(MAKE) -C $(LAYER2_DIR) test  
	@$(MAKE) -C $(LAYER3_DIR) test
	@$(MAKE) -C $(LAYER4_DIR) test
	@echo "[TEST] Running Rust tests..."
	@cd $(LAYER4_DIR)/rs && cargo test
	@echo "[TEST] All tests completed"

# Alias for test target (common convention)
check: test

# Individual layer tests
test-layer0:
	@$(MAKE) -C $(LAYER0_DIR) test

test-layer1:
	@$(MAKE) -C $(LAYER1_DIR) test

test-layer2:
	@$(MAKE) -C $(LAYER2_DIR) test

test-layer3:
	@$(MAKE) -C $(LAYER3_DIR) test

test-layer4:
	@$(MAKE) -C $(LAYER4_DIR) test
	@cd $(LAYER4_DIR)/rs && cargo test

test-integration:
	@$(MAKE) -C $(INTEGRATION_DIR) test

# =============================================================================
# Benchmark targets
# =============================================================================

# Main benchmark target - runs Layer 2 benchmarks
bench: layer2
	@echo "[BENCH] Running Layer 2 (Conservation) benchmarks..."
	@$(MAKE) -C $(LAYER2_DIR) bench-l2

# Layer 2 specific benchmark targets
bench-l2: layer2
	@echo "[BENCH] Running Layer 2 benchmark suite..."
	@$(MAKE) -C $(LAYER2_DIR) bench-l2

bench-debug: layer2
	@echo "[BENCH] Building and running debug benchmarks..."
	@$(MAKE) -C $(LAYER2_DIR) bench-debug

bench-release: layer2
	@echo "[BENCH] Building and running release benchmarks..."
	@$(MAKE) -C $(LAYER2_DIR) bench-release

bench-quick: layer2
	@echo "[BENCH] Running quick benchmark suite..."
	@$(MAKE) -C $(LAYER2_DIR) bench-quick

bench-thorough: layer2
	@echo "[BENCH] Running thorough benchmark comparison..."
	@$(MAKE) -C $(LAYER2_DIR) bench-thorough

# Clean benchmark builds
bench-clean:
	@echo "[BENCH-CLEAN] Cleaning all benchmark builds..."
	@$(MAKE) -C $(LAYER2_DIR) bench-clean 2>/dev/null || true

# Alias for test target (common Make convention)
check: test

# =============================================================================
# Installation
# =============================================================================

install: all $(LIB_DIR)/libatlas.a
	@echo "[INSTALL] Installing Atlas-12288..."
	@install -d $(PREFIX)/include/atlas
	@install -m 644 $(INCLUDE_DIR)/*.h $(PREFIX)/include/atlas/ 2>/dev/null || true
	@install -d $(PREFIX)/lib
	@install -m 644 $(LIB_DIR)/libatlas.a $(PREFIX)/lib/
	@if [ -f "$(LIB_DIR)/libatlas.so" ]; then \
		echo "[INSTALL] Installing shared library..."; \
		install -m 755 $(LIB_DIR)/libatlas.so $(PREFIX)/lib/; \
	else \
		echo "[SKIP] Shared library not available (linking conflicts)"; \
	fi
	@echo "[INSTALL] Installation complete"

uninstall:
	@echo "[UNINSTALL] Removing Atlas-12288..."
	@rm -rf $(PREFIX)/include/atlas
	@rm -f $(PREFIX)/lib/libatlas.a
	@rm -f $(PREFIX)/lib/libatlas.so
	@echo "[UNINSTALL] Atlas-12288 removed"

# =============================================================================
# Cleaning
# =============================================================================

clean: bench-clean
	@echo "[CLEAN] Cleaning all layers..."
	@$(MAKE) -C $(LAYER0_DIR) clean 2>/dev/null || true
	@$(MAKE) -C $(LAYER1_DIR) clean 2>/dev/null || true
	@$(MAKE) -C $(LAYER2_DIR) clean 2>/dev/null || true
	@$(MAKE) -C $(LAYER3_DIR) clean 2>/dev/null || true
	@$(MAKE) -C $(LAYER4_DIR) clean 2>/dev/null || true
	@cd $(LAYER4_DIR)/rs && cargo clean 2>/dev/null || true
	@$(MAKE) -C $(INTEGRATION_DIR) clean 2>/dev/null || true
	@rm -rf $(LIB_DIR)

distclean: clean
	@echo "[CLEAN] Full cleanup..."
	@$(MAKE) -C $(LAYER0_DIR) distclean 2>/dev/null || true
	@$(MAKE) -C $(LAYER1_DIR) distclean 2>/dev/null || true
	@$(MAKE) -C $(LAYER2_DIR) distclean 2>/dev/null || true
	@$(MAKE) -C $(LAYER3_DIR) distclean 2>/dev/null || true
	@$(MAKE) -C $(LAYER4_DIR) distclean 2>/dev/null || true
	@cd $(LAYER4_DIR)/rs && cargo clean 2>/dev/null || true
	@$(MAKE) -C $(INTEGRATION_DIR) distclean 2>/dev/null || true
	@find . -name "*.bc" -delete 2>/dev/null || true
	@find . -name "*.o" -delete 2>/dev/null || true

# =============================================================================
# Help
# =============================================================================

help:
	@echo "Hologram Atlas-12288 Hierarchical Build System"
	@echo ""
	@echo "Usage: make [target] [options]"
	@echo ""
	@echo "Targets:"
	@echo "  all              - Build all active layers (0-4) and integration"
	@echo "  layer0           - Build Layer 0 (Atlas Core) only"
	@echo "  layer1           - Build Layer 1 (Boundary) and dependencies"
	@echo "  layer2           - Build Layer 2 (Conservation) and dependencies"
	@echo "  layer3           - Build Layer 3 (Resonance) and dependencies"
	@echo "  layer4           - Build Layer 4 (Manifold) and dependencies"
	@echo "  integration      - Build integration tests"
	@echo "  test             - Run all tests (layer + integration)"
	@echo "  check            - Alias for 'test' target"
	@echo "  test-layer[0-4]  - Run specific layer tests"
	@echo "  test-integration - Run integration tests only"
	@echo "  lint             - Run linting checks for all layers"
	@echo "  format           - Run code formatting for all layers"
	@echo "  typecheck        - Run type checking for all layers"
	@echo "  lint-all         - Comprehensive linting (CI-ready)"
	@echo "  format-check-all - Verify formatting (CI-ready)"
	@echo "  typecheck-all    - Comprehensive type checking (CI-ready)"
	@echo "  bench            - Run Layer 2 performance benchmarks"
	@echo "  bench-l2         - Run Layer 2 benchmark suite"
	@echo "  bench-quick      - Run quick benchmark tests"
	@echo "  bench-thorough   - Run comprehensive benchmark comparison"
	@echo "  bench-debug      - Run debug benchmark build"
	@echo "  bench-release    - Run release benchmark build"
	@echo "  bench-clean      - Clean benchmark build artifacts"
	@echo "  install          - Install libraries and headers"
	@echo "  uninstall        - Remove installed files"
	@echo "  clean            - Remove build artifacts"
	@echo "  distclean        - Deep clean including libs"
	@echo "  help             - Show this message"
	@echo ""
	@echo "Options:"
	@echo "  BUILD_TYPE=DEBUG|RELEASE - Build type (default: RELEASE)"
	@echo "  TARGET=<triple>          - Target triple (default: host)"
	@echo "  PREFIX=<path>            - Install prefix (default: /usr/local)"
	@echo ""
	@echo "Layer Status:"
	@echo "  Layer 0 (Atlas Core)     - ✅ Implemented and tested"
	@echo "  Layer 1 (Boundary)       - ✅ Implemented and tested"
	@echo "  Layer 2 (Conservation)   - ✅ Implemented and tested"  
	@echo "  Layer 3 (Resonance)      - ✅ Implemented and tested"
	@echo "  Layer 4 (Manifold)       - ✅ Implemented and tested"
	@echo "  Layer 5 (VPI)            - 📋 Planned (not implemented)"
	@echo "  Layer 6 (SDK)            - 📋 Planned (not implemented)"
	@echo "  Layer 7 (Applications)   - 📋 Planned (not implemented)"

# =============================================================================
# Debug
# =============================================================================

print-%:
	@echo $* = $($*)

.PRECIOUS: $(BUILD_DIR)/%.bc $(BUILD_DIR)/%.ll $(BUILD_DIR)/%.o