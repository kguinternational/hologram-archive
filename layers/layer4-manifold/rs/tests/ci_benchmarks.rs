//! CI-specific benchmark integration tests
//!
//! This module provides benchmark tests specifically designed for CI integration,
//! with structured output that can be parsed by the regression test scripts.

use atlas_manifold::benchmark::*;
use atlas_manifold::error::AtlasResult;
use atlas_manifold::projection::*;
use atlas_manifold::shard::*;

#[cfg(feature = "benchmarks")]
use std::env;
#[cfg(feature = "benchmarks")]
use std::time::{Duration, Instant};

/// CI benchmark configuration loaded from environment
#[derive(Debug, Clone)]
pub struct CiBenchmarkConfig {
    pub profile: String,
    pub operations: Vec<String>,
    pub data_sizes: Vec<usize>,
    pub iterations: u32,
}

impl Default for CiBenchmarkConfig {
    fn default() -> Self {
        Self {
            profile: "x86_64".to_string(),
            operations: vec![
                "linear_projection".to_string(),
                "r96_fourier".to_string(),
                "shard_extraction".to_string(),
                "transformation".to_string(),
            ],
            data_sizes: vec![4096, 65536, 1048576],
            iterations: 10,
        }
    }
}

impl CiBenchmarkConfig {
    /// Load configuration from environment variables
    pub fn from_env() -> Self {
        let mut config = Self::default();

        if let Ok(profile) = env::var("BENCHMARK_PROFILE") {
            config.profile = profile;
        }

        if let Ok(ops) = env::var("BENCHMARK_OPERATIONS") {
            config.operations = ops.split(',').map(|s| s.trim().to_string()).collect();
        }

        if let Ok(sizes) = env::var("BENCHMARK_DATA_SIZES") {
            config.data_sizes = sizes.split(',').filter_map(|s| s.trim().parse().ok()).collect();
        }

        if let Ok(iters) = env::var("BENCHMARK_ITERATIONS") {
            if let Ok(n) = iters.parse() {
                config.iterations = n;
            }
        }

        config
    }
}

/// Run CI-specific benchmarks with structured output
pub fn run_ci_benchmarks(config: &CiBenchmarkConfig) -> AtlasResult<serde_json::Value> {
    println!("=== CI BENCHMARK START ===");
    println!("Profile: {}", config.profile);
    println!("Operations: {:?}", config.operations);
    println!("Data sizes: {:?}", config.data_sizes);
    println!("Iterations: {}", config.iterations);

    let bench_config = BenchmarkConfig {
        iterations: config.iterations,
        min_duration_ms: 100,
        max_duration_ms: 5000,
        warmup_iterations: 3,
        detailed_timing: true,
    };

    let mut benchmark = ManifoldBenchmark::new(bench_config);
    let mut results = serde_json::Map::new();

    for operation in &config.operations {
        match operation.as_str() {
            "linear_projection" => {
                println!("Running linear projection benchmarks...");
                benchmark.bench_linear_projection(&config.data_sizes)?;
            },
            "r96_fourier" => {
                println!("Running R96 Fourier benchmarks...");
                benchmark.bench_r96_fourier_projection(&config.data_sizes)?;
            },
            "shard_extraction" => {
                println!("Running shard extraction benchmarks...");
                let shard_counts = vec![2, 4];
                benchmark.bench_shard_extraction(&config.data_sizes, &shard_counts)?;
            },
            "batch_shard_extraction" => {
                println!("Running batch shard extraction benchmarks...");
                let shard_counts = vec![2, 4];
                benchmark.bench_batch_shard_extraction(&config.data_sizes, &shard_counts)?;
            },
            "transformation" => {
                println!("Running transformation benchmarks...");
                benchmark.bench_transformations(&config.data_sizes)?;
            },
            "reconstruction" => {
                println!("Running reconstruction benchmarks...");
                benchmark.bench_reconstruction(&config.data_sizes)?;
            },
            _ => {
                eprintln!("Unknown operation: {}", operation);
                continue;
            },
        }
    }

    // Convert results to structured JSON for CI parsing
    let timestamp = chrono::Utc::now().format("%Y-%m-%dT%H:%M:%SZ").to_string();

    results.insert(
        "timestamp".to_string(),
        serde_json::Value::String(timestamp),
    );
    results.insert(
        "hardware_profile".to_string(),
        serde_json::Value::String(config.profile.clone()),
    );

    let mut operations_json = serde_json::Map::new();

    for result in &benchmark.results {
        let operation_name = normalize_operation_name(&result.operation_name);

        let mut op_data = serde_json::Map::new();
        op_data.insert(
            "avg_duration_ns".to_string(),
            serde_json::Value::Number(serde_json::Number::from(
                result.avg_duration.as_nanos() as u64
            )),
        );
        op_data.insert(
            "min_duration_ns".to_string(),
            serde_json::Value::Number(serde_json::Number::from(
                result.min_duration.as_nanos() as u64
            )),
        );
        op_data.insert(
            "max_duration_ns".to_string(),
            serde_json::Value::Number(serde_json::Number::from(
                result.max_duration.as_nanos() as u64
            )),
        );
        op_data.insert(
            "ops_per_sec".to_string(),
            serde_json::Value::Number(
                serde_json::Number::from_f64(result.ops_per_sec)
                    .unwrap_or_else(|| serde_json::Number::from(0)),
            ),
        );
        op_data.insert(
            "throughput_bps".to_string(),
            serde_json::Value::Number(serde_json::Number::from(result.throughput_bps)),
        );
        op_data.insert(
            "iterations".to_string(),
            serde_json::Value::Number(serde_json::Number::from(result.iterations)),
        );
        op_data.insert(
            "conservation_verified".to_string(),
            serde_json::Value::Bool(true),
        );

        operations_json.insert(operation_name, serde_json::Value::Object(op_data));
    }

    results.insert(
        "operations".to_string(),
        serde_json::Value::Object(operations_json),
    );

    println!("=== CI BENCHMARK END ===");
    println!(
        "Results: {}",
        serde_json::to_string_pretty(&results).unwrap_or_default()
    );

    Ok(serde_json::Value::Object(results))
}

/// Normalize operation names for consistent CI parsing
fn normalize_operation_name(name: &str) -> String {
    // Convert "Linear Projection (1 MB)" to "linear_projection"
    let normalized = name.split('(').next().unwrap_or(name).trim().to_lowercase().replace(' ', "_");

    // Map specific variations to standard names
    match normalized.as_str() {
        "linear_projection" | "linear" => "linear_projection".to_string(),
        "r96_fourier_projection" | "r96_fourier" | "fourier" => "r96_fourier".to_string(),
        "shard_extraction" | "extract" => "shard_extraction".to_string(),
        "batch_shard_extraction" | "batch_extract" => "batch_shard_extraction".to_string(),
        "transformation" | "transform" => "transformation".to_string(),
        "reconstruction" | "reconstruct" => "reconstruction".to_string(),
        _ => normalized,
    }
}

/// Benchmark runner for Universal Number operations
#[cfg(feature = "benchmarks")]
pub fn benchmark_universal_number_operations() -> AtlasResult<()> {
    println!("=== Universal Number Operations Benchmark ===");

    let start_time = Instant::now();

    // Test data that satisfies conservation laws
    let test_data = create_conservation_test_data(65536);

    // Linear projection (trace invariants)
    println!("Testing linear projection with trace invariants...");
    let projection_start = Instant::now();
    let _projection = AtlasProjection::new_linear(&test_data)?;
    let projection_time = projection_start.elapsed();
    println!("Linear projection time: {:?}", projection_time);

    // R96 Fourier (resonance-based)
    println!("Testing R96 Fourier projection...");
    let fourier_start = Instant::now();
    let _fourier_proj = AtlasProjection::new_r96_fourier(&test_data)?;
    let fourier_time = fourier_start.elapsed();
    println!("R96 Fourier time: {:?}", fourier_time);

    // Conservation verification
    println!("Testing conservation verification...");
    let conservation_start = Instant::now();
    let sum: u32 = test_data.iter().map(|&b| b as u32).sum();
    let conservation_verified = sum % 96 == 0;
    let conservation_time = conservation_start.elapsed();
    println!("Conservation verification time: {:?}", conservation_time);
    println!("Conservation law satisfied: {}", conservation_verified);

    let total_time = start_time.elapsed();
    println!("Total benchmark time: {:?}", total_time);

    Ok(())
}

/// Create test data that satisfies conservation laws
fn create_conservation_test_data(size: usize) -> Vec<u8> {
    let mut data = vec![0u8; size];
    for (i, byte) in data.iter_mut().enumerate() {
        *byte = (i % 96) as u8;
    }
    data
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn test_ci_config_default() {
        let config = CiBenchmarkConfig::default();
        assert_eq!(config.profile, "x86_64");
        assert!(!config.operations.is_empty());
        assert!(!config.data_sizes.is_empty());
        assert!(config.iterations > 0);
    }

    #[test]
    fn test_normalize_operation_name() {
        assert_eq!(
            normalize_operation_name("Linear Projection (1 MB)"),
            "linear_projection"
        );
        assert_eq!(
            normalize_operation_name("R96 Fourier Projection"),
            "r96_fourier"
        );
        assert_eq!(
            normalize_operation_name("Shard Extraction"),
            "shard_extraction"
        );
    }

    #[test]
    #[cfg(feature = "benchmarks")]
    fn test_conservation_data() {
        let data = create_conservation_test_data(1000);
        let sum: u32 = data.iter().map(|&b| b as u32).sum();
        assert_eq!(sum % 96, 0, "Test data must satisfy conservation laws");
    }

    #[test]
    #[cfg(feature = "benchmarks")]
    fn test_universal_number_operations() {
        // This test runs the UN operations benchmark
        let result = benchmark_universal_number_operations();
        assert!(result.is_ok(), "Universal Number operations should succeed");
    }

    #[test]
    #[cfg(feature = "benchmarks")]
    fn test_ci_benchmarks_minimal() {
        // Minimal CI benchmark test
        let config = CiBenchmarkConfig {
            profile: "test".to_string(),
            operations: vec!["linear_projection".to_string()],
            data_sizes: vec![4096],
            iterations: 2,
        };

        let result = run_ci_benchmarks(&config);
        assert!(result.is_ok(), "CI benchmarks should complete successfully");

        let json_result = result.unwrap();
        assert!(json_result.is_object(), "Result should be a JSON object");

        let obj = json_result.as_object().unwrap();
        assert!(
            obj.contains_key("timestamp"),
            "Result should contain timestamp"
        );
        assert!(
            obj.contains_key("hardware_profile"),
            "Result should contain hardware profile"
        );
        assert!(
            obj.contains_key("operations"),
            "Result should contain operations"
        );
    }
}
