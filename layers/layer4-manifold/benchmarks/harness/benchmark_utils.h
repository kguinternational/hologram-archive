/* benchmark_utils.h - Atlas Layer 4 Benchmark Utilities
 * (c) 2024-2025 UOR Foundation - MIT License
 * 
 * Utility functions for Atlas Layer 4 benchmarking providing:
 * - Random data generation with conservation compliance
 * - Conservation checking and validation
 * - Result formatting and reporting (console, CSV, JSON)
 * - Benchmark comparison and analysis
 */

#ifndef ATLAS_BENCHMARK_UTILS_H
#define ATLAS_BENCHMARK_UTILS_H

#include "benchmark_framework.h"
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>

#ifdef __cplusplus
extern "C" {
#endif

// =============================================================================
// Configuration Constants
// =============================================================================

#define BENCHMARK_MAX_REPORT_LENGTH 8192
#define BENCHMARK_MAX_CSV_LINE_LENGTH 1024
#define BENCHMARK_MAX_JSON_LENGTH 16384
#define BENCHMARK_RANDOM_SEED_DEFAULT 42

// =============================================================================
// Data Generation Functions
// =============================================================================

/**
 * Generate random data buffer that satisfies conservation laws.
 * 
 * Creates a buffer filled with pseudo-random data that has a byte sum
 * modulo 96 equal to zero, satisfying Atlas conservation requirements.
 * 
 * @param buffer Output buffer (must be pre-allocated)
 * @param size Size of buffer in bytes (must be > 0)
 * @param seed Random seed for reproducible generation
 * @return 0 on success, -1 on error
 * 
 * Thread safety: Safe to call from multiple threads with different buffers
 */
int benchmark_generate_conserved_data(uint8_t* buffer, size_t size, uint32_t seed);

// Forward declarations for temporary types (until Atlas types are available)
typedef struct {
    uint32_t start_coord;
    uint32_t end_coord; 
    uint16_t page_count;
    uint8_t region_class;
    bool is_conserved;
} temp_boundary_region_t;

typedef struct {
    double scaling_factor;
    double rotation_angle;
    double translation_x;
    double translation_y;  
} temp_transform_params_t;

/**
 * Generate random Atlas boundary region for testing.
 * 
 * Creates a valid boundary region structure with random but mathematically
 * consistent parameters for use in benchmark testing.
 * 
 * @param region Output region structure
 * @param seed Random seed for reproducible generation
 * @return 0 on success, -1 on error
 */
int benchmark_generate_boundary_region(temp_boundary_region_t* region, uint32_t seed);

/**
 * Generate random transformation parameters for testing.
 * 
 * Creates valid transformation parameters with reasonable ranges
 * suitable for benchmark testing without causing numerical instabilities.
 * 
 * @param params Output transformation parameters
 * @param seed Random seed for reproducible generation
 * @return 0 on success, -1 on error
 */
int benchmark_generate_transform_params(temp_transform_params_t* params, uint32_t seed);

/**
 * Generate test data with specific conservation properties.
 * 
 * Creates data with a known conservation checksum for testing
 * conservation verification functions.
 * 
 * @param buffer Output buffer (must be pre-allocated)
 * @param size Size of buffer in bytes
 * @param target_checksum Desired conservation checksum (0-95)
 * @param seed Random seed for data generation
 * @return 0 on success, -1 on error
 */
int benchmark_generate_data_with_checksum(uint8_t* buffer, size_t size, 
                                         uint8_t target_checksum, uint32_t seed);

// =============================================================================
// Conservation Verification Functions
// =============================================================================

/**
 * Verify that data buffer satisfies Atlas conservation laws.
 * 
 * Checks that the sum of all bytes in the buffer modulo 96 equals zero.
 * This is the fundamental conservation law for Atlas computational space.
 * 
 * @param data Pointer to data buffer
 * @param size Size of buffer in bytes
 * @return true if conservation law satisfied, false otherwise
 * 
 * Thread safety: Safe to call from multiple threads (read-only)
 */
bool benchmark_verify_conservation(const uint8_t* data, size_t size);

/**
 * Calculate conservation checksum for data buffer.
 * 
 * Computes the sum of all bytes modulo 96. A result of 0 indicates
 * the data satisfies Atlas conservation laws.
 * 
 * @param data Pointer to data buffer
 * @param size Size of buffer in bytes
 * @return Conservation checksum (0-95)
 * 
 * Thread safety: Safe to call from multiple threads (read-only)
 */
uint8_t benchmark_calculate_checksum(const uint8_t* data, size_t size);

/**
 * Generate conservation witness for data buffer.
 * 
 * Creates a cryptographic witness that can be used to verify the
 * conservation properties of the data at a later time.
 * 
 * @param data Pointer to data buffer
 * @param size Size of buffer in bytes
 * @param witness Output buffer for witness (must be at least 32 bytes)
 * @return 0 on success, -1 on error
 * 
 * Thread safety: Safe to call from multiple threads
 */
int benchmark_generate_conservation_witness(const uint8_t* data, size_t size, 
                                           uint8_t witness[32]);

/**
 * Verify data against conservation witness.
 * 
 * Checks that the provided data matches the conservation witness,
 * confirming both data integrity and conservation compliance.
 * 
 * @param data Pointer to data buffer
 * @param size Size of buffer in bytes
 * @param witness Witness data (32 bytes)
 * @return true if verification succeeds, false otherwise
 * 
 * Thread safety: Safe to call from multiple threads (read-only)
 */
bool benchmark_verify_conservation_witness(const uint8_t* data, size_t size,
                                          const uint8_t witness[32]);

// =============================================================================
// Result Formatting Functions
// =============================================================================

/**
 * Format benchmark results as human-readable text report.
 * 
 * Creates a detailed text report with timing statistics, memory usage,
 * conservation verification results, and performance analysis.
 * 
 * @param results Benchmark results to format
 * @param buffer Output buffer for formatted report
 * @param buffer_size Size of output buffer
 * @return Number of characters written, or -1 on error
 * 
 * Thread safety: Safe to call from multiple threads with different buffers
 */
int benchmark_format_text_report(const benchmark_results_t* results, 
                                char* buffer, size_t buffer_size);

/**
 * Format benchmark results as CSV line.
 * 
 * Creates a comma-separated values line suitable for spreadsheet analysis.
 * Includes all key metrics in a standardized format.
 * 
 * @param results Benchmark results to format
 * @param buffer Output buffer for CSV line
 * @param buffer_size Size of output buffer
 * @return Number of characters written, or -1 on error
 * 
 * Thread safety: Safe to call from multiple threads with different buffers
 */
int benchmark_format_csv_line(const benchmark_results_t* results,
                             char* buffer, size_t buffer_size);

/**
 * Generate CSV header line for benchmark results.
 * 
 * Creates the header row for CSV output that matches the format
 * produced by benchmark_format_csv_line().
 * 
 * @param buffer Output buffer for CSV header
 * @param buffer_size Size of output buffer
 * @return Number of characters written, or -1 on error
 */
int benchmark_format_csv_header(char* buffer, size_t buffer_size);

/**
 * Format benchmark results as JSON object.
 * 
 * Creates a JSON representation of the benchmark results suitable
 * for programmatic analysis and web-based reporting.
 * 
 * @param results Benchmark results to format
 * @param buffer Output buffer for JSON object
 * @param buffer_size Size of output buffer
 * @param pretty_print If true, format with indentation for readability
 * @return Number of characters written, or -1 on error
 * 
 * Thread safety: Safe to call from multiple threads with different buffers
 */
int benchmark_format_json(const benchmark_results_t* results,
                         char* buffer, size_t buffer_size, bool pretty_print);

// =============================================================================
// File Output Functions
// =============================================================================

/**
 * Write benchmark results to text file.
 * 
 * Writes a detailed text report to the specified file, creating the file
 * if it doesn't exist or appending to it if it does.
 * 
 * @param results Benchmark results to write
 * @param filename Path to output file
 * @param append If true, append to file; if false, overwrite file
 * @return 0 on success, -1 on error
 * 
 * Thread safety: Not thread-safe (file I/O)
 */
int benchmark_write_text_report(const benchmark_results_t* results,
                               const char* filename, bool append);

/**
 * Write benchmark results to CSV file.
 * 
 * Writes results in CSV format, automatically adding headers if the file
 * is new or empty. Suitable for importing into spreadsheet applications.
 * 
 * @param results Benchmark results to write
 * @param filename Path to output CSV file
 * @param write_header If true, write CSV header row (for new files)
 * @return 0 on success, -1 on error
 * 
 * Thread safety: Not thread-safe (file I/O)
 */
int benchmark_write_csv_report(const benchmark_results_t* results,
                              const char* filename, bool write_header);

/**
 * Write benchmark results to JSON file.
 * 
 * Writes results as a JSON object to the specified file. If the file
 * exists and contains valid JSON, appends to an array structure.
 * 
 * @param results Benchmark results to write
 * @param filename Path to output JSON file
 * @param pretty_print If true, format with indentation
 * @return 0 on success, -1 on error
 * 
 * Thread safety: Not thread-safe (file I/O)
 */
int benchmark_write_json_report(const benchmark_results_t* results,
                               const char* filename, bool pretty_print);

// =============================================================================
// Analysis and Comparison Functions
// =============================================================================

/**
 * Compare two benchmark results and generate analysis report.
 * 
 * Performs detailed comparison between baseline and current results,
 * identifying performance regressions, improvements, and statistical
 * significance of changes.
 * 
 * @param baseline Baseline benchmark results
 * @param current Current benchmark results
 * @param buffer Output buffer for analysis report
 * @param buffer_size Size of output buffer
 * @return Number of characters written, or -1 on error
 * 
 * Thread safety: Safe to call from multiple threads with different buffers
 */
int benchmark_compare_results(const benchmark_results_t* baseline,
                             const benchmark_results_t* current,
                             char* buffer, size_t buffer_size);

/**
 * Calculate performance regression statistics.
 * 
 * Analyzes timing differences between baseline and current results,
 * computing regression ratios and statistical significance.
 * 
 * @param baseline Baseline benchmark results
 * @param current Current benchmark results
 * @param regression_ratio Output parameter for regression ratio (>1.0 = slower)
 * @param confidence_level Output parameter for statistical confidence (0.0-1.0)
 * @return 0 on success, -1 on error
 */
int benchmark_calculate_regression(const benchmark_results_t* baseline,
                                  const benchmark_results_t* current,
                                  double* regression_ratio,
                                  double* confidence_level);

/**
 * Generate performance summary across multiple benchmark results.
 * 
 * Creates a summary report analyzing trends across multiple benchmark
 * runs, identifying best/worst performance and stability metrics.
 * 
 * @param results Array of benchmark results
 * @param result_count Number of results in array
 * @param buffer Output buffer for summary report
 * @param buffer_size Size of output buffer
 * @return Number of characters written, or -1 on error
 * 
 * Thread safety: Safe to call from multiple threads with different buffers
 */
int benchmark_generate_summary(const benchmark_results_t* results,
                              size_t result_count,
                              char* buffer, size_t buffer_size);

// =============================================================================
// Console Output Functions
// =============================================================================

/**
 * Print benchmark results to console with color formatting.
 * 
 * Displays results in a human-readable format with color coding for
 * easy identification of performance issues and conservation violations.
 * 
 * @param results Benchmark results to display
 * @param use_colors If true, use ANSI color codes for formatting
 * 
 * Thread safety: Not thread-safe (console output)
 */
void benchmark_print_results(const benchmark_results_t* results, bool use_colors);

/**
 * Print benchmark comparison to console with highlighting.
 * 
 * Displays side-by-side comparison of two benchmark results with
 * highlighting for regressions and improvements.
 * 
 * @param baseline Baseline benchmark results  
 * @param current Current benchmark results
 * @param use_colors If true, use ANSI color codes for highlighting
 * 
 * Thread safety: Not thread-safe (console output)
 */
void benchmark_print_comparison(const benchmark_results_t* baseline,
                               const benchmark_results_t* current,
                               bool use_colors);

/**
 * Print progress indicator for long-running benchmarks.
 * 
 * Displays a progress bar and estimated time remaining for benchmarks
 * with many iterations.
 * 
 * @param current_iteration Current iteration number
 * @param total_iterations Total number of iterations
 * @param elapsed_time_ns Time elapsed so far (nanoseconds)
 * 
 * Thread safety: Not thread-safe (console output)
 */
void benchmark_print_progress(size_t current_iteration, size_t total_iterations,
                             uint64_t elapsed_time_ns);

// =============================================================================
// Utility Helper Functions
// =============================================================================

/**
 * Format time duration in human-readable form.
 * 
 * Converts nanosecond duration to appropriate units (ns, μs, ms, s)
 * with proper precision and unit labels.
 * 
 * @param nanoseconds Duration in nanoseconds
 * @param buffer Output buffer for formatted string
 * @param buffer_size Size of output buffer
 * @return Number of characters written, or -1 on error
 */
int benchmark_format_duration(uint64_t nanoseconds, char* buffer, size_t buffer_size);

/**
 * Format memory size in human-readable form.
 * 
 * Converts byte count to appropriate units (B, KB, MB, GB) with
 * proper precision and unit labels.
 * 
 * @param bytes Memory size in bytes
 * @param buffer Output buffer for formatted string
 * @param buffer_size Size of output buffer
 * @return Number of characters written, or -1 on error
 */
int benchmark_format_memory_size(size_t bytes, char* buffer, size_t buffer_size);

/**
 * Format percentage with appropriate precision.
 * 
 * Formats percentage values with context-appropriate precision and
 * formatting for benchmark reports.
 * 
 * @param value Percentage value (0.0-100.0)
 * @param buffer Output buffer for formatted string
 * @param buffer_size Size of output buffer
 * @return Number of characters written, or -1 on error
 */
int benchmark_format_percentage(double value, char* buffer, size_t buffer_size);

/**
 * Get benchmark utilities version information.
 * 
 * @return Version packed as (major << 16) | (minor << 8) | patch
 */
uint32_t benchmark_utils_version(void);

/**
 * Get last error message from benchmark utilities.
 * 
 * @return Pointer to static error message string, or NULL if no error
 */
const char* benchmark_utils_get_last_error(void);

#ifdef __cplusplus
}
#endif

#endif /* ATLAS_BENCHMARK_UTILS_H */