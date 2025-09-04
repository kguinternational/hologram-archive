# Makefile for Atlas Layer 4 Geometric Benchmarks
# (c) 2024-2025 UOR Foundation - MIT License

# Build configuration
BUILD_TYPE ?= release
CC ?= clang

# Directories
GEOMETRIC_DIR := $(CURDIR)
LAYER4_DIR := $(abspath $(GEOMETRIC_DIR)/../..)
BUILD_DIR := $(GEOMETRIC_DIR)/build

# Include paths
INCLUDES := -I$(LAYER4_DIR)/include \
           -I$(LAYER4_DIR)/../../../include \
           -I$(LAYER4_DIR)/../layer0-atlas/include \
           -I$(LAYER4_DIR)/../layer1-boundary/include \
           -I$(LAYER4_DIR)/../layer2-conservation/include \
           -I$(LAYER4_DIR)/../layer3-resonance/include

# Libraries
LIBS := -L$(LAYER4_DIR)/build -latlas-manifold \
        -L$(LAYER4_DIR)/../layer3-resonance/build -latlas-resonance \
        -L$(LAYER4_DIR)/../layer2-conservation/build -latlas-conservation \
        -L$(LAYER4_DIR)/../layer1-boundary/build -latlas-boundary \
        -L$(LAYER4_DIR)/../layer0-atlas/build -latlas-core \
        -lm -lpthread

# Compiler flags
CFLAGS := -std=c11 -Wall -Wextra -Wpedantic $(INCLUDES)

# Build type specific flags
ifeq ($(BUILD_TYPE),debug)
    CFLAGS += -O0 -g -DDEBUG
else ifeq ($(BUILD_TYPE),release)
    CFLAGS += -O3 -DNDEBUG -march=native -flto
    LIBS += -flto
else
    $(error Invalid BUILD_TYPE: $(BUILD_TYPE). Use 'debug' or 'release')
endif

# Source files
SRCS := harmonic_distance.c \
        spectral_curvature.c \
        manifold_projections.c

# Object files
OBJS := $(addprefix $(BUILD_DIR)/, $(SRCS:.c=.o))

# Benchmark executables
BENCHMARKS := $(addprefix $(BUILD_DIR)/, $(SRCS:.c=))

# Default target
.PHONY: all
all: $(BUILD_DIR) $(BENCHMARKS)

# Create build directory
$(BUILD_DIR):
	@mkdir -p $(BUILD_DIR)

# Compile object files
$(BUILD_DIR)/%.o: %.c
	@echo "Compiling $<..."
	@$(CC) $(CFLAGS) -c $< -o $@

# Link benchmarks
$(BUILD_DIR)/%: $(BUILD_DIR)/%.o
	@echo "Linking $@..."
	@$(CC) $(CFLAGS) $< $(LIBS) -o $@

# Individual targets
.PHONY: distance curvature projections
distance: $(BUILD_DIR)/harmonic_distance
curvature: $(BUILD_DIR)/spectral_curvature  
projections: $(BUILD_DIR)/manifold_projections

# Run all geometric benchmarks
.PHONY: run
run: all
	@echo "Running geometric benchmarks..."
	@for bench in $(BENCHMARKS); do \
		echo "Running $$bench..."; \
		$$bench || true; \
	done

# Clean
.PHONY: clean
clean:
	@echo "Cleaning geometric benchmarks..."
	@rm -rf $(BUILD_DIR)

# Help
.PHONY: help
help:
	@echo "Atlas Layer 4 Geometric Benchmarks"
	@echo ""
	@echo "Targets:"
	@echo "  all          Build all geometric benchmarks"
	@echo "  distance     Build harmonic distance benchmark"
	@echo "  curvature    Build spectral curvature benchmark" 
	@echo "  projections  Build manifold projections benchmark"
	@echo "  run          Run all benchmarks"
	@echo "  clean        Remove build artifacts"