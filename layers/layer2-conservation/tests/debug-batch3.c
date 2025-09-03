#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

// Match the exact LLVM structure
typedef struct {
    void* data;
    uint64_t size;    // Note: uint64_t not uint32_t
    uint32_t status;
    uint8_t reserved[4];
} llvm_batch_buffer_desc;

void test_batch_check(const void* buffers, uint32_t count, uint8_t* results) {
    if (!buffers || count == 0 || !results) return;
    
    const llvm_batch_buffer_desc* batch_buffers = (const llvm_batch_buffer_desc*)buffers;
    
    printf("Testing %u buffers:\n", count);
    for (uint32_t i = 0; i < count; i++) {
        printf("  Buffer[%u]: data=%p, size=%lu, status=%u\n",
               i, batch_buffers[i].data, batch_buffers[i].size, batch_buffers[i].status);
        
        if (!batch_buffers[i].data || batch_buffers[i].size == 0) {
            results[i] = 0;
            continue;
        }
        
        // Calculate sum
        uint32_t sum = 0;
        const uint8_t* bytes = (const uint8_t*)batch_buffers[i].data;
        for (uint64_t j = 0; j < batch_buffers[i].size; j++) {
            sum += bytes[j];
        }
        
        results[i] = (sum % 96) == 0 ? 1 : 0;
        printf("    -> sum=%u, mod96=%u, conserved=%u\n", sum, sum % 96, results[i]);
    }
}

int main(void) {
    uint8_t* buffer1 = calloc(256, 1);
    uint8_t* buffer2 = calloc(256, 1);
    uint8_t* buffer3 = malloc(256);
    
    for (int i = 0; i < 256; i++) {
        buffer3[i] = 1;
    }
    
    llvm_batch_buffer_desc multi[3];
    multi[0].data = buffer1;
    multi[0].size = 256;
    multi[0].status = 0;
    memset(multi[0].reserved, 0, 4);
    
    multi[1].data = buffer2;
    multi[1].size = 256;
    multi[1].status = 0;
    memset(multi[1].reserved, 0, 4);
    
    multi[2].data = buffer3;
    multi[2].size = 256;
    multi[2].status = 0;
    memset(multi[2].reserved, 0, 4);
    
    uint8_t results[3] = {255, 255, 255};
    test_batch_check(multi, 3, results);
    
    printf("\nFinal results: [%d, %d, %d]\n", results[0], results[1], results[2]);
    
    free(buffer1);
    free(buffer2);
    free(buffer3);
    return 0;
}
