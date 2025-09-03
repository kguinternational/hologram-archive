# Atlas-12288 Layer 2 Test Suite

This directory contains comprehensive lit/FileCheck tests for the Atlas-12288 Layer 2 operations. The tests validate the correctness, performance, and edge case handling of Layer 2 functionality.

## Test Files

### 1. `conserved-delta.ll` - Delta Computation Tests
Tests the mathematical delta computation operations:
- **Basic delta computation**: Tests delta calculation between buffer states
- **Zero delta cases**: Validates handling of identical buffers  
- **Mixed pattern deltas**: Tests complex patterns with symmetric properties
- **Delta application**: Tests applying computed deltas to buffers
- **Delta validation**: Tests overflow and boundary validation
- **Edge cases**: Single byte, maximum size, and boundary conditions

**Key Tests:**
- `test_basic_delta()`: Basic delta between uniform patterns
- `test_zero_delta()`: Identical buffer delta (should be 0)
- `test_mixed_pattern_delta()`: Complex symmetric patterns
- `test_delta_apply()`: Applying deltas to modify buffers
- `test_large_buffer_delta()`: Maximum Atlas-12288 size testing

### 2. `conserved-window.ll` - Windowed Conservation Tests
Tests sliding window conservation checking:
- **Basic window checks**: Tests conservation within fixed-size windows
- **Overlapping windows**: Tests conservation with overlapping window patterns
- **Window sum computation**: Tests accurate sum calculation within windows
- **Window adjustment**: Tests adjusting buffers to maintain window conservation
- **Multiple window sizes**: Tests conservation across different window dimensions
- **Boundary conditions**: Tests windows at buffer boundaries

**Key Tests:**
- `test_basic_window_check()`: 32-byte window conservation validation
- `test_overlapping_windows()`: 16-byte windows with 8-byte stride
- `test_window_sum()`: Accurate sum computation within windows
- `test_window_adjust()`: Automatic adjustment to maintain conservation
- `test_sliding_window()`: Dynamic sliding window patterns

### 3. `conserved-memops.ll` - Conserved Memory Operations Tests
Tests memory operations that maintain conservation laws:
- **Conserved memcpy**: Tests copying while maintaining conservation
- **Conserved memset**: Tests setting values while preserving conservation
- **Memcpy with fixup**: Tests copying with automatic conservation fixups
- **Overlapping regions**: Tests handling of overlapping memory regions
- **Partial operations**: Tests operations on buffer sub-regions
- **Large buffer operations**: Tests performance on maximum-sized buffers

**Key Tests:**
- `test_basic_conserved_memcpy()`: Conservation-preserving buffer copies
- `test_conserved_memset()`: Conservation-aware memory initialization
- `test_memcpy_with_fixup()`: Automatic fixup during non-conserved copies
- `test_overlapping_memcpy()`: Handling overlapping memory regions
- `test_large_buffer_ops()`: Performance with 12KB+ buffers

### 4. `witness-ops.ll` - Witness Generation and Verification Tests
Tests cryptographic witness operations for integrity verification:
- **Basic witness operations**: Generation and verification workflow
- **Multiple hash algorithms**: SHA256, Blake3, CRC32 algorithm support
- **Witness updates**: Incremental witness updates for modified regions
- **Witness comparison**: Equality testing between witnesses
- **Tamper detection**: Detection of data modification attempts
- **Performance characteristics**: Benchmarking different algorithm performance

**Key Tests:**
- `test_basic_witness_ops()`: Complete generation/verification cycle
- `test_witness_algorithms()`: SHA256, Blake3, and CRC32 algorithm testing
- `test_witness_update()`: Incremental updates for modified regions
- `test_tamper_detection()`: Single-bit modification detection
- `test_large_buffer_witness()`: Performance with large buffers

### 5. `conservation-edge-cases.ll` - Edge Cases and Boundary Conditions
Tests boundary conditions and edge cases:
- **Empty buffer handling**: Zero-length buffer conservation
- **Single byte operations**: Minimal buffer size operations
- **Overflow boundaries**: Prevention of integer overflow conditions
- **Maximum buffer sizes**: Atlas-12288 limit testing (12KB)
- **Non-standard sizes**: Prime numbers, odd sizes, power-of-2 testing
- **Integer wraparound**: Byte value wraparound and modular arithmetic
- **Unaligned access**: Non-aligned memory access patterns

**Key Tests:**
- `test_empty_buffer()`: Zero-length buffer edge case
- `test_single_byte()`: Single byte conservation (trivial case)
- `test_overflow_boundaries()`: Integer overflow prevention
- `test_maximum_buffer()`: 12288-byte maximum size testing
- `test_nonstandard_sizes()`: Prime, odd, and power-of-2 sizes
- `test_integer_wraparound()`: Full byte range (0-255) handling

## Test Configuration (`lit.cfg.py`)

The `lit.cfg.py` file provides comprehensive test configuration:

### Features
- **Tool detection**: Automatic detection of opt, llc, lli, FileCheck
- **Platform support**: Linux, macOS, Windows compatibility
- **Architecture support**: x86_64, AArch64 architecture detection
- **Library integration**: Automatic Atlas-12288 library loading

### Environment Variables
- `ATLAS_L2_TEST_MODE=1`: Enables Layer 2 test mode
- `ATLAS_L2_BENCHMARK=1`: Enables performance benchmarking
- `ATLAS_L2_DEBUG=1`: Enables verbose debug output
- `ATLAS_L2_VALIDATION=strict|standard|minimal`: Sets validation level
- `ATLAS_L2_ASAN=1`: Enables AddressSanitizer for memory safety

### Test Categories
Tests are automatically categorized based on content:
- **delta-computation**: Mathematical delta operations
- **windowed-conservation**: Window-based conservation
- **memory-operations**: Memory operation primitives
- **witness-verification**: Integrity verification
- **edge-cases**: Boundary and error conditions

## Running Tests

### Basic Usage
```bash
# Run all Layer 2 tests
lit /workspaces/Hologram/llvm/tests/layer2/

# Run with verbose output
lit /workspaces/Hologram/llvm/tests/layer2/ -v

# Run specific test file
lit /workspaces/Hologram/llvm/tests/layer2/conserved-delta.ll
```

### Advanced Usage
```bash
# Enable benchmarking
ATLAS_L2_BENCHMARK=1 lit /workspaces/Hologram/llvm/tests/layer2/

# Enable debug output
ATLAS_L2_DEBUG=1 lit /workspaces/Hologram/llvm/tests/layer2/ -v

# Strict validation mode
ATLAS_L2_VALIDATION=strict lit /workspaces/Hologram/llvm/tests/layer2/

# Memory safety testing
ATLAS_L2_ASAN=1 lit /workspaces/Hologram/llvm/tests/layer2/
```

## FileCheck Patterns

Each test uses FileCheck directives for validation:

### Standard Patterns
- `; RUN:` - Test execution commands
- `; CHECK:` - Output verification patterns
- `; CHECK-LABEL:` - Function/section labels
- `; ASM:` - Assembly code verification
- `; EXEC:` - Execution output verification

### Layer 2 Specific Patterns
- `; CHECK-CONSERVED:` - Conservation law verification
- `; CHECK-WITNESS:` - Witness operation verification
- `; CHECK-DELTA:` - Delta computation verification
- `; CHECK-WINDOW:` - Window operation verification

## Expected Test Counts

- **Delta Computation**: 7 test functions
- **Windowed Conservation**: 9 test functions
- **Memory Operations**: 10 test functions
- **Witness Verification**: 9 test functions
- **Edge Cases**: 10 test functions
- **Total**: 45 comprehensive test functions

## Performance Thresholds

- **Small buffers** (≤1KB): <1ms execution time
- **Medium buffers** (≤4KB): <10ms execution time
- **Large buffers** (≤12KB): <100ms execution time

## Integration

These tests integrate with the existing Atlas-12288 build system:
- Uses libraries from `/workspaces/Hologram/llvm/lib/`
- Integrates with the main test suite via `run-tests.sh`
- Supports parallel execution for performance
- Provides detailed error reporting and diagnostics

## Coverage

The test suite provides comprehensive coverage of:
- **Functional correctness**: All Layer 2 operations work as specified
- **Edge cases**: Boundary conditions and error handling
- **Performance**: Acceptable performance characteristics
- **Integration**: Proper interaction between Layer 2 components
- **Regression prevention**: Guards against future breaking changes

This test suite ensures the reliability and correctness of the Atlas-12288 Layer 2 implementation across all supported platforms and use cases.