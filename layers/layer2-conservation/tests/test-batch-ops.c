/* test-batch-ops.c - Comprehensive tests for Atlas-12288 Layer 2 batch operations
 * (c) 2024-2025 UOR Foundation - MIT License
 * 
 * Test suite for batch processing functions including:
 * - Batch conserved checking with SIMD optimization
 * - Batch delta computation with vectorization
 * - Batch witness generation with pipeline optimization
 * - Performance benchmarks and validation
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>
#include <time.h>
#include <assert.h>
#include <sys/time.h>

#include "../include/atlas-conservation.h"

// Test configuration
#define MAX_TEST_BUFFERS 256
#define TEST_BUFFER_SIZES { 16, 32, 64, 128, 256, 512, 1024 }
#define TEST_BATCH_SIZES { 1, 4, 8, 16, 32, 64, 128, 256 }
#define PERFORMANCE_ITERATIONS 1000

// Test statistics
typedef struct {
    int tests_run;
    int tests_passed;
    int tests_failed;
    double total_time_ms;
} test_stats_t;

static test_stats_t global_stats = {0};

// Utility functions
static double get_time_ms(void) {
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return tv.tv_sec * 1000.0 + tv.tv_usec / 1000.0;
}

static void test_start(const char* test_name) {
    printf("Running test: %s\n", test_name);
    global_stats.tests_run++;
}

static void test_pass(const char* test_name) {
    printf("  ✓ %s PASSED\n", test_name);
    global_stats.tests_passed++;
}

static void test_fail(const char* test_name, const char* reason) {
    printf("  ✗ %s FAILED: %s\n", test_name, reason);
    global_stats.tests_failed++;
}

// Generate test data with known conservation properties
static void generate_conserved_buffer(uint8_t* buffer, size_t size) {
    // Fill with pattern that ensures conservation (sum % 96 == 0)
    uint32_t sum = 0;
    for (size_t i = 0; i < size - 1; i++) {
        buffer[i] = (uint8_t)(i % 96);
        sum += buffer[i];
    }
    // Adjust last byte to ensure conservation
    buffer[size - 1] = (uint8_t)((96 - (sum % 96)) % 96);
}

static void generate_non_conserved_buffer(uint8_t* buffer, size_t size) {
    // Fill with pattern that ensures non-conservation (sum % 96 != 0)
    for (size_t i = 0; i < size; i++) {
        buffer[i] = (uint8_t)((i % 95) + 1);  // Never 0, so sum % 96 != 0
    }
}

static void generate_random_buffer(uint8_t* buffer, size_t size) {
    for (size_t i = 0; i < size; i++) {
        buffer[i] = (uint8_t)rand();
    }
}

// Test 1: Basic batch conserved check functionality
static void test_basic_batch_conserved_check(void) {
    test_start("basic_batch_conserved_check");
    
    const size_t buffer_size = 256;
    const size_t batch_count = 16;
    
    // Allocate test data
    uint8_t* test_data = malloc(batch_count * buffer_size);
    atlas_batch_buffer_t* buffers = malloc(batch_count * sizeof(atlas_batch_buffer_t));
    
    if (!test_data || !buffers) {
        test_fail("basic_batch_conserved_check", "Memory allocation failed");
        goto cleanup;
    }
    
    // Setup mixed conserved/non-conserved buffers
    for (size_t i = 0; i < batch_count; i++) {
        buffers[i].data = test_data + (i * buffer_size);
        buffers[i].size = buffer_size;
        buffers[i].status = 0;
        
        if (i % 2 == 0) {
            generate_conserved_buffer(buffers[i].data, buffer_size);
        } else {
            generate_non_conserved_buffer(buffers[i].data, buffer_size);
        }
    }
    
    // Test batch conserved check
    double start_time = get_time_ms();
    uint8_t* results = atlas_batch_conserved_check(buffers, batch_count);
    double batch_time = get_time_ms() - start_time;
    
    if (!results) {
        test_fail("basic_batch_conserved_check", "Batch function returned NULL");
        goto cleanup;
    }
    
    // Verify results
    bool all_correct = true;
    for (size_t i = 0; i < batch_count; i++) {
        bool expected_conserved = (i % 2 == 0);
        bool actual_conserved = (results[i] != 0);
        
        if (expected_conserved != actual_conserved) {
            printf("    Buffer %zu: expected %s, got %s\n", 
                   i, expected_conserved ? "conserved" : "not conserved",
                   actual_conserved ? "conserved" : "not conserved");
            all_correct = false;
        }
    }
    
    // Compare with individual calls for correctness
    start_time = get_time_ms();
    for (size_t i = 0; i < batch_count; i++) {
        bool individual_result = atlas_conserved_check(buffers[i].data, buffers[i].size);
        bool batch_result = (results[i] != 0);
        
        if (individual_result != batch_result) {
            printf("    Mismatch at buffer %zu: individual=%s, batch=%s\n",
                   i, individual_result ? "conserved" : "not conserved",
                   batch_result ? "conserved" : "not conserved");
            all_correct = false;
        }
    }
    double individual_time = get_time_ms() - start_time;
    
    printf("    Performance: batch=%.2fms, individual=%.2fms, speedup=%.1fx\n",
           batch_time, individual_time, individual_time / batch_time);
    
    if (all_correct && batch_time > 0) {
        test_pass("basic_batch_conserved_check");
    } else {
        test_fail("basic_batch_conserved_check", "Results incorrect or performance issue");
    }
    
    global_stats.total_time_ms += batch_time;
    free(results);
    
cleanup:
    free(test_data);
    free(buffers);
}

// Test 2: Batch delta computation
static void test_batch_delta_compute(void) {
    test_start("batch_delta_compute");
    
    const size_t buffer_size = 256;
    const size_t batch_count = 32;
    
    // Allocate test data
    uint8_t* before_data = malloc(batch_count * buffer_size);
    uint8_t* after_data = malloc(batch_count * buffer_size);
    atlas_batch_delta_t* deltas = malloc(batch_count * sizeof(atlas_batch_delta_t));
    
    if (!before_data || !after_data || !deltas) {
        test_fail("batch_delta_compute", "Memory allocation failed");
        goto cleanup;
    }
    
    // Setup test data with known delta patterns
    for (size_t i = 0; i < batch_count; i++) {
        uint8_t* before = before_data + (i * buffer_size);
        uint8_t* after = after_data + (i * buffer_size);
        
        // Create before buffer
        generate_random_buffer(before, buffer_size);
        
        // Create after buffer with known delta
        memcpy(after, before, buffer_size);
        uint8_t expected_delta = (uint8_t)(i % 96);  // Known delta
        if (buffer_size > 0) {
            after[0] = (after[0] + expected_delta) % 256;
        }
        
        deltas[i].before = before;
        deltas[i].after = after;
        deltas[i].size = buffer_size;
        deltas[i].delta = 0;  // Will be filled by function
    }
    
    // Test batch delta computation
    double start_time = get_time_ms();
    int result = atlas_batch_delta_compute(deltas, batch_count);
    double batch_time = get_time_ms() - start_time;
    
    if (result != 0) {
        test_fail("batch_delta_compute", "Batch function failed");
        goto cleanup;
    }
    
    // Verify results against individual computations
    bool all_correct = true;
    start_time = get_time_ms();
    for (size_t i = 0; i < batch_count; i++) {
        uint8_t individual_delta = atlas_conserved_delta(deltas[i].before, deltas[i].after, deltas[i].size);
        
        if (deltas[i].delta != individual_delta) {
            printf("    Delta mismatch at %zu: batch=%u, individual=%u\n",
                   i, deltas[i].delta, individual_delta);
            all_correct = false;
        }
    }
    double individual_time = get_time_ms() - start_time;
    
    printf("    Performance: batch=%.2fms, individual=%.2fms, speedup=%.1fx\n",
           batch_time, individual_time, individual_time / batch_time);
    
    if (all_correct) {
        test_pass("batch_delta_compute");
    } else {
        test_fail("batch_delta_compute", "Results incorrect");
    }
    
    global_stats.total_time_ms += batch_time;
    
cleanup:
    free(before_data);
    free(after_data);
    free(deltas);
}

// Test 3: Batch witness generation
static void test_batch_witness_generate(void) {
    test_start("batch_witness_generate");
    
    const size_t buffer_size = 128;
    const size_t batch_count = 8;  // Smaller batch for witness generation
    
    // Allocate test data
    uint8_t* test_data = malloc(batch_count * buffer_size);
    atlas_batch_witness_t* witnesses = malloc(batch_count * sizeof(atlas_batch_witness_t));
    
    if (!test_data || !witnesses) {
        test_fail("batch_witness_generate", "Memory allocation failed");
        goto cleanup;
    }
    
    // Setup test data
    for (size_t i = 0; i < batch_count; i++) {
        witnesses[i].data = test_data + (i * buffer_size);
        witnesses[i].size = buffer_size;
        witnesses[i].witness = NULL;
        witnesses[i].status = 0;
        
        generate_random_buffer((uint8_t*)witnesses[i].data, buffer_size);
    }
    
    // Test batch witness generation
    double start_time = get_time_ms();
    int result = atlas_batch_witness_generate(witnesses, batch_count);
    double batch_time = get_time_ms() - start_time;
    
    if (result != 0) {
        test_fail("batch_witness_generate", "Batch function failed");
        goto cleanup;
    }
    
    // Verify witness generation success and correctness
    bool all_correct = true;
    start_time = get_time_ms();
    for (size_t i = 0; i < batch_count; i++) {
        if (witnesses[i].status != 1 || !witnesses[i].witness) {
            printf("    Witness generation failed for buffer %zu\n", i);
            all_correct = false;
            continue;
        }
        
        // Verify witness by trying to verify it
        bool verify_result = atlas_witness_verify(witnesses[i].witness, witnesses[i].data, witnesses[i].size);
        if (!verify_result) {
            printf("    Witness verification failed for buffer %zu\n", i);
            all_correct = false;
        }
        
        // Compare with individual witness generation
        atlas_witness_t* individual_witness = atlas_witness_generate(witnesses[i].data, witnesses[i].size);
        if (!individual_witness) {
            printf("    Individual witness generation failed for buffer %zu\n", i);
            all_correct = false;
        } else {
            bool individual_verify = atlas_witness_verify(individual_witness, witnesses[i].data, witnesses[i].size);
            if (!individual_verify) {
                printf("    Individual witness verification failed for buffer %zu\n", i);
                all_correct = false;
            }
            atlas_witness_destroy(individual_witness);
        }
    }
    double individual_time = get_time_ms() - start_time;
    
    printf("    Performance: batch=%.2fms, individual=%.2fms, speedup=%.1fx\n",
           batch_time, individual_time, individual_time / batch_time);
    
    if (all_correct) {
        test_pass("batch_witness_generate");
    } else {
        test_fail("batch_witness_generate", "Results incorrect or witnesses invalid");
    }
    
    // Clean up witnesses
    for (size_t i = 0; i < batch_count; i++) {
        if (witnesses[i].witness) {
            atlas_witness_destroy(witnesses[i].witness);
        }
    }
    
    global_stats.total_time_ms += batch_time;
    
cleanup:
    free(test_data);
    free(witnesses);
}

// Test 4: Performance scaling with different batch sizes
static void test_performance_scaling(void) {
    test_start("performance_scaling");
    
    const size_t buffer_size = 256;
    const size_t batch_sizes[] = TEST_BATCH_SIZES;
    const size_t num_batch_sizes = sizeof(batch_sizes) / sizeof(batch_sizes[0]);
    
    printf("    Testing performance scaling:\n");
    printf("    Batch Size | Batch Time | Individual Time | Speedup\n");
    printf("    -----------+------------+-----------------+--------\n");
    
    bool scaling_reasonable = true;
    double best_speedup = 0.0;
    
    for (size_t b = 0; b < num_batch_sizes; b++) {
        size_t batch_count = batch_sizes[b];
        
        // Allocate test data
        uint8_t* test_data = malloc(batch_count * buffer_size);
        atlas_batch_buffer_t* buffers = malloc(batch_count * sizeof(atlas_batch_buffer_t));
        
        if (!test_data || !buffers) {
            printf("    Memory allocation failed for batch size %zu\n", batch_count);
            scaling_reasonable = false;
            free(test_data);
            free(buffers);
            continue;
        }
        
        // Setup test data
        for (size_t i = 0; i < batch_count; i++) {
            buffers[i].data = test_data + (i * buffer_size);
            buffers[i].size = buffer_size;
            buffers[i].status = 0;
            generate_conserved_buffer(buffers[i].data, buffer_size);
        }
        
        // Measure batch performance
        double batch_total = 0.0;
        for (int iter = 0; iter < 10; iter++) {
            double start = get_time_ms();
            uint8_t* results = atlas_batch_conserved_check(buffers, batch_count);
            double end = get_time_ms();
            
            if (results) {
                batch_total += (end - start);
                free(results);
            }
        }
        double batch_avg = batch_total / 10.0;
        
        // Measure individual performance
        double individual_total = 0.0;
        for (int iter = 0; iter < 10; iter++) {
            double start = get_time_ms();
            for (size_t i = 0; i < batch_count; i++) {
                atlas_conserved_check(buffers[i].data, buffers[i].size);
            }
            double end = get_time_ms();
            individual_total += (end - start);
        }
        double individual_avg = individual_total / 10.0;
        
        double speedup = (individual_avg > 0) ? individual_avg / batch_avg : 0.0;
        
        printf("    %10zu | %9.2fms | %14.2fms | %6.1fx\n",
               batch_count, batch_avg, individual_avg, speedup);
        
        if (speedup > best_speedup) {
            best_speedup = speedup;
        }
        
        // For larger batches, we should see meaningful speedup
        if (batch_count >= 16 && speedup < 1.5) {
            scaling_reasonable = false;
        }
        
        free(test_data);
        free(buffers);
    }
    
    printf("    Best speedup achieved: %.1fx\n", best_speedup);
    
    if (scaling_reasonable && best_speedup >= 2.0) {
        test_pass("performance_scaling");
        printf("    Target 2-3x speedup achieved: %.1fx\n", best_speedup);
    } else {
        test_fail("performance_scaling", "Performance scaling not adequate");
    }
}

// Test 5: Error handling and edge cases
static void test_error_handling(void) {
    test_start("error_handling");
    
    bool all_errors_handled = true;
    
    // Test NULL buffer array
    uint8_t* result1 = atlas_batch_conserved_check(NULL, 10);
    if (result1 != NULL || atlas_get_last_error() != ATLAS_ERROR_INVALID_ARGUMENT) {
        printf("    Failed to handle NULL buffer array\n");
        all_errors_handled = false;
    }
    
    // Test zero count
    atlas_batch_buffer_t dummy_buffer = {0};
    uint8_t* result2 = atlas_batch_conserved_check(&dummy_buffer, 0);
    if (result2 != NULL || atlas_get_last_error() != ATLAS_ERROR_INVALID_ARGUMENT) {
        printf("    Failed to handle zero count\n");
        all_errors_handled = false;
    }
    
    // Test excessive count
    uint8_t* result3 = atlas_batch_conserved_check(&dummy_buffer, 1000);
    if (result3 != NULL || atlas_get_last_error() != ATLAS_ERROR_INVALID_ARGUMENT) {
        printf("    Failed to handle excessive count\n");
        all_errors_handled = false;
    }
    
    // Test buffer with NULL data
    atlas_batch_buffer_t bad_buffer = { .data = NULL, .size = 100, .status = 0 };
    uint8_t* result4 = atlas_batch_conserved_check(&bad_buffer, 1);
    if (result4 != NULL || atlas_get_last_error() != ATLAS_ERROR_INVALID_ARGUMENT) {
        printf("    Failed to handle NULL data in buffer\n");
        all_errors_handled = false;
    }
    
    // Test buffer with zero size
    uint8_t test_data[100];
    atlas_batch_buffer_t zero_size_buffer = { .data = test_data, .size = 0, .status = 0 };
    uint8_t* result5 = atlas_batch_conserved_check(&zero_size_buffer, 1);
    if (result5 != NULL || atlas_get_last_error() != ATLAS_ERROR_INVALID_ARGUMENT) {
        printf("    Failed to handle zero size buffer\n");
        all_errors_handled = false;
    }
    
    if (all_errors_handled) {
        test_pass("error_handling");
    } else {
        test_fail("error_handling", "Some error cases not properly handled");
    }
}

// Test 6: Statistics tracking
static void test_statistics_tracking(void) {
    test_start("statistics_tracking");
    
    // Reset statistics
    atlas_batch_reset_statistics();
    
    // Get initial statistics
    atlas_batch_stats_t initial_stats;
    if (!atlas_batch_get_statistics(&initial_stats)) {
        test_fail("statistics_tracking", "Failed to get initial statistics");
        return;
    }
    
    // Perform some operations
    const size_t buffer_size = 128;
    const size_t batch_count = 16;
    
    uint8_t* test_data = malloc(batch_count * buffer_size);
    atlas_batch_buffer_t* buffers = malloc(batch_count * sizeof(atlas_batch_buffer_t));
    
    if (!test_data || !buffers) {
        test_fail("statistics_tracking", "Memory allocation failed");
        goto cleanup;
    }
    
    // Setup test data
    for (size_t i = 0; i < batch_count; i++) {
        buffers[i].data = test_data + (i * buffer_size);
        buffers[i].size = buffer_size;
        buffers[i].status = 0;
        generate_conserved_buffer(buffers[i].data, buffer_size);
    }
    
    // Perform batch operations
    uint8_t* results = atlas_batch_conserved_check(buffers, batch_count);
    if (results) {
        free(results);
    }
    
    // Get updated statistics
    atlas_batch_stats_t updated_stats;
    if (!atlas_batch_get_statistics(&updated_stats)) {
        test_fail("statistics_tracking", "Failed to get updated statistics");
        goto cleanup;
    }
    
    // Verify statistics were updated
    bool stats_correct = true;
    if (updated_stats.conserved_calls != initial_stats.conserved_calls + 1) {
        printf("    Expected conserved_calls %llu, got %llu\n",
               initial_stats.conserved_calls + 1, updated_stats.conserved_calls);
        stats_correct = false;
    }
    
    if (updated_stats.total_buffers < initial_stats.total_buffers + batch_count) {
        printf("    Expected total_buffers >= %llu, got %llu\n",
               initial_stats.total_buffers + batch_count, updated_stats.total_buffers);
        stats_correct = false;
    }
    
    if (stats_correct) {
        test_pass("statistics_tracking");
    } else {
        test_fail("statistics_tracking", "Statistics not properly tracked");
    }
    
cleanup:
    free(test_data);
    free(buffers);
}

// Test 7: Optimal batch size calculation
static void test_optimal_batch_size(void) {
    test_start("optimal_batch_size");
    
    const size_t buffer_sizes[] = { 16, 64, 256, 1024, 4096, 65536 };
    const size_t num_sizes = sizeof(buffer_sizes) / sizeof(buffer_sizes[0]);
    
    bool reasonable_sizes = true;
    
    printf("    Buffer Size | Optimal Batch Size\n");
    printf("    ------------+-------------------\n");
    
    for (size_t i = 0; i < num_sizes; i++) {
        size_t buffer_size = buffer_sizes[i];
        size_t optimal = atlas_batch_get_optimal_size(buffer_size);
        
        printf("    %10zu | %17zu\n", buffer_size, optimal);
        
        // Sanity checks
        if (optimal == 0 || optimal > 256) {
            printf("    Unreasonable batch size for buffer size %zu\n", buffer_size);
            reasonable_sizes = false;
        }
        
        // Expect smaller batches for larger buffers
        if (i > 0 && buffer_size > buffer_sizes[i-1] * 4) {
            size_t prev_optimal = atlas_batch_get_optimal_size(buffer_sizes[i-1]);
            if (optimal > prev_optimal * 2) {
                printf("    Batch size doesn't decrease appropriately for larger buffers\n");
                reasonable_sizes = false;
            }
        }
    }
    
    if (reasonable_sizes) {
        test_pass("optimal_batch_size");
    } else {
        test_fail("optimal_batch_size", "Unreasonable optimal batch sizes");
    }
}

// Main test runner
int main(int argc, char* argv[]) {
    printf("Atlas-12288 Layer 2 Batch Operations Test Suite\n");
    printf("================================================\n\n");
    
    // Initialize random seed
    srand((unsigned int)time(NULL));
    
    // Run all tests
    test_basic_batch_conserved_check();
    test_batch_delta_compute();
    test_batch_witness_generate();
    test_performance_scaling();
    test_error_handling();
    test_statistics_tracking();
    test_optimal_batch_size();
    
    // Print final statistics
    printf("\nTest Results Summary:\n");
    printf("=====================\n");
    printf("Tests run:    %d\n", global_stats.tests_run);
    printf("Tests passed: %d\n", global_stats.tests_passed);
    printf("Tests failed: %d\n", global_stats.tests_failed);
    printf("Success rate: %.1f%%\n", 
           global_stats.tests_run > 0 ? 
           (100.0 * global_stats.tests_passed / global_stats.tests_run) : 0.0);
    printf("Total time:   %.2fms\n", global_stats.total_time_ms);
    
    // Return appropriate exit code
    return (global_stats.tests_failed == 0) ? 0 : 1;
}