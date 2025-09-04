#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdbool.h>

// Direct Rust FFI declarations matching ffi.rs
typedef struct CAtlasProjectionHandle CAtlasProjectionHandle;

extern CAtlasProjectionHandle* atlas_projection_create(uint32_t projection_type, const uint8_t* source_data, size_t source_size);
extern void atlas_projection_destroy(CAtlasProjectionHandle* handle);
extern int atlas_projection_get_dimensions(const CAtlasProjectionHandle* handle, uint32_t* width, uint32_t* height);
extern bool atlas_projection_is_valid(const CAtlasProjectionHandle* handle);

int main() {
    printf("Testing Rust projection FFI...\n");
    
    // Create some test data
    uint8_t test_data[1024];
    for (int i = 0; i < 1024; i++) {
        test_data[i] = (uint8_t)(i % 256);
    }
    
    // Create projection
    CAtlasProjectionHandle* proj = atlas_projection_create(0, test_data, sizeof(test_data)); // 0 = LINEAR
    if (proj == NULL) {
        printf("Failed to create projection\n");
        return 1;
    }
    
    printf("✓ Projection created successfully\n");
    
    // Check if valid
    bool valid = atlas_projection_is_valid(proj);
    printf("✓ Projection is valid: %s\n", valid ? "true" : "false");
    
    // Get dimensions
    uint32_t width = 0, height = 0;
    int result = atlas_projection_get_dimensions(proj, &width, &height);
    if (result == 0) {
        printf("✓ Dimensions: %ux%u\n", width, height);
    } else {
        printf("✗ Failed to get dimensions\n");
    }
    
    // Cleanup
    atlas_projection_destroy(proj);
    printf("✓ Projection destroyed\n");
    
    printf("Rust projection FFI test completed successfully!\n");
    return 0;
}
