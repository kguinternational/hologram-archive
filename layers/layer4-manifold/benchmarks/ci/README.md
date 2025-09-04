# Layer 4 Manifold CI Performance Benchmarks

This directory contains the CI performance regression testing system for Layer 4 (Manifold) operations. The system automatically runs performance benchmarks on every PR and detects performance regressions.

## Files

### Core Scripts
- **`regression_test.sh`** - Main regression test script that compares performance against baselines
- **`benchmark_runner.sh`** - Unified benchmark runner with hardware profile support
- **`performance_baselines.json`** - Expected performance metrics for different hardware profiles
- **`hardware-profiles.json`** - Hardware configuration profiles (x86_64, ARM64, WASM)

### GitHub Actions Integration
- **`/.github/workflows/layer4-benchmarks.yml`** - GitHub Actions workflow for automated benchmarks

### Rust Integration
- **`../rs/tests/ci_benchmarks.rs`** - Rust integration tests with structured JSON output

## Usage

### Running Regression Tests Locally

```bash
# Run with default settings
./regression_test.sh

# Run with specific hardware profile and threshold  
./regression_test.sh --hardware-profile arm64 --threshold 15

# Update baseline on improvements
./regression_test.sh --update-baseline --verbose
```

### Running Custom Benchmarks

```bash
# Run all benchmarks with auto-detected profile
./benchmark_runner.sh

# Run specific operations
./benchmark_runner.sh --operations "linear_projection,r96_fourier" --output-format csv

# Run with custom data sizes
./benchmark_runner.sh --data-sizes "4096,65536,1048576" --iterations 50
```

## Hardware Profiles

### x86_64 (High Performance)
- **Target**: Desktop/server systems with modern SIMD
- **Features**: SSE4, AVX2, FMA support
- **Regression Threshold**: 10%
- **Expected Performance**: Baseline (1.0x)

### ARM64 (Medium-High Performance) 
- **Target**: ARM64 systems with NEON/ASIMD
- **Features**: NEON, ASIMD, FP16 support
- **Regression Threshold**: 15%
- **Expected Performance**: 0.75x of x86_64

### WASM32 (Medium Performance)
- **Target**: WebAssembly runtime with SIMD
- **Features**: SIMD128 support
- **Regression Threshold**: 20%
- **Expected Performance**: 0.5x of x86_64

## Core Operations Benchmarked

### Universal Number Operations
1. **Linear Projection** - Holographic projection using trace invariants
2. **R96 Fourier** - Resonance-based Fourier projection with conservation
3. **Shard Extraction** - Boundary region extraction with harmonic adjacency
4. **Transformation** - Geometric transformation using spectral moments
5. **Reconstruction** - Manifold reconstruction from distributed shards

### Performance Metrics
- **Average Duration** - Mean time per operation in nanoseconds
- **Operations per Second** - Throughput metric
- **Memory Usage** - Peak memory consumption during operation
- **Conservation Verification** - Time to verify conservation laws
- **Witness Generation** - Time to generate verifiable certificates

## CI Integration

### Automated Testing
The GitHub Actions workflow automatically:
1. Builds dependencies (Layers 0-3)
2. Runs benchmarks on x86_64, ARM64 (emulated), and WASM
3. Compares results against baseline performance
4. Posts performance report as PR comment
5. Fails PR if performance regresses beyond threshold
6. Updates baseline on main branch improvements

### PR Comments
Performance results are automatically posted to PRs with:
- Comparison against baseline performance
- Operations per second and throughput metrics
- Hardware-specific results
- Regression warnings if thresholds exceeded

### Baseline Management
- Baselines automatically updated on main branch when performance improves
- Manual baseline updates possible with workflow dispatch
- Hardware-specific baselines maintained separately

## Performance Expectations

### Theoretical Foundation
All operations are based on **Universal Numbers** (UN) - scalar invariants that:
- Are invariant under symmetry transformations  
- Support witnessable computation with verifiable certificates
- Compose algebraically (pointwise operations)
- Preserve conservation laws (`sum(bytes) % 96 == 0`)

### Conservation Computation Theory
The platform implements conservation-preserving operations where:
- State space fixed at 12,288 elements (Atlas-12,288)
- All transformations maintain `C(s) ≡ 0 (mod 96)`
- Witness chains provide verifiable computation trails
- Complexity: WC ⊆ HC ⊆ RC ⊆ CC

### Key Optimizations
1. **Trace Invariants** - Use `Tr(A^k)` instead of matrix decomposition
2. **Harmonic Adjacency** - Elements adjacent if `(r₁ + r₂) % 96 == 0`  
3. **Witness Timestamps** - Layer 2 witness IDs as monotonic timestamps
4. **Direct Integration** - Use Layer 0-3 conservation infrastructure
5. **Spectral Moments** - UN-based curvature without Riemann tensor

## Troubleshooting

### Common Issues
1. **Missing Dependencies**: Install `jq`, `bc`, and Rust toolchain
2. **Build Failures**: Ensure Layers 0-3 are built first
3. **Timeout Issues**: Increase thresholds for slower systems
4. **ARM64 Emulation**: Results approximate due to QEMU overhead
5. **WASM Limitations**: Some features disabled in WASM builds

### Debug Mode
Run with `--verbose` flag for detailed execution information:
```bash
./regression_test.sh --verbose
./benchmark_runner.sh --verbose
```

### Manual Verification
Verify conservation laws in test data:
```rust
let sum: u32 = data.iter().map(|&b| b as u32).sum();
assert_eq!(sum % 96, 0); // Must satisfy conservation
```

## Contributing

When modifying benchmarks:
1. Ensure all operations preserve conservation laws
2. Update baselines when making performance improvements
3. Test on multiple hardware profiles before merging
4. Document any changes to Universal Number implementations
5. Verify witness generation works correctly for all operations

## References

- [Layer 4 Documentation](../../README.md)
- [Universal Numbers Theory](../../docs/universal-numbers.md)
- [Conservation Computation](../../docs/conservation-computation.md)
- [Resonance Field Theory](../../docs/resonance-field-theory.md)