#!/usr/bin/env python3
"""
benchmark_utils.py - Atlas Layer 4 Benchmark Utilities
(c) 2024-2025 UOR Foundation - MIT License

Common utilities for benchmark data processing, analysis, and reporting.
"""

import json
import statistics
import time
from dataclasses import dataclass
from typing import Dict, List, Optional, Tuple, Any
from pathlib import Path
import numpy as np


@dataclass
class BenchmarkResult:
    """Represents a single benchmark measurement."""
    name: str
    duration_ns: int
    iterations: int
    throughput_ops_sec: Optional[float] = None
    memory_bytes: Optional[int] = None
    cpu_cycles: Optional[int] = None
    cache_misses: Optional[int] = None
    metadata: Optional[Dict[str, Any]] = None


@dataclass
class BenchmarkSuite:
    """Collection of benchmark results for a suite."""
    name: str
    results: List[BenchmarkResult]
    timestamp: str
    build_type: str
    platform_info: Dict[str, str]


class BenchmarkAnalyzer:
    """Analyzes and processes benchmark results."""
    
    @staticmethod
    def load_results(results_dir: Path) -> List[BenchmarkSuite]:
        """Load all benchmark results from a directory."""
        suites = []
        
        for suite_dir in results_dir.iterdir():
            if not suite_dir.is_dir():
                continue
                
            suite = BenchmarkAnalyzer._load_suite(suite_dir)
            if suite:
                suites.append(suite)
                
        return suites
    
    @staticmethod
    def _load_suite(suite_dir: Path) -> Optional[BenchmarkSuite]:
        """Load a single benchmark suite."""
        results = []
        
        # Load JSON result files
        for result_file in suite_dir.glob("*.json"):
            try:
                with open(result_file, 'r') as f:
                    data = json.load(f)
                    result = BenchmarkAnalyzer._parse_result(data)
                    if result:
                        results.append(result)
            except (json.JSONDecodeError, KeyError, ValueError) as e:
                print(f"Warning: Failed to parse {result_file}: {e}")
                continue
        
        if not results:
            return None
            
        return BenchmarkSuite(
            name=suite_dir.name,
            results=results,
            timestamp=time.strftime("%Y-%m-%d %H:%M:%S"),
            build_type="release",  # Default, could be parsed from metadata
            platform_info=BenchmarkAnalyzer._get_platform_info()
        )
    
    @staticmethod
    def _parse_result(data: Dict[str, Any]) -> Optional[BenchmarkResult]:
        """Parse benchmark result from JSON data."""
        try:
            # Handle different JSON formats (criterion, custom, etc.)
            if "criterion" in data:
                return BenchmarkAnalyzer._parse_criterion_result(data)
            elif "benchmark_name" in data:
                return BenchmarkAnalyzer._parse_custom_result(data)
            else:
                return None
        except (KeyError, ValueError, TypeError):
            return None
    
    @staticmethod
    def _parse_criterion_result(data: Dict[str, Any]) -> BenchmarkResult:
        """Parse Rust criterion benchmark result."""
        criterion_data = data["criterion"]
        
        return BenchmarkResult(
            name=criterion_data["benchmark_name"],
            duration_ns=int(criterion_data["mean"]["estimate"]),
            iterations=criterion_data["iterations"],
            throughput_ops_sec=criterion_data.get("throughput"),
            metadata={
                "std_dev": criterion_data["std_dev"]["estimate"],
                "confidence_interval": criterion_data["mean"]["confidence_interval"]
            }
        )
    
    @staticmethod
    def _parse_custom_result(data: Dict[str, Any]) -> BenchmarkResult:
        """Parse custom benchmark result format."""
        return BenchmarkResult(
            name=data["benchmark_name"],
            duration_ns=data["duration_ns"],
            iterations=data.get("iterations", 1),
            throughput_ops_sec=data.get("throughput_ops_sec"),
            memory_bytes=data.get("memory_bytes"),
            cpu_cycles=data.get("cpu_cycles"),
            cache_misses=data.get("cache_misses"),
            metadata=data.get("metadata", {})
        )
    
    @staticmethod
    def _get_platform_info() -> Dict[str, str]:
        """Get platform information for benchmark context."""
        import platform
        import subprocess
        
        info = {
            "os": platform.system(),
            "arch": platform.machine(),
            "python": platform.python_version(),
            "processor": platform.processor()
        }
        
        # Try to get CPU info on Linux
        try:
            with open("/proc/cpuinfo", "r") as f:
                for line in f:
                    if line.startswith("model name"):
                        info["cpu"] = line.split(":", 1)[1].strip()
                        break
        except (FileNotFoundError, PermissionError):
            pass
            
        # Try to get memory info on Linux
        try:
            with open("/proc/meminfo", "r") as f:
                for line in f:
                    if line.startswith("MemTotal"):
                        info["memory"] = line.split(":", 1)[1].strip()
                        break
        except (FileNotFoundError, PermissionError):
            pass
            
        return info


class StatisticalAnalysis:
    """Statistical analysis utilities for benchmark data."""
    
    @staticmethod
    def compute_stats(values: List[float]) -> Dict[str, float]:
        """Compute comprehensive statistics for a list of values."""
        if not values:
            return {}
            
        return {
            "mean": statistics.mean(values),
            "median": statistics.median(values),
            "std_dev": statistics.stdev(values) if len(values) > 1 else 0.0,
            "min": min(values),
            "max": max(values),
            "q25": np.percentile(values, 25),
            "q75": np.percentile(values, 75),
            "cv": statistics.stdev(values) / statistics.mean(values) if len(values) > 1 and statistics.mean(values) != 0 else 0.0
        }
    
    @staticmethod
    def detect_outliers(values: List[float], method: str = "iqr") -> List[bool]:
        """Detect outliers in benchmark results."""
        if len(values) < 4:
            return [False] * len(values)
            
        if method == "iqr":
            q25, q75 = np.percentile(values, [25, 75])
            iqr = q75 - q25
            lower_bound = q25 - 1.5 * iqr
            upper_bound = q75 + 1.5 * iqr
            return [v < lower_bound or v > upper_bound for v in values]
        elif method == "zscore":
            mean = statistics.mean(values)
            std_dev = statistics.stdev(values)
            if std_dev == 0:
                return [False] * len(values)
            z_scores = [(v - mean) / std_dev for v in values]
            return [abs(z) > 2.5 for z in z_scores]
        else:
            raise ValueError(f"Unknown outlier detection method: {method}")
    
    @staticmethod
    def compare_distributions(baseline: List[float], current: List[float]) -> Dict[str, float]:
        """Compare two distributions of benchmark results."""
        if not baseline or not current:
            return {}
            
        baseline_stats = StatisticalAnalysis.compute_stats(baseline)
        current_stats = StatisticalAnalysis.compute_stats(current)
        
        # Compute relative changes
        comparison = {}
        for key in ["mean", "median", "std_dev", "min", "max"]:
            if baseline_stats.get(key, 0) != 0:
                change = (current_stats.get(key, 0) - baseline_stats.get(key, 0)) / baseline_stats[key]
                comparison[f"{key}_change_pct"] = change * 100
                
        # Statistical significance (simple t-test approximation)
        try:
            from scipy import stats
            t_stat, p_value = stats.ttest_ind(baseline, current)
            comparison["t_statistic"] = t_stat
            comparison["p_value"] = p_value
            comparison["significant"] = p_value < 0.05
        except ImportError:
            # scipy not available, skip statistical tests
            pass
            
        return comparison


class PerformanceClassifier:
    """Classifies benchmark performance characteristics."""
    
    # Performance thresholds for Atlas Layer 4 operations (nanoseconds)
    THRESHOLDS = {
        "un_operations": {
            "excellent": 10,      # < 10ns per UN operation
            "good": 50,           # < 50ns per UN operation
            "acceptable": 200,    # < 200ns per UN operation
            "poor": 1000         # > 1000ns per UN operation
        },
        "geometric": {
            "excellent": 100,     # < 100ns per geometric operation
            "good": 500,          # < 500ns per geometric operation
            "acceptable": 2000,   # < 2μs per geometric operation
            "poor": 10000        # > 10μs per geometric operation
        },
        "signal": {
            "excellent": 1000,    # < 1μs per signal operation
            "good": 5000,         # < 5μs per signal operation
            "acceptable": 20000,  # < 20μs per signal operation
            "poor": 100000       # > 100μs per signal operation
        },
        "memory": {
            "excellent": 1024,    # < 1KB memory usage
            "good": 8192,         # < 8KB memory usage
            "acceptable": 65536,  # < 64KB memory usage
            "poor": 1048576      # > 1MB memory usage
        }
    }
    
    @staticmethod
    def classify_performance(result: BenchmarkResult, category: str) -> str:
        """Classify benchmark performance as excellent/good/acceptable/poor."""
        if category not in PerformanceClassifier.THRESHOLDS:
            return "unknown"
            
        thresholds = PerformanceClassifier.THRESHOLDS[category]
        
        # Use duration per operation
        duration_per_op = result.duration_ns / max(result.iterations, 1)
        
        if duration_per_op <= thresholds["excellent"]:
            return "excellent"
        elif duration_per_op <= thresholds["good"]:
            return "good"
        elif duration_per_op <= thresholds["acceptable"]:
            return "acceptable"
        else:
            return "poor"
    
    @staticmethod
    def classify_memory_usage(memory_bytes: int) -> str:
        """Classify memory usage performance."""
        thresholds = PerformanceClassifier.THRESHOLDS["memory"]
        
        if memory_bytes <= thresholds["excellent"]:
            return "excellent"
        elif memory_bytes <= thresholds["good"]:
            return "good"
        elif memory_bytes <= thresholds["acceptable"]:
            return "acceptable"
        else:
            return "poor"


class ReportGenerator:
    """Generates formatted reports from benchmark data."""
    
    @staticmethod
    def generate_summary(suites: List[BenchmarkSuite]) -> Dict[str, Any]:
        """Generate a summary of all benchmark results."""
        total_benchmarks = sum(len(suite.results) for suite in suites)
        
        # Aggregate performance classifications
        classifications = {"excellent": 0, "good": 0, "acceptable": 0, "poor": 0}
        
        for suite in suites:
            for result in suite.results:
                # Determine category based on suite name
                category = "un_operations" if "core" in suite.name else "geometric"
                classification = PerformanceClassifier.classify_performance(result, category)
                classifications[classification] += 1
        
        return {
            "total_suites": len(suites),
            "total_benchmarks": total_benchmarks,
            "performance_breakdown": classifications,
            "timestamp": time.strftime("%Y-%m-%d %H:%M:%S")
        }
    
    @staticmethod
    def format_duration(nanoseconds: int) -> str:
        """Format duration in human-readable form."""
        if nanoseconds < 1000:
            return f"{nanoseconds}ns"
        elif nanoseconds < 1_000_000:
            return f"{nanoseconds / 1000:.1f}μs"
        elif nanoseconds < 1_000_000_000:
            return f"{nanoseconds / 1_000_000:.1f}ms"
        else:
            return f"{nanoseconds / 1_000_000_000:.1f}s"
    
    @staticmethod
    def format_throughput(ops_per_sec: float) -> str:
        """Format throughput in human-readable form."""
        if ops_per_sec < 1000:
            return f"{ops_per_sec:.1f} ops/sec"
        elif ops_per_sec < 1_000_000:
            return f"{ops_per_sec / 1000:.1f}K ops/sec"
        elif ops_per_sec < 1_000_000_000:
            return f"{ops_per_sec / 1_000_000:.1f}M ops/sec"
        else:
            return f"{ops_per_sec / 1_000_000_000:.1f}G ops/sec"


def main():
    """Example usage of benchmark utilities."""
    print("Atlas Layer 4 Benchmark Utilities")
    print("==================================")
    print()
    print("This module provides utilities for:")
    print("• Loading and parsing benchmark results")
    print("• Statistical analysis of performance data")
    print("• Performance classification and reporting")
    print("• Comparison between different benchmark runs")
    print()
    print("Usage:")
    print("  from benchmark_utils import BenchmarkAnalyzer, StatisticalAnalysis")
    print("  suites = BenchmarkAnalyzer.load_results(Path('results/session_20241201_120000'))")
    print("  summary = ReportGenerator.generate_summary(suites)")


if __name__ == "__main__":
    main()