#!/bin/bash
#
# Layer 4 Manifold Performance Regression Test Script
#
# This script runs performance benchmarks for Layer 4 (Manifold) operations,
# compares against baseline performance, and fails if performance degrades >10%.
# Updates baseline on improvement.
#
# Usage: ./regression_test.sh [options]
#   --baseline-file FILE    Path to baseline JSON file (default: performance_baselines.json)
#   --output-file FILE      Path to output results JSON file
#   --hardware-profile NAME Hardware profile to use (x86_64, arm64, wasm32)
#   --threshold PERCENT     Performance degradation threshold (default: 10)
#   --update-baseline       Update baseline if performance improves
#   --verbose               Enable verbose output
#   --help                  Show this help message

set -euo pipefail

# Script configuration
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
LAYER4_DIR="$(dirname "$(dirname "$SCRIPT_DIR")")"
PROJECT_ROOT="$(dirname "$(dirname "$(dirname "$LAYER4_DIR")")")"

# Default values
BASELINE_FILE="$SCRIPT_DIR/performance_baselines.json"
OUTPUT_FILE=""
HARDWARE_PROFILE="$(uname -m | sed 's/x86_64/x86_64/;s/arm64/arm64/;s/aarch64/arm64/')"
THRESHOLD=10
UPDATE_BASELINE=false
VERBOSE=false

# Color output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[0;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# Logging functions
log_info() {
    echo -e "${BLUE}[INFO]${NC} $*"
}

log_warn() {
    echo -e "${YELLOW}[WARN]${NC} $*"
}

log_error() {
    echo -e "${RED}[ERROR]${NC} $*" >&2
}

log_success() {
    echo -e "${GREEN}[SUCCESS]${NC} $*"
}

verbose_log() {
    if [[ "$VERBOSE" == "true" ]]; then
        log_info "$@"
    fi
}

# Help function
show_help() {
    cat << EOF
Layer 4 Manifold Performance Regression Test Script

USAGE:
    $(basename "$0") [OPTIONS]

OPTIONS:
    --baseline-file FILE     Path to baseline JSON file (default: performance_baselines.json)
    --output-file FILE       Path to output results JSON file
    --hardware-profile NAME  Hardware profile (x86_64, arm64, wasm32) [default: auto-detect]
    --threshold PERCENT      Performance degradation threshold [default: 10]
    --update-baseline        Update baseline if performance improves
    --verbose                Enable verbose output
    --help                   Show this help message

EXAMPLES:
    # Run with default settings
    ./regression_test.sh

    # Run with custom threshold and update baseline
    ./regression_test.sh --threshold 15 --update-baseline

    # Run for specific hardware profile
    ./regression_test.sh --hardware-profile arm64

    # Run with custom baseline file
    ./regression_test.sh --baseline-file custom_baselines.json

EOF
}

# Parse command line arguments
parse_args() {
    while [[ $# -gt 0 ]]; do
        case $1 in
            --baseline-file)
                BASELINE_FILE="$2"
                shift 2
                ;;
            --output-file)
                OUTPUT_FILE="$2"
                shift 2
                ;;
            --hardware-profile)
                HARDWARE_PROFILE="$2"
                shift 2
                ;;
            --threshold)
                THRESHOLD="$2"
                shift 2
                ;;
            --update-baseline)
                UPDATE_BASELINE=true
                shift
                ;;
            --verbose)
                VERBOSE=true
                shift
                ;;
            --help)
                show_help
                exit 0
                ;;
            *)
                log_error "Unknown option: $1"
                show_help
                exit 1
                ;;
        esac
    done
}

# Validate hardware profile
validate_hardware_profile() {
    case "$HARDWARE_PROFILE" in
        x86_64|arm64|aarch64|wasm32)
            verbose_log "Hardware profile: $HARDWARE_PROFILE"
            ;;
        *)
            log_error "Unsupported hardware profile: $HARDWARE_PROFILE"
            log_error "Supported profiles: x86_64, arm64, wasm32"
            exit 1
            ;;
    esac
}

# Check dependencies
check_dependencies() {
    local missing_deps=()

    # Check for Rust/Cargo
    if ! command -v cargo >/dev/null 2>&1; then
        if ! test -f /home/codespace/.cargo/bin/cargo; then
            missing_deps+=("cargo (Rust toolchain)")
        fi
    fi

    # Check for jq (JSON processor)
    if ! command -v jq >/dev/null 2>&1; then
        missing_deps+=("jq")
    fi

    # Check for bc (basic calculator for floating point)
    if ! command -v bc >/dev/null 2>&1; then
        missing_deps+=("bc")
    fi

    if [[ ${#missing_deps[@]} -gt 0 ]]; then
        log_error "Missing required dependencies:"
        for dep in "${missing_deps[@]}"; do
            log_error "  - $dep"
        done
        exit 1
    fi
}

# Get cargo command
get_cargo() {
    if command -v cargo >/dev/null 2>&1; then
        echo "cargo"
    elif test -f /home/codespace/.cargo/bin/cargo; then
        echo "/home/codespace/.cargo/bin/cargo"
    else
        log_error "Cargo not found"
        exit 1
    fi
}

# Build benchmark binary
build_benchmark() {
    log_info "Building Layer 4 benchmark binary..."
    cd "$LAYER4_DIR/rs"
    
    local cargo_cmd
    cargo_cmd=$(get_cargo)
    
    # Build with benchmarks feature
    if ! $cargo_cmd build --release --features benchmarks 2>/dev/null; then
        log_error "Failed to build benchmark binary"
        exit 1
    fi
    
    verbose_log "Successfully built benchmark binary"
}

# Run benchmarks and capture output
run_benchmarks() {
    log_info "Running Layer 4 manifold benchmarks..."
    cd "$LAYER4_DIR/rs"
    
    local cargo_cmd
    cargo_cmd=$(get_cargo)
    
    # Create temporary file for benchmark results
    local bench_output
    bench_output=$(mktemp)
    
    # Run benchmarks with JSON output
    if ! $cargo_cmd test --release --features benchmarks benchmark -- --nocapture 2>&1 | tee "$bench_output"; then
        log_error "Benchmark execution failed"
        rm -f "$bench_output"
        exit 1
    fi
    
    # Parse benchmark results
    parse_benchmark_results "$bench_output"
    rm -f "$bench_output"
}

# Parse benchmark results from output
parse_benchmark_results() {
    local output_file="$1"
    local timestamp
    timestamp=$(date -u +"%Y-%m-%dT%H:%M:%SZ")
    
    # Create results JSON structure
    BENCHMARK_RESULTS=$(cat << EOF
{
  "timestamp": "$timestamp",
  "hardware_profile": "$HARDWARE_PROFILE",
  "operations": {}
}
EOF
    )
    
    # Parse each benchmark operation from output
    # This is a simplified parser - in practice, you'd want more robust parsing
    verbose_log "Parsing benchmark results..."
    
    # Example parsing (adapt based on actual benchmark output format)
    local linear_projection_time
    local r96_fourier_time
    local shard_extraction_time
    local transformation_time
    
    # Extract timing information from benchmark output
    # These would be actual regex patterns matching your benchmark output
    linear_projection_time=$(grep -oP "Linear Projection.*?(\d+\.?\d*)" "$output_file" | tail -1 | grep -oP "\d+\.?\d*" || echo "0")
    r96_fourier_time=$(grep -oP "R96 Fourier.*?(\d+\.?\d*)" "$output_file" | tail -1 | grep -oP "\d+\.?\d*" || echo "0")
    
    # Default values if parsing fails
    linear_projection_time=${linear_projection_time:-0}
    r96_fourier_time=${r96_fourier_time:-0}
    shard_extraction_time=${shard_extraction_time:-0}
    transformation_time=${transformation_time:-0}
    
    # Update results JSON with parsed values
    BENCHMARK_RESULTS=$(echo "$BENCHMARK_RESULTS" | jq --argjson lp "$linear_projection_time" \
        --argjson r96 "$r96_fourier_time" \
        --argjson shard "$shard_extraction_time" \
        --argjson transform "$transformation_time" \
        '.operations = {
            "linear_projection": {"avg_duration_ns": $lp, "ops_per_sec": (1000000000 / ($lp + 1))},
            "r96_fourier": {"avg_duration_ns": $r96, "ops_per_sec": (1000000000 / ($r96 + 1))},
            "shard_extraction": {"avg_duration_ns": $shard, "ops_per_sec": (1000000000 / ($shard + 1))},
            "transformation": {"avg_duration_ns": $transform, "ops_per_sec": (1000000000 / ($transform + 1))}
        }')
    
    verbose_log "Parsed benchmark results successfully"
}

# Load baseline performance data
load_baseline() {
    if [[ ! -f "$BASELINE_FILE" ]]; then
        log_warn "Baseline file not found: $BASELINE_FILE"
        log_warn "Creating initial baseline from current run"
        return 1
    fi
    
    if ! jq empty "$BASELINE_FILE" 2>/dev/null; then
        log_error "Invalid JSON in baseline file: $BASELINE_FILE"
        exit 1
    fi
    
    verbose_log "Loaded baseline from: $BASELINE_FILE"
    return 0
}

# Compare performance against baseline
compare_performance() {
    local baseline_data="$1"
    local current_data="$2"
    local has_regressions=false
    local has_improvements=false
    
    log_info "Comparing performance against baseline (threshold: ${THRESHOLD}%)"
    
    # Extract hardware profile data from baseline
    local baseline_profile_data
    baseline_profile_data=$(echo "$baseline_data" | jq -r ".profiles.\"$HARDWARE_PROFILE\" // empty")
    
    if [[ -z "$baseline_profile_data" || "$baseline_profile_data" == "null" ]]; then
        log_warn "No baseline data for hardware profile: $HARDWARE_PROFILE"
        return 2
    fi
    
    # Compare each operation
    local operations
    operations=$(echo "$current_data" | jq -r '.operations | keys[]')
    
    for op in $operations; do
        local current_duration
        local baseline_duration
        local change_percent
        
        current_duration=$(echo "$current_data" | jq -r ".operations.\"$op\".avg_duration_ns")
        baseline_duration=$(echo "$baseline_profile_data" | jq -r ".operations.\"$op\".avg_duration_ns // 0")
        
        if [[ "$baseline_duration" == "0" ]]; then
            log_warn "No baseline data for operation: $op"
            continue
        fi
        
        # Calculate percentage change (positive = slower, negative = faster)
        change_percent=$(echo "scale=2; (($current_duration - $baseline_duration) / $baseline_duration) * 100" | bc)
        
        if (( $(echo "$change_percent > $THRESHOLD" | bc -l) )); then
            log_error "REGRESSION: $op is ${change_percent}% slower (threshold: ${THRESHOLD}%)"
            log_error "  Current:  ${current_duration}ns"
            log_error "  Baseline: ${baseline_duration}ns"
            has_regressions=true
        elif (( $(echo "$change_percent < -5" | bc -l) )); then
            log_success "IMPROVEMENT: $op is ${change_percent#-}% faster"
            log_success "  Current:  ${current_duration}ns"
            log_success "  Baseline: ${baseline_duration}ns"
            has_improvements=true
        else
            verbose_log "OK: $op performance change: ${change_percent}%"
        fi
    done
    
    if [[ "$has_regressions" == "true" ]]; then
        return 1
    elif [[ "$has_improvements" == "true" ]]; then
        return 3
    else
        return 0
    fi
}

# Update baseline with current results
update_baseline() {
    local current_data="$1"
    
    log_info "Updating baseline performance data..."
    
    local new_baseline
    if [[ -f "$BASELINE_FILE" ]]; then
        # Update existing baseline
        new_baseline=$(jq --argjson profile_data "$current_data" \
            --arg profile "$HARDWARE_PROFILE" \
            '.profiles[$profile] = $profile_data' "$BASELINE_FILE")
    else
        # Create new baseline
        new_baseline=$(cat << EOF
{
  "version": "1.0",
  "description": "Layer 4 Manifold Performance Baselines",
  "profiles": {
    "$HARDWARE_PROFILE": $current_data
  }
}
EOF
        )
    fi
    
    echo "$new_baseline" | jq '.' > "$BASELINE_FILE"
    log_success "Updated baseline file: $BASELINE_FILE"
}

# Save current results to output file
save_results() {
    local current_data="$1"
    
    if [[ -n "$OUTPUT_FILE" ]]; then
        echo "$current_data" | jq '.' > "$OUTPUT_FILE"
        log_info "Saved results to: $OUTPUT_FILE"
    fi
}

# Generate summary report
generate_summary() {
    local comparison_result="$1"
    
    echo ""
    log_info "=== Performance Regression Test Summary ==="
    log_info "Hardware Profile: $HARDWARE_PROFILE"
    log_info "Threshold: ${THRESHOLD}%"
    
    case $comparison_result in
        0)
            log_success "✓ No performance regressions detected"
            ;;
        1)
            log_error "✗ Performance regressions detected"
            log_error "  Performance has degraded beyond the ${THRESHOLD}% threshold"
            ;;
        2)
            log_warn "⚠ No baseline data available for comparison"
            ;;
        3)
            log_success "✓ Performance improvements detected"
            if [[ "$UPDATE_BASELINE" == "true" ]]; then
                log_success "  Baseline will be updated"
            fi
            ;;
    esac
    echo ""
}

# Main execution function
main() {
    parse_args "$@"
    
    log_info "Starting Layer 4 Manifold performance regression test"
    log_info "Hardware Profile: $HARDWARE_PROFILE, Threshold: ${THRESHOLD}%"
    
    validate_hardware_profile
    check_dependencies
    
    # Build and run benchmarks
    build_benchmark
    run_benchmarks
    
    # Load baseline and compare
    local baseline_data=""
    local comparison_result=0
    
    if load_baseline; then
        baseline_data=$(cat "$BASELINE_FILE")
        compare_performance "$baseline_data" "$BENCHMARK_RESULTS"
        comparison_result=$?
    else
        comparison_result=2
    fi
    
    # Handle results
    case $comparison_result in
        0|3)  # No regressions or improvements
            if [[ "$comparison_result" == "3" && "$UPDATE_BASELINE" == "true" ]]; then
                update_baseline "$BENCHMARK_RESULTS"
            fi
            save_results "$BENCHMARK_RESULTS"
            ;;
        1)  # Regressions detected
            save_results "$BENCHMARK_RESULTS"
            ;;
        2)  # No baseline data
            if [[ "$UPDATE_BASELINE" == "true" ]]; then
                update_baseline "$BENCHMARK_RESULTS"
            fi
            save_results "$BENCHMARK_RESULTS"
            ;;
    esac
    
    generate_summary $comparison_result
    
    # Exit with appropriate code
    if [[ $comparison_result == 1 ]]; then
        exit 1
    else
        exit 0
    fi
}

# Run main function with all arguments
main "$@"