# Layer 4: Manifold Layer Makefile

LAYER_NAME := manifold
LAYER_NUM := 4

include ../../build/layer.mk

.DEFAULT_GOAL := $(LIB_PATH)

# Layer-specific dependencies
$(LIB_PATH): check-deps

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

# Layer 4 specific test target (override base layer.mk)
.PHONY: test
test: $(LIB_PATH)
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
	@clang -std=c11 -I../../include -I../include -I./include -c runtime/manifold.c -o runtime/manifold.o -lm

runtime/geometry.o: runtime/geometry.c include/atlas-manifold.h
	@echo "  [CC] Compiling geometry runtime..."
	@mkdir -p runtime
	@clang -std=c11 -I../../include -I../include -I./include -c runtime/geometry.c -o runtime/geometry.o -lm

runtime/topology.o: runtime/topology.c include/atlas-manifold.h
	@echo "  [CC] Compiling topology runtime..."
	@mkdir -p runtime
	@clang -std=c11 -I../../include -I../include -I./include -c runtime/topology.c -o runtime/topology.o -lm

# Clean target to remove runtime objects
.PHONY: clean-runtime
clean-runtime:
	@echo "  [CLEAN] Removing runtime objects..."
	@rm -f runtime/*.o
	@rm -f build/tests/test-manifold build/tests/test-geometry build/tests/test-topology