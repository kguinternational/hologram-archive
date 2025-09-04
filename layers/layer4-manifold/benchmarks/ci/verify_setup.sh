#!/bin/bash
#
# Verification script for Layer 4 CI benchmark setup
#
# This script verifies that all components are properly configured
# and can run together successfully.

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
cd "$SCRIPT_DIR"

RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[0;33m'
BLUE='\033[0;34m'
NC='\033[0m'

log_info() {
    echo -e "${BLUE}[INFO]${NC} $*"
}

log_success() {
    echo -e "${GREEN}[SUCCESS]${NC} $*"
}

log_error() {
    echo -e "${RED}[ERROR]${NC} $*"
}

log_warn() {
    echo -e "${YELLOW}[WARN]${NC} $*"
}

echo "=== Layer 4 CI Benchmark Setup Verification ==="
echo ""

# Check file existence
log_info "Checking required files..."

required_files=(
    "regression_test.sh"
    "benchmark_runner.sh"
    "performance_baselines.json"
    "hardware-profiles.json"
    "README.md"
)

for file in "${required_files[@]}"; do
    if [[ -f "$file" ]]; then
        log_success "✓ $file exists"
    else
        log_error "✗ $file missing"
        exit 1
    fi
done

# Check script permissions
log_info "Checking script permissions..."
for script in regression_test.sh benchmark_runner.sh; do
    if [[ -x "$script" ]]; then
        log_success "✓ $script is executable"
    else
        log_error "✗ $script is not executable"
        exit 1
    fi
done

# Check dependencies
log_info "Checking system dependencies..."

dependencies=(
    "jq JSON processor"
    "python3 Math calculations"
)

for dep in "${dependencies[@]}"; do
    cmd=$(echo "$dep" | cut -d' ' -f1)
    desc=$(echo "$dep" | cut -d' ' -f2-)
    
    if command -v "$cmd" >/dev/null 2>&1; then
        log_success "✓ $cmd available ($desc)"
    else
        log_error "✗ $cmd missing ($desc)"
        exit 1
    fi
done

# Check JSON validity
log_info "Checking JSON file validity..."

json_files=(
    "performance_baselines.json"
    "hardware-profiles.json"
    "test-config.json"
)

for json_file in "${json_files[@]}"; do
    if jq empty "$json_file" 2>/dev/null; then
        log_success "✓ $json_file is valid JSON"
    else
        log_error "✗ $json_file has invalid JSON"
        exit 1
    fi
done

# Test baseline data extraction
log_info "Testing baseline data extraction..."

x86_baseline=$(jq -r '.profiles.x86_64.operations.linear_projection.avg_duration_ns' performance_baselines.json)
if [[ "$x86_baseline" =~ ^[0-9]+$ ]]; then
    log_success "✓ Can extract x86_64 baseline data: ${x86_baseline}ns"
else
    log_error "✗ Failed to extract baseline data"
    exit 1
fi

# Test hardware profile loading
log_info "Testing hardware profile loading..."

profiles=$(jq -r '.profiles | keys[]' hardware-profiles.json)
profile_count=$(echo "$profiles" | wc -l)

if [[ $profile_count -ge 3 ]]; then
    log_success "✓ Found $profile_count hardware profiles:"
    for profile in $profiles; do
        echo "    - $profile"
    done
else
    log_error "✗ Insufficient hardware profiles found"
    exit 1
fi

# Test script help functions
log_info "Testing script help functions..."

if ./regression_test.sh --help >/dev/null 2>&1; then
    log_success "✓ regression_test.sh help works"
else
    log_error "✗ regression_test.sh help failed"
    exit 1
fi

if ./benchmark_runner.sh --help >/dev/null 2>&1; then
    log_success "✓ benchmark_runner.sh help works"
else
    log_error "✗ benchmark_runner.sh help failed"
    exit 1
fi

# Check GitHub Actions workflow
log_info "Checking GitHub Actions workflow..."

workflow_file="../../../../.github/workflows/layer4-benchmarks.yml"
if [[ -f "$workflow_file" ]]; then
    log_success "✓ GitHub Actions workflow exists"
    
    # Check for key sections
    if grep -q "benchmark-x86_64:" "$workflow_file"; then
        log_success "✓ x86_64 benchmark job found"
    else
        log_warn "⚠ x86_64 benchmark job not found in workflow"
    fi
    
    if grep -q "benchmark-arm64:" "$workflow_file"; then
        log_success "✓ ARM64 benchmark job found"
    else
        log_warn "⚠ ARM64 benchmark job not found in workflow"
    fi
    
    if grep -q "benchmark-wasm:" "$workflow_file"; then
        log_success "✓ WASM benchmark job found"
    else
        log_warn "⚠ WASM benchmark job not found in workflow"
    fi
else
    log_error "✗ GitHub Actions workflow missing"
    exit 1
fi

# Check Rust integration
log_info "Checking Rust integration files..."

rust_test_file="../../rs/tests/ci_benchmarks.rs"
if [[ -f "$rust_test_file" ]]; then
    log_success "✓ Rust CI benchmark integration exists"
else
    log_error "✗ Rust CI benchmark integration missing"
    exit 1
fi

# Test math calculations (bc alternative)
log_info "Testing performance calculation methods..."

# Test Python-based calculations
test_result=$(python3 -c "print(round(((60000 - 50000) / 50000) * 100, 2))")
if [[ "$test_result" == "20.0" ]]; then
    log_success "✓ Python-based percentage calculations work"
else
    log_error "✗ Python-based calculations failed"
    exit 1
fi

echo ""
log_success "=== All verification checks passed! ==="
echo ""
log_info "System is ready for Layer 4 performance regression testing"
log_info "Key commands:"
echo "  ./regression_test.sh                 # Run regression test"
echo "  ./benchmark_runner.sh                # Run custom benchmarks"  
echo "  ./regression_test.sh --update-baseline  # Update baselines"
echo ""
log_info "For detailed usage information, run scripts with --help"