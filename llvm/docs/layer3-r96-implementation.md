# Layer 3 (Resonance) R96 LLVM IR Implementation

## Overview

This document describes the Layer 3 (Resonance) LLVM IR kernels implemented for the Atlas-12288 system. These kernels are hot-path optimized for GB/s processing speeds and include explicit vectorization hints.

## Core Functions Implemented

### 1. `atlas.r96.classify.page.ptr(ptr in256, ptr out256)`

**Purpose**: Classify a full page (256 bytes) to 256 resonance classes using pointer-based interface.

**Features**:
- Processes 256 input bytes, producing 256 classification outputs
- Uses the existing pure modulo R96 classification logic
- Output stored as i8 (zero-extended i7 classes)
- Optimized for SIMD auto-vectorization with 16-element vector width hints
- `nounwind readonly willreturn` attributes for optimization

**Performance**: Designed to achieve GB/s speeds when vectorized by LLVM.

### 2. `atlas.r96.histogram.page(ptr in256, ptr out96_u16)`

**Purpose**: Generate a 96-bin histogram from a 256-byte page using 16-bit counters.

**Features**:
- Processes full pages (256 bytes) efficiently
- Uses 16-bit counters to handle up to 65,535 counts per bin
- Automatically zeros histogram before processing
- 8-element vector width hint (conservative due to data dependencies)
- 192 bytes output (96 × 2 bytes)

**Note**: Histogram accumulation has data dependencies that limit vectorization, so the vector width is set conservatively.

### 3. `atlas.r96.harmonizes(i7 r1, i7 r2) -> i1`

**Purpose**: Check if two R96 resonance classes harmonize using the condition `(r1 + r2) % 96 == 0`.

**Features**:
- Pure computation with no memory access
- Zero-extends i7 to i16 to avoid overflow
- `nounwind readnone willreturn` for maximum optimization
- Can be inlined and vectorized easily

## SIMD Helper Functions

### `atlas.r96.classify.page.v16(ptr in16, ptr out16)`
- Processes 16 bytes using existing vector classification
- Demonstrates explicit vectorization approach
- Useful for block-level processing

### `atlas.r96.histogram.block32(ptr in32, ptr hist96_u16)`
- Processes 32-byte blocks for histogram accumulation
- Can be used to build larger histograms incrementally
- Atomic-style updates for potential parallelization

### `atlas.r96.harmonizes.batch(ptr r1_array, ptr r2_array, ptr results, i32 count)`
- Batch harmonization checking
- Processes arrays of resonance class pairs
- Results stored as i8 boolean values
- Vectorizable for large-scale harmony analysis

## Optimization Features

### Loop Vectorization Hints
- **Classification loop**: 16-element vector width for optimal SIMD utilization
- **Histogram loop**: 8-element vector width (conservative due to dependencies)
- Explicit `llvm.loop.vectorize.enable` metadata

### Function Attributes
- **Pure functions** (`#0`): `nounwind readnone willreturn speculatable`
- **Read-only functions** (`#1`): `nounwind readonly willreturn`
- Proper alignment hints for memory operations

### Memory Access Patterns
- Sequential access patterns for optimal cache utilization
- Aligned loads/stores where possible
- Efficient pointer arithmetic

## Integration

### Intrinsics Declaration
All Layer 3 functions are declared in `atlas-12288-intrinsics.ll` with proper attributes:

```llvm
declare void @atlas.r96.classify.page.ptr(ptr %in256, ptr %out256) #1
declare void @atlas.r96.histogram.page(ptr %in256, ptr %out96_u16) #1
declare i1 @atlas.r96.harmonizes(i7 %r1, i7 %r2) #0
```

### Compilation
The implementation compiles cleanly with LLVM 15+ and uses opaque pointers throughout for forward compatibility.

## Performance Expectations

When compiled with optimizations (`-O3`) and vectorization enabled:

1. **Classification**: Expected to achieve 1-4 GB/s throughput depending on target architecture
2. **Histogram**: Limited by memory bandwidth and data dependencies, expect 0.5-2 GB/s
3. **Harmonization**: Extremely fast, limited mainly by memory bandwidth for batch operations

## Usage Patterns

### High-Throughput Page Processing
```c
// Classify pages in streaming fashion
for (int i = 0; i < num_pages; i++) {
    atlas.r96.classify.page.ptr(&input_pages[i*256], &output_pages[i*256]);
}
```

### Real-Time Histogram Analysis
```c
// Generate histogram for analysis
uint16_t histogram[96];
atlas.r96.histogram.page(page_data, histogram);
// Find dominant resonance class from histogram
```

### Harmony Detection
```c
// Check if two resonance classes work together
bool compatible = atlas.r96.harmonizes(class1, class2);
```

The Layer 3 implementation provides the foundation for high-performance resonance processing in the Atlas-12288 system, with careful attention to vectorization and cache optimization.