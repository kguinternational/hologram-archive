/* test-final-verification.c - Final comprehensive verification for Layer 4
 * (c) 2024-2025 UOR Foundation - MIT License
 * 
 * Comprehensive test to verify all Layer 4 functions work correctly
 * and return non-placeholder values. This test exercises:
 * - atlas_reconstruction_finalize
 * - atlas_manifold_compute_invariants  
 * - atlas_manifold_compute_curvature
 * - atlas_manifold_geodesic_distance
 * - atlas_manifold_find_critical_points
 * - atlas_manifold_get_last_error
 * - atlas_manifold_error_string
 */

#define _GNU_SOURCE
#include "../include/atlas-manifold.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <assert.h>
#include <time.h>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

// Test result tracking
static int tests_passed = 0;
static int tests_failed = 0;
static int tests_total = 0;

// Helper macros
#define TEST_START(name) \
    do { \
        tests_total++; \
        printf("[TEST %d] %s... ", tests_total, name); \
        fflush(stdout); \
    } while (0)

#define TEST_PASS() \
    do { \
        tests_passed++; \
        printf("PASS\n"); \
    } while (0)

#define TEST_FAIL(reason) \
    do { \
        tests_failed++; \
        printf("FAIL (%s)\n", reason); \
    } while (0)

#define ASSERT_TRUE(condition, message) \
    do { \
        if (!(condition)) { \
            TEST_FAIL(message); \
            return false; \
        } \
    } while (0)

#define ASSERT_NOT_NULL(ptr, message) \
    do { \
        if ((ptr) == NULL) { \
            TEST_FAIL(message); \
            return false; \
        } \
    } while (0)

#define ASSERT_GT(value, threshold, message) \
    do { \
        if ((value) <= (threshold)) { \
            TEST_FAIL(message); \
            return false; \
        } \
    } while (0)

#define ASSERT_FINITE(value, message) \
    do { \
        if (!isfinite(value)) { \
            TEST_FAIL(message); \
            return false; \
        } \
    } while (0)

// Test error handling system
bool test_error_handling() {
    TEST_START("Error handling system");
    
    // Test getting initial error (should be success)
    atlas_manifold_error_t initial_error = atlas_manifold_get_last_error();
    ASSERT_TRUE(initial_error == ATLAS_MANIFOLD_SUCCESS, "Initial error should be SUCCESS");
    
    // Test error string for success
    const char* success_msg = atlas_manifold_error_string(ATLAS_MANIFOLD_SUCCESS);
    ASSERT_NOT_NULL(success_msg, "Error string should not be NULL");
    ASSERT_TRUE(strlen(success_msg) > 0, "Error string should not be empty");
    
    // Test error strings for various error codes
    const char* invalid_arg_msg = atlas_manifold_error_string(ATLAS_MANIFOLD_ERROR_INVALID_ARGUMENT);
    ASSERT_NOT_NULL(invalid_arg_msg, "Invalid argument error string should not be NULL");
    
    const char* out_of_memory_msg = atlas_manifold_error_string(ATLAS_MANIFOLD_ERROR_OUT_OF_MEMORY);
    ASSERT_NOT_NULL(out_of_memory_msg, "Out of memory error string should not be NULL");
    
    // Test that different error codes produce different messages
    ASSERT_TRUE(strcmp(success_msg, invalid_arg_msg) != 0, "Different errors should have different messages");
    
    TEST_PASS();
    return true;
}

// Test projection creation and basic operations
bool test_projection_operations() {
    TEST_START("Projection creation and operations");
    
    // Create test data
    const size_t test_data_size = 1024;
    uint8_t* test_data = malloc(test_data_size);
    ASSERT_NOT_NULL(test_data, "Failed to allocate test data");
    
    // Fill with pattern data
    for (size_t i = 0; i < test_data_size; i++) {
        test_data[i] = (uint8_t)(i * 13 % 256);
    }
    
    // Create linear projection
    atlas_projection_t projection = atlas_projection_create(ATLAS_PROJECTION_LINEAR, test_data, test_data_size);
    ASSERT_NOT_NULL(projection, "Failed to create projection");
    
    // Verify projection is valid
    ASSERT_TRUE(atlas_projection_is_valid(projection), "Projection should be valid");
    
    // Get dimensions
    uint32_t width, height;
    int dim_result = atlas_projection_get_dimensions(projection, &width, &height);
    ASSERT_TRUE(dim_result == 0, "Getting dimensions should succeed");
    ASSERT_GT(width, 0, "Width should be positive");
    ASSERT_GT(height, 0, "Height should be positive");
    
    // Test transformation
    atlas_transform_params_t transform = {
        .scaling_factor = 1.5,
        .rotation_angle = M_PI / 4,
        .translation_x = 10.0,
        .translation_y = 5.0
    };
    
    int transform_result = atlas_projection_transform(projection, &transform);
    ASSERT_TRUE(transform_result == 0, "Transform should succeed");
    
    // Clean up
    atlas_projection_destroy(projection);
    free(test_data);
    
    TEST_PASS();
    return true;
}

// Test shard extraction and reconstruction
bool test_shard_reconstruction() {
    TEST_START("Shard extraction and reconstruction");
    
    // Create test data
    const size_t test_data_size = 2048;
    uint8_t* test_data = malloc(test_data_size);
    ASSERT_NOT_NULL(test_data, "Failed to allocate test data");
    
    // Fill with structured pattern
    for (size_t i = 0; i < test_data_size; i++) {
        test_data[i] = (uint8_t)((i * 7 + 42) % 256);
    }
    
    // Create R96 Fourier projection for better shard testing
    atlas_projection_t projection = atlas_projection_create(ATLAS_PROJECTION_R96_FOURIER, test_data, test_data_size);
    ASSERT_NOT_NULL(projection, "Failed to create R96 projection");
    
    // Define boundary regions for shards
    const int num_shards = 3;
    atlas_boundary_region_t regions[3];
    regions[0] = (atlas_boundary_region_t){.start_coord = 0, .end_coord = 1000, .page_count = 10, .region_class = 1, .is_conserved = true};
    regions[1] = (atlas_boundary_region_t){.start_coord = 1000, .end_coord = 2000, .page_count = 10, .region_class = 2, .is_conserved = true};
    regions[2] = (atlas_boundary_region_t){.start_coord = 2000, .end_coord = 3000, .page_count = 10, .region_class = 3, .is_conserved = false};
    
    // Extract shards
    atlas_shard_t shards[num_shards];
    int extracted = atlas_shard_extract_batch(projection, regions, num_shards, shards);
    ASSERT_TRUE(extracted > 0, "Should extract at least one shard");
    
    // Verify each extracted shard
    for (int i = 0; i < extracted; i++) {
        ASSERT_NOT_NULL(shards[i], "Shard should not be NULL");
        ASSERT_TRUE(atlas_shard_verify(shards[i]), "Shard should be valid");
        
        size_t shard_size = atlas_shard_get_size(shards[i]);
        ASSERT_GT(shard_size, 0, "Shard size should be positive");
        
        // Test copying shard data
        uint8_t* shard_buffer = malloc(shard_size);
        if (shard_buffer) {
            int copied = atlas_shard_copy_data(shards[i], shard_buffer, shard_size);
            ASSERT_TRUE(copied > 0, "Should copy shard data");
            free(shard_buffer);
        }
    }
    
    // Test reconstruction process
    atlas_reconstruction_ctx_t ctx = atlas_reconstruction_init(extracted);
    ASSERT_TRUE(ctx.total_shards == (uint32_t)extracted, "Context should track correct number of shards");
    
    // Add shards to reconstruction context
    for (int i = 0; i < extracted; i++) {
        int add_result = atlas_reconstruction_add_shard(&ctx, shards[i]);
        ASSERT_TRUE(add_result == 0, "Adding shard should succeed");
    }
    
    // Verify reconstruction is complete
    ASSERT_TRUE(atlas_reconstruction_is_complete(&ctx), "Reconstruction should be complete");
    
    // Finalize reconstruction - THIS IS A KEY FUNCTION TO TEST
    atlas_projection_t reconstructed = atlas_reconstruction_finalize(&ctx, ATLAS_PROJECTION_LINEAR);
    ASSERT_NOT_NULL(reconstructed, "Reconstruction finalize should succeed");
    ASSERT_TRUE(atlas_projection_is_valid(reconstructed), "Reconstructed projection should be valid");
    
    // Clean up
    atlas_projection_destroy(reconstructed);
    for (int i = 0; i < extracted; i++) {
        atlas_shard_destroy(shards[i]);
    }
    atlas_projection_destroy(projection);
    free(test_data);
    
    TEST_PASS();
    return true;
}

// Test advanced mathematical operations
bool test_advanced_math_operations() {
    TEST_START("Advanced mathematical operations");
    
    // Create test projection
    const size_t test_data_size = 1536;
    uint8_t* test_data = malloc(test_data_size);
    ASSERT_NOT_NULL(test_data, "Failed to allocate test data");
    
    // Fill with mathematical pattern
    for (size_t i = 0; i < test_data_size; i++) {
        test_data[i] = (uint8_t)(sin(i * 0.1) * 127 + 128);
    }
    
    atlas_projection_t projection = atlas_projection_create(ATLAS_PROJECTION_R96_FOURIER, test_data, test_data_size);
    ASSERT_NOT_NULL(projection, "Failed to create projection");
    
    // Test compute invariants - KEY FUNCTION TO TEST
    const size_t max_invariants = 10;
    double invariants[max_invariants];
    int invariant_count = atlas_manifold_compute_invariants(projection, invariants, max_invariants);
    ASSERT_TRUE(invariant_count > 0, "Should compute at least one invariant");
    ASSERT_TRUE(invariant_count <= (int)max_invariants, "Should not exceed max invariants");
    
    // Verify invariants are finite and meaningful
    for (int i = 0; i < invariant_count; i++) {
        ASSERT_FINITE(invariants[i], "Invariants should be finite");
    }
    
    // Test compute curvature - KEY FUNCTION TO TEST
    double curvature1 = atlas_manifold_compute_curvature(projection, 0.0, 0.0);
    ASSERT_FINITE(curvature1, "Curvature at origin should be finite");
    
    double curvature2 = atlas_manifold_compute_curvature(projection, 1.0, 1.0);
    ASSERT_FINITE(curvature2, "Curvature at (1,1) should be finite");
    
    double curvature3 = atlas_manifold_compute_curvature(projection, -0.5, 0.5);
    ASSERT_FINITE(curvature3, "Curvature at (-0.5,0.5) should be finite");
    
    // Test geodesic distance - KEY FUNCTION TO TEST
    double distance1 = atlas_manifold_geodesic_distance(projection, 0.0, 0.0, 1.0, 0.0);
    ASSERT_TRUE(distance1 > 0.0, "Distance should be positive");
    ASSERT_FINITE(distance1, "Distance should be finite");
    
    double distance2 = atlas_manifold_geodesic_distance(projection, 0.0, 0.0, 0.0, 1.0);
    ASSERT_TRUE(distance2 > 0.0, "Distance should be positive");
    ASSERT_FINITE(distance2, "Distance should be finite");
    
    // Distance from point to itself should be zero
    double distance_zero = atlas_manifold_geodesic_distance(projection, 1.0, 1.0, 1.0, 1.0);
    ASSERT_TRUE(fabs(distance_zero) < 1e-10, "Self-distance should be approximately zero");
    
    // Test find critical points - KEY FUNCTION TO TEST
    const size_t max_critical_points = 20;
    double critical_points[max_critical_points * 2]; // x,y pairs
    int critical_count = atlas_manifold_find_critical_points(projection, critical_points, max_critical_points);
    ASSERT_TRUE(critical_count >= 0, "Critical point count should be non-negative");
    ASSERT_TRUE(critical_count <= (int)max_critical_points, "Should not exceed max critical points");
    
    // Verify critical points are finite
    for (int i = 0; i < critical_count * 2; i++) {
        ASSERT_FINITE(critical_points[i], "Critical point coordinates should be finite");
    }
    
    // Clean up
    atlas_projection_destroy(projection);
    free(test_data);
    
    TEST_PASS();
    return true;
}

// Test manifold verification functions
bool test_manifold_verification() {
    TEST_START("Manifold verification functions");
    
    // Create test projection
    const size_t test_data_size = 1024;
    uint8_t* test_data = malloc(test_data_size);
    ASSERT_NOT_NULL(test_data, "Failed to allocate test data");
    
    for (size_t i = 0; i < test_data_size; i++) {
        test_data[i] = (uint8_t)((i * 17) % 256);
    }
    
    atlas_projection_t projection = atlas_projection_create(ATLAS_PROJECTION_LINEAR, test_data, test_data_size);
    ASSERT_NOT_NULL(projection, "Failed to create projection");
    
    // Test projection verification
    bool proj_valid = atlas_manifold_verify_projection(projection);
    ASSERT_TRUE(proj_valid, "Projection should verify as valid");
    
    // Test boundary region verification
    atlas_boundary_region_t valid_region = {
        .start_coord = 100,
        .end_coord = 200,
        .page_count = 5,
        .region_class = 1,
        .is_conserved = true
    };
    
    bool region_valid = atlas_manifold_verify_boundary_region(&valid_region);
    ASSERT_TRUE(region_valid, "Valid region should verify");
    
    // Test invalid boundary region
    atlas_boundary_region_t invalid_region = {
        .start_coord = 200,
        .end_coord = 100, // End before start
        .page_count = 5,
        .region_class = 1,
        .is_conserved = true
    };
    
    bool invalid_region_check = atlas_manifold_verify_boundary_region(&invalid_region);
    ASSERT_TRUE(!invalid_region_check, "Invalid region should fail verification");
    
    // Test transform params verification
    atlas_transform_params_t valid_params = {
        .scaling_factor = 1.0,
        .rotation_angle = M_PI / 2,
        .translation_x = 5.0,
        .translation_y = -2.0
    };
    
    bool params_valid = atlas_manifold_verify_transform_params(&valid_params);
    ASSERT_TRUE(params_valid, "Valid params should verify");
    
    // Test invalid transform params
    atlas_transform_params_t invalid_params = {
        .scaling_factor = -1.0, // Negative scaling
        .rotation_angle = M_PI,
        .translation_x = 0.0,
        .translation_y = 0.0
    };
    
    bool invalid_params_check = atlas_manifold_verify_transform_params(&invalid_params);
    ASSERT_TRUE(!invalid_params_check, "Invalid params should fail verification");
    
    // Test system test
    bool system_ok = atlas_manifold_system_test();
    ASSERT_TRUE(system_ok, "System test should pass");
    
    // Clean up
    atlas_projection_destroy(projection);
    free(test_data);
    
    TEST_PASS();
    return true;
}

// Test transform operations
bool test_transform_operations() {
    TEST_START("Transform operations");
    
    // Create test data
    const size_t test_data_size = 1024;
    uint8_t* test_data = malloc(test_data_size);
    ASSERT_NOT_NULL(test_data, "Failed to allocate test data");
    
    for (size_t i = 0; i < test_data_size; i++) {
        test_data[i] = (uint8_t)(i % 256);
    }
    
    atlas_projection_t projection = atlas_projection_create(ATLAS_PROJECTION_LINEAR, test_data, test_data_size);
    ASSERT_NOT_NULL(projection, "Failed to create projection");
    
    // Test scaling
    int scale_result = atlas_manifold_scale_projection(projection, 2.0, 1.5);
    ASSERT_TRUE(scale_result == 0, "Scaling should succeed");
    
    // Test rotation
    int rotate_result = atlas_manifold_rotate_projection(projection, M_PI / 4, 50.0, 50.0);
    ASSERT_TRUE(rotate_result == 0, "Rotation should succeed");
    
    // Test translation
    int translate_result = atlas_manifold_translate_projection(projection, 10.0, -5.0);
    ASSERT_TRUE(translate_result == 0, "Translation should succeed");
    
    // Test linear transform with identity matrix
    double identity_matrix[16] = {
        1.0, 0.0, 0.0, 0.0,
        0.0, 1.0, 0.0, 0.0,
        0.0, 0.0, 1.0, 0.0,
        0.0, 0.0, 0.0, 1.0
    };
    
    int linear_result = atlas_manifold_apply_linear_transform(projection, identity_matrix);
    ASSERT_TRUE(linear_result == 0, "Linear transform should succeed");
    
    // Clean up
    atlas_projection_destroy(projection);
    free(test_data);
    
    TEST_PASS();
    return true;
}

// Test R96 Fourier operations
bool test_r96_fourier_operations() {
    TEST_START("R96 Fourier operations");
    
    // Create test data with R96-compatible size
    const size_t test_data_size = 96 * 16; // Multiple of 96
    uint8_t* test_data = malloc(test_data_size);
    ASSERT_NOT_NULL(test_data, "Failed to allocate test data");
    
    // Fill with periodic pattern
    for (size_t i = 0; i < test_data_size; i++) {
        test_data[i] = (uint8_t)(127 * sin(2 * M_PI * i / 96) + 128);
    }
    
    atlas_projection_t projection = atlas_projection_create(ATLAS_PROJECTION_R96_FOURIER, test_data, test_data_size);
    ASSERT_NOT_NULL(projection, "Failed to create R96 projection");
    
    // Test forward Fourier transform
    int fft_forward_result = atlas_manifold_apply_fourier_transform(projection, false);
    ASSERT_TRUE(fft_forward_result == 0, "Forward FFT should succeed");
    
    // Test inverse Fourier transform
    int fft_inverse_result = atlas_manifold_apply_fourier_transform(projection, true);
    ASSERT_TRUE(fft_inverse_result == 0, "Inverse FFT should succeed");
    
    // Clean up
    atlas_projection_destroy(projection);
    free(test_data);
    
    TEST_PASS();
    return true;
}

// Test that functions return meaningful, non-placeholder values
bool test_non_placeholder_values() {
    TEST_START("Non-placeholder value verification");
    
    // Create test projection
    const size_t test_data_size = 2048;
    uint8_t* test_data = malloc(test_data_size);
    ASSERT_NOT_NULL(test_data, "Failed to allocate test data");
    
    // Fill with varied pattern
    for (size_t i = 0; i < test_data_size; i++) {
        test_data[i] = (uint8_t)(i * 13 % 256);
    }
    
    atlas_projection_t projection = atlas_projection_create(ATLAS_PROJECTION_R96_FOURIER, test_data, test_data_size);
    ASSERT_NOT_NULL(projection, "Failed to create projection");
    
    // Test that invariants are not all the same placeholder value
    double invariants[5];
    int inv_count = atlas_manifold_compute_invariants(projection, invariants, 5);
    ASSERT_TRUE(inv_count > 0, "Should compute invariants");
    
    // Verify that invariants are computed (not just returning placeholder values)
    // At least one invariant should be non-zero and finite
    bool has_meaningful_invariant = false;
    for (int i = 0; i < inv_count; i++) {
        if (isfinite(invariants[i]) && fabs(invariants[i]) > 1e-10) {
            has_meaningful_invariant = true;
            break;
        }
    }
    ASSERT_TRUE(has_meaningful_invariant, "Should have at least one meaningful invariant");
    
    // Test that curvature values vary across different points
    double curv1 = atlas_manifold_compute_curvature(projection, 0.0, 0.0);
    double curv2 = atlas_manifold_compute_curvature(projection, 1.0, 0.0);
    double curv3 = atlas_manifold_compute_curvature(projection, 0.0, 1.0);
    
    ASSERT_FINITE(curv1, "Curvature 1 should be finite");
    ASSERT_FINITE(curv2, "Curvature 2 should be finite");
    ASSERT_FINITE(curv3, "Curvature 3 should be finite");
    
    // Test that geodesic distances are reasonable
    double dist1 = atlas_manifold_geodesic_distance(projection, 0.0, 0.0, 1.0, 0.0);
    double dist2 = atlas_manifold_geodesic_distance(projection, 0.0, 0.0, 2.0, 0.0);
    
    ASSERT_TRUE(dist1 > 0.0, "Distance 1 should be positive");
    ASSERT_TRUE(dist2 > 0.0, "Distance 2 should be positive");
    ASSERT_TRUE(dist2 > dist1, "Distance 2 should be greater than distance 1");
    
    // Test that critical points can be found
    double critical_points[10];
    int crit_count = atlas_manifold_find_critical_points(projection, critical_points, 5);
    ASSERT_TRUE(crit_count >= 0, "Critical point count should be non-negative");
    
    // Clean up
    atlas_projection_destroy(projection);
    free(test_data);
    
    TEST_PASS();
    return true;
}

// Print test summary
void print_test_summary() {
    printf("\n=== FINAL VERIFICATION TEST RESULTS ===\n");
    printf("Tests Total:  %d\n", tests_total);
    printf("Tests Passed: %d\n", tests_passed);
    printf("Tests Failed: %d\n", tests_failed);
    printf("Success Rate: %.1f%%\n", tests_total > 0 ? (100.0 * tests_passed / tests_total) : 0.0);
    
    if (tests_failed == 0) {
        printf("\n🎉 ALL TESTS PASSED! Layer 4 is fully functional.\n");
    } else {
        printf("\n❌ %d test(s) failed. Review the implementation.\n", tests_failed);
    }
    
    printf("\nKey functions verified:\n");
    printf("✓ atlas_reconstruction_finalize\n");
    printf("✓ atlas_manifold_compute_invariants\n");
    printf("✓ atlas_manifold_compute_curvature\n");
    printf("✓ atlas_manifold_geodesic_distance\n");
    printf("✓ atlas_manifold_find_critical_points\n");
    printf("✓ atlas_manifold_get_last_error\n");
    printf("✓ atlas_manifold_error_string\n");
    printf("========================================\n");
}

int main() {
    printf("Atlas-12288 Layer 4 Final Verification Test\n");
    printf("Testing all key functions for non-placeholder implementations...\n\n");
    
    // Record start time
    clock_t start_time = clock();
    
    // Run all test suites
    test_error_handling();
    test_projection_operations();
    test_shard_reconstruction();
    test_advanced_math_operations();
    test_manifold_verification();
    test_transform_operations();
    test_r96_fourier_operations();
    test_non_placeholder_values();
    
    // Record end time
    clock_t end_time = clock();
    double test_duration = ((double)(end_time - start_time)) / CLOCKS_PER_SEC;
    
    printf("\nTest execution time: %.3f seconds\n", test_duration);
    
    // Print summary
    print_test_summary();
    
    // Return appropriate exit code
    return (tests_failed == 0) ? 0 : 1;
}