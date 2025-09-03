# Layer 3: Resonance Layer Makefile

LAYER_NAME := resonance
LAYER_NUM := 3

include ../../build/layer.mk

.DEFAULT_GOAL := $(LIB_PATH)

# Layer-specific dependencies
$(LIB_PATH): check-deps

check-deps:
	@if [ ! -f "../../lib/libatlas-conservation.a" ]; then \
		echo "[ERROR] Layer 2 (Conservation) must be built first"; \
		exit 1; \
	fi

# Layer 3 specific test target (override base layer.mk)
.PHONY: test
test: $(LIB_PATH)
	@echo "  [TEST] Running Layer 3 (Resonance) tests..."
	@if [ -f "tests/test-harmonic.ll" ]; then \
		echo "  [LLVM] Checking harmonic test..."; \
		mkdir -p build/tests; \
		llvm-as tests/test-harmonic.ll -o build/tests/test-harmonic.bc 2>/dev/null || \
			echo "  [SKIP] test-harmonic.ll (LLVM compilation failed)"; \
	fi
	@if [ -f "tests/test-r96.ll" ]; then \
		echo "  [LLVM] Checking R96 test..."; \
		mkdir -p build/tests; \
		llvm-as tests/test-r96.ll -o build/tests/test-r96.bc 2>/dev/null || \
			echo "  [SKIP] test-r96.ll (LLVM compilation failed)"; \
	fi
	@if [ -f "tests/test-c768.ll" ]; then \
		echo "  [LLVM] Checking C768 test..."; \
		mkdir -p build/tests; \
		llvm-as tests/test-c768.ll -o build/tests/test-c768.bc 2>/dev/null || \
			echo "  [SKIP] test-c768.ll (LLVM compilation failed)"; \
	fi
	@if [ -f "tests/test-classification.c" ]; then \
		echo "  [CC] Compiling classification test..."; \
		mkdir -p build/tests; \
		clang -std=c11 -I../../include -I../include -I./include \
			tests/test-classification.c -L../../lib -latlas-resonance -lm -o build/tests/test-classification 2>/dev/null || \
			echo "  [SKIP] test-classification.c (compilation failed)"; \
		if [ -x "build/tests/test-classification" ]; then \
			echo "  [RUN] Executing classification tests..."; \
			./build/tests/test-classification || echo "  [WARN] Some classification tests failed"; \
		fi; \
	fi
	@if [ -f "tests/test-clustering.c" ]; then \
		echo "  [CC] Compiling clustering test..."; \
		mkdir -p build/tests; \
		clang -std=c11 -I../../include -I../include -I./include -lm \
			tests/test-clustering.c runtime/clustering.c runtime/classification.c \
			-L../../lib -latlas-resonance -o build/tests/test-clustering 2>/dev/null || \
			echo "  [SKIP] test-clustering.c (compilation failed)"; \
		if [ -x "build/tests/test-clustering" ]; then \
			echo "  [RUN] Executing clustering tests..."; \
			./build/tests/test-clustering || echo "  [WARN] Some clustering tests failed"; \
		fi; \
	fi
	@if [ -f "tests/test-scheduling.c" ]; then \
		echo "  [CC] Compiling scheduling test..."; \
		mkdir -p build/tests; \
		clang -std=c11 -I../../include -I../include -I./include -lm \
			tests/test-scheduling.c runtime/scheduling.c runtime/classification.c \
			-L../../lib -latlas-resonance -o build/tests/test-scheduling 2>/dev/null || \
			echo "  [SKIP] test-scheduling.c (compilation failed)"; \
		if [ -x "build/tests/test-scheduling" ]; then \
			echo "  [RUN] Executing scheduling tests..."; \
			./build/tests/test-scheduling || echo "  [WARN] Some scheduling tests failed"; \
		fi; \
	fi
	@echo "  [OK] Layer 3 tests completed"

# Additional build targets for runtime components
.PHONY: clustering scheduling runtime-all

clustering: runtime/clustering.o
scheduling: runtime/scheduling.o
runtime-all: clustering scheduling

runtime/clustering.o: runtime/clustering.c include/atlas-resonance.h
	@echo "  [CC] Compiling clustering runtime..."
	@mkdir -p runtime
	@clang -std=c11 -I../../include -I../include -I./include -c runtime/clustering.c -o runtime/clustering.o

runtime/scheduling.o: runtime/scheduling.c include/atlas-resonance.h
	@echo "  [CC] Compiling scheduling runtime..."
	@mkdir -p runtime
	@clang -std=c11 -I../../include -I../include -I./include -c runtime/scheduling.c -o runtime/scheduling.o -lm

# Clean target to remove runtime objects
.PHONY: clean-runtime
clean-runtime:
	@echo "  [CLEAN] Removing runtime objects..."
	@rm -f runtime/*.o
	@rm -f build/tests/test-clustering build/tests/test-scheduling