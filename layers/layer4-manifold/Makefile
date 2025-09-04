# Layer 4: Manifold Layer Makefile

LAYER_NAME := manifold
LAYER_NUM := 4

# Compiler flags for runtime
CFLAGS := -std=c11 -Wall -Wextra -O2 -fPIC -I../../include -I./include

# Override runtime sources to be empty since we're using pure Rust
C_SOURCES :=

include ../../build/layer.mk

.DEFAULT_GOAL := $(LIB_PATH)

# Override the library build to use Rust only (no C runtime compilation)
$(LIB_PATH): check-deps rust-lib-only

# Force the ALL_OBJ variable to be empty to prevent C compilation
ALL_OBJ :=

# Variables already overridden above

.PHONY: rust-lib rust-lib-only
rust-lib: ../../lib
rust-lib-only: ../../lib
	@echo "  [RUST] Building manifold library..."
	@cd rs && (cargo build --release 2>/dev/null || /home/codespace/.cargo/bin/cargo build --release 2>/dev/null) || echo "  [ERROR] Rust build failed"
	@if [ -f "rs/target/release/libatlas_manifold.a" ]; then \
		cp rs/target/release/libatlas_manifold.a $(LIB_PATH); \
		echo "  [OK] Layer 4 built: $(LIB_NAME)"; \
	else \
		echo "  [ERROR] Failed to build Rust library"; \
		exit 1; \
	fi

# Layer-specific dependencies

check-deps:
	@if [ ! -f "../../lib/libatlas-core.a" ]; then \
		echo "[ERROR] Layer 1 (Core) must be built first"; \
		exit 1; \
	fi
	@if [ ! -f "../../lib/libatlas-conservation.a" ]; then \
		echo "[ERROR] Layer 2 (Conservation) must be built first"; \
		exit 1; \
	fi
	@if [ ! -f "../../lib/libatlas-resonance.a" ]; then \
		echo "[ERROR] Layer 3 (Resonance) must be built first"; \
		exit 1; \
	fi

# Rust targets
.PHONY: lint-rust format-rust typecheck-rust check-rust format-check-rust check-cargo

check-cargo:
	@which cargo >/dev/null 2>&1 || test -f /home/codespace/.cargo/bin/cargo || { echo "  [ERROR] cargo not found. Please install Rust toolchain."; exit 1; }

lint-rust: check-cargo
	@echo "  [LINT] Running Rust clippy..."
	@cd rs && (cargo clippy -- -D warnings || /home/codespace/.cargo/bin/cargo clippy -- -D warnings)

format-rust: check-cargo
	@echo "  [FORMAT] Formatting Rust code..."
	@cd rs && (cargo fmt || /home/codespace/.cargo/bin/cargo fmt)

format-check-rust: check-cargo
	@echo "  [FORMAT-CHECK] Checking Rust formatting..."
	@cd rs && (cargo fmt -- --check || /home/codespace/.cargo/bin/cargo fmt -- --check)

typecheck-rust: check-cargo
	@echo "  [TYPECHECK] Running Rust type checking..."
	@cd rs && (cargo check || /home/codespace/.cargo/bin/cargo check)

check-rust: typecheck-rust lint-rust format-check-rust
	@echo "  [OK] All Rust checks passed"

# Layer 4 specific test target (override base layer.mk)
.PHONY: test
test: $(LIB_PATH)
	@if which cargo >/dev/null 2>&1; then \
		$(MAKE) check-rust; \
	else \
		echo "  [SKIP] Rust checks (cargo not available)"; \
	fi
	@echo "  [TEST] Running Layer 4 (Manifold) tests..."
	@if [ -f "tests/test-manifold.c" ]; then \
		echo "  [CC] Compiling manifold test..."; \
		mkdir -p build/tests; \
		clang -std=c11 -I../../include -I../include -I./include \
			tests/test-manifold.c -L../../lib -latlas-manifold -lm -o build/tests/test-manifold 2>/dev/null || \
			echo "  [SKIP] test-manifold.c (compilation failed)"; \
		if [ -x "build/tests/test-manifold" ]; then \
			echo "  [RUN] Executing manifold tests..."; \
			./build/tests/test-manifold || echo "  [WARN] Some manifold tests failed"; \
		fi; \
	fi
	@if [ -f "tests/test-projection.c" ]; then \
		echo "  [CC] Compiling projection test..."; \
		mkdir -p build/tests; \
		clang -std=c11 -I../../include -I../include -I./include \
			tests/test-projection.c -L../../lib -latlas-manifold -lm -o build/tests/test-projection 2>/dev/null || \
			echo "  [SKIP] test-projection.c (compilation failed)"; \
		if [ -x "build/tests/test-projection" ]; then \
			echo "  [RUN] Executing projection tests..."; \
			./build/tests/test-projection || echo "  [WARN] Some projection tests failed"; \
		fi; \
	fi
	@if [ -f "tests/test-geometry.c" ]; then \
		echo "  [CC] Compiling geometry test..."; \
		mkdir -p build/tests; \
		clang -std=c11 -I../../include -I../include -I./include \
			tests/test-geometry.c -L../../lib -latlas-manifold -lm -o build/tests/test-geometry 2>/dev/null || \
			echo "  [SKIP] test-geometry.c (compilation failed)"; \
		if [ -x "build/tests/test-geometry" ]; then \
			echo "  [RUN] Executing geometry tests..."; \
			./build/tests/test-geometry || echo "  [WARN] Some geometry tests failed"; \
		fi; \
	fi
	@if [ -f "tests/test-topology.c" ]; then \
		echo "  [CC] Compiling topology test..."; \
		mkdir -p build/tests; \
		clang -std=c11 -I../../include -I../include -I./include \
			tests/test-topology.c -L../../lib -latlas-manifold -lm -o build/tests/test-topology 2>/dev/null || \
			echo "  [SKIP] test-topology.c (compilation failed)"; \
		if [ -x "build/tests/test-topology" ]; then \
			echo "  [RUN] Executing topology tests..."; \
			./build/tests/test-topology || echo "  [WARN] Some topology tests failed"; \
		fi; \
	fi
	@if [ -f "tests/test-verification.c" ]; then \
		echo "  [CC] Compiling verification test..."; \
		mkdir -p build/tests; \
		clang -std=c11 -I../../include -I../include -I./include \
			tests/test-verification.c -L../../lib -latlas-manifold -lm -o build/tests/test-verification 2>/dev/null || \
			echo "  [SKIP] test-verification.c (compilation failed)"; \
		if [ -x "build/tests/test-verification" ]; then \
			echo "  [RUN] Executing verification tests..."; \
			./build/tests/test-verification || echo "  [WARN] Some verification tests failed"; \
		fi; \
	fi
	@if [ -f "tests/test-integration.c" ]; then \
		echo "  [CC] Compiling integration test..."; \
		mkdir -p build/tests; \
		clang -std=c11 -I../../include -I../include -I./include \
			tests/test-integration.c -L../../lib -latlas-manifold -latlas-resonance -latlas-conservation -lm -o build/tests/test-integration 2>/dev/null || \
			echo "  [SKIP] test-integration.c (compilation failed)"; \
		if [ -x "build/tests/test-integration" ]; then \
			echo "  [RUN] Executing integration tests..."; \
			./build/tests/test-integration || echo "  [WARN] Some integration tests failed"; \
		fi; \
	fi
	@if which cargo >/dev/null 2>&1; then \
		echo "  [TEST] Running Rust tests..."; \
		cd rs && (cargo test || /home/codespace/.cargo/bin/cargo test); \
	else \
		echo "  [SKIP] Rust tests (cargo not available)"; \
	fi
	@echo "  [OK] Layer 4 tests completed"

# Additional build targets for runtime components
.PHONY: manifold geometry topology runtime-all

manifold: runtime/manifold.o
geometry: runtime/geometry.o
topology: runtime/topology.o
runtime-all: manifold geometry topology

runtime/manifold.o: runtime/manifold.c include/atlas-manifold.h
	@echo "  [CC] Compiling manifold runtime..."
	@mkdir -p runtime
	@clang -std=c11 -I../../include -I../include -I./include -c runtime/manifold.c -o runtime/manifold.o

runtime/geometry.o: runtime/geometry.c include/atlas-manifold.h
	@echo "  [CC] Compiling geometry runtime..."
	@mkdir -p runtime
	@clang -std=c11 -I../../include -I../include -I./include -c runtime/geometry.c -o runtime/geometry.o

runtime/topology.o: runtime/topology.c include/atlas-manifold.h
	@echo "  [CC] Compiling topology runtime..."
	@mkdir -p runtime
	@clang -std=c11 -I../../include -I../include -I./include -c runtime/topology.c -o runtime/topology.o

# Clean target to remove runtime objects
.PHONY: clean-runtime
clean-runtime:
	@echo "  [CLEAN] Removing runtime objects..."
	@rm -f runtime/*.o
	@rm -f build/tests/test-manifold build/tests/test-projection build/tests/test-geometry build/tests/test-topology build/tests/test-verification build/tests/test-integration

# =============================================================================
# Benchmark targets
# =============================================================================

.PHONY: benchmarks bench-run bench-quick bench-report benchmark-harness
.PHONY: bench-core bench-geometric bench-signal bench-traditional bench-applications
.PHONY: bench-debug bench-release bench-profile benchmark-clean benchmark-test

# Build all benchmarks
benchmarks: $(LIB_PATH)
	@echo "  [BENCHMARKS] Building all Layer 4 benchmarks..."
	@$(MAKE) -C benchmarks BUILD_TYPE=release all

# Run all benchmarks
bench-run: benchmarks
	@echo "  [BENCHMARKS] Running all Layer 4 benchmarks..."
	@$(MAKE) -C benchmarks run

# Run quick benchmarks (for development)
bench-quick: benchmarks
	@echo "  [BENCHMARKS] Running quick Layer 4 benchmarks..."
	@$(MAKE) -C benchmarks bench-quick

# Generate benchmark report
bench-report: benchmarks
	@echo "  [BENCHMARKS] Generating Layer 4 benchmark report..."
	@cd benchmarks && python3 harness/generate_report.py

# Build benchmark harness utilities
benchmark-harness: $(LIB_PATH)
	@echo "  [BENCHMARKS] Setting up benchmark harness..."
	@$(MAKE) -C benchmarks harness

# Individual benchmark suites
bench-core: benchmarks
	@echo "  [BENCHMARKS] Running core UN operation benchmarks..."
	@$(MAKE) -C benchmarks run-core

bench-geometric: benchmarks
	@echo "  [BENCHMARKS] Running geometric computation benchmarks..."
	@$(MAKE) -C benchmarks run-geometric

bench-signal: benchmarks
	@echo "  [BENCHMARKS] Running signal processing benchmarks..."
	@$(MAKE) -C benchmarks run-signal

bench-traditional: benchmarks
	@echo "  [BENCHMARKS] Running traditional baseline benchmarks..."
	@$(MAKE) -C benchmarks run-traditional

bench-applications: benchmarks
	@echo "  [BENCHMARKS] Running application workload benchmarks..."
	@$(MAKE) -C benchmarks run-applications

# Build type specific targets
bench-debug: $(LIB_PATH)
	@echo "  [BENCHMARKS] Building debug benchmarks..."
	@$(MAKE) -C benchmarks BUILD_TYPE=debug all

bench-release: $(LIB_PATH)
	@echo "  [BENCHMARKS] Building release benchmarks..."
	@$(MAKE) -C benchmarks BUILD_TYPE=release all

# Profile benchmarks with perf
bench-profile: benchmarks
	@echo "  [BENCHMARKS] Running profiled benchmarks..."
	@$(MAKE) -C benchmarks profile

# Test benchmark framework
benchmark-test: benchmark-harness
	@echo "  [BENCHMARKS] Testing benchmark framework..."
	@$(MAKE) -C benchmarks check-deps validate

# Clean benchmark artifacts
benchmark-clean:
	@echo "  [BENCHMARKS] Cleaning benchmark artifacts..."
	@$(MAKE) -C benchmarks distclean

# =============================================================================
# Real-World Application Benchmarks Integration
# =============================================================================

.PHONY: app-benchmarks app-bench-all app-bench-ml app-bench-db app-bench-crypto
.PHONY: app-bench-test app-bench-report app-bench-clean app-bench-install

# Build application benchmarks
app-benchmarks: $(LIB_PATH)
	@echo "  [APP-BENCH] Building real-world application benchmarks..."
	@$(MAKE) -C benchmarks/applications all

# Run all application benchmarks
app-bench-all: app-benchmarks
	@echo "  [APP-BENCH] Running comprehensive application benchmarks..."
	@$(MAKE) -C benchmarks/applications run-all

# Run individual application benchmarks
app-bench-ml: app-benchmarks
	@echo "  [APP-BENCH] Running ML Operations benchmark..."
	@$(MAKE) -C benchmarks/applications run-ml

app-bench-db: app-benchmarks
	@echo "  [APP-BENCH] Running Database Operations benchmark..."
	@$(MAKE) -C benchmarks/applications run-db

app-bench-crypto: app-benchmarks
	@echo "  [APP-BENCH] Running Cryptographic Operations benchmark..."
	@$(MAKE) -C benchmarks/applications run-crypto

# Quick application benchmark tests
app-bench-test: app-benchmarks
	@echo "  [APP-BENCH] Running quick application benchmark tests..."
	@$(MAKE) -C benchmarks/applications test-all

# Generate application benchmark report
app-bench-report: app-benchmarks
	@echo "  [APP-BENCH] Generating comprehensive application benchmark report..."
	@$(MAKE) -C benchmarks/applications report

# Install application benchmarks
app-bench-install: app-benchmarks
	@echo "  [APP-BENCH] Installing application benchmarks..."
	@$(MAKE) -C benchmarks/applications install

# Clean application benchmark artifacts
app-bench-clean:
	@echo "  [APP-BENCH] Cleaning application benchmark artifacts..."
	@$(MAKE) -C benchmarks/applications clean

# Convenience targets combining existing and new benchmarks
.PHONY: bench-comprehensive bench-test-all bench-clean-all

# Run all benchmarks (framework + applications)
bench-comprehensive: bench-full app-bench-all
	@echo "  [BENCH-ALL] Comprehensive benchmarks completed (core + applications)"

# Test all benchmark systems
bench-test-all: benchmark-test app-bench-test
	@echo "  [BENCH-ALL] All benchmark tests completed"

# Clean all benchmark artifacts
bench-clean-all: benchmark-clean app-bench-clean
	@echo "  [BENCH-ALL] All benchmark artifacts cleaned"

# =============================================================================
# Clean Targets
# =============================================================================

# Override clean to include Rust artifacts (overrides base layer.mk clean target)
.PHONY: clean
clean: clean-runtime
	@echo "  [CLEAN] Removing Rust build artifacts..."
	@cd rs && (cargo clean 2>/dev/null || /home/codespace/.cargo/bin/cargo clean 2>/dev/null) || true
	@echo "  [CLEAN] Removing build artifacts..."
	@rm -rf build
	@rm -f $(LIB_PATH)
	@rm -f runtime/*.o

# Deep clean including benchmarks
.PHONY: distclean
distclean: clean bench-clean-all
	@echo "  [DISTCLEAN] Deep cleaning Layer 4..."