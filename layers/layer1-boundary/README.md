# Layer 1: Boundary Layer

## Overview

Layer 1 manages the boundary structure of Atlas-12288, implementing coordinate systems, page management, Klein orbit operations, and the Φ isomorphism. This layer provides the spatial and algebraic foundation for higher-layer operations.

## Components

### LLVM IR Modules
- `llvm/coordinates.ll` - Coordinate system (48×256) and Φ isomorphism
- `llvm/pages.ll` - Page management and structure operations
- `llvm/klein.ll` - Klein orbit operations and canonicalization
- `llvm/phi.ll` - Φ-preserving morphisms and automorphisms

### Headers
- `include/atlas-boundary.h` - Boundary layer public API

## Responsibilities

1. **Coordinate System**: 48×256 coordinate management:
   - Encoding/decoding between (page, byte) and boundary representations
   - Coordinate validation and bounds checking
   - Distance calculations and navigation

2. **Page Management**: Atlas structure page operations:
   - Page pointer arithmetic and access
   - Page initialization and clearing
   - Copy and comparison operations
   - Integrity checking and validation

3. **Klein Orbits**: Klein group (V₄) orbit operations:
   - Orbit classification and canonicalization
   - Privileged orbit identification {0, 1, 48, 49}
   - Group action computations

4. **Φ Isomorphism**: Structure-preserving transformations:
   - Boundary automorphisms with unit multipliers
   - Morphism composition and inversion
   - Invariant preservation verification

## Mathematical Foundation

- **Coordinate Space**: ℤ₄₈ × ℤ₂₅₆ → ℤ₁₂₂₈₈
- **Klein Group**: V₄ = ℤ₂ × ℤ₂ acting on coordinates
- **Φ Map**: Canonical isomorphism preserving R96, C768 invariants
- **Unit Groups**: U(48) × U(256) for automorphisms

## Dependencies

- **Layer 0**: Core types, SIMD operations
- **External**: None beyond Layer 0

## Interface Contract

Layer 1 provides deterministic coordinate and page management with:
- **Coordinate Validation**: All operations validate bounds [0, 12288)
- **Klein Invariants**: Orbit operations preserve group structure
- **Φ Preservation**: Morphisms maintain isomorphism properties
- **Page Integrity**: Operations maintain 256-byte page alignment

## Tests

- `tests/test-klein.ll` - Klein orbit operations
- `tests/test-phi.ll` - Φ isomorphism and morphisms

## Build Output

- Static library: `libatlas-boundary.a`
- Depends on: `libatlas-core.a`

## Version

- Interface Version: 1.0.0
- Stability: Stable