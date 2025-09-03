#!/bin/bash
# run-tests.sh - Test runner script for Atlas Layer 2 Runtime Tests
# (c) 2024-2025 UOR Foundation - MIT License
#
# Compiles and runs Layer 2 tests with various configurations:
# - Property-based tests
# - Concurrency tests  
# - ThreadSanitizer race detection
# - Memory error detection

set -e  # Exit on any error

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# Test configuration
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
RUNTIME_ROOT="$(dirname "$SCRIPT_DIR")"
BUILD_DIR="$SCRIPT_DIR/build"
RESULTS_DIR="$SCRIPT_DIR/results"

# Compiler and flags
CC="${CC:-gcc}"
# Include paths for Layer 2 headers and Layer 0 headers
CFLAGS_BASE="-std=c11 -Wall -Wextra -I$RUNTIME_ROOT/include -I$RUNTIME_ROOT/../../layer0-atlas/include -I$RUNTIME_ROOT/../../../include"
CFLAGS_DEBUG="$CFLAGS_BASE -g -O0 -DDEBUG"
CFLAGS_RELEASE="$CFLAGS_BASE -O2 -DNDEBUG"
CFLAGS_TSAN="$CFLAGS_DEBUG -fsanitize=thread -fPIE"
CFLAGS_ASAN="$CFLAGS_DEBUG -fsanitize=address -fsanitize=undefined"

# Link against both Layer 0 and Layer 2 libraries
PROJECT_ROOT="$(cd "$RUNTIME_ROOT/../.." && pwd)"
LAYER0_LIB="$PROJECT_ROOT/lib/libatlas-core.a"
LAYER2_LIB="$PROJECT_ROOT/lib/libatlas-conservation.a"
LDFLAGS_BASE="-L$PROJECT_ROOT/lib -latlas-conservation -latlas-core -lm -lpthread"
LDFLAGS_TSAN="$LDFLAGS_BASE -fsanitize=thread -pie"
LDFLAGS_ASAN="$LDFLAGS_BASE -fsanitize=address -fsanitize=undefined"

# Runtime source files - include Layer 0 stubs for C-API exports
RUNTIME_SRC="$RUNTIME_ROOT/runtime/conservation.c $PROJECT_ROOT/layers/layer0-atlas/runtime-stubs.c"

print_header() {
    echo -e "${BLUE}===============================================${NC}"
    echo -e "${BLUE}$1${NC}"
    echo -e "${BLUE}===============================================${NC}"
}

print_success() {
    echo -e "${GREEN}✓ $1${NC}"
}

print_error() {
    echo -e "${RED}✗ $1${NC}"
}

print_warning() {
    echo -e "${YELLOW}⚠ $1${NC}"
}

print_info() {
    echo -e "${BLUE}ℹ $1${NC}"
}

# Create build directories
setup_directories() {
    mkdir -p "$BUILD_DIR"
    mkdir -p "$RESULTS_DIR"
    print_info "Created build directories"
}

# Check dependencies
check_dependencies() {
    print_info "Checking dependencies..."
    
    # Check compiler
    if ! command -v "$CC" &> /dev/null; then
        print_error "Compiler '$CC' not found"
        exit 1
    fi
    
    # Check for required layer libraries
    if [ ! -f "$LAYER0_LIB" ]; then
        print_error "Layer 0 library not found: $LAYER0_LIB"
        print_info "Please build Layer 0 first: make -C ../../layer0-atlas"
        exit 1
    fi
    
    if [ ! -f "$LAYER2_LIB" ]; then
        print_warning "Layer 2 library not found - building it now..."
        (cd "$RUNTIME_ROOT" && make) || {
            print_error "Failed to build Layer 2 library"
            exit 1
        }
    fi
    
    print_success "Dependencies checked"
}

# Compile test program
compile_test() {
    local test_name="$1"
    local cflags="$2"
    local ldflags="$3"
    local output_suffix="$4"
    
    local src_file="$SCRIPT_DIR/test-$test_name.c"
    local obj_file="$BUILD_DIR/test-$test_name$output_suffix.o"
    local exe_file="$BUILD_DIR/test-$test_name$output_suffix"
    
    if [ ! -f "$src_file" ]; then
        print_error "Test source file not found: $src_file"
        return 1
    fi
    
    print_info "Compiling $test_name with $output_suffix configuration..." >&2
    
    # Compile test and runtime together
    local sources="$src_file $RUNTIME_SRC"
    
    if ! $CC $cflags $sources -o "$exe_file" $ldflags 2>/dev/null; then
        print_error "Compilation failed for $test_name$output_suffix" >&2
        return 1
    fi
    
    print_success "Compiled $test_name$output_suffix" >&2
    echo "$exe_file"
}

# Run a single test
run_test() {
    local test_exe="$1"
    local test_name="$2"
    local config="$3"
    local timeout="${4:-300}" # Default 5 minute timeout
    
    local log_file="$RESULTS_DIR/$(basename "$test_exe").log"
    local result_file="$RESULTS_DIR/$(basename "$test_exe").result"
    
    print_info "Running $test_name ($config)..."
    
    # Run test with timeout
    local start_time=$(date +%s)
    if timeout "$timeout" "$test_exe" > "$log_file" 2>&1; then
        local end_time=$(date +%s)
        local duration=$((end_time - start_time))
        echo "PASSED $duration" > "$result_file"
        print_success "$test_name ($config) - PASSED in ${duration}s"
        return 0
    else
        local exit_code=$?
        local end_time=$(date +%s)
        local duration=$((end_time - start_time))
        echo "FAILED $exit_code $duration" > "$result_file"
        
        if [ $exit_code -eq 124 ]; then
            print_error "$test_name ($config) - TIMEOUT after ${timeout}s"
        else
            print_error "$test_name ($config) - FAILED with exit code $exit_code after ${duration}s"
        fi
        
        # Show last few lines of output for debugging
        echo "Last 10 lines of output:"
        tail -n 10 "$log_file"
        return 1
    fi
}

# Run property-based tests
run_property_tests() {
    print_header "PROPERTY-BASED TESTS"
    
    local passed=0
    local failed=0
    
    # Standard build
    if exe=$(compile_test "properties" "$CFLAGS_RELEASE" "$LDFLAGS_BASE" ""); then
        if run_test "$exe" "Property Tests" "Release"; then
            ((passed++))
        else
            ((failed++))
        fi
    else
        ((failed++))
    fi
    
    # Debug build with address sanitizer
    if exe=$(compile_test "properties" "$CFLAGS_ASAN" "$LDFLAGS_ASAN" "-asan"); then
        if run_test "$exe" "Property Tests" "AddressSanitizer"; then
            ((passed++))
        else
            ((failed++))
        fi
    else
        print_warning "AddressSanitizer build failed - skipping"
    fi
    
    echo
    echo "Property Tests Summary: $passed passed, $failed failed"
    return $failed
}

# Run concurrency tests
run_concurrency_tests() {
    print_header "CONCURRENCY TESTS"
    
    local passed=0
    local failed=0
    
    # Standard threaded build
    if exe=$(compile_test "concurrency" "$CFLAGS_RELEASE" "$LDFLAGS_BASE" ""); then
        if run_test "$exe" "Concurrency Tests" "Release" 600; then # Longer timeout
            ((passed++))
        else
            ((failed++))
        fi
    else
        ((failed++))
    fi
    
    # ThreadSanitizer build for race detection
    if command -v "$CC" &> /dev/null && $CC --version | grep -q "clang\|gcc"; then
        if exe=$(compile_test "concurrency" "$CFLAGS_TSAN" "$LDFLAGS_TSAN" "-tsan"); then
            print_info "Running ThreadSanitizer race detection..."
            if run_test "$exe" "Concurrency Tests" "ThreadSanitizer" 900; then # Even longer timeout
                ((passed++))
            else
                print_warning "ThreadSanitizer detected issues - check logs"
                ((failed++))
            fi
        else
            print_warning "ThreadSanitizer build failed - skipping"
        fi
    else
        print_warning "ThreadSanitizer requires clang or newer gcc - skipping"
    fi
    
    echo
    echo "Concurrency Tests Summary: $passed passed, $failed failed"
    return $failed
}

# Generate test report
generate_report() {
    local total_passed=0
    local total_failed=0
    local report_file="$RESULTS_DIR/test-report.txt"
    
    print_header "TEST REPORT"
    
    echo "Atlas Layer 2 Runtime Test Report" > "$report_file"
    echo "Generated: $(date)" >> "$report_file"
    echo "=========================================" >> "$report_file"
    echo >> "$report_file"
    
    # Process results
    for result_file in "$RESULTS_DIR"/*.result; do
        if [ -f "$result_file" ]; then
            local test_name=$(basename "$result_file" .result)
            local result=$(cat "$result_file")
            
            if echo "$result" | grep -q "^PASSED"; then
                local duration=$(echo "$result" | cut -d' ' -f2)
                echo "✓ $test_name - PASSED (${duration}s)" | tee -a "$report_file"
                ((total_passed++))
            else
                local exit_code=$(echo "$result" | cut -d' ' -f2)
                local duration=$(echo "$result" | cut -d' ' -f3)
                echo "✗ $test_name - FAILED (exit $exit_code, ${duration}s)" | tee -a "$report_file"
                ((total_failed++))
            fi
        fi
    done
    
    echo >> "$report_file"
    echo "Summary: $total_passed passed, $total_failed failed" | tee -a "$report_file"
    
    if [ $total_failed -eq 0 ]; then
        print_success "All tests PASSED!"
        echo "🎉 All tests PASSED!" >> "$report_file"
    else
        print_error "Some tests FAILED!"
        echo "💥 $total_failed tests FAILED!" >> "$report_file"
    fi
    
    print_info "Full report written to: $report_file"
    return $total_failed
}

# Clean build artifacts
clean() {
    if [ -d "$BUILD_DIR" ]; then
        rm -rf "$BUILD_DIR"
        print_success "Cleaned build directory"
    fi
    
    if [ -d "$RESULTS_DIR" ]; then
        rm -rf "$RESULTS_DIR"
        print_success "Cleaned results directory"
    fi
}

# Show help
show_help() {
    echo "Usage: $0 [OPTIONS]"
    echo
    echo "Options:"
    echo "  --property-only    Run only property-based tests"
    echo "  --concurrency-only Run only concurrency tests"
    echo "  --no-tsan         Skip ThreadSanitizer tests"
    echo "  --no-asan         Skip AddressSanitizer tests"
    echo "  --clean           Clean build artifacts and exit"
    echo "  --help            Show this help message"
    echo
    echo "Environment variables:"
    echo "  CC                Compiler to use (default: gcc)"
    echo
    echo "Examples:"
    echo "  $0                         # Run all tests"
    echo "  $0 --property-only         # Run only property tests"
    echo "  CC=clang $0                # Use clang compiler"
    echo "  $0 --clean                 # Clean and exit"
}

# Main function
main() {
    local run_property=true
    local run_concurrency=true
    local use_tsan=true
    local use_asan=true
    
    # Parse command line arguments
    while [[ $# -gt 0 ]]; do
        case $1 in
            --property-only)
                run_property=true
                run_concurrency=false
                shift
                ;;
            --concurrency-only)
                run_property=false
                run_concurrency=true
                shift
                ;;
            --no-tsan)
                use_tsan=false
                shift
                ;;
            --no-asan)
                use_asan=false
                shift
                ;;
            --clean)
                clean
                exit 0
                ;;
            --help)
                show_help
                exit 0
                ;;
            *)
                print_error "Unknown option: $1"
                show_help
                exit 1
                ;;
        esac
    done
    
    print_header "ATLAS LAYER 2 RUNTIME TESTS"
    echo "Compiler: $CC"
    echo "Test directory: $SCRIPT_DIR"
    echo "Runtime root: $RUNTIME_ROOT"
    echo
    
    # Setup
    setup_directories
    check_dependencies
    
    local total_failures=0
    
    # Run tests
    if [ "$run_property" = true ]; then
        if ! run_property_tests; then
            ((total_failures += $?))
        fi
    fi
    
    if [ "$run_concurrency" = true ]; then
        if ! run_concurrency_tests; then
            ((total_failures += $?))
        fi
    fi
    
    # Generate final report
    generate_report
    local report_failures=$?
    
    exit $report_failures
}

# Run main function with all arguments
main "$@"