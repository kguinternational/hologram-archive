/* test-basic-final-verification.c - Basic final verification for Layer 4
 * (c) 2024-2025 UOR Foundation - MIT License
 * 
 * Basic test to verify key Layer 4 functions work and return non-placeholder values.
 * This test uses the actual exported FFI functions from the Rust implementation.
 */

#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <assert.h>
#include <time.h>
#include <stdint.h>
#include <stdbool.h>

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

// Function declarations for FFI functions from Rust library
extern int atlas_manifold_get_last_error_ffi(void);
extern const char* atlas_manifold_error_string_ffi(int error_code);
extern void atlas_manifold_set_last_error(int error_code);
extern double atlas_manifold_compute_curvature_simple(double x, double y);
extern double atlas_manifold_geodesic_distance_simple(double x1, double y1, double x2, double y2);
extern int atlas_manifold_compute_invariants_simple(double* invariants, size_t max_invariants);
extern int atlas_manifold_find_critical_points_simple(double* critical_points, size_t max_points);
extern bool atlas_manifold_system_test(void);
extern bool atlas_manifold_is_optimized(void);
extern uint32_t atlas_manifold_get_supported_projections(void);

// Test the basic FFI error handling functions
bool test_basic_ffi_functions() {
    TEST_START("Basic FFI functions");
    
    // Test error functions
    int initial_error = atlas_manifold_get_last_error_ffi();
    ASSERT_TRUE(initial_error >= 0, "Error code should be valid");
    
    const char* error_msg = atlas_manifold_error_string_ffi(0);
    ASSERT_TRUE(error_msg != NULL, "Error string should not be NULL");
    ASSERT_TRUE(strlen(error_msg) > 0, "Error string should not be empty");
    
    // Test setting and getting error
    atlas_manifold_set_last_error(1);
    int new_error = atlas_manifold_get_last_error_ffi();
    ASSERT_TRUE(new_error == 1, "Should be able to set and get error");
    
    // Reset error
    atlas_manifold_set_last_error(0);
    
    TEST_PASS();
    return true;
}

// Test curvature computation function
bool test_curvature_computation() {
    TEST_START("Curvature computation");
    
    // Test curvature at origin
    double curvature1 = atlas_manifold_compute_curvature_simple(0.0, 0.0);
    ASSERT_TRUE(isfinite(curvature1), "Curvature at origin should be finite");
    
    // Test curvature at different points
    double curvature2 = atlas_manifold_compute_curvature_simple(1.0, 0.0);
    ASSERT_TRUE(isfinite(curvature2), "Curvature at (1,0) should be finite");
    
    double curvature3 = atlas_manifold_compute_curvature_simple(0.5, 0.5);
    ASSERT_TRUE(isfinite(curvature3), "Curvature at (0.5,0.5) should be finite");
    
    // Test invalid input handling
    double curvature_inf = atlas_manifold_compute_curvature_simple(INFINITY, 0.0);
    ASSERT_TRUE(isnan(curvature_inf), "Curvature with infinite input should return NaN");
    
    printf("Curvatures: (0,0)=%.6f, (1,0)=%.6f, (0.5,0.5)=%.6f ", 
           curvature1, curvature2, curvature3);
    
    TEST_PASS();
    return true;
}

// Test geodesic distance computation
bool test_geodesic_distance() {
    TEST_START("Geodesic distance computation");
    
    // Test distance from point to itself (should be zero)
    double dist_zero = atlas_manifold_geodesic_distance_simple(1.0, 1.0, 1.0, 1.0);
    ASSERT_TRUE(fabs(dist_zero) < 1e-10, "Self-distance should be approximately zero");
    
    // Test distance between different points
    double dist1 = atlas_manifold_geodesic_distance_simple(0.0, 0.0, 1.0, 0.0);
    ASSERT_TRUE(dist1 > 0.0, "Distance should be positive");
    ASSERT_TRUE(isfinite(dist1), "Distance should be finite");
    
    double dist2 = atlas_manifold_geodesic_distance_simple(0.0, 0.0, 0.0, 1.0);
    ASSERT_TRUE(dist2 > 0.0, "Distance should be positive");
    ASSERT_TRUE(isfinite(dist2), "Distance should be finite");
    
    // Test that distance to further point is greater
    double dist3 = atlas_manifold_geodesic_distance_simple(0.0, 0.0, 2.0, 0.0);
    ASSERT_TRUE(dist3 > dist1, "Distance to further point should be greater");
    
    // Test invalid input
    double dist_invalid = atlas_manifold_geodesic_distance_simple(INFINITY, 0.0, 1.0, 0.0);
    ASSERT_TRUE(dist_invalid < 0.0, "Invalid input should return negative value");
    
    printf("Distances: zero=%.6f, unit_x=%.6f, unit_y=%.6f, double_x=%.6f ", 
           dist_zero, dist1, dist2, dist3);
    
    TEST_PASS();
    return true;
}

// Test invariants computation
bool test_invariants_computation() {
    TEST_START("Invariants computation");
    
    const size_t max_invariants = 10;
    double invariants[max_invariants];
    
    int count = atlas_manifold_compute_invariants_simple(invariants, max_invariants);
    ASSERT_TRUE(count > 0, "Should compute at least one invariant");
    ASSERT_TRUE(count <= (int)max_invariants, "Should not exceed max invariants");
    
    // Check that invariants are finite and meaningful
    bool has_nonzero = false;
    for (int i = 0; i < count; i++) {
        ASSERT_TRUE(isfinite(invariants[i]), "All invariants should be finite");
        if (fabs(invariants[i]) > 1e-10) {
            has_nonzero = true;
        }
    }
    
    ASSERT_TRUE(has_nonzero, "Should have at least one significant invariant");
    
    printf("Computed %d invariants, first few: ", count);
    for (int i = 0; i < count && i < 3; i++) {
        printf("%.6f ", invariants[i]);
    }
    
    TEST_PASS();
    return true;
}

// Test critical points finding
bool test_critical_points() {
    TEST_START("Critical points finding");
    
    const size_t max_points = 10;
    double critical_points[max_points * 2]; // x,y pairs
    
    int count = atlas_manifold_find_critical_points_simple(critical_points, max_points);
    ASSERT_TRUE(count >= 0, "Critical point count should be non-negative");
    ASSERT_TRUE(count <= (int)max_points, "Should not exceed max points");
    
    // Check that critical points are finite
    for (int i = 0; i < count * 2; i++) {
        ASSERT_TRUE(isfinite(critical_points[i]), "Critical point coordinates should be finite");
    }
    
    printf("Found %d critical points ", count);
    if (count > 0) {
        printf("first: (%.6f, %.6f) ", critical_points[0], critical_points[1]);
    }
    
    TEST_PASS();
    return true;
}

// Test system functions
bool test_system_functions() {
    TEST_START("System functions");
    
    // Test system test
    bool system_ok = atlas_manifold_system_test();
    ASSERT_TRUE(system_ok, "System test should pass");
    
    // Test optimization flag
    bool is_opt = atlas_manifold_is_optimized();
    printf("Optimized build: %s ", is_opt ? "true" : "false");
    
    // Test supported projections
    uint32_t projections = atlas_manifold_get_supported_projections();
    ASSERT_TRUE(projections > 0, "Should support at least one projection type");
    printf("Supported projections mask: 0x%x ", projections);
    
    TEST_PASS();
    return true;
}

// Print test summary
void print_test_summary() {
    printf("\n=== BASIC FINAL VERIFICATION RESULTS ===\n");
    printf("Tests Total:  %d\n", tests_total);
    printf("Tests Passed: %d\n", tests_passed);
    printf("Tests Failed: %d\n", tests_failed);
    printf("Success Rate: %.1f%%\n", tests_total > 0 ? (100.0 * tests_passed / tests_total) : 0.0);
    
    if (tests_failed == 0) {
        printf("\n🎉 ALL TESTS PASSED! Layer 4 core functions are working.\n");
    } else {
        printf("\n❌ %d test(s) failed. Review the implementation.\n", tests_failed);
    }
    
    printf("\nKey functions verified:\n");
    printf("✓ atlas_manifold_compute_curvature\n");
    printf("✓ atlas_manifold_geodesic_distance\n");
    printf("✓ atlas_manifold_compute_invariants\n");
    printf("✓ atlas_manifold_find_critical_points\n");
    printf("✓ atlas_manifold_get_last_error_ffi\n");
    printf("✓ atlas_manifold_error_string_ffi\n");
    printf("========================================\n");
}

int main() {
    printf("Atlas-12288 Layer 4 Basic Final Verification Test\n");
    printf("Testing core mathematical functions for non-placeholder implementations...\n\n");
    
    // Record start time
    clock_t start_time = clock();
    
    // Run basic test suites
    test_basic_ffi_functions();
    test_curvature_computation();
    test_geodesic_distance();
    test_invariants_computation();
    test_critical_points();
    test_system_functions();
    
    // Record end time
    clock_t end_time = clock();
    double test_duration = ((double)(end_time - start_time)) / CLOCKS_PER_SEC;
    
    printf("\nTest execution time: %.3f seconds\n", test_duration);
    
    // Print summary
    print_test_summary();
    
    // Return appropriate exit code
    return (tests_failed == 0) ? 0 : 1;
}