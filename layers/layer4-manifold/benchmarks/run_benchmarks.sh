#!/usr/bin/env bash

# run_benchmarks.sh - Atlas Layer 4 Manifold Benchmark Runner
# (c) 2024-2025 UOR Foundation - MIT License
#
# Comprehensive benchmark suite for Layer 4 operations including:
# - Universal Number (UN) operations
# - Geometric computations (distance, curvature)
# - Signal processing (R96 Fourier, convolution)
# - Traditional baselines for comparison
# - Real-world applications (ML, DB, crypto)

set -euo pipefail

# Configuration
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
LAYER4_DIR="$(dirname "$SCRIPT_DIR")"
RESULTS_DIR="$SCRIPT_DIR/results"
BUILD_TYPE="${BUILD_TYPE:-release}"
NUM_ITERATIONS="${NUM_ITERATIONS:-1000}"
WARMUP_ITERATIONS="${WARMUP_ITERATIONS:-100}"

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# Logging functions
log_info() {
    echo -e "${BLUE}[INFO]${NC} $1"
}

log_success() {
    echo -e "${GREEN}[SUCCESS]${NC} $1"
}

log_warning() {
    echo -e "${YELLOW}[WARNING]${NC} $1"
}

log_error() {
    echo -e "${RED}[ERROR]${NC} $1"
}

# Print usage
usage() {
    cat << EOF
Usage: $0 [OPTIONS] [SUITE...]

Run Atlas Layer 4 Manifold benchmarks

OPTIONS:
    -h, --help              Show this help message
    -t, --type TYPE         Build type: debug|release (default: release)
    -i, --iterations N      Number of benchmark iterations (default: 1000)
    -w, --warmup N          Number of warmup iterations (default: 100)
    -o, --output DIR        Output directory for results (default: results/)
    -q, --quiet             Quiet mode - minimal output
    -v, --verbose           Verbose mode - detailed output
    --no-build              Skip building benchmarks
    --compare               Compare with baseline results
    --profile               Enable profiling with perf

SUITES:
    core                    Universal Number operations benchmarks
    geometric               Distance and curvature benchmarks  
    signal                  R96 Fourier and convolution benchmarks
    traditional             Baseline comparison implementations
    applications            Real-world ML, DB, crypto benchmarks
    all                     All benchmark suites (default)

EXAMPLES:
    $0                      Run all benchmarks with default settings
    $0 core geometric       Run only core and geometric benchmarks
    $0 -t debug -i 500      Run with debug build, 500 iterations
    $0 --profile core       Profile core benchmarks with perf

EOF
}

# Parse command line arguments
BUILD_BENCHMARKS=true
QUIET=false
VERBOSE=false
COMPARE=false
PROFILE=false
SUITES=()

while [[ $# -gt 0 ]]; do
    case $1 in
        -h|--help)
            usage
            exit 0
            ;;
        -t|--type)
            BUILD_TYPE="$2"
            shift 2
            ;;
        -i|--iterations)
            NUM_ITERATIONS="$2"
            shift 2
            ;;
        -w|--warmup)
            WARMUP_ITERATIONS="$2"
            shift 2
            ;;
        -o|--output)
            RESULTS_DIR="$2"
            shift 2
            ;;
        -q|--quiet)
            QUIET=true
            shift
            ;;
        -v|--verbose)
            VERBOSE=true
            shift
            ;;
        --no-build)
            BUILD_BENCHMARKS=false
            shift
            ;;
        --compare)
            COMPARE=true
            shift
            ;;
        --profile)
            PROFILE=true
            shift
            ;;
        core|geometric|signal|traditional|applications|all)
            SUITES+=("$1")
            shift
            ;;
        *)
            log_error "Unknown option: $1"
            usage
            exit 1
            ;;
    esac
done

# Default to all suites if none specified
if [[ ${#SUITES[@]} -eq 0 ]]; then
    SUITES=("all")
fi

# Validate build type
case "$BUILD_TYPE" in
    debug|release)
        ;;
    *)
        log_error "Invalid build type: $BUILD_TYPE (must be debug or release)"
        exit 1
        ;;
esac

# Create results directory
mkdir -p "$RESULTS_DIR"
timestamp=$(date '+%Y%m%d_%H%M%S')
session_dir="$RESULTS_DIR/session_${timestamp}"
mkdir -p "$session_dir"

# Setup environment
export ATLAS_BUILD_TYPE="$BUILD_TYPE"
export ATLAS_BENCHMARK_ITERATIONS="$NUM_ITERATIONS"
export ATLAS_BENCHMARK_WARMUP="$WARMUP_ITERATIONS"

log_info "Starting Atlas Layer 4 Manifold Benchmarks"
log_info "Build type: $BUILD_TYPE"
log_info "Iterations: $NUM_ITERATIONS (warmup: $WARMUP_ITERATIONS)"
log_info "Results: $session_dir"

# Build benchmarks if requested
if [[ "$BUILD_BENCHMARKS" == "true" ]]; then
    log_info "Building benchmarks..."
    cd "$LAYER4_DIR"
    
    if [[ "$QUIET" == "true" ]]; then
        make benchmarks BUILD_TYPE="$BUILD_TYPE" > /dev/null 2>&1
    else
        make benchmarks BUILD_TYPE="$BUILD_TYPE"
    fi
    
    if [[ $? -eq 0 ]]; then
        log_success "Benchmarks built successfully"
    else
        log_error "Failed to build benchmarks"
        exit 1
    fi
fi

# Check for perf if profiling requested
if [[ "$PROFILE" == "true" ]]; then
    if ! command -v perf &> /dev/null; then
        log_error "perf not found. Install linux-perf for profiling support."
        exit 1
    fi
fi

# Function to run a benchmark suite
run_suite() {
    local suite="$1"
    local suite_dir="$SCRIPT_DIR/$suite"
    local suite_results="$session_dir/$suite"
    
    if [[ ! -d "$suite_dir" ]]; then
        log_warning "Suite directory not found: $suite_dir"
        return 1
    fi
    
    mkdir -p "$suite_results"
    
    log_info "Running $suite benchmarks..."
    
    # Find all benchmark executables in the suite
    local benchmarks=()
    if [[ -f "$suite_dir/Makefile" ]]; then
        # Build suite-specific benchmarks
        cd "$suite_dir"
        make BUILD_TYPE="$BUILD_TYPE" > /dev/null 2>&1
        
        # Find built executables
        while IFS= read -r -d '' file; do
            benchmarks+=("$file")
        done < <(find "$suite_dir" -type f -executable -name "*benchmark*" -print0 2>/dev/null)
    fi
    
    if [[ ${#benchmarks[@]} -eq 0 ]]; then
        log_warning "No benchmarks found in $suite"
        return 1
    fi
    
    # Run each benchmark
    for benchmark in "${benchmarks[@]}"; do
        local bench_name=$(basename "$benchmark")
        local bench_results="$suite_results/${bench_name}.json"
        
        log_info "  Running $bench_name..."
        
        if [[ "$PROFILE" == "true" ]]; then
            # Run with perf profiling
            perf record -g -o "$suite_results/${bench_name}.perf" \
                "$benchmark" --output-format json > "$bench_results" 2>&1
        else
            # Regular benchmark run
            "$benchmark" --output-format json > "$bench_results" 2>&1
        fi
        
        if [[ $? -eq 0 ]]; then
            if [[ "$QUIET" != "true" ]]; then
                log_success "  $bench_name completed"
            fi
        else
            log_error "  $bench_name failed"
        fi
    done
}

# Function to run Rust criterion benchmarks
run_rust_benchmarks() {
    log_info "Running Rust criterion benchmarks..."
    cd "$LAYER4_DIR/rs"
    
    local cargo_args="--release"
    if [[ "$BUILD_TYPE" == "debug" ]]; then
        cargo_args=""
    fi
    
    # Run cargo bench with JSON output
    cargo bench $cargo_args -- --output-format json > "$session_dir/criterion.json" 2>&1
    
    if [[ $? -eq 0 ]]; then
        log_success "Rust benchmarks completed"
    else
        log_error "Rust benchmarks failed"
    fi
}

# Main benchmark execution
for suite in "${SUITES[@]}"; do
    case "$suite" in
        all)
            run_rust_benchmarks
            run_suite "core"
            run_suite "geometric"
            run_suite "signal"
            run_suite "traditional"
            run_suite "applications"
            ;;
        core|geometric|signal|traditional|applications)
            run_suite "$suite"
            ;;
        *)
            log_error "Unknown benchmark suite: $suite"
            ;;
    esac
done

# Generate summary report
log_info "Generating benchmark report..."
"$SCRIPT_DIR/harness/generate_report.py" "$session_dir" > "$session_dir/report.html"

if [[ "$COMPARE" == "true" ]] && [[ -f "$RESULTS_DIR/baseline.json" ]]; then
    log_info "Comparing with baseline..."
    "$SCRIPT_DIR/harness/compare_results.py" \
        "$RESULTS_DIR/baseline.json" \
        "$session_dir" > "$session_dir/comparison.html"
fi

log_success "Benchmarks completed successfully!"
log_info "Results saved to: $session_dir"

if [[ "$PROFILE" == "true" ]]; then
    log_info "Profile data available in *.perf files"
    log_info "Analyze with: perf report -i <file>.perf"
fi

# Print summary if not quiet
if [[ "$QUIET" != "true" ]]; then
    echo ""
    log_info "=== Benchmark Summary ==="
    if [[ -f "$session_dir/report.html" ]]; then
        echo "Full report: $session_dir/report.html"
    fi
    echo "Raw results: $session_dir/"
    echo "Timestamp: $timestamp"
    echo "Build type: $BUILD_TYPE"
    echo "Iterations: $NUM_ITERATIONS"
fi