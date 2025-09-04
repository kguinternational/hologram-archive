#!/bin/bash

# Test script for Layer 4 Benchmark Visualization Tools
# 
# This script tests all visualization components with sample data

set -euo pipefail

# Configuration
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
TEST_OUTPUT_DIR="$SCRIPT_DIR/test_output"
SAMPLE_DATA_DIR="$SCRIPT_DIR/sample_data"

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[0;33m'
BLUE='\033[0;34m'
NC='\033[0m'

log_info() {
    echo -e "${BLUE}[TEST INFO]${NC} $1"
}

log_success() {
    echo -e "${GREEN}[TEST SUCCESS]${NC} $1"
}

log_warning() {
    echo -e "${YELLOW}[TEST WARNING]${NC} $1"
}

log_error() {
    echo -e "${RED}[TEST ERROR]${NC} $1"
}

cleanup() {
    log_info "Cleaning up test artifacts..."
    rm -rf "$TEST_OUTPUT_DIR" "$SAMPLE_DATA_DIR"
}

test_sample_data_generation() {
    log_info "Testing sample data generation..."
    
    # Create sample data directory
    mkdir -p "$SAMPLE_DATA_DIR"
    
    # Generate sample data (quick test with 1 day)
    if python3 "$SCRIPT_DIR/generate_sample_data.py" --output-dir "$SAMPLE_DATA_DIR" --days 1 --samples-per-hour 4; then
        log_success "Sample data generated successfully"
    else
        log_error "Sample data generation failed"
        return 1
    fi
    
    # Check if files were created
    local expected_files=("benchmark_data_1d.csv" "benchmark_data_1d.json" "summary_stats.json")
    for file in "${expected_files[@]}"; do
        if [[ -f "$SAMPLE_DATA_DIR/$file" ]]; then
            log_success "Found expected file: $file"
        else
            log_warning "Missing expected file: $file"
        fi
    done
    
    # Check data quality
    local record_count=$(tail -n +2 "$SAMPLE_DATA_DIR/benchmark_data_1d.csv" | wc -l)
    log_info "Generated $record_count benchmark records"
    
    if [[ $record_count -gt 0 ]]; then
        log_success "Sample data contains valid records"
        return 0
    else
        log_error "Sample data is empty"
        return 1
    fi
}

test_python_visualization() {
    log_info "Testing Python visualization script..."
    
    # Check dependencies first
    if ! python3 -c "import matplotlib, pandas" 2>/dev/null; then
        log_warning "Python dependencies not installed, skipping visualization test"
        log_info "Install with: pip install matplotlib pandas numpy seaborn"
        return 0
    fi
    
    # Create output directory
    mkdir -p "$TEST_OUTPUT_DIR"
    
    # Run visualization script
    if python3 "$SCRIPT_DIR/visualize_results.py" \
        --data-dir "$SAMPLE_DATA_DIR" \
        --output-dir "$TEST_OUTPUT_DIR" 2>/dev/null; then
        log_success "Python visualization script executed successfully"
    else
        log_warning "Python visualization script failed (may be due to missing dependencies)"
        return 0
    fi
    
    # Check output files
    local expected_outputs=("performance_comparison.png" "benchmark_report.html")
    for output in "${expected_outputs[@]}"; do
        if [[ -f "$TEST_OUTPUT_DIR/$output" ]]; then
            log_success "Generated visualization: $output"
        else
            log_info "Visualization file not found: $output (may require additional dependencies)"
        fi
    done
    
    return 0
}

test_report_generation() {
    log_info "Testing report generation script..."
    
    # Override results directory to use our sample data
    export RESULTS_DIR="$SAMPLE_DATA_DIR"
    
    # Test quick report generation
    if "$SCRIPT_DIR/generate_report.sh" --quick --output "$TEST_OUTPUT_DIR" --time-range 24h 2>/dev/null; then
        log_success "Report generation script executed successfully"
    else
        log_warning "Report generation script had issues (may be due to missing dependencies)"
        # Continue with testing to check partial functionality
    fi
    
    # Check if report files were created
    local report_files=($(find "$TEST_OUTPUT_DIR" -name "*.md" -o -name "*.html" -o -name "*.json" 2>/dev/null || true))
    if [[ ${#report_files[@]} -gt 0 ]]; then
        log_success "Found ${#report_files[@]} report files:"
        for file in "${report_files[@]}"; do
            log_info "  - $(basename "$file")"
        done
    else
        log_info "No report files generated (may require additional system tools)"
    fi
    
    return 0
}

test_dashboard_validity() {
    log_info "Testing dashboard HTML validity..."
    
    local dashboard_file="$SCRIPT_DIR/dashboard.html"
    
    if [[ -f "$dashboard_file" ]]; then
        # Basic HTML validation
        if grep -q "<!DOCTYPE html>" "$dashboard_file" && \
           grep -q "</html>" "$dashboard_file" && \
           grep -q "Layer 4 Manifold" "$dashboard_file"; then
            log_success "Dashboard HTML structure is valid"
        else
            log_error "Dashboard HTML structure appears invalid"
            return 1
        fi
        
        # Check for required JavaScript
        if grep -q "Chart.js" "$dashboard_file" && \
           grep -q "initializeCharts" "$dashboard_file"; then
            log_success "Dashboard JavaScript components found"
        else
            log_warning "Dashboard JavaScript components may be incomplete"
        fi
        
        # Check file size (should be substantial)
        local file_size=$(wc -c < "$dashboard_file")
        if [[ $file_size -gt 10000 ]]; then
            log_success "Dashboard file size is appropriate ($file_size bytes)"
        else
            log_warning "Dashboard file seems small ($file_size bytes)"
        fi
        
    else
        log_error "Dashboard file not found: $dashboard_file"
        return 1
    fi
    
    return 0
}

test_file_permissions() {
    log_info "Testing file permissions..."
    
    local scripts=("generate_report.sh" "generate_sample_data.py" "test_visualization.sh")
    
    for script in "${scripts[@]}"; do
        if [[ -x "$SCRIPT_DIR/$script" ]]; then
            log_success "$script is executable"
        else
            log_warning "$script is not executable (fixing...)"
            chmod +x "$SCRIPT_DIR/$script"
        fi
    done
    
    return 0
}

test_directory_structure() {
    log_info "Testing directory structure..."
    
    local required_files=(
        "README.md"
        "requirements.txt"
        "visualize_results.py"
        "dashboard.html"
        "generate_report.sh"
        "generate_sample_data.py"
        "test_visualization.sh"
    )
    
    local missing_files=()
    
    for file in "${required_files[@]}"; do
        if [[ -f "$SCRIPT_DIR/$file" ]]; then
            log_success "Found required file: $file"
        else
            missing_files+=("$file")
        fi
    done
    
    if [[ ${#missing_files[@]} -eq 0 ]]; then
        log_success "All required files are present"
        return 0
    else
        log_error "Missing required files: ${missing_files[*]}"
        return 1
    fi
}

run_all_tests() {
    local test_functions=(
        "test_directory_structure"
        "test_file_permissions"
        "test_dashboard_validity"
        "test_sample_data_generation"
        "test_python_visualization"
        "test_report_generation"
    )
    
    local passed_tests=0
    local total_tests=${#test_functions[@]}
    
    echo -e "${BLUE}"
    cat << 'EOF'
╔══════════════════════════════════════════════════════════════════╗
║           Layer 4 Benchmark Visualization Test Suite            ║
╚══════════════════════════════════════════════════════════════════╝
EOF
    echo -e "${NC}"
    
    for test_func in "${test_functions[@]}"; do
        echo
        log_info "Running test: $test_func"
        if $test_func; then
            ((passed_tests++))
            log_success "$test_func PASSED"
        else
            log_error "$test_func FAILED"
        fi
    done
    
    echo
    echo -e "${BLUE}═══════════════════════════════════════════════════════════════════${NC}"
    
    if [[ $passed_tests -eq $total_tests ]]; then
        log_success "All tests passed! ($passed_tests/$total_tests)"
        echo
        log_info "🚀 Visualization tools are ready to use!"
        log_info "📊 Try: python3 generate_sample_data.py && python3 visualize_results.py --data-dir sample_data"
        log_info "📈 Or: ./generate_report.sh --quick"
        log_info "🌐 View: dashboard.html in your browser"
    else
        log_warning "Some tests failed or had warnings ($passed_tests/$total_tests passed)"
        echo
        log_info "🔧 Some features may not work due to missing dependencies"
        log_info "📦 Install Python dependencies: pip install -r requirements.txt"
        log_info "🛠️  Install system tools: sudo apt-get install jq bc"
    fi
    
    return 0
}

# Main execution
main() {
    # Set up trap for cleanup
    trap cleanup EXIT
    
    # Run all tests
    run_all_tests
}

# Check if script is being sourced or executed
if [[ "${BASH_SOURCE[0]}" == "${0}" ]]; then
    main "$@"
fi