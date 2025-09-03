# Layer 3 (Resonance) Benchmark Suite

Comprehensive performance benchmarks for Atlas-12288 Layer 3 operations including R96 classification, CSR clustering, histogram generation, and harmonic analysis.

## Overview

The benchmark suite provides detailed performance measurements for:

### Classification Benchmarks (`bench-classification.c`)
- **R96 Classification Throughput**: Tests scalar vs SIMD performance for resonance classification
- **Histogram Generation**: Measures histogram creation speed for different page counts  
- **Batch Processing**: Evaluates efficiency of multi-page classification operations
- **Memory Bandwidth**: Analyzes data throughput at different sizes (256B, 12KB, 1MB)

### Clustering Benchmarks (`bench-clustering.c`)
- **CSR Matrix Construction**: Measures Compressed Sparse Row matrix build performance
- **Cluster Access Patterns**: Tests speed of resonance class lookups and iteration
- **Memory Efficiency**: Analyzes memory usage vs theoretical minimum
- **Harmonic Analysis**: Benchmarks harmonic pair detection and conjugate calculations
- **Distribution Analysis**: Evaluates clustering quality and homogeneity metrics

## Performance Targets

| Operation | Target | Unit | Notes |
|-----------|---------|------|-------|
| R96 Classification | ≥8.0 | GB/s | SIMD-optimized throughput |
| Histogram Generation | ≥500 | MB/s | Page histogram creation |
| Batch Processing | ≥100,000 | pages/s | Multi-page classification |
| CSR Construction | ≥50,000 | pages/s | Clustering matrix build |
| Cluster Access | <1,000 | ns/class | Individual class lookup |
| Harmonic Analysis | ≥1,000,000 | ops/s | Harmonic pair checks |
| Memory Efficiency | ≥80% | ratio | Actual vs theoretical usage |

## Build System

The benchmark suite supports multiple optimization levels:

### Build Configurations
- **Debug** (`-O0 -g`): No optimizations, debug symbols
- **Release** (`-O2 -mavx2`): Standard optimizations, SIMD enabled  
- **Performance** (`-O3 -march=native -flto`): Maximum optimizations, LTO

### Build Targets
```bash
make all          # Build release versions (default)
make debug        # Build debug versions
make release      # Build release versions
make performance  # Build performance optimized versions
```

### Run Targets
```bash
make run-all           # Run both benchmark suites
make run-classification # Run classification benchmarks only
make run-clustering    # Run clustering benchmarks only
make run-performance   # Run performance optimized builds
make run-comparison    # Compare optimization levels
```

### Utility Targets
```bash
make test-suite    # Run comprehensive test suite with results logging
make profile       # Profile benchmarks with perf (if available)
make validate      # Verify benchmarks run correctly
make memcheck      # Check for memory leaks with valgrind
make install       # Install to system location
make clean         # Remove build artifacts
```

## Usage Examples

### Quick Performance Check
```bash
cd benchmark/
make quick-test
```

### Full Benchmark Suite
```bash
cd benchmark/
make test-suite
# Results saved to results/ directory
```

### Performance Comparison
```bash
cd benchmark/
make run-comparison
# Compare debug vs release vs performance builds
```

### Memory Analysis
```bash
cd benchmark/
make memcheck
# Check for memory leaks and usage patterns
```

## Output Formats

### Human-Readable Results
- Tabular performance summaries
- Target vs actual comparisons  
- System configuration details
- Memory usage statistics

### Machine-Parseable Results
- CSV format results for automation
- Individual test pass/fail status
- Detailed timing and throughput metrics
- Memory efficiency measurements

## Benchmark Details

### Data Generation
- **Clusterable Data**: Creates patterns that exercise different clustering scenarios
- **Harmonic Patterns**: Generates data with known harmonic relationships
- **Size Scaling**: Tests from single pages (256B) to large datasets (16MB)
- **Reproducible Seeds**: Consistent test data across runs

### Statistical Analysis
- **Multiple Samples**: 10 measurement samples per test
- **Warmup Iterations**: 100-1000 warmup runs to stabilize performance
- **Median/Average**: Reports both median and average timing
- **Min/Max Ranges**: Shows performance variability

### SIMD Optimization
- **AVX2 Detection**: Automatically uses SIMD when available
- **Scalar Fallback**: Graceful degradation on older processors
- **Speedup Analysis**: Compares SIMD vs scalar performance
- **Alignment**: Proper memory alignment for SIMD operations

## Dependencies

### Required Libraries
- `libatlas-resonance.a` - Layer 3 (Resonance) runtime
- `libatlas-conservation.a` - Layer 2 (Conservation) runtime

### Build Dependencies
- `clang` - C compiler with C11 support
- `make` - Build system
- Standard C library with POSIX extensions

### Optional Dependencies
- `perf` - For performance profiling
- `valgrind` - For memory leak detection
- `gnuplot` - For result visualization (future enhancement)

## Performance Notes

### CPU Requirements
- **Minimum**: x86-64 with basic SSE support
- **Recommended**: Intel/AMD with AVX2 support
- **Optimal**: Modern CPU with AVX2, FMA, BMI instructions

### Memory Requirements
- **Classification**: ~50MB peak usage for largest tests
- **Clustering**: ~200MB peak usage for 64K page tests
- **Alignment**: 32-byte alignment for optimal SIMD performance

### System Tuning
- Disable CPU frequency scaling for consistent results
- Set CPU governor to 'performance' mode
- Ensure adequate free memory (>1GB recommended)
- Close other applications during benchmarking

## Integration

### Continuous Integration
The benchmark suite is designed for automated testing:
- Returns exit code 0 on all tests passing
- Provides machine-readable CSV output
- Consistent runtime across different systems
- Minimal external dependencies

### Performance Regression Detection
- Baseline performance targets for automated checking
- Historical result comparison capabilities
- Threshold-based pass/fail criteria
- Detailed performance breakdown for diagnosis

## Future Enhancements

- GPU acceleration benchmarks (CUDA/OpenCL)
- Multi-threaded clustering performance
- Network I/O performance for distributed clustering
- Visualization of clustering patterns and performance trends
- Integration with continuous benchmarking systems