#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

typedef struct {
    void* data;
    uint32_t size;
    uint32_t status;
} atlas_batch_buffer_t;

void check_batch(atlas_batch_buffer_t* buffers, int count) {
    for (int i = 0; i < count; i++) {
        uint32_t sum = 0;
        uint8_t* data = (uint8_t*)buffers[i].data;
        for (uint32_t j = 0; j < buffers[i].size; j++) {
            sum += data[j];
        }
        printf("Buffer %d: ptr=%p, size=%u, sum=%u, mod96=%u, conserved=%s\n",
               i, buffers[i].data, buffers[i].size, sum, sum % 96,
               (sum % 96 == 0) ? "YES" : "NO");
    }
}

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
    
    check_batch(multi, 3);
    
    free(buffer1);
    free(buffer2);
    free(buffer3);
    return 0;
}
