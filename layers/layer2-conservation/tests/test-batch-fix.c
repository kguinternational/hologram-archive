/* test-batch-fix.c - Test that batch operations work after fix */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>
#include "../include/atlas-conservation.h"

int main(void) {
    printf("Testing Layer 2 Batch Operations Fix\n");
    printf("=====================================\n\n");
    
    // Test 1: Single buffer
    printf("1. Single buffer test...\n");
    uint8_t* buffer1 = calloc(256, 1);  // All zeros, sum = 0, conserved
    
    atlas_batch_buffer_t single = {
        .data = buffer1,
        .size = 256,
        .status = 0
    };
    
    uint8_t* result = atlas_batch_conserved_check(&single, 1);
    if (result) {
        printf("   ✓ Single buffer: result=%d (expected 1 for conserved)\n", result[0]);
        free(result);
    } else {
        printf("   ✗ Single buffer failed\n");
        free(buffer1);
        return 1;
    }
    
    // Test 2: Multiple buffers
    printf("\n2. Multiple buffer test...\n");
    uint8_t* buffer2 = calloc(256, 1);
    uint8_t* buffer3 = malloc(256);
    
    // Make buffer3 non-conserved
    for (int i = 0; i < 256; i++) {
        buffer3[i] = 1;  // Sum = 256, not divisible by 96 (256 % 96 = 64)
    }
    
    atlas_batch_buffer_t multi[3] = {
        { .data = buffer1, .size = 256, .status = 0 },
        { .data = buffer2, .size = 256, .status = 0 },
        { .data = buffer3, .size = 256, .status = 0 }
    };
    
    result = atlas_batch_conserved_check(multi, 3);
    if (result) {
        printf("   Results: [%d, %d, %d]\n", result[0], result[1], result[2]);
        printf("   ✓ Buffer 1 (zeros): %s\n", result[0] ? "conserved" : "not conserved");
        printf("   ✓ Buffer 2 (zeros): %s\n", result[1] ? "conserved" : "not conserved");
        printf("   ✓ Buffer 3 (all 1s): %s\n", result[2] ? "conserved" : "not conserved");
        
        bool correct = (result[0] == 1) && (result[1] == 1) && (result[2] == 0);
        if (correct) {
            printf("   ✓ All results correct!\n");
        } else {
            printf("   ✗ Results incorrect\n");
        }
        free(result);
    } else {
        printf("   ✗ Multiple buffer test failed\n");
        free(buffer1);
        free(buffer2);
        free(buffer3);
        return 1;
    }
    
    // Test 3: Larger batch (8 buffers for SIMD path)
    printf("\n3. SIMD batch test (8 buffers)...\n");
    uint8_t* buffers[8];
    atlas_batch_buffer_t batch[8];
    
    for (int i = 0; i < 8; i++) {
        buffers[i] = calloc(256, 1);  // All conserved
        batch[i].data = buffers[i];
        batch[i].size = 256;
        batch[i].status = 0;
    }
    
    result = atlas_batch_conserved_check(batch, 8);
    if (result) {
        printf("   Results: ");
        bool all_conserved = true;
        for (int i = 0; i < 8; i++) {
            printf("%d ", result[i]);
            if (result[i] != 1) all_conserved = false;
        }
        printf("\n");
        
        if (all_conserved) {
            printf("   ✓ SIMD batch test passed!\n");
        } else {
            printf("   ✗ Some buffers incorrectly marked as non-conserved\n");
        }
        free(result);
    } else {
        printf("   ✗ SIMD batch test failed\n");
    }
    
    // Cleanup
    free(buffer1);
    free(buffer2);
    free(buffer3);
    for (int i = 0; i < 8; i++) {
        free(buffers[i]);
    }
    
    printf("\n✅ Batch operation tests completed!\n");
    return 0;
}