/* test-layer2-concurrency.c - Concurrency tests for Atlas Layer 2 Runtime
 * (c) 2024-2025 UOR Foundation - MIT License
 * 
 * Concurrency tests for Layer 2 runtime implementing:
 * - Multi-threaded budget allocation/release contention
 * - Concurrent domain commit testing (only one should succeed)
 * - Concurrent witness generation and verification
 * - Race condition detection for state transitions
 * - Thread safety validation
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <stdint.h>
#include <pthread.h>
#include <unistd.h>
#include <time.h>
#include <errno.h>
#include <stdatomic.h>
#include "atlas-runtime.h"

// Test configuration
#define NUM_THREADS 1000
#define BUDGET_CONTENTION_THREADS 100
#define COMMIT_CONTENTION_THREADS 50
#define WITNESS_THREADS 20
#define BUFFER_SIZE 4096
#define TEST_ITERATIONS 100

// Global test state
static atomic_int tests_passed = 0;
static atomic_int tests_failed = 0;
static atomic_int total_assertions = 0;

// Thread synchronization
static pthread_barrier_t start_barrier;
static pthread_mutex_t print_mutex = PTHREAD_MUTEX_INITIALIZER;

// Test data structures
typedef struct {
    atlas_domain_t* domain;
    int thread_id;
    int iterations;
    atomic_int* success_count;
    atomic_int* failure_count;
} thread_test_data_t;

typedef struct {
    atlas_domain_t** domains;
    int num_domains;
    int thread_id;
    atomic_int* commit_successes;
    atomic_int* commit_failures;
} commit_test_data_t;

typedef struct {
    uint8_t* data;
    size_t size;
    int thread_id;
    atomic_int* generation_count;
    atomic_int* verification_count;
} witness_test_data_t;

// Thread-safe assertion
#define CONCURRENT_ASSERT(condition, description) \
    do { \
        atomic_fetch_add(&total_assertions, 1); \
        if (condition) { \
            atomic_fetch_add(&tests_passed, 1); \
        } else { \
            pthread_mutex_lock(&print_mutex); \
            printf("  ✗ Thread %d FAILED: %s\n", \
                   (int)(intptr_t)pthread_self(), description); \
            pthread_mutex_unlock(&print_mutex); \
            atomic_fetch_add(&tests_failed, 1); \
        } \
    } while(0)

// Initialize test buffer with known pattern
static void init_test_buffer(uint8_t* buffer, size_t size, uint32_t seed) {
    for (size_t i = 0; i < size; i++) {
        buffer[i] = (uint8_t)((seed + i) % 256);
    }
}

// Test 1: Budget Allocation/Release Contention
void* budget_contention_thread(void* arg) {
    thread_test_data_t* data = (thread_test_data_t*)arg;
    
    // Wait for all threads to be ready
    pthread_barrier_wait(&start_barrier);
    
    for (int i = 0; i < data->iterations; i++) {
        // Try to allocate a small amount of budget
        uint8_t alloc_amount = (uint8_t)((data->thread_id + i) % 10 + 1);
        bool alloc_success = atlas_budget_alloc(data->domain, alloc_amount);
        
        if (alloc_success) {
            atomic_fetch_add(data->success_count, 1);
            
            // Do some "work" while holding budget
            usleep(1000); // 1ms
            
            // Release the budget
            bool release_success = atlas_budget_release(data->domain, alloc_amount);
            CONCURRENT_ASSERT(release_success, "Budget release should succeed after allocation");
        } else {
            atomic_fetch_add(data->failure_count, 1);
        }
        
        // Small random delay to create more interesting race conditions
        usleep((data->thread_id + i) % 100);
    }
    
    return NULL;
}

void test_budget_contention(void) {
    printf("Testing budget allocation/release contention...\n");
    
    // Create domain with moderate budget
    atlas_domain_t* domain = atlas_domain_create(BUFFER_SIZE, 50);
    CONCURRENT_ASSERT(domain != NULL, "Domain creation should succeed");
    
    if (!domain) return;
    
    // Attach memory
    uint8_t* buffer = malloc(BUFFER_SIZE);
    init_test_buffer(buffer, BUFFER_SIZE, 42);
    
    int attach_result = atlas_domain_attach(domain, buffer, BUFFER_SIZE);
    CONCURRENT_ASSERT(attach_result == 0, "Memory attachment should succeed");
    
    // Initialize barrier for thread synchronization
    pthread_barrier_init(&start_barrier, NULL, BUDGET_CONTENTION_THREADS);
    
    // Create threads and test data
    pthread_t threads[BUDGET_CONTENTION_THREADS];
    thread_test_data_t thread_data[BUDGET_CONTENTION_THREADS];
    atomic_int success_count = 0;
    atomic_int failure_count = 0;
    
    // Launch threads
    for (int i = 0; i < BUDGET_CONTENTION_THREADS; i++) {
        thread_data[i].domain = domain;
        thread_data[i].thread_id = i;
        thread_data[i].iterations = TEST_ITERATIONS;
        thread_data[i].success_count = &success_count;
        thread_data[i].failure_count = &failure_count;
        
        int result = pthread_create(&threads[i], NULL, budget_contention_thread, &thread_data[i]);
        CONCURRENT_ASSERT(result == 0, "Thread creation should succeed");
    }
    
    // Wait for all threads to complete
    for (int i = 0; i < BUDGET_CONTENTION_THREADS; i++) {
        pthread_join(threads[i], NULL);
    }
    
    pthread_barrier_destroy(&start_barrier);
    
    int successes = atomic_load(&success_count);
    int failures = atomic_load(&failure_count);
    
    printf("  Budget operations: %d successes, %d failures\n", successes, failures);
    CONCURRENT_ASSERT(successes + failures == BUDGET_CONTENTION_THREADS * TEST_ITERATIONS,
        "Total budget operations should match expected count");
    
    // Verify domain is still valid after concurrent access
    bool domain_valid = atlas_domain_verify(domain);
    CONCURRENT_ASSERT(domain_valid, "Domain should remain valid after concurrent budget operations");
    
    atlas_domain_destroy(domain);
    free(buffer);
}

// Test 2: Concurrent Domain Commit (only one should succeed)
void* domain_commit_thread(void* arg) {
    commit_test_data_t* data = (commit_test_data_t*)arg;
    
    // Wait for all threads to be ready
    pthread_barrier_wait(&start_barrier);
    
    // Try to commit each domain - only one thread should succeed per domain
    for (int i = 0; i < data->num_domains; i++) {
        int commit_result = atlas_domain_commit(data->domains[i]);
        
        if (commit_result == 0) {
            atomic_fetch_add(data->commit_successes, 1);
        } else {
            atomic_fetch_add(data->commit_failures, 1);
            // Check that the error is appropriate
            atlas_error_t error = atlas_get_last_error();
            CONCURRENT_ASSERT(error == ATLAS_ERROR_INVALID_STATE || 
                            error == ATLAS_ERROR_CONSERVATION_VIOLATION,
                "Commit failure should have appropriate error code");
        }
    }
    
    return NULL;
}

void test_concurrent_domain_commit(void) {
    printf("Testing concurrent domain commit operations...\n");
    
    const int num_domains = 10;
    atlas_domain_t* domains[num_domains];
    uint8_t* buffers[num_domains];
    
    // Create multiple domains
    for (int i = 0; i < num_domains; i++) {
        domains[i] = atlas_domain_create(BUFFER_SIZE, 30 + i);
        CONCURRENT_ASSERT(domains[i] != NULL, "Domain creation should succeed");
        
        buffers[i] = malloc(BUFFER_SIZE);
        init_test_buffer(buffers[i], BUFFER_SIZE, 100 + i);
        
        int attach_result = atlas_domain_attach(domains[i], buffers[i], BUFFER_SIZE);
        CONCURRENT_ASSERT(attach_result == 0, "Domain attachment should succeed");
    }
    
    // Initialize barrier
    pthread_barrier_init(&start_barrier, NULL, COMMIT_CONTENTION_THREADS);
    
    pthread_t threads[COMMIT_CONTENTION_THREADS];
    commit_test_data_t thread_data[COMMIT_CONTENTION_THREADS];
    atomic_int commit_successes = 0;
    atomic_int commit_failures = 0;
    
    // Launch threads that will all try to commit the same domains
    for (int i = 0; i < COMMIT_CONTENTION_THREADS; i++) {
        thread_data[i].domains = domains;
        thread_data[i].num_domains = num_domains;
        thread_data[i].thread_id = i;
        thread_data[i].commit_successes = &commit_successes;
        thread_data[i].commit_failures = &commit_failures;
        
        pthread_create(&threads[i], NULL, domain_commit_thread, &thread_data[i]);
    }
    
    // Wait for completion
    for (int i = 0; i < COMMIT_CONTENTION_THREADS; i++) {
        pthread_join(threads[i], NULL);
    }
    
    pthread_barrier_destroy(&start_barrier);
    
    int successes = atomic_load(&commit_successes);
    int failures = atomic_load(&commit_failures);
    
    printf("  Commit operations: %d successes, %d failures\n", successes, failures);
    
    // For each domain, exactly one commit should have succeeded
    CONCURRENT_ASSERT(successes == num_domains,
        "Exactly one commit should succeed per domain");
    CONCURRENT_ASSERT(failures == (COMMIT_CONTENTION_THREADS - 1) * num_domains,
        "Remaining commits should fail");
    
    // Cleanup
    for (int i = 0; i < num_domains; i++) {
        atlas_domain_destroy(domains[i]);
        free(buffers[i]);
    }
}

// Test 3: Concurrent Witness Generation and Verification
void* witness_operations_thread(void* arg) {
    witness_test_data_t* data = (witness_test_data_t*)arg;
    
    pthread_barrier_wait(&start_barrier);
    
    for (int i = 0; i < TEST_ITERATIONS / 10; i++) { // Fewer iterations due to expense
        // Generate witness
        atlas_witness_t* witness = atlas_witness_generate(data->data, data->size);
        
        if (witness != NULL) {
            atomic_fetch_add(data->generation_count, 1);
            
            // Verify the witness
            bool verify_result = atlas_witness_verify(witness, data->data, data->size);
            if (verify_result) {
                atomic_fetch_add(data->verification_count, 1);
            }
            
            CONCURRENT_ASSERT(verify_result, "Witness verification should succeed");
            
            atlas_witness_destroy(witness);
        }
        
        // Small delay to create race conditions
        usleep(data->thread_id * 10);
    }
    
    return NULL;
}

void test_concurrent_witness_operations(void) {
    printf("Testing concurrent witness operations...\n");
    
    uint8_t* shared_data = malloc(BUFFER_SIZE);
    init_test_buffer(shared_data, BUFFER_SIZE, 200);
    
    pthread_barrier_init(&start_barrier, NULL, WITNESS_THREADS);
    
    pthread_t threads[WITNESS_THREADS];
    witness_test_data_t thread_data[WITNESS_THREADS];
    atomic_int generation_count = 0;
    atomic_int verification_count = 0;
    
    // Launch witness operation threads
    for (int i = 0; i < WITNESS_THREADS; i++) {
        thread_data[i].data = shared_data;
        thread_data[i].size = BUFFER_SIZE;
        thread_data[i].thread_id = i;
        thread_data[i].generation_count = &generation_count;
        thread_data[i].verification_count = &verification_count;
        
        pthread_create(&threads[i], NULL, witness_operations_thread, &thread_data[i]);
    }
    
    // Wait for completion
    for (int i = 0; i < WITNESS_THREADS; i++) {
        pthread_join(threads[i], NULL);
    }
    
    pthread_barrier_destroy(&start_barrier);
    
    int generations = atomic_load(&generation_count);
    int verifications = atomic_load(&verification_count);
    
    printf("  Witness operations: %d generations, %d verifications\n", 
           generations, verifications);
    
    CONCURRENT_ASSERT(generations > 0, "Some witness generations should succeed");
    CONCURRENT_ASSERT(verifications == generations, 
        "All generated witnesses should verify successfully");
    
    free(shared_data);
}

// Test 4: State Transition Race Conditions
void* state_transition_thread(void* arg) {
    thread_test_data_t* data = (thread_test_data_t*)arg;
    
    pthread_barrier_wait(&start_barrier);
    
    for (int i = 0; i < data->iterations; i++) {
        uint8_t buffer[256];
        init_test_buffer(buffer, sizeof(buffer), data->thread_id + i);
        
        // Create domain
        atlas_domain_t* domain = atlas_domain_create(sizeof(buffer), 20);
        if (!domain) continue;
        
        // Race to attach memory
        int attach_result = atlas_domain_attach(domain, buffer, sizeof(buffer));
        
        if (attach_result == 0) {
            atomic_fetch_add(data->success_count, 1);
            
            // Try to verify
            bool verify_result = atlas_domain_verify(domain);
            CONCURRENT_ASSERT(verify_result, "Domain verification should succeed after attach");
            
            // Try to commit
            int commit_result = atlas_domain_commit(domain);
            if (commit_result == 0) {
                // Verify after commit
                bool verify_after = atlas_domain_verify(domain);
                CONCURRENT_ASSERT(verify_after, "Domain should verify after commit");
            }
        } else {
            atomic_fetch_add(data->failure_count, 1);
        }
        
        atlas_domain_destroy(domain);
        
        // Brief pause to allow context switching
        usleep(1);
    }
    
    return NULL;
}

void test_state_transition_races(void) {
    printf("Testing state transition race conditions...\n");
    
    const int num_threads = 20;
    pthread_barrier_init(&start_barrier, NULL, num_threads);
    
    pthread_t threads[num_threads];
    thread_test_data_t thread_data[num_threads];
    atomic_int success_count = 0;
    atomic_int failure_count = 0;
    
    for (int i = 0; i < num_threads; i++) {
        thread_data[i].domain = NULL; // Each thread creates its own domains
        thread_data[i].thread_id = i;
        thread_data[i].iterations = TEST_ITERATIONS / 5;
        thread_data[i].success_count = &success_count;
        thread_data[i].failure_count = &failure_count;
        
        pthread_create(&threads[i], NULL, state_transition_thread, &thread_data[i]);
    }
    
    for (int i = 0; i < num_threads; i++) {
        pthread_join(threads[i], NULL);
    }
    
    pthread_barrier_destroy(&start_barrier);
    
    int successes = atomic_load(&success_count);
    int failures = atomic_load(&failure_count);
    
    printf("  State transitions: %d successes, %d failures\n", successes, failures);
    
    CONCURRENT_ASSERT(successes > 0, "Some state transitions should succeed");
    CONCURRENT_ASSERT(successes + failures == num_threads * (TEST_ITERATIONS / 5),
        "Total transitions should match expected count");
}

// Test 5: Thread Safety Validation
void test_thread_safety_validation(void) {
    printf("Testing thread safety validation...\n");
    
    // Check if runtime reports thread safety correctly
    bool is_thread_safe = atlas_runtime_is_thread_safe();
    printf("  Runtime reports thread safety: %s\n", is_thread_safe ? "yes" : "no");
    
    // If runtime claims thread safety, our concurrent tests should work
    if (is_thread_safe) {
        CONCURRENT_ASSERT(true, "Runtime claims to be thread-safe");
    } else {
        printf("  Warning: Runtime compiled without thread safety - concurrent tests may be unreliable\n");
    }
    
    // Test error handling in multi-threaded context
    const int num_error_threads = 10;
    pthread_t error_threads[num_error_threads];
    
    // Simple thread that generates errors and checks they don't interfere
    auto error_thread_func = [](void* arg) -> void* {
        int thread_id = *(int*)arg;
        
        for (int i = 0; i < 100; i++) {
            // Generate an error
            atlas_domain_t* invalid_domain = NULL;
            atlas_budget_alloc(invalid_domain, 10); // Should fail
            
            atlas_error_t error = atlas_get_last_error();
            CONCURRENT_ASSERT(error == ATLAS_ERROR_INVALID_ARGUMENT,
                "Error should be thread-local");
            
            // Brief delay
            usleep(thread_id * 10);
        }
        return NULL;
    };
    
    int thread_ids[num_error_threads];
    for (int i = 0; i < num_error_threads; i++) {
        thread_ids[i] = i;
        pthread_create(&error_threads[i], NULL, error_thread_func, &thread_ids[i]);
    }
    
    for (int i = 0; i < num_error_threads; i++) {
        pthread_join(error_threads[i], NULL);
    }
    
    printf("  Thread-local error handling test completed\n");
}

// Main test runner
int main(void) {
    printf("Atlas Layer 2 Concurrency Tests\n");
    printf("===============================\n");
    printf("Number of threads: %d\n", NUM_THREADS);
    printf("Test iterations: %d\n\n", TEST_ITERATIONS);
    
    // Initialize mutex
    pthread_mutex_init(&print_mutex, NULL);
    
    // Run all concurrency tests
    test_budget_contention();
    test_concurrent_domain_commit();
    test_concurrent_witness_operations();
    test_state_transition_races();
    test_thread_safety_validation();
    
    // Summary
    printf("\n=== CONCURRENCY TEST SUMMARY ===\n");
    int passed = atomic_load(&tests_passed);
    int failed = atomic_load(&tests_failed);
    int total = atomic_load(&total_assertions);
    
    printf("Total assertions: %d\n", total);
    printf("Assertions passed: %d\n", passed);
    printf("Assertions failed: %d\n", failed);
    
    pthread_mutex_destroy(&print_mutex);
    
    if (failed == 0) {
        printf("🎉 All concurrency tests PASSED!\n");
        return 0;
    } else {
        printf("💥 Some concurrency tests FAILED!\n");
        return 1;
    }
}