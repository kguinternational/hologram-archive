//! Performance benchmarks for Layer 4 manifold operations
//!
//! This module provides comprehensive benchmarking capabilities for projection
//! and shard operations to measure and optimize performance characteristics.

#![allow(clippy::module_name_repetitions)]

use crate::error::*;
use crate::projection::*;
use crate::shard::*;

#[cfg(feature = "std")]
use std::time::{Duration, Instant};

#[cfg(feature = "std")]
use std::vec::Vec;

#[cfg(not(feature = "std"))]
use alloc::vec::Vec;

/// Benchmark result for a single operation
#[derive(Debug, Clone)]
pub struct BenchmarkResult {
    /// Name of the operation benchmarked
    pub operation_name: String,
    /// Number of iterations performed
    pub iterations: u32,
    /// Total time elapsed
    pub total_duration: Duration,
    /// Average time per operation
    pub avg_duration: Duration,
    /// Minimum time recorded
    pub min_duration: Duration,
    /// Maximum time recorded
    pub max_duration: Duration,
    /// Data processed per second (bytes/sec)
    pub throughput_bps: u64,
    /// Operations per second
    pub ops_per_sec: f64,
}

/// Benchmark suite for manifold operations
#[derive(Debug)]
pub struct ManifoldBenchmark {
    /// Results from completed benchmarks
    pub results: Vec<BenchmarkResult>,
    /// Configuration for benchmarks
    pub config: BenchmarkConfig,
}

/// Configuration for benchmark execution
#[derive(Debug, Clone)]
pub struct BenchmarkConfig {
    /// Number of iterations to run for each benchmark
    pub iterations: u32,
    /// Minimum time to run each benchmark (milliseconds)
    pub min_duration_ms: u64,
    /// Maximum time to run each benchmark (milliseconds)
    pub max_duration_ms: u64,
    /// Warm-up iterations before timing
    pub warmup_iterations: u32,
    /// Enable detailed timing per iteration
    pub detailed_timing: bool,
}

impl Default for BenchmarkConfig {
    fn default() -> Self {
        Self {
            iterations: 100,
            min_duration_ms: 1000,   // 1 second minimum
            max_duration_ms: 10000,  // 10 second maximum
            warmup_iterations: 10,
            detailed_timing: false,
        }
    }
}

impl BenchmarkResult {
    /// Create a new benchmark result
    pub fn new(operation_name: String, iterations: u32, durations: &[Duration], data_size: u64) -> Self {
        let total_duration: Duration = durations.iter().sum();
        let avg_duration = total_duration / iterations;
        let min_duration = *durations.iter().min().unwrap_or(&Duration::ZERO);
        let max_duration = *durations.iter().max().unwrap_or(&Duration::ZERO);
        
        let throughput_bps = if total_duration.as_nanos() > 0 {
            (data_size * iterations as u64 * 1_000_000_000) / total_duration.as_nanos() as u64
        } else {
            0
        };
        
        let ops_per_sec = if total_duration.as_secs_f64() > 0.0 {
            iterations as f64 / total_duration.as_secs_f64()
        } else {
            0.0
        };

        Self {
            operation_name,
            iterations,
            total_duration,
            avg_duration,
            min_duration,
            max_duration,
            throughput_bps,
            ops_per_sec,
        }
    }

    /// Format throughput as human-readable string
    pub fn throughput_string(&self) -> String {
        if self.throughput_bps >= 1_000_000_000 {
            format!("{:.2} GB/s", self.throughput_bps as f64 / 1_000_000_000.0)
        } else if self.throughput_bps >= 1_000_000 {
            format!("{:.2} MB/s", self.throughput_bps as f64 / 1_000_000.0)
        } else if self.throughput_bps >= 1_000 {
            format!("{:.2} KB/s", self.throughput_bps as f64 / 1_000.0)
        } else {
            format!("{} B/s", self.throughput_bps)
        }
    }

    /// Format average duration as human-readable string
    pub fn avg_duration_string(&self) -> String {
        if self.avg_duration.as_secs() > 0 {
            format!("{:.3} s", self.avg_duration.as_secs_f64())
        } else if self.avg_duration.as_millis() > 0 {
            format!("{:.3} ms", self.avg_duration.as_millis())
        } else if self.avg_duration.as_micros() > 0 {
            format!("{:.3} μs", self.avg_duration.as_micros())
        } else {
            format!("{} ns", self.avg_duration.as_nanos())
        }
    }
}

impl ManifoldBenchmark {
    /// Create a new benchmark suite
    pub fn new(config: BenchmarkConfig) -> Self {
        Self {
            results: Vec::new(),
            config,
        }
    }

    /// Run linear projection benchmark
    pub fn bench_linear_projection(&mut self, data_sizes: &[usize]) -> AtlasResult<()> {
        for &size in data_sizes {
            let test_data = Self::create_test_data(size);
            let mut durations = Vec::new();

            // Warmup
            for _ in 0..self.config.warmup_iterations {
                let _ = AtlasProjection::new_linear(&test_data)?;
            }

            // Benchmark
            let start_time = Instant::now();
            let mut iterations = 0;

            while iterations < self.config.iterations {
                let iter_start = Instant::now();
                let _projection = AtlasProjection::new_linear(&test_data)?;
                let iter_duration = iter_start.elapsed();
                durations.push(iter_duration);
                iterations += 1;

                // Check time limits
                if start_time.elapsed().as_millis() > self.config.max_duration_ms as u128 {
                    break;
                }
            }

            let result = BenchmarkResult::new(
                format!("Linear Projection ({})", Self::format_size(size)),
                iterations,
                &durations,
                size as u64,
            );

            self.results.push(result);
        }

        Ok(())
    }

    /// Run R96 Fourier projection benchmark
    pub fn bench_r96_fourier_projection(&mut self, data_sizes: &[usize]) -> AtlasResult<()> {
        for &size in data_sizes {
            let test_data = Self::create_conservation_test_data(size);
            let mut durations = Vec::new();

            // Warmup
            for _ in 0..self.config.warmup_iterations {
                let _ = AtlasProjection::new_r96_fourier(&test_data)?;
            }

            // Benchmark
            let start_time = Instant::now();
            let mut iterations = 0;

            while iterations < self.config.iterations {
                let iter_start = Instant::now();
                let _projection = AtlasProjection::new_r96_fourier(&test_data)?;
                let iter_duration = iter_start.elapsed();
                durations.push(iter_duration);
                iterations += 1;

                // Check time limits
                if start_time.elapsed().as_millis() > self.config.max_duration_ms as u128 {
                    break;
                }
            }

            let result = BenchmarkResult::new(
                format!("R96 Fourier Projection ({})", Self::format_size(size)),
                iterations,
                &durations,
                size as u64,
            );

            self.results.push(result);
        }

        Ok(())
    }

    /// Run shard extraction benchmark
    pub fn bench_shard_extraction(&mut self, data_sizes: &[usize], shard_counts: &[u32]) -> AtlasResult<()> {
        for &size in data_sizes {
            let test_data = Self::create_test_data(size);
            let projection = AtlasProjection::new_linear(&test_data)?;

            for &shard_count in shard_counts {
                let regions = Self::create_boundary_regions(shard_count, size);
                let mut durations = Vec::new();

                // Warmup
                for _ in 0..self.config.warmup_iterations.min(5) {
                    for region in &regions {
                        let _ = projection.extract_shard(region)?;
                    }
                }

                // Benchmark
                let start_time = Instant::now();
                let mut iterations = 0;

                while iterations < self.config.iterations.min(50) { // Limit iterations for shard extraction
                    let iter_start = Instant::now();
                    
                    for region in &regions {
                        let _shard = projection.extract_shard(region)?;
                    }
                    
                    let iter_duration = iter_start.elapsed();
                    durations.push(iter_duration);
                    iterations += 1;

                    // Check time limits
                    if start_time.elapsed().as_millis() > self.config.max_duration_ms as u128 {
                        break;
                    }
                }

                let result = BenchmarkResult::new(
                    format!("Shard Extraction ({}, {} shards)", Self::format_size(size), shard_count),
                    iterations,
                    &durations,
                    size as u64,
                );

                self.results.push(result);
            }
        }

        Ok(())
    }

    /// Run batch shard extraction benchmark
    pub fn bench_batch_shard_extraction(&mut self, data_sizes: &[usize], shard_counts: &[u32]) -> AtlasResult<()> {
        for &size in data_sizes {
            let test_data = Self::create_test_data(size);
            let projection = AtlasProjection::new_linear(&test_data)?;

            for &shard_count in shard_counts {
                let regions = Self::create_boundary_regions(shard_count, size);
                let mut durations = Vec::new();

                // Warmup
                for _ in 0..self.config.warmup_iterations.min(5) {
                    let _ = projection.extract_shards_batch(&regions)?;
                }

                // Benchmark
                let start_time = Instant::now();
                let mut iterations = 0;

                while iterations < self.config.iterations.min(20) { // Limit for batch operations
                    let iter_start = Instant::now();
                    let _shards = projection.extract_shards_batch(&regions)?;
                    let iter_duration = iter_start.elapsed();
                    durations.push(iter_duration);
                    iterations += 1;

                    // Check time limits
                    if start_time.elapsed().as_millis() > self.config.max_duration_ms as u128 {
                        break;
                    }
                }

                let result = BenchmarkResult::new(
                    format!("Batch Shard Extraction ({}, {} shards)", Self::format_size(size), shard_count),
                    iterations,
                    &durations,
                    size as u64,
                );

                self.results.push(result);
            }
        }

        Ok(())
    }

    /// Run transformation benchmark
    pub fn bench_transformations(&mut self, data_sizes: &[usize]) -> AtlasResult<()> {
        for &size in data_sizes {
            let test_data = Self::create_test_data(size);
            let mut projection = AtlasProjection::new_linear(&test_data)?;
            let transform_params = TransformationParams {
                scaling_factor: 1.5,
                rotation_angle: std::f64::consts::PI / 4.0,
                translation_x: 10.0,
                translation_y: 20.0,
            };

            let mut durations = Vec::new();

            // Warmup
            for _ in 0..self.config.warmup_iterations {
                let _ = projection.apply_transform(transform_params)?;
            }

            // Benchmark
            let start_time = Instant::now();
            let mut iterations = 0;

            while iterations < self.config.iterations {
                let iter_start = Instant::now();
                let _ = projection.apply_transform(transform_params)?;
                let iter_duration = iter_start.elapsed();
                durations.push(iter_duration);
                iterations += 1;

                // Check time limits
                if start_time.elapsed().as_millis() > self.config.max_duration_ms as u128 {
                    break;
                }
            }

            let result = BenchmarkResult::new(
                format!("Transformation ({})", Self::format_size(size)),
                iterations,
                &durations,
                size as u64,
            );

            self.results.push(result);
        }

        Ok(())
    }

    /// Run reconstruction benchmark
    pub fn bench_reconstruction(&mut self, data_sizes: &[usize]) -> AtlasResult<()> {
        for &size in data_sizes {
            let test_data = Self::create_test_data(size);
            let projection = AtlasProjection::new_linear(&test_data)?;
            
            // Create shards for reconstruction
            let regions = Self::create_boundary_regions(4, size);
            let mut ctx = AtlasReconstructionCtx::new(regions.len() as u32);
            
            for region in &regions {
                let shard_handle = projection.extract_shard(region)?;
                // SAFETY: We just created this handle from a valid shard
                let shard = unsafe {
                    if let Some(shard_ref) = shard_handle.as_ref() {
                        shard_ref.clone()
                    } else {
                        return Err(AtlasError::InvalidInput("invalid shard handle"));
                    }
                };
                ctx.add_shard(shard)?;
            }

            let mut durations = Vec::new();

            // Warmup
            for _ in 0..self.config.warmup_iterations.min(3) {
                let _ = reconstruct_projection_from_shards(&ctx, ProjectionType::Linear)?;
            }

            // Benchmark
            let start_time = Instant::now();
            let mut iterations = 0;

            while iterations < self.config.iterations.min(10) { // Limit for complex operations
                let iter_start = Instant::now();
                let _reconstructed = reconstruct_projection_from_shards(&ctx, ProjectionType::Linear)?;
                let iter_duration = iter_start.elapsed();
                durations.push(iter_duration);
                iterations += 1;

                // Check time limits
                if start_time.elapsed().as_millis() > self.config.max_duration_ms as u128 {
                    break;
                }
            }

            let result = BenchmarkResult::new(
                format!("Reconstruction ({})", Self::format_size(size)),
                iterations,
                &durations,
                size as u64,
            );

            self.results.push(result);
        }

        Ok(())
    }

    /// Run comprehensive benchmark suite
    pub fn run_full_benchmark_suite(&mut self) -> AtlasResult<()> {
        let data_sizes = vec![
            4 * 1024,      // 4 KB
            64 * 1024,     // 64 KB
            1024 * 1024,   // 1 MB
            16 * 1024 * 1024, // 16 MB
        ];
        
        let shard_counts = vec![2, 4, 8];

        println!("Running Linear Projection benchmarks...");
        self.bench_linear_projection(&data_sizes)?;

        println!("Running R96 Fourier Projection benchmarks...");
        self.bench_r96_fourier_projection(&data_sizes)?;

        println!("Running Shard Extraction benchmarks...");
        self.bench_shard_extraction(&data_sizes, &shard_counts)?;

        println!("Running Batch Shard Extraction benchmarks...");
        self.bench_batch_shard_extraction(&data_sizes, &shard_counts)?;

        println!("Running Transformation benchmarks...");
        self.bench_transformations(&data_sizes)?;

        println!("Running Reconstruction benchmarks...");
        self.bench_reconstruction(&data_sizes)?;

        Ok(())
    }

    /// Print benchmark results
    pub fn print_results(&self) {
        println!("\n=== Layer 4 Manifold Benchmark Results ===\n");
        
        for result in &self.results {
            println!("Operation: {}", result.operation_name);
            println!("  Iterations: {}", result.iterations);
            println!("  Average Time: {}", result.avg_duration_string());
            println!("  Min Time: {:?}", result.min_duration);
            println!("  Max Time: {:?}", result.max_duration);
            println!("  Throughput: {}", result.throughput_string());
            println!("  Ops/Second: {:.2}", result.ops_per_sec);
            println!();
        }
    }

    /// Get benchmark summary
    pub fn get_summary(&self) -> BenchmarkSummary {
        let mut summary = BenchmarkSummary {
            total_operations: self.results.len(),
            avg_throughput_bps: 0,
            fastest_operation: String::new(),
            slowest_operation: String::new(),
            fastest_duration: Duration::from_secs(u64::MAX),
            slowest_duration: Duration::ZERO,
        };

        if self.results.is_empty() {
            return summary;
        }

        let total_throughput: u64 = self.results.iter().map(|r| r.throughput_bps).sum();
        summary.avg_throughput_bps = total_throughput / self.results.len() as u64;

        for result in &self.results {
            if result.avg_duration < summary.fastest_duration {
                summary.fastest_duration = result.avg_duration;
                summary.fastest_operation = result.operation_name.clone();
            }
            if result.avg_duration > summary.slowest_duration {
                summary.slowest_duration = result.avg_duration;
                summary.slowest_operation = result.operation_name.clone();
            }
        }

        summary
    }

    // Helper methods

    fn create_test_data(size: usize) -> Vec<u8> {
        vec![0u8; size] // Simple test data that satisfies conservation laws
    }

    fn create_conservation_test_data(size: usize) -> Vec<u8> {
        let mut data = vec![0u8; size];
        // Create pattern that satisfies conservation laws
        for (i, byte) in data.iter_mut().enumerate() {
            *byte = (i % 96) as u8;
        }
        data
    }

    fn create_boundary_regions(count: u32, data_size: usize) -> Vec<AtlasBoundaryRegion> {
        let mut regions = Vec::new();
        let region_size = data_size / count as usize;

        for i in 0..count {
            let start = i as usize * region_size;
            let end = if i == count - 1 { data_size } else { (i + 1) as usize * region_size };
            let page_count = ((end - start) / 4096).max(1) as u16;

            regions.push(AtlasBoundaryRegion::new(
                start as u32,
                end as u32,
                page_count,
                (i % 96) as u8,
            ));
        }

        regions
    }

    fn format_size(size: usize) -> String {
        if size >= 1024 * 1024 {
            format!("{} MB", size / (1024 * 1024))
        } else if size >= 1024 {
            format!("{} KB", size / 1024)
        } else {
            format!("{} B", size)
        }
    }
}

/// Summary of benchmark results
#[derive(Debug, Clone)]
pub struct BenchmarkSummary {
    /// Total number of operations benchmarked
    pub total_operations: usize,
    /// Average throughput across all operations
    pub avg_throughput_bps: u64,
    /// Name of the fastest operation
    pub fastest_operation: String,
    /// Name of the slowest operation
    pub slowest_operation: String,
    /// Duration of the fastest operation
    pub fastest_duration: Duration,
    /// Duration of the slowest operation
    pub slowest_duration: Duration,
}

/// Run quick benchmark for basic operations
pub fn run_quick_benchmark() -> AtlasResult<BenchmarkSummary> {
    let config = BenchmarkConfig {
        iterations: 10,
        min_duration_ms: 100,
        max_duration_ms: 2000,
        warmup_iterations: 3,
        detailed_timing: false,
    };

    let mut benchmark = ManifoldBenchmark::new(config);
    
    // Quick test with small data sizes
    let data_sizes = vec![4096, 65536]; // 4KB, 64KB
    let shard_counts = vec![2, 4];

    benchmark.bench_linear_projection(&data_sizes)?;
    benchmark.bench_shard_extraction(&data_sizes, &shard_counts)?;

    Ok(benchmark.get_summary())
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn test_benchmark_config() {
        let config = BenchmarkConfig::default();
        assert_eq!(config.iterations, 100);
        assert_eq!(config.warmup_iterations, 10);
    }

    #[test]
    fn test_benchmark_result() {
        let durations = vec![
            Duration::from_millis(10),
            Duration::from_millis(15),
            Duration::from_millis(12),
        ];
        
        let result = BenchmarkResult::new(
            "Test Operation".to_string(),
            3,
            &durations,
            1000,
        );

        assert_eq!(result.iterations, 3);
        assert_eq!(result.min_duration, Duration::from_millis(10));
        assert_eq!(result.max_duration, Duration::from_millis(15));
        assert!(result.ops_per_sec > 0.0);
    }

    #[test]
    fn test_quick_benchmark() {
        let summary = run_quick_benchmark();
        assert!(summary.is_ok());
        
        let summary = summary.unwrap();
        assert!(summary.total_operations > 0);
    }

    #[test]
    fn test_format_functions() {
        let result = BenchmarkResult::new(
            "Test".to_string(),
            1,
            &[Duration::from_millis(5)],
            1000,
        );

        let throughput_str = result.throughput_string();
        assert!(!throughput_str.is_empty());
        
        let duration_str = result.avg_duration_string();
        assert!(!duration_str.is_empty());
    }
}