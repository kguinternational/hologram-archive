/* benchmark_framework.h - Atlas Layer 4 Benchmark Framework
 * (c) 2024-2025 UOR Foundation - MIT License
 * 
 * Core benchmarking framework for Atlas Layer 4 (Manifold) operations providing:
 * - High-resolution timing (nanosecond precision)
 * - Memory usage tracking
 * - Statistical analysis (mean, stddev, percentiles)
 * - Warmup runs and multiple iterations
 * - Conservation verification for all operations
 */

#ifndef ATLAS_BENCHMARK_FRAMEWORK_H
#define ATLAS_BENCHMARK_FRAMEWORK_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <time.h>

#ifdef __cplusplus
extern "C" {
#endif

// =============================================================================
// Configuration Constants
// =============================================================================

#define BENCHMARK_MAX_NAME_LENGTH 128
#define BENCHMARK_MAX_DESCRIPTION_LENGTH 256
#define BENCHMARK_DEFAULT_ITERATIONS 1000
#define BENCHMARK_DEFAULT_WARMUP_ITERATIONS 100
#define BENCHMARK_MAX_ITERATIONS 100000
#define BENCHMARK_MAX_PERCENTILES 10

// =============================================================================
// Type Definitions
// =============================================================================

/* Forward declaration for benchmark context */
struct benchmark_context;
typedef struct benchmark_context benchmark_context_t;

/* Benchmark function signature */
typedef int (*benchmark_func_t)(benchmark_context_t* ctx, void* user_data);

/* Setup function signature (called before each iteration) */
typedef int (*benchmark_setup_func_t)(benchmark_context_t* ctx, void* user_data);

/* Cleanup function signature (called after each iteration) */
typedef int (*benchmark_cleanup_func_t)(benchmark_context_t* ctx, void* user_data);

/* Memory usage statistics */
typedef struct {
    size_t allocated_bytes;      /* Total bytes allocated */
    size_t peak_allocated_bytes; /* Peak memory usage */
    size_t allocation_count;     /* Number of allocations */
    size_t deallocation_count;   /* Number of deallocations */
    size_t current_allocated;    /* Currently allocated bytes */
} benchmark_memory_stats_t;

/* Timing statistics */
typedef struct {
    uint64_t min_ns;            /* Minimum execution time (nanoseconds) */
    uint64_t max_ns;            /* Maximum execution time (nanoseconds) */
    uint64_t mean_ns;           /* Mean execution time (nanoseconds) */
    uint64_t median_ns;         /* Median execution time (nanoseconds) */
    double stddev_ns;           /* Standard deviation (nanoseconds) */
    uint64_t total_ns;          /* Total execution time (nanoseconds) */
    uint64_t percentiles[BENCHMARK_MAX_PERCENTILES]; /* Percentile values */
    size_t percentile_count;    /* Number of valid percentiles */
} benchmark_timing_stats_t;

/* Conservation verification statistics */
typedef struct {
    size_t total_checks;        /* Total conservation checks performed */
    size_t passed_checks;       /* Number of checks that passed */
    size_t failed_checks;       /* Number of checks that failed */
    uint64_t total_delta;       /* Sum of all conservation deltas */
    uint64_t max_delta;         /* Maximum conservation delta observed */
    bool conservation_maintained; /* Overall conservation status */
} benchmark_conservation_stats_t;

/* Complete benchmark results */
typedef struct {
    char name[BENCHMARK_MAX_NAME_LENGTH];
    char description[BENCHMARK_MAX_DESCRIPTION_LENGTH];
    size_t iterations_completed;
    size_t iterations_failed;
    benchmark_timing_stats_t timing;
    benchmark_memory_stats_t memory;
    benchmark_conservation_stats_t conservation;
    bool success;
    uint64_t timestamp_start_ns;
    uint64_t timestamp_end_ns;
} benchmark_results_t;

/* Benchmark configuration */
typedef struct {
    size_t iterations;          /* Number of iterations to run */
    size_t warmup_iterations;   /* Number of warmup iterations */
    bool enable_memory_tracking; /* Enable memory usage tracking */
    bool enable_conservation_checking; /* Enable conservation verification */
    bool enable_detailed_timing; /* Collect detailed timing statistics */
    double percentiles[BENCHMARK_MAX_PERCENTILES]; /* Percentiles to calculate */
    size_t percentile_count;    /* Number of percentiles requested */
    bool abort_on_failure;      /* Stop benchmark on first failure */
} benchmark_config_t;

/* Benchmark context - opaque structure for benchmark execution */
struct benchmark_context {
    benchmark_config_t config;
    benchmark_results_t* results;
    benchmark_memory_stats_t memory_stats;
    uint64_t* timing_samples;   /* Array of timing samples */
    size_t timing_sample_count; /* Number of timing samples collected */
    size_t timing_sample_capacity; /* Capacity of timing samples array */
    void* conservation_before;  /* Memory state before operation */
    void* conservation_after;   /* Memory state after operation */
    size_t conservation_size;   /* Size of conservation memory region */
    struct timespec start_time; /* High-resolution start time */
    struct timespec end_time;   /* High-resolution end time */
    bool in_benchmark;          /* True if currently executing benchmark */
    bool benchmark_failed;      /* True if current iteration failed */
};

// =============================================================================
// Framework Management Functions
// =============================================================================

/**
 * Initialize benchmark framework and allocate resources.
 * 
 * Must be called before any other benchmark functions.
 * Sets up global state and initializes timing subsystems.
 * 
 * @return 0 on success, -1 on error
 * 
 * Thread safety: Not thread-safe, call once at program startup
 */
int benchmark_framework_init(void);

/**
 * Shutdown benchmark framework and free resources.
 * 
 * Should be called at program exit to clean up framework resources.
 * 
 * Thread safety: Not thread-safe, call once at program shutdown
 */
void benchmark_framework_shutdown(void);

/**
 * Get default benchmark configuration.
 * 
 * Returns a configuration structure with sensible defaults that can
 * be modified for specific benchmark requirements.
 * 
 * @return Default configuration structure
 */
benchmark_config_t benchmark_get_default_config(void);

// =============================================================================
// Benchmark Execution Functions
// =============================================================================

/**
 * Create a new benchmark context with specified configuration.
 * 
 * @param name Benchmark name (will be truncated if too long)
 * @param description Benchmark description (will be truncated if too long)
 * @param config Benchmark configuration (or NULL for defaults)
 * @return New benchmark context, or NULL on error
 * 
 * Memory: Caller must call benchmark_context_destroy() to free
 */
benchmark_context_t* benchmark_context_create(const char* name,
                                             const char* description,
                                             const benchmark_config_t* config);

/**
 * Run a benchmark with the specified context and functions.
 * 
 * Executes the benchmark according to the context configuration,
 * including warmup iterations, timing collection, and statistics.
 * 
 * @param ctx Benchmark context (must be valid)
 * @param bench_func Main benchmark function to execute
 * @param setup_func Setup function (called before each iteration, can be NULL)
 * @param cleanup_func Cleanup function (called after each iteration, can be NULL)
 * @param user_data User data passed to all functions
 * @return Pointer to results structure, or NULL on error
 * 
 * The returned results pointer is owned by the context and remains
 * valid until benchmark_context_destroy() is called.
 */
const benchmark_results_t* benchmark_run(benchmark_context_t* ctx,
                                        benchmark_func_t bench_func,
                                        benchmark_setup_func_t setup_func,
                                        benchmark_cleanup_func_t cleanup_func,
                                        void* user_data);

/**
 * Destroy benchmark context and free all resources.
 * 
 * @param ctx Benchmark context (can be NULL - no-op)
 */
void benchmark_context_destroy(benchmark_context_t* ctx);

// =============================================================================
// Timing Functions (for use within benchmark functions)
// =============================================================================

/**
 * Start timing measurement for current benchmark iteration.
 * 
 * Records high-resolution start time. Must be called within a benchmark
 * function before the operation being measured.
 * 
 * @param ctx Benchmark context
 * @return 0 on success, -1 on error
 */
int benchmark_start_timer(benchmark_context_t* ctx);

/**
 * Stop timing measurement and record elapsed time.
 * 
 * Records high-resolution end time and adds elapsed time to statistics.
 * Must be called after benchmark_start_timer() within the same iteration.
 * 
 * @param ctx Benchmark context
 * @return Elapsed time in nanoseconds, or 0 on error
 */
uint64_t benchmark_stop_timer(benchmark_context_t* ctx);

/**
 * Get high-resolution timestamp in nanoseconds.
 * 
 * @return Current timestamp in nanoseconds since epoch
 */
uint64_t benchmark_get_timestamp_ns(void);

// =============================================================================
// Memory Tracking Functions
// =============================================================================

/**
 * Record memory allocation for tracking.
 * 
 * Call this function whenever allocating memory during a benchmark
 * to track memory usage statistics.
 * 
 * @param ctx Benchmark context
 * @param bytes Number of bytes allocated
 * @return 0 on success, -1 on error
 */
int benchmark_record_allocation(benchmark_context_t* ctx, size_t bytes);

/**
 * Record memory deallocation for tracking.
 * 
 * Call this function whenever freeing memory during a benchmark
 * to track memory usage statistics.
 * 
 * @param ctx Benchmark context  
 * @param bytes Number of bytes deallocated
 * @return 0 on success, -1 on error
 */
int benchmark_record_deallocation(benchmark_context_t* ctx, size_t bytes);

/**
 * Get current memory usage statistics.
 * 
 * @param ctx Benchmark context
 * @return Pointer to current memory statistics
 */
const benchmark_memory_stats_t* benchmark_get_memory_stats(const benchmark_context_t* ctx);

// =============================================================================
// Conservation Verification Functions
// =============================================================================

/**
 * Set up conservation verification for benchmark iteration.
 * 
 * Captures the initial memory state for conservation checking.
 * Must be called before the benchmarked operation modifies memory.
 * 
 * @param ctx Benchmark context
 * @param memory Pointer to memory region to monitor
 * @param size Size of memory region in bytes
 * @return 0 on success, -1 on error
 */
int benchmark_conservation_setup(benchmark_context_t* ctx, 
                                const void* memory, size_t size);

/**
 * Verify conservation law compliance after benchmark iteration.
 * 
 * Checks that the memory region satisfies Atlas conservation laws
 * (sum of bytes mod 96 == 0) and records the results.
 * 
 * @param ctx Benchmark context
 * @param memory Pointer to memory region to verify
 * @param size Size of memory region in bytes
 * @return 0 if conservation maintained, -1 if violated
 */
int benchmark_conservation_verify(benchmark_context_t* ctx,
                                 const void* memory, size_t size);

/**
 * Get current conservation statistics.
 * 
 * @param ctx Benchmark context
 * @return Pointer to current conservation statistics
 */
const benchmark_conservation_stats_t* benchmark_get_conservation_stats(const benchmark_context_t* ctx);

// =============================================================================
// Statistical Analysis Functions  
// =============================================================================

/**
 * Calculate statistical measures from timing samples.
 * 
 * Computes mean, median, standard deviation, and percentiles from
 * collected timing samples. Called automatically by benchmark_run().
 * 
 * @param samples Array of timing samples (nanoseconds)
 * @param count Number of samples
 * @param stats Output structure for calculated statistics
 * @param percentiles Array of percentile values to calculate (0.0-1.0)
 * @param percentile_count Number of percentiles requested
 * @return 0 on success, -1 on error
 */
int benchmark_calculate_timing_stats(const uint64_t* samples, size_t count,
                                    benchmark_timing_stats_t* stats,
                                    const double* percentiles, size_t percentile_count);

/**
 * Compare two benchmark results for performance regression detection.
 * 
 * Compares timing statistics between baseline and current results,
 * returning the performance change ratio.
 * 
 * @param baseline Baseline benchmark results
 * @param current Current benchmark results  
 * @return Performance ratio (>1.0 = slower, <1.0 = faster, 0.0 = error)
 */
double benchmark_compare_performance(const benchmark_results_t* baseline,
                                   const benchmark_results_t* current);

/**
 * Detect performance regression based on threshold.
 * 
 * @param baseline Baseline benchmark results
 * @param current Current benchmark results
 * @param threshold Regression threshold (e.g., 1.1 for 10% slower)
 * @return true if regression detected, false otherwise
 */
bool benchmark_detect_regression(const benchmark_results_t* baseline,
                                const benchmark_results_t* current,
                                double threshold);

// =============================================================================
// Error Handling Functions
// =============================================================================

/**
 * Mark current benchmark iteration as failed.
 * 
 * Call this function from within a benchmark function if the
 * operation fails and the iteration should not be counted.
 * 
 * @param ctx Benchmark context
 * @param error_message Optional error message (can be NULL)
 */
void benchmark_mark_failed(benchmark_context_t* ctx, const char* error_message);

/**
 * Check if current benchmark iteration has been marked as failed.
 * 
 * @param ctx Benchmark context
 * @return true if iteration failed, false otherwise
 */
bool benchmark_is_failed(const benchmark_context_t* ctx);

/**
 * Get last benchmark framework error message.
 * 
 * @return Pointer to static error message string, or NULL if no error
 */
const char* benchmark_get_last_error(void);

#ifdef __cplusplus
}
#endif

#endif /* ATLAS_BENCHMARK_FRAMEWORK_H */