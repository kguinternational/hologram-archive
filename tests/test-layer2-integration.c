/**
 * test-layer2-integration.c - Layer 2 Integration Tests
 * 
 * Comprehensive integration tests validating Layer 2 functionality
 * including domain management, conservation laws, witness generation,
 * and interaction with Layer 3 operations.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>
#include <assert.h>
#include <time.h>
#include "../include/atlas.h"

// Test utilities
#define TEST_ASSERT(cond, msg) do { \
    if (!(cond)) { \
        fprintf(stderr, "❌ FAIL: %s\n   at %s:%d\n", msg, __FILE__, __LINE__); \
        return false; \
    } \
} while(0)

#define TEST_LOG(fmt, ...) printf("   " fmt "\n", ##__VA_ARGS__)

// Generate test data with known conservation properties
static void generate_conserved_data(uint8_t* data, size_t size, uint32_t seed) {
    // Use simple PRNG for reproducibility
    uint32_t state = seed;
    for (size_t i = 0; i < size; i++) {
        state = state * 1103515245 + 12345;
        data[i] = (uint8_t)(state >> 16);
    }
    
    // Ensure conservation (sum % 96 == 0)
    uint32_t sum = 0;
    for (size_t i = 0; i < size; i++) {
        sum += data[i];
    }
    uint8_t deficit = sum % 96;
    if (deficit > 0 && size > 0) {
        data[size-1] = (data[size-1] + 96 - deficit) % 256;
    }
}

// Test 1: Complete domain lifecycle with conservation
static bool test_domain_lifecycle(void) {
    TEST_LOG("Testing domain lifecycle with conservation...");
    
    // Create domain with budget class 42
    atlas_domain_t* domain = atlas_domain_create(12288, 42);
    TEST_ASSERT(domain != NULL, "Failed to create domain");
    
    // Create test data
    uint8_t* data = malloc(12288);
    TEST_ASSERT(data != NULL, "Failed to allocate test data");
    generate_conserved_data(data, 12288, 0xDEADBEEF);
    
    // Attach data to domain
    int err = atlas_domain_attach(domain, data, 12288);
    TEST_ASSERT(err == ATLAS_OK, "Failed to attach data to domain");
    
    // Verify conservation
    bool conserved = atlas_domain_verify(domain);
    TEST_ASSERT(conserved, "Domain conservation verification failed");
    
    // Test budget allocation
    bool alloc_ok = atlas_budget_alloc(domain, 10);
    TEST_ASSERT(alloc_ok, "Budget allocation failed");
    
    bool alloc_fail = atlas_budget_alloc(domain, 100); // Should fail - exceeds budget
    TEST_ASSERT(!alloc_fail, "Budget over-allocation should have failed");
    
    // Release some budget
    bool release_ok = atlas_budget_release(domain, 5);
    TEST_ASSERT(release_ok, "Budget release failed");
    
    // Commit domain
    err = atlas_domain_commit(domain);
    TEST_ASSERT(err == ATLAS_OK, "Domain commit failed");
    
    // Second commit should fail (already committed)
    err = atlas_domain_commit(domain);
    TEST_ASSERT(err == ATLAS_E_STATE, "Second commit should fail with state error");
    
    // Clean up
    atlas_domain_destroy(domain);
    free(data);
    
    TEST_LOG("✓ Domain lifecycle test passed");
    return true;
}

// Test 2: Witness generation and verification
static bool test_witness_operations(void) {
    TEST_LOG("Testing witness generation and verification...");
    
    // Create test data
    uint8_t* data = malloc(12288);
    TEST_ASSERT(data != NULL, "Failed to allocate test data");
    generate_conserved_data(data, 12288, 0xCAFEBABE);
    
    // Generate witness
    atlas_witness_t* witness = atlas_witness_generate(data, 12288);
    TEST_ASSERT(witness != NULL, "Failed to generate witness");
    
    // Verify witness with same data
    bool valid = atlas_witness_verify(witness, data, 12288);
    TEST_ASSERT(valid, "Witness verification failed with original data");
    
    // Modify data and verify witness should fail
    data[100] = (data[100] + 1) % 256;
    bool invalid = atlas_witness_verify(witness, data, 12288);
    TEST_ASSERT(!invalid, "Witness verification should fail with modified data");
    
    // Restore data and verify again
    data[100] = (data[100] - 1 + 256) % 256;
    valid = atlas_witness_verify(witness, data, 12288);
    TEST_ASSERT(valid, "Witness verification failed after restoration");
    
    // Clean up
    atlas_witness_destroy(witness);
    free(data);
    
    TEST_LOG("✓ Witness operations test passed");
    return true;
}

// Test 3: Conservation delta computation
static bool test_conservation_delta(void) {
    TEST_LOG("Testing conservation delta computation...");
    
    // Create before and after states
    uint8_t* before = malloc(12288);
    uint8_t* after = malloc(12288);
    TEST_ASSERT(before != NULL && after != NULL, "Failed to allocate buffers");
    
    // Generate initial conserved data
    generate_conserved_data(before, 12288, 0x12345678);
    memcpy(after, before, 12288);
    
    // Test 1: No change should give delta 0
    uint8_t delta = atlas_conserved_delta(before, after, 12288);
    TEST_ASSERT(delta == 0, "Delta should be 0 for identical buffers");
    
    // Test 2: Single byte change
    after[1000] = (after[1000] + 10) % 256;
    after[2000] = (after[2000] - 10 + 256) % 256; // Maintain conservation
    delta = atlas_conserved_delta(before, after, 12288);
    TEST_ASSERT(delta == 0, "Delta should be 0 for conserved modification");
    
    // Test 3: Non-conserved change
    after[3000] = (after[3000] + 5) % 256;
    delta = atlas_conserved_delta(before, after, 12288);
    TEST_ASSERT(delta == 5, "Delta should be 5 for non-conserved change");
    
    // Clean up
    free(before);
    free(after);
    
    TEST_LOG("✓ Conservation delta test passed");
    return true;
}

// Test 4: Integration with Layer 3 resonance operations
static bool test_layer3_integration(void) {
    TEST_LOG("Testing Layer 2/3 integration...");
    
    // Create domain
    atlas_domain_t* domain = atlas_domain_create(12288, 50);
    TEST_ASSERT(domain != NULL, "Failed to create domain");
    
    // Create test data
    uint8_t* data = malloc(12288);
    TEST_ASSERT(data != NULL, "Failed to allocate test data");
    generate_conserved_data(data, 12288, 0xABCDEF00);
    
    // Attach to domain
    int err = atlas_domain_attach(domain, data, 12288);
    TEST_ASSERT(err == ATLAS_OK, "Failed to attach data");
    
    // Generate resonance histogram (Layer 3 operation)
    uint16_t total_histogram[96] = {0};
    for (int page = 0; page < 48; page++) {
        uint16_t page_histogram[96];
        atlas_r96_histogram_page(&data[page * 256], page_histogram);
        for (int i = 0; i < 96; i++) {
            total_histogram[i] += page_histogram[i];
        }
    }
    
    // Verify histogram totals
    uint32_t total = 0;
    for (int i = 0; i < 96; i++) {
        total += total_histogram[i];
    }
    TEST_ASSERT(total == 12288, "Histogram total should equal data size");
    
    // Build clusters (Layer 3 operation)
    atlas_cluster_view clusters = atlas_cluster_by_resonance(data, 48);
    TEST_ASSERT(clusters.n == 12288, "Cluster count should equal data size");
    
    // Verify domain is still conserved after Layer 3 operations
    bool conserved = atlas_domain_verify(domain);
    TEST_ASSERT(conserved, "Domain should remain conserved after Layer 3 ops");
    
    // Commit domain with Layer 3 data processed
    err = atlas_domain_commit(domain);
    TEST_ASSERT(err == ATLAS_OK, "Domain commit failed");
    
    // Generate witness for the processed data
    atlas_witness_t* witness = atlas_witness_generate(data, 12288);
    TEST_ASSERT(witness != NULL, "Failed to generate witness");
    
    // Verify witness
    bool valid = atlas_witness_verify(witness, data, 12288);
    TEST_ASSERT(valid, "Witness verification failed");
    
    // Clean up
    atlas_witness_destroy(witness);
    atlas_cluster_destroy(&clusters);
    atlas_domain_destroy(domain);
    free(data);
    
    TEST_LOG("✓ Layer 2/3 integration test passed");
    return true;
}

// Test 5: Error handling and edge cases
static bool test_error_handling(void) {
    TEST_LOG("Testing error handling and edge cases...");
    
    // Test NULL pointer handling
    atlas_domain_destroy(NULL); // Should not crash
    atlas_witness_destroy(NULL); // Should not crash
    
    // Test invalid parameters
    atlas_domain_t* domain = atlas_domain_create(0, 42); // Zero size
    TEST_ASSERT(domain == NULL, "Should fail with zero size");
    
    domain = atlas_domain_create(12288, 100); // Invalid budget class
    TEST_ASSERT(domain == NULL, "Should fail with invalid budget class");
    
    // Test empty buffer operations
    uint8_t empty[1] = {0};
    uint8_t delta = atlas_conserved_delta(empty, empty, 0);
    TEST_ASSERT(delta == 0, "Empty buffer delta should be 0");
    
    // Test witness with NULL data
    atlas_witness_t* witness = atlas_witness_generate(NULL, 100);
    TEST_ASSERT(witness == NULL, "Should fail with NULL data");
    
    // Test domain operations in wrong state
    domain = atlas_domain_create(12288, 42);
    TEST_ASSERT(domain != NULL, "Failed to create domain");
    
    // Try to commit without attaching data
    int err = atlas_domain_commit(domain);
    TEST_ASSERT(err != ATLAS_OK, "Commit should fail without attached data");
    
    atlas_domain_destroy(domain);
    
    TEST_LOG("✓ Error handling test passed");
    return true;
}

// Test 6: Performance characteristics
static bool test_performance_characteristics(void) {
    TEST_LOG("Testing performance characteristics...");
    
    // Allocate large buffer for performance testing
    size_t size = 12288 * 10; // 10 Atlas structures
    uint8_t* data = malloc(size);
    TEST_ASSERT(data != NULL, "Failed to allocate performance test data");
    generate_conserved_data(data, size, 0x98765432);
    
    // Measure witness generation time
    clock_t start = clock();
    atlas_witness_t* witness = atlas_witness_generate(data, size);
    clock_t end = clock();
    double witness_time = ((double)(end - start)) / CLOCKS_PER_SEC;
    TEST_ASSERT(witness != NULL, "Failed to generate witness");
    
    // Calculate throughput
    double throughput_mbps = (size / (1024.0 * 1024.0)) / witness_time;
    TEST_LOG("Witness generation: %.2f MB/s", throughput_mbps);
    
    // Measure witness verification time
    start = clock();
    bool valid = atlas_witness_verify(witness, data, size);
    end = clock();
    double verify_time = ((double)(end - start)) / CLOCKS_PER_SEC;
    TEST_ASSERT(valid, "Witness verification failed");
    
    throughput_mbps = (size / (1024.0 * 1024.0)) / verify_time;
    TEST_LOG("Witness verification: %.2f MB/s", throughput_mbps);
    
    // Measure delta computation
    uint8_t* data2 = malloc(size);
    TEST_ASSERT(data2 != NULL, "Failed to allocate second buffer");
    memcpy(data2, data, size);
    
    start = clock();
    uint8_t delta = atlas_conserved_delta(data, data2, size);
    end = clock();
    double delta_time = ((double)(end - start)) / CLOCKS_PER_SEC;
    
    double ns_per_byte = (delta_time * 1e9) / size;
    TEST_LOG("Delta computation: %.2f ns/byte", ns_per_byte);
    TEST_ASSERT(delta == 0, "Delta should be 0 for identical buffers");
    
    // Clean up
    atlas_witness_destroy(witness);
    free(data);
    free(data2);
    
    TEST_LOG("✓ Performance characteristics test passed");
    return true;
}

// Main test runner
int main(void) {
    printf("========================================\n");
    printf("Atlas-12288 Layer 2 Integration Tests\n");
    printf("========================================\n\n");
    
    int passed = 0;
    int failed = 0;
    
    typedef struct {
        const char* name;
        bool (*test_fn)(void);
    } test_case_t;
    
    test_case_t tests[] = {
        {"Domain Lifecycle", test_domain_lifecycle},
        {"Witness Operations", test_witness_operations},
        {"Conservation Delta", test_conservation_delta},
        {"Layer 2/3 Integration", test_layer3_integration},
        {"Error Handling", test_error_handling},
        {"Performance Characteristics", test_performance_characteristics},
    };
    
    size_t num_tests = sizeof(tests) / sizeof(tests[0]);
    
    for (size_t i = 0; i < num_tests; i++) {
        printf("Test %zu: %s\n", i + 1, tests[i].name);
        
        bool result = tests[i].test_fn();
        if (result) {
            passed++;
        } else {
            failed++;
            printf("   ❌ Test failed\n");
        }
        printf("\n");
    }
    
    printf("========================================\n");
    printf("Results: %d passed, %d failed\n", passed, failed);
    if (failed == 0) {
        printf("✅ All Layer 2 integration tests passed!\n");
    } else {
        printf("❌ Some tests failed. Please review the output.\n");
    }
    printf("========================================\n");
    
    return failed > 0 ? 1 : 0;
}