# Hologram LLVM Implementation

This directory contains the LLVM-based implementation of the **Atlas‑12,288** computational model, which forms the foundation layer of the Hologram platform under the **UOR Foundation**.

> Repository: **https://github.com/UOR-Foundation/Hologram**  
> License: **MIT**

---

## Overview

Atlas‑12,288 is a conservation‑based computational model implemented as native LLVM IR. The model defines:

- **12,288 elements** organized as **48 pages × 256 bytes**
- **96 resonance classes** (R96) for classification with multiplicative structure
- **Conservation laws** ensuring information preservation (sum mod 96 == 0)
- **C768 triple-cycle** closure over 768 steps (16×48 = 3×256)
- **Klein orbits** using V₄ group structure for canonicalization
- **Witness generation** for verifiable provenance
- **Boundary/bulk isomorphism** (Φ) for coordinate mapping
- **Domain isolation** with budget transfers and witness chains
- **Harmonic operations** for resonance pairing and clustering
- **Statistical validation** with entropy and anomaly detection

The implementation targets **LLVM 15+** (opaque pointers) and provides portable SIMD paths for x86‑64 (SSE2/AVX2/AVX‑512), ARM64 (NEON), and WASM SIMD.

---

## Directory Structure

```
llvm/
├── Makefile                     # Build system (quick start)
├── README.md                    # This file
├── src/
│   ├── atlas-12288-types.ll     # Type definitions
│   ├── atlas-12288-intrinsics.ll# Intrinsic declarations (atlas.*)
│   ├── atlas-12288-r96.ll       # R96 classification (scalar + helpers)
│   ├── atlas-12288-ops.ll       # Core operations (boundary, conservation, budget, witness)
│   ├── atlas-12288-simd.ll      # SIMD/vector specializations (SSE2/AVX2/AVX‑512/NEON)
│   ├── atlas-12288-memory.ll    # Memory model (aligned alloc, conserved memset/memcpy)
│   ├── atlas-12288-c768.ll      # C768 triple-cycle conservation (768-step closure)
│   ├── atlas-12288-morphisms.ll # Structure-preserving maps (automorphisms, gauge, lifts)
│   ├── atlas-12288-klein.ll     # Klein orbit operations (V₄ group, canonicalization)
│   ├── atlas-12288-domains.ll   # Domain isolation, budget transfers, witness chains
│   ├── atlas-12288-harmonic.ll  # Harmonic pairing, resonance clustering, scheduling
│   ├── atlas-12288-validation.ll# Statistical validation, entropy, anomaly detection
│   ├── atlas-12288-acceptance.ll# Comprehensive acceptance tests for conformance
│   └── atlas-12288-module.ll    # Complete top‑level module & smoke test
├── include/
│   ├── atlas.h                  # C header for FFI (stable surface)
│   └── atlas-c-api.h            # C API wrapper
├── lib/
│   └── (built libraries)
├── tests/
│   ├── test-r96.ll              # R96 classification tests
│   ├── test-conservation.ll     # Conservation law tests
│   ├── test-witness.ll          # Witness generation tests
│   ├── test-simd.ll             # SIMD optimization tests
│   ├── test-c768.ll             # C768 triple-cycle tests
│   ├── test-morphisms.ll        # Morphism preservation tests
│   ├── test-klein.ll            # Klein orbit tests
│   ├── test-domains.ll          # Domain isolation tests
│   ├── test-harmonic.ll         # Harmonic operations tests
│   ├── test-acceptance.ll       # Full acceptance test suite
│   └── run-tests.sh             # Test runner
├── tools/
│   ├── atlas-opt                # Atlas optimizer (optional plugin)
│   ├── atlas-verify             # Verification tool
│   └── atlas-jit                # JIT harness
└── docs/
    ├── specification.md         # Full specification
    └── api.md                   # API documentation
```

---

## Requirements

- **LLVM 15+** (opaque pointers enabled)
- **CMake 3.20+** (optional; Makefile provided)
- **C++17** compatible compiler
- Target architectures: **x86‑64**, **ARM64**, or **WASM**

---

## Building

### Quick Build (Makefile)

```bash
# Build all components (libraries and tools)
make

# Run tests
make test               # Run all tests
make smoke-tests        # Run quick smoke tests

# Build with different configurations
make BUILD_TYPE=DEBUG   # Debug build with symbols
make BUILD_TYPE=RELEASE # Release build (default)

# Build for a specific target triple
make TARGET=aarch64-linux-gnu

# Clean
make clean              # Remove build artifacts
make distclean          # Remove all generated files
```

### Advanced Options

```bash
# Enable specific SIMD path
make SIMD=avx512         # one of: auto, avx2, avx512, neon

# Build with JIT support
make JIT=1 tools         # Build tools including JIT

# WebAssembly target
make TARGET=wasm32-unknown-unknown

# Installation
make install PREFIX=/opt/atlas  # Install to custom prefix
make uninstall                  # Remove installed files

# Documentation
make docs                # Generate documentation (requires Doxygen)

# Help
make help                # Show all targets and options
```

---

## Usage

### As LLVM Modules

```bash
# Link your IR with the Atlas module
llvm-link your-module.ll src/atlas-12288-module.ll -o combined.ll

# Optimize (vanilla pipeline)
opt -passes='default<O3>' combined.ll -o optimized.bc

# (Optional) run Atlas plugin passes if you built tools/atlas-opt
# opt -load-pass-plugin=lib/AtlasPass.so -passes='atlas-opt' optimized.bc -o optimized.bc

# Compile to native code
llc -O3 optimized.bc -o output.s
```

### As a C Library

```c
#include "atlas.h"

int main(void) {
    // Initialize Atlas runtime
    atlas_init();

    // Classify byte to resonance class
    uint8_t byte = 42;
    uint8_t resonance = atlas_r96_classify(byte); // 0..95

    // Check conservation
    uint8_t data[256] = {0};
    bool conserved = atlas_conserved_check(data, 256);

    // Generate witness
    atlas_witness_t w = atlas_witness_generate(data, 256);

    // Cleanup
    atlas_witness_destroy(w);
    atlas_cleanup();
    return 0;
}
```

### JIT Compilation (optional tool)

```c
#include "atlas.h"

atlas_jit_t* jit = atlas_jit_create();
atlas_jit_compile(jit, "atlas.r96.classify");

typedef uint8_t (*classify_fn)(uint8_t);
classify_fn classify = (classify_fn)atlas_jit_get_function(jit, "atlas.r96.classify");
uint8_t result = classify(42);

atlas_jit_destroy(jit);
```

---

## Core Operations (IR Signatures)

> **Note:** Intrinsics are declared under the **`atlas.*`** namespace (not `llvm.atlas.*`) and use **opaque pointers (`ptr`)**.

### R96 Classification
Maps 256 byte values to 96 resonance classes.

```llvm
declare i7 @atlas.r96.classify(i8 %byte)
declare i7 @atlas.r96.classify_simd.v16i8(<16 x i8> %bytes)  ; SSE2/NEON
```

### Boundary Encoding (Φ Isomorphism)
Encodes/decodes coordinates in the 48×256 space.

```llvm
declare i32 @atlas.boundary.encode(i16 %page, i8 %offset)
declare { i16, i8 } @atlas.boundary.decode(i32 %boundary)
```

### Conservation Verification
Ensures data maintains conservation laws (sum mod 96 == 0).

```llvm
declare i1 @atlas.conserved.check(ptr %data, i64 %len)
declare i64 @atlas.conserved.compute_deficit(ptr %data, i64 %len)
```

### C768 Triple-Cycle
Verifies 768-step closure and rhythm alignment.

```llvm
declare i1 @atlas.c768.verify_closure(ptr %structure, i64 %window_start)
declare i1 @atlas.c768.check_rhythm_alignment(ptr %structure)
```

### Klein Orbits
Canonical forms using V₄ group structure.

```llvm
declare i32 @atlas.klein.canonicalize_coord(i32 %coord)
declare i1 @atlas.klein.is_privileged_orbit(i32 %coord)
```

### Domain Operations
Isolated conservation contexts with budget management.

```llvm
declare ptr @atlas.domain.create(i7 %budget)
declare i1 @atlas.domain.transfer_budget(ptr %from, ptr %to, i7 %amount)
```

### Witness Generation
Creates verifiable provenance handles over buffers.

```llvm
declare ptr @atlas.witness.generate(ptr %data, i64 %len)
declare i1 @atlas.witness.verify(ptr %witness)
```

---

## Performance

### Optimization Levels

- **O0** – No optimization, debug friendly
- **O1** – Basic optimizations, conservation checks preserved
- **O2** – Standard optimizations with vectorization
- **O3** – Aggressive optimizations
- **Os** – Size‑optimized for embedded/wasm

### SIMD Support

- **SSE2/SSE4.2** – 16‑byte vectors (baseline x86‑64)
- **AVX2** – 32‑byte vectors (modern x86‑64)
- **AVX‑512** – 64‑byte vectors (server x86‑64)
- **NEON** – 16‑byte vectors (ARM64)
- **WASM SIMD** – 16‑byte vectors (WebAssembly)

---

## Testing

```bash
# Run all tests
make test

# Run specific categories
make test-r96            # R96 classification tests
make test-conservation   # Conservation law tests
make test-witness        # Witness generation tests
make test-simd           # SIMD optimization tests
make test-c768           # C768 triple-cycle tests
make test-morphisms      # Morphism preservation tests
make test-klein          # Klein orbit tests
make test-domains        # Domain isolation tests
make test-harmonic       # Harmonic operations tests
make test-acceptance     # Full acceptance suite

# Memory checks (e.g., with Valgrind)
make test-memory

# Benchmarks
make benchmark
```

---

## Integration

### With the Hologram stack

The LLVM implementation provides the low‑level computational model for higher‑level Hologram components (C/C++, Rust, Go, Python, Node). Typical SDKs bind to the C API layer and call into the IR‑backed operations.

### Language Bindings

- **Rust** – via `atlas-sys` (FFI) crate
- **Go** – via CGO
- **Python** – via `ctypes`/`cffi`
- **Node.js** – via N‑API

(See `include/` and `docs/api.md` for exact function signatures.)

---

## Module Architecture

The implementation is organized into layered modules:

### Core Modules (Layer 0-1)
- **types** – Type definitions and structures
- **intrinsics** – Function declarations under `atlas.*` namespace
- **r96** – R96 resonance classification (96 classes from 256 bytes)
- **ops** – Core operations (boundary, conservation, witness)

### Mathematical Modules (Layer 2-3)
- **c768** – Triple-cycle conservation (768-step closure verification)
- **morphisms** – Structure-preserving maps maintaining invariants
- **klein** – Klein V₄ group orbits for canonicalization
- **harmonic** – Resonance pairing and harmonic clustering

### System Modules (Layer 4-5)
- **domains** – Isolated conservation contexts with budget management
- **memory** – Aligned allocation and conserved memory operations
- **simd** – SIMD optimizations for multiple architectures

### Validation Modules
- **validation** – Statistical validation, entropy, anomaly detection
- **acceptance** – Comprehensive conformance test suite

## Documentation

- **Specification:** `docs/specification.md` – Complete Atlas‑12288 spec
- **API Reference:** `docs/api.md` – C/FFI surface
- **Optimization Guide:** `docs/optimization.md`
- **Examples:** `docs/examples.md`

---

## Contributing

Contributions are welcome! Please see the main repository’s guidelines:  
**UOR‑Foundation/Hologram →** `CONTRIBUTING.md`

---

## License

**MIT License** – see [`LICENSE`](../LICENSE) for details.

---

## Support

- **Issues:** https://github.com/UOR-Foundation/Hologram/issues
- **Discussions:** https://github.com/UOR-Foundation/Hologram/discussions
- **Website:** https://uor.foundation

