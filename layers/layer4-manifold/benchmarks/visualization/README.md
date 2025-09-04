# Layer 4 Manifold Benchmark Visualization Tools

This directory contains comprehensive visualization and reporting tools for Layer 4 Manifold benchmark analysis.

## 🚀 Quick Start

### Installation

1. **Install Python Dependencies**:
   ```bash
   pip install -r requirements.txt
   ```

2. **Install System Dependencies** (Ubuntu/Debian):
   ```bash
   sudo apt-get install jq bc
   ```

### Basic Usage

1. **Generate Visualizations**:
   ```bash
   python3 visualize_results.py --data-dir ../results --output-dir ./output
   ```

2. **Create Summary Report**:
   ```bash
   ./generate_report.sh
   ```

3. **View Interactive Dashboard**:
   ```bash
   # Open dashboard.html in your browser
   firefox dashboard.html
   # or
   python3 -m http.server 8000  # then visit http://localhost:8000/dashboard.html
   ```

## 📊 Tools Overview

### 1. Python Visualization Script (`visualize_results.py`)

Comprehensive performance analysis tool that creates:

- **Performance Comparison Charts**: Atlas vs Traditional operations
- **Speedup Analysis**: Performance gains visualization  
- **Memory Usage Comparisons**: Efficiency metrics
- **Conservation Compliance**: Law adherence visualization
- **Interactive HTML Reports**: Plotly-powered dashboards

**Usage Examples**:
```bash
# Basic analysis
python3 visualize_results.py --data-dir ../results --output-dir ./reports

# PNG format with specific output directory
python3 visualize_results.py --data-dir ../results --output-dir ./charts --format png

# Generate only interactive charts
python3 visualize_results.py --data-dir ../results --output-dir ./interactive
```

**Input Formats**:
- CSV files with columns: `operation`, `avg_time_ms`, `ops_per_sec`, `throughput_bps`
- JSON files with benchmark result objects
- Supports mixed file types in the same directory

**Output Files**:
- `performance_comparison.png` - Static performance charts
- `speedup_analysis.png` - Atlas vs Traditional speedup
- `memory_analysis.png` - Memory efficiency visualization
- `conservation_compliance.png` - Conservation law compliance
- `interactive_performance.html` - Interactive Plotly dashboard
- `benchmark_report.html` - Comprehensive HTML report

### 2. Interactive Dashboard (`dashboard.html`)

Real-time benchmark monitoring dashboard featuring:

- **Live Performance Metrics**: Current performance indicators
- **Historical Trends**: Performance evolution over time
- **Conservation Status**: Real-time compliance monitoring
- **Regression Alerts**: Automatic performance degradation detection
- **Responsive Design**: Mobile-friendly interface

**Features**:
- Auto-refresh every 30 seconds
- Customizable time ranges (1h, 24h, 7d, 30d, 90d)
- Operation type filtering
- Export functionality
- Interactive charts with Chart.js

### 3. Report Generator (`generate_report.sh`)

Automated markdown/HTML report generation with:

- **Performance Analysis**: Detailed operation-by-operation breakdown
- **Conservation Compliance**: R96 adherence verification
- **Memory Efficiency**: Usage optimization analysis
- **Historical Trends**: Performance evolution tracking
- **Recommendations**: Actionable optimization suggestions

**Usage Examples**:
```bash
# Default 7-day report
./generate_report.sh

# 24-hour HTML report
./generate_report.sh --time-range 24h --format html

# Verbose 30-day analysis
./generate_report.sh -t 30d --verbose

# Quick report without charts
./generate_report.sh --quick --no-charts

# Custom output directory
./generate_report.sh --output /path/to/reports
```

**Command Line Options**:
```
-t, --time-range TIME   Time range (1h, 24h, 7d, 30d, 90d)
-f, --format FORMAT     Output format (markdown, html, json)
-o, --output DIR        Output directory
-c, --charts            Include charts (default: true)
--no-charts            Disable chart generation
-v, --verbose           Enable verbose logging
--quick                 Generate quick report (faster, less detailed)
```

## 📁 Directory Structure

```
visualization/
├── README.md                    # This documentation
├── requirements.txt             # Python dependencies
├── visualize_results.py         # Main visualization script
├── dashboard.html               # Interactive dashboard
├── generate_report.sh           # Report generator script
├── reports/                     # Generated reports (auto-created)
├── output/                      # Visualization output (auto-created)
└── templates/                   # Report templates (optional)
```

## 🔧 Configuration

### Environment Variables

```bash
export BENCHMARK_TIME_RANGE="7d"           # Default time range
export BENCHMARK_OUTPUT_FORMAT="markdown"   # Default output format
export BENCHMARK_INCLUDE_CHARTS="true"     # Enable charts by default
export BENCHMARK_VERBOSE="false"           # Verbose logging
```

### Python Configuration

The visualization script can be configured by modifying the `BenchmarkVisualizer` class:

```python
# Color schemes
atlas_color = '#1f77b4'         # Blue for Atlas operations
traditional_color = '#ff7f0e'    # Orange for traditional operations  
conservation_color = '#2ca02c'   # Green for conservation compliance
violation_color = '#d62728'      # Red for violations
```

### Dashboard Configuration

Customize the dashboard by modifying JavaScript variables in `dashboard.html`:

```javascript
// Auto-refresh interval (milliseconds)
const REFRESH_INTERVAL = 30000;

// Default time range
const DEFAULT_TIME_RANGE = '7d';

// Chart colors
const CHART_COLORS = {
    atlas: '#3498db',
    traditional: '#e67e22',
    conservation: '#27ae60'
};
```

## 🎯 Benchmark Data Format

### Expected CSV Format

```csv
timestamp,operation,avg_time_ms,ops_per_sec,throughput_bps,iterations,conservation_compliant
2024-01-15T10:30:00Z,Linear Projection (1MB),3.247,2847,145200000,100,true
2024-01-15T10:30:05Z,R96 Fourier Projection (1MB),2.857,3498,132800000,100,true
```

### Expected JSON Format

```json
{
  "timestamp": "2024-01-15T10:30:00Z",
  "operation_name": "Linear Projection (1MB)",
  "avg_duration": 3.247,
  "ops_per_sec": 2847,
  "throughput_bps": 145200000,
  "iterations": 100,
  "conservation_compliant": true
}
```

### Rust Benchmark Integration

The tools automatically detect and parse Rust benchmark output from:

```rust
// From benchmark.rs
let result = BenchmarkResult::new(
    "Linear Projection (1MB)".to_string(),
    100,
    &durations,
    1024 * 1024
);
```

## 📈 Key Metrics Tracked

### Performance Metrics
- **Operations per Second**: Raw computational throughput
- **Average Execution Time**: Per-operation latency
- **Throughput (MB/s)**: Data processing rate
- **Memory Usage**: Peak and average memory consumption

### Atlas-Specific Metrics
- **Conservation Compliance Rate**: R96 law adherence percentage
- **Speedup Factor**: Performance gain vs traditional methods
- **Witness Generation Rate**: Verification certificate creation
- **Harmonic Efficiency**: R96-based optimization effectiveness

### Quality Metrics
- **Performance Stability**: Variance in execution times
- **Scalability**: Performance vs data size relationship
- **Regression Detection**: Performance degradation identification
- **Resource Efficiency**: CPU and memory utilization

## 🔍 Troubleshooting

### Common Issues

1. **No Benchmark Data Found**:
   ```bash
   # Ensure benchmarks have been run
   cd ../../ && make test-layer4
   # or
   cd ../rs && cargo bench
   ```

2. **Python Dependencies Missing**:
   ```bash
   pip install -r requirements.txt
   ```

3. **System Tools Missing**:
   ```bash
   # Ubuntu/Debian
   sudo apt-get install jq bc
   
   # CentOS/RHEL
   sudo yum install jq bc
   ```

4. **Permission Errors**:
   ```bash
   chmod +x generate_report.sh
   ```

5. **Chart Generation Fails**:
   ```bash
   # Check matplotlib backend
   python3 -c "import matplotlib; print(matplotlib.get_backend())"
   
   # Try different backend
   export MPLBACKEND=Agg
   python3 visualize_results.py ...
   ```

### Debug Mode

Enable verbose logging for troubleshooting:

```bash
./generate_report.sh --verbose
python3 visualize_results.py --data-dir ../results --output-dir ./debug 2>&1 | tee debug.log
```

## 🚀 Performance Tips

### For Large Datasets

1. **Use Quick Mode**: `./generate_report.sh --quick`
2. **Disable Charts**: `--no-charts` for faster processing
3. **Filter Time Range**: Use shorter time ranges for faster analysis
4. **Parallel Processing**: Set `NUMBA_NUM_THREADS` for numerical operations

### Memory Optimization

```bash
# Limit memory usage for large datasets
export NUMBA_NUM_THREADS=2
python3 -m memory_profiler visualize_results.py --data-dir ../results
```

### CI/CD Integration

```bash
# Automated report generation
./generate_report.sh --quick --format json > benchmark_results.json
```

## 🤝 Contributing

When adding new visualization features:

1. **Follow Color Scheme**: Use predefined colors for consistency
2. **Add Tests**: Include test data and validation
3. **Update Documentation**: Modify this README for new features
4. **Performance**: Consider memory usage for large datasets

### Adding New Chart Types

1. Extend `BenchmarkVisualizer` class in `visualize_results.py`
2. Add corresponding HTML template sections
3. Update dashboard JavaScript for real-time data
4. Test with sample benchmark data

## 📝 License

This visualization toolkit is part of the Hologram Layer 4 project and follows the same licensing terms.