# Makefile for Atlas Layer 4 Traditional/Baseline Benchmarks
# (c) 2024-2025 UOR Foundation - MIT License

BUILD_TYPE ?= release
CC ?= clang

TRADITIONAL_DIR := $(CURDIR)
LAYER4_DIR := $(abspath $(TRADITIONAL_DIR)/../..)
BUILD_DIR := $(TRADITIONAL_DIR)/build

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

CFLAGS := -std=c11 -Wall -Wextra -Wpedantic $(INCLUDES)

ifeq ($(BUILD_TYPE),debug)
    CFLAGS += -O0 -g -DDEBUG
else ifeq ($(BUILD_TYPE),release)
    CFLAGS += -O3 -DNDEBUG -march=native -flto
    LIBS += -flto
endif

# Traditional baseline implementations for comparison
SRCS := linear_algebra_baseline.c \
        differential_geometry_baseline.c \
        fft_baseline.c

BENCHMARKS := $(addprefix $(BUILD_DIR)/, $(SRCS:.c=))

.PHONY: all
all: $(BUILD_DIR) $(BENCHMARKS)

$(BUILD_DIR):
	@mkdir -p $(BUILD_DIR)

$(BUILD_DIR)/%.o: %.c
	@$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/%: $(BUILD_DIR)/%.o
	@$(CC) $(CFLAGS) $< $(LIBS) -o $@

.PHONY: clean
clean:
	@rm -rf $(BUILD_DIR)