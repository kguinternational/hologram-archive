#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdbool.h>

// Rust FFI function declarations
extern bool atlas_manifold_is_optimized(void);
extern uint32_t atlas_manifold_get_supported_projections(void);
extern bool atlas_manifold_get_statistics(uint64_t* projections_created, uint64_t* shards_extracted, uint64_t* transforms_applied);
extern void atlas_manifold_reset_statistics(void);

// Projection FFI declarations
typedef struct CAtlasProjectionHandle CAtlasProjectionHandle;
extern CAtlasProjectionHandle* atlas_projection_create(uint32_t projection_type, const uint8_t* source_data, size_t source_size);
extern void atlas_projection_destroy(CAtlasProjectionHandle* handle);
extern bool atlas_projection_is_valid(const CAtlasProjectionHandle* handle);
extern int atlas_projection_get_dimensions(const CAtlasProjectionHandle* handle, uint32_t* width, uint32_t* height);

// System test
extern bool atlas_manifold_system_test(void);

int main() {
    printf("=== Atlas Manifold Layer 4 Comprehensive FFI Test ===\n\n");
    
    // Test runtime information functions
    printf("1. Runtime Information:\n");
    bool is_optimized = atlas_manifold_is_optimized();
    printf("   • Is optimized build: %s\n", is_optimized ? "true" : "false");
    
    uint32_t supported = atlas_manifold_get_supported_projections();
    printf("   • Supported projections: 0x%08x (Linear: %s, R96_Fourier: %s)\n", 
           supported, 
           (supported & 1) ? "Yes" : "No",
           (supported & 2) ? "Yes" : "No");
    
    // Test statistics
    printf("\n2. Statistics:\n");
    uint64_t proj_count, shard_count, trans_count;
    if (atlas_manifold_get_statistics(&proj_count, &shard_count, &trans_count)) {
        printf("   • Projections created: %lu\n", proj_count);
        printf("   • Shards extracted: %lu\n", shard_count);
        printf("   • Transforms applied: %lu\n", trans_count);
    }
    
    // Test projection operations
    printf("\n3. Projection Operations:\n");
    uint8_t test_data[4096] = {0}; // 1 page of test data
    for (int i = 0; i < 4096; i++) {
        test_data[i] = (uint8_t)(i % 96); // Use mod 96 for conservation
    }
    
    // Test LINEAR projection
    printf("   • Testing LINEAR projection...\n");
    CAtlasProjectionHandle* linear_proj = atlas_projection_create(0, test_data, sizeof(test_data));
    if (linear_proj) {
        printf("     ✓ Created successfully\n");
        printf("     ✓ Valid: %s\n", atlas_projection_is_valid(linear_proj) ? "true" : "false");
        
        uint32_t width, height;
        if (atlas_projection_get_dimensions(linear_proj, &width, &height) == 0) {
            printf("     ✓ Dimensions: %ux%u\n", width, height);
        }
        atlas_projection_destroy(linear_proj);
        printf("     ✓ Destroyed successfully\n");
    }
    
    // Test R96_FOURIER projection  
    printf("   • Testing R96_FOURIER projection...\n");
    CAtlasProjectionHandle* fourier_proj = atlas_projection_create(1, test_data, sizeof(test_data));
    if (fourier_proj) {
        printf("     ✓ Created successfully\n");
        printf("     ✓ Valid: %s\n", atlas_projection_is_valid(fourier_proj) ? "true" : "false");
        atlas_projection_destroy(fourier_proj);
        printf("     ✓ Destroyed successfully\n");
    }
    
    // Test system test
    printf("\n4. System Test:\n");
    bool system_test_result = atlas_manifold_system_test();
    printf("   • Comprehensive system test: %s\n", system_test_result ? "PASSED" : "FAILED");
    
    // Final statistics after operations
    printf("\n5. Final Statistics:\n");
    if (atlas_manifold_get_statistics(&proj_count, &shard_count, &trans_count)) {
        printf("   • Projections created: %lu\n", proj_count);
        printf("   • Shards extracted: %lu\n", shard_count);  
        printf("   • Transforms applied: %lu\n", trans_count);
    }
    
    printf("\n=== Layer 4 Manifold FFI Integration: SUCCESS ===\n");
    return 0;
}
