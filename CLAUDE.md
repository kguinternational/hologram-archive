# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Core Architecture

Hologram is a 7-layer computing platform built on the Atlas-12,288 structure (48 pages × 256 bytes). The architecture implements mathematical invariants:
- **R96**: 96 resonance classes partition byte space
- **Conservation**: All operations maintain `sum(bytes) % 96 == 0`
- **Φ isomorphism**: Boundary↔bulk mapping enables perfect reconstruction

## Theoretical Foundation

The implementation is informed by three key theoretical frameworks:

### Universal Numbers (UN)
All Layer 4+ operations are **Universal Numbers** - scalar invariants that:
- Are invariant under symmetry transformations
- Support witnessable computation (verifiable with certificates)
- Compose algebraically (pointwise addition/multiplication)
- Preserve conservation laws

### Conservation Computation Theory (CCT)
The platform implements a conservation computer where:
- State space is fixed at 12,288 elements
- All transformations preserve `C(s) ≡ 0 (mod 96)`
- Witness chains provide verifiable computation trails
- Complexity classes: WC ⊆ HC ⊆ RC ⊆ CC

### Resonance Field Theory (RFT)
The R96/C768 structure emerges from:
- 8 base oscillators with unity pair normalization
- Triple-cycle conservation (768 = 16×48 = 3×256)
- Harmonic pairing: classes r₁, r₂ harmonize if `(r₁ + r₂) % 96 == 0`

### Layer Stack

| Layer | Directory | Status | Key Purpose |
|-------|-----------|--------|-------------|
| 0 | `layers/layer0-atlas` | ✅ Complete | 12,288-element core, LLVM primitives |
| 1 | `layers/layer1-boundary` | ✅ Complete | Coordinate system (48×256), spatial organization |
| 2 | `layers/layer2-conservation` | ✅ Complete | Witness generation, conservation laws |
| 3 | `layers/layer3-resonance` | ✅ Complete | R96 classification, harmonic scheduling |
| 4 | `layers/layer4-manifold` | ✅ Complete | Holographic projections (C + Rust hybrid) |
| 5 | `layers/layer5-vpi` | 📋 Planned | Virtual platform interface |
| 6 | `layers/layer6-sdk` | 📋 Planned | Developer APIs |
| 7 | `layers/layer7-apps` | 📋 Planned | User applications |

### Key Implementation Details

- **Layers 0-3**: Pure C with LLVM optimization
- **Layer 4**: C/Rust hybrid (`layers/layer4-manifold/rs/` for Rust components)
- **Build System**: Hierarchical Makefiles with layer dependencies
- **Testing**: Per-layer tests + integration tests in `integration/`

## Essential Development Commands

### Build Commands
```bash
# Build everything (all implemented layers + integration)
make all

# Build specific layer (automatically builds dependencies)
make layer0  # Atlas core
make layer1  # Boundary
make layer2  # Conservation
make layer3  # Resonance 
make layer4  # Manifold (includes Rust components)

# Clean build
make clean
make distclean  # Deep clean including dependencies
```

### Testing Commands
```bash
# Run all tests (unit + integration)
make test

# Test specific layer
make test-layer0
make test-layer1
make test-layer2
make test-layer3
make test-layer4  # Runs both C and Rust tests

# Integration tests only
make test-integration

# Benchmarks (Layer 2 conservation benchmarks)
make bench
make bench-l2
make bench-debug    # Debug build benchmarks
make bench-release  # Release build benchmarks
```

### Quality Checks (Important for Layer 4 Rust code)
```bash
# Linting
make lint          # Run linters (mainly Rust clippy for Layer 4)
make lint-all      # Comprehensive linting (CI-level)

# Formatting
make format        # Auto-format code (Rust for Layer 4)
make format-check-all  # Check formatting without modifying

# Type checking
make typecheck     # Type check all layers
make typecheck-all # Comprehensive type checking (CI-level)
```

### Layer 4 Rust-specific Commands
```bash
# Navigate to Rust directory
cd layers/layer4-manifold/rs

# Build Rust components
cargo build --release

# Run Rust tests
cargo test

# Run Rust linter
cargo clippy --all-targets --all-features -- -D warnings

# Format Rust code
cargo fmt

# Type check
cargo check --all-targets --all-features
```

## Critical Invariants to Maintain

When modifying code, ensure these invariants are preserved:

1. **Conservation Law**: Every operation in Layer 2+ must maintain `sum(bytes) % 96 == 0`
2. **Coordinate Bounds**: Pages ∈ [0,47], Offsets ∈ [0,255]
3. **R96 Classification**: Bytes map to classes [0,95] via `byte % 96`
4. **Witness Immutability**: Generated witnesses cannot be modified
5. **Layer Dependencies**: Higher layers depend on lower; never reverse
6. **Domain Isolation**: Operations in different domains must not interfere
7. **Universal Number Properties**: All Layer 4+ operations must be:
   - Invariant under symmetry transformations
   - Witnessable (verifiable with certificates)
   - Compositional (results compose algebraically)
8. **Harmonic Adjacency**: Elements are "adjacent" if their resonance classes harmonize

## Cross-Layer Communication

Each layer exposes typed interfaces to the layer above:
- `atlas_core_interface_t` (L0→L1)
- `boundary_interface_t` (L1→L2)
- `conservation_interface_t` (L2→L3)
- `resonance_interface_t` (L3→L4)
- `manifold_interface_t` (L4→L5)

Interface headers are in `include/atlas/` with naming pattern `layer{N}_{name}.h`

## FFI Safety (Layer 4)

Layer 4 uses Rust with C FFI bindings. Key considerations:
- All FFI functions use `#[no_mangle]` and `extern "C"`
- Pod/Zeroable traits ensure memory safety across FFI boundary
- Panic handling: `panic = "abort"` in release builds
- Static library output: `libatlas-manifold-rs.a`

## Performance Optimization Points

- **SIMD**: Layers 0-2 have SIMD implementations (`*-simd.ll` files)
- **LLVM Optimization**: Use `-O3` for release builds via `BUILD_TYPE=RELEASE`
- **Rust LTO**: Layer 4 uses link-time optimization in release mode
- **Parallelism**: Layer 4 supports parallel processing via `rayon` feature

## Common Development Patterns

1. **Adding new operations**: Implement in lowest appropriate layer, expose via interface
2. **Cross-layer testing**: Add tests to `integration/` directory
3. **Conservation violations**: Check witness generation in Layer 2
4. **Resonance mismatches**: Verify R96 classification in Layer 3
5. **Memory issues**: Use valgrind on test executables in `build/test/`

## Layer 4 Implementation Strategy

### Key Simplifications Using Universal Numbers

Layer 4 operations should leverage the theoretical frameworks:

1. **Matrix Operations as UN**: 
   - Use trace invariants (Tr(A^k)) instead of complex decompositions
   - Spectral moments are Universal Numbers that compose naturally

2. **Adjacency via Harmonics**:
   - Two elements are adjacent if `(r₁ + r₂) % 96 == 0`
   - Replaces complex geometric distance calculations

3. **Timestamps from Witnesses**:
   - Use Layer 2's witness IDs as monotonic UN timestamps
   - Already conservation-preserving and verifiable

4. **Direct Layer Integration**:
   - Use Layer 2's actual domains instead of dummy pointers
   - Use Layer 3's R96 classification directly
   - Use Layer 3's C768 state management

5. **Simplified Computations**:
   - Metric tensor: Use trace invariants instead of Jacobian
   - Curvature: Use spectral moments instead of Riemann tensor
   - Move operations: Simple conservation-preserving copies

### Implementation Principles

- **Don't rebuild what exists**: Layers 0-3 provide complete conservation infrastructure
- **Recognize UN patterns**: Most complex operations are just Universal Numbers
- **Compose, don't compute**: UN properties mean operations compose algebraically
- **Witness everything**: Every operation should generate verifiable certificates
- **Harmonize, don't distance**: Use R96 harmonic relationships instead of Euclidean metrics