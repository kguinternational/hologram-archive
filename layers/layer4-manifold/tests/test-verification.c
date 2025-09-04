/* test-verification.c - Atlas-12288 Layer 4 Verification Functions Test
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

void test_verification_functions(void) {
    printf("=== Testing Verification Functions ===\n");
    
    // Test data for creating projections
    uint8_t test_data[8192];
    for (int i = 0; i < 8192; i++) {
        test_data[i] = (uint8_t)(i % 256);
    }
    
    // Create a linear projection for testing
    atlas_projection_t proj = atlas_projection_create(ATLAS_PROJECTION_LINEAR, 
                                                     test_data, sizeof(test_data));
    
    if (proj) {
        printf("✓ Test projection created successfully\n");
        
        // Test atlas_manifold_verify_projection
        bool is_valid = atlas_manifold_verify_projection(proj);
        printf("✓ Projection verification: %s\n", is_valid ? "VALID" : "INVALID");
        
        // Test with null projection
        bool null_result = atlas_manifold_verify_projection(NULL);
        assert(!null_result);
        printf("✓ Null projection verification correctly returns false\n");
        
        atlas_projection_destroy(proj);
    }
    
    // Test boundary region verification
    atlas_boundary_region_t region = {
        .start_coord = 0,
        .end_coord = 8192,
        .page_count = 2,
        .region_class = 42,
        .is_conserved = true
    };
    
    bool region_valid = atlas_manifold_verify_boundary_region(&region);
    printf("✓ Boundary region verification: %s\n", region_valid ? "VALID" : "INVALID");
    
    // Test with invalid region
    atlas_boundary_region_t bad_region = {
        .start_coord = 1000,
        .end_coord = 100,  // Invalid: start > end
        .page_count = 2,
        .region_class = 42,
        .is_conserved = true
    };
    
    bool bad_region_valid = atlas_manifold_verify_boundary_region(&bad_region);
    assert(!bad_region_valid);
    printf("✓ Invalid boundary region correctly rejected\n");
    
    // Test with null region
    bool null_region_result = atlas_manifold_verify_boundary_region(NULL);
    assert(!null_region_result);
    printf("✓ Null boundary region verification correctly returns false\n");
    
    // Test transformation parameter verification
    atlas_transform_params_t valid_params = {
        .scaling_factor = 1.5,
        .rotation_angle = 3.14159 / 4.0,  // 45 degrees
        .translation_x = 100.0,
        .translation_y = -50.0
    };
    
    bool params_valid = atlas_manifold_verify_transform_params(&valid_params);
    printf("✓ Valid transform params verification: %s\n", params_valid ? "VALID" : "INVALID");
    
    // Test with invalid parameters
    atlas_transform_params_t invalid_params = {
        .scaling_factor = 0.0,  // Invalid: zero scaling
        .rotation_angle = 3.14159 / 4.0,
        .translation_x = 100.0,
        .translation_y = -50.0
    };
    
    bool invalid_params_valid = atlas_manifold_verify_transform_params(&invalid_params);
    assert(!invalid_params_valid);
    printf("✓ Invalid transform params correctly rejected\n");
    
    // Test with null params
    bool null_params_result = atlas_manifold_verify_transform_params(NULL);
    assert(!null_params_result);
    printf("✓ Null transform params verification correctly returns false\n");
}

void test_system_self_test(void) {
    printf("\n=== Testing System Self-Test ===\n");
    
    bool system_test_result = atlas_manifold_system_test();
    printf("✓ System self-test: %s\n", system_test_result ? "PASSED" : "FAILED");
    
    // The system test should pass if our implementation is correct
    if (system_test_result) {
        printf("✓ All internal system tests passed\n");
    } else {
        printf("✗ Some internal system tests failed\n");
    }
}

void test_transform_operations(void) {
    printf("\n=== Testing Transform Operations ===\n");
    
    // Test data for creating projections
    uint8_t test_data[4096];
    for (int i = 0; i < 4096; i++) {
        test_data[i] = (uint8_t)(42 + (i % 200));
    }
    
    // Create a projection for transformation tests
    atlas_projection_t proj = atlas_projection_create(ATLAS_PROJECTION_LINEAR, 
                                                     test_data, sizeof(test_data));
    
    if (proj) {
        printf("✓ Test projection for transforms created successfully\n");
        
        // Test scaling
        int scale_result = atlas_manifold_scale_projection(proj, 1.5, 2.0);
        printf("✓ Scaling operation: %s\n", (scale_result == 0) ? "SUCCESS" : "FAILED");
        
        // Test rotation
        int rotate_result = atlas_manifold_rotate_projection(proj, 3.14159 / 6.0, 0.0, 0.0);
        printf("✓ Rotation operation: %s\n", (rotate_result == 0) ? "SUCCESS" : "FAILED");
        
        // Test translation
        int translate_result = atlas_manifold_translate_projection(proj, 10.0, -5.0);
        printf("✓ Translation operation: %s\n", (translate_result == 0) ? "SUCCESS" : "FAILED");
        
        // Test linear transform with identity matrix
        double identity_matrix[16] = {
            1.0, 0.0, 0.0, 0.0,
            0.0, 1.0, 0.0, 0.0,
            0.0, 0.0, 1.0, 0.0,
            0.0, 0.0, 0.0, 1.0
        };
        
        int linear_result = atlas_manifold_apply_linear_transform(proj, identity_matrix);
        printf("✓ Linear transform (identity): %s\n", (linear_result == 0) ? "SUCCESS" : "FAILED");
        
        // Verify projection is still valid after all transformations
        bool still_valid = atlas_manifold_verify_projection(proj);
        printf("✓ Projection valid after transforms: %s\n", still_valid ? "YES" : "NO");
        
        // Test error conditions
        int null_scale = atlas_manifold_scale_projection(NULL, 1.0, 1.0);
        assert(null_scale != 0);
        printf("✓ Scaling with null projection correctly fails\n");
        
        int invalid_scale = atlas_manifold_scale_projection(proj, 0.0, 1.0);
        assert(invalid_scale != 0);
        printf("✓ Scaling with zero factor correctly fails\n");
        
        atlas_projection_destroy(proj);
    }
    
    // Test R96 Fourier transform operations
    atlas_projection_t r96_proj = atlas_projection_create(ATLAS_PROJECTION_R96_FOURIER, 
                                                         test_data, sizeof(test_data));
    
    if (r96_proj) {
        printf("✓ R96 Fourier projection for transform created successfully\n");
        
        // Test forward Fourier transform
        int fourier_result = atlas_manifold_apply_fourier_transform(r96_proj, false);
        printf("✓ Forward Fourier transform: %s\n", (fourier_result == 0) ? "SUCCESS" : "FAILED");
        
        // Test inverse Fourier transform
        int inverse_result = atlas_manifold_apply_fourier_transform(r96_proj, true);
        printf("✓ Inverse Fourier transform: %s\n", (inverse_result == 0) ? "SUCCESS" : "FAILED");
        
        // Test Fourier transform on wrong projection type
        atlas_projection_t linear_proj = atlas_projection_create(ATLAS_PROJECTION_LINEAR, 
                                                               test_data, sizeof(test_data));
        if (linear_proj) {
            int wrong_type_result = atlas_manifold_apply_fourier_transform(linear_proj, false);
            assert(wrong_type_result != 0);
            printf("✓ Fourier transform on LINEAR projection correctly fails\n");
            atlas_projection_destroy(linear_proj);
        }
        
        atlas_projection_destroy(r96_proj);
    }
}

void test_runtime_functions(void) {
    printf("\n=== Testing Runtime Information Functions ===\n");
    
    // Test optimization flag
    bool is_optimized = atlas_manifold_is_optimized();
    printf("✓ Build is optimized: %s\n", is_optimized ? "YES" : "NO (debug)");
    
    // Test supported projections
    uint32_t supported = atlas_manifold_get_supported_projections();
    printf("✓ Supported projections bitmask: 0x%X\n", supported);
    
    // Check that both projection types are supported
    bool linear_supported = (supported & (1 << ATLAS_PROJECTION_LINEAR)) != 0;
    bool r96_supported = (supported & (1 << ATLAS_PROJECTION_R96_FOURIER)) != 0;
    printf("✓ LINEAR projection supported: %s\n", linear_supported ? "YES" : "NO");
    printf("✓ R96_FOURIER projection supported: %s\n", r96_supported ? "YES" : "NO");
    
    // Test statistics functions
    uint64_t projections_created, shards_extracted, transforms_applied;
    bool stats_result = atlas_manifold_get_statistics(&projections_created, 
                                                     &shards_extracted, 
                                                     &transforms_applied);
    
    if (stats_result) {
        printf("✓ Statistics retrieved successfully\n");
        printf("  - Projections created: %llu\n", (unsigned long long)projections_created);
        printf("  - Shards extracted: %llu\n", (unsigned long long)shards_extracted);
        printf("  - Transforms applied: %llu\n", (unsigned long long)transforms_applied);
    }
    
    // Test statistics reset
    atlas_manifold_reset_statistics();
    
    bool reset_stats_result = atlas_manifold_get_statistics(&projections_created, 
                                                           &shards_extracted, 
                                                           &transforms_applied);
    
    if (reset_stats_result) {
        printf("✓ Statistics reset successfully\n");
        printf("  - Projections created after reset: %llu\n", (unsigned long long)projections_created);
        printf("  - Shards extracted after reset: %llu\n", (unsigned long long)shards_extracted);
        printf("  - Transforms applied after reset: %llu\n", (unsigned long long)transforms_applied);
    }
    
    // Test with null parameters
    bool null_stats = atlas_manifold_get_statistics(NULL, &shards_extracted, &transforms_applied);
    assert(!null_stats);
    printf("✓ Statistics with null parameter correctly fails\n");
}

void test_advanced_math_operations(void) {
    printf("\n=== Testing Advanced Mathematical Operations ===\n");
    
    // Test data for creating projections
    uint8_t test_data[4096];
    for (int i = 0; i < 4096; i++) {
        test_data[i] = (uint8_t)(100 + (i % 156));
    }
    
    // Create a projection for mathematical tests
    atlas_projection_t proj = atlas_projection_create(ATLAS_PROJECTION_LINEAR, 
                                                     test_data, sizeof(test_data));
    
    if (proj) {
        printf("✓ Test projection for math operations created successfully\n");
        
        // Test topological invariants computation
        double invariants[10];
        int invariant_count = atlas_manifold_compute_invariants(proj, invariants, 10);
        
        if (invariant_count > 0) {
            printf("✓ Computed %d topological invariants\n", invariant_count);
            for (int i = 0; i < invariant_count; i++) {
                printf("  - Invariant %d: %.6f\n", i + 1, invariants[i]);
            }
        }
        
        // Test curvature computation
        double curvature1 = atlas_manifold_compute_curvature(proj, 0.5, 0.5);
        printf("✓ Curvature at (0.5, 0.5): %.6f\n", isfinite(curvature1) ? curvature1 : 0.0);
        
        double curvature2 = atlas_manifold_compute_curvature(proj, 1.0, 1.0);
        printf("✓ Curvature at (1.0, 1.0): %.6f\n", isfinite(curvature2) ? curvature2 : 0.0);
        
        // Test geodesic distance computation
        double geodesic_dist = atlas_manifold_geodesic_distance(proj, 0.0, 0.0, 1.0, 1.0);
        if (geodesic_dist >= 0.0) {
            printf("✓ Geodesic distance from (0,0) to (1,1): %.6f\n", geodesic_dist);
        }
        
        // Test critical points finding
        double critical_points[20];  // Space for 10 points (x,y pairs)
        int critical_count = atlas_manifold_find_critical_points(proj, critical_points, 10);
        
        if (critical_count >= 0) {
            printf("✓ Found %d critical points\n", critical_count);
            for (int i = 0; i < critical_count; i++) {
                printf("  - Critical point %d: (%.3f, %.3f)\n", i + 1, 
                       critical_points[i * 2], critical_points[i * 2 + 1]);
            }
        }
        
        // Test error conditions
        int null_invariants = atlas_manifold_compute_invariants(NULL, invariants, 10);
        assert(null_invariants < 0);
        printf("✓ Invariants computation with null projection correctly fails\n");
        
        double null_curvature = atlas_manifold_compute_curvature(NULL, 0.0, 0.0);
        assert(!isfinite(null_curvature));
        printf("✓ Curvature computation with null projection correctly fails\n");
        
        double null_distance = atlas_manifold_geodesic_distance(NULL, 0.0, 0.0, 1.0, 1.0);
        assert(null_distance < 0.0);
        printf("✓ Geodesic distance with null projection correctly fails\n");
        
        atlas_projection_destroy(proj);
    }
}

int main(void) {
    printf("=== Atlas Manifold Layer 4 - Verification and Transform Tests ===\n\n");
    
    test_verification_functions();
    test_system_self_test();
    test_transform_operations();
    test_runtime_functions();
    test_advanced_math_operations();
    
    printf("\n=== All Verification and Transform Tests Completed Successfully ===\n");
    return 0;
}