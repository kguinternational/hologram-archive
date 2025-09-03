# Layer 0: Atlas Core Documentation

## Overview
Layer 0 provides the fundamental computational substrate for the Hologram platform, implementing the 12,288-element Atlas structure and basic operations.

## Status
✅ **Complete**

## Components
- **R96 Classification**: Maps byte values (0-255) to 96 resonance classes
- **Conservation Checks**: Validates sum(bytes) % 96 == 0
- **Boundary Encoding**: Φ isomorphism for coordinate packing
- **LLVM Implementation**: Native LLVM IR with SIMD optimizations

## Interface
The Atlas Core layer exposes fundamental primitives to Layer 1 (Boundary):
- `classify()`: R96 classification (0..95)
- `verify_sum()`: Conservation check
- `encode_coord()`: Φ encoding
- `decode_coord()`: Coordinate decoding

## Implementation Files
- `layers/layer0-atlas/llvm/`: LLVM IR implementations
- `layers/layer0-atlas/include/atlas-core.h`: Core interface

## Invariants
- All operations preserve conservation (sum % 96 == 0)
- R96 classification is deterministic and bijective
- Coordinate encoding/decoding is lossless

## Testing
See `layers/layer0-atlas/tests/` for comprehensive test coverage.