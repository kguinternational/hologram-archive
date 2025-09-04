# Atlas Layer 4 Benchmark Harness

A comprehensive benchmarking framework for Atlas Layer 4 (Manifold) operations with high-resolution timing, memory tracking, statistical analysis, and conservation verification.

## Features

- **High-resolution timing** with nanosecond precision using POSIX monotonic clocks
- **Memory usage tracking** with allocation/deallocation monitoring
- **Statistical analysis** including mean, median, standard deviation, and percentiles
- **Conservation verification** ensuring Atlas conservation laws (sum % 96 == 0)
- **Multiple output formats** including text, CSV, and JSON
- **Warmup iterations** to minimize cold-start effects
- **Error handling** with detailed failure reporting
- **Progress indicators** for long-running benchmarks

## Components

### Core Framework (`benchmark_framework.h/c`)
- Benchmark context management
- High-resolution timing functions
- Memory tracking utilities
- Statistical analysis functions
- Conservation verification

### Utilities (`benchmark_utils.h/c`)
- Random data generation with conservation compliance
- Result formatting and reporting
- File I/O operations
- Console output with color support
- Performance comparison functions

### Runner (`benchmark_runner.c`)
- Main executable for running benchmarks
- Command-line interface
- Comprehensive configuration options
- Automated benchmark execution

## Building

### Prerequisites
- C11-compatible compiler (clang or gcc)
- Layer 4 (Manifold) library built
- POSIX-compliant system for timing functions

### Build Commands
```bash
# Build everything
make all

# Build with specific configuration
make debug          # Debug build
make release        # Release build (optimized)

# Build and run tests
make test           # Quick tests
make test-verbose   # Verbose tests
```

## Running Benchmarks

### Basic Usage
```bash
# Run all benchmarks with defaults
./build/bin/benchmark_runner

# Quick test with minimal iterations
./build/bin/benchmark_runner --iterations 10 --warmup 2

# Comprehensive benchmarking
./build/bin/benchmark_runner --iterations 10000 --warmup 1000 --verbose --csv --json --text
```

### Command Line Options
- `-h, --help` - Show help message
- `-o, --output-dir DIR` - Output directory (default: `benchmark_results`)
- `-i, --iterations N` - Number of iterations (default: 1000)
- `-w, --warmup N` - Warmup iterations (default: 100)
- `-v, --verbose` - Verbose output with detailed statistics
- `--csv` - Write CSV output files
- `--json` - Write JSON output files  
- `--text` - Write text report files
- `--no-colors` - Disable colored console output

### Advanced Options
- `-b, --baseline FILE` - Baseline file for comparison
- `-g, --generate-baseline` - Generate new baseline
- `-c, --compare` - Compare against baseline
- `-t, --threshold X` - Regression threshold (default: 1.1)
- `--abort-on-regression` - Stop on performance regression

## Benchmark Types

The harness includes the following Layer 4 benchmark tests:

1. **`projection_create`** - Atlas projection creation performance
2. **`projection_transform`** - Projection transformation performance  
3. **`shard_extract`** - Shard extraction from projections
4. **`shard_verify`** - Shard integrity verification
5. **`conservation_check`** - Conservation law verification
6. **`manifold_verify`** - Complete manifold verification
7. **`system_test`** - Comprehensive system test

## Output Formats

### Text Reports
Detailed human-readable reports with:
- Benchmark metadata (name, description, success status)
- Timing statistics (mean, median, min, max, std dev, percentiles)
- Memory usage statistics
- Conservation verification results

### CSV Format
Machine-readable format suitable for:
- Spreadsheet analysis
- Database import
- Automated processing
- Time-series analysis

### JSON Format  
Structured data format for:
- Web-based dashboards
- API integration
- Programmatic analysis
- Modern tooling integration

## Conservation Verification

The benchmark harness automatically verifies Atlas conservation laws:
- **Conservation Law**: Sum of all bytes modulo 96 must equal 0
- **Witness Generation**: Cryptographic witnesses for data integrity
- **Delta Calculation**: Conservation changes during operations
- **Failure Detection**: Automatic detection of conservation violations

## Performance Analysis

### Statistical Metrics
- **Mean/Median**: Central tendency measures
- **Min/Max**: Performance bounds
- **Standard Deviation**: Variability measurement
- **Percentiles**: Distribution analysis (50th, 90th, 95th, 99th)

### Memory Tracking
- **Total Allocated**: Cumulative memory allocation
- **Peak Usage**: Maximum concurrent memory usage
- **Allocation Count**: Number of allocation operations
- **Leak Detection**: Current allocated vs. deallocated

## Integration with Layer 4

The benchmark harness integrates seamlessly with Layer 4:

```bash
# From Layer 4 directory
make benchmark-test      # Build and test harness
make bench              # Run standard benchmarks
make bench-quick        # Run quick benchmarks
make bench-full         # Run comprehensive benchmarks
make benchmark-report   # Generate detailed report
```

## Example Output

```
Atlas Layer 4 Benchmark Runner v1.0.0
=======================================

Configuration:
  Output directory: benchmark_results
  Iterations: 1000
  Warmup iterations: 100
  Verbose output: Yes
  Colors: Yes
  Output formats: Text CSV JSON

Running benchmark: projection_create
=== projection_create ===
Description: Atlas projection creation performance
Success: Yes
Iterations: 1000 completed, 0 failed

Timing: mean=25847ns, median=25123ns, min=21456ns, max=45123ns
Memory: allocated=12288 bytes, peak=12288 bytes
Conservation: OK (checks=1000, passed=1000)

=== Benchmark Suite Summary ===
Total benchmarks: 7
Failed benchmarks: 0
Success rate: 100.0%
Total time: 2.34 s
Output directory: benchmark_results
```

## Extending the Harness

### Adding New Benchmarks
1. Implement benchmark function with signature:
   ```c
   int my_benchmark(benchmark_context_t* ctx, void* user_data);
   ```
2. Add entry to benchmark registry in `benchmark_runner.c`
3. Implement setup/cleanup functions if needed

### Custom Data Generation
Use the utility functions for conservation-compliant data:
```c
uint8_t* data = malloc(12288);
benchmark_generate_conserved_data(data, 12288, seed);
```

### Conservation Verification
```c
bool conserved = benchmark_verify_conservation(data, size);
uint8_t checksum = benchmark_calculate_checksum(data, size);
```

## Troubleshooting

### Build Issues
- Ensure Layer 4 library is built first
- Check compiler supports C11 standard
- Verify POSIX timing functions available

### Runtime Issues
- Insufficient permissions for file creation
- Clock resolution not available
- Memory allocation failures

### Performance Issues
- Too many iterations causing timeouts
- Excessive memory usage
- Conservation violations affecting results

## License

MIT License - See Layer 4 license for details.

(c) 2024-2025 UOR Foundation