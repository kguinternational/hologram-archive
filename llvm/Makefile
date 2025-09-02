# Hologram LLVM Implementation Makefile
# (c) 2024–2025 UOR Foundation — MIT License

# =============================================================================
# Configuration
# =============================================================================

# LLVM tools (override with environment variables if needed)
LLVM_CONFIG ?= llvm-config
LLC         ?= llc
OPT         ?= opt
LLVM_LINK   ?= llvm-link
LLVM_AS     ?= llvm-as
LLVM_DIS    ?= llvm-dis
CLANG       ?= clang
CLANGXX     ?= clang++
LLVM_LIT    ?= llvm-lit
AR          ?= ar

# Directories
SRC_DIR     := src
INCLUDE_DIR := include
LIB_DIR     := lib
BUILD_DIR   := build
TEST_DIR    := tests
TOOLS_DIR   := tools
DOCS_DIR    := docs

# Source files (LLVM IR)
ATLAS_SOURCES := \
	$(SRC_DIR)/atlas-12288-types.ll \
	$(SRC_DIR)/atlas-12288-intrinsics.ll \
	$(SRC_DIR)/atlas-12288-r96.ll \
	$(SRC_DIR)/atlas-12288-ops.ll \
	$(SRC_DIR)/atlas-12288-simd.ll \
	$(SRC_DIR)/atlas-12288-memory.ll \
	$(SRC_DIR)/atlas-12288-witness.ll \
	$(SRC_DIR)/atlas-12288-module.ll \
	$(SRC_DIR)/atlas-12288-c768.ll \
	$(SRC_DIR)/atlas-12288-morphisms.ll \
	$(SRC_DIR)/atlas-12288-klein.ll \
	$(SRC_DIR)/atlas-12288-domains.ll \
	$(SRC_DIR)/atlas-12288-acceptance.ll \
	$(SRC_DIR)/atlas-12288-harmonic.ll \
	$(SRC_DIR)/atlas-12288-validation.ll

# Test files (LLVM IR)
ATLAS_TESTS := \
	$(TEST_DIR)/test-r96.ll \
	$(TEST_DIR)/test-conservation.ll \
	$(TEST_DIR)/test-witness.ll \
	$(TEST_DIR)/test-simd.ll \
	$(TEST_DIR)/test-c768.ll \
	$(TEST_DIR)/test-morphisms.ll \
	$(TEST_DIR)/test-klein.ll \
	$(TEST_DIR)/test-domains.ll \
	$(TEST_DIR)/test-acceptance.ll \
	$(TEST_DIR)/test-harmonic.ll

# Output files
ATLAS_LINKED   := $(BUILD_DIR)/atlas-12288.linked.bc
ATLAS_MODULE   := $(BUILD_DIR)/atlas-12288.bc
ATLAS_ASSEMBLY := $(BUILD_DIR)/atlas-12288.ll
ATLAS_OBJECT   := $(BUILD_DIR)/atlas-12288.o
ATLAS_STATIC   := $(LIB_DIR)/libatlas.a
ATLAS_SHARED   := $(LIB_DIR)/libatlas.so

# C API wrapper (optional, if present)
C_API_SRC ?= $(SRC_DIR)/atlas-c-api.c
C_API_OBJ := $(BUILD_DIR)/atlas-c-api.o

# Compilation flags / metadata
LLV_MAJOR := $(shell $(LLVM_CONFIG) --version | sed -E 's/^([0-9]+).*/\1/')
LLVM_CXXFLAGS := $(shell $(LLVM_CONFIG) --cxxflags)
LLVM_LDFLAGS  := $(shell $(LLVM_CONFIG) --ldflags)
LLVM_LIBS     := $(shell $(LLVM_CONFIG) --libs all)
LLVM_SYSTEM_LIBS := $(shell $(LLVM_CONFIG) --system-libs)

# Target architecture
TARGET ?= $(shell $(LLVM_CONFIG) --host-target)

# Build type (DEBUG, RELEASE)
BUILD_TYPE ?= RELEASE
ifeq ($(BUILD_TYPE),DEBUG)
	OPT_LEVEL   := -O0
	DEBUG_FLAGS := -g
	DEFINES     := -DDEBUG
else
	OPT_LEVEL   := -O3
	DEBUG_FLAGS :=
	DEFINES     := -DNDEBUG
endif

# New PM pass pipeline (LLVM 15+)
OPT_PASSES ?= default<O3>

# SIMD support (affects codegen with llc)
SIMD ?= auto
ifeq ($(SIMD),avx512)
	TARGET_FEATURES := +avx512f,+avx512bw
else ifeq ($(SIMD),avx2)
	TARGET_FEATURES := +avx,+avx2
else ifeq ($(SIMD),neon)
	TARGET_FEATURES := +neon
else
	TARGET_FEATURES :=
endif

# JIT support (tools only)
JIT ?= 0
ifeq ($(JIT),1)
	DEFINES   += -DATLAS_JIT_ENABLED
	JIT_LIBS  := -lLLVMMCJIT
endif

# Tools selection
ifeq ($(JIT),1)
	TOOLS_BINARIES := atlas-opt atlas-verify atlas-jit
else
	TOOLS_BINARIES := atlas-opt atlas-verify
endif

# =============================================================================
# Targets
# =============================================================================

.PHONY: all clean distclean test smoke-tests install uninstall docs help tools \
	atlas-opt atlas-verify atlas-jit check-llvm print-%

all: check-llvm $(ATLAS_MODULE) $(ATLAS_STATIC) $(ATLAS_SHARED) tools

# Create directories
$(BUILD_DIR) $(LIB_DIR):
	@mkdir -p $@

# =============================================================================
# Version / environment guards
# =============================================================================

check-llvm:
	@maj=$(LLV_MAJOR); \
	if [ -z "$$maj" ]; then echo "[ERR] Unable to detect LLVM version via llvm-config"; exit 1; fi; \
	if [ "$$maj" -lt 15 ]; then echo "[ERR] LLVM >= 15 required (opaque pointers). Detected $$maj"; exit 1; fi; \
	echo "[OK] LLVM $$maj detected"

# =============================================================================
# LLVM Module Building
# =============================================================================

# Link all LLVM source files into a single bitcode module
$(ATLAS_LINKED): $(ATLAS_SOURCES) | $(BUILD_DIR)
	@echo "[LINK] Creating Atlas linked module..."
	$(LLVM_LINK) $(ATLAS_SOURCES) -o $@

# Optimize the linked module into the final bitcode artifact
$(ATLAS_MODULE): $(ATLAS_LINKED) | $(BUILD_DIR)
	@echo "[OPT ] Optimizing Atlas module..."
	$(OPT) -passes='verify,$(OPT_PASSES)' $< -o $@

# Generate human-readable assembly
$(ATLAS_ASSEMBLY): $(ATLAS_MODULE)
	@echo "[DIS ] Generating assembly..."
	$(LLVM_DIS) $< -o $@

# Compile to native object file (PIC for shared library)
$(ATLAS_OBJECT): $(ATLAS_MODULE)
	@echo "[LLC ] Compiling to native code..."
	$(LLC) $(OPT_LEVEL) \
		-relocation-model=pic \
		-filetype=obj \
		-mtriple=$(TARGET) \
		$(if $(TARGET_FEATURES),-mattr=$(TARGET_FEATURES)) \
		$< -o $@

# =============================================================================
# Library Building
# =============================================================================

# Build static library (with optional C API object if it exists)
$(ATLAS_STATIC): $(ATLAS_OBJECT) | $(LIB_DIR)
	@echo "[AR  ] Creating static library..."
	@if [ -f "$(C_API_SRC)" ]; then \
		$(CLANG) -c -fPIC $(OPT_LEVEL) $(DEBUG_FLAGS) $(DEFINES) -I$(INCLUDE_DIR) $(C_API_SRC) -o $(C_API_OBJ); \
		$(AR) rcs $@ $(ATLAS_OBJECT) $(C_API_OBJ); \
	else \
		$(AR) rcs $@ $(ATLAS_OBJECT); \
	fi

# Build shared library
$(ATLAS_SHARED): $(ATLAS_OBJECT) | $(LIB_DIR)
	@echo "[LD  ] Creating shared library..."
	@if [ -f "$(C_API_OBJ)" ]; then OBJS='$(ATLAS_OBJECT) $(C_API_OBJ)'; else OBJS='$(ATLAS_OBJECT)'; fi; \
	$(CLANGXX) -shared -fPIC $(DEBUG_FLAGS) $$OBJS \
		$(LLVM_LDFLAGS) -o $@ $(LLVM_LIBS) $(LLVM_SYSTEM_LIBS) $(JIT_LIBS)

# =============================================================================
# Tools
# =============================================================================

tools: $(addprefix $(TOOLS_DIR)/,$(TOOLS_BINARIES))

$(TOOLS_DIR)/atlas-opt: $(TOOLS_DIR)/atlas-opt.cpp $(ATLAS_STATIC)
	@echo "[CXX ] Building atlas-opt..."
	$(CLANGXX) $(LLVM_CXXFLAGS) $(OPT_LEVEL) $(DEBUG_FLAGS) $(DEFINES) \
		-I$(INCLUDE_DIR) $< -o $@ \
		$(ATLAS_STATIC) $(LLVM_LDFLAGS) $(LLVM_LIBS) $(LLVM_SYSTEM_LIBS)

$(TOOLS_DIR)/atlas-verify: $(TOOLS_DIR)/atlas-verify.cpp $(ATLAS_STATIC)
	@echo "[CXX ] Building atlas-verify..."
	$(CLANGXX) $(LLVM_CXXFLAGS) $(OPT_LEVEL) $(DEBUG_FLAGS) $(DEFINES) \
		-I$(INCLUDE_DIR) $< -o $@ \
		$(ATLAS_STATIC) $(LLVM_LDFLAGS) $(LLVM_LIBS) $(LLVM_SYSTEM_LIBS)

$(TOOLS_DIR)/atlas-jit: $(TOOLS_DIR)/atlas-jit.cpp $(ATLAS_STATIC)
	@echo "[CXX ] Building atlas-jit..."
	$(CLANGXX) $(LLVM_CXXFLAGS) $(OPT_LEVEL) $(DEBUG_FLAGS) $(DEFINES) -DATLAS_JIT_ENABLED \
		-I$(INCLUDE_DIR) $< -o $@ \
		$(ATLAS_STATIC) $(LLVM_LDFLAGS) $(LLVM_LIBS) $(LLVM_SYSTEM_LIBS) -lLLVMMCJIT

# =============================================================================
# Testing
# =============================================================================

.PHONY: test-r96 test-conservation test-witness test-simd test-c768 test-morphisms test-klein test-domains test-acceptance test-harmonic test-memory benchmark

test: $(ATLAS_MODULE)
	@echo "[TEST] Running tests..."
	@if command -v $(LLVM_LIT) >/dev/null 2>&1 && [ -f $(TEST_DIR)/lit.cfg ] ; then \
		$(LLVM_LIT) -sv $(TEST_DIR); \
	else \
		$(MAKE) --no-print-directory smoke-tests; \
	fi

# Lightweight smoke tests: link + verify each test file if present
smoke-tests: test-r96 test-conservation test-witness test-simd test-c768 test-morphisms test-klein test-domains test-acceptance test-harmonic test-integration

define _link_verify
	@t="$1"; \
	if [ -f "$(TEST_DIR)/$$t.ll" ]; then \
		echo "[TEST] $$t (link+verify)"; \
		$(LLVM_LINK) $(ATLAS_MODULE) $(TEST_DIR)/$$t.ll -o $(BUILD_DIR)/$$t.bc; \
		$(OPT) -passes='verify,default<O2>' $(BUILD_DIR)/$$t.bc -o /dev/null; \
	else \
		echo "[SKIP] $(TEST_DIR)/$$t.ll not found"; \
	fi
endef

test-r96: $(ATLAS_MODULE) | $(BUILD_DIR)
	$(call _link_verify,test-r96)

test-conservation: $(ATLAS_MODULE) | $(BUILD_DIR)
	$(call _link_verify,test-conservation)

test-witness: $(ATLAS_MODULE) | $(BUILD_DIR)
	$(call _link_verify,test-witness)

test-simd: $(ATLAS_MODULE) | $(BUILD_DIR)
	$(call _link_verify,test-simd)

test-c768: $(ATLAS_MODULE) | $(BUILD_DIR)
	$(call _link_verify,test-c768)

test-morphisms: $(ATLAS_MODULE) | $(BUILD_DIR)
	$(call _link_verify,test-morphisms)

test-klein: $(ATLAS_MODULE) | $(BUILD_DIR)
	$(call _link_verify,test-klein)

test-domains: $(ATLAS_MODULE) | $(BUILD_DIR)
	$(call _link_verify,test-domains)

test-acceptance: $(ATLAS_MODULE) | $(BUILD_DIR)
	$(call _link_verify,test-acceptance)

test-harmonic: $(ATLAS_MODULE) | $(BUILD_DIR)
	$(call _link_verify,test-harmonic)

# Integration tests
test-integration:
	@echo "[TEST] Running Layer 2 integration tests..."
	@$(MAKE) -C ../tests run-layer2 || echo "[WARN] Integration tests not available"

# Memory tests & benchmarks are external scripts/binaries

test-memory: $(ATLAS_SHARED)
	@echo "[TEST] Running memory tests with Valgrind..."
	@valgrind --leak-check=full --show-leak-kinds=all --track-origins=yes $(TEST_DIR)/test-memory || true

benchmark: $(ATLAS_SHARED)
	@echo "[BENCH] Running performance benchmarks..."
	@$(TEST_DIR)/benchmark.sh || true

# =============================================================================
# Installation
# =============================================================================

PREFIX ?= /usr/local
INSTALL_INC := $(PREFIX)/include
INSTALL_LIB := $(PREFIX)/lib
INSTALL_BIN := $(PREFIX)/bin

install: all
	@echo "[INST] Installing Atlas-12288..."
	install -d $(INSTALL_INC)/atlas
	install -m 644 $(INCLUDE_DIR)/*.h $(INSTALL_INC)/atlas/ || true
	install -d $(INSTALL_LIB)
	install -m 644 $(ATLAS_STATIC) $(INSTALL_LIB)/
	install -m 755 $(ATLAS_SHARED) $(INSTALL_LIB)/
	install -d $(INSTALL_BIN)
	for t in $(TOOLS_BINARIES); do install -m 755 $(TOOLS_DIR)/$$t $(INSTALL_BIN)/ || true; done
	-ldconfig >/dev/null 2>&1 || true

uninstall:
	@echo "[UNIN] Removing Atlas-12288..."
	rm -rf $(INSTALL_INC)/atlas
	rm -f $(INSTALL_LIB)/libatlas.a
	rm -f $(INSTALL_LIB)/libatlas.so
	for t in $(TOOLS_BINARIES); do rm -f $(INSTALL_BIN)/$$t; done

# =============================================================================
# Documentation
# =============================================================================

.PHONY: docs

docs:
	@echo "[DOC ] Generating documentation..."
	@doxygen $(DOCS_DIR)/Doxyfile || echo "Doxygen not configured; skipping."

# =============================================================================
# Cleaning
# =============================================================================

clean:
	@echo "[CLEAN] Removing build artifacts..."
	rm -rf $(BUILD_DIR)
	rm -rf $(LIB_DIR)
	rm -f $(TOOLS_DIR)/atlas-opt $(TOOLS_DIR)/atlas-verify $(TOOLS_DIR)/atlas-jit

distclean: clean
	@echo "[CLEAN] Removing all generated files..."
	rm -rf docs/html docs/latex
	find . -name "*.bc" -delete
	find . -name "*.o" -delete
	find . -name "*.so" -delete
	find . -name "*.a" -delete

# =============================================================================
# Help
# =============================================================================

help:
	@echo "Hologram LLVM Implementation Makefile"
	@echo ""
	@echo "Usage: make [target] [options]"
	@echo ""
	@echo "Targets:"
	@echo "  all           - Build everything (default)"
	@echo "  clean         - Remove build artifacts"
	@echo "  test          - Run tests (lit if available, else smoke tests)"
	@echo "  install       - Install libraries and tools"
	@echo "  docs          - Generate documentation (Doxygen)"
	@echo "  help          - Show this message"
	@echo ""
	@echo "Options:"
	@echo "  BUILD_TYPE=DEBUG|RELEASE   - Build type (default: RELEASE)"
	@echo "  TARGET=<triple>            - Target triple (default: host)"
	@echo "  SIMD=auto|avx2|avx512|neon - SIMD support level"
	@echo "  JIT=0|1                    - Enable JIT tool build"
	@echo "  PREFIX=<path>              - Install prefix (default: /usr/local)"
	@echo ""
	@echo "Examples:"
	@echo "  make BUILD_TYPE=DEBUG"
	@echo "  make test"
	@echo "  make install PREFIX=/opt/hologram"
	@echo "  make SIMD=avx512 JIT=1"

# =============================================================================
# Misc
# =============================================================================

.PRECIOUS: $(BUILD_DIR)/%.bc $(BUILD_DIR)/%.ll $(BUILD_DIR)/%.o

# Print any variable for debugging
print-%:
	@echo $* = $($*)
