#!/usr/bin/env python3
"""
generate_report.py - Atlas Layer 4 Benchmark Report Generator
(c) 2024-2025 UOR Foundation - MIT License

Generates comprehensive HTML reports from benchmark results.
"""

import sys
import json
import argparse
from pathlib import Path
from typing import List, Dict, Any
from jinja2 import Template
from benchmark_utils import BenchmarkAnalyzer, ReportGenerator, StatisticalAnalysis, PerformanceClassifier


HTML_TEMPLATE = """
<!DOCTYPE html>
<html lang="en">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>Atlas Layer 4 Benchmark Report</title>
    <style>
        body {
            font-family: -apple-system, BlinkMacSystemFont, 'Segoe UI', Roboto, sans-serif;
            margin: 0;
            padding: 20px;
            background-color: #f8f9fa;
            line-height: 1.6;
        }
        .container {
            max-width: 1200px;
            margin: 0 auto;
            background: white;
            padding: 30px;
            border-radius: 8px;
            box-shadow: 0 2px 10px rgba(0,0,0,0.1);
        }
        .header {
            text-align: center;
            margin-bottom: 40px;
            padding-bottom: 20px;
            border-bottom: 2px solid #e9ecef;
        }
        .header h1 {
            color: #2c3e50;
            margin: 0 0 10px 0;
            font-size: 2.5em;
        }
        .header .subtitle {
            color: #6c757d;
            font-size: 1.2em;
        }
        .summary-grid {
            display: grid;
            grid-template-columns: repeat(auto-fit, minmax(250px, 1fr));
            gap: 20px;
            margin-bottom: 40px;
        }
        .summary-card {
            background: #f8f9fa;
            padding: 20px;
            border-radius: 6px;
            border-left: 4px solid #007bff;
        }
        .summary-card h3 {
            margin: 0 0 10px 0;
            color: #495057;
        }
        .summary-card .value {
            font-size: 2em;
            font-weight: bold;
            color: #007bff;
        }
        .suite {
            margin-bottom: 40px;
            border: 1px solid #dee2e6;
            border-radius: 6px;
            overflow: hidden;
        }
        .suite-header {
            background: #007bff;
            color: white;
            padding: 15px 20px;
            font-size: 1.3em;
            font-weight: bold;
        }
        .suite-content {
            padding: 20px;
        }
        .benchmark-table {
            width: 100%;
            border-collapse: collapse;
            margin-bottom: 20px;
        }
        .benchmark-table th,
        .benchmark-table td {
            padding: 10px 12px;
            text-align: left;
            border-bottom: 1px solid #dee2e6;
        }
        .benchmark-table th {
            background: #f8f9fa;
            font-weight: 600;
            color: #495057;
        }
        .benchmark-table tbody tr:hover {
            background: #f8f9fa;
        }
        .performance-excellent { color: #28a745; font-weight: bold; }
        .performance-good { color: #17a2b8; font-weight: bold; }
        .performance-acceptable { color: #ffc107; font-weight: bold; }
        .performance-poor { color: #dc3545; font-weight: bold; }
        .chart-container {
            margin: 20px 0;
            height: 400px;
            background: #f8f9fa;
            border: 1px dashed #dee2e6;
            display: flex;
            align-items: center;
            justify-content: center;
            color: #6c757d;
        }
        .metadata {
            background: #e9ecef;
            padding: 15px;
            border-radius: 4px;
            font-family: monospace;
            font-size: 0.9em;
            margin-top: 20px;
        }
        .performance-distribution {
            display: grid;
            grid-template-columns: repeat(4, 1fr);
            gap: 15px;
            margin: 20px 0;
        }
        .perf-card {
            text-align: center;
            padding: 15px;
            border-radius: 6px;
            border: 2px solid;
        }
        .perf-excellent { background: #d4edda; border-color: #28a745; }
        .perf-good { background: #d1ecf1; border-color: #17a2b8; }
        .perf-acceptable { background: #fff3cd; border-color: #ffc107; }
        .perf-poor { background: #f8d7da; border-color: #dc3545; }
        .footnote {
            margin-top: 40px;
            padding-top: 20px;
            border-top: 1px solid #dee2e6;
            color: #6c757d;
            font-size: 0.9em;
        }
    </style>
</head>
<body>
    <div class="container">
        <div class="header">
            <h1>Atlas Layer 4 Benchmark Report</h1>
            <div class="subtitle">Manifold Operations Performance Analysis</div>
            <div style="margin-top: 10px; color: #6c757d;">
                Generated: {{ summary.timestamp }}<br>
                Build Type: {{ build_type }} | Total Suites: {{ summary.total_suites }} | Total Benchmarks: {{ summary.total_benchmarks }}
            </div>
        </div>

        <div class="summary-grid">
            <div class="summary-card">
                <h3>Total Benchmarks</h3>
                <div class="value">{{ summary.total_benchmarks }}</div>
            </div>
            <div class="summary-card">
                <h3>Test Suites</h3>
                <div class="value">{{ summary.total_suites }}</div>
            </div>
            <div class="summary-card">
                <h3>Excellent Performance</h3>
                <div class="value performance-excellent">{{ summary.performance_breakdown.excellent }}</div>
            </div>
            <div class="summary-card">
                <h3>Poor Performance</h3>
                <div class="value performance-poor">{{ summary.performance_breakdown.poor }}</div>
            </div>
        </div>

        <div class="performance-distribution">
            <div class="perf-card perf-excellent">
                <div style="font-size: 2em; font-weight: bold;">{{ summary.performance_breakdown.excellent }}</div>
                <div>Excellent</div>
            </div>
            <div class="perf-card perf-good">
                <div style="font-size: 2em; font-weight: bold;">{{ summary.performance_breakdown.good }}</div>
                <div>Good</div>
            </div>
            <div class="perf-card perf-acceptable">
                <div style="font-size: 2em; font-weight: bold;">{{ summary.performance_breakdown.acceptable }}</div>
                <div>Acceptable</div>
            </div>
            <div class="perf-card perf-poor">
                <div style="font-size: 2em; font-weight: bold;">{{ summary.performance_breakdown.poor }}</div>
                <div>Poor</div>
            </div>
        </div>

        {% for suite in suites %}
        <div class="suite">
            <div class="suite-header">
                {{ suite.name|title }} Benchmarks ({{ suite.results|length }} tests)
            </div>
            <div class="suite-content">
                <table class="benchmark-table">
                    <thead>
                        <tr>
                            <th>Benchmark Name</th>
                            <th>Duration</th>
                            <th>Iterations</th>
                            <th>Performance</th>
                            <th>Throughput</th>
                            <th>Memory</th>
                        </tr>
                    </thead>
                    <tbody>
                        {% for result in suite.results %}
                        <tr>
                            <td>{{ result.name }}</td>
                            <td>{{ format_duration(result.duration_ns) }}</td>
                            <td>{{ result.iterations }}</td>
                            <td class="performance-{{ classify_performance(result, suite.name) }}">
                                {{ classify_performance(result, suite.name)|title }}
                            </td>
                            <td>
                                {% if result.throughput_ops_sec %}
                                    {{ format_throughput(result.throughput_ops_sec) }}
                                {% else %}
                                    N/A
                                {% endif %}
                            </td>
                            <td>
                                {% if result.memory_bytes %}
                                    {{ format_memory(result.memory_bytes) }}
                                {% else %}
                                    N/A
                                {% endif %}
                            </td>
                        </tr>
                        {% endfor %}
                    </tbody>
                </table>

                {% if suite.name == 'core' %}
                <div class="chart-container">
                    Universal Number Operations Performance Chart
                    <br><small>(Chart implementation requires JavaScript/Canvas)</small>
                </div>
                {% endif %}
            </div>
        </div>
        {% endfor %}

        <div class="metadata">
            <strong>System Information:</strong><br>
            {% for key, value in platform_info.items() %}
            {{ key }}: {{ value }}<br>
            {% endfor %}
        </div>

        <div class="footnote">
            <strong>Performance Classifications:</strong><br>
            • <span class="performance-excellent">Excellent</span>: Optimal performance, meeting or exceeding target specifications<br>
            • <span class="performance-good">Good</span>: Above-average performance, suitable for production use<br>
            • <span class="performance-acceptable">Acceptable</span>: Adequate performance, may need optimization<br>
            • <span class="performance-poor">Poor</span>: Below-target performance, optimization required<br><br>
            
            <strong>Universal Number Operations:</strong> Layer 4 implements complex mathematical operations as Universal Numbers (UN) - 
            scalar invariants that preserve conservation laws and support witnessable computation. Performance measurements reflect 
            the efficiency of these theoretical optimizations over traditional approaches.
        </div>
    </div>
</body>
</html>
"""


def format_memory(bytes_val: int) -> str:
    """Format memory usage in human-readable form."""
    if bytes_val < 1024:
        return f"{bytes_val}B"
    elif bytes_val < 1024 * 1024:
        return f"{bytes_val / 1024:.1f}KB"
    elif bytes_val < 1024 * 1024 * 1024:
        return f"{bytes_val / (1024 * 1024):.1f}MB"
    else:
        return f"{bytes_val / (1024 * 1024 * 1024):.1f}GB"


def classify_performance_for_template(result, suite_name: str) -> str:
    """Classify performance for template rendering."""
    # Map suite names to categories
    category_map = {
        "core": "un_operations",
        "geometric": "geometric", 
        "signal": "signal",
        "traditional": "geometric",  # Use geometric thresholds for traditional
        "applications": "geometric"  # Use geometric thresholds for applications
    }
    
    category = category_map.get(suite_name, "geometric")
    return PerformanceClassifier.classify_performance(result, category)


def generate_html_report(results_dir: Path, output_file: Path = None) -> str:
    """Generate HTML benchmark report."""
    print(f"Loading benchmark results from {results_dir}...")
    
    # Load benchmark results
    suites = BenchmarkAnalyzer.load_results(results_dir)
    if not suites:
        raise ValueError(f"No benchmark results found in {results_dir}")
    
    print(f"Found {len(suites)} benchmark suites")
    
    # Generate summary
    summary = ReportGenerator.generate_summary(suites)
    
    # Get platform info (use first suite's info)
    platform_info = suites[0].platform_info if suites else {}
    build_type = suites[0].build_type if suites else "unknown"
    
    # Prepare template context
    template_context = {
        "suites": suites,
        "summary": summary,
        "platform_info": platform_info,
        "build_type": build_type,
        "format_duration": ReportGenerator.format_duration,
        "format_throughput": ReportGenerator.format_throughput,
        "format_memory": format_memory,
        "classify_performance": classify_performance_for_template
    }
    
    # Render template
    template = Template(HTML_TEMPLATE)
    html_content = template.render(**template_context)
    
    # Write output
    if output_file:
        output_file.write_text(html_content)
        print(f"Report generated: {output_file}")
    
    return html_content


def main():
    parser = argparse.ArgumentParser(
        description="Generate Atlas Layer 4 benchmark report"
    )
    parser.add_argument(
        "results_dir",
        type=Path,
        help="Directory containing benchmark results"
    )
    parser.add_argument(
        "-o", "--output",
        type=Path,
        help="Output HTML file (default: stdout)"
    )
    
    args = parser.parse_args()
    
    try:
        html_content = generate_html_report(args.results_dir, args.output)
        
        if not args.output:
            print(html_content)
            
    except Exception as e:
        print(f"Error generating report: {e}", file=sys.stderr)
        sys.exit(1)


if __name__ == "__main__":
    main()