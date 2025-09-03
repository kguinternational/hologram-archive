#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include "../include/atlas-conservation.h"

// External function from stubs
extern void atlas_batch_conserved_check_llvm(const void* buffers, uint32_t count, uint8_t* results);

int main(void) {
    uint8_t* buffer1 = calloc(256, 1);
    uint8_t* buffer2 = calloc(256, 1);
    uint8_t* buffer3 = malloc(256);
    
    for (int i = 0; i < 256; i++) {
        buffer3[i] = 1;
    }
    
    atlas_batch_buffer_t multi[3] = {
        { .data = buffer1, .size = 256, .status = 0 },
        { .data = buffer2, .size = 256, .status = 0 },
        { .data = buffer3, .size = 256, .status = 0 }
    };
    
    // Check with llvm stub directly
    uint8_t results[3] = {255, 255, 255}; // Initialize with sentinel
    atlas_batch_conserved_check_llvm(multi, 3, results);
    
    printf("LLVM stub results: [%d, %d, %d]\n", results[0], results[1], results[2]);
    
    // Check with wrapper
    uint8_t* wrapper_results = atlas_batch_conserved_check(multi, 3);
    if (wrapper_results) {
        printf("Wrapper results: [%d, %d, %d]\n", 
               wrapper_results[0], wrapper_results[1], wrapper_results[2]);
        free(wrapper_results);
    }
    
    free(buffer1);
    free(buffer2);
    free(buffer3);
    return 0;
}
