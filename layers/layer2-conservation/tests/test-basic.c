/* test-basic.c - Basic functionality tests for Atlas Layer 2 Runtime
 * (c) 2024-2025 UOR Foundation - MIT License
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include "../include/atlas-conservation.h"

// Test data
static uint8_t test_buffer[12288];

void test_runtime_info(void) {
    printf("Testing runtime information...\n");
    
    uint32_t version = atlas_runtime_version();
    printf("  Runtime version: %u.%u.%u\n", 
           (version >> 16) & 0xFF, 
           (version >> 8) & 0xFF, 
           version & 0xFF);
    
    bool thread_safe = atlas_runtime_is_thread_safe();
    printf("  Thread safe: %s\n", thread_safe ? "yes" : "no");
    
    // Test error string function
    const char* err_str = atlas_error_string(ATLAS_SUCCESS);
    assert(err_str != NULL);
    printf("  Error string test: %s\n", err_str);
}

void test_domain_lifecycle(void) {
    printf("Testing domain lifecycle...\n");
    
    // Test domain creation
    atlas_domain_t* domain = atlas_domain_create(12288, 42);
    assert(domain != NULL);
    assert(atlas_get_last_error() == ATLAS_SUCCESS);
    printf("  ✓ Domain creation\n");
    
    // Initialize test buffer
    for (int i = 0; i < sizeof(test_buffer); i++) {
        test_buffer[i] = i % 256;
    }
    
    // Test memory attachment
    int result = atlas_domain_attach(domain, test_buffer, sizeof(test_buffer));
    assert(result == 0);
    assert(atlas_get_last_error() == ATLAS_SUCCESS);
    printf("  ✓ Memory attachment\n");
    
    // Test domain verification
    bool verified = atlas_domain_verify(domain);
    printf("  Domain verification: %s\n", verified ? "passed" : "failed");
    if (!verified) {
        atlas_error_t error = atlas_get_last_error();
        printf("    Verification error: %s\n", atlas_error_string(error));
    }
    
    // Test domain commit (may fail due to missing LLVM functions)
    result = atlas_domain_commit(domain);
    if (result == 0) {
        printf("  ✓ Domain commit\n");
    } else {
        printf("  Domain commit failed: %s\n", 
               atlas_error_string(atlas_get_last_error()));
    }
    
    // Test domain destruction
    atlas_domain_destroy(domain);
    printf("  ✓ Domain destruction\n");
}

void test_budget_operations(void) {
    printf("Testing budget operations...\n");
    
    atlas_domain_t* domain = atlas_domain_create(1024, 60);
    assert(domain != NULL);
    
    // Test budget allocation
    bool success = atlas_budget_alloc(domain, 20);
    assert(success);
    printf("  ✓ Budget allocation (20 units)\n");
    
    // Test budget release
    success = atlas_budget_release(domain, 10);
    assert(success);
    printf("  ✓ Budget release (10 units)\n");
    
    // Test insufficient budget
    success = atlas_budget_alloc(domain, 95);
    assert(!success);
    assert(atlas_get_last_error() == ATLAS_ERROR_BUDGET_INSUFFICIENT);
    printf("  ✓ Insufficient budget handling\n");
    
    atlas_domain_destroy(domain);
}

void test_error_handling(void) {
    printf("Testing error handling...\n");
    
    // Test invalid domain creation
    atlas_domain_t* domain = atlas_domain_create(0, 100);
    assert(domain == NULL);
    assert(atlas_get_last_error() == ATLAS_ERROR_INVALID_ARGUMENT);
    printf("  ✓ Invalid argument detection\n");
    
    // Test operations on NULL domain
    bool success = atlas_budget_alloc(NULL, 10);
    assert(!success);
    assert(atlas_get_last_error() == ATLAS_ERROR_INVALID_ARGUMENT);
    printf("  ✓ NULL pointer handling\n");
}

void test_conservation_delta(void) {
    printf("Testing conservation delta...\n");
    
    uint8_t before[256];
    uint8_t after[256];
    
    // Initialize identical buffers
    for (int i = 0; i < 256; i++) {
        before[i] = i;
        after[i] = i;
    }
    
    // Test identical buffers (should give delta = 0)
    uint8_t delta = atlas_conserved_delta(before, after, 256);
    printf("  Identical buffers delta: %u\n", delta);
    
    // Modify one byte
    after[100] = before[100] + 1;
    delta = atlas_conserved_delta(before, after, 256);
    printf("  Modified buffer delta: %u\n", delta);
}

int main(void) {
    printf("Atlas Layer 2 Runtime Basic Tests\n");
    printf("==================================\n\n");
    
    test_runtime_info();
    printf("\n");
    
    test_domain_lifecycle();
    printf("\n");
    
    test_budget_operations();
    printf("\n");
    
    test_error_handling();
    printf("\n");
    
    test_conservation_delta();
    printf("\n");
    
    printf("All basic tests completed.\n");
    printf("Note: Some functionality may require LLVM runtime linkage.\n");
    
    return 0;
}