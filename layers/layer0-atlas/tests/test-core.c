/* test-core.c - Core functionality tests for Atlas Layer 0
 * (c) 2024-2025 UOR Foundation - MIT License
 * 
 * Tests only the functions that are actually implemented in Layer 0.
 * Layer 0 mainly provides stub implementations for testing higher layers.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

// Test data
static uint8_t test_buffer[1024];

// Forward declarations of stub functions we can test
extern bool atlas_conserved_check(const void* data, size_t length);
extern uint32_t atlas_conserved_sum(const void* data, size_t length);
extern void* atlas_witness_generate_llvm(const void* data, size_t length);
extern bool atlas_witness_verify_llvm(void* witness, const void* data, size_t length);
extern void atlas_witness_destroy_llvm(void* witness);
extern bool atlas_runtime_using_stubs(void);
extern void atlas_r96_classify_page_llvm(const uint8_t* in256, uint8_t out256[256]);
extern void atlas_r96_histogram_page_llvm(const uint8_t* in256, uint16_t out96[96]);
extern uint8_t atlas_page_resonance_class_llvm(const uint8_t* page256);

void test_stub_detection(void) {
    printf("Testing stub detection...\n");
    
    bool using_stubs = atlas_runtime_using_stubs();
    printf("  Using stubs: %s\n", using_stubs ? "yes" : "no");
    
    // We expect stubs to be used in Layer 0
    assert(using_stubs);
    printf("  ✓ Stub detection working\n");
}

void test_conservation_stubs(void) {
    printf("Testing conservation stub operations...\n");
    
    // Initialize test data with pattern
    for (int i = 0; i < sizeof(test_buffer); i++) {
        test_buffer[i] = (i * 7 + 23) % 256;
    }
    
    // Test conservation check (should always return true in stubs)
    bool conserved = atlas_conserved_check(test_buffer, sizeof(test_buffer));
    printf("  Conservation check result: %s\n", conserved ? "conserved" : "not conserved");
    assert(conserved); // Stub always returns true
    printf("  ✓ Conservation check stub working\n");
    
    // Test conservation sum
    uint32_t sum = atlas_conserved_sum(test_buffer, sizeof(test_buffer));
    printf("  Conservation sum: %u\n", sum);
    
    // Test with zero buffer
    memset(test_buffer, 0, sizeof(test_buffer));
    sum = atlas_conserved_sum(test_buffer, sizeof(test_buffer));
    assert(sum == 0);
    printf("  ✓ Zero buffer conservation sum is zero\n");
    
    // Test with all 255 buffer
    memset(test_buffer, 255, sizeof(test_buffer));
    sum = atlas_conserved_sum(test_buffer, sizeof(test_buffer));
    printf("  All 255s conservation sum: %u\n", sum);
    assert(sum == (uint32_t)(255 * sizeof(test_buffer)));
    printf("  ✓ Conservation sum calculation correct\n");
}

void test_witness_stubs(void) {
    printf("Testing witness stub operations...\n");
    
    // Initialize test data
    for (int i = 0; i < 256; i++) {
        test_buffer[i] = i;
    }
    
    // Test witness generation
    void* witness = atlas_witness_generate_llvm(test_buffer, 256);
    if (witness == NULL) {
        printf("  Error: Witness generation returned NULL\n");
        return;
    }
    printf("  ✓ Witness generation\n");
    
    // Test witness verification with same data
    bool verified = atlas_witness_verify_llvm(witness, test_buffer, 256);
    printf("  Witness verification (same data): %s\n", verified ? "passed" : "failed");
    assert(verified);
    printf("  ✓ Witness verification with unchanged data\n");
    
    // Test witness verification after modification
    uint8_t original_value = test_buffer[100];
    test_buffer[100] = original_value + 1;
    verified = atlas_witness_verify_llvm(witness, test_buffer, 256);
    printf("  Witness verification (modified data): %s\n", verified ? "passed" : "failed");
    assert(!verified); // Should fail with modified data
    printf("  ✓ Witness verification correctly detects changes\n");
    
    // Restore original data
    test_buffer[100] = original_value;
    
    // Clean up witness
    atlas_witness_destroy_llvm(witness);
    printf("  ✓ Witness cleanup\n");
}

void test_r96_classification_stubs(void) {
    printf("Testing R96 classification stubs...\n");
    
    uint8_t page[256];
    uint8_t classification[256];
    uint16_t histogram[96];
    
    // Initialize test page
    for (int i = 0; i < 256; i++) {
        page[i] = i;
    }
    
    // Test page classification
    atlas_r96_classify_page_llvm(page, classification);
    
    // Check all results are in valid range (0-95)
    bool all_valid = true;
    for (int i = 0; i < 256; i++) {
        if (classification[i] >= 96) {
            all_valid = false;
            printf("  Invalid classification at %d: %u\n", i, classification[i]);
            break;
        }
    }
    assert(all_valid);
    printf("  ✓ Page classification - all results in valid range\n");
    
    // Test histogram generation
    atlas_r96_histogram_page_llvm(page, histogram);
    
    // Check histogram sums to 256
    uint32_t total = 0;
    for (int i = 0; i < 96; i++) {
        total += histogram[i];
    }
    assert(total == 256);
    printf("  ✓ Histogram generation - total count correct\n");
    
    // Test resonance class calculation
    uint8_t resonance = atlas_page_resonance_class_llvm(page);
    assert(resonance < 96);
    printf("  ✓ Resonance class calculation - result in valid range: %u\n", resonance);
}

void test_edge_cases(void) {
    printf("Testing edge cases...\n");
    
    // Test with NULL pointers
    bool check_result = atlas_conserved_check(NULL, 0);
    assert(check_result); // Stub should return true even for NULL
    printf("  ✓ Conservation check handles NULL input\n");
    
    uint32_t sum_result = atlas_conserved_sum(NULL, 0);
    assert(sum_result == 0);
    printf("  ✓ Conservation sum handles NULL input\n");
    
    void* witness = atlas_witness_generate_llvm(NULL, 0);
    assert(witness == NULL);
    printf("  ✓ Witness generation handles NULL input\n");
    
    // Test witness operations with NULL
    atlas_witness_destroy_llvm(NULL); // Should not crash
    printf("  ✓ Witness destroy handles NULL input\n");
}

int main(void) {
    printf("Atlas Layer 0 Core Stub Tests\n");
    printf("==============================\n\n");
    
    test_stub_detection();
    printf("\n");
    
    test_conservation_stubs();
    printf("\n");
    
    test_witness_stubs();
    printf("\n");
    
    test_r96_classification_stubs();
    printf("\n");
    
    test_edge_cases();
    printf("\n");
    
    printf("All Layer 0 core stub tests completed successfully.\n");
    printf("Note: These are stub implementations for testing higher layers.\n");
    
    return 0;
}