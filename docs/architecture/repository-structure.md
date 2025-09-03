# Hologram Repository Structure

## Overview

This document defines the organizational structure for the Hologram project, based on its 7-layer architecture. The structure ensures clear separation of concerns, explicit dependencies, and scalability as the project evolves.

## Current Implementation Status

- **Layer 0 (Atlas Core)**: ✅ Complete
- **Layer 1 (Boundary)**: ✅ Complete  
- **Layer 2 (Conservation)**: ✅ Complete
- **Layer 3 (Resonance)**: 🚧 In Progress
- **Layer 4 (Manifold)**: 📋 Planned
- **Layer 5 (VPI)**: 📋 Planned
- **Layer 6 (SDK)**: 📋 Planned
- **Layer 7 (Applications)**: 📋 Planned

## Proposed Directory Structure

```
/workspaces/Hologram/
│
├── layers/                           # Core layer implementations
│   │
│   ├── layer0-atlas/                # Layer 0: Atlas Core
│   │   ├── llvm/                   # LLVM IR implementations
│   │   │   ├── types.ll           # Core types and structures
│   │   │   ├── intrinsics.ll      # LLVM intrinsics
│   │   │   ├── core.ll            # Core operations
│   │   │   └── simd.ll            # SIMD optimizations
│   │   ├── include/                # C headers for Layer 0
│   │   │   └── atlas-core.h       # Core interface
│   │   ├── tests/                  # Layer 0 specific tests
│   │   │   ├── test-types.ll
│   │   │   └── test-core.ll
│   │   └── README.md               # Layer 0 documentation
│   │
│   ├── layer1-boundary/             # Layer 1: Boundary Layer
│   │   ├── llvm/                   # LLVM IR implementations
│   │   │   ├── coordinates.ll     # Coordinate system (48×256)
│   │   │   ├── klein.ll           # Klein orbit operations
│   │   │   ├── phi.ll             # Φ isomorphism
│   │   │   └── pages.ll           # Page management
│   │   ├── include/                # C headers for Layer 1
│   │   │   └── atlas-boundary.h   # Boundary interface
│   │   ├── tests/                  # Layer 1 specific tests
│   │   │   ├── test-coordinates.ll
│   │   │   ├── test-klein.ll
│   │   │   └── test-phi.ll
│   │   └── README.md               # Layer 1 documentation
│   │
│   ├── layer2-conservation/        # Layer 2: Conservation Layer
│   │   ├── llvm/                   # LLVM IR implementations
│   │   │   ├── domains.ll         # Domain management
│   │   │   ├── witness.ll         # SHA-256 witness generation
│   │   │   ├── memory.ll          # Conserved memory ops
│   │   │   ├── delta.ll           # Delta computation
│   │   │   └── budget.ll          # Budget management (RL-96)
│   │   ├── runtime/                # C runtime implementation
│   │   │   ├── domains.c          # Domain lifecycle
│   │   │   ├── witness.c          # Witness operations
│   │   │   └── conservation.c     # Conservation checks
│   │   ├── include/                # C headers for Layer 2
│   │   │   └── atlas-conservation.h
│   │   ├── tests/                  # Layer 2 specific tests
│   │   │   ├── test-domains.ll
│   │   │   ├── test-witness.ll
│   │   │   ├── test-conservation.ll
│   │   │   └── integration/       # Layer 2 integration tests
│   │   └── README.md               # Layer 2 documentation
│   │
│   ├── layer3-resonance/           # Layer 3: Resonance Layer
│   │   ├── llvm/                   # LLVM IR implementations
│   │   │   ├── r96.ll             # R96 classification
│   │   │   ├── clustering.ll      # CSR clustering
│   │   │   ├── harmonic.ll        # Harmonic analysis
│   │   │   ├── c768.ll            # C768 triple-cycle
│   │   │   └── scheduling.ll      # Resonance scheduling
│   │   ├── runtime/                # C runtime implementation
│   │   │   ├── classification.c   # R96 classification
│   │   │   ├── clustering.c       # Clustering algorithms
│   │   │   └── scheduling.c       # Harmonic scheduling
│   │   ├── include/                # C headers for Layer 3
│   │   │   └── atlas-resonance.h
│   │   ├── tests/                  # Layer 3 specific tests
│   │   │   ├── test-r96.ll
│   │   │   ├── test-clustering.ll
│   │   │   ├── test-harmonic.ll
│   │   │   └── test-c768.ll
│   │   └── README.md               # Layer 3 documentation
│   │
│   ├── layer4-manifold/            # Layer 4: Manifold Layer (Future)
│   │   ├── src/                    # Source implementations
│   │   ├── include/                # C headers for Layer 4
│   │   ├── tests/                  # Layer 4 specific tests
│   │   └── README.md               # Layer 4 documentation
│   │
│   ├── layer5-vpi/                 # Layer 5: VPI Layer (Future)
│   │   ├── src/                    # Platform interface implementations
│   │   ├── include/                # C headers for Layer 5
│   │   ├── tests/                  # Layer 5 specific tests
│   │   └── README.md               # Layer 5 documentation
│   │
│   ├── layer6-sdk/                 # Layer 6: SDK Layer (Future)
│   │   ├── typescript/             # TypeScript SDK
│   │   ├── python/                 # Python SDK
│   │   ├── go/                     # Go SDK
│   │   ├── docs/                   # SDK documentation
│   │   └── README.md               # SDK overview
│   │
│   └── layer7-apps/                # Layer 7: Applications (Future)
│       ├── examples/               # Example applications
│       ├── templates/              # App templates
│       └── README.md               # Application guide
│
├── integration/                     # Cross-layer integration
│   ├── tests/                      # Integration tests across layers
│   │   ├── l0-l1-integration.c
│   │   ├── l1-l2-integration.c
│   │   ├── l2-l3-integration.c
│   │   └── full-stack-test.c
│   ├── benchmarks/                 # Performance benchmarks
│   │   ├── layer-benchmarks.c
│   │   └── full-stack-bench.c
│   └── examples/                   # Cross-layer examples
│       └── atlas-demo.c
│
├── include/                        # Public API headers
│   ├── atlas.h                    # Unified public API
│   ├── atlas-layers.h             # Layer interface definitions
│   └── atlas-types.h              # Common type definitions
│
├── docs/                           # Project documentation
│   ├── architecture/               # Architecture documents
│   │   ├── layer-specification.md
│   │   └── design-principles.md
│   ├── layers/                     # Per-layer documentation
│   │   ├── layer0-atlas.md
│   │   ├── layer1-boundary.md
│   │   ├── layer2-conservation.md
│   │   └── layer3-resonance.md
│   ├── benchmarks/                 # Benchmark results
│   │   └── performance.md
│   └── guides/                     # Developer guides
│       ├── getting-started.md
│       └── contributing.md
│
├── build/                          # Build system
│   ├── Makefile                   # Main Makefile
│   ├── layer.mk                   # Common layer build rules
│   └── config/                    # Build configuration
│       ├── x86_64.mk
│       ├── arm64.mk
│       └── wasm32.mk
│
├── tools/                          # Development tools
│   ├── code-gen/                  # Code generation tools
│   ├── testing/                   # Test utilities
│   └── analysis/                  # Analysis tools
│
├── lib/                           # Compiled libraries (generated)
│   ├── libatlas-core.a           # Layer 0 library
│   ├── libatlas-boundary.a       # Layer 1 library
│   ├── libatlas-conservation.a   # Layer 2 library
│   ├── libatlas-resonance.a      # Layer 3 library
│   └── libatlas.a                # Complete static library
│
├── context/                       # Project context documents
│   ├── hologram_layer_architecture_specification.md
│   ├── the_hologram_the_12_288_atlas_compact_comprehensive_introduction.md
│   └── l_2_and_l_3_completion_plan.md
│
├── LICENSE                        # MIT License
├── README.md                      # Project overview
├── REPOSITORY_STRUCTURE.md        # This document
└── .gitignore                     # Git ignore rules

```

## Layer Interface Principles

### 1. Strict Layering
- Each layer communicates ONLY with adjacent layers
- No layer bypassing except for performance-critical paths (with validation)
- Downward dependencies only (higher layers depend on lower)

### 2. Interface Contracts
Each layer exports a versioned interface structure:
```c
typedef struct {
    uint16_t major;  // Breaking changes
    uint16_t minor;  // New features
    uint32_t patch;  // Bug fixes
    void* ops;       // Function pointer table
} layer_interface_t;
```

### 3. Build Independence
- Each layer can be built independently
- Lower layers have no knowledge of higher layers
- Each layer produces a static library

## File Organization Guidelines

### LLVM IR Files (.ll)
- One primary concern per file
- Clear naming: `<component>-<function>.ll`
- Include comprehensive module documentation
- Use `atlas.*` namespace for all functions

### C Runtime Files (.c/.h)
- Implement C ABI for LLVM functions
- Provide high-level abstractions
- Include comprehensive error handling
- Thread-safe where applicable

### Test Files
- Unit tests: `test-<component>.ll` or `.c`
- Integration tests: `<layer1>-<layer2>-integration.c`
- Property tests: `prop-<property>.c`
- Benchmarks: `bench-<operation>.c`

## Migration Plan

### Phase 1: Directory Creation (Immediate)
1. Create `layers/` directory structure
2. Create layer-specific subdirectories
3. Set up README.md for each layer

### Phase 2: Code Migration (Before Layer 3 Completion)
1. Move existing LLVM files to appropriate layers
2. Split mixed-concern files
3. Update include paths
4. Update Makefiles

### Phase 3: Layer 3 Implementation (Current)
1. Implement remaining Layer 3 components in new structure
2. Ensure clean L2-L3 interface
3. Add comprehensive tests

### Phase 4: Documentation (Ongoing)
1. Document each layer's purpose and interface
2. Create integration guides
3. Update architecture documentation

## Build System Structure

### Hierarchical Makefiles
```makefile
# Root Makefile
all: layer0 layer1 layer2 layer3

layer0:
    $(MAKE) -C layers/layer0-atlas

layer1: layer0
    $(MAKE) -C layers/layer1-boundary

layer2: layer1
    $(MAKE) -C layers/layer2-conservation

layer3: layer2
    $(MAKE) -C layers/layer3-resonance
```

### Per-Layer Makefiles
Each layer has its own Makefile that:
- Builds LLVM IR to bitcode
- Compiles C runtime
- Links layer library
- Runs layer-specific tests

## Testing Strategy

### Unit Tests
- Each layer has comprehensive unit tests
- Tests run in isolation
- Focus on layer-specific functionality

### Integration Tests
- Test interactions between adjacent layers
- Verify interface contracts
- Ensure conservation laws maintained

### Property-Based Tests
- Verify mathematical properties
- Test invariants across operations
- Fuzz testing for edge cases

### Performance Benchmarks
- Measure operation throughput
- Track performance regressions
- Compare implementations

## Documentation Standards

### Layer Documentation
Each layer must document:
1. Purpose and responsibilities
2. Interface specification
3. Invariants maintained
4. Dependencies
5. Implementation notes

### Code Documentation
- LLVM IR: Module-level and function-level comments
- C code: Doxygen-style comments
- Tests: Purpose and methodology

### Architecture Documentation
- Design decisions and rationale
- Mathematical foundations
- Performance characteristics

## Advantages of This Structure

1. **Clear Separation of Concerns** - Each layer has a single, well-defined purpose
2. **Explicit Dependencies** - Build system enforces correct layering
3. **Parallel Development** - Teams can work on different layers independently
4. **Easy Testing** - Layer isolation simplifies testing
5. **Scalability** - New layers can be added without disrupting existing code
6. **Documentation** - Structure makes documentation natural and discoverable
7. **Build Control** - Fine-grained control over what gets built
8. **Performance Analysis** - Easy to benchmark individual layers

## Next Steps

1. **Approve Structure** - Review and approve this proposed structure
2. **Create Directories** - Set up the directory hierarchy
3. **Migrate Code** - Move existing code to new locations
4. **Update Build** - Modify Makefiles for new structure
5. **Complete Layer 3** - Implement remaining Layer 3 components
6. **Document** - Update all documentation for new structure

## Questions for Consideration

1. Should runtime and LLVM code be in the same layer directory or separate?
2. How should we handle cross-layer utilities and shared code?
3. Should each layer have its own include directory or share a common one?
4. What naming conventions should we use for inter-layer interfaces?
5. How do we version layer interfaces independently?

---

*This document is a living specification and will be updated as the project evolves.*