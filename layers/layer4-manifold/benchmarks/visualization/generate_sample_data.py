#!/usr/bin/env python3
"""
Sample Benchmark Data Generator

This script generates realistic sample benchmark data for testing
the visualization tools when real benchmark data is not available.
"""

import argparse
import csv
import json
import random
from datetime import datetime, timedelta
from pathlib import Path
from typing import Dict, List, Any
import numpy as np

class SampleDataGenerator:
    """Generate realistic benchmark data for testing"""
    
    def __init__(self):
        # Atlas operation definitions with realistic performance characteristics
        self.atlas_operations = {
            'Linear Projection': {
                'base_ops_per_sec': 2500,
                'variance': 400,
                'base_throughput': 140_000_000,  # bytes/sec
                'memory_efficiency': 0.85,
                'conservation_rate': 1.0
            },
            'R96 Fourier Projection': {
                'base_ops_per_sec': 2800,
                'variance': 350,
                'base_throughput': 130_000_000,
                'memory_efficiency': 0.88,
                'conservation_rate': 1.0
            },
            'Shard Extraction': {
                'base_ops_per_sec': 1800,
                'variance': 300,
                'base_throughput': 85_000_000,
                'memory_efficiency': 0.75,
                'conservation_rate': 1.0
            },
            'Batch Shard Extraction': {
                'base_ops_per_sec': 1300,
                'variance': 250,
                'base_throughput': 150_000_000,
                'memory_efficiency': 0.92,
                'conservation_rate': 1.0
            },
            'Transformation': {
                'base_ops_per_sec': 1900,
                'variance': 280,
                'base_throughput': 75_000_000,
                'memory_efficiency': 0.70,
                'conservation_rate': 1.0
            },
            'Reconstruction': {
                'base_ops_per_sec': 1100,
                'variance': 200,
                'base_throughput': 65_000_000,
                'memory_efficiency': 0.68,
                'conservation_rate': 1.0
            }
        }
        
        # Traditional operation performance (generally worse)
        self.traditional_operations = {
            'Traditional Matrix Operation': {
                'base_ops_per_sec': 800,
                'variance': 150,
                'base_throughput': 45_000_000,
                'memory_efficiency': 0.35,
                'conservation_rate': 0.0
            },
            'Standard FFT': {
                'base_ops_per_sec': 750,
                'variance': 120,
                'base_throughput': 38_000_000,
                'memory_efficiency': 0.30,
                'conservation_rate': 0.0
            },
            'Basic Memory Copy': {
                'base_ops_per_sec': 600,
                'variance': 100,
                'base_throughput': 28_000_000,
                'memory_efficiency': 0.25,
                'conservation_rate': 0.0
            }
        }
        
        # Data size categories
        self.data_sizes = [
            ('4 KB', 4 * 1024),
            ('64 KB', 64 * 1024),
            ('1 MB', 1024 * 1024),
            ('4 MB', 4 * 1024 * 1024),
            ('16 MB', 16 * 1024 * 1024)
        ]
        
        # Shard counts for batch operations
        self.shard_counts = [2, 4, 8, 16]
    
    def generate_operation_data(self, operation_name: str, op_config: Dict[str, float], 
                              data_size: tuple, timestamp: datetime, 
                              iterations: int = 100) -> Dict[str, Any]:
        """Generate realistic benchmark data for a single operation"""
        
        size_name, size_bytes = data_size
        
        # Calculate performance with size scaling
        size_factor = np.log2(size_bytes / 1024) / 10  # Logarithmic scaling factor
        ops_per_sec = max(100, op_config['base_ops_per_sec'] * (1 - size_factor * 0.3) + 
                         random.gauss(0, op_config['variance']))
        
        # Calculate timing
        avg_time_ms = 1000 / ops_per_sec if ops_per_sec > 0 else 1000
        
        # Throughput scales with data size and operations per second
        throughput_bps = op_config['base_throughput'] * (1 - size_factor * 0.2) + random.gauss(0, op_config['base_throughput'] * 0.1)
        throughput_bps = max(1000, throughput_bps)
        
        # Memory usage
        memory_usage_mb = (size_bytes * 1.5) / (1024 * 1024) / op_config['memory_efficiency']
        
        # Conservation compliance (Atlas operations are always compliant)
        conservation_compliant = random.random() < op_config['conservation_rate']
        conservation_score = op_config['conservation_rate'] if conservation_compliant else random.uniform(0.6, 0.9)
        
        # Create operation name with size
        full_operation_name = f"{operation_name} ({size_name})"
        if 'Batch' in operation_name and random.choice([True, False]):
            shard_count = random.choice(self.shard_counts)
            full_operation_name = f"{operation_name} ({size_name}, {shard_count} shards)"
        
        return {
            'timestamp': timestamp.isoformat(),
            'operation': full_operation_name,
            'operation_type': operation_name,
            'avg_time_ms': round(avg_time_ms, 3),
            'min_time_ms': round(avg_time_ms * 0.8, 3),
            'max_time_ms': round(avg_time_ms * 1.3, 3),
            'ops_per_sec': round(ops_per_sec, 2),
            'throughput_bps': int(throughput_bps),
            'memory_usage_mb': round(memory_usage_mb, 2),
            'iterations': iterations,
            'conservation_compliant': conservation_compliant,
            'conservation_score': round(conservation_score, 3),
            'data_size_bytes': size_bytes,
            'is_atlas': operation_name in self.atlas_operations
        }
    
    def generate_time_series_data(self, hours: int = 24, samples_per_hour: int = 4) -> List[Dict[str, Any]]:
        """Generate time series benchmark data"""
        
        data = []
        current_time = datetime.now() - timedelta(hours=hours)
        time_step = timedelta(hours=1/samples_per_hour)
        
        for _ in range(hours * samples_per_hour):
            # Generate Atlas operations
            for op_name, op_config in self.atlas_operations.items():
                # Not all operations run at every time step
                if random.random() < 0.7:  # 70% chance of running
                    data_size = random.choice(self.data_sizes)
                    iterations = random.choice([10, 50, 100, 200])
                    
                    operation_data = self.generate_operation_data(
                        op_name, op_config, data_size, current_time, iterations
                    )
                    data.append(operation_data)
            
            # Generate some traditional operations for comparison
            if random.random() < 0.3:  # 30% chance
                op_name = random.choice(list(self.traditional_operations.keys()))
                op_config = self.traditional_operations[op_name]
                data_size = random.choice(self.data_sizes[:3])  # Smaller sizes for traditional
                iterations = random.choice([10, 25, 50])
                
                operation_data = self.generate_operation_data(
                    op_name, op_config, data_size, current_time, iterations
                )
                data.append(operation_data)
            
            current_time += time_step
        
        return data
    
    def save_csv_data(self, data: List[Dict[str, Any]], filename: str) -> None:
        """Save data as CSV file"""
        
        if not data:
            return
        
        fieldnames = data[0].keys()
        
        with open(filename, 'w', newline='') as csvfile:
            writer = csv.DictWriter(csvfile, fieldnames=fieldnames)
            writer.writeheader()
            writer.writerows(data)
    
    def save_json_data(self, data: List[Dict[str, Any]], filename: str) -> None:
        """Save data as JSON file"""
        
        with open(filename, 'w') as jsonfile:
            json.dump(data, jsonfile, indent=2, default=str)
    
    def generate_comprehensive_dataset(self, output_dir: Path, 
                                     days: int = 7, 
                                     samples_per_hour: int = 2) -> None:
        """Generate a comprehensive benchmark dataset"""
        
        output_dir = Path(output_dir)
        output_dir.mkdir(parents=True, exist_ok=True)
        
        print(f"Generating {days} days of sample benchmark data...")
        
        # Generate time series data
        all_data = self.generate_time_series_data(
            hours=days * 24, 
            samples_per_hour=samples_per_hour
        )
        
        print(f"Generated {len(all_data)} benchmark records")
        
        # Save complete dataset
        self.save_csv_data(all_data, output_dir / f"benchmark_data_{days}d.csv")
        self.save_json_data(all_data, output_dir / f"benchmark_data_{days}d.json")
        
        # Create daily files for time-based analysis
        data_by_day = {}
        for record in all_data:
            day_key = record['timestamp'][:10]  # Extract date (YYYY-MM-DD)
            if day_key not in data_by_day:
                data_by_day[day_key] = []
            data_by_day[day_key].append(record)
        
        for day, day_data in data_by_day.items():
            self.save_csv_data(day_data, output_dir / f"benchmark_data_{day}.csv")
        
        # Generate operation-specific files
        operations_data = {}
        for record in all_data:
            op_type = record['operation_type']
            if op_type not in operations_data:
                operations_data[op_type] = []
            operations_data[op_type].append(record)
        
        for op_name, op_data in operations_data.items():
            safe_name = op_name.replace(' ', '_').lower()
            self.save_csv_data(op_data, output_dir / f"benchmark_{safe_name}.csv")
        
        # Generate summary statistics
        self.generate_summary_stats(all_data, output_dir / "summary_stats.json")
        
        print(f"Sample data saved to: {output_dir}")
        print("Files generated:")
        for file_path in sorted(output_dir.glob("*")):
            print(f"  - {file_path.name}")
    
    def generate_summary_stats(self, data: List[Dict[str, Any]], filename: Path) -> None:
        """Generate summary statistics"""
        
        if not data:
            return
        
        # Calculate statistics
        atlas_data = [d for d in data if d['is_atlas']]
        traditional_data = [d for d in data if not d['is_atlas']]
        
        stats = {
            'generation_timestamp': datetime.now().isoformat(),
            'total_records': len(data),
            'atlas_records': len(atlas_data),
            'traditional_records': len(traditional_data),
            'date_range': {
                'start': min(d['timestamp'] for d in data),
                'end': max(d['timestamp'] for d in data)
            },
            'performance_summary': {
                'avg_atlas_ops_per_sec': np.mean([d['ops_per_sec'] for d in atlas_data]) if atlas_data else 0,
                'avg_traditional_ops_per_sec': np.mean([d['ops_per_sec'] for d in traditional_data]) if traditional_data else 0,
                'avg_atlas_throughput': np.mean([d['throughput_bps'] for d in atlas_data]) if atlas_data else 0,
                'avg_traditional_throughput': np.mean([d['throughput_bps'] for d in traditional_data]) if traditional_data else 0,
                'conservation_compliance_rate': np.mean([d['conservation_compliant'] for d in atlas_data]) if atlas_data else 0
            },
            'operation_counts': {}
        }
        
        # Count operations by type
        for record in data:
            op_type = record['operation_type']
            if op_type not in stats['operation_counts']:
                stats['operation_counts'][op_type] = 0
            stats['operation_counts'][op_type] += 1
        
        with open(filename, 'w') as f:
            json.dump(stats, f, indent=2, default=str)


def main():
    parser = argparse.ArgumentParser(description='Generate sample benchmark data for testing')
    parser.add_argument('--output-dir', type=str, default='../results',
                        help='Output directory for sample data')
    parser.add_argument('--days', type=int, default=7,
                        help='Number of days of data to generate')
    parser.add_argument('--samples-per-hour', type=int, default=2,
                        help='Number of benchmark samples per hour')
    parser.add_argument('--format', choices=['csv', 'json', 'both'], default='both',
                        help='Output format')
    
    args = parser.parse_args()
    
    generator = SampleDataGenerator()
    
    try:
        generator.generate_comprehensive_dataset(
            output_dir=args.output_dir,
            days=args.days,
            samples_per_hour=args.samples_per_hour
        )
        
        print("\n✅ Sample data generation completed successfully!")
        print(f"📁 Data saved to: {args.output_dir}")
        print("\n🚀 Next steps:")
        print("1. Run visualization: python3 visualize_results.py --data-dir ../results")
        print("2. Generate report: ./generate_report.sh")
        print("3. View dashboard: open dashboard.html")
        
    except Exception as e:
        print(f"❌ Error generating sample data: {e}")
        return 1
    
    return 0


if __name__ == '__main__':
    exit(main())