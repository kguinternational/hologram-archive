/* benchmark_framework.c - Atlas Layer 4 Benchmark Framework Implementation
 * (c) 2024-2025 UOR Foundation - MIT License
 * 
 * Core benchmarking framework implementation with high-resolution timing,
 * memory tracking, statistical analysis, and conservation verification.
 */

#define _POSIX_C_SOURCE 199309L
#include "benchmark_framework.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <math.h>
#include <errno.h>
#include <assert.h>
#include <stdarg.h>
#include <sys/resource.h>

// =============================================================================
// Internal Constants and Macros
// =============================================================================

#define NSEC_PER_SEC 1000000000ULL
#define MAX_ERROR_MESSAGE_LENGTH 512

// Conservation law: sum of bytes mod 96 must equal 0
#define ATLAS_CONSERVATION_MODULUS 96

// =============================================================================
// Internal Global State
// =============================================================================

static bool g_framework_initialized = false;
static char g_last_error_message[MAX_ERROR_MESSAGE_LENGTH] = {0};

// =============================================================================
// Internal Helper Functions
// =============================================================================

/**
 * Set error message for framework error reporting.
 */
static void set_error_message(const char* format, ...) {
    va_list args;
    va_start(args, format);
    vsnprintf(g_last_error_message, sizeof(g_last_error_message), format, args);
    va_end(args);
}

/**
 * Calculate conservation checksum (sum mod 96).
 */
static uint8_t calculate_conservation_checksum(const void* data, size_t size) {
    const uint8_t* bytes = (const uint8_t*)data;
    uint64_t sum = 0;
    
    for (size_t i = 0; i < size; i++) {
        sum += bytes[i];
    }
    
    return (uint8_t)(sum % ATLAS_CONSERVATION_MODULUS);
}

/**
 * Compare function for sorting timing samples (for percentile calculation).
 */
static int compare_uint64(const void* a, const void* b) {
    uint64_t val_a = *(const uint64_t*)a;
    uint64_t val_b = *(const uint64_t*)b;
    
    if (val_a < val_b) return -1;
    if (val_a > val_b) return 1;
    return 0;
}

/**
 * Get high-resolution time difference in nanoseconds.
 */
static uint64_t timespec_diff_ns(const struct timespec* start, const struct timespec* end) {
    uint64_t start_ns = start->tv_sec * NSEC_PER_SEC + start->tv_nsec;
    uint64_t end_ns = end->tv_sec * NSEC_PER_SEC + end->tv_nsec;
    return end_ns - start_ns;
}

// =============================================================================
// Framework Management Functions
// =============================================================================

int benchmark_framework_init(void) {
    if (g_framework_initialized) {
        set_error_message("Benchmark framework already initialized");
        return -1;
    }
    
    // Clear any previous error messages
    g_last_error_message[0] = '\0';
    
    // Test high-resolution clock availability
    struct timespec ts;
    if (clock_gettime(CLOCK_MONOTONIC, &ts) != 0) {
        set_error_message("High-resolution clock not available: %s", strerror(errno));
        return -1;
    }
    
    g_framework_initialized = true;
    return 0;
}

void benchmark_framework_shutdown(void) {
    if (!g_framework_initialized) {
        return;
    }
    
    // Clear error state
    g_last_error_message[0] = '\0';
    g_framework_initialized = false;
}

benchmark_config_t benchmark_get_default_config(void) {
    benchmark_config_t config = {0};
    
    config.iterations = BENCHMARK_DEFAULT_ITERATIONS;
    config.warmup_iterations = BENCHMARK_DEFAULT_WARMUP_ITERATIONS;
    config.enable_memory_tracking = true;
    config.enable_conservation_checking = true;
    config.enable_detailed_timing = true;
    config.abort_on_failure = false;
    
    // Default percentiles: 50th, 90th, 95th, 99th
    config.percentiles[0] = 0.50;
    config.percentiles[1] = 0.90;
    config.percentiles[2] = 0.95;
    config.percentiles[3] = 0.99;
    config.percentile_count = 4;
    
    return config;
}

// =============================================================================
// Benchmark Context Management
// =============================================================================

benchmark_context_t* benchmark_context_create(const char* name,
                                             const char* description,
                                             const benchmark_config_t* config) {
    if (!g_framework_initialized) {
        set_error_message("Benchmark framework not initialized");
        return NULL;
    }
    
    if (!name) {
        set_error_message("Benchmark name cannot be NULL");
        return NULL;
    }
    
    // Allocate context structure
    benchmark_context_t* ctx = calloc(1, sizeof(benchmark_context_t));
    if (!ctx) {
        set_error_message("Failed to allocate benchmark context: %s", strerror(errno));
        return NULL;
    }
    
    // Use provided config or defaults
    if (config) {
        ctx->config = *config;
    } else {
        ctx->config = benchmark_get_default_config();
    }
    
    // Validate configuration
    if (ctx->config.iterations > BENCHMARK_MAX_ITERATIONS) {
        set_error_message("Too many iterations requested: %zu (max %d)", 
                         ctx->config.iterations, BENCHMARK_MAX_ITERATIONS);
        free(ctx);
        return NULL;
    }
    
    if (ctx->config.percentile_count > BENCHMARK_MAX_PERCENTILES) {
        set_error_message("Too many percentiles requested: %zu (max %d)",
                         ctx->config.percentile_count, BENCHMARK_MAX_PERCENTILES);
        free(ctx);
        return NULL;
    }
    
    // Allocate results structure
    ctx->results = calloc(1, sizeof(benchmark_results_t));
    if (!ctx->results) {
        set_error_message("Failed to allocate results structure: %s", strerror(errno));
        free(ctx);
        return NULL;
    }
    
    // Initialize results with metadata
    strncpy(ctx->results->name, name, BENCHMARK_MAX_NAME_LENGTH - 1);
    if (description) {
        strncpy(ctx->results->description, description, BENCHMARK_MAX_DESCRIPTION_LENGTH - 1);
    }
    
    // Allocate timing samples array
    size_t total_iterations = ctx->config.iterations + ctx->config.warmup_iterations;
    ctx->timing_sample_capacity = total_iterations;
    ctx->timing_samples = malloc(total_iterations * sizeof(uint64_t));
    if (!ctx->timing_samples) {
        set_error_message("Failed to allocate timing samples: %s", strerror(errno));
        free(ctx->results);
        free(ctx);
        return NULL;
    }
    
    return ctx;
}

void benchmark_context_destroy(benchmark_context_t* ctx) {
    if (!ctx) {
        return;
    }
    
    free(ctx->timing_samples);
    free(ctx->conservation_before);
    free(ctx->conservation_after);
    free(ctx->results);
    free(ctx);
}

// =============================================================================
// Timing Functions
// =============================================================================

uint64_t benchmark_get_timestamp_ns(void) {
    struct timespec ts;
    if (clock_gettime(CLOCK_MONOTONIC, &ts) != 0) {
        return 0;
    }
    return ts.tv_sec * NSEC_PER_SEC + ts.tv_nsec;
}

int benchmark_start_timer(benchmark_context_t* ctx) {
    if (!ctx || !ctx->in_benchmark) {
        set_error_message("Invalid context or not in benchmark");
        return -1;
    }
    
    if (clock_gettime(CLOCK_MONOTONIC, &ctx->start_time) != 0) {
        set_error_message("Failed to get start time: %s", strerror(errno));
        return -1;
    }
    
    return 0;
}

uint64_t benchmark_stop_timer(benchmark_context_t* ctx) {
    if (!ctx || !ctx->in_benchmark) {
        set_error_message("Invalid context or not in benchmark");
        return 0;
    }
    
    if (clock_gettime(CLOCK_MONOTONIC, &ctx->end_time) != 0) {
        set_error_message("Failed to get end time: %s", strerror(errno));
        return 0;
    }
    
    uint64_t elapsed_ns = timespec_diff_ns(&ctx->start_time, &ctx->end_time);
    
    // Record timing sample if we have capacity and detailed timing is enabled
    if (ctx->config.enable_detailed_timing && 
        ctx->timing_sample_count < ctx->timing_sample_capacity) {
        ctx->timing_samples[ctx->timing_sample_count++] = elapsed_ns;
    }
    
    return elapsed_ns;
}

// =============================================================================
// Memory Tracking Functions
// =============================================================================

int benchmark_record_allocation(benchmark_context_t* ctx, size_t bytes) {
    if (!ctx || !ctx->config.enable_memory_tracking) {
        return 0;
    }
    
    ctx->memory_stats.allocated_bytes += bytes;
    ctx->memory_stats.current_allocated += bytes;
    ctx->memory_stats.allocation_count++;
    
    if (ctx->memory_stats.current_allocated > ctx->memory_stats.peak_allocated_bytes) {
        ctx->memory_stats.peak_allocated_bytes = ctx->memory_stats.current_allocated;
    }
    
    return 0;
}

int benchmark_record_deallocation(benchmark_context_t* ctx, size_t bytes) {
    if (!ctx || !ctx->config.enable_memory_tracking) {
        return 0;
    }
    
    if (bytes > ctx->memory_stats.current_allocated) {
        set_error_message("Deallocation exceeds current allocated memory");
        return -1;
    }
    
    ctx->memory_stats.current_allocated -= bytes;
    ctx->memory_stats.deallocation_count++;
    
    return 0;
}

const benchmark_memory_stats_t* benchmark_get_memory_stats(const benchmark_context_t* ctx) {
    if (!ctx) {
        return NULL;
    }
    
    return &ctx->memory_stats;
}

// =============================================================================
// Conservation Verification Functions
// =============================================================================

int benchmark_conservation_setup(benchmark_context_t* ctx, 
                                const void* memory, size_t size) {
    if (!ctx || !memory || size == 0) {
        set_error_message("Invalid arguments for conservation setup");
        return -1;
    }
    
    if (!ctx->config.enable_conservation_checking) {
        return 0;
    }
    
    // Allocate or reallocate conservation memory buffers
    if (ctx->conservation_size != size) {
        free(ctx->conservation_before);
        free(ctx->conservation_after);
        
        ctx->conservation_before = malloc(size);
        ctx->conservation_after = malloc(size);
        
        if (!ctx->conservation_before || !ctx->conservation_after) {
            set_error_message("Failed to allocate conservation buffers: %s", strerror(errno));
            free(ctx->conservation_before);
            free(ctx->conservation_after);
            ctx->conservation_before = NULL;
            ctx->conservation_after = NULL;
            ctx->conservation_size = 0;
            return -1;
        }
        
        ctx->conservation_size = size;
    }
    
    // Copy initial memory state
    memcpy(ctx->conservation_before, memory, size);
    
    return 0;
}

int benchmark_conservation_verify(benchmark_context_t* ctx,
                                 const void* memory, size_t size) {
    if (!ctx || !memory || size == 0) {
        set_error_message("Invalid arguments for conservation verification");
        return -1;
    }
    
    if (!ctx->config.enable_conservation_checking || 
        !ctx->conservation_before || ctx->conservation_size != size) {
        return 0;
    }
    
    // Copy final memory state
    memcpy(ctx->conservation_after, memory, size);
    
    // Calculate conservation checksums
    uint8_t before_sum = calculate_conservation_checksum(ctx->conservation_before, size);
    uint8_t after_sum = calculate_conservation_checksum(ctx->conservation_after, size);
    
    // Update conservation statistics
    benchmark_conservation_stats_t* stats = &ctx->results->conservation;
    stats->total_checks++;
    
    // Calculate conservation delta
    uint8_t delta = (after_sum >= before_sum) ? 
                   (after_sum - before_sum) : 
                   (ATLAS_CONSERVATION_MODULUS - before_sum + after_sum);
    
    stats->total_delta += delta;
    if (delta > stats->max_delta) {
        stats->max_delta = delta;
    }
    
    // Check conservation law compliance
    if (after_sum == 0) {  // Conservation law: sum mod 96 == 0
        stats->passed_checks++;
        return 0;
    } else {
        stats->failed_checks++;
        stats->conservation_maintained = false;
        
        if (ctx->config.abort_on_failure) {
            set_error_message("Conservation law violated: checksum = %u", after_sum);
            return -1;
        }
        
        return -1;
    }
}

const benchmark_conservation_stats_t* benchmark_get_conservation_stats(const benchmark_context_t* ctx) {
    if (!ctx) {
        return NULL;
    }
    
    return &ctx->results->conservation;
}

// =============================================================================
// Statistical Analysis Functions
// =============================================================================

int benchmark_calculate_timing_stats(const uint64_t* samples, size_t count,
                                    benchmark_timing_stats_t* stats,
                                    const double* percentiles, size_t percentile_count) {
    if (!samples || !stats || count == 0) {
        set_error_message("Invalid arguments for timing statistics calculation");
        return -1;
    }
    
    // Make a copy of samples for sorting (for percentiles)
    uint64_t* sorted_samples = malloc(count * sizeof(uint64_t));
    if (!sorted_samples) {
        set_error_message("Failed to allocate sorted samples array: %s", strerror(errno));
        return -1;
    }
    memcpy(sorted_samples, samples, count * sizeof(uint64_t));
    qsort(sorted_samples, count, sizeof(uint64_t), compare_uint64);
    
    // Calculate basic statistics
    stats->min_ns = sorted_samples[0];
    stats->max_ns = sorted_samples[count - 1];
    stats->median_ns = sorted_samples[count / 2];
    
    // Calculate mean and total
    uint64_t sum = 0;
    for (size_t i = 0; i < count; i++) {
        sum += samples[i];
    }
    stats->mean_ns = sum / count;
    stats->total_ns = sum;
    
    // Calculate standard deviation
    double variance = 0.0;
    for (size_t i = 0; i < count; i++) {
        double diff = (double)samples[i] - (double)stats->mean_ns;
        variance += diff * diff;
    }
    stats->stddev_ns = sqrt(variance / count);
    
    // Calculate requested percentiles
    size_t max_percentiles = BENCHMARK_MAX_PERCENTILES;
    if (percentile_count < max_percentiles) {
        max_percentiles = percentile_count;
    }
    
    for (size_t i = 0; i < max_percentiles && i < percentile_count; i++) {
        if (percentiles[i] >= 0.0 && percentiles[i] <= 1.0) {
            size_t index = (size_t)(percentiles[i] * (count - 1));
            stats->percentiles[i] = sorted_samples[index];
        } else {
            stats->percentiles[i] = 0;
        }
    }
    stats->percentile_count = max_percentiles;
    
    free(sorted_samples);
    return 0;
}

double benchmark_compare_performance(const benchmark_results_t* baseline,
                                   const benchmark_results_t* current) {
    if (!baseline || !current || baseline->timing.mean_ns == 0) {
        return 0.0;
    }
    
    return (double)current->timing.mean_ns / (double)baseline->timing.mean_ns;
}

bool benchmark_detect_regression(const benchmark_results_t* baseline,
                                const benchmark_results_t* current,
                                double threshold) {
    double ratio = benchmark_compare_performance(baseline, current);
    return ratio > threshold;
}

// =============================================================================
// Main Benchmark Execution Function
// =============================================================================

const benchmark_results_t* benchmark_run(benchmark_context_t* ctx,
                                        benchmark_func_t bench_func,
                                        benchmark_setup_func_t setup_func,
                                        benchmark_cleanup_func_t cleanup_func,
                                        void* user_data) {
    if (!ctx || !bench_func) {
        set_error_message("Invalid context or benchmark function");
        return NULL;
    }
    
    // Record start timestamp
    ctx->results->timestamp_start_ns = benchmark_get_timestamp_ns();
    
    // Initialize conservation tracking
    ctx->results->conservation.conservation_maintained = true;
    
    // Run warmup iterations
    ctx->in_benchmark = true;
    for (size_t i = 0; i < ctx->config.warmup_iterations; i++) {
        ctx->benchmark_failed = false;
        
        if (setup_func && setup_func(ctx, user_data) != 0) {
            continue;
        }
        
        int result = bench_func(ctx, user_data);
        
        if (cleanup_func) {
            cleanup_func(ctx, user_data);
        }
        
        // Don't count warmup failures in statistics
        if (result != 0 || ctx->benchmark_failed) {
            continue;
        }
    }
    
    // Reset timing sample count after warmup
    ctx->timing_sample_count = 0;
    
    // Run actual benchmark iterations
    for (size_t i = 0; i < ctx->config.iterations; i++) {
        ctx->benchmark_failed = false;
        
        // Run setup function if provided
        if (setup_func && setup_func(ctx, user_data) != 0) {
            ctx->results->iterations_failed++;
            if (ctx->config.abort_on_failure) {
                break;
            }
            continue;
        }
        
        // Execute benchmark function
        int result = bench_func(ctx, user_data);
        
        // Run cleanup function if provided
        if (cleanup_func) {
            cleanup_func(ctx, user_data);
        }
        
        // Check iteration result
        if (result != 0 || ctx->benchmark_failed) {
            ctx->results->iterations_failed++;
            if (ctx->config.abort_on_failure) {
                break;
            }
        } else {
            ctx->results->iterations_completed++;
        }
    }
    
    ctx->in_benchmark = false;
    
    // Calculate timing statistics
    if (ctx->config.enable_detailed_timing && ctx->timing_sample_count > 0) {
        benchmark_calculate_timing_stats(
            ctx->timing_samples, 
            ctx->timing_sample_count,
            &ctx->results->timing,
            ctx->config.percentiles,
            ctx->config.percentile_count
        );
    }
    
    // Copy memory statistics
    ctx->results->memory = ctx->memory_stats;
    
    // Record end timestamp
    ctx->results->timestamp_end_ns = benchmark_get_timestamp_ns();
    
    // Set success flag
    ctx->results->success = (ctx->results->iterations_failed == 0);
    
    return ctx->results;
}

// =============================================================================
// Error Handling Functions
// =============================================================================

void benchmark_mark_failed(benchmark_context_t* ctx, const char* error_message) {
    if (!ctx) {
        return;
    }
    
    ctx->benchmark_failed = true;
    
    if (error_message) {
        set_error_message("Benchmark iteration failed: %s", error_message);
    }
}

bool benchmark_is_failed(const benchmark_context_t* ctx) {
    return ctx ? ctx->benchmark_failed : true;
}

const char* benchmark_get_last_error(void) {
    return g_last_error_message[0] != '\0' ? g_last_error_message : NULL;
}