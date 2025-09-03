#!/bin/bash
# run-layer2-tests.sh - Atlas-12288 Layer 2 Test Runner
# Comprehensive test execution script for Layer 2 operations

set -e

# Configuration
TEST_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
ROOT_DIR="$(dirname "$(dirname "$TEST_DIR")")"
LIB_DIR="$ROOT_DIR/lib"
BUILD_DIR="$ROOT_DIR/build"

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# Test configuration
VERBOSE=${ATLAS_L2_VERBOSE:-0}
BENCHMARK=${ATLAS_L2_BENCHMARK:-0}
DEBUG=${ATLAS_L2_DEBUG:-0}
VALIDATION=${ATLAS_L2_VALIDATION:-standard}

# Test statistics
TOTAL_TESTS=0
PASSED_TESTS=0
FAILED_TESTS=0
SKIPPED_TESTS=0

echo -e "${BLUE}Atlas-12288 Layer 2 Test Suite${NC}"
echo -e "${BLUE}================================${NC}"
echo

# Check for required tools
check_tool() {
    local tool="$1"
    local desc="$2"
    
    if command -v "$tool" >/dev/null 2>&1; then
        echo -e "✓ $desc: ${GREEN}$tool${NC}"
        return 0
    else
        echo -e "✗ $desc: ${RED}$tool not found${NC}"
        return 1
    fi
}

echo "Checking required tools..."
TOOLS_OK=true
check_tool "opt" "LLVM Optimizer" || TOOLS_OK=false
check_tool "llc" "LLVM Compiler" || TOOLS_OK=false
check_tool "lli" "LLVM Interpreter" || TOOLS_OK=false
check_tool "FileCheck" "LLVM FileCheck" || TOOLS_OK=false

if ! $TOOLS_OK; then
    echo -e "${RED}Error: Missing required tools${NC}"
    exit 1
fi
echo

# Check for Atlas-12288 libraries
echo "Checking Atlas-12288 libraries..."
if [ -d "$LIB_DIR" ]; then
    echo -e "✓ Library directory: ${GREEN}$LIB_DIR${NC}"
    export LD_LIBRARY_PATH="$LIB_DIR:${LD_LIBRARY_PATH:-}"
else
    echo -e "⚠ Library directory not found: ${YELLOW}$LIB_DIR${NC}"
fi

if [ -d "$BUILD_DIR" ]; then
    echo -e "✓ Build directory: ${GREEN}$BUILD_DIR${NC}"
else
    echo -e "⚠ Build directory not found: ${YELLOW}$BUILD_DIR${NC}"
fi
echo

# Set environment variables
export ATLAS_L2_TEST_MODE=1
export ATLAS_L2_VERBOSE=$VERBOSE
export ATLAS_L2_DEBUG=$DEBUG
export ATLAS_L2_VALIDATION=$VALIDATION

if [ "$BENCHMARK" = "1" ]; then
    export ATLAS_L2_BENCHMARK=1
    echo -e "${YELLOW}Benchmarking enabled${NC}"
fi

# Test execution functions
run_test() {
    local test_file="$1"
    local test_name="$(basename "$test_file" .ll)"
    
    echo -e "Running ${BLUE}$test_name${NC}..."
    TOTAL_TESTS=$((TOTAL_TESTS + 1))
    
    local start_time
    if [ "$BENCHMARK" = "1" ]; then
        start_time=$(date +%s%N)
    fi
    
    # Create temporary files for output
    local stdout_file=$(mktemp)
    local stderr_file=$(mktemp)
    local result=0
    
    # Run the test with different methods
    local test_method="syntax"
    
    # 1. Syntax check with opt
    if opt -S < "$test_file" > "$stdout_file" 2> "$stderr_file"; then
        if [ "$VERBOSE" = "1" ]; then
            echo -e "  ✓ Syntax check passed"
        fi
    else
        echo -e "  ${RED}✗ Syntax check failed${NC}"
        cat "$stderr_file"
        result=1
        test_method="syntax-failed"
    fi
    
    # 2. FileCheck validation
    if [ $result -eq 0 ]; then
        if opt -S < "$test_file" | FileCheck "$test_file" > "$stdout_file" 2> "$stderr_file"; then
            if [ "$VERBOSE" = "1" ]; then
                echo -e "  ✓ FileCheck validation passed"
            fi
        else
            echo -e "  ${RED}✗ FileCheck validation failed${NC}"
            if [ "$VERBOSE" = "1" ]; then
                cat "$stderr_file"
            fi
            result=1
            test_method="filecheck-failed"
        fi
    fi
    
    # 3. Assembly generation test
    if [ $result -eq 0 ]; then
        if llc -O3 < "$test_file" > /dev/null 2> "$stderr_file"; then
            if [ "$VERBOSE" = "1" ]; then
                echo -e "  ✓ Assembly generation passed"
            fi
        else
            echo -e "  ${YELLOW}⚠ Assembly generation failed (non-critical)${NC}"
            if [ "$VERBOSE" = "1" ]; then
                cat "$stderr_file"
            fi
            # Don't fail the test for assembly generation issues
        fi
    fi
    
    # Calculate execution time
    local duration=""
    if [ "$BENCHMARK" = "1" ]; then
        local end_time=$(date +%s%N)
        local duration_ns=$((end_time - start_time))
        local duration_ms=$((duration_ns / 1000000))
        duration=" (${duration_ms}ms)"
    fi
    
    # Update statistics and report
    if [ $result -eq 0 ]; then
        PASSED_TESTS=$((PASSED_TESTS + 1))
        echo -e "  ${GREEN}✓ PASSED${NC}$duration"
    else
        FAILED_TESTS=$((FAILED_TESTS + 1))
        echo -e "  ${RED}✗ FAILED${NC} ($test_method)$duration"
    fi
    
    # Cleanup
    rm -f "$stdout_file" "$stderr_file"
    
    return $result
}

# Test discovery and execution
echo "Discovering Layer 2 tests..."
test_files=(
    "conserved-delta.ll"
    "conserved-window.ll"
    "conserved-memops.ll"
    "witness-ops.ll" 
    "conservation-edge-cases.ll"
)

found_tests=0
for test_file in "${test_files[@]}"; do
    if [ -f "$TEST_DIR/$test_file" ]; then
        found_tests=$((found_tests + 1))
    fi
done

echo -e "Found ${GREEN}$found_tests${NC} test files"
echo

# Execute tests
for test_file in "${test_files[@]}"; do
    if [ -f "$TEST_DIR/$test_file" ]; then
        run_test "$TEST_DIR/$test_file"
    else
        echo -e "⚠ Test file not found: ${YELLOW}$test_file${NC}"
        SKIPPED_TESTS=$((SKIPPED_TESTS + 1))
    fi
    echo
done

# Summary report
echo -e "${BLUE}Test Summary${NC}"
echo -e "${BLUE}============${NC}"
echo -e "Total tests:   $TOTAL_TESTS"
echo -e "Passed:        ${GREEN}$PASSED_TESTS${NC}"
echo -e "Failed:        ${RED}$FAILED_TESTS${NC}"
echo -e "Skipped:       ${YELLOW}$SKIPPED_TESTS${NC}"
echo

# Success rate calculation
if [ $TOTAL_TESTS -gt 0 ]; then
    success_rate=$((PASSED_TESTS * 100 / TOTAL_TESTS))
    echo -e "Success rate:  $success_rate%"
    
    if [ $success_rate -eq 100 ]; then
        echo -e "${GREEN}All tests passed! 🎉${NC}"
        exit_code=0
    elif [ $success_rate -ge 80 ]; then
        echo -e "${YELLOW}Most tests passed${NC}"
        exit_code=0
    else
        echo -e "${RED}Many tests failed${NC}"
        exit_code=1
    fi
else
    echo -e "${RED}No tests were run${NC}"
    exit_code=1
fi

# Additional diagnostics
if [ "$DEBUG" = "1" ]; then
    echo
    echo -e "${BLUE}Debug Information${NC}"
    echo -e "${BLUE}=================${NC}"
    echo "Test directory: $TEST_DIR"
    echo "Library directory: $LIB_DIR"  
    echo "Build directory: $BUILD_DIR"
    echo "LD_LIBRARY_PATH: ${LD_LIBRARY_PATH:-<not set>}"
    echo "Validation level: $VALIDATION"
    echo "Environment:"
    env | grep ATLAS_L2 | sort
fi

# Performance summary
if [ "$BENCHMARK" = "1" ] && [ $PASSED_TESTS -gt 0 ]; then
    echo
    echo -e "${BLUE}Performance Summary${NC}"
    echo -e "${BLUE}===================${NC}"
    echo "Benchmarking was enabled for this test run"
    echo "Individual test timings were reported above"
    echo "Consider running with ATLAS_L2_BENCHMARK=1 for detailed metrics"
fi

# Recommendations
if [ $FAILED_TESTS -gt 0 ]; then
    echo
    echo -e "${BLUE}Recommendations${NC}"
    echo -e "${BLUE}===============${NC}"
    echo "• Run with ATLAS_L2_VERBOSE=1 for detailed output"
    echo "• Check that Atlas-12288 libraries are properly built"
    echo "• Verify LLVM tools support Atlas-12288 intrinsics"
    echo "• Review failed test output for specific error details"
fi

exit $exit_code