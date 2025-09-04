# Makefile for Atlas Layer 4 Signal Processing Benchmarks
# (c) 2024-2025 UOR Foundation - MIT License

# Build configuration
BUILD_TYPE ?= release
CC ?= clang

# Directories  
SIGNAL_DIR := $(CURDIR)
LAYER4_DIR := $(abspath $(SIGNAL_DIR)/../..)
BUILD_DIR := $(SIGNAL_DIR)/build

# Include paths and libraries (same pattern as other suites)
INCLUDES := -I$(LAYER4_DIR)/include \
           -I$(LAYER4_DIR)/../../../include \
           -I$(LAYER4_DIR)/../layer0-atlas/include \
           -I$(LAYER4_DIR)/../layer1-boundary/include \
           -I$(LAYER4_DIR)/../layer2-conservation/include \
           -I$(LAYER4_DIR)/../layer3-resonance/include

LIBS := -L$(LAYER4_DIR)/build -latlas-manifold \
        -L$(LAYER4_DIR)/../layer3-resonance/build -latlas-resonance \
        -L$(LAYER4_DIR)/../layer2-conservation/build -latlas-conservation \
        -L$(LAYER4_DIR)/../layer1-boundary/build -latlas-boundary \
        -L$(LAYER4_DIR)/../layer0-atlas/build -latlas-core \
        -lm -lpthread

# Compiler flags
CFLAGS := -std=c11 -Wall -Wextra -Wpedantic $(INCLUDES)

ifeq ($(BUILD_TYPE),debug)
    CFLAGS += -O0 -g -DDEBUG
else ifeq ($(BUILD_TYPE),release)
    CFLAGS += -O3 -DNDEBUG -march=native -flto
    LIBS += -flto
endif

# Source files (R96 Fourier transforms, harmonic filtering, spectral analysis)
SRCS := r96_fourier.c \
        harmonic_filtering.c \
        resonance_convolution.c

BENCHMARKS := $(addprefix $(BUILD_DIR)/, $(SRCS:.c=))

.PHONY: all
all: $(BUILD_DIR) $(BENCHMARKS)

$(BUILD_DIR):
	@mkdir -p $(BUILD_DIR)

$(BUILD_DIR)/%.o: %.c
	@echo "Compiling $<..."
	@$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/%: $(BUILD_DIR)/%.o
	@echo "Linking $@..."
	@$(CC) $(CFLAGS) $< $(LIBS) -o $@

.PHONY: clean
clean:
	@rm -rf $(BUILD_DIR)