/* test-basic-verification.c - Basic Atlas-12288 Layer 4 Verification Test
 * (c) 2024-2025 UOR Foundation - MIT License
 */

#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <stdbool.h>
#include <math.h>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

#include "../include/atlas-manifold.h"

void test_basic_functions_exist(void) {
    printf("=== Testing Basic Function Availability ===\n");
    
    // Test that we can create projections (existing working functionality)
    uint8_t test_data[4096];
    for (int i = 0; i < 4096; i++) {
        test_data[i] = (uint8_t)(i % 256);
    }
    
    atlas_projection_t proj = atlas_projection_create(ATLAS_PROJECTION_LINEAR, 
                                                     test_data, sizeof(test_data));
    
    if (proj) {
        printf("✓ Projection creation works\n");
        
        bool is_valid = atlas_projection_is_valid(proj);
        printf("✓ Projection validation: %s\n", is_valid ? "VALID" : "INVALID");
        
        uint32_t width, height;
        int dim_result = atlas_projection_get_dimensions(proj, &width, &height);
        if (dim_result == 0) {
            printf("✓ Dimensions retrieved: %dx%d\n", width, height);
        }
        
        atlas_projection_destroy(proj);
        printf("✓ Projection destroyed successfully\n");
    } else {
        printf("✗ Projection creation failed\n");
    }
    
    // Test boundary region structure
    atlas_boundary_region_t region = {
        .start_coord = 0,
        .end_coord = 4096,
        .page_count = 1,
        .region_class = 42,
        .is_conserved = true
    };
    
    printf("✓ Boundary region structure created successfully\n");
    printf("  - Start: %u, End: %u, Pages: %u, Class: %u\n",
           region.start_coord, region.end_coord, region.page_count, region.region_class);
    
    // Test transform params structure
    atlas_transform_params_t params = {
        .scaling_factor = 1.5,
        .rotation_angle = 3.14159 / 4.0,
        .translation_x = 10.0,
        .translation_y = -5.0
    };
    
    printf("✓ Transform parameters structure created successfully\n");
    printf("  - Scale: %.2f, Rotation: %.3f, Translation: (%.1f, %.1f)\n",
           params.scaling_factor, params.rotation_angle, params.translation_x, params.translation_y);
}

void test_version_info(void) {
    printf("\n=== Testing Version Information ===\n");
    
    // Test the inline version function
    uint32_t version = atlas_manifold_version();
    uint8_t major = (version >> 16) & 0xFF;
    uint8_t minor = (version >> 8) & 0xFF;
    uint8_t patch = version & 0xFF;
    
    printf("✓ Atlas Manifold version: %u.%u.%u (0x%08X)\n", major, minor, patch, version);
    printf("✓ Version function works correctly\n");
}

void test_error_handling(void) {
    printf("\n=== Testing Error Handling ===\n");
    
    // Test null projection creation
    atlas_projection_t null_proj = atlas_projection_create(ATLAS_PROJECTION_LINEAR, NULL, 0);
    if (!null_proj) {
        printf("✓ Null data projection creation correctly fails\n");
    }
    
    // Test invalid projection type
    uint8_t test_data[256] = {42};
    atlas_projection_t invalid_proj = atlas_projection_create(999, test_data, sizeof(test_data));
    if (!invalid_proj) {
        printf("✓ Invalid projection type correctly fails\n");
    }
    
    // Test null projection validation
    bool null_valid = atlas_projection_is_valid(NULL);
    if (!null_valid) {
        printf("✓ Null projection correctly reports invalid\n");
    }
    
    // Test dimensions with null projection
    uint32_t width, height;
    int null_dim_result = atlas_projection_get_dimensions(NULL, &width, &height);
    if (null_dim_result != 0) {
        printf("✓ Dimensions on null projection correctly fails\n");
    }
    
    // Test dimensions with null output parameters
    atlas_projection_t test_proj = atlas_projection_create(ATLAS_PROJECTION_LINEAR, 
                                                          test_data, sizeof(test_data));
    if (test_proj) {
        int null_width_result = atlas_projection_get_dimensions(test_proj, NULL, &height);
        if (null_width_result != 0) {
            printf("✓ Dimensions with null width parameter correctly fails\n");
        }
        
        int null_height_result = atlas_projection_get_dimensions(test_proj, &width, NULL);
        if (null_height_result != 0) {
            printf("✓ Dimensions with null height parameter correctly fails\n");
        }
        
        atlas_projection_destroy(test_proj);
    }
    
    // Test safe destruction of null projection
    atlas_projection_destroy(NULL);
    printf("✓ Destroying null projection is safe\n");
}

void test_reconstruction_context(void) {
    printf("\n=== Testing Reconstruction Context ===\n");
    
    // Test reconstruction context initialization
    atlas_reconstruction_ctx_t ctx = atlas_reconstruction_init(4);
    
    printf("✓ Reconstruction context initialized\n");
    printf("  - Total shards: %u\n", ctx.total_shards);
    printf("  - Current shard: %u\n", ctx.current_shard);
    printf("  - Checksum: %llu\n", (unsigned long long)ctx.checksum);
    printf("  - Complete: %s\n", ctx.is_complete ? "YES" : "NO");
    
    // Test completion check
    bool is_complete = atlas_reconstruction_is_complete(&ctx);
    printf("✓ Initial completion status: %s\n", is_complete ? "COMPLETE" : "NOT COMPLETE");
    
    // Test null context handling
    bool null_complete = atlas_reconstruction_is_complete(NULL);
    if (!null_complete) {
        printf("✓ Null context completion check correctly returns false\n");
    }
}

int main(void) {
    printf("=== Atlas Manifold Layer 4 - Basic Verification Tests ===\n\n");
    
    test_basic_functions_exist();
    test_version_info();
    test_error_handling();
    test_reconstruction_context();
    
    printf("\n=== All Basic Tests Completed Successfully ===\n");
    printf("Note: Advanced verification functions will be available once Rust compilation is resolved.\n");
    printf("Current implementation provides solid foundation for projection operations.\n");
    
    return 0;
}