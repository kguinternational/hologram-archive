/* test-conservation.c - Comprehensive Conservation Layer Tests
 * (c) 2024-2025 UOR Foundation - MIT License
 * 
 * Comprehensive test suite for Layer 2 Conservation runtime including:
 * - Domain lifecycle operations
 * - Budget management with mod-96 arithmetic
 * - Witness generation and verification
 * - Conservation law verification
 * - Thread safety (when not WASM)
 * - Error handling and edge cases
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>
#include <assert.h>
#include <errno.h>
#include <time.h>

// Include WASM compatibility first
#include "../wasm/wasm-compat.h"

// Include the conservation API
#include "../include/atlas-conservation.h"

// Test framework macros
#define TEST_ASSERT(cond, msg) \
    do { \
        if (!(cond)) { \
            printf("FAIL: %s (line %d): %s\n", __func__, __LINE__, msg); \
            return false; \
        } \
    } while(0)

#define TEST_ASSERT_EQ(a, b, msg) \
    do { \
        if ((a) != (b)) { \
            printf("FAIL: %s (line %d): %s (got %d, expected %d)\n", __func__, __LINE__, msg, (int)(a), (int)(b)); \
            return false; \
        } \
    } while(0)

#define RUN_TEST(test_func) \
    do { \
        printf("Running %s...", #test_func); \
        fflush(stdout); \
        if (test_func()) { \
            printf(" PASS\n"); \
            tests_passed++; \
        } else { \
            printf(" FAIL\n"); \
            tests_failed++; \
        } \
        total_tests++; \
    } while(0)

// Global test counters
static int total_tests = 0;
static int tests_passed = 0;
static int tests_failed = 0;

// Test data patterns
static const uint8_t TEST_PATTERN_ZEROS[256] = {0};
static const uint8_t TEST_PATTERN_ONES[256] = {
    [0 ... 255] = 0xFF
};
static const uint8_t TEST_PATTERN_SEQUENTIAL[256] = {
    0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15,
    16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31,
    32, 33, 34, 35, 36, 37, 38, 39, 40, 41, 42, 43, 44, 45, 46, 47,
    48, 49, 50, 51, 52, 53, 54, 55, 56, 57, 58, 59, 60, 61, 62, 63,
    64, 65, 66, 67, 68, 69, 70, 71, 72, 73, 74, 75, 76, 77, 78, 79,
    80, 81, 82, 83, 84, 85, 86, 87, 88, 89, 90, 91, 92, 93, 94, 95,
    0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15,
    16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31,
    32, 33, 34, 35, 36, 37, 38, 39, 40, 41, 42, 43, 44, 45, 46, 47,
    48, 49, 50, 51, 52, 53, 54, 55, 56, 57, 58, 59, 60, 61, 62, 63,
    64, 65, 66, 67, 68, 69, 70, 71, 72, 73, 74, 75, 76, 77, 78, 79,
    80, 81, 82, 83, 84, 85, 86, 87, 88, 89, 90, 91, 92, 93, 94, 95,
    0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15,
    16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31,
    32, 33, 34, 35, 36, 37, 38, 39, 40, 41, 42, 43, 44, 45, 46, 47,
    48, 49, 50, 51, 52, 53, 54, 55, 56, 57, 58, 59, 60, 61, 62, 63
};

// =============================================================================
// Domain Lifecycle Tests
// =============================================================================

static bool test_domain_creation(void) {
    // Test valid domain creation
    atlas_domain_t* domain = atlas_domain_create(1024, 42);
    TEST_ASSERT(domain != NULL, "Domain creation should succeed");
    TEST_ASSERT_EQ(atlas_get_last_error(), ATLAS_OK, "No error should occur");
    
    atlas_domain_destroy(domain);
    
    // Test invalid parameters
    domain = atlas_domain_create(0, 42);
    TEST_ASSERT(domain == NULL, "Domain creation with zero size should fail");
    TEST_ASSERT_EQ(atlas_get_last_error(), ATLAS_E_INVALID, "Should set INVALID error");
    
    domain = atlas_domain_create(1024, 96);
    TEST_ASSERT(domain == NULL, "Domain creation with invalid budget class should fail");
    TEST_ASSERT_EQ(atlas_get_last_error(), ATLAS_E_INVALID, "Should set INVALID error");
    
    return true;
}

static bool test_domain_attach(void) {
    atlas_domain_t* domain = atlas_domain_create(1024, 42);
    TEST_ASSERT(domain != NULL, "Domain creation should succeed");
    
    uint8_t buffer[1024];
    memset(buffer, 0, sizeof(buffer));
    
    // Test valid attach
    int result = atlas_domain_attach(domain, buffer, sizeof(buffer));
    TEST_ASSERT_EQ(result, 0, "Domain attach should succeed");
    TEST_ASSERT_EQ(atlas_get_last_error(), ATLAS_OK, "No error should occur");
    
    // Test double attach (should fail)
    result = atlas_domain_attach(domain, buffer, sizeof(buffer));
    TEST_ASSERT_EQ(result, -1, "Double attach should fail");
    TEST_ASSERT_EQ(atlas_get_last_error(), ATLAS_E_STATE, "Should set STATE error");
    
    atlas_domain_destroy(domain);
    
    // Test invalid parameters
    domain = atlas_domain_create(1024, 42);
    result = atlas_domain_attach(domain, NULL, 1024);
    TEST_ASSERT_EQ(result, -1, "Attach with NULL base should fail");
    TEST_ASSERT_EQ(atlas_get_last_error(), ATLAS_E_INVALID, "Should set INVALID error");
    
    result = atlas_domain_attach(domain, buffer, 0);
    TEST_ASSERT_EQ(result, -1, "Attach with zero length should fail");
    TEST_ASSERT_EQ(atlas_get_last_error(), ATLAS_E_INVALID, "Should set INVALID error");
    
    atlas_domain_destroy(domain);
    return true;
}

static bool test_domain_verify(void) {
    atlas_domain_t* domain = atlas_domain_create(256, 0);
    TEST_ASSERT(domain != NULL, "Domain creation should succeed");
    
    uint8_t buffer[256];
    memcpy(buffer, TEST_PATTERN_ZEROS, sizeof(buffer));
    
    int result = atlas_domain_attach(domain, buffer, sizeof(buffer));
    TEST_ASSERT_EQ(result, 0, "Domain attach should succeed");
    
    // Test verification
    bool verified = atlas_domain_verify(domain);
    TEST_ASSERT(verified, "Domain verification should succeed for conserved buffer");
    TEST_ASSERT_EQ(atlas_get_last_error(), ATLAS_OK, "No error should occur");
    
    atlas_domain_destroy(domain);
    
    // Test verification with NULL domain
    verified = atlas_domain_verify(NULL);
    TEST_ASSERT(!verified, "Verification with NULL domain should fail");
    TEST_ASSERT_EQ(atlas_get_last_error(), ATLAS_E_INVALID, "Should set INVALID error");
    
    return true;
}

static bool test_domain_commit(void) {
    atlas_domain_t* domain = atlas_domain_create(256, 0);
    TEST_ASSERT(domain != NULL, "Domain creation should succeed");
    
    uint8_t buffer[256];
    memcpy(buffer, TEST_PATTERN_ZEROS, sizeof(buffer));
    
    int result = atlas_domain_attach(domain, buffer, sizeof(buffer));
    TEST_ASSERT_EQ(result, 0, "Domain attach should succeed");
    
    // Test commit
    result = atlas_domain_commit(domain);
    TEST_ASSERT_EQ(result, 0, "Domain commit should succeed");
    TEST_ASSERT_EQ(atlas_get_last_error(), ATLAS_OK, "No error should occur");
    
    // Test double commit (should fail)
    result = atlas_domain_commit(domain);
    TEST_ASSERT_EQ(result, -1, "Double commit should fail");
    TEST_ASSERT_EQ(atlas_get_last_error(), ATLAS_E_STATE, "Should set STATE error");
    
    atlas_domain_destroy(domain);
    return true;
}

static bool test_domain_lifecycle(void) {
    // Test complete domain lifecycle
    atlas_domain_t* domain = atlas_domain_create(256, 50);
    TEST_ASSERT(domain != NULL, "Domain creation should succeed");
    
    uint8_t buffer[256];
    memcpy(buffer, TEST_PATTERN_ZEROS, sizeof(buffer));
    
    // Attach -> Verify -> Commit -> Destroy
    TEST_ASSERT_EQ(atlas_domain_attach(domain, buffer, sizeof(buffer)), 0, "Attach should succeed");
    TEST_ASSERT(atlas_domain_verify(domain), "Verify should succeed");
    TEST_ASSERT_EQ(atlas_domain_commit(domain), 0, "Commit should succeed");
    
    // Domain should still verify after commit
    TEST_ASSERT(atlas_domain_verify(domain), "Verify should succeed after commit");
    
    atlas_domain_destroy(domain);
    return true;
}

// =============================================================================
// Budget Management Tests
// =============================================================================

static bool test_budget_alloc_release(void) {
    atlas_domain_t* domain = atlas_domain_create(256, 50);
    TEST_ASSERT(domain != NULL, "Domain creation should succeed");
    
    // Test budget allocation
    bool result = atlas_budget_alloc(domain, 10);
    TEST_ASSERT(result, "Budget allocation should succeed");
    TEST_ASSERT_EQ(atlas_get_last_error(), ATLAS_OK, "No error should occur");
    
    // Test budget release
    result = atlas_budget_release(domain, 10);
    TEST_ASSERT(result, "Budget release should succeed");
    TEST_ASSERT_EQ(atlas_get_last_error(), ATLAS_OK, "No error should occur");
    
    // Test insufficient budget
    result = atlas_budget_alloc(domain, 60); // More than initial budget of 50
    TEST_ASSERT(!result, "Allocation beyond budget should fail");
    TEST_ASSERT_EQ(atlas_get_last_error(), ATLAS_E_BUDGET, "Should set BUDGET error");
    
    atlas_domain_destroy(domain);
    return true;
}

static bool test_budget_mod96_arithmetic(void) {
    atlas_domain_t* domain = atlas_domain_create(256, 95);
    TEST_ASSERT(domain != NULL, "Domain creation should succeed");
    
    // Test mod-96 wraparound on release
    bool result = atlas_budget_release(domain, 2);
    TEST_ASSERT(result, "Budget release should succeed");
    // Budget should now be (95 + 2) % 96 = 1
    
    // Should be able to allocate 1 but not 2
    result = atlas_budget_alloc(domain, 1);
    TEST_ASSERT(result, "Should be able to allocate 1");
    
    result = atlas_budget_alloc(domain, 1);
    TEST_ASSERT(!result, "Should not be able to allocate another 1");
    TEST_ASSERT_EQ(atlas_get_last_error(), ATLAS_E_BUDGET, "Should set BUDGET error");
    
    atlas_domain_destroy(domain);
    return true;
}

static bool test_budget_invalid_params(void) {
    atlas_domain_t* domain = atlas_domain_create(256, 50);
    TEST_ASSERT(domain != NULL, "Domain creation should succeed");
    
    // Test invalid allocation amount
    bool result = atlas_budget_alloc(domain, 96);
    TEST_ASSERT(!result, "Allocation of 96 should fail");
    TEST_ASSERT_EQ(atlas_get_last_error(), ATLAS_E_INVALID, "Should set INVALID error");
    
    // Test invalid release amount
    result = atlas_budget_release(domain, 200);
    TEST_ASSERT(!result, "Release of 200 should fail");
    TEST_ASSERT_EQ(atlas_get_last_error(), ATLAS_E_INVALID, "Should set INVALID error");
    
    // Test NULL domain
    result = atlas_budget_alloc(NULL, 10);
    TEST_ASSERT(!result, "Allocation on NULL domain should fail");
    TEST_ASSERT_EQ(atlas_get_last_error(), ATLAS_E_INVALID, "Should set INVALID error");
    
    atlas_domain_destroy(domain);
    return true;
}

// =============================================================================
// Witness Operation Tests
// =============================================================================

static bool test_witness_generation(void) {
    uint8_t buffer[256];
    memcpy(buffer, TEST_PATTERN_SEQUENTIAL, sizeof(buffer));
    
    // Test witness generation
    atlas_witness_t* witness = atlas_witness_generate(buffer, sizeof(buffer));
    TEST_ASSERT(witness != NULL, "Witness generation should succeed");
    TEST_ASSERT_EQ(atlas_get_last_error(), ATLAS_OK, "No error should occur");
    
    atlas_witness_destroy(witness);
    
    // Test invalid parameters
    witness = atlas_witness_generate(NULL, 256);
    TEST_ASSERT(witness == NULL, "Generation with NULL data should fail");
    TEST_ASSERT_EQ(atlas_get_last_error(), ATLAS_E_INVALID, "Should set INVALID error");
    
    witness = atlas_witness_generate(buffer, 0);
    TEST_ASSERT(witness == NULL, "Generation with zero length should fail");
    TEST_ASSERT_EQ(atlas_get_last_error(), ATLAS_E_INVALID, "Should set INVALID error");
    
    return true;
}

static bool test_witness_verification(void) {
    uint8_t buffer[256];
    memcpy(buffer, TEST_PATTERN_SEQUENTIAL, sizeof(buffer));
    
    atlas_witness_t* witness = atlas_witness_generate(buffer, sizeof(buffer));
    TEST_ASSERT(witness != NULL, "Witness generation should succeed");
    
    // Test successful verification
    bool result = atlas_witness_verify(witness, buffer, sizeof(buffer));
    TEST_ASSERT(result, "Witness verification should succeed");
    TEST_ASSERT_EQ(atlas_get_last_error(), ATLAS_OK, "No error should occur");
    
    // Test verification with modified data
    buffer[100] ^= 0xFF; // Flip all bits in one byte
    result = atlas_witness_verify(witness, buffer, sizeof(buffer));
    TEST_ASSERT(!result, "Verification with modified data should fail");
    TEST_ASSERT_EQ(atlas_get_last_error(), ATLAS_E_WITNESS, "Should set WITNESS error");
    
    // Restore buffer for cleanup
    buffer[100] ^= 0xFF;
    
    atlas_witness_destroy(witness);
    return true;
}

static bool test_witness_invalid_params(void) {
    uint8_t buffer[256];
    memcpy(buffer, TEST_PATTERN_ZEROS, sizeof(buffer));
    
    atlas_witness_t* witness = atlas_witness_generate(buffer, sizeof(buffer));
    TEST_ASSERT(witness != NULL, "Witness generation should succeed");
    
    // Test verification with NULL witness
    bool result = atlas_witness_verify(NULL, buffer, sizeof(buffer));
    TEST_ASSERT(!result, "Verification with NULL witness should fail");
    TEST_ASSERT_EQ(atlas_get_last_error(), ATLAS_E_INVALID, "Should set INVALID error");
    
    // Test verification with NULL data
    result = atlas_witness_verify(witness, NULL, sizeof(buffer));
    TEST_ASSERT(!result, "Verification with NULL data should fail");
    TEST_ASSERT_EQ(atlas_get_last_error(), ATLAS_E_INVALID, "Should set INVALID error");
    
    // Test verification with wrong length
    result = atlas_witness_verify(witness, buffer, sizeof(buffer) / 2);
    TEST_ASSERT(!result, "Verification with wrong length should fail");
    TEST_ASSERT_EQ(atlas_get_last_error(), ATLAS_E_INVALID, "Should set INVALID error");
    
    atlas_witness_destroy(witness);
    return true;
}

// =============================================================================
// Conservation Law Tests
// =============================================================================

static bool test_conserved_delta(void) {
    uint8_t before[256];
    uint8_t after[256];
    
    // Test zero delta (identical buffers)
    memcpy(before, TEST_PATTERN_ZEROS, sizeof(before));
    memcpy(after, TEST_PATTERN_ZEROS, sizeof(after));
    
    uint8_t delta = atlas_conserved_delta(before, after, sizeof(before));
    TEST_ASSERT_EQ(delta, 0, "Delta of identical buffers should be 0");
    
    // Test non-zero delta
    memcpy(before, TEST_PATTERN_ZEROS, sizeof(before));
    memcpy(after, TEST_PATTERN_ONES, sizeof(after));
    
    delta = atlas_conserved_delta(before, after, sizeof(before));
    // Delta should be (255 * 256) % 96 = 0 (since 255 = -1 mod 96, and 256 * -1 = -256 = 64 mod 96, but actually 0)
    // Actually: 255 * 256 = 65280, 65280 % 96 = 0
    TEST_ASSERT_EQ(delta, 0, "Delta should be 0 for this pattern");
    
    return true;
}

static bool test_runtime_info(void) {
    // Test thread safety query
    bool is_thread_safe = atlas_runtime_is_thread_safe();
    
#ifdef ATLAS_SINGLE_THREAD
    TEST_ASSERT(!is_thread_safe, "Runtime should report as single-threaded in WASM mode");
#else
    TEST_ASSERT(is_thread_safe, "Runtime should report as multi-threaded in normal mode");
#endif
    
    // Test error string function
    const char* error_str = atlas_error_string(ATLAS_OK);
    TEST_ASSERT(error_str != NULL, "Error string should not be NULL");
    TEST_ASSERT(strlen(error_str) > 0, "Error string should not be empty");
    
    error_str = atlas_error_string(ATLAS_E_INVALID);
    TEST_ASSERT(error_str != NULL, "Invalid error string should not be NULL");
    TEST_ASSERT(strstr(error_str, "Invalid") != NULL, "Error string should contain 'Invalid'");
    
    return true;
}

// =============================================================================
// Batch Processing Tests
// =============================================================================

static bool test_batch_conserved_check(void) {
    // Create test buffers
    atlas_batch_buffer_t buffers[3];
    uint8_t data1[256], data2[256], data3[256];
    
    memcpy(data1, TEST_PATTERN_ZEROS, sizeof(data1));
    memcpy(data2, TEST_PATTERN_ZEROS, sizeof(data2));
    memcpy(data3, TEST_PATTERN_ONES, sizeof(data3));
    
    buffers[0] = (atlas_batch_buffer_t){data1, sizeof(data1), 0, {0}};
    buffers[1] = (atlas_batch_buffer_t){data2, sizeof(data2), 0, {0}};
    buffers[2] = (atlas_batch_buffer_t){data3, sizeof(data3), 0, {0}};
    
    // Test batch conservation check
    uint8_t* results = atlas_batch_conserved_check(buffers, 3);
    TEST_ASSERT(results != NULL, "Batch conservation check should succeed");
    TEST_ASSERT_EQ(atlas_get_last_error(), ATLAS_OK, "No error should occur");
    
    // Check results (zeros pattern should be conserved, ones might not be)
    TEST_ASSERT_EQ(results[0], 1, "Zeros buffer should be conserved");
    TEST_ASSERT_EQ(results[1], 1, "Zeros buffer should be conserved");
    
    free(results);
    return true;
}

static bool test_batch_delta_compute(void) {
    atlas_batch_delta_t deltas[2];
    uint8_t before1[256], after1[256];
    uint8_t before2[256], after2[256];
    
    memcpy(before1, TEST_PATTERN_ZEROS, sizeof(before1));
    memcpy(after1, TEST_PATTERN_ZEROS, sizeof(after1));
    memcpy(before2, TEST_PATTERN_ZEROS, sizeof(before2));
    memcpy(after2, TEST_PATTERN_SEQUENTIAL, sizeof(after2));
    
    deltas[0] = (atlas_batch_delta_t){before1, after1, sizeof(before1), 0, {0}};
    deltas[1] = (atlas_batch_delta_t){before2, after2, sizeof(before2), 0, {0}};
    
    int result = atlas_batch_delta_compute(deltas, 2);
    TEST_ASSERT_EQ(result, 0, "Batch delta compute should succeed");
    TEST_ASSERT_EQ(atlas_get_last_error(), ATLAS_OK, "No error should occur");
    
    TEST_ASSERT_EQ(deltas[0].delta, 0, "Delta between identical buffers should be 0");
    
    return true;
}

static bool test_batch_get_optimal_size(void) {
    size_t optimal = atlas_batch_get_optimal_size(64);
    TEST_ASSERT(optimal > 0, "Optimal batch size should be positive");
    TEST_ASSERT(optimal <= 256, "Optimal batch size should not exceed maximum");
    
    optimal = atlas_batch_get_optimal_size(16384);
    TEST_ASSERT(optimal > 0, "Optimal batch size for large buffers should be positive");
    TEST_ASSERT(optimal <= 256, "Optimal batch size should not exceed maximum");
    
    return true;
}

// =============================================================================
// Thread Safety Tests (only when threads are available)
// =============================================================================

#ifndef ATLAS_SINGLE_THREAD
#include <pthread.h>

typedef struct {
    atlas_domain_t* domain;
    int thread_id;
    int iterations;
    bool success;
} thread_test_data_t;

static void* budget_stress_thread(void* arg) {
    thread_test_data_t* data = (thread_test_data_t*)arg;
    
    data->success = true;
    
    for (int i = 0; i < data->iterations && data->success; i++) {
        // Try to allocate small amounts
        if (atlas_budget_alloc(data->domain, 1)) {
            // Successfully allocated, now release it
            if (!atlas_budget_release(data->domain, 1)) {
                data->success = false;
                break;
            }
        }
        // If allocation failed, that's okay (insufficient budget)
    }
    
    return NULL;
}

static bool test_budget_thread_safety(void) {
    atlas_domain_t* domain = atlas_domain_create(256, 50);
    TEST_ASSERT(domain != NULL, "Domain creation should succeed");
    
    const int NUM_THREADS = 4;
    const int ITERATIONS = 100;
    
    pthread_t threads[NUM_THREADS];
    thread_test_data_t thread_data[NUM_THREADS];
    
    // Start threads
    for (int i = 0; i < NUM_THREADS; i++) {
        thread_data[i] = (thread_test_data_t){
            .domain = domain,
            .thread_id = i,
            .iterations = ITERATIONS,
            .success = false
        };
        
        int result = pthread_create(&threads[i], NULL, budget_stress_thread, &thread_data[i]);
        TEST_ASSERT_EQ(result, 0, "Thread creation should succeed");
    }
    
    // Wait for threads
    for (int i = 0; i < NUM_THREADS; i++) {
        pthread_join(threads[i], NULL);
        TEST_ASSERT(thread_data[i].success, "Thread should complete successfully");
    }
    
    atlas_domain_destroy(domain);
    return true;
}
#endif

// =============================================================================
// Error Handling Tests
// =============================================================================

static bool test_error_handling(void) {
    // Clear any previous error
    atlas_get_last_error();
    
    // Test error propagation
    atlas_domain_t* domain = atlas_domain_create(0, 50);
    TEST_ASSERT(domain == NULL, "Invalid domain creation should fail");
    TEST_ASSERT_EQ(atlas_get_last_error(), ATLAS_E_INVALID, "Error should be set");
    
    // Test error persistence
    TEST_ASSERT_EQ(atlas_get_last_error(), ATLAS_E_INVALID, "Error should persist until next operation");
    
    // Test error clearing on successful operation
    domain = atlas_domain_create(256, 50);
    TEST_ASSERT(domain != NULL, "Valid domain creation should succeed");
    TEST_ASSERT_EQ(atlas_get_last_error(), ATLAS_OK, "Error should be cleared");
    
    atlas_domain_destroy(domain);
    return true;
}

// =============================================================================
// Performance Tests
// =============================================================================

static bool test_performance_basic(void) {
    const int NUM_ITERATIONS = 1000;
    clock_t start, end;
    
    // Test domain creation/destruction performance
    start = clock();
    for (int i = 0; i < NUM_ITERATIONS; i++) {
        atlas_domain_t* domain = atlas_domain_create(256, i % 96);
        if (domain) {
            atlas_domain_destroy(domain);
        }
    }
    end = clock();
    
    double creation_time = ((double)(end - start)) / CLOCKS_PER_SEC;
    printf("\n  Domain creation/destruction: %d ops in %.3f seconds (%.0f ops/sec)\n", 
           NUM_ITERATIONS, creation_time, NUM_ITERATIONS / creation_time);
    
    // Test witness generation performance
    uint8_t buffer[256];
    memcpy(buffer, TEST_PATTERN_SEQUENTIAL, sizeof(buffer));
    
    start = clock();
    for (int i = 0; i < NUM_ITERATIONS / 10; i++) {  // Fewer iterations for witness ops
        atlas_witness_t* witness = atlas_witness_generate(buffer, sizeof(buffer));
        if (witness) {
            atlas_witness_destroy(witness);
        }
    }
    end = clock();
    
    double witness_time = ((double)(end - start)) / CLOCKS_PER_SEC;
    printf("  Witness generation: %d ops in %.3f seconds (%.0f ops/sec)\n", 
           NUM_ITERATIONS / 10, witness_time, (NUM_ITERATIONS / 10) / witness_time);
    
    return true;
}

// =============================================================================
// Main Test Runner
// =============================================================================

int main(void) {
    printf("Atlas-12288 Layer 2 Conservation Runtime Test Suite\n");
    printf("==================================================\n\n");
    
#ifdef ATLAS_SINGLE_THREAD
    printf("Running in single-threaded (WASM) mode\n\n");
#else
    printf("Running in multi-threaded mode\n\n");
#endif
    
    // Domain lifecycle tests
    printf("Domain Lifecycle Tests:\n");
    RUN_TEST(test_domain_creation);
    RUN_TEST(test_domain_attach);
    RUN_TEST(test_domain_verify);
    RUN_TEST(test_domain_commit);
    RUN_TEST(test_domain_lifecycle);
    printf("\n");
    
    // Budget management tests
    printf("Budget Management Tests:\n");
    RUN_TEST(test_budget_alloc_release);
    RUN_TEST(test_budget_mod96_arithmetic);
    RUN_TEST(test_budget_invalid_params);
    printf("\n");
    
    // Witness operation tests
    printf("Witness Operation Tests:\n");
    RUN_TEST(test_witness_generation);
    RUN_TEST(test_witness_verification);
    RUN_TEST(test_witness_invalid_params);
    printf("\n");
    
    // Conservation law tests
    printf("Conservation Law Tests:\n");
    RUN_TEST(test_conserved_delta);
    RUN_TEST(test_runtime_info);
    printf("\n");
    
    // Batch processing tests
    printf("Batch Processing Tests:\n");
    RUN_TEST(test_batch_conserved_check);
    RUN_TEST(test_batch_delta_compute);
    RUN_TEST(test_batch_get_optimal_size);
    printf("\n");
    
#ifndef ATLAS_SINGLE_THREAD
    // Thread safety tests (only in multi-threaded builds)
    printf("Thread Safety Tests:\n");
    RUN_TEST(test_budget_thread_safety);
    printf("\n");
#endif
    
    // Error handling tests
    printf("Error Handling Tests:\n");
    RUN_TEST(test_error_handling);
    printf("\n");
    
    // Performance tests
    printf("Performance Tests:\n");
    RUN_TEST(test_performance_basic);
    printf("\n");
    
    // Summary
    printf("Test Results Summary:\n");
    printf("====================\n");
    printf("Total tests: %d\n", total_tests);
    printf("Passed: %d\n", tests_passed);
    printf("Failed: %d\n", tests_failed);
    printf("Success rate: %.1f%%\n", (tests_passed * 100.0) / total_tests);
    
    if (tests_failed == 0) {
        printf("\nAll tests passed! ✓\n");
        return 0;
    } else {
        printf("\nSome tests failed. ✗\n");
        return 1;
    }
}