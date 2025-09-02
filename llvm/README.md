# Hologram LLVM Implementation

This directory contains the LLVM-based implementation of the **Atlas‑12,288** computational model, which forms the foundation layer of the Hologram platform under the **UOR Foundation**.

> Repository: **https://github.com/UOR-Foundation/Hologram**  
> License: **MIT**

---

## Overview

Atlas‑12,288 is a conservation‑based computational model implemented as native LLVM IR. The model defines:

- **12,288 elements** organized as **48 pages × 256 bytes**
- **96 resonance classes** (R96) for classification
- **Conservation laws** ensuring information preservation
- **Witness generation** for verifiable provenance
- **Boundary/bulk isomorphism** (Φ) for coordinate mapping

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
│   └── atlas-12288-module.ll    # Complete top‑level module & smoke test
├── include/
│   ├── atlas.h                  # C header for FFI (stable surface)
│   └── atlas-c-api.h            # C API wrapper
├── lib/
│   └── (built libraries)
├── tests/
│   ├── test-r96.ll              # R96 tests
│   ├── test-conservation.ll     # Conservation tests
│   ├── test-witness.ll          # Witness tests
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
```

### Witness Generation
Creates verifiable provenance handles over buffers.

```llvm
declare ptr @atlas.witness.generate(ptr %data, i64 %len)
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

