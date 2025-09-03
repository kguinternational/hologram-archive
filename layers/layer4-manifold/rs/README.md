# Atlas Manifold - Layer 4 Rust Implementation

This directory contains the Rust implementation for Layer 4 (Manifold) of the Atlas Hologram system.

## Structure

- `Cargo.toml` - Project configuration with FFI support and minimal dependencies
- `build.rs` - Build script for linking with Layer 2 and Layer 3 libraries
- `src/lib.rs` - Main library entry point with C exports
- `src/types.rs` - Core type definitions (AtlasPoint, AtlasVector, TransformMatrix, etc.)
- `src/error.rs` - Error handling types and utilities
- `src/linear.rs` - Linear algebra operations and transformations
- `src/manifold.rs` - Core manifold operations and geometric primitives
- `src/coords.rs` - Coordinate system transformations
- `src/shard.rs` - Data sharding and distribution for parallel operations
- `src/tlv.rs` - Type-Length-Value serialization support
- `src/ffi.rs` - Foreign Function Interface for C interoperability

## Key Features

- **Type-safe manifold operations** with compile-time dimension checking
- **Multiple coordinate systems** (Cartesian, Spherical, Cylindrical, etc.)
- **Data sharding** for distributed computation
- **TLV serialization** for efficient data interchange
- **C FFI** for integration with other layers
- **no_std compatible** (can be configured for embedded systems)

## Building

```bash
cargo build --release
```

This generates both static and dynamic libraries suitable for linking with C/C++ code.

## Integration

The build script automatically links with:
- Layer 2 (Conservation) libraries from `../../layer2-conservation/lib/`
- Layer 3 (Resonance) libraries from `../../layer3-resonance/build/`

Environment variables can override default paths:
- `ATLAS_LAYER2_PATH` - Override Layer 2 path
- `ATLAS_LAYER3_PATH` - Override Layer 3 path