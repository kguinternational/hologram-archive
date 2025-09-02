#!/bin/bash
# run-tests.sh - Test runner for Atlas-12288 LLVM tests

set -e

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
NC='\033[0m' # No Color

echo "Running Atlas-12288 tests..."

# Directory setup
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_DIR="$(dirname "$SCRIPT_DIR")"
BUILD_DIR="$PROJECT_DIR/build"

# Check if atlas module exists
if [ ! -f "$BUILD_DIR/atlas-12288.bc" ]; then
    echo -e "${RED}Error: Atlas module not built. Run 'make' first.${NC}"
    exit 1
fi

# Test counter
TESTS_RUN=0
TESTS_PASSED=0

# Function to run a test
run_test() {
    local test_name=$1
    local test_file="$SCRIPT_DIR/$test_name.ll"
    
    if [ ! -f "$test_file" ]; then
        echo -e "${RED}Test file not found: $test_file${NC}"
        return 1
    fi
    
    echo -n "Running $test_name... "
    
    # Link test with atlas module
    llvm-link "$BUILD_DIR/atlas-12288.bc" "$test_file" -o "$BUILD_DIR/$test_name.linked.bc" 2>/dev/null
    
    # Optimize
    opt -O2 "$BUILD_DIR/$test_name.linked.bc" -o "$BUILD_DIR/$test_name.opt.bc" 2>/dev/null
    
    # Compile to executable
    llc "$BUILD_DIR/$test_name.opt.bc" -o "$BUILD_DIR/$test_name.s" 2>/dev/null
    clang "$BUILD_DIR/$test_name.s" -o "$BUILD_DIR/$test_name.exe" -lm 2>/dev/null
    
    # Run test
    if "$BUILD_DIR/$test_name.exe" >/dev/null 2>&1; then
        echo -e "${GREEN}PASSED${NC}"
        ((TESTS_PASSED++))
    else
        echo -e "${RED}FAILED${NC}"
    fi
    
    ((TESTS_RUN++))
}

# Run all tests
run_test "test-r96"
run_test "test-conservation"
run_test "test-witness"

# Summary
echo ""
echo "Test Results: $TESTS_PASSED/$TESTS_RUN passed"

if [ $TESTS_PASSED -eq $TESTS_RUN ]; then
    echo -e "${GREEN}All tests passed!${NC}"
    exit 0
else
    echo -e "${RED}Some tests failed.${NC}"
    exit 1
fi