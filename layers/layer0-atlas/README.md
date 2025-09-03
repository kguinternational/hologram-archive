# Layer 0: Atlas Core

## Overview

Layer 0 forms the foundational layer of the Atlas-12288 architecture, providing core types, intrinsics, and essential operations that all higher layers depend on. This layer is pure and stateless, focusing on mathematical operations and data structures.

## Components

### LLVM IR Modules
- `llvm/types.ll` - Core type definitions for Atlas-12288
- `llvm/intrinsics.ll` - LLVM intrinsic declarations
- `llvm/core.ll` - Core operations and functions  
- `llvm/simd.ll` - SIMD optimizations and vectorized operations
- `llvm/module.ll` - Main module coordination and runtime hooks

### Headers
- `include/atlas-core.h` - Core Atlas types and functions
- `include/atlas-c-api.h` - C API bindings

### Runtime Support
- `runtime-stubs.c` - LLVM runtime stub implementations

## Responsibilities

1. **Type System**: Defines canonical types for Atlas-12288:
   - `%atlas.byte`, `%atlas.page`, `%atlas.structure`
   - `%atlas.resonance`, `%atlas.budget`, `%atlas.klein`
   - `%atlas.coordinate`, `%atlas.boundary`, `%atlas.witness`

2. **Core Operations**: Fundamental mathematical operations:
   - Arithmetic and comparison operations
   - Memory layout and structure management
   - Basic type conversions

3. **SIMD Operations**: Vectorized implementations for performance:
   - AVX/AVX2 optimized routines
   - ARM NEON support
   - WebAssembly SIMD

4. **Runtime Infrastructure**: Platform initialization and cleanup:
   - Memory management setup
   - Platform-specific optimizations
   - Global constructors/destructors

## Dependencies

- **External**: Standard C library, LLVM runtime
- **Internal**: None (foundational layer)

## Interface Contract

Layer 0 provides a pure functional interface with no side effects or state. All operations are:
- **Deterministic**: Same inputs always produce same outputs
- **Thread-safe**: No shared mutable state
- **Memory-safe**: Bounds checking and validation
- **Performance-optimized**: SIMD where beneficial

## Tests

- `tests/test-simd.ll` - SIMD operation validation
- `tests/test-basic.c` - Basic runtime functionality

## Build Output

- Static library: `libatlas-core.a`
- Header installation: Public API headers

## Version

- Interface Version: 1.0.0
- Stability: Stable (breaking changes require major version bump)