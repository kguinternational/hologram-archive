# Atlas-12288 Specification

## Overview

Atlas-12288 is a conservation-based computational model that operates on a fixed-size structure of 12,288 bytes organized as 48 pages of 256 bytes each. The system implements three fundamental invariants: R96 (resonance classification), C768 (triple-cycle conservation), and Φ (boundary↔bulk isomorphism).

## Mathematical Foundation

### Minimality Proof
The 12,288 element structure is mathematically minimal, satisfying simultaneous constraints:
1. **Modular closure**: LCM-compatible rhythms for page (48) and byte (256) windows
2. **Resonance completeness**: Exactly 96 classes with unity pair and single pin: `(3·2^(8-2))/2 = 96`
3. **Binary–ternary coupling**: Triple cycle C768 = 16×48 = 3×256
4. **Group action**: V₄ orbits tile the basis without gaps
5. **Positive geometry**: Φ preserves orientation/grade structure

## Core Concepts

### 1. Structure Organization
- **Total Size**: 12,288 bytes (unique minimal satisfying object)
- **Organization**: 48 pages × 256 bytes/page
- **Addressing**: Page index (0-47) + byte offset (0-255)
- **Alignment**: Pages at 256-byte boundaries, structures at 4096-byte boundaries

### 2. R96 Resonance Classification
- **Mathematical Model**: 8 toggles with multiplicative structure `R(b) = ∏ α_i^{b_i}`
- **Unity Pair**: `α_u · α_v = 1` (removes 1 degree of freedom)
- **Pinned Oscillator**: `α_0 = 1` (removes 1 degree of freedom)
- **Image Size**: `(3·2^{8-2})/2 = 192/2 = 96` classes
- **Group Law**: Classes add modulo 96
- **Harmonizing Pairs**: `r1 ⊕ r2 ≡ 0 (mod 96)`
- **Klein Orbits**: Canonical orbits {0,1,48,49} for fast acceptance

### 3. C768 Triple-Cycle Conservation
- **Cycle Length**: 768 steps (LCM of 16×48 and 3×256 rhythms)
- **Page Rhythm**: 16-period cycle over 48-page boundaries
- **Byte Rhythm**: 3-period cycle over 256-byte windows
- **Conservation Closure**: Window sums stabilize at 768 steps
- **Phase Lock**: Page and byte rhythms synchronize at 768-step intervals
- **Verification**: `Σ window ≡ 0 (mod 96)` for all admissible windows

### 4. Conservation Laws
- **Primary Law**: `sum(data) % 96 == 0` must be preserved across all operations
- **Window Conservation**: Each 256-byte window maintains local conservation
- **Page Conservation**: Each page maintains conservation independently
- **Global Conservation**: Entire 12,288 structure maintains conservation
- **Operation Preservation**: All writes must preserve conservation or fail closed

### 5. Witness Generation & Provenance
- **Immutable Records**: Witnesses are cryptographically bound to data
- **Chain of Custody**: Witnesses can form verifiable chains
- **Metadata**: Includes checksum, timestamp, resonance summary
- **Verification**: Linear-time local verification without global ordering
- **Merge Operations**: Witnesses can be composed and merged

### 6. Boundary Encoding (Φ Isomorphism)
- **Encoding**: `Φ(page, offset) = page*256 + offset` with gauge bits
- **Decoding**: Inverse mapping with Klein alignment preservation
- **Bijection**: Perfect boundary↔bulk correspondence
- **NF-Lift**: Canonical lifts preserve resonance histograms and budget
- **Round-trip**: `decode(encode(coord)) = coord` (identity property)

### 7. Klein Orbits & V₄ Group Structure
- **Privileged Orbits**: {0, 1, 48, 49} enable canonicalization
- **V₄ Group**: Klein four-group `{e, a, b, ab}` where `a² = b² = (ab)² = e`
- **Coset Partition**: 4 cosets × 3,072 elements = 12,288 total
- **Orbit Classification**: 16 orbits × 768 elements = 12,288 total
- **Canonicalization**: Maps coordinates to minimal orbit representatives
- **Fast Acceptance**: Privileged orbits enable O(1) acceptance tests

### 8. Domain Management
- **Conservation Domains**: Isolated conservation contexts with budget tracking
- **Domain Structure**: `{id, structure, budget, conservation_sum, witness, isolation_proof, c768_phase}`
- **Budget Management**: RL-96 algebra with transfer, fork, and merge operations
- **Isolation Verification**: Mathematical proofs using GCD-based separation
- **Lifecycle**: Create, validate, fork, merge, destroy with proper cleanup

### 9. Harmonic Operations
- **Golden Ratio Scaling**: φ = 1.618... for optimal harmonic relationships
- **Harmonic Windows**: Resonance-based optimal processing windows
- **Harmonic Pairs**: Precomputed 48 pairs where `a + b ≡ 0 (mod 96)`
- **Affinity Clustering**: Exponential decay clustering with mathematical affinity
- **Scheduling**: Resonance-aware execution scheduling with period optimization

### 10. Structure-Preserving Morphisms
- **Boundary Automorphisms**: `(p,b) ↦ (u₄₈·p, u₂₅₆·b)` with coprime units
- **Selector Gauge**: Toggle permutations respecting unity/pin constraints
- **NF-Lift Operations**: Canonical boundary↔bulk lifts with round-trip verification
- **Resonance Arrows**: Evaluators and Klein window characters
- **RL-96 Arrows**: Budget transformations with conservative Boolean collapse

## Complete Operations Reference

### R96 Classification Operations
```llvm
declare i7 @atlas.r96.classify(i8 %byte)                    ; Single byte classification
declare void @atlas.r96.histogram(ptr %data, ptr %hist)     ; Generate resonance histogram
declare i7 @atlas.r96.dominant(ptr %data)                   ; Find dominant class
declare <16 x i7> @atlas.r96.classify.v16i8(<16 x i8>)     ; SIMD classification
declare <256 x i7> @atlas.r96.classify.page(<256 x i8>)     ; Full page classification
```

### C768 Triple-Cycle Operations
```llvm
declare i1 @atlas.c768.verify_closure(ptr %structure, i64 %window_start)
declare i64 @atlas.c768.compute_window_sum(ptr %data, i64 %offset, i64 %size)
declare i1 @atlas.c768.check_page_rhythm(ptr %structure, i64 %step)
declare i1 @atlas.c768.check_byte_rhythm(ptr %structure, i64 %step)
declare i64 @atlas.c768.next_cycle_point(i64 %current)
declare i1 @atlas.c768.verify_phase_lock(ptr %structure)
```

### Conservation Operations
```llvm
declare i1 @atlas.conserved.check(ptr %data, i64 %len)      ; Conservation verification
declare i16 @atlas.conserved.sum.page(ptr %page)            ; Page-level sum
declare i32 @atlas.conserved.sum.structure(ptr %structure)  ; Structure-level sum  
declare i32 @atlas.conserved.delta(ptr %before, ptr %after, i64 %len)
declare void @atlas.conserved.add(ptr %dst, ptr %src1, ptr %src2, i64 %len)
declare i1 @atlas.conserved.domain(ptr %domain)
```

### Witness Operations
```llvm
declare ptr @atlas.witness.generate(ptr %data, i64 %len)
declare i1 @atlas.witness.verify(ptr %w, ptr %data, i64 %len)
declare void @atlas.witness.destroy(ptr %w)
declare ptr @atlas.witness.chain(ptr %current, ptr %previous)
declare ptr @atlas.witness.merge(ptr %w1, ptr %w2)
declare i64 @atlas.witness.timestamp(ptr %w)
declare i7 @atlas.witness.resonance(ptr %w)
```

### Boundary Operations (Φ Isomorphism)
```llvm
declare i32 @atlas.boundary.encode(i16 %page, i8 %offset)
declare { i16, i8 } @atlas.boundary.decode(i32 %boundary)
declare i32 @atlas.boundary.transform(i32 %boundary, i7 %resonance)
declare i2 @atlas.boundary.klein(i32 %boundary)
declare i32 @atlas.boundary.rotate(i32 %boundary, i2 %klein)
declare <16 x i32> @atlas.boundary.encode.v16(<16 x i16>, <16 x i8>)
```

### Domain Management Operations
```llvm
declare ptr @atlas.domain.create(i7 %initial_budget)
declare void @atlas.domain.destroy(ptr %domain)
declare i1 @atlas.domain.validate(ptr %domain)
declare i1 @atlas.domain.transfer_budget(ptr %from, ptr %to, i7 %amount)
declare ptr @atlas.domain.fork(ptr %parent, i7 %child_budget)
declare ptr @atlas.domain.merge(ptr %d1, ptr %d2)
declare i1 @atlas.domain.verify_isolated(ptr %d1, ptr %d2)
```

### Klein Orbit Operations
```llvm
declare i8 @atlas.klein.get_orbit_id(i32 %coord)
declare i1 @atlas.klein.is_privileged_orbit(i32 %coord)
declare i32 @atlas.klein.canonicalize_coord(i32 %coord)
declare i1 @atlas.klein.verify_coset_partition(ptr %structure)
declare i1 @atlas.klein.quick_accept(ptr %data, i64 %len)
declare i64 @atlas.klein.compute_orbit_signature(ptr %structure)
```

### Harmonic Operations
```llvm
declare %atlas.harmonic.window @atlas.harmonic.compute_window(i7 %resonance, i16 %c768_base)
declare ptr @atlas.harmonic.compute_windows(ptr %resonance_array, i32 %count)
declare ptr @atlas.harmonic.create_schedule(ptr %windows, i32 %window_count)
declare %atlas.harmonic.pair @atlas.harmonic.find_pair(i7 %resonance)
declare i1 @atlas.harmonic.validate_pair(%atlas.harmonic.pair %pair)
declare double @atlas.harmonic.calculate_affinity(i7 %res_a, i7 %res_b)
```

### Morphism Operations
```llvm
declare i32 @atlas.morphism.boundary_auto(i32 %coord, i8 %u48, i8 %u256)
declare ptr @atlas.morphism.nf_lift(ptr %boundary_trace, i64 %trace_len)
declare ptr @atlas.morphism.nf_project(ptr %bulk_section)
declare i1 @atlas.morphism.verify_roundtrip(ptr %original, ptr %result)
declare i7 @atlas.morphism.rl_compose(i7 %a, i7 %b)
declare i1 @atlas.morphism.rl_conservative_collapse(i7 %budget)
```

### Validation Operations
```llvm
declare double @atlas.validation.calculate_entropy(ptr %page)
declare %atlas.validation.page_info @atlas.validation.validate_page(ptr %page)
declare %atlas.validation.structure_info @atlas.validation.validate_structure(ptr %structure)
declare %atlas.validation.result @atlas.validation.validate_complete(ptr %structure, ptr %witness)
declare i32 @atlas.validation.count_anomalies(ptr %page)
declare i1 @atlas.validation.repair_page(ptr %page)
```

### Budget Operations (RL-96 Algebra)
```llvm
declare i7 @atlas.budget.add(i7 %a, i7 %b)             ; Modulo 96 addition
declare i7 @atlas.budget.mul(i7 %a, i7 %b)             ; Modulo 96 multiplication
declare i7 @atlas.budget.inv(i7 %budget)               ; Modular inverse
declare i1 @atlas.budget.zero(i7 %budget)              ; Zero test
declare i1 @atlas.budget.less(i7 %a, i7 %b)            ; Ordering relation
declare i7 @atlas.budget.alloc(i32 %amount)            ; Budget allocation
declare void @atlas.budget.release(i7 %budget)         ; Budget release
```

## Memory Model

### Alignment Requirements
- **Pages**: 256-byte aligned for optimal cache performance
- **Structures**: 4096-byte aligned for page-level operations
- **Witness blocks**: 64-byte aligned for SIMD efficiency  
- **Domain structures**: 32-byte aligned for atomic operations

### Allocation Types
- **Witnessed allocation**: `atlas.alloc.witnessed(size, resonance)` with provenance
- **Page-aligned bulk**: `atlas.alloc.pages(count)` for structure allocation
- **Resonance-aware**: `atlas.alloc.resonant(size, resonance, alignment)` with hints
- **Conservation pools**: Memory pools maintaining conservation invariants

### Memory Operations
```llvm
declare ptr @atlas.alloc.witnessed(i64 %size, i7 %resonance)
declare void @atlas.free.witnessed(ptr %ptr)
declare ptr @atlas.alloc.pages(i32 %count)
declare ptr @atlas.alloc.resonant(i64 %size, i7 %resonance, i32 %alignment)
declare void @atlas.memcpy.conserved(ptr %dst, ptr %src, i64 %len)
declare void @atlas.memset.conserved(ptr %dst, i8 %val, i64 %len)
```

## SIMD Support & Vectorization

### Vector Widths by Architecture
- **SSE2**: 16 bytes (baseline x86-64 compatibility)
- **AVX2**: 32 bytes (modern x86-64 standard)
- **AVX-512**: 64 bytes (server-class x86-64)
- **NEON**: 16 bytes (ARM64 standard)
- **WASM SIMD**: 16 bytes (WebAssembly with SIMD128)

### Vectorized Operations
- **Bulk R96 classification**: Process 16-64 bytes per instruction
- **Parallel conservation checking**: Vector accumulation with modular arithmetic
- **SIMD witness generation**: Parallel checksum and metadata computation
- **Vector boundary operations**: Batch coordinate encoding/decoding
- **Parallel validation**: Entropy calculation and anomaly detection

### Performance Characteristics
- **Classification**: ~0.5 cycles/byte (AVX2), ~0.25 cycles/byte (AVX-512)
- **Conservation checks**: ~0.3 cycles/byte with SIMD accumulation
- **Witness generation**: ~50 cycles/page (256 bytes)
- **Klein operations**: ~10 cycles/coordinate with lookup tables

## Acceptance Tests & Conformance

### Required Conformance Tests
1. **R96 Count**: Verify exactly 96 distinct classes over 256 bytes
2. **C768 Closure**: Window sums stabilize at 768-step cycle
3. **Klein Canonicalization**: Privileged orbits {0,1,48,49} and V₄ cosets
4. **Φ Round-trip**: encode→decode identity with NF-Lift reconstruction
5. **RL Budget Books**: End-to-end proof conservation under composition

### Validation Hierarchy
- **Page-level**: Entropy, alignment, anomaly detection
- **Structure-level**: Conservation, page integrity, repair capabilities  
- **Cross-layer**: Boundaries, resonance distribution, witness integrity
- **Mathematical**: Klein orbit tiling, harmonic pair validation, morphism preservation

## Implementation Requirements

### LLVM Integration
- **Target**: LLVM 15+ with opaque pointers (`ptr` type)
- **Intrinsics**: Use `atlas.*` namespace (not `llvm.atlas.*`)
- **Attributes**: Specialized attribute groups for pure, readonly, conserving operations
- **Optimization**: Support LTO, PGO, and custom Atlas optimization passes
- **ABI Stability**: Maintain C-compatible ABI across versions

### Platform Support
- **Primary**: x86-64 (SSE2/AVX2/AVX-512)
- **Secondary**: ARM64 (NEON), WebAssembly (SIMD128)
- **Build System**: Make-based with CMake compatibility
- **Testing**: Comprehensive test suite with conformance validation
- **Debugging**: DWARF debug info with Atlas-specific metadata

### Integration Patterns
- **C Library**: Stable C API for system integration
- **Language Bindings**: FFI-compatible for Rust, Go, Python, Node.js
- **Memory Safety**: Bounds checking and conservation verification
- **Thread Safety**: Read operations thread-safe, writes require synchronization
- **Error Handling**: Structured error codes with descriptive messages