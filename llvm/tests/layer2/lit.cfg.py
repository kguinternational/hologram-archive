# -*- Python -*-
# Configuration file for Layer 2 Atlas-12288 LLVM tests

import os
import sys
import subprocess
import lit.formats

# Basic configuration
config.name = 'Atlas-12288-Layer2'
config.test_format = lit.formats.ShTest(True)

# Test suffixes
config.suffixes = ['.ll']

# Exclude files that aren't tests
config.excludes = ['lit.cfg.py']

# Test source root
config.test_source_root = os.path.dirname(__file__)

# Atlas-12288 specific tools and paths
atlas_build_dir = os.path.join(config.test_source_root, '..', '..', 'build')
atlas_lib_dir = os.path.join(config.test_source_root, '..', '..', 'lib')

# Tool substitutions
config.substitutions.append(('%opt', 'opt'))
config.substitutions.append(('%llc', 'llc')) 
config.substitutions.append(('%lli', 'lli'))
config.substitutions.append(('%FileCheck', 'FileCheck'))

# Atlas-12288 library paths
if os.path.exists(atlas_lib_dir):
    config.environment['LD_LIBRARY_PATH'] = atlas_lib_dir + ':' + config.environment.get('LD_LIBRARY_PATH', '')

# Feature detection for different test capabilities
features = []

# Check for Atlas-12288 Layer 2 support
try:
    # Try to run a simple opt command to check if Atlas intrinsics are available
    devnull = open(os.devnull, 'w')
    result = subprocess.call(['opt', '--version'], stdout=devnull, stderr=devnull)
    if result == 0:
        features.append('opt-available')
    devnull.close()
except:
    pass

# Check for execution support
try:
    devnull = open(os.devnull, 'w')
    result = subprocess.call(['lli', '--version'], stdout=devnull, stderr=devnull)
    if result == 0:
        features.append('lli-available')
    devnull.close()
except:
    pass

# Check for assembly generation
try:
    devnull = open(os.devnull, 'w')
    result = subprocess.call(['llc', '--version'], stdout=devnull, stderr=devnull)
    if result == 0:
        features.append('llc-available')
    devnull.close()
except:
    pass

# Check for FileCheck availability
try:
    devnull = open(os.devnull, 'w')
    result = subprocess.call(['FileCheck', '--version'], stdout=devnull, stderr=devnull)
    if result == 0:
        features.append('filecheck-available')
    devnull.close()
except:
    pass

# Platform-specific features
import platform
if platform.system() == 'Linux':
    features.append('linux')
elif platform.system() == 'Darwin':
    features.append('macos')
elif platform.system() == 'Windows':
    features.append('windows')

# Architecture-specific features
machine = platform.machine().lower()
if machine in ['x86_64', 'amd64']:
    features.append('x86_64')
elif machine in ['aarch64', 'arm64']:
    features.append('aarch64')

config.available_features.update(features)

# Timeout for tests (30 seconds)
try:
    import psutil
    config.maxIndividualTestTime = 30
except ImportError:
    pass

# Environment variables for Atlas-12288 Layer 2 testing
config.environment['ATLAS_L2_TEST_MODE'] = '1'
config.environment['ATLAS_L2_VERBOSE'] = '0'  # Set to 1 for verbose output

# Test-specific substitutions for Atlas-12288 Layer 2
config.substitutions.extend([
    # Common test patterns
    ('%atlas-opt', 'opt -load=' + os.path.join(atlas_lib_dir, 'libatlas.so') if os.path.exists(os.path.join(atlas_lib_dir, 'libatlas.so')) else 'opt'),
    ('%atlas-llc', 'llc -load=' + os.path.join(atlas_lib_dir, 'libatlas.so') if os.path.exists(os.path.join(atlas_lib_dir, 'libatlas.so')) else 'llc'),
    ('%atlas-lli', 'lli -load=' + os.path.join(atlas_lib_dir, 'libatlas.so') if os.path.exists(os.path.join(atlas_lib_dir, 'libatlas.so')) else 'lli'),
    
    # Layer 2 specific patterns
    ('%l2-test-size', '96'),  # Standard test buffer size
    ('%l2-max-size', '12288'),  # Maximum Atlas-12288 buffer size
    ('%l2-check-conservation', 'FileCheck --check-prefix=CONSERVED'),
    ('%l2-check-witness', 'FileCheck --check-prefix=WITNESS'),
    ('%l2-check-delta', 'FileCheck --check-prefix=DELTA'),
    ('%l2-check-window', 'FileCheck --check-prefix=WINDOW'),
])

# Test execution order and dependencies
config.test_exec_order = 'lexical'

# Parallelism - Layer 2 tests can run in parallel
config.parallelism_group = 'atlas-l2'

# Test result formatting
def format_test_output(result):
    """Format test output for better readability"""
    if result.code == 0:
        return f"✓ {result.name}"
    else:
        return f"✗ {result.name} - {result.output}"

# Custom test discovery for Layer 2 patterns
def discover_layer2_tests(path):
    """Discover Layer 2 specific test patterns"""
    tests = []
    for root, dirs, files in os.walk(path):
        for file in files:
            if file.endswith('.ll'):
                # Check if file contains Layer 2 intrinsics
                full_path = os.path.join(root, file)
                try:
                    with open(full_path, 'r') as f:
                        content = f.read()
                        if 'atlas.l2.' in content:
                            tests.append(full_path)
                except:
                    pass
    return tests

# Performance benchmarking support
if 'ATLAS_L2_BENCHMARK' in os.environ:
    config.environment['ATLAS_L2_BENCHMARK'] = '1'
    config.substitutions.append(('%benchmark', 'time'))
else:
    config.substitutions.append(('%benchmark', ''))

# Debug mode support
if 'ATLAS_L2_DEBUG' in os.environ:
    config.environment['ATLAS_L2_DEBUG'] = '1'
    config.substitutions.extend([
        ('%debug-opt', 'opt -debug'),
        ('%debug-llc', 'llc -debug'),
    ])
else:
    config.substitutions.extend([
        ('%debug-opt', 'opt'),
        ('%debug-llc', 'llc'),
    ])

# Memory sanitizer support for edge case testing
if 'ATLAS_L2_ASAN' in os.environ:
    config.environment['ASAN_OPTIONS'] = 'detect_stack_use_after_return=1:check_initialization_order=1'
    config.substitutions.extend([
        ('%asan-opt', 'opt -fsanitize=address'),
        ('%asan-llc', 'llc -fsanitize=address'),
    ])

# Validation levels
validation_level = os.environ.get('ATLAS_L2_VALIDATION', 'standard')
if validation_level == 'strict':
    config.environment['ATLAS_L2_STRICT_VALIDATION'] = '1'
elif validation_level == 'minimal':
    config.environment['ATLAS_L2_MINIMAL_VALIDATION'] = '1'

# Test categories based on filename patterns
def categorize_test(test_name):
    """Categorize tests based on filename patterns"""
    if 'delta' in test_name:
        return 'delta-computation'
    elif 'window' in test_name:
        return 'windowed-conservation'
    elif 'memops' in test_name:
        return 'memory-operations'
    elif 'witness' in test_name:
        return 'witness-verification'
    elif 'edge-cases' in test_name:
        return 'edge-cases'
    else:
        return 'general'

# Expected test counts for validation
expected_test_counts = {
    'delta-computation': 7,      # conserved-delta.ll tests
    'windowed-conservation': 9,   # conserved-window.ll tests  
    'memory-operations': 10,     # conserved-memops.ll tests
    'witness-verification': 9,   # witness-ops.ll tests
    'edge-cases': 10,           # conservation-edge-cases.ll tests
}

# Regression test markers
regression_tests = [
    'test_empty_buffer',
    'test_single_byte',
    'test_overflow_boundaries',
    'test_basic_delta',
    'test_basic_window_check',
    'test_basic_conserved_memcpy',
    'test_basic_witness_ops',
]

# Mark regression tests for priority execution
for test in regression_tests:
    config.substitutions.append((f'%{test}', f'{test} # REGRESSION'))

# Cross-validation patterns - tests that should produce consistent results
cross_validation_patterns = [
    ('delta-computation', 'memory-operations'),  # Delta and memops should be consistent
    ('witness-verification', 'edge-cases'),      # Witness should handle edge cases
    ('windowed-conservation', 'edge-cases'),     # Window ops should handle edge cases
]

# Performance thresholds for benchmarking
performance_thresholds = {
    'small-buffer': 0.001,   # < 1ms for buffers <= 1KB
    'medium-buffer': 0.01,   # < 10ms for buffers <= 4KB  
    'large-buffer': 0.1,     # < 100ms for buffers <= 12KB
}

# Test result validation
def validate_test_result(test_name, result):
    """Validate test results meet Layer 2 requirements"""
    category = categorize_test(test_name)
    
    # Check expected test count
    if category in expected_test_counts:
        # This would need integration with actual test runner
        pass
    
    # Performance validation
    if 'ATLAS_L2_BENCHMARK' in os.environ and result.elapsed_time:
        if 'small' in test_name and result.elapsed_time > performance_thresholds['small-buffer']:
            return False, f"Performance regression: {result.elapsed_time}s > {performance_thresholds['small-buffer']}s"
        elif 'large' in test_name and result.elapsed_time > performance_thresholds['large-buffer']:
            return False, f"Performance regression: {result.elapsed_time}s > {performance_thresholds['large-buffer']}s"
    
    return True, "OK"

# Documentation and reporting
config.test_description = """
Atlas-12288 Layer 2 Test Suite

This test suite validates the Layer 2 operations of the Atlas-12288 system:
- Delta computation and application
- Windowed conservation checking  
- Conserved memory operations (memcpy/memset with fixups)
- Witness generation and verification
- Edge cases and boundary conditions

Test Categories:
- Delta Computation: Tests mathematical delta operations
- Windowed Conservation: Tests sliding window conservation checks
- Memory Operations: Tests conserved memcpy/memset operations
- Witness Verification: Tests integrity witness operations
- Edge Cases: Tests boundary conditions and error cases

Usage:
  lit /path/to/layer2/tests/           # Run all tests
  lit /path/to/layer2/tests/ -v        # Verbose output
  ATLAS_L2_BENCHMARK=1 lit ...         # Enable benchmarking
  ATLAS_L2_DEBUG=1 lit ...             # Enable debug output
"""

# Configuration summary
def print_config_summary():
    """Print configuration summary if requested"""
    if os.environ.get('ATLAS_L2_SHOW_CONFIG'):
        print(f"Atlas-12288 Layer 2 Test Configuration:")
        print(f"  Available features: {sorted(config.available_features)}")
        print(f"  Test source root: {config.test_source_root}")
        print(f"  Library directory: {atlas_lib_dir}")
        print(f"  Validation level: {validation_level}")
        print(f"  Expected categories: {list(expected_test_counts.keys())}")

print_config_summary()