# Atlas-12288 Layer 2 Benchmark Suite

## Quick Start

```bash
# Build and run simplified benchmarks (recommended)
make simple-release && make test

# Build all variants  
make all

# Run comprehensive comparison
make test-all

# Get help
make help
```

## Files

- **layer2-bench.c** - Complete benchmark suite (requires full Atlas C API)
- **layer2-bench-simple.c** - Simplified benchmarks focusing on memory operations and SIMD
- **Makefile** - Build system with multiple compiler and architecture targets
- **README.md** - This file

## Results

Latest benchmark run on AMD EPYC 7763:

```
Conserved memcpy          | Avg: 2629.0 ns  | Throughput:   4.67 GB/s | FAIL
Conserved memset          | Avg: 1857.0 ns  | Throughput:   6.62 GB/s | FAIL
Delta computation         | Avg:  908.4 ns  | Rate:      0.07 ns/byte | PASS
```

## Performance Targets

- Conserved memcpy (AVX2): ≥25 GB/s
- Conserved memset (AVX2): ≥30 GB/s  
- Delta computation: <10 ns/byte

## Documentation

See `/workspaces/Hologram/docs/benchmarks/l2.md` for detailed performance analysis and optimization recommendations.