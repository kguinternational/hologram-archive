/* benchmark_utils.c - Atlas Layer 4 Benchmark Utilities Implementation
 * (c) 2024-2025 UOR Foundation - MIT License
 * 
 * Implementation of utility functions for benchmarking with data generation,
 * conservation checking, result formatting, and analysis capabilities.
 */

#include "benchmark_utils.h"
#include "../../include/atlas-manifold.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <math.h>
#include <time.h>
#include <errno.h>
#include <stdarg.h>

// =============================================================================
// Internal Constants and Macros
// =============================================================================

#define MAX_ERROR_MESSAGE_LENGTH 512
#define ATLAS_CONSERVATION_MODULUS 96
#define NSEC_PER_SEC 1000000000ULL
#define NSEC_PER_MSEC 1000000ULL
#define NSEC_PER_USEC 1000ULL

// ANSI color codes for console output
#define COLOR_RESET   "\033[0m"
#define COLOR_RED     "\033[31m"
#define COLOR_GREEN   "\033[32m"
#define COLOR_YELLOW  "\033[33m"
#define COLOR_BLUE    "\033[34m"
#define COLOR_MAGENTA "\033[35m"
#define COLOR_CYAN    "\033[36m"
#define COLOR_BOLD    "\033[1m"

// =============================================================================
// Internal Global State
// =============================================================================

static char g_last_error_message[MAX_ERROR_MESSAGE_LENGTH] = {0};

// =============================================================================
// Internal Helper Functions
// =============================================================================

/**
 * Set error message for utilities error reporting.
 */
static void set_error_message(const char* format, ...) {
    va_list args;
    va_start(args, format);
    vsnprintf(g_last_error_message, sizeof(g_last_error_message), format, args);
    va_end(args);
}

/**
 * Simple XOR-shift pseudo-random number generator.
 */
static uint32_t xorshift32(uint32_t* state) {
    uint32_t x = *state;
    x ^= x << 13;
    x ^= x >> 17;
    x ^= x << 5;
    *state = x;
    return x;
}

/**
 * Calculate conservation checksum for data.
 */
static uint8_t calculate_conservation_checksum(const uint8_t* data, size_t size) {
    uint64_t sum = 0;
    for (size_t i = 0; i < size; i++) {
        sum += data[i];
    }
    return (uint8_t)(sum % ATLAS_CONSERVATION_MODULUS);
}

// =============================================================================
// Data Generation Functions
// =============================================================================

int benchmark_generate_conserved_data(uint8_t* buffer, size_t size, uint32_t seed) {
    if (!buffer || size == 0) {
        set_error_message("Invalid buffer or size for data generation");
        return -1;
    }
    
    uint32_t rng_state = seed;
    uint64_t sum = 0;
    
    // Fill buffer with random data
    for (size_t i = 0; i < size - 1; i++) {
        buffer[i] = (uint8_t)(xorshift32(&rng_state) & 0xFF);
        sum += buffer[i];
    }
    
    // Calculate final byte to ensure conservation (sum % 96 == 0)
    uint8_t remainder = (uint8_t)(sum % ATLAS_CONSERVATION_MODULUS);
    if (remainder != 0) {
        buffer[size - 1] = ATLAS_CONSERVATION_MODULUS - remainder;
    } else {
        buffer[size - 1] = 0;
    }
    
    return 0;
}

// Types are defined in the header file

int benchmark_generate_boundary_region(temp_boundary_region_t* region, uint32_t seed) {
    if (!region) {
        set_error_message("Invalid region pointer");
        return -1;
    }
    
    uint32_t rng_state = seed;
    
    // Generate valid Atlas coordinates (48 pages × 256 bytes)
    uint32_t start_page = xorshift32(&rng_state) % 48;
    uint32_t start_offset = xorshift32(&rng_state) % 256;
    uint32_t page_count = 1 + (xorshift32(&rng_state) % 8); // 1-8 pages
    
    region->start_coord = start_page * 256 + start_offset;
    region->end_coord = region->start_coord + (page_count * 256) - 1;
    region->page_count = page_count;
    region->region_class = (uint8_t)(xorshift32(&rng_state) % ATLAS_CONSERVATION_MODULUS);
    region->is_conserved = true; // Assume conserved for testing
    
    return 0;
}

int benchmark_generate_transform_params(temp_transform_params_t* params, uint32_t seed) {
    if (!params) {
        set_error_message("Invalid parameters pointer");
        return -1;
    }
    
    uint32_t rng_state = seed;
    
    // Generate reasonable transformation parameters
    params->scaling_factor = 0.5 + ((double)(xorshift32(&rng_state) % 1000) / 1000.0); // 0.5-1.5
    params->rotation_angle = ((double)(xorshift32(&rng_state) % 628) / 100.0); // 0-2π
    params->translation_x = ((double)(xorshift32(&rng_state) % 200) - 100.0); // -100 to 100
    params->translation_y = ((double)(xorshift32(&rng_state) % 200) - 100.0); // -100 to 100
    
    return 0;
}

int benchmark_generate_data_with_checksum(uint8_t* buffer, size_t size, 
                                         uint8_t target_checksum, uint32_t seed) {
    if (!buffer || size == 0 || target_checksum >= ATLAS_CONSERVATION_MODULUS) {
        set_error_message("Invalid parameters for checksum data generation");
        return -1;
    }
    
    uint32_t rng_state = seed;
    uint64_t sum = 0;
    
    // Fill buffer except last byte with random data
    for (size_t i = 0; i < size - 1; i++) {
        buffer[i] = (uint8_t)(xorshift32(&rng_state) & 0xFF);
        sum += buffer[i];
    }
    
    // Calculate final byte to achieve target checksum
    uint8_t current_checksum = (uint8_t)(sum % ATLAS_CONSERVATION_MODULUS);
    uint8_t adjustment = (target_checksum >= current_checksum) ?
                        (target_checksum - current_checksum) :
                        (ATLAS_CONSERVATION_MODULUS - current_checksum + target_checksum);
    
    buffer[size - 1] = adjustment;
    
    return 0;
}

// =============================================================================
// Conservation Verification Functions
// =============================================================================

bool benchmark_verify_conservation(const uint8_t* data, size_t size) {
    if (!data || size == 0) {
        return false;
    }
    
    return calculate_conservation_checksum(data, size) == 0;
}

uint8_t benchmark_calculate_checksum(const uint8_t* data, size_t size) {
    if (!data || size == 0) {
        return 255; // Error value
    }
    
    return calculate_conservation_checksum(data, size);
}

int benchmark_generate_conservation_witness(const uint8_t* data, size_t size, 
                                           uint8_t witness[32]) {
    if (!data || !witness || size == 0) {
        set_error_message("Invalid parameters for witness generation");
        return -1;
    }
    
    // Simple witness: first 24 bytes are hash-like data, last 8 are metadata
    memset(witness, 0, 32);
    
    // Calculate checksum
    uint8_t checksum = calculate_conservation_checksum(data, size);
    witness[0] = checksum;
    
    // Store size in witness
    witness[1] = (uint8_t)(size & 0xFF);
    witness[2] = (uint8_t)((size >> 8) & 0xFF);
    witness[3] = (uint8_t)((size >> 16) & 0xFF);
    witness[4] = (uint8_t)((size >> 24) & 0xFF);
    
    // Simple hash of data
    uint32_t hash = 0;
    for (size_t i = 0; i < size; i++) {
        hash = hash * 31 + data[i];
    }
    
    witness[5] = (uint8_t)(hash & 0xFF);
    witness[6] = (uint8_t)((hash >> 8) & 0xFF);
    witness[7] = (uint8_t)((hash >> 16) & 0xFF);
    witness[8] = (uint8_t)((hash >> 24) & 0xFF);
    
    return 0;
}

bool benchmark_verify_conservation_witness(const uint8_t* data, size_t size,
                                          const uint8_t witness[32]) {
    if (!data || !witness || size == 0) {
        return false;
    }
    
    // Verify checksum
    uint8_t expected_checksum = witness[0];
    uint8_t actual_checksum = calculate_conservation_checksum(data, size);
    if (expected_checksum != actual_checksum) {
        return false;
    }
    
    // Verify size
    size_t expected_size = (size_t)witness[1] |
                          ((size_t)witness[2] << 8) |
                          ((size_t)witness[3] << 16) |
                          ((size_t)witness[4] << 24);
    if (expected_size != size) {
        return false;
    }
    
    // Verify hash
    uint32_t expected_hash = (uint32_t)witness[5] |
                            ((uint32_t)witness[6] << 8) |
                            ((uint32_t)witness[7] << 16) |
                            ((uint32_t)witness[8] << 24);
    
    uint32_t actual_hash = 0;
    for (size_t i = 0; i < size; i++) {
        actual_hash = actual_hash * 31 + data[i];
    }
    
    return expected_hash == actual_hash;
}

// =============================================================================
// Result Formatting Functions
// =============================================================================

int benchmark_format_text_report(const benchmark_results_t* results, 
                                char* buffer, size_t buffer_size) {
    if (!results || !buffer || buffer_size == 0) {
        set_error_message("Invalid parameters for text report formatting");
        return -1;
    }
    
    int written = 0;
    int ret;
    
    ret = snprintf(buffer + written, buffer_size - written,
        "=== Benchmark Results: %s ===\n"
        "Description: %s\n"
        "Success: %s\n"
        "Iterations: %zu completed, %zu failed\n\n",
        results->name,
        results->description[0] ? results->description : "No description",
        results->success ? "Yes" : "No",
        results->iterations_completed,
        results->iterations_failed
    );
    if (ret < 0 || (size_t)ret >= buffer_size - written) return -1;
    written += ret;
    
    // Timing statistics
    ret = snprintf(buffer + written, buffer_size - written,
        "=== Timing Statistics ===\n"
        "Mean: %lu ns\n"
        "Median: %lu ns\n"
        "Min: %lu ns\n"
        "Max: %lu ns\n"
        "Std Dev: %.2f ns\n"
        "Total: %lu ns\n",
        results->timing.mean_ns,
        results->timing.median_ns,
        results->timing.min_ns,
        results->timing.max_ns,
        results->timing.stddev_ns,
        results->timing.total_ns
    );
    if (ret < 0 || (size_t)ret >= buffer_size - written) return -1;
    written += ret;
    
    // Percentiles
    if (results->timing.percentile_count > 0) {
        ret = snprintf(buffer + written, buffer_size - written, "Percentiles: ");
        if (ret < 0 || (size_t)ret >= buffer_size - written) return -1;
        written += ret;
        
        for (size_t i = 0; i < results->timing.percentile_count; i++) {
            ret = snprintf(buffer + written, buffer_size - written,
                "P%zu=%lu ", i * 25 + 50, results->timing.percentiles[i]);
            if (ret < 0 || (size_t)ret >= buffer_size - written) return -1;
            written += ret;
        }
        
        ret = snprintf(buffer + written, buffer_size - written, "ns\n\n");
        if (ret < 0 || (size_t)ret >= buffer_size - written) return -1;
        written += ret;
    }
    
    // Memory statistics
    ret = snprintf(buffer + written, buffer_size - written,
        "=== Memory Statistics ===\n"
        "Allocated: %zu bytes\n"
        "Peak: %zu bytes\n"
        "Allocations: %zu\n"
        "Deallocations: %zu\n"
        "Still allocated: %zu bytes\n\n",
        results->memory.allocated_bytes,
        results->memory.peak_allocated_bytes,
        results->memory.allocation_count,
        results->memory.deallocation_count,
        results->memory.current_allocated
    );
    if (ret < 0 || (size_t)ret >= buffer_size - written) return -1;
    written += ret;
    
    // Conservation statistics
    ret = snprintf(buffer + written, buffer_size - written,
        "=== Conservation Statistics ===\n"
        "Total checks: %zu\n"
        "Passed: %zu\n"
        "Failed: %zu\n"
        "Conservation maintained: %s\n"
        "Total delta: %lu\n"
        "Max delta: %lu\n\n",
        results->conservation.total_checks,
        results->conservation.passed_checks,
        results->conservation.failed_checks,
        results->conservation.conservation_maintained ? "Yes" : "No",
        results->conservation.total_delta,
        results->conservation.max_delta
    );
    if (ret < 0 || (size_t)ret >= buffer_size - written) return -1;
    written += ret;
    
    return written;
}

int benchmark_format_csv_line(const benchmark_results_t* results,
                             char* buffer, size_t buffer_size) {
    if (!results || !buffer || buffer_size == 0) {
        set_error_message("Invalid parameters for CSV formatting");
        return -1;
    }
    
    return snprintf(buffer, buffer_size,
        "%s,%s,%s,%zu,%zu,%lu,%lu,%lu,%lu,%.2f,%zu,%zu,%zu,%zu,%zu,%s,%lu,%lu\n",
        results->name,
        results->description,
        results->success ? "true" : "false",
        results->iterations_completed,
        results->iterations_failed,
        results->timing.mean_ns,
        results->timing.median_ns,
        results->timing.min_ns,
        results->timing.max_ns,
        results->timing.stddev_ns,
        results->memory.allocated_bytes,
        results->memory.peak_allocated_bytes,
        results->memory.allocation_count,
        results->conservation.total_checks,
        results->conservation.passed_checks,
        results->conservation.conservation_maintained ? "true" : "false",
        results->conservation.total_delta,
        results->conservation.max_delta
    );
}

int benchmark_format_csv_header(char* buffer, size_t buffer_size) {
    if (!buffer || buffer_size == 0) {
        set_error_message("Invalid buffer for CSV header");
        return -1;
    }
    
    return snprintf(buffer, buffer_size,
        "name,description,success,iterations_completed,iterations_failed,"
        "mean_ns,median_ns,min_ns,max_ns,stddev_ns,"
        "allocated_bytes,peak_bytes,allocations,"
        "conservation_checks,conservation_passed,conservation_maintained,"
        "conservation_delta,max_delta\n"
    );
}

int benchmark_format_json(const benchmark_results_t* results,
                         char* buffer, size_t buffer_size, bool pretty_print) {
    if (!results || !buffer || buffer_size == 0) {
        set_error_message("Invalid parameters for JSON formatting");
        return -1;
    }
    
    const char* indent = pretty_print ? "  " : "";
    const char* newline = pretty_print ? "\n" : "";
    
    return snprintf(buffer, buffer_size,
        "{%s"
        "%s\"name\": \"%s\",%s"
        "%s\"description\": \"%s\",%s"
        "%s\"success\": %s,%s"
        "%s\"iterations_completed\": %zu,%s"
        "%s\"iterations_failed\": %zu,%s"
        "%s\"timing\": {%s"
        "%s%s\"mean_ns\": %lu,%s"
        "%s%s\"median_ns\": %lu,%s"
        "%s%s\"min_ns\": %lu,%s"
        "%s%s\"max_ns\": %lu,%s"
        "%s%s\"stddev_ns\": %.2f%s"
        "%s},%s"
        "%s\"memory\": {%s"
        "%s%s\"allocated_bytes\": %zu,%s"
        "%s%s\"peak_bytes\": %zu,%s"
        "%s%s\"allocations\": %zu%s"
        "%s},%s"
        "%s\"conservation\": {%s"
        "%s%s\"total_checks\": %zu,%s"
        "%s%s\"passed_checks\": %zu,%s"
        "%s%s\"maintained\": %s,%s"
        "%s%s\"total_delta\": %lu,%s"
        "%s%s\"max_delta\": %lu%s"
        "%s}%s"
        "}",
        newline,
        indent, results->name, newline,
        indent, results->description, newline,
        indent, results->success ? "true" : "false", newline,
        indent, results->iterations_completed, newline,
        indent, results->iterations_failed, newline,
        indent, newline,
        indent, indent, results->timing.mean_ns, newline,
        indent, indent, results->timing.median_ns, newline,
        indent, indent, results->timing.min_ns, newline,
        indent, indent, results->timing.max_ns, newline,
        indent, indent, results->timing.stddev_ns, newline,
        indent, newline,
        indent, newline,
        indent, indent, results->memory.allocated_bytes, newline,
        indent, indent, results->memory.peak_allocated_bytes, newline,
        indent, indent, results->memory.allocation_count, newline,
        indent, newline,
        indent, newline,
        indent, indent, results->conservation.total_checks, newline,
        indent, indent, results->conservation.passed_checks, newline,
        indent, indent, results->conservation.conservation_maintained ? "true" : "false", newline,
        indent, indent, results->conservation.total_delta, newline,
        indent, indent, results->conservation.max_delta, newline,
        indent, newline,
        newline
    );
}

// =============================================================================
// File Output Functions  
// =============================================================================

int benchmark_write_text_report(const benchmark_results_t* results,
                               const char* filename, bool append) {
    if (!results || !filename) {
        set_error_message("Invalid parameters for text file output");
        return -1;
    }
    
    FILE* file = fopen(filename, append ? "a" : "w");
    if (!file) {
        set_error_message("Failed to open file %s: %s", filename, strerror(errno));
        return -1;
    }
    
    char report_buffer[BENCHMARK_MAX_REPORT_LENGTH];
    int report_length = benchmark_format_text_report(results, report_buffer, sizeof(report_buffer));
    
    if (report_length < 0) {
        fclose(file);
        return -1;
    }
    
    size_t written = fwrite(report_buffer, 1, report_length, file);
    fclose(file);
    
    if (written != (size_t)report_length) {
        set_error_message("Failed to write complete report to file");
        return -1;
    }
    
    return 0;
}

int benchmark_write_csv_report(const benchmark_results_t* results,
                              const char* filename, bool write_header) {
    if (!results || !filename) {
        set_error_message("Invalid parameters for CSV file output");
        return -1;
    }
    
    FILE* file = fopen(filename, "a");
    if (!file) {
        set_error_message("Failed to open CSV file %s: %s", filename, strerror(errno));
        return -1;
    }
    
    // Write header if requested
    if (write_header) {
        char header_buffer[BENCHMARK_MAX_CSV_LINE_LENGTH];
        int header_length = benchmark_format_csv_header(header_buffer, sizeof(header_buffer));
        if (header_length > 0) {
            fwrite(header_buffer, 1, header_length, file);
        }
    }
    
    // Write data line
    char csv_buffer[BENCHMARK_MAX_CSV_LINE_LENGTH];
    int csv_length = benchmark_format_csv_line(results, csv_buffer, sizeof(csv_buffer));
    
    if (csv_length < 0) {
        fclose(file);
        return -1;
    }
    
    size_t written = fwrite(csv_buffer, 1, csv_length, file);
    fclose(file);
    
    if (written != (size_t)csv_length) {
        set_error_message("Failed to write CSV line to file");
        return -1;
    }
    
    return 0;
}

int benchmark_write_json_report(const benchmark_results_t* results,
                               const char* filename, bool pretty_print) {
    if (!results || !filename) {
        set_error_message("Invalid parameters for JSON file output");
        return -1;
    }
    
    FILE* file = fopen(filename, "w");
    if (!file) {
        set_error_message("Failed to open JSON file %s: %s", filename, strerror(errno));
        return -1;
    }
    
    char json_buffer[BENCHMARK_MAX_JSON_LENGTH];
    int json_length = benchmark_format_json(results, json_buffer, sizeof(json_buffer), pretty_print);
    
    if (json_length < 0) {
        fclose(file);
        return -1;
    }
    
    size_t written = fwrite(json_buffer, 1, json_length, file);
    fclose(file);
    
    if (written != (size_t)json_length) {
        set_error_message("Failed to write JSON to file");
        return -1;
    }
    
    return 0;
}

// =============================================================================
// Analysis and Comparison Functions
// =============================================================================

int benchmark_compare_results(const benchmark_results_t* baseline,
                             const benchmark_results_t* current,
                             char* buffer, size_t buffer_size) {
    if (!baseline || !current || !buffer || buffer_size == 0) {
        set_error_message("Invalid parameters for results comparison");
        return -1;
    }
    
    double time_ratio = (double)current->timing.mean_ns / (double)baseline->timing.mean_ns;
    const char* performance_desc;
    
    if (time_ratio > 1.1) {
        performance_desc = "SLOWER";
    } else if (time_ratio < 0.9) {
        performance_desc = "FASTER";
    } else {
        performance_desc = "SIMILAR";
    }
    
    return snprintf(buffer, buffer_size,
        "=== Benchmark Comparison ===\n"
        "Baseline: %s\n"
        "Current:  %s\n\n"
        "Performance: %s (ratio: %.3f)\n"
        "Time change: %.1f%% \n"
        "Memory change: %+ld bytes\n"
        "Conservation: Baseline %s, Current %s\n",
        baseline->name,
        current->name,
        performance_desc,
        time_ratio,
        (time_ratio - 1.0) * 100.0,
        (long)current->memory.peak_allocated_bytes - (long)baseline->memory.peak_allocated_bytes,
        baseline->conservation.conservation_maintained ? "OK" : "FAILED",
        current->conservation.conservation_maintained ? "OK" : "FAILED"
    );
}

int benchmark_calculate_regression(const benchmark_results_t* baseline,
                                  const benchmark_results_t* current,
                                  double* regression_ratio,
                                  double* confidence_level) {
    if (!baseline || !current || !regression_ratio || !confidence_level) {
        set_error_message("Invalid parameters for regression calculation");
        return -1;
    }
    
    *regression_ratio = (double)current->timing.mean_ns / (double)baseline->timing.mean_ns;
    
    // Simple confidence calculation based on standard deviations
    double baseline_cv = baseline->timing.stddev_ns / (double)baseline->timing.mean_ns;
    double current_cv = current->timing.stddev_ns / (double)current->timing.mean_ns;
    double combined_cv = sqrt(baseline_cv * baseline_cv + current_cv * current_cv);
    
    // Higher confidence when coefficient of variation is lower
    *confidence_level = 1.0 - fmin(combined_cv, 0.5);
    
    return 0;
}

// =============================================================================
// Console Output Functions
// =============================================================================

void benchmark_print_results(const benchmark_results_t* results, bool use_colors) {
    if (!results) {
        return;
    }
    
    const char* success_color = use_colors ? (results->success ? COLOR_GREEN : COLOR_RED) : "";
    const char* conservation_color = use_colors ? 
        (results->conservation.conservation_maintained ? COLOR_GREEN : COLOR_RED) : "";
    const char* reset_color = use_colors ? COLOR_RESET : "";
    const char* bold_color = use_colors ? COLOR_BOLD : "";
    
    printf("%s=== %s ===%s\n", bold_color, results->name, reset_color);
    if (results->description[0]) {
        printf("Description: %s\n", results->description);
    }
    printf("Success: %s%s%s\n", success_color, results->success ? "Yes" : "No", reset_color);
    printf("Iterations: %zu completed, %zu failed\n\n", 
           results->iterations_completed, results->iterations_failed);
    
    printf("Timing: mean=%luns, median=%luns, min=%luns, max=%luns\n",
           results->timing.mean_ns, results->timing.median_ns,
           results->timing.min_ns, results->timing.max_ns);
    
    printf("Memory: allocated=%zu bytes, peak=%zu bytes\n",
           results->memory.allocated_bytes, results->memory.peak_allocated_bytes);
    
    printf("Conservation: %s%s%s (checks=%zu, passed=%zu)\n",
           conservation_color, 
           results->conservation.conservation_maintained ? "OK" : "FAILED",
           reset_color,
           results->conservation.total_checks,
           results->conservation.passed_checks);
    
    printf("\n");
}

void benchmark_print_comparison(const benchmark_results_t* baseline,
                               const benchmark_results_t* current,
                               bool use_colors) {
    if (!baseline || !current) {
        return;
    }
    
    double time_ratio = (double)current->timing.mean_ns / (double)baseline->timing.mean_ns;
    const char* perf_color = "";
    
    if (use_colors) {
        if (time_ratio > 1.1) {
            perf_color = COLOR_RED;    // Slower
        } else if (time_ratio < 0.9) {
            perf_color = COLOR_GREEN;  // Faster
        } else {
            perf_color = COLOR_YELLOW; // Similar
        }
    }
    
    const char* reset_color = use_colors ? COLOR_RESET : "";
    const char* bold_color = use_colors ? COLOR_BOLD : "";
    
    printf("%s=== Comparison: %s vs %s ===%s\n", 
           bold_color, baseline->name, current->name, reset_color);
    
    printf("Performance: %s%.3fx %s(%.1f%% change)%s\n",
           perf_color, time_ratio,
           time_ratio > 1.0 ? "slower " : time_ratio < 1.0 ? "faster " : "",
           (time_ratio - 1.0) * 100.0, reset_color);
    
    printf("Mean time: %lu ns -> %lu ns\n", 
           baseline->timing.mean_ns, current->timing.mean_ns);
    
    printf("Peak memory: %zu -> %zu bytes (%+ld)\n",
           baseline->memory.peak_allocated_bytes,
           current->memory.peak_allocated_bytes,
           (long)current->memory.peak_allocated_bytes - (long)baseline->memory.peak_allocated_bytes);
    
    printf("\n");
}

void benchmark_print_progress(size_t current_iteration, size_t total_iterations,
                             uint64_t elapsed_time_ns) {
    if (total_iterations == 0) {
        return;
    }
    
    double progress = (double)current_iteration / (double)total_iterations;
    int bar_width = 40;
    int filled_width = (int)(progress * bar_width);
    
    printf("\rProgress: [");
    for (int i = 0; i < bar_width; i++) {
        if (i < filled_width) {
            printf("=");
        } else if (i == filled_width && current_iteration < total_iterations) {
            printf(">");
        } else {
            printf(" ");
        }
    }
    printf("] %3.1f%% (%zu/%zu)", progress * 100.0, current_iteration, total_iterations);
    
    if (current_iteration > 0 && elapsed_time_ns > 0) {
        uint64_t estimated_total_ns = (elapsed_time_ns * total_iterations) / current_iteration;
        uint64_t remaining_ns = estimated_total_ns - elapsed_time_ns;
        
        if (remaining_ns < NSEC_PER_SEC) {
            printf(" ETA: <1s");
        } else if (remaining_ns < 60 * NSEC_PER_SEC) {
            printf(" ETA: %llus", remaining_ns / NSEC_PER_SEC);
        } else {
            printf(" ETA: %llum%llus", 
                   remaining_ns / (60 * NSEC_PER_SEC),
                   (remaining_ns % (60 * NSEC_PER_SEC)) / NSEC_PER_SEC);
        }
    }
    
    fflush(stdout);
}

// =============================================================================
// Utility Helper Functions
// =============================================================================

int benchmark_format_duration(uint64_t nanoseconds, char* buffer, size_t buffer_size) {
    if (!buffer || buffer_size == 0) {
        return -1;
    }
    
    if (nanoseconds < NSEC_PER_USEC) {
        return snprintf(buffer, buffer_size, "%lu ns", nanoseconds);
    } else if (nanoseconds < NSEC_PER_MSEC) {
        return snprintf(buffer, buffer_size, "%.2f μs", 
                       (double)nanoseconds / NSEC_PER_USEC);
    } else if (nanoseconds < NSEC_PER_SEC) {
        return snprintf(buffer, buffer_size, "%.2f ms", 
                       (double)nanoseconds / NSEC_PER_MSEC);
    } else {
        return snprintf(buffer, buffer_size, "%.3f s", 
                       (double)nanoseconds / NSEC_PER_SEC);
    }
}

int benchmark_format_memory_size(size_t bytes, char* buffer, size_t buffer_size) {
    if (!buffer || buffer_size == 0) {
        return -1;
    }
    
    if (bytes < 1024) {
        return snprintf(buffer, buffer_size, "%zu B", bytes);
    } else if (bytes < 1024 * 1024) {
        return snprintf(buffer, buffer_size, "%.2f KB", (double)bytes / 1024.0);
    } else if (bytes < 1024 * 1024 * 1024) {
        return snprintf(buffer, buffer_size, "%.2f MB", (double)bytes / (1024.0 * 1024.0));
    } else {
        return snprintf(buffer, buffer_size, "%.2f GB", 
                       (double)bytes / (1024.0 * 1024.0 * 1024.0));
    }
}

int benchmark_format_percentage(double value, char* buffer, size_t buffer_size) {
    if (!buffer || buffer_size == 0) {
        return -1;
    }
    
    if (fabs(value) < 0.01) {
        return snprintf(buffer, buffer_size, "0.00%%");
    } else if (fabs(value) < 1.0) {
        return snprintf(buffer, buffer_size, "%.2f%%", value);
    } else if (fabs(value) < 10.0) {
        return snprintf(buffer, buffer_size, "%.1f%%", value);
    } else {
        return snprintf(buffer, buffer_size, "%.0f%%", value);
    }
}

// =============================================================================
// Version and Error Functions
// =============================================================================

uint32_t benchmark_utils_version(void) {
    return (1 << 16) | (0 << 8) | 0;  // Version 1.0.0
}

const char* benchmark_utils_get_last_error(void) {
    return g_last_error_message[0] != '\0' ? g_last_error_message : NULL;
}