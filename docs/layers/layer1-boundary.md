# Layer 1: Boundary Layer Documentation

## Overview
Layer 1 manages the coordinate system and spatial organization of the 12,288 structure (48 pages × 256 bytes).

## Status
✅ **Complete**

## Components
- **Coordinate System**: 48×256 grid addressing
- **Page Management**: Page allocation and validation
- **Boundary Encoding**: Φ isomorphism for coordinate packing
- **Klein Orbits**: 4 privileged orbit classes for alignment

## Interface
The Boundary layer exposes spatial operations to Layer 2 (Conservation):
- `encode()`: Convert (page, offset) to boundary coordinate
- `decode()`: Extract page and offset from boundary coordinate
- `get_page()`: Retrieve page by index
- `page_valid()`: Validate page structure
- `get_klein_orbit()`: Determine Klein orbit class (0..3)
- `transform()`: Apply resonance transformation

## Implementation Files
- `layers/layer1-boundary/llvm/`: LLVM IR implementations
- `layers/layer1-boundary/include/atlas-boundary.h`: Boundary interface

## Invariants
- Valid pages: 0..47
- Valid offsets: 0..255
- Boundary encoding is bijective
- Klein orbits tile the 12,288 space without gaps

## Testing
See `layers/layer1-boundary/tests/` for comprehensive test coverage.