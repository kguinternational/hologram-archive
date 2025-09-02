# Makefile for Atlas-12288 Layer 2 Host Runtime
# (c) 2024-2025 UOR Foundation - MIT License

# =============================================================================
# Configuration
# =============================================================================

CC := clang
LD := ld.lld
AR := llvm-ar
LLVM_CONFIG := llvm-config

# Project directories
ROOT_DIR := $(shell dirname $(realpath $(firstword $(MAKEFILE_LIST))))
LLVM_DIR := $(ROOT_DIR)/../llvm
SRC_DIR := $(ROOT_DIR)/src
BUILD_DIR := $(ROOT_DIR)/build
LIB_DIR := $(ROOT_DIR)/lib
INCLUDE_DIR := $(ROOT_DIR)/include

# LLVM directories
LLVM_SRC_DIR := $(LLVM_DIR)/src
LLVM_BUILD_DIR := $(LLVM_DIR)/build
LLVM_LIB_DIR := $(LLVM_DIR)/lib
LLVM_INCLUDE_DIR := $(LLVM_DIR)/include

# =============================================================================
# Compiler and Linker Flags
# =============================================================================

# Get LLVM configuration
LLVM_VERSION := $(shell $(LLVM_CONFIG) --version 2>/dev/null || echo "15.0.0")
LLVM_CFLAGS := $(shell $(LLVM_CONFIG) --cflags 2>/dev/null || echo "")
LLVM_LDFLAGS := $(shell $(LLVM_CONFIG) --ldflags 2>/dev/null || echo "")
LLVM_LIBS := $(shell $(LLVM_CONFIG) --libs 2>/dev/null || echo "-lLLVM")

# Base compilation flags
CFLAGS := -std=c11 -fPIC -O2 -g
CFLAGS += -Wall -Wextra -Werror -Wno-unused-parameter
CFLAGS += -fvisibility=hidden
CFLAGS += -I$(INCLUDE_DIR) -I$(LLVM_INCLUDE_DIR)

# Architecture-specific optimizations
CFLAGS += -march=native -mtune=native

# Thread safety configuration
ifndef ATLAS_SINGLE_THREAD
    CFLAGS += -pthread
    LDFLAGS += -pthread
endif

# Debug configuration
ifdef DEBUG
    CFLAGS += -DDEBUG -O0 -fsanitize=address,undefined
    LDFLAGS += -fsanitize=address,undefined
endif

# WASM configuration
ifdef WASM
    CFLAGS += -DATLAS_SINGLE_THREAD -target wasm32-unknown-wasi
    LDFLAGS += -target wasm32-unknown-wasi
endif

# Linker flags
LDFLAGS += -L$(LLVM_LIB_DIR) -L$(LIB_DIR)
LDFLAGS += $(LLVM_LDFLAGS)

# Libraries
LIBS := -latlas $(LLVM_LIBS)
LIBS += -lm -ldl

# =============================================================================
# Source Files and Targets
# =============================================================================

# Runtime source files
RUNTIME_SOURCES := $(SRC_DIR)/layer2.c $(SRC_DIR)/layer3.c $(SRC_DIR)/llvm-stubs.c
RUNTIME_OBJECTS := $(RUNTIME_SOURCES:$(SRC_DIR)/%.c=$(BUILD_DIR)/%.o)

# LLVM IR files to link
LLVM_IR_FILES := \
    $(LLVM_BUILD_DIR)/atlas-12288.bc \
    $(LLVM_BUILD_DIR)/test-domains.bc \
    $(LLVM_BUILD_DIR)/test-c768.bc \
    $(LLVM_BUILD_DIR)/test-harmonic.bc

# Library targets
STATIC_LIB := $(LIB_DIR)/libatlas-runtime.a
SHARED_LIB := $(LIB_DIR)/libatlas-runtime.so

# Test programs
TEST_SOURCES := $(wildcard tests/*.c)
TEST_PROGRAMS := $(TEST_SOURCES:tests/%.c=$(BUILD_DIR)/test-%)

# =============================================================================
# Build Rules
# =============================================================================

.PHONY: all clean install uninstall test docs help
.DEFAULT_GOAL := all

all: directories $(STATIC_LIB) $(SHARED_LIB)

# Create build directories
directories:
	@mkdir -p $(BUILD_DIR) $(LIB_DIR) $(INCLUDE_DIR)

# Compile C source files
$(BUILD_DIR)/%.o: $(SRC_DIR)/%.c
	@echo "Compiling $<..."
	@$(CC) $(CFLAGS) -c $< -o $@

# Build static library
$(STATIC_LIB): $(RUNTIME_OBJECTS)
	@echo "Creating static library $@..."
	@$(AR) rcs $@ $^

# Build shared library
$(SHARED_LIB): $(RUNTIME_OBJECTS)
	@echo "Creating shared library $@..."
	@$(CC) -shared $(LDFLAGS) -o $@ $^ $(LIBS)

# Build test programs
$(BUILD_DIR)/test-%: tests/test-%.c $(STATIC_LIB)
	@echo "Building test $@..."
	@$(CC) $(CFLAGS) -o $@ $< -L$(LIB_DIR) -latlas-runtime -lm

# =============================================================================
# LLVM Integration
# =============================================================================

# Link with LLVM IR modules
llvm-link: $(LLVM_IR_FILES)
	@echo "Linking LLVM IR modules..."
	@llvm-link $(LLVM_IR_FILES) -o $(BUILD_DIR)/atlas-runtime-linked.bc

# Compile LLVM IR to object file
$(BUILD_DIR)/atlas-runtime-llvm.o: llvm-link
	@echo "Compiling LLVM IR to object file..."
	@llc $(BUILD_DIR)/atlas-runtime-linked.bc -filetype=obj -o $@

# Enhanced library with LLVM IR
$(LIB_DIR)/libatlas-runtime-full.so: $(RUNTIME_OBJECTS) $(BUILD_DIR)/atlas-runtime-llvm.o
	@echo "Creating full runtime library with LLVM IR..."
	@$(CC) -shared $(LDFLAGS) -o $@ $^ $(LIBS)

# =============================================================================
# Testing
# =============================================================================

test: $(TEST_PROGRAMS)
	@echo "Running runtime tests..."
	@for test in $(TEST_PROGRAMS); do \
		echo "Running $$test..."; \
		$$test || exit 1; \
	done

# Valgrind memory testing
test-memory: $(TEST_PROGRAMS)
	@echo "Running memory tests with Valgrind..."
	@for test in $(TEST_PROGRAMS); do \
		echo "Memory testing $$test..."; \
		valgrind --leak-check=full --error-exitcode=1 $$test || exit 1; \
	done

# Performance benchmarks
benchmark: $(BUILD_DIR)/test-benchmark
	@echo "Running performance benchmarks..."
	@$(BUILD_DIR)/test-benchmark

# =============================================================================
# Installation
# =============================================================================

PREFIX ?= /usr/local
LIBDIR := $(PREFIX)/lib
INCLUDEDIR := $(PREFIX)/include/atlas

install: all
	@echo "Installing Atlas runtime libraries..."
	@install -d $(LIBDIR) $(INCLUDEDIR)
	@install -m 644 $(STATIC_LIB) $(SHARED_LIB) $(LIBDIR)
	@install -m 644 $(INCLUDE_DIR)/*.h $(INCLUDEDIR)
	@ldconfig $(LIBDIR) 2>/dev/null || true

uninstall:
	@echo "Uninstalling Atlas runtime libraries..."
	@rm -f $(LIBDIR)/libatlas-runtime.* 
	@rm -rf $(INCLUDEDIR)
	@ldconfig $(LIBDIR) 2>/dev/null || true

# =============================================================================
# Documentation
# =============================================================================

docs:
	@echo "Generating documentation..."
	@doxygen Doxyfile 2>/dev/null || echo "Doxygen not available"

# =============================================================================
# Cleanup
# =============================================================================

clean:
	@echo "Cleaning build artifacts..."
	@rm -rf $(BUILD_DIR) $(LIB_DIR)
	@rm -f $(ROOT_DIR)/*.tmp $(ROOT_DIR)/*.log

distclean: clean
	@echo "Deep cleaning..."
	@rm -rf docs/html docs/latex

# =============================================================================
# Help
# =============================================================================

help:
	@echo "Atlas-12288 Layer 2 Host Runtime Build System"
	@echo ""
	@echo "Targets:"
	@echo "  all          - Build static and shared libraries"
	@echo "  clean        - Remove build artifacts"
	@echo "  distclean    - Deep clean including documentation"
	@echo "  test         - Build and run tests"
	@echo "  test-memory  - Run memory leak tests with Valgrind"
	@echo "  benchmark    - Run performance benchmarks"
	@echo "  install      - Install libraries and headers"
	@echo "  uninstall    - Remove installed files"
	@echo "  docs         - Generate documentation"
	@echo "  llvm-link    - Link LLVM IR modules"
	@echo "  help         - Show this help message"
	@echo ""
	@echo "Configuration:"
	@echo "  DEBUG=1           - Enable debug build with sanitizers"
	@echo "  WASM=1           - Build for WebAssembly target"
	@echo "  ATLAS_SINGLE_THREAD=1  - Disable thread safety for single-threaded use"
	@echo "  PREFIX=/path     - Set installation prefix (default: /usr/local)"
	@echo ""
	@echo "Examples:"
	@echo "  make                    - Standard build"
	@echo "  make DEBUG=1 test      - Debug build with tests"
	@echo "  make WASM=1            - WebAssembly build"
	@echo "  make install PREFIX=~/.local  - Install to home directory"

# =============================================================================
# Dependencies
# =============================================================================

# Ensure LLVM library is built first (optional - runtime can work with stubs)
ifneq ($(RUNTIME_STANDALONE),1)
$(RUNTIME_OBJECTS): | $(LLVM_LIB_DIR)/libatlas.so

$(LLVM_LIB_DIR)/libatlas.so:
	@echo "Building LLVM Atlas library dependency..."
	@$(MAKE) -C $(LLVM_DIR) || echo "Warning: LLVM library build failed, using stubs"
endif

# Header dependencies (simplified - in practice use makedepend or similar)
$(BUILD_DIR)/layer2.o: $(INCLUDE_DIR)/*.h $(LLVM_INCLUDE_DIR)/*.h
$(BUILD_DIR)/layer3.o: $(INCLUDE_DIR)/*.h $(LLVM_INCLUDE_DIR)/*.h

# =============================================================================
# Special Rules
# =============================================================================

# Prevent deletion of intermediate files
.PRECIOUS: $(BUILD_DIR)/%.o

# Force rebuild if Makefile changes
$(RUNTIME_OBJECTS): Makefile