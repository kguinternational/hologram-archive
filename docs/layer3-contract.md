# Layer 3 (Resonance) Contract

## Overview

Layer 3 provides resonance classification, clustering, and harmonic scheduling for the Atlas-12288 computational model. It maps the 256 byte values to 96 resonance classes and organizes data by harmonic affinity.

## Core Invariants

1. **R96 Partition**: Every byte maps to exactly one of 96 resonance classes
2. **Harmonic Pairs**: Two resonances harmonize if `(r1 + r2) % 96 == 0`
3. **Cluster Homogeneity**: Each cluster contains only elements of the same resonance
4. **Coverage**: Clustering covers all input elements exactly once
5. **Scheduling Determinism**: Same inputs always produce same schedule

## API Signatures

### Classification & Histograms
```c
// Classify 256 bytes to 256 resonance classes
void atlas_r96_classify_page(const uint8_t* in256, uint8_t out256[256]);

// Generate 96-bin histogram from 256 bytes
void atlas_r96_histogram_page(const uint8_t* in256, uint16_t out96[96]);
```

### Clustering (CSR Format)
```c
// Cluster view structure (Compressed Sparse Row)
typedef struct {
    const uint32_t* offsets;  // Length 97: offsets[r]..offsets[r+1]-1 = class r
    const uint32_t* indices;  // Length n: linearized coordinates
    uint32_t n;              // Total number of elements
} atlas_cluster_view;

// Build clusters from pages (base points to pages*256 bytes)
atlas_cluster_view atlas_cluster_by_resonance(const uint8_t* base, size_t pages);

// Destroy cluster and free resources
void atlas_cluster_destroy(atlas_cluster_view* view);
```

### Harmonic Scheduling
```c
// Compute next harmonic window for resonance r starting from time 'now'
uint64_t atlas_next_harmonic_window_from(uint64_t now, uint8_t r);

// Convenience: next window from t=0
static inline uint64_t atlas_next_harmonic_window(uint8_t r) {
    return atlas_next_harmonic_window_from(0, r);
}
```

## LLVM IR Kernels (Hot Paths)

```llvm
; Classify page with SIMD optimization
declare void @atlas.r96.classify.page.ptr(ptr in256, ptr out256)

; Generate histogram with vectorization
declare void @atlas.r96.histogram.page(ptr in256, ptr out96_u16)

; Check if two resonances harmonize
declare i1 @atlas.r96.harmonizes(i7 r1, i7 r2)
```

## Data Formats

### Spectrum Representation
- **Type**: `uint8_t[256]`
- **Range**: Each element in [0, 95]
- **Layout**: Sequential, one class per input byte

### Histogram Format
- **Type**: `uint16_t[96]`
- **Range**: Each bin can count up to 65,535
- **Layout**: bin[r] = count of class r occurrences

### CSR Cluster Format
- **Offsets**: `uint32_t[97]` where element r spans `[offsets[r], offsets[r+1])`
- **Indices**: `uint32_t[n]` containing linearized coordinates
- **Coordinate**: `coord = page * 256 + offset` (range [0, 12287])
- **Ordering**: Indices within each class are in ascending order

## Performance Characteristics

| Operation | Target Throughput | Notes |
|-----------|------------------|-------|
| Classify page | ≥10 GB/s (AVX2) | 256-byte pages |
| Histogram page | ≥8 GB/s (AVX2) | With bin updates |
| Cluster build (48 pages) | <1 ms | CSR construction |
| Harmonic scheduling | <10 ns | Per decision |
| SIMD classify (16 bytes) | 1 cycle/byte | Vectorized path |

## R96 Classification Algorithm

The R96 classification uses a multiplicative structure over 8 toggle bits:
```
R(byte) = ∏(i=0..7) α[i]^bit[i]
```

With constraints:
- One unity pair: `α[u] * α[v] = 1`
- One pinned oscillator: `α[0] = 1`
- Results in exactly 96 classes from 256 values

## Harmonic Scheduling Formula

```c
next = now + ((96 - ((now + r) % 96)) % 96)
```

This ensures:
- Phase-locked to 96-beat rhythm
- Deterministic for any (now, r) pair
- Uniform distribution over time

## Usage Examples

### Page Classification
```c
uint8_t input[256];
uint8_t spectrum[256];

// Classify entire page
atlas_r96_classify_page(input, spectrum);

// Each spectrum[i] is now in range [0, 95]
for (int i = 0; i < 256; i++) {
    assert(spectrum[i] < 96);
}
```

### Histogram Generation
```c
uint8_t page[256];
uint16_t histogram[96] = {0};

// Generate histogram
atlas_r96_histogram_page(page, histogram);

// Total should equal 256
uint32_t total = 0;
for (int i = 0; i < 96; i++) {
    total += histogram[i];
}
assert(total == 256);
```

### Clustering Multiple Pages
```c
// Cluster 48 pages
uint8_t pages[48][256];
atlas_cluster_view clusters = atlas_cluster_by_resonance(
    (uint8_t*)pages, 48
);

// Access all elements of resonance class 42
uint32_t start = clusters.offsets[42];
uint32_t end = clusters.offsets[43];
for (uint32_t i = start; i < end; i++) {
    uint32_t coord = clusters.indices[i];
    uint32_t page = coord / 256;
    uint32_t offset = coord % 256;
    // Process element at pages[page][offset]
}

// Clean up
atlas_cluster_destroy(&clusters);
```

### Harmonic Scheduling
```c
// Schedule next window for resonance 24
uint64_t now = 1000;
uint64_t next = atlas_next_harmonic_window_from(now, 24);

// Check harmonization
if (atlas_r96_harmonizes(24, 72)) {
    // 24 + 72 = 96 ≡ 0 (mod 96), so they harmonize
}
```

## Memory Model

- **Input**: Read-only, no alignment requirements
- **Output**: Caller-allocated, no special alignment
- **CSR**: Internally allocated, freed by destroy function
- **Thread Safety**: All functions are thread-safe for different data

## Error Handling

- Classification and histogram functions always succeed
- Cluster functions return empty view on allocation failure
- NULL to destroy functions is safe (no-op)
- Scheduling functions always succeed (pure computation)

## WASM Compatibility

- Simple malloc/free for cluster allocation
- No atomics required (stateless operations)
- Identical outputs across all platforms
- SIMD optimizations via wasm-simd when available

## Conformance Requirements

An implementation is conformant if:
1. R96 classification is bijective and total
2. Histograms sum to input size
3. Clusters are homogeneous by resonance
4. CSR indices are strictly ascending within each class
5. Harmonic pairs satisfy `(r1 + r2) % 96 == 0`
6. Scheduling is deterministic and phase-locked