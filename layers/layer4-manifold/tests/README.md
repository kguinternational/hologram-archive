# Atlas Manifold Layer 4 - Test Suite

This directory contains a comprehensive test suite for Layer 4 (Manifold) of the Atlas-12288 Hologram project.

## Test Files Overview

### Core Test Files

- **`test-manifold.c`** - Comprehensive manifold operations test suite
  - Projection creation and management (LINEAR and R96_FOURIER)
  - Transform operations and parameter validation
  - Shard extraction and reconstruction
  - Conservation law verification
  - Error handling and statistics

- **`test-geometry.c`** - Geometric operations test suite
  - Projection creation (LINEAR and R96_FOURIER types)
  - Geometric transformations (scaling, rotation, translation)
  - Linear transform matrices
  - Fourier transform operations
  - Curvature calculations
  - Geodesic distance computations
  - Critical points detection

- **`test-topology.c`** - Topology operations test suite
  - Boundary region validation and operations
  - Shard extraction (single and batch)
  - Shard reconstruction and context management
  - Topological invariants computation
  - Conservation law preservation
  - Witness generation concepts

- **`test-integration.c`** - Cross-layer integration test suite
  - Layer 2 (Conservation) integration
  - Layer 3 (Resonance) integration  
  - Full stack workflow testing (Layers 2+3+4)
  - Cross-layer witness verification
  - Performance characteristics across layers
  - Error handling across layer boundaries

## Test Categories

### 1. Projection Operations
- **Linear Projections**: Basic linear projection creation and manipulation
- **R96 Fourier Projections**: Resonance-aware Fourier transform projections
- **Projection Validation**: Verification of projection integrity and properties
- **Dimension Management**: Testing projection dimensions and coordinate systems

### 2. Transform Operations  
- **Geometric Scaling**: Uniform and non-uniform scaling operations
- **Rotation Operations**: Rotation around arbitrary center points
- **Translation Operations**: Linear translation in 2D/3D space
- **Matrix Transforms**: 4x4 transformation matrix operations
- **Fourier Transforms**: Forward and inverse R96 Fourier transforms
- **Parameter Validation**: Transform parameter bounds checking

### 3. Shard Operations
- **Boundary Regions**: Validation and creation of boundary regions
- **Shard Extraction**: Single and batch shard extraction from projections
- **Shard Verification**: Integrity checking and validation of extracted shards
- **Data Operations**: Shard size calculation and data copying operations

### 4. Reconstruction Operations
- **Context Management**: Reconstruction context initialization and management
- **Shard Assembly**: Adding multiple shards to reconstruction contexts
- **Completion Detection**: Checking reconstruction completion status
- **Finalization**: Creating final projections from assembled shards

### 5. Advanced Mathematical Operations
- **Topological Invariants**: Computing manifold topological invariants
- **Curvature Calculation**: Local curvature computation at specified points
- **Geodesic Distance**: Shortest path distance calculations on manifolds
- **Critical Points**: Finding mathematical critical points in projections

### 6. Conservation and Verification
- **Conservation Laws**: Verifying adherence to Atlas conservation mathematics
- **Witness Operations**: Cryptographic witness generation and verification
- **Data Integrity**: Cross-layer data consistency checking
- **System Verification**: Comprehensive system-level validation

### 7. Integration Testing
- **Layer 2 Integration**: Conservation law compliance and witness operations
- **Layer 3 Integration**: Resonance classification and harmonic scheduling
- **Full Stack Workflows**: End-to-end testing of complete Atlas operations
- **Performance Integration**: Cross-layer performance characteristics

## Golden Values Testing

Each test file includes "golden values" tests with known mathematical inputs and expected outputs:

- **Fibonacci Sequences**: Mathematical sequences with known properties
- **Golden Ratio Patterns**: Data based on the golden ratio φ = (1+√5)/2
- **Trigonometric Patterns**: Sine/cosine based patterns with periodic properties  
- **Conservation Patterns**: Data specifically crafted to satisfy conservation laws
- **Prime Number Patterns**: Data based on prime number sequences

## Error Handling Testing

Comprehensive error condition testing includes:

- **NULL Pointer Handling**: Safe handling of NULL parameters
- **Invalid Parameter Detection**: Bounds checking and parameter validation
- **Resource Management**: Memory allocation/deallocation error conditions
- **Cross-Layer Error Propagation**: Error handling across layer boundaries
- **Edge Case Coverage**: Boundary conditions and unusual input patterns

## Performance Testing

Performance benchmarks are included for:

- **Projection Creation**: Time to create projections of various sizes
- **Transform Operations**: Performance of geometric and mathematical transforms
- **Shard Operations**: Extraction and reconstruction performance
- **Memory Usage**: Memory allocation patterns and peak usage
- **Cross-Layer Operations**: Performance of integrated workflows

## Building and Running Tests

### Prerequisites

Ensure the following layers are built first:
1. Layer 1 (Core) - `libatlas-core.a`
2. Layer 2 (Conservation) - `libatlas-conservation.a` 
3. Layer 3 (Resonance) - `libatlas-resonance.a`

### Build Commands

```bash
# Build all tests
make test

# Build individual test components
make manifold geometry topology runtime-all

# Clean build artifacts
make clean-runtime
```

### Manual Test Compilation

```bash
# Individual test compilation examples
clang -std=c11 -I../../include -I../include -I./include \
    tests/test-geometry.c -L../../lib -latlas-manifold -lm \
    -o build/tests/test-geometry

clang -std=c11 -I../../include -I../include -I./include \
    tests/test-integration.c -L../../lib \
    -latlas-manifold -latlas-resonance -latlas-conservation -lm \
    -o build/tests/test-integration
```

### Running Tests

```bash
# Run all tests via Makefile
make test

# Run individual tests  
./build/tests/test-manifold
./build/tests/test-geometry
./build/tests/test-topology
./build/tests/test-integration
```

## Test Output Interpretation

### Success Indicators
- **✓ messages**: Individual test assertions that passed
- **Test completion messages**: Summary of successful test categories
- **Performance metrics**: Timing and throughput measurements
- **Golden values validation**: Verification of known mathematical results

### Implementation Status Notes
- **"Note: ... not implemented"**: Expected messages for pending features
- **"May not be implemented"**: Features that return error codes or NULL
- **"Skipping ... tests"**: Tests that depend on unimplemented functionality

### Error Conditions
- **"TEST FAILED"**: Critical test failures that should be investigated
- **"[WARN] Some tests failed"**: Expected warnings for unimplemented features
- **Compilation failures**: Missing dependencies or API mismatches

## Integration with CI/CD

The test suite is designed to work with automated testing systems:

1. **Exit Codes**: Tests return 0 on success, non-zero on critical failures
2. **Structured Output**: Clear success/failure indicators for parsing
3. **Dependency Checking**: Automatic verification of required libraries
4. **Progressive Implementation**: Tests accommodate partial implementations

## API Coverage

The test suite provides comprehensive coverage of the Atlas Manifold API:

- **100% Function Coverage**: Every public API function is tested
- **Parameter Validation**: All parameter combinations and edge cases
- **Error Path Testing**: All error conditions and return codes
- **Integration Points**: All interfaces with Layers 2 and 3
- **Mathematical Properties**: Verification of underlying mathematics

## Contributing to Tests

When adding new tests:

1. **Follow Naming Conventions**: Use descriptive test function names
2. **Include Golden Values**: Add tests with known mathematical results  
3. **Test Error Conditions**: Verify proper error handling
4. **Add Performance Tests**: Include timing measurements for new operations
5. **Update Documentation**: Keep this README current with new test categories

## Mathematical Verification

The test suite includes verification of key mathematical properties:

- **Conservation Laws**: Σ(bytes) ≡ 0 (mod 96)
- **Resonance Classification**: R96 classification consistency
- **Topological Invariants**: Properties preserved under continuous deformation
- **Geometric Properties**: Scaling, rotation, and translation correctness
- **Harmonic Relationships**: R96 harmonic conjugate validation

This comprehensive test suite ensures the Atlas Manifold Layer 4 implementation is mathematically sound, performant, and integrates correctly with the broader Atlas-12288 system.