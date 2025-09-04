#include <stdio.h>
#include <stdlib.h>
#include <math.h>

// Simple test to verify the segfault is fixed - uses only basic functions
extern double atlas_manifold_compute_curvature_simple(double x, double y);
extern double atlas_manifold_geodesic_distance_simple(double x1, double y1, double x2, double y2);
extern int atlas_manifold_compute_invariants_simple(double* invariants, size_t max_invariants);
extern int atlas_manifold_find_critical_points_simple(double* critical_points, size_t max_points);

int main() {
    printf("Testing segfault fix...\n");
    
    // Test 1: Curvature computation
    printf("Testing curvature computation... ");
    double curvature = atlas_manifold_compute_curvature_simple(0.0, 0.0);
    if (isfinite(curvature)) {
        printf("PASS (%.6f)\n", curvature);
    } else {
        printf("FAIL (NaN or infinite)\n");
        return 1;
    }
    
    // Test 2: Geodesic distance
    printf("Testing geodesic distance... ");
    double distance = atlas_manifold_geodesic_distance_simple(0.0, 0.0, 1.0, 1.0);
    if (isfinite(distance) && distance >= 0.0) {
        printf("PASS (%.6f)\n", distance);
    } else {
        printf("FAIL (negative or invalid)\n");
        return 1;
    }
    
    // Test 3: Invariants computation (this is where the original segfault occurred)
    printf("Testing invariants computation... ");
    double invariants[5];
    int count = atlas_manifold_compute_invariants_simple(invariants, 5);
    if (count > 0) {
        printf("PASS (%d invariants computed)\n", count);
    } else {
        printf("FAIL (no invariants computed)\n");
        return 1;
    }
    
    // Test 4: Critical points
    printf("Testing critical points... ");
    double critical_points[10]; // 5 points * 2 coordinates
    int crit_count = atlas_manifold_find_critical_points_simple(critical_points, 5);
    if (crit_count >= 0) {
        printf("PASS (%d critical points found)\n", crit_count);
    } else {
        printf("FAIL (error occurred)\n");
        return 1;
    }
    
    printf("All tests passed! Segfault is fixed.\n");
    return 0;
}