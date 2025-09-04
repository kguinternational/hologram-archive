#!/usr/bin/env python3
"""
compare_results.py - Atlas Layer 4 Benchmark Comparison Tool
(c) 2024-2025 UOR Foundation - MIT License

Compares benchmark results between different runs to identify performance regressions or improvements.
"""

import sys
import json
import argparse
from pathlib import Path
from typing import Dict, List, Tuple, Any
from benchmark_utils import BenchmarkAnalyzer, StatisticalAnalysis, ReportGenerator


def load_baseline(baseline_file: Path) -> Dict[str, Any]:
    """Load baseline benchmark results."""
    try:
        with open(baseline_file, 'r') as f:
            return json.load(f)
    except (FileNotFoundError, json.JSONDecodeError) as e:
        raise ValueError(f"Failed to load baseline from {baseline_file}: {e}")


def compare_benchmark_results(baseline_dir: Path, current_dir: Path) -> Dict[str, Any]:
    """Compare benchmark results between baseline and current runs."""
    
    # Load baseline and current results
    baseline_suites = BenchmarkAnalyzer.load_results(baseline_dir)
    current_suites = BenchmarkAnalyzer.load_results(current_dir)
    
    if not baseline_suites or not current_suites:
        raise ValueError("Could not load benchmark results from one or both directories")
    
    # Create lookup maps
    baseline_map = {
        f"{suite.name}::{result.name}": result 
        for suite in baseline_suites 
        for result in suite.results
    }
    current_map = {
        f"{suite.name}::{result.name}": result 
        for suite in current_suites 
        for result in suite.results
    }
    
    comparisons = []
    regressions = []
    improvements = []
    new_benchmarks = []
    missing_benchmarks = []
    
    # Compare matching benchmarks
    for key, current_result in current_map.items():
        if key in baseline_map:
            baseline_result = baseline_map[key]
            comparison = compare_single_benchmark(baseline_result, current_result)
            comparisons.append(comparison)
            
            # Classify as regression or improvement
            if comparison["duration_change_pct"] > 20:  # More than 20% slower
                regressions.append(comparison)
            elif comparison["duration_change_pct"] < -10:  # More than 10% faster
                improvements.append(comparison)
        else:
            new_benchmarks.append({
                "name": current_result.name,
                "suite": key.split("::")[0],
                "duration": current_result.duration_ns
            })
    
    # Find missing benchmarks
    for key, baseline_result in baseline_map.items():
        if key not in current_map:
            missing_benchmarks.append({
                "name": baseline_result.name,
                "suite": key.split("::")[0],
                "duration": baseline_result.duration_ns
            })
    
    return {
        "summary": {
            "total_comparisons": len(comparisons),
            "regressions": len(regressions),
            "improvements": len(improvements),
            "new_benchmarks": len(new_benchmarks),
            "missing_benchmarks": len(missing_benchmarks)
        },
        "comparisons": comparisons,
        "regressions": regressions,
        "improvements": improvements,
        "new_benchmarks": new_benchmarks,
        "missing_benchmarks": missing_benchmarks
    }


def compare_single_benchmark(baseline, current) -> Dict[str, Any]:
    """Compare a single benchmark between baseline and current."""
    baseline_duration = baseline.duration_ns / max(baseline.iterations, 1)
    current_duration = current.duration_ns / max(current.iterations, 1)
    
    duration_change = current_duration - baseline_duration
    duration_change_pct = (duration_change / baseline_duration) * 100 if baseline_duration > 0 else 0
    
    comparison = {
        "name": current.name,
        "baseline_duration_ns": baseline_duration,
        "current_duration_ns": current_duration,
        "duration_change_ns": duration_change,
        "duration_change_pct": duration_change_pct,
        "baseline_iterations": baseline.iterations,
        "current_iterations": current.iterations
    }
    
    # Compare throughput if available
    if baseline.throughput_ops_sec and current.throughput_ops_sec:
        throughput_change = current.throughput_ops_sec - baseline.throughput_ops_sec
        throughput_change_pct = (throughput_change / baseline.throughput_ops_sec) * 100
        comparison.update({
            "baseline_throughput": baseline.throughput_ops_sec,
            "current_throughput": current.throughput_ops_sec,
            "throughput_change": throughput_change,
            "throughput_change_pct": throughput_change_pct
        })
    
    # Compare memory usage if available
    if baseline.memory_bytes and current.memory_bytes:
        memory_change = current.memory_bytes - baseline.memory_bytes
        memory_change_pct = (memory_change / baseline.memory_bytes) * 100
        comparison.update({
            "baseline_memory": baseline.memory_bytes,
            "current_memory": current.memory_bytes,
            "memory_change": memory_change,
            "memory_change_pct": memory_change_pct
        })
    
    return comparison


def generate_comparison_html(comparison_data: Dict[str, Any]) -> str:
    """Generate HTML comparison report."""
    html = f"""
    <!DOCTYPE html>
    <html lang="en">
    <head>
        <meta charset="UTF-8">
        <meta name="viewport" content="width=device-width, initial-scale=1.0">
        <title>Atlas Layer 4 Benchmark Comparison</title>
        <style>
            body {{
                font-family: -apple-system, BlinkMacSystemFont, 'Segoe UI', Roboto, sans-serif;
                margin: 0;
                padding: 20px;
                background-color: #f8f9fa;
                line-height: 1.6;
            }}
            .container {{
                max-width: 1200px;
                margin: 0 auto;
                background: white;
                padding: 30px;
                border-radius: 8px;
                box-shadow: 0 2px 10px rgba(0,0,0,0.1);
            }}
            .header {{
                text-align: center;
                margin-bottom: 40px;
                padding-bottom: 20px;
                border-bottom: 2px solid #e9ecef;
            }}
            .summary-grid {{
                display: grid;
                grid-template-columns: repeat(auto-fit, minmax(200px, 1fr));
                gap: 20px;
                margin-bottom: 40px;
            }}
            .summary-card {{
                background: #f8f9fa;
                padding: 20px;
                border-radius: 6px;
                border-left: 4px solid #007bff;
                text-align: center;
            }}
            .summary-card h3 {{
                margin: 0 0 10px 0;
                color: #495057;
            }}
            .summary-card .value {{
                font-size: 2em;
                font-weight: bold;
                color: #007bff;
            }}
            .regression {{ border-left-color: #dc3545; }}
            .regression .value {{ color: #dc3545; }}
            .improvement {{ border-left-color: #28a745; }}
            .improvement .value {{ color: #28a745; }}
            .table-container {{
                margin: 30px 0;
                overflow-x: auto;
            }}
            table {{
                width: 100%;
                border-collapse: collapse;
                background: white;
            }}
            th, td {{
                padding: 12px 15px;
                text-align: left;
                border-bottom: 1px solid #dee2e6;
            }}
            th {{
                background: #f8f9fa;
                font-weight: 600;
                color: #495057;
                position: sticky;
                top: 0;
            }}
            tbody tr:hover {{
                background: #f8f9fa;
            }}
            .change-positive {{ color: #dc3545; font-weight: bold; }}
            .change-negative {{ color: #28a745; font-weight: bold; }}
            .change-neutral {{ color: #6c757d; }}
        </style>
    </head>
    <body>
        <div class="container">
            <div class="header">
                <h1>Atlas Layer 4 Benchmark Comparison</h1>
                <p>Performance comparison between baseline and current benchmark runs</p>
            </div>
            
            <div class="summary-grid">
                <div class="summary-card">
                    <h3>Total Comparisons</h3>
                    <div class="value">{comparison_data['summary']['total_comparisons']}</div>
                </div>
                <div class="summary-card regression">
                    <h3>Regressions</h3>
                    <div class="value">{comparison_data['summary']['regressions']}</div>
                </div>
                <div class="summary-card improvement">
                    <h3>Improvements</h3>
                    <div class="value">{comparison_data['summary']['improvements']}</div>
                </div>
                <div class="summary-card">
                    <h3>New Benchmarks</h3>
                    <div class="value">{comparison_data['summary']['new_benchmarks']}</div>
                </div>
            </div>
    """
    
    # Add regressions table
    if comparison_data['regressions']:
        html += """
            <h2>Performance Regressions (> 20% slower)</h2>
            <div class="table-container">
                <table>
                    <thead>
                        <tr>
                            <th>Benchmark Name</th>
                            <th>Baseline Duration</th>
                            <th>Current Duration</th>
                            <th>Change</th>
                            <th>Change %</th>
                        </tr>
                    </thead>
                    <tbody>
        """
        for reg in comparison_data['regressions']:
            duration_change_class = "change-positive" if reg['duration_change_pct'] > 0 else "change-negative"
            html += f"""
                        <tr>
                            <td>{reg['name']}</td>
                            <td>{ReportGenerator.format_duration(int(reg['baseline_duration_ns']))}</td>
                            <td>{ReportGenerator.format_duration(int(reg['current_duration_ns']))}</td>
                            <td class="{duration_change_class}">
                                {ReportGenerator.format_duration(int(abs(reg['duration_change_ns'])))}
                            </td>
                            <td class="{duration_change_class}">
                                {reg['duration_change_pct']:+.1f}%
                            </td>
                        </tr>
            """
        html += """
                    </tbody>
                </table>
            </div>
        """
    
    # Add improvements table
    if comparison_data['improvements']:
        html += """
            <h2>Performance Improvements (> 10% faster)</h2>
            <div class="table-container">
                <table>
                    <thead>
                        <tr>
                            <th>Benchmark Name</th>
                            <th>Baseline Duration</th>
                            <th>Current Duration</th>
                            <th>Change</th>
                            <th>Change %</th>
                        </tr>
                    </thead>
                    <tbody>
        """
        for imp in comparison_data['improvements']:
            duration_change_class = "change-negative"  # Negative change is good for duration
            html += f"""
                        <tr>
                            <td>{imp['name']}</td>
                            <td>{ReportGenerator.format_duration(int(imp['baseline_duration_ns']))}</td>
                            <td>{ReportGenerator.format_duration(int(imp['current_duration_ns']))}</td>
                            <td class="{duration_change_class}">
                                -{ReportGenerator.format_duration(int(abs(imp['duration_change_ns'])))}
                            </td>
                            <td class="{duration_change_class}">
                                {imp['duration_change_pct']:.1f}%
                            </td>
                        </tr>
            """
        html += """
                    </tbody>
                </table>
            </div>
        """
    
    # Add all comparisons table
    html += """
            <h2>All Benchmark Comparisons</h2>
            <div class="table-container">
                <table>
                    <thead>
                        <tr>
                            <th>Benchmark Name</th>
                            <th>Baseline</th>
                            <th>Current</th>
                            <th>Change %</th>
                            <th>Status</th>
                        </tr>
                    </thead>
                    <tbody>
    """
    
    for comp in comparison_data['comparisons']:
        change_pct = comp['duration_change_pct']
        if change_pct > 20:
            status = "Regression"
            status_class = "change-positive"
        elif change_pct < -10:
            status = "Improvement"
            status_class = "change-negative"
        elif abs(change_pct) < 5:
            status = "Stable"
            status_class = "change-neutral"
        else:
            status = "Minor Change"
            status_class = "change-neutral"
        
        change_class = "change-positive" if change_pct > 0 else "change-negative" if change_pct < -5 else "change-neutral"
        
        html += f"""
                        <tr>
                            <td>{comp['name']}</td>
                            <td>{ReportGenerator.format_duration(int(comp['baseline_duration_ns']))}</td>
                            <td>{ReportGenerator.format_duration(int(comp['current_duration_ns']))}</td>
                            <td class="{change_class}">{change_pct:+.1f}%</td>
                            <td class="{status_class}">{status}</td>
                        </tr>
        """
    
    html += """
                    </tbody>
                </table>
            </div>
        </div>
    </body>
    </html>
    """
    
    return html


def main():
    parser = argparse.ArgumentParser(
        description="Compare Atlas Layer 4 benchmark results"
    )
    parser.add_argument(
        "baseline",
        type=Path,
        help="Baseline benchmark results directory"
    )
    parser.add_argument(
        "current", 
        type=Path,
        help="Current benchmark results directory"
    )
    parser.add_argument(
        "-o", "--output",
        type=Path,
        help="Output HTML file (default: stdout)"
    )
    parser.add_argument(
        "--json",
        action="store_true",
        help="Output JSON instead of HTML"
    )
    
    args = parser.parse_args()
    
    try:
        comparison_data = compare_benchmark_results(args.baseline, args.current)
        
        if args.json:
            output = json.dumps(comparison_data, indent=2)
        else:
            output = generate_comparison_html(comparison_data)
        
        if args.output:
            args.output.write_text(output)
            print(f"Comparison report generated: {args.output}")
        else:
            print(output)
            
        # Print summary to stderr for visibility
        summary = comparison_data['summary']
        print(f"Summary: {summary['total_comparisons']} comparisons, "
              f"{summary['regressions']} regressions, "
              f"{summary['improvements']} improvements", 
              file=sys.stderr)
              
    except Exception as e:
        print(f"Error comparing results: {e}", file=sys.stderr)
        sys.exit(1)


if __name__ == "__main__":
    main()