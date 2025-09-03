#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

int main(void) {
    // Test conservation check
    uint8_t* buffer = calloc(256, 1);
    uint32_t sum = 0;
    for (int i = 0; i < 256; i++) {
        sum += buffer[i];
    }
    printf("Buffer of zeros: sum = %u, mod 96 = %u (should be 0)\n", sum, sum % 96);
    
    // Check non-zero buffer (0-255)
    for (int i = 0; i < 256; i++) {
        buffer[i] = i;
    }
    sum = 0;
    for (int i = 0; i < 256; i++) {
        sum += buffer[i];
    }
    printf("Buffer [0..255]: sum = %u, mod 96 = %u\n", sum, sum % 96);
    
    free(buffer);
    return 0;
}
