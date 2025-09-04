#!/usr/bin/env bash
#
# run_all_benchmarks.sh - Comprehensive Layer 4 Benchmark Runner
# (c) 2024-2025 UOR Foundation - MIT License
#
# Comprehensive benchmark runner for Atlas Layer 4 Manifold operations.
# Builds, runs, and reports on all benchmark categories with graceful error handling.

set -euo pipefail

# Configuration and paths
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
LAYER4_DIR="$(dirname "$SCRIPT_DIR")"
RESULTS_DIR="$SCRIPT_DIR/results"
BUILD_DIR="$SCRIPT_DIR/build"
timestamp=$(date '+%Y%m%d_%H%M%S')

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
CYAN='\033[0;36m'
MAGENTA='\033[0;35m'
BOLD='\033[1m'
NC='\033[0m' # No Color

# Progress tracking
declare -A CATEGORY_STATUS
declare -A BENCHMARK_RESULTS
declare -a FAILED_BENCHMARKS
declare -a SKIPPED_BENCHMARKS
TOTAL_BENCHMARKS=0
SUCCESSFUL_BENCHMARKS=0
FAILED_COUNT=0
SKIPPED_COUNT=0

# Logging functions with enhanced formatting
print_header() {
    echo -e "\n${BOLD}${BLUE}================================================================${NC}"
    echo -e "${BOLD}${BLUE} $1${NC}"
    echo -e "${BOLD}${BLUE}================================================================${NC}\n"
}

print_section() {
    echo -e "\n${BOLD}${CYAN}--- $1 ---${NC}"
}

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

log_progress() {
    local current=$1
    local total=$2
    local name=$3
    local percent=$((current * 100 / total))
    echo -e "${CYAN}[PROGRESS ${percent}%]${NC} (${current}/${total}) Running ${name}..."
}

# Print script usage
print_usage() {
    cat << EOF
${BOLD}Atlas Layer 4 Comprehensive Benchmark Runner${NC}

${BOLD}Usage:${NC} $0 [OPTIONS]

${BOLD}DESCRIPTION:${NC}
    Builds and runs all Layer 4 benchmark categories, saving results to organized
    subdirectories and generating a comprehensive summary report.

${BOLD}OPTIONS:${NC}
    -h, --help          Show this help message
    -q, --quiet         Quiet mode (suppress detailed output)
    -v, --verbose       Verbose mode (show detailed build output)
    --no-build          Skip building benchmarks (use existing binaries)
    --build-only        Build benchmarks but don't run them
    --clean-first       Clean all builds before starting
    --save-logs         Save detailed logs for debugging
    --timeout SECONDS   Set timeout for individual benchmarks (default: 300)

${BOLD}BENCHMARK CATEGORIES:${NC}
    standalone          Basic Layer 4 operations and conservation tests
    core               Projection and shard benchmarks  
    geometric          Transform and manifold benchmarks
    signal             Resonance and frequency domain benchmarks
    applications       Computational workload benchmarks
    traditional        Algorithm comparison benchmarks

${BOLD}OUTPUT:${NC}
    Results are saved to: ${RESULTS_DIR}/session_${timestamp}/
    - Individual category subdirectories
    - Comprehensive summary report
    - Performance highlights and analysis
    - Build and execution logs

${BOLD}EXAMPLES:${NC}
    $0                  Run all benchmarks with default settings
    $0 --quiet          Run quietly with minimal output
    $0 --verbose        Run with detailed build and execution logs
    $0 --no-build       Use existing binaries without rebuilding
    $0 --clean-first    Clean build and run fresh benchmarks

EOF
}

# Parse command line arguments
QUIET=false
VERBOSE=false
BUILD_BENCHMARKS=true
RUN_BENCHMARKS=true
CLEAN_FIRST=false
SAVE_LOGS=false
BENCHMARK_TIMEOUT=300

while [[ $# -gt 0 ]]; do
    case $1 in
        -h|--help)
            print_usage
            exit 0
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
        --build-only)
            RUN_BENCHMARKS=false
            shift
            ;;
        --clean-first)
            CLEAN_FIRST=true
            shift
            ;;
        --save-logs)
            SAVE_LOGS=true
            shift
            ;;
        --timeout)
            BENCHMARK_TIMEOUT="$2"
            shift 2
            ;;
        *)
            log_error "Unknown option: $1"
            print_usage
            exit 1
            ;;
    esac
done

# Create results directory structure
setup_results_dir() {
    local session_dir="$RESULTS_DIR/session_${timestamp}"
    
    mkdir -p "$session_dir"/{standalone,core,geometric,signal,applications,traditional,logs}
    
    if [[ "$SAVE_LOGS" == "true" ]]; then
        mkdir -p "$session_dir/logs"/{build,execution}
    fi
    
    echo "$session_dir"
}

# Build all benchmarks with proper error handling
build_benchmarks() {
    print_section "Building Layer 4 Benchmarks"
    
    cd "$SCRIPT_DIR"
    
    if [[ "$CLEAN_FIRST" == "true" ]]; then
        log_info "Cleaning existing builds..."
        if [[ "$VERBOSE" == "true" ]]; then
            make clean
        else
            make clean >/dev/null 2>&1
        fi
    fi
    
    local build_log=""
    if [[ "$SAVE_LOGS" == "true" ]]; then
        build_log="$SESSION_DIR/logs/build/build_${timestamp}.log"
    fi
    
    log_info "Building all benchmark categories..."
    
    # Build categories in dependency order
    local categories=("core" "geometric" "signal" "applications" "traditional" "standalone")
    local build_success=true
    
    # First try to build all at once (often more reliable)
    log_info "Attempting to build all benchmarks at once..."
    local global_result=0
    if [[ "$VERBOSE" == "true" ]]; then
        make all || global_result=$?
    elif [[ "$QUIET" == "true" ]]; then
        make all >/dev/null 2>&1 || global_result=$?
    else
        make all 2>&1 || global_result=$?
    fi
    
    if [[ $global_result -eq 0 ]]; then
        log_success "All benchmarks built successfully"
        for category in "${categories[@]}"; do
            CATEGORY_STATUS["$category"]="built"
        done
        return 0
    else
        log_warning "Global build failed, trying individual categories..."
    fi
    
    # Try individual categories if global build failed
    for category in "${categories[@]}"; do
        log_info "Building $category benchmarks..."
        
        local cmd_output=""
        local result=0
        
        if [[ "$VERBOSE" == "true" ]]; then
            if [[ -n "$build_log" ]]; then
                make "$category" 2>&1 | tee -a "$build_log" || result=$?
            else
                make "$category" || result=$?
            fi
        elif [[ "$QUIET" == "true" ]]; then
            if [[ -n "$build_log" ]]; then
                make "$category" >>"$build_log" 2>&1 || result=$?
            else
                make "$category" >/dev/null 2>&1 || result=$?
            fi
        else
            if [[ -n "$build_log" ]]; then
                cmd_output=$(make "$category" 2>&1 | tee -a "$build_log") || result=$?
            else
                cmd_output=$(make "$category" 2>&1) || result=$?
            fi
        fi
        
        if [[ $result -eq 0 ]]; then
            log_success "$category benchmarks built successfully"
            CATEGORY_STATUS["$category"]="built"
        else
            log_error "$category benchmarks failed to build"
            CATEGORY_STATUS["$category"]="build_failed"
            build_success=false
            
            if [[ "$QUIET" != "true" && -n "$cmd_output" ]]; then
                echo -e "${RED}Last few lines of build output:${NC}"
                echo "$cmd_output" | tail -5
            fi
        fi
    done
    
    if [[ "$build_success" == "true" ]]; then
        log_success "All benchmarks built successfully"
        return 0
    else
        log_warning "Some benchmarks failed to build but will continue with available ones"
        return 1
    fi
}

# Run a single benchmark with timeout and error handling
run_single_benchmark() {
    local benchmark_path="$1"
    local output_file="$2"
    local benchmark_name=$(basename "$benchmark_path")
    
    if [[ ! -x "$benchmark_path" ]]; then
        log_warning "Benchmark not executable: $benchmark_name"
        SKIPPED_BENCHMARKS+=("$benchmark_name (not executable)")
        return 1
    fi
    
    log_info "  Running $benchmark_name..."
    
    local start_time=$(date +%s.%N)
    local temp_output=$(mktemp)
    local temp_error=$(mktemp)
    
    # Run benchmark with timeout
    if timeout "$BENCHMARK_TIMEOUT" "$benchmark_path" >"$temp_output" 2>"$temp_error"; then
        local end_time=$(date +%s.%N)
        local duration=$(echo "$end_time - $start_time" | bc -l 2>/dev/null || echo "0")
        
        # Save results
        {
            echo "# Benchmark: $benchmark_name"
            echo "# Timestamp: $(date -Iseconds)"
            echo "# Duration: ${duration}s"
            echo "# Status: SUCCESS"
            echo ""
            cat "$temp_output"
        } > "$output_file"
        
        # Save errors if any (but don't fail the benchmark)
        if [[ -s "$temp_error" ]]; then
            echo -e "\n# Errors/Warnings:" >> "$output_file"
            cat "$temp_error" >> "$output_file"
        fi
        
        log_success "  $benchmark_name completed (${duration}s)"
        BENCHMARK_RESULTS["$benchmark_name"]="SUCCESS:${duration}"
        ((SUCCESSFUL_BENCHMARKS++))
        
        # Extract performance highlights
        extract_performance_data "$benchmark_name" "$temp_output"
        
    else
        local exit_code=$?
        local end_time=$(date +%s.%N)
        local duration=$(echo "$end_time - $start_time" | bc -l 2>/dev/null || echo "0")
        
        {
            echo "# Benchmark: $benchmark_name"
            echo "# Timestamp: $(date -Iseconds)"
            echo "# Duration: ${duration}s"
            echo "# Status: FAILED (exit code: $exit_code)"
            echo ""
            echo "# STDOUT:"
            cat "$temp_output"
            echo -e "\n# STDERR:"
            cat "$temp_error"
        } > "$output_file"
        
        if [[ $exit_code -eq 124 ]]; then
            log_error "  $benchmark_name TIMEOUT after ${BENCHMARK_TIMEOUT}s"
            BENCHMARK_RESULTS["$benchmark_name"]="TIMEOUT:${BENCHMARK_TIMEOUT}"
        else
            log_error "  $benchmark_name FAILED (exit code: $exit_code)"
            BENCHMARK_RESULTS["$benchmark_name"]="FAILED:$exit_code"
        fi
        
        FAILED_BENCHMARKS+=("$benchmark_name")
        ((FAILED_COUNT++))
    fi
    
    rm -f "$temp_output" "$temp_error"
    return 0
}

# Extract performance metrics for highlights
extract_performance_data() {
    local benchmark_name="$1"
    local output_file="$2"
    
    # Look for common performance patterns
    local throughput=$(grep -i "throughput\|ops/sec\|operations/sec" "$output_file" | head -1 || true)
    local latency=$(grep -i "latency\|time\|duration" "$output_file" | grep -v "total" | head -1 || true)
    local memory=$(grep -i "memory\|mem\|bytes" "$output_file" | head -1 || true)
    
    if [[ -n "$throughput" ]]; then
        PERFORMANCE_HIGHLIGHTS["$benchmark_name:throughput"]="$throughput"
    fi
    if [[ -n "$latency" ]]; then
        PERFORMANCE_HIGHLIGHTS["$benchmark_name:latency"]="$latency"
    fi
    if [[ -n "$memory" ]]; then
        PERFORMANCE_HIGHLIGHTS["$benchmark_name:memory"]="$memory"
    fi
}

# Run benchmarks in a specific category
run_benchmark_category() {
    local category="$1"
    local category_dir="$SESSION_DIR/$category"
    local current_count=0
    
    print_section "Running $category benchmarks"
    
    # Check if category was built successfully
    if [[ "${CATEGORY_STATUS[$category]:-}" == "build_failed" ]]; then
        log_warning "Skipping $category - build failed"
        CATEGORY_STATUS["$category"]="skipped_build_failed"
        return 1
    fi
    
    # Find benchmark executables for this category
    local benchmarks=()
    
    # Look in the main build directory first
    local main_build_benchmarks=()
    while IFS= read -r -d '' file; do
        local basename_file=$(basename "$file")
        # Match category-specific benchmarks
        case "$category" in
            standalone)
                if [[ "$basename_file" =~ (run_layer4_benchmarks|test_conservation) ]]; then
                    main_build_benchmarks+=("$file")
                fi
                ;;
            core)
                if [[ "$basename_file" =~ (projection_benchmarks|shard_benchmarks) ]]; then
                    main_build_benchmarks+=("$file")
                fi
                ;;
            geometric)
                if [[ "$basename_file" =~ (transform_benchmarks|manifold_benchmarks|distance_bench|curvature_bench) ]]; then
                    main_build_benchmarks+=("$file")
                fi
                ;;
            signal)
                if [[ "$basename_file" =~ (resonance_benchmarks|fourier_bench) ]]; then
                    main_build_benchmarks+=("$file")
                fi
                ;;
            applications)
                if [[ "$basename_file" =~ (computational_benchmarks|ml_.*bench|db_.*bench|crypto_bench) ]]; then
                    main_build_benchmarks+=("$file")
                fi
                ;;
            traditional)
                if [[ "$basename_file" =~ (algorithm_benchmarks|traditional_.*) ]]; then
                    main_build_benchmarks+=("$file")
                fi
                ;;
        esac
    done < <(find "$BUILD_DIR" -type f -executable 2>/dev/null | sort | tr '\n' '\0')
    
    # Add main build benchmarks
    benchmarks+=("${main_build_benchmarks[@]}")
    
    # Also look in category-specific build directories
    local category_build_dir="${category}/build"
    if [[ -d "$category_build_dir" ]]; then
        while IFS= read -r -d '' file; do
            benchmarks+=("$file")
        done < <(find "$category_build_dir" -type f -executable 2>/dev/null | sort | tr '\n' '\0')
    fi
    
    # Look for any other executables in the category directory
    while IFS= read -r -d '' file; do
        # Only add if it looks like a benchmark and is not a source file directory
        if [[ "$file" =~ benchmark && ! "$file" =~ \.(c|h|o)$ ]]; then
            benchmarks+=("$file")
        fi
    done < <(find "$category"/ -type f -executable 2>/dev/null | sort | tr '\n' '\0')
    
    # Remove duplicates while preserving order
    local unique_benchmarks=()
    local seen_benchmarks=()
    for bench in "${benchmarks[@]}"; do
        local bench_name=$(basename "$bench")
        if [[ ! " ${seen_benchmarks[*]} " =~ " ${bench_name} " ]]; then
            unique_benchmarks+=("$bench")
            seen_benchmarks+=("$bench_name")
        fi
    done
    benchmarks=("${unique_benchmarks[@]}")
    
    if [[ ${#benchmarks[@]} -eq 0 ]]; then
        log_warning "No executable benchmarks found for $category"
        CATEGORY_STATUS["$category"]="no_benchmarks"
        return 1
    fi
    
    log_info "Found ${#benchmarks[@]} benchmarks in $category"
    TOTAL_BENCHMARKS=$((TOTAL_BENCHMARKS + ${#benchmarks[@]}))
    
    # Run each benchmark
    for benchmark in "${benchmarks[@]}"; do
        ((current_count++))
        local benchmark_name=$(basename "$benchmark")
        local output_file="$category_dir/${benchmark_name}_${timestamp}.txt"
        
        if [[ "$QUIET" != "true" ]]; then
            log_progress $((SUCCESSFUL_BENCHMARKS + FAILED_COUNT + current_count)) $TOTAL_BENCHMARKS "$benchmark_name"
        fi
        
        run_single_benchmark "$benchmark" "$output_file"
    done
    
    CATEGORY_STATUS["$category"]="completed"
    log_success "$category benchmarks completed"
    
    return 0
}

# Generate comprehensive summary report
generate_summary_report() {
    local report_file="$SESSION_DIR/BENCHMARK_SUMMARY.txt"
    local html_report="$SESSION_DIR/benchmark_report.html"
    
    print_section "Generating Summary Report"
    
    {
        echo "================================================================"
        echo "Atlas Layer 4 Manifold Benchmark Summary"
        echo "================================================================"
        echo "Timestamp: $(date -Iseconds)"
        echo "Session: $timestamp"
        echo "Timeout: ${BENCHMARK_TIMEOUT}s per benchmark"
        echo ""
        
        echo "OVERALL RESULTS:"
        echo "  Total benchmarks: $TOTAL_BENCHMARKS"
        echo "  Successful: $SUCCESSFUL_BENCHMARKS"
        echo "  Failed: $FAILED_COUNT"
        local skipped_count=0
        if [[ -n "${SKIPPED_BENCHMARKS:-}" ]]; then
            skipped_count=${#SKIPPED_BENCHMARKS[@]}
        fi
        echo "  Skipped: $skipped_count"
        echo ""
        
        if [[ $TOTAL_BENCHMARKS -gt 0 ]]; then
            local success_rate=$((SUCCESSFUL_BENCHMARKS * 100 / TOTAL_BENCHMARKS))
            echo "  Success rate: ${success_rate}%"
        fi
        echo ""
        
        echo "CATEGORY STATUS:"
        for category in standalone core geometric signal applications traditional; do
            local status="${CATEGORY_STATUS[$category]:-not_run}"
            echo "  $category: $status"
        done
        echo ""
        
        local failed_count_check=0
        if [[ -n "${FAILED_BENCHMARKS:-}" ]]; then
            failed_count_check=${#FAILED_BENCHMARKS[@]}
        fi
        
        if [[ $failed_count_check -gt 0 ]]; then
            echo "FAILED BENCHMARKS:"
            for failed in "${FAILED_BENCHMARKS[@]}"; do
                echo "  - $failed (${BENCHMARK_RESULTS[$failed]:-unknown})"
            done
            echo ""
        fi
        
        if [[ $skipped_count -gt 0 ]]; then
            echo "SKIPPED BENCHMARKS:"
            for skipped in "${SKIPPED_BENCHMARKS[@]}"; do
                echo "  - $skipped"
            done
            echo ""
        fi
        
        echo "PERFORMANCE HIGHLIGHTS:"
        local perf_count=0
        if [[ -n "${PERFORMANCE_HIGHLIGHTS:-}" ]]; then
            perf_count=${#PERFORMANCE_HIGHLIGHTS[@]}
        fi
        
        if [[ $perf_count -gt 0 ]]; then
            for key in "${!PERFORMANCE_HIGHLIGHTS[@]}"; do
                echo "  $key: ${PERFORMANCE_HIGHLIGHTS[$key]}"
            done
        else
            echo "  No performance data extracted"
        fi
        echo ""
        
        echo "RESULTS LOCATION:"
        echo "  Session directory: $SESSION_DIR"
        echo "  Individual results in category subdirectories"
        if [[ "$SAVE_LOGS" == "true" ]]; then
            echo "  Detailed logs in: $SESSION_DIR/logs/"
        fi
        echo ""
        
        echo "================================================================"
        
    } > "$report_file"
    
    # Also display summary to console (unless quiet)
    if [[ "$QUIET" != "true" ]]; then
        cat "$report_file"
    fi
    
    log_success "Summary report saved to: $report_file"
}

# Main execution flow
main() {
    print_header "Atlas Layer 4 Manifold Benchmark Runner"
    
    # Initialize performance highlights array
    declare -gA PERFORMANCE_HIGHLIGHTS
    
    # Setup
    SESSION_DIR=$(setup_results_dir)
    log_info "Results will be saved to: $SESSION_DIR"
    
    if [[ "$BUILD_BENCHMARKS" == "true" ]]; then
        build_benchmarks || true  # Continue even if some builds fail
    fi
    
    if [[ "$RUN_BENCHMARKS" == "true" ]]; then
        # Run benchmark categories in order
        cd "$SCRIPT_DIR"
        
        local categories=("standalone" "core" "geometric" "signal" "applications" "traditional")
        
        for category in "${categories[@]}"; do
            run_benchmark_category "$category" || true  # Continue even if category fails
        done
        
        # Generate comprehensive report
        generate_summary_report
        
        print_header "Benchmark Run Complete"
        
        if [[ $SUCCESSFUL_BENCHMARKS -eq $TOTAL_BENCHMARKS ]] && [[ $TOTAL_BENCHMARKS -gt 0 ]]; then
            log_success "All $TOTAL_BENCHMARKS benchmarks completed successfully!"
        elif [[ $SUCCESSFUL_BENCHMARKS -gt 0 ]]; then
            log_warning "$SUCCESSFUL_BENCHMARKS/$TOTAL_BENCHMARKS benchmarks completed successfully"
        else
            log_error "No benchmarks completed successfully"
            exit 1
        fi
        
        echo -e "\n${BOLD}Results saved to:${NC} $SESSION_DIR"
        echo -e "${BOLD}Summary report:${NC} $SESSION_DIR/BENCHMARK_SUMMARY.txt"
        
    else
        log_info "Skipping benchmark execution (--build-only specified)"
    fi
}

# Handle script interruption gracefully
trap 'echo -e "\n${RED}Benchmark run interrupted${NC}"; exit 130' INT TERM

# Run main function
main "$@"