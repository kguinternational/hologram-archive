# Atlas-12288 Layer 3 Host Runtime Implementation

This document describes the implementation of Layer 3 clustering and scheduling functionality for the Atlas-12288 runtime.

## Overview

The Layer 3 runtime provides C ABI functions for:
- **Spectra & Histogram Operations**: R96 resonance classification and histogram generation
- **CSR-based Clustering**: Efficient grouping of memory pages by resonance class
- **Harmonic Scheduling**: Time window calculation based on resonance properties
- **Memory Management**: WASM-compatible arena allocation for cluster views

## Implementation Files

- **`/workspaces/Hologram/runtime/src/layer3.c`**: Main Layer 3 implementation
- **`/workspaces/Hologram/runtime/include/atlas-runtime.h`**: Updated with Layer 3 API declarations
- **`/workspaces/Hologram/runtime/src/llvm-stubs.c`**: Stub implementations for LLVM IR functions
- **`/workspaces/Hologram/runtime/tests/test-layer3.c`**: Comprehensive test suite

## Core API Functions

### Classification and Histograms

```c
void atlas_r96_classify_page(const uint8_t* in256, uint8_t out256[256]);
void atlas_r96_histogram_page(const uint8_t* in256, uint16_t out96[96]);
```

These functions analyze 256-byte pages and produce resonance classifications or histograms of resonance class distributions.

### Clustering

```c
atlas_cluster_view atlas_cluster_by_resonance(const uint8_t* base, size_t pages);
void atlas_cluster_destroy(atlas_cluster_view* cluster);
```

The clustering system uses **Compressed Sparse Row (CSR)** format:
- **offsets[97]**: Start indices for each resonance class (0..95) plus terminator
- **indices[n]**: Page indices grouped by resonance class

This provides O(1) access to pages within specific resonance classes.

### Scheduling

```c
uint64_t atlas_next_harmonic_window_from(uint64_t now, uint8_t r);
uint64_t atlas_schedule_next_window(uint64_t now, uint8_t r);
```

Two scheduling approaches:
1. **Harmonic**: Uses complex LLVM IR-based harmonic analysis
2. **Simple**: Formula-based: `next = now + ((96 - ((now + r) % 96)) % 96)`

## CSR Clustering Format

The CSR format efficiently represents sparse resonance class distributions:

```
Pages: [P0, P1, P2, P3, P4]
Resonance classes: [1, 3, 1, 3, 1]

CSR representation:
- offsets[0] = 0    (resonance class 0: empty)
- offsets[1] = 0    (resonance class 1: starts at index 0)
- offsets[2] = 3    (resonance class 2: empty)  
- offsets[3] = 3    (resonance class 3: starts at index 3)
- offsets[4] = 5    (terminator)

- indices = [0, 2, 4, 1, 3]  (pages grouped by class)
```

## Memory Management

All memory allocation uses WASM-compatible functions:
- **`atlas_arena_alloc()`**: Aligned allocation for cluster structures
- **`atlas_arena_free()`**: Simple free implementation
- **Clean destruction**: `atlas_cluster_destroy()` handles all cleanup

## LLVM IR Integration

The implementation calls LLVM IR functions for core operations:
- `atlas_r96_classify_page_llvm()`
- `atlas_r96_histogram_page_llvm()`
- `atlas_page_resonance_class_llvm()`
- `atlas_next_harmonic_window_from_llvm()`
- `atlas_resonance_harmonizes_llvm()`

**Stub implementations** are provided for development/testing without full LLVM library.

## Usage Example

```c
#include "atlas-runtime.h"

// Prepare memory (3 pages × 256 bytes)
uint8_t* memory = aligned_alloc(256, 3 * 256);

// Initialize with test patterns
for (int page = 0; page < 3; page++) {
    uint8_t* page_ptr = memory + (page * 256);
    for (int i = 0; i < 256; i++) {
        page_ptr[i] = (page * 37 + i * 7) % 256;
    }
}

// Build cluster view
atlas_cluster_view cluster = atlas_cluster_by_resonance(memory, 3);

// Access pages by resonance class
for (uint8_t r = 0; r < 96; r++) {
    size_t count = atlas_cluster_count_for_resonance(cluster, r);
    if (count > 0) {
        const uint32_t* pages = atlas_cluster_pages_for_resonance(cluster, r, &count);
        printf("Resonance class %u: %zu pages\n", r, count);
    }
}

// Schedule next window
uint64_t current_time = 1000;
uint64_t next_window = atlas_next_harmonic_window_from(current_time, 42);

// Cleanup
atlas_cluster_destroy(&cluster);
free(memory);
```

## Build Integration

The Layer 3 implementation is integrated into the runtime build system:
- Added to `RUNTIME_SOURCES` in Makefile
- Symbols exported in both static and shared libraries
- Headers properly declared in `atlas-runtime.h`

## Testing

The test suite (`test-layer3.c`) validates:
- ✅ R96 classification and histogram generation
- ✅ CSR cluster building and structure validation
- ✅ Cluster access by resonance class
- ✅ Harmonic scheduling calculations
- ✅ Resonance harmonization checks
- ✅ Batch processing functions
- ✅ Memory management and cleanup

All tests pass with both stub and full LLVM implementations.

## Performance Characteristics

- **Classification**: O(1) per 256-byte page
- **Clustering**: O(n) where n = number of pages
- **Cluster Access**: O(1) lookup by resonance class
- **Scheduling**: O(1) harmonic window calculation
- **Memory**: Minimal overhead with arena allocation

## Mod-96 Arithmetic

All resonance operations use **mod-96 arithmetic**:
- Resonance classes: 0..95
- Harmonic calculations preserve mod-96 properties
- Classification ensures values stay within bounds

This implementation provides the complete Layer 3 functionality as specified in the Atlas-12288 architecture, with clean C ABI, efficient data structures, and comprehensive testing.