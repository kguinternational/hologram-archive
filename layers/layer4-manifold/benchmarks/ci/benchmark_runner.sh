#!/bin/bash
#
# Layer 4 Manifold Benchmark Runner
#
# This script provides a unified interface to run Layer 4 benchmarks
# across different hardware profiles and integrate with existing Rust benchmarks.
#
# Usage: ./benchmark_runner.sh [options]
#   --profile NAME          Hardware profile to use (default: auto-detect)
#   --output-format FORMAT  Output format: json, csv, human (default: json)
#   --output-file FILE      Output file path
#   --operations LIST       Comma-separated list of operations to benchmark
#   --data-sizes LIST       Comma-separated list of data sizes in bytes
#   --iterations COUNT      Number of iterations per benchmark
#   --rust-only            Only run Rust benchmarks (skip C integration tests)
#   --verbose              Enable verbose output
#   --help                 Show help message

set -euo pipefail

# Script configuration
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
LAYER4_DIR="$(dirname "$(dirname "$SCRIPT_DIR")")"
RS_DIR="$LAYER4_DIR/rs"

# Default configuration
PROFILE=""
OUTPUT_FORMAT="json"
OUTPUT_FILE=""
OPERATIONS=""
DATA_SIZES=""
ITERATIONS=""
RUST_ONLY=false
VERBOSE=false

# Available operations
AVAILABLE_OPERATIONS=(
    "linear_projection"
    "r96_fourier"
    "shard_extraction"
    "batch_shard_extraction"
    "transformation"
    "reconstruction"
    "invariant_computation"
    "conservation_verification"
)

# Color output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[0;33m'
BLUE='\033[0;34m'
NC='\033[0m'

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

show_help() {
    cat << EOF
Layer 4 Manifold Benchmark Runner

USAGE:
    $(basename "$0") [OPTIONS]

OPTIONS:
    --profile NAME           Hardware profile (x86_64, arm64, wasm32) [default: auto-detect]
    --output-format FORMAT   Output format: json, csv, human [default: json]
    --output-file FILE       Output file path [default: stdout]
    --operations LIST        Operations to benchmark [default: all]
    --data-sizes LIST        Data sizes in bytes [default: profile defaults]
    --iterations COUNT       Iterations per benchmark [default: profile defaults]
    --rust-only             Only run Rust benchmarks, skip C integration
    --verbose               Enable verbose output
    --help                  Show this help message

AVAILABLE OPERATIONS:
$(printf "    %s\n" "${AVAILABLE_OPERATIONS[@]}")

EXAMPLES:
    # Run all benchmarks with auto-detected profile
    ./benchmark_runner.sh

    # Run specific operations on x86_64
    ./benchmark_runner.sh --profile x86_64 --operations "linear_projection,r96_fourier"

    # Save results as CSV
    ./benchmark_runner.sh --output-format csv --output-file results.csv

    # Run with custom data sizes
    ./benchmark_runner.sh --data-sizes "4096,65536,1048576" --iterations 50

EOF
}

parse_args() {
    while [[ $# -gt 0 ]]; do
        case $1 in
            --profile)
                PROFILE="$2"
                shift 2
                ;;
            --output-format)
                OUTPUT_FORMAT="$2"
                shift 2
                ;;
            --output-file)
                OUTPUT_FILE="$2"
                shift 2
                ;;
            --operations)
                OPERATIONS="$2"
                shift 2
                ;;
            --data-sizes)
                DATA_SIZES="$2"
                shift 2
                ;;
            --iterations)
                ITERATIONS="$2"
                shift 2
                ;;
            --rust-only)
                RUST_ONLY=true
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

detect_hardware_profile() {
    if [[ -n "$PROFILE" ]]; then
        return 0
    fi

    local arch
    arch=$(uname -m)
    
    case "$arch" in
        x86_64)
            PROFILE="x86_64"
            ;;
        arm64|aarch64)
            PROFILE="arm64"
            ;;
        *)
            log_warn "Unknown architecture: $arch, defaulting to x86_64"
            PROFILE="x86_64"
            ;;
    esac
    
    verbose_log "Auto-detected hardware profile: $PROFILE"
}

load_profile_config() {
    local profile_file="$SCRIPT_DIR/hardware-profiles.json"
    
    if [[ ! -f "$profile_file" ]]; then
        log_error "Hardware profiles file not found: $profile_file"
        exit 1
    fi

    if ! command -v jq >/dev/null 2>&1; then
        log_error "jq is required for profile configuration parsing"
        exit 1
    fi

    # Validate profile exists
    if ! jq -e ".profiles.\"$PROFILE\"" "$profile_file" >/dev/null; then
        log_error "Hardware profile not found: $PROFILE"
        log_error "Available profiles: $(jq -r '.profiles | keys | join(", ")' "$profile_file")"
        exit 1
    fi

    # Extract profile configuration
    PROFILE_DATA=$(jq ".profiles.\"$PROFILE\"" "$profile_file")
    
    # Set defaults from profile if not specified
    if [[ -z "$DATA_SIZES" ]]; then
        DATA_SIZES=$(echo "$PROFILE_DATA" | jq -r '.benchmark_config.data_sizes | join(",")')
    fi
    
    if [[ -z "$ITERATIONS" ]]; then
        ITERATIONS=$(echo "$PROFILE_DATA" | jq -r '.benchmark_config.iterations')
    fi
    
    verbose_log "Loaded configuration for profile: $PROFILE"
    verbose_log "Data sizes: $DATA_SIZES"
    verbose_log "Iterations: $ITERATIONS"
}

validate_operations() {
    if [[ -z "$OPERATIONS" ]]; then
        OPERATIONS=$(IFS=,; echo "${AVAILABLE_OPERATIONS[*]}")
        return 0
    fi

    IFS=',' read -ra OPS_ARRAY <<< "$OPERATIONS"
    for op in "${OPS_ARRAY[@]}"; do
        if [[ ! " ${AVAILABLE_OPERATIONS[*]} " =~ " $op " ]]; then
            log_error "Invalid operation: $op"
            log_error "Available operations: $(IFS=,; echo "${AVAILABLE_OPERATIONS[*]}")"
            exit 1
        fi
    done
}

get_cargo_command() {
    if command -v cargo >/dev/null 2>&1; then
        echo "cargo"
    elif test -f /home/codespace/.cargo/bin/cargo; then
        echo "/home/codespace/.cargo/bin/cargo"
    else
        log_error "Cargo not found"
        exit 1
    fi
}

build_benchmark_binary() {
    log_info "Building Layer 4 benchmark binary with profile optimizations..."
    cd "$RS_DIR"
    
    local cargo_cmd
    cargo_cmd=$(get_cargo_command)
    
    # Extract compiler flags from profile
    local rust_flags=""
    if [[ -n "$PROFILE_DATA" ]]; then
        rust_flags=$(echo "$PROFILE_DATA" | jq -r '.compiler_flags.rust | join(" ")' 2>/dev/null || echo "")
    fi
    
    # Set RUSTFLAGS if profile specifies them
    if [[ -n "$rust_flags" && "$rust_flags" != "null" ]]; then
        export RUSTFLAGS="$rust_flags"
        verbose_log "Using Rust compiler flags: $rust_flags"
    fi
    
    # Build with appropriate target and features
    local build_cmd="$cargo_cmd build --release --features benchmarks"
    
    # Add target if specified for cross-compilation
    case "$PROFILE" in
        wasm32)
            build_cmd="$build_cmd --target wasm32-unknown-unknown --no-default-features"
            ;;
        arm64)
            if [[ "$(uname -m)" != "arm64" && "$(uname -m)" != "aarch64" ]]; then
                # Cross-compilation for ARM64
                build_cmd="$build_cmd --target aarch64-unknown-linux-gnu"
                export CARGO_TARGET_AARCH64_UNKNOWN_LINUX_GNU_LINKER=aarch64-linux-gnu-gcc
            fi
            ;;
    esac
    
    if ! $build_cmd 2>/dev/null; then
        log_error "Failed to build benchmark binary"
        exit 1
    fi
    
    verbose_log "Successfully built benchmark binary"
}

create_benchmark_config() {
    local config_file
    config_file=$(mktemp)
    
    cat > "$config_file" << EOF
{
  "profile": "$PROFILE",
  "operations": [$(echo "$OPERATIONS" | sed 's/,/", "/g' | sed 's/^/"/; s/$/"/')],"
  "data_sizes": [$(echo "$DATA_SIZES" | tr ',' '\n' | while read -r size; do echo -n "$size,"; done | sed 's/,$//')],"
  "iterations": $ITERATIONS,
  "output_format": "$OUTPUT_FORMAT"
}
EOF
    
    echo "$config_file"
}

run_rust_benchmarks() {
    log_info "Running Rust benchmarks for operations: $OPERATIONS"
    cd "$RS_DIR"
    
    local cargo_cmd
    cargo_cmd=$(get_cargo_command)
    
    local results_file
    results_file=$(mktemp)
    
    # Create benchmark configuration
    local config_file
    config_file=$(create_benchmark_config)
    
    # Run benchmarks with configuration
    local bench_output
    bench_output=$(mktemp)
    
    if [[ "$PROFILE" == "wasm32" ]]; then
        # WASM benchmarks require special handling
        run_wasm_benchmarks "$config_file" "$results_file"
    else
        # Native benchmarks
        BENCHMARK_CONFIG="$config_file" $cargo_cmd test --release --features benchmarks benchmark -- --nocapture > "$bench_output" 2>&1
        
        # Parse benchmark output and create structured results
        parse_rust_benchmark_output "$bench_output" "$config_file" "$results_file"
    fi
    
    # Clean up temporary files
    rm -f "$config_file" "$bench_output"
    
    echo "$results_file"
}

run_wasm_benchmarks() {
    local config_file="$1"
    local results_file="$2"
    
    verbose_log "Running WASM benchmarks (limited functionality)"
    
    # For WASM, we need to use a different approach
    # This is a simplified version - in practice you'd compile to WASM and run with wasmtime
    
    # Create minimal results structure for WASM
    local timestamp
    timestamp=$(date -u +"%Y-%m-%dT%H:%M:%SZ")
    
    cat > "$results_file" << EOF
{
  "timestamp": "$timestamp",
  "hardware_profile": "wasm32",
  "operations": {},
  "note": "WASM benchmarks require special runtime setup"
}
EOF
}

parse_rust_benchmark_output() {
    local output_file="$1"
    local config_file="$2"
    local results_file="$3"
    
    verbose_log "Parsing Rust benchmark output..."
    
    local timestamp
    timestamp=$(date -u +"%Y-%m-%dT%H:%M:%SZ")
    
    # Create results structure
    cat > "$results_file" << EOF
{
  "timestamp": "$timestamp",
  "hardware_profile": "$PROFILE",
  "operations": {}
}
EOF
    
    # Parse each operation from the benchmark output
    IFS=',' read -ra OPS_ARRAY <<< "$OPERATIONS"
    for op in "${OPS_ARRAY[@]}"; do
        # Extract timing information for this operation
        # This is a simplified parser - adapt based on actual benchmark output format
        local avg_time
        avg_time=$(grep -i "$op" "$output_file" | grep -oP '\d+\.?\d*\s*(ns|μs|ms|s)' | head -1 | grep -oP '\d+\.?\d*' || echo "0")
        
        # Convert to nanoseconds (simplified)
        local avg_time_ns
        if grep -i "$op" "$output_file" | grep -q "ms"; then
            avg_time_ns=$(echo "$avg_time * 1000000" | bc 2>/dev/null || echo "0")
        elif grep -i "$op" "$output_file" | grep -q "μs"; then
            avg_time_ns=$(echo "$avg_time * 1000" | bc 2>/dev/null || echo "0")
        else
            avg_time_ns="$avg_time"
        fi
        
        avg_time_ns=${avg_time_ns:-0}
        
        # Calculate ops per second
        local ops_per_sec
        if [[ "$avg_time_ns" -gt 0 ]]; then
            ops_per_sec=$(echo "scale=2; 1000000000 / $avg_time_ns" | bc 2>/dev/null || echo "0")
        else
            ops_per_sec=0
        fi
        
        # Update results JSON
        local updated_results
        updated_results=$(jq --arg op "$op" --argjson avg_ns "$avg_time_ns" --argjson ops_sec "$ops_per_sec" \
            '.operations[$op] = {"avg_duration_ns": $avg_ns, "ops_per_sec": $ops_sec}' "$results_file")
        echo "$updated_results" > "$results_file"
    done
}

run_integration_tests() {
    if [[ "$RUST_ONLY" == "true" ]]; then
        verbose_log "Skipping integration tests (--rust-only flag)"
        return 0
    fi
    
    log_info "Running C integration tests..."
    cd "$LAYER4_DIR"
    
    # Run Layer 4 tests to ensure integration works
    if ! make test >/dev/null 2>&1; then
        log_warn "Some integration tests failed, but continuing with benchmarks"
    else
        verbose_log "Integration tests passed"
    fi
}

format_output() {
    local results_file="$1"
    
    case "$OUTPUT_FORMAT" in
        json)
            cat "$results_file"
            ;;
        csv)
            format_csv_output "$results_file"
            ;;
        human)
            format_human_output "$results_file"
            ;;
        *)
            log_error "Unknown output format: $OUTPUT_FORMAT"
            exit 1
            ;;
    esac
}

format_csv_output() {
    local results_file="$1"
    
    echo "operation,avg_duration_ns,ops_per_sec,hardware_profile,timestamp"
    
    local operations
    operations=$(jq -r '.operations | keys[]' "$results_file")
    
    local profile timestamp
    profile=$(jq -r '.hardware_profile' "$results_file")
    timestamp=$(jq -r '.timestamp' "$results_file")
    
    for op in $operations; do
        local avg_ns ops_sec
        avg_ns=$(jq -r ".operations.\"$op\".avg_duration_ns" "$results_file")
        ops_sec=$(jq -r ".operations.\"$op\".ops_per_sec" "$results_file")
        echo "$op,$avg_ns,$ops_sec,$profile,$timestamp"
    done
}

format_human_output() {
    local results_file="$1"
    
    echo "=== Layer 4 Manifold Benchmark Results ==="
    echo ""
    echo "Hardware Profile: $(jq -r '.hardware_profile' "$results_file")"
    echo "Timestamp: $(jq -r '.timestamp' "$results_file")"
    echo ""
    echo "Results:"
    echo "----------------------------------------"
    printf "%-25s %15s %15s\n" "Operation" "Avg Time" "Ops/Second"
    echo "----------------------------------------"
    
    local operations
    operations=$(jq -r '.operations | keys[]' "$results_file")
    
    for op in $operations; do
        local avg_ns ops_sec
        avg_ns=$(jq -r ".operations.\"$op\".avg_duration_ns" "$results_file")
        ops_sec=$(jq -r ".operations.\"$op\".ops_per_sec" "$results_file")
        
        # Format duration
        local formatted_time
        if [[ "$avg_ns" -gt 1000000 ]]; then
            formatted_time="$(echo "scale=2; $avg_ns / 1000000" | bc)ms"
        elif [[ "$avg_ns" -gt 1000 ]]; then
            formatted_time="$(echo "scale=2; $avg_ns / 1000" | bc)μs"
        else
            formatted_time="${avg_ns}ns"
        fi
        
        # Format operation name
        local formatted_op
        formatted_op=$(echo "$op" | tr '_' ' ' | sed 's/\b\w/\u&/g')
        
        printf "%-25s %15s %15.2f\n" "$formatted_op" "$formatted_time" "$ops_sec"
    done
    echo ""
}

main() {
    parse_args "$@"
    
    log_info "Starting Layer 4 Manifold benchmark runner"
    
    detect_hardware_profile
    load_profile_config
    validate_operations
    
    # Build benchmark binary
    build_benchmark_binary
    
    # Run integration tests if requested
    run_integration_tests
    
    # Run benchmarks
    local results_file
    results_file=$(run_rust_benchmarks)
    
    # Output results
    if [[ -n "$OUTPUT_FILE" ]]; then
        format_output "$results_file" > "$OUTPUT_FILE"
        log_success "Results saved to: $OUTPUT_FILE"
    else
        format_output "$results_file"
    fi
    
    # Clean up
    rm -f "$results_file"
    
    log_success "Benchmark run completed successfully"
}

# Run main function with all arguments
main "$@"