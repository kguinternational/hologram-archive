# Getting Started with Hologram

## Prerequisites
- LLVM 15+ with development headers
- Clang compiler
- Make build system
- Python 3.8+ (for testing)
- Git

## Quick Start

### 1. Clone the Repository
```bash
git clone https://github.com/UOR-Foundation/Hologram.git
cd Hologram
```

### 2. Build All Layers
```bash
make all
```

This builds layers 0-3 in sequence, respecting dependencies.

### 3. Run Tests
```bash
make test
```

### 4. Build Individual Layers
```bash
make -C layers/layer0-atlas
make -C layers/layer1-boundary
make -C layers/layer2-conservation
make -C layers/layer3-resonance
```

## Architecture Overview
Hologram is structured as a 7-layer stack:
- **Layer 0**: Atlas Core - Fundamental 12,288-element structure
- **Layer 1**: Boundary - Coordinate system and spatial organization
- **Layer 2**: Conservation - Witness generation and conservation laws
- **Layer 3**: Resonance - R96 classification and harmonic scheduling
- **Layer 4**: Manifold (planned) - Holographic projections
- **Layer 5**: VPI (planned) - Virtual platform interface
- **Layer 6**: SDK (planned) - Developer-friendly APIs
- **Layer 7**: Applications (planned) - User applications

## Key Concepts
- **Atlas-12,288**: 48 pages × 256 bytes = 12,288 elements
- **R96 Classification**: Bytes map to 96 resonance classes
- **Conservation Law**: sum(bytes) % 96 == 0 must be preserved
- **Witnesses**: Immutable proofs of conservation

## Next Steps
- Read the [Layer Architecture Specification](../architecture/layer-specification.md)
- Explore the [Design Principles](../architecture/design-principles.md)
- Check individual layer documentation in `docs/layers/`
- Review example code in `integration/examples/`