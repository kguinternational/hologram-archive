# Layer 4 (Manifold) Implementation Guide

## Overview
This guide provides detailed instructions for implementing Layer 4 (Manifold) in Rust while maintaining C ABI compatibility with the existing Hologram stack.

## Design Principles

### Core Tenets
1. **C ABI Contract**: The C header (`atlas-manifold.h`) defines the public interface
2. **No FFI Unwinding**: All panics abort; functions return status codes or null
3. **Zero-Copy Design**: Operate on caller-provided buffers with minimal allocation
4. **Stateless Operations**: No global state; all context in opaque handles
5. **Cross-Platform**: Support x86_64, AArch64, and WASM targets

### Safety Requirements
- Use `#![deny(unsafe_op_in_unsafe_fn)]` throughout
- Validate all pointers and lengths at FFI boundaries
- Never expose Rust types directly through FFI
- Abort on panic rather than unwinding

## Repository Structure

```
layers/layer4-manifold/
├── include/
│   └── atlas-manifold.h         # C ABI (authoritative)
├── rs/
│   ├── Cargo.toml               # Rust package configuration
│   ├── build.rs                 # Build script for linking
│   └── src/
│       ├── lib.rs               # FFI exports and handle wrappers
│       ├── error.rs             # Error type mappings
│       ├── types.rs             # FFI-safe structures
│       ├── tlv.rs               # TLV serialization
│       ├── linear.rs            # LINEAR projection implementation
│       ├── r96_fourier.rs       # R96_FOURIER implementation
│       ├── shard.rs             # Shard management
│       └── verify.rs            # Verification routines
├── tests/
│   ├── ffi_roundtrip.c         # C interop tests
│   ├── golden_linear.rs        # Known-good test cases
│   ├── golden_r96.rs           # R96 reconstruction tests
│   └── fuzz_shards.rs          # Property-based testing
└── benches/
    ├── bench_build.rs           # Build performance
    ├── bench_extract.rs         # Extraction benchmarks
    └── bench_reconstruct.rs     # Reconstruction benchmarks
```

## Build Configuration

### Cargo.toml
```toml
[package]
name = "atlas-manifold"
version = "0.1.0"
edition = "2021"
license = "MIT"

[lib]
crate-type = ["cdylib", "staticlib"]

[features]
wasm = []                # WebAssembly build
metrics = []             # Internal performance counters

[profile.release]
panic = "abort"          # Never unwind across FFI
lto = true
codegen-units = 1
opt-level = 3

[dependencies]
bytemuck = "1"           # Safe transmutes for TLV
thiserror = "1"          # Error handling

[dev-dependencies]
proptest = "1"           # Property testing
criterion = "0.5"        # Benchmarking
```

### Build Script (build.rs)
```rust
fn main() {
    // Link to Layer 2 and Layer 3 libraries
    if let Ok(lib_dir) = std::env::var("HOLOGRAM_LIB_DIR") {
        println!("cargo:rustc-link-search=native={}", lib_dir);
        println!("cargo:rustc-link-lib=static=atlas-conservation");
        println!("cargo:rustc-link-lib=static=atlas-resonance");
    }
}
```

## FFI Interface

### Opaque Handle Types
```rust
// Never expose internal structure
#[repr(C)]
pub struct atlas_projection_t {
    _private: [u8; 0],
}

#[repr(C)]
pub struct atlas_shard_t {
    _private: [u8; 0],
}
```

### Exported Functions
```rust
#[no_mangle]
pub extern "C" fn atlas_proj_create(
    domain: *const c_void,
    proj_type: u32,
) -> *mut atlas_projection_t {
    if domain.is_null() {
        return std::ptr::null_mut();
    }
    
    match create_projection_internal(domain, proj_type) {
        Ok(proj) => Box::into_raw(Box::new(proj)) as *mut _,
        Err(_) => std::ptr::null_mut(),
    }
}

#[no_mangle]
pub extern "C" fn atlas_proj_destroy(
    proj: *mut atlas_projection_t,
) {
    if !proj.is_null() {
        unsafe {
            drop(Box::from_raw(proj as *mut Projection));
        }
    }
}
```

### Layer 2/3 Imports
```rust
#[link(name = "atlas-conservation")]
extern "C" {
    fn atlas_witness_generate(data: *const u8, len: usize) -> *mut c_void;
    fn atlas_witness_verify(witness: *const c_void, data: *const u8, len: usize) -> bool;
    fn atlas_domain_verify(domain: *const c_void) -> bool;
}

#[link(name = "atlas-resonance")]
extern "C" {
    fn atlas_r96_classify_page(input: *const u8, output: *mut u8);
    fn atlas_r96_histogram_page(input: *const u8, histogram: *mut u16);
}
```

## Internal Implementation

### Projection Structure
```rust
pub struct Projection {
    proj_type: ProjectionType,
    dimensions: u32,
    timestamp: u64,
    domain_witness: *mut c_void,  // From Layer 2
    
    // LINEAR specific
    tiles: Vec<u8>,               // 48 * 256 bytes
    block_deltas: Vec<u8>,        // Conservation corrections
    
    // R96_FOURIER specific
    coefficients: Vec<f32>,       // Harmonic coefficients
    class_histogram: Vec<u16>,    // 96 resonance classes
}
```

### Shard Structure
```rust
pub struct Shard {
    proj_type: ProjectionType,
    region: BoundaryRegion,
    checksum: u32,
    witness: *mut c_void,          // Layer 2 witness
    payload: Vec<u8>,              // TLV-encoded data
}
```

## Algorithm Implementation

### LINEAR Projection Build
```rust
pub fn build_linear(domain: &[u8]) -> Result<Projection> {
    let mut proj = Projection::new_linear();
    
    // Copy domain data to tiles
    proj.tiles = domain.to_vec();
    
    // Compute conservation deltas per block
    for (i, chunk) in domain.chunks(256).enumerate() {
        let sum: u32 = chunk.iter().map(|&b| b as u32).sum();
        let delta = (96 - (sum % 96)) % 96;
        proj.block_deltas.push(delta as u8);
        
        // Apply correction to maintain conservation
        if delta != 0 {
            let last_idx = i * 256 + 255;
            proj.tiles[last_idx] = proj.tiles[last_idx].wrapping_add(delta as u8);
        }
    }
    
    // Generate witness via Layer 2
    unsafe {
        proj.domain_witness = atlas_witness_generate(
            proj.tiles.as_ptr(),
            proj.tiles.len(),
        );
    }
    
    Ok(proj)
}
```

### Shard Extraction
```rust
pub fn extract_shard(proj: &Projection, region: BoundaryRegion) -> Result<Shard> {
    // Convert region to linear coordinates
    let start = region.page_lo as usize * 256 + region.offset_lo as usize;
    let end = region.page_hi as usize * 256 + region.offset_hi as usize;
    
    // Validate bounds
    if end > proj.tiles.len() {
        return Err(Error::InvalidRegion);
    }
    
    // Extract data slice
    let data = &proj.tiles[start..end];
    
    // Build shard with witness
    let mut shard = Shard {
        proj_type: proj.proj_type,
        region,
        checksum: calculate_checksum(data),
        witness: std::ptr::null_mut(),
        payload: Vec::new(),
    };
    
    // Serialize to TLV
    shard.payload = serialize_tlv(data, &proj.block_deltas[start/256..end/256])?;
    
    // Generate shard witness
    unsafe {
        shard.witness = atlas_witness_generate(
            shard.payload.as_ptr(),
            shard.payload.len(),
        );
    }
    
    Ok(shard)
}
```

## Serialization (TLV Format)

### Header Structures
```rust
#[repr(C, packed)]
#[derive(Copy, Clone, bytemuck::Pod, bytemuck::Zeroable)]
struct TlvHeader {
    tag: u32,      // Type identifier
    length: u32,   // Payload length
    version: u16,  // Format version
    reserved: u16,
}

#[repr(C, packed)]
#[derive(Copy, Clone, bytemuck::Pod, bytemuck::Zeroable)]
struct ShardHeader {
    proj_type: u32,
    page_lo: u16,
    page_hi: u16,
    offset_lo: u16,
    offset_hi: u16,
    checksum: u32,
}
```

### Serialization
```rust
pub fn serialize_tlv(data: &[u8], metadata: &[u8]) -> Result<Vec<u8>> {
    let mut buffer = Vec::with_capacity(data.len() + metadata.len() + 64);
    
    // Write header
    let header = TlvHeader {
        tag: TLV_TAG_SHARD,
        length: (data.len() + metadata.len()) as u32,
        version: 1,
        reserved: 0,
    };
    buffer.extend_from_slice(bytemuck::bytes_of(&header));
    
    // Write shard metadata
    buffer.extend_from_slice(metadata);
    
    // Write data payload
    buffer.extend_from_slice(data);
    
    Ok(buffer)
}
```

## Testing Strategy

### Unit Tests
```rust
#[test]
fn test_linear_conservation() {
    let mut data = vec![0u8; 12288];
    // Make data conserved
    let sum: u32 = data.iter().map(|&b| b as u32).sum();
    if sum % 96 != 0 {
        data[0] = data[0].wrapping_add((96 - (sum % 96)) as u8);
    }
    
    let proj = build_linear(&data).unwrap();
    assert!(verify_conservation(&proj.tiles));
}
```

### Property Tests
```rust
use proptest::prelude::*;

proptest! {
    #[test]
    fn test_shard_reconstruction(
        regions in prop::collection::vec(region_strategy(), 1..10)
    ) {
        let proj = create_test_projection();
        let shards: Vec<_> = regions.iter()
            .map(|r| extract_shard(&proj, *r).unwrap())
            .collect();
        
        let reconstructed = reconstruct_from_shards(&shards).unwrap();
        prop_assert_eq!(reconstructed.tiles, proj.tiles);
    }
}
```

### FFI Tests (C)
```c
void test_ffi_roundtrip() {
    // Create domain via Layer 2
    atlas_domain_t* domain = atlas_domain_create(12288, 0);
    
    // Create projection
    atlas_projection_t* proj = atlas_proj_create(domain, PROJ_LINEAR);
    assert(proj != NULL);
    
    // Extract shard
    atlas_boundary_region_t region = {0, 10, 0, 255};
    atlas_shard_t* shard = atlas_proj_extract_shard(proj, region);
    assert(shard != NULL);
    
    // Verify shard
    assert(atlas_shard_verify(shard, domain));
    
    // Cleanup
    atlas_shard_destroy(shard);
    atlas_proj_destroy(proj);
    atlas_domain_destroy(domain);
}
```

## Benchmarking

### Criterion Setup
```rust
use criterion::{criterion_group, criterion_main, Criterion};

fn bench_linear_build(c: &mut Criterion) {
    let data = vec![0u8; 12288];
    
    c.bench_function("linear_build", |b| {
        b.iter(|| build_linear(&data))
    });
}

fn bench_shard_extract(c: &mut Criterion) {
    let proj = create_test_projection();
    let region = BoundaryRegion::new(0, 10, 0, 255);
    
    c.bench_function("shard_extract", |b| {
        b.iter(|| extract_shard(&proj, region))
    });
}

criterion_group!(benches, bench_linear_build, bench_shard_extract);
criterion_main!(benches);
```

## WebAssembly Support

### Build Configuration
```bash
# Build for WASM
cargo build --target wasm32-unknown-unknown --features wasm

# With SIMD support (if needed by lower layers)
RUSTFLAGS="-C target-feature=+simd128" cargo build --target wasm32-unknown-unknown
```

### JavaScript Bindings
```javascript
// Load WASM module
const manifold = await import('./atlas_manifold.js');

// Create projection
const domain = new Uint8Array(12288);
const proj = manifold.proj_create(domain.buffer, manifold.PROJ_LINEAR);

// Extract shard
const region = {page_lo: 0, page_hi: 10, offset_lo: 0, offset_hi: 255};
const shard = manifold.proj_extract_shard(proj, region);

// Cleanup
manifold.shard_destroy(shard);
manifold.proj_destroy(proj);
```

## Performance Optimization

### Memory Layout
- Align critical data structures to cache lines (64 bytes)
- Use contiguous memory for tile data
- Minimize allocations in hot paths

### Parallelization
- Use Rayon for parallel shard extraction
- Batch witness generation where possible
- Consider SIMD for checksum calculation

### Profiling
```bash
# CPU profiling
cargo build --release
perf record --call-graph=dwarf ./target/release/bench
perf report

# Memory profiling
valgrind --tool=massif ./target/release/bench
```

## Security Considerations

1. **Input Validation**: Always validate buffer sizes and regions
2. **Integer Overflow**: Use checked arithmetic for size calculations
3. **Memory Safety**: Leverage Rust's ownership system
4. **Determinism**: Ensure all operations are reproducible
5. **Side Channels**: Avoid timing-dependent operations

## Deployment

### Static Library
```bash
cargo build --release
cp target/release/libatlas_manifold.a ../../lib/
```

### Dynamic Library
```bash
cargo build --release
cp target/release/libatlas_manifold.so ../../lib/
```

### Header Installation
```bash
cp include/atlas-manifold.h ../../include/
```

## Next Steps

1. Implement core LINEAR projection algorithm
2. Add R96_FOURIER projection support
3. Create comprehensive test suite
4. Set up CI/CD pipeline
5. Add performance benchmarks
6. Document API with examples
7. Create WASM demo application

## References

- [Layer 4 Specification](../layers/layer4-manifold.md)
- [Rust FFI Guide](https://doc.rust-lang.org/nomicon/ffi.html)
- [WebAssembly Integration](https://rustwasm.github.io/book/)