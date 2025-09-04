/* runtime/topology.c - Topological operations for manifolds
 * (c) 2024-2025 UOR Foundation - MIT License
 * 
 * C runtime support for topological operations in Atlas-12288 Layer 4:
 * - Topological invariant calculations
 * - Manifold connectivity analysis
 * - Critical point detection and classification
 * - Boundary analysis and validation
 * - Curvature approximation methods
 */

#include "../include/atlas-manifold.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <float.h>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

// =============================================================================
// External Function Declarations from manifold.c and geometry.c
// =============================================================================

extern void atlas_manifold_set_error(atlas_manifold_error_t error);
extern bool atlas_manifold_validate_pointer(const void* ptr, size_t required_alignment);
extern double atlas_manifold_euclidean_distance_2d(double x1, double y1, double x2, double y2);
extern bool atlas_manifold_point_in_bounds(double x, double y, double min_x, double min_y, double max_x, double max_y);

// =============================================================================
// Topological Data Structures
// =============================================================================

/**
 * Structure representing a critical point in the manifold.
 */
typedef struct {
    double x;                    /* X coordinate */
    double y;                    /* Y coordinate */
    double value;                /* Function value at the point */
    int type;                    /* Critical point type (0=minimum, 1=maximum, 2=saddle) */
    double curvature;            /* Local curvature estimate */
    bool is_boundary;            /* Whether point is on boundary */
} atlas_critical_point_t;

/**
 * Structure for storing topological invariants.
 */
typedef struct {
    int euler_characteristic;    /* Euler characteristic (χ = V - E + F) */
    int genus;                   /* Topological genus */
    int num_connected_components; /* Number of connected components */
    int num_boundary_components; /* Number of boundary components */
    double total_curvature;      /* Total curvature (approximate) */
    double surface_area;         /* Total surface area */
} atlas_topology_invariants_t;

// =============================================================================
// Curvature Computation Functions
// =============================================================================

/**
 * Estimate local curvature using finite differences.
 * This is a simplified approach suitable for discrete manifold data.
 * 
 * @param projection Projection handle for accessing manifold data
 * @param x X coordinate for curvature calculation
 * @param y Y coordinate for curvature calculation
 * @param epsilon Step size for finite differences
 * @return Curvature estimate, or NaN on error
 */
static double atlas_topology_estimate_local_curvature(const atlas_projection_t projection,
                                                     double x, double y, double epsilon) {
    if (projection == NULL || !isfinite(x) || !isfinite(y) || epsilon <= 0.0) {
        atlas_manifold_set_error(ATLAS_MANIFOLD_ERROR_INVALID_ARGUMENT);
        return NAN;
    }
    
    // Use the public API function for curvature calculation
    double curvature = atlas_manifold_compute_curvature(projection, x, y);
    
    if (!isfinite(curvature)) {
        atlas_manifold_set_error(ATLAS_MANIFOLD_ERROR_VERIFICATION_FAILED);
        return NAN;
    }
    
    return curvature;
}

/**
 * Calculate Gaussian curvature approximation at a point.
 * Uses surrounding neighborhood to estimate curvature.
 * 
 * @param projection Projection handle  
 * @param x X coordinate
 * @param y Y coordinate
 * @param radius Neighborhood radius for sampling
 * @param num_samples Number of samples around the point
 * @return Gaussian curvature estimate, or NaN on error
 */
double atlas_topology_gaussian_curvature(const atlas_projection_t projection,
                                        double x, double y, 
                                        double radius, int num_samples) {
    if (projection == NULL || !isfinite(x) || !isfinite(y) ||
        radius <= 0.0 || num_samples < 3) {
        atlas_manifold_set_error(ATLAS_MANIFOLD_ERROR_INVALID_ARGUMENT);
        return NAN;
    }
    
    if (num_samples > 360) num_samples = 360; // Reasonable upper bound
    
    double total_curvature = 0.0;
    int valid_samples = 0;
    
    // Sample points around the circle
    for (int i = 0; i < num_samples; i++) {
        double angle = 2.0 * M_PI * i / num_samples;
        double sample_x = x + radius * cos(angle);
        double sample_y = y + radius * sin(angle);
        
        double local_curvature = atlas_manifold_compute_curvature(projection, sample_x, sample_y);
        
        if (isfinite(local_curvature)) {
            total_curvature += local_curvature;
            valid_samples++;
        }
    }
    
    if (valid_samples == 0) {
        atlas_manifold_set_error(ATLAS_MANIFOLD_ERROR_VERIFICATION_FAILED);
        return NAN;
    }
    
    double average_curvature = total_curvature / valid_samples;
    
    // Apply scaling factor based on radius for Gaussian curvature approximation
    double gaussian_curvature = average_curvature / (radius * radius);
    
    atlas_manifold_set_error(ATLAS_MANIFOLD_SUCCESS);
    return gaussian_curvature;
}

/**
 * Calculate mean curvature approximation at a point.
 * 
 * @param projection Projection handle
 * @param x X coordinate  
 * @param y Y coordinate
 * @param radius Neighborhood radius for sampling
 * @return Mean curvature estimate, or NaN on error
 */
double atlas_topology_mean_curvature(const atlas_projection_t projection,
                                    double x, double y, double radius) {
    if (projection == NULL || !isfinite(x) || !isfinite(y) || radius <= 0.0) {
        atlas_manifold_set_error(ATLAS_MANIFOLD_ERROR_INVALID_ARGUMENT);
        return NAN;
    }
    
    // For 2D manifolds embedded in 3D, mean curvature is half the sum of principal curvatures
    // This is a simplified approximation
    double center_curvature = atlas_manifold_compute_curvature(projection, x, y);
    
    if (!isfinite(center_curvature)) {
        atlas_manifold_set_error(ATLAS_MANIFOLD_ERROR_VERIFICATION_FAILED);
        return NAN;
    }
    
    // Sample in orthogonal directions to estimate principal curvatures
    double curvature_x_pos = atlas_manifold_compute_curvature(projection, x + radius, y);
    double curvature_x_neg = atlas_manifold_compute_curvature(projection, x - radius, y);
    double curvature_y_pos = atlas_manifold_compute_curvature(projection, x, y + radius);
    double curvature_y_neg = atlas_manifold_compute_curvature(projection, x, y - radius);
    
    double valid_samples = 1.0; // Center point
    double total_curvature = center_curvature;
    
    if (isfinite(curvature_x_pos)) {
        total_curvature += curvature_x_pos;
        valid_samples += 1.0;
    }
    if (isfinite(curvature_x_neg)) {
        total_curvature += curvature_x_neg;
        valid_samples += 1.0;
    }
    if (isfinite(curvature_y_pos)) {
        total_curvature += curvature_y_pos;
        valid_samples += 1.0;
    }
    if (isfinite(curvature_y_neg)) {
        total_curvature += curvature_y_neg;
        valid_samples += 1.0;
    }
    
    double mean_curvature = total_curvature / valid_samples;
    
    atlas_manifold_set_error(ATLAS_MANIFOLD_SUCCESS);
    return mean_curvature;
}

// =============================================================================
// Critical Point Detection and Classification
// =============================================================================

/**
 * Find and classify critical points in a projection.
 * This implements a simplified critical point detection algorithm.
 * 
 * @param projection Projection to analyze
 * @param critical_points Output array for critical points  
 * @param max_points Maximum number of points to find
 * @param search_region Bounding box for search (min_x, min_y, max_x, max_y) or NULL for full projection
 * @return Number of critical points found, or -1 on error
 */
int atlas_topology_find_critical_points_detailed(const atlas_projection_t projection,
                                                atlas_critical_point_t* critical_points,
                                                size_t max_points,
                                                const double search_region[4]) {
    if (projection == NULL || critical_points == NULL || max_points == 0) {
        atlas_manifold_set_error(ATLAS_MANIFOLD_ERROR_INVALID_ARGUMENT);
        return -1;
    }
    
    // Get projection dimensions
    uint32_t width, height;
    if (atlas_projection_get_dimensions(projection, &width, &height) != 0) {
        return -1;
    }
    
    // Set search bounds
    double min_x = 0.0, min_y = 0.0;
    double max_x = (double)width, max_y = (double)height;
    
    if (search_region != NULL) {
        min_x = search_region[0];
        min_y = search_region[1]; 
        max_x = search_region[2];
        max_y = search_region[3];
        
        if (min_x >= max_x || min_y >= max_y) {
            atlas_manifold_set_error(ATLAS_MANIFOLD_ERROR_INVALID_ARGUMENT);
            return -1;
        }
    }
    
    size_t found_points = 0;
    double search_step = fmin((max_x - min_x) / 50.0, (max_y - min_y) / 50.0);
    if (search_step <= 0.0) search_step = 1.0;
    
    // Grid-based search for critical points
    for (double x = min_x; x < max_x && found_points < max_points; x += search_step) {
        for (double y = min_y; y < max_y && found_points < max_points; y += search_step) {
            
            // Calculate local curvature
            double curvature = atlas_manifold_compute_curvature(projection, x, y);
            if (!isfinite(curvature)) {
                continue;
            }
            
            // Simple critical point detection: look for significant curvature changes
            bool is_critical = false;
            int point_type = 0; // 0=minimum, 1=maximum, 2=saddle
            
            // Sample neighborhood to determine critical point type
            double epsilon = search_step * 0.5;
            double curvature_left = atlas_manifold_compute_curvature(projection, x - epsilon, y);
            double curvature_right = atlas_manifold_compute_curvature(projection, x + epsilon, y);
            double curvature_up = atlas_manifold_compute_curvature(projection, x, y + epsilon);
            double curvature_down = atlas_manifold_compute_curvature(projection, x, y - epsilon);
            
            if (isfinite(curvature_left) && isfinite(curvature_right) &&
                isfinite(curvature_up) && isfinite(curvature_down)) {
                
                // Analyze curvature gradient
                double grad_x = (curvature_right - curvature_left) / (2.0 * epsilon);
                double grad_y = (curvature_up - curvature_down) / (2.0 * epsilon);
                double grad_magnitude = sqrt(grad_x * grad_x + grad_y * grad_y);
                
                // Critical point if gradient is small (local extremum)
                if (grad_magnitude < 0.1) {
                    is_critical = true;
                    
                    // Classify based on surrounding curvature values
                    int higher_count = 0;
                    int lower_count = 0;
                    
                    if (curvature_left > curvature) higher_count++; else lower_count++;
                    if (curvature_right > curvature) higher_count++; else lower_count++;
                    if (curvature_up > curvature) higher_count++; else lower_count++;
                    if (curvature_down > curvature) higher_count++; else lower_count++;
                    
                    if (higher_count == 4) {
                        point_type = 0; // Local minimum
                    } else if (lower_count == 4) {
                        point_type = 1; // Local maximum
                    } else {
                        point_type = 2; // Saddle point
                    }
                }
            }
            
            if (is_critical) {
                atlas_critical_point_t* cp = &critical_points[found_points];
                cp->x = x;
                cp->y = y;
                cp->value = curvature;
                cp->type = point_type;
                cp->curvature = curvature;
                cp->is_boundary = (x <= min_x + epsilon || x >= max_x - epsilon ||
                                  y <= min_y + epsilon || y >= max_y - epsilon);
                
                found_points++;
            }
        }
    }
    
    atlas_manifold_set_error(ATLAS_MANIFOLD_SUCCESS);
    return (int)found_points;
}

// =============================================================================
// Connected Components Analysis
// =============================================================================

// Helper: Find root with path compression
static int find_root_impl(int* parent_array, int node) {
    if (parent_array[node] != node) {
        parent_array[node] = find_root_impl(parent_array, parent_array[node]);
    }
    return parent_array[node];
}

// Helper: Union two sets
static void union_sets(int* parent_array, int a, int b) {
    int root_a = find_root_impl(parent_array, a);
    int root_b = find_root_impl(parent_array, b);
    if (root_a != root_b) {
        parent_array[root_a] = root_b;
    }
}

/**
 * Count connected components in the manifold using Union-Find algorithm.
 * 
 * @param projection Projection containing the manifold
 * @param width Width of the manifold grid
 * @param height Height of the manifold grid
 * @return Number of connected components, or 1 if unable to determine
 */
static int atlas_topology_count_connected_components(
    const atlas_projection_t* projection,
    uint32_t width,
    uint32_t height
) {
    if (projection == NULL || width == 0 || height == 0) {
        return 1;
    }
    
    // Allocate Union-Find parent array
    size_t total_nodes = width * height;
    int* parent = (int*)calloc(total_nodes, sizeof(int));
    if (parent == NULL) {
        return 1; // Memory allocation failed, default to single component
    }
    
    // Initialize each node as its own parent
    for (size_t i = 0; i < total_nodes; i++) {
        parent[i] = (int)i;
    }
    
    // Check connectivity between adjacent nodes
    for (uint32_t y = 0; y < height; y++) {
        for (uint32_t x = 0; x < width; x++) {
            int current = (int)(y * width + x);
            
            // Calculate curvature at current point
            double current_curvature = atlas_manifold_compute_curvature(
                *projection, (double)x, (double)y);
            
            // Skip invalid points
            if (!isfinite(current_curvature)) {
                continue;
            }
            
            // Check right neighbor
            if (x + 1 < width) {
                double right_curvature = atlas_manifold_compute_curvature(
                    *projection, (double)(x + 1), (double)y);
                if (isfinite(right_curvature)) {
                    // Connect if curvature difference is small (continuous surface)
                    if (fabs(current_curvature - right_curvature) < 1.0) {
                        union_sets(parent, current, current + 1);
                    }
                }
            }
            
            // Check bottom neighbor
            if (y + 1 < height) {
                double bottom_curvature = atlas_manifold_compute_curvature(
                    *projection, (double)x, (double)(y + 1));
                if (isfinite(bottom_curvature)) {
                    // Connect if curvature difference is small (continuous surface)
                    if (fabs(current_curvature - bottom_curvature) < 1.0) {
                        union_sets(parent, current, (int)((y + 1) * width + x));
                    }
                }
            }
        }
    }
    
    // Count unique components
    int component_count = 0;
    bool* seen = (bool*)calloc(total_nodes, sizeof(bool));
    if (seen != NULL) {
        for (size_t i = 0; i < total_nodes; i++) {
            int root = find_root_impl(parent, (int)i);
            if (!seen[root]) {
                seen[root] = true;
                component_count++;
            }
        }
        free(seen);
    } else {
        component_count = 1; // Default if memory allocation failed
    }
    
    free(parent);
    return (component_count > 0) ? component_count : 1;
}

// =============================================================================
// Topological Invariant Calculations  
// =============================================================================

/**
 * Calculate topological invariants for a manifold projection.
 * This provides approximate values based on discrete sampling.
 * 
 * @param projection Projection to analyze
 * @param invariants Output structure for invariants
 * @return 0 on success, -1 on error
 */
int atlas_topology_calculate_invariants(const atlas_projection_t projection,
                                       atlas_topology_invariants_t* invariants) {
    if (projection == NULL || invariants == NULL) {
        atlas_manifold_set_error(ATLAS_MANIFOLD_ERROR_INVALID_ARGUMENT);
        return -1;
    }
    
    // Initialize invariants structure
    memset(invariants, 0, sizeof(atlas_topology_invariants_t));
    
    // Get projection dimensions
    uint32_t width, height;
    if (atlas_projection_get_dimensions(projection, &width, &height) != 0) {
        return -1;
    }
    
    // Calculate surface area approximation
    double dx = 1.0, dy = 1.0; // Grid spacing
    invariants->surface_area = (double)width * (double)height * dx * dy;
    
    // Find critical points
    atlas_critical_point_t critical_points[100]; // Maximum 100 critical points
    int num_critical = atlas_topology_find_critical_points_detailed(
        projection, critical_points, 100, NULL);
    
    if (num_critical < 0) {
        return -1;
    }
    
    // Count different types of critical points
    int num_minima = 0, num_maxima = 0, num_saddles = 0;
    double total_curvature = 0.0;
    
    for (int i = 0; i < num_critical; i++) {
        switch (critical_points[i].type) {
            case 0: num_minima++; break;
            case 1: num_maxima++; break;
            case 2: num_saddles++; break;
        }
        total_curvature += critical_points[i].curvature;
    }
    
    invariants->total_curvature = total_curvature;
    
    // Simplified Euler characteristic calculation
    // For a 2D manifold: χ = V - E + F
    // In our discrete case, approximate as: χ ≈ minima - saddles + maxima
    invariants->euler_characteristic = num_minima - num_saddles + num_maxima;
    
    // Estimate genus from Euler characteristic
    // For a closed orientable surface: χ = 2 - 2g, so g = (2 - χ) / 2
    invariants->genus = (2 - invariants->euler_characteristic) / 2;
    if (invariants->genus < 0) invariants->genus = 0;
    
    // Calculate connected components using Union-Find algorithm
    invariants->num_connected_components = atlas_topology_count_connected_components(
        &projection, width, height);
    
    // Estimate boundary components
    int boundary_critical_points = 0;
    for (int i = 0; i < num_critical; i++) {
        if (critical_points[i].is_boundary) {
            boundary_critical_points++;
        }
    }
    invariants->num_boundary_components = (boundary_critical_points > 0) ? 1 : 0;
    
    atlas_manifold_set_error(ATLAS_MANIFOLD_SUCCESS);
    return 0;
}

// =============================================================================
// Boundary Analysis Functions
// =============================================================================

/**
 * Validate a boundary region for topological consistency.
 * 
 * @param region Boundary region to validate
 * @return true if region is topologically valid, false otherwise
 */
bool atlas_topology_validate_boundary_region(const atlas_boundary_region_t* region) {
    if (region == NULL) {
        atlas_manifold_set_error(ATLAS_MANIFOLD_ERROR_INVALID_ARGUMENT);
        return false;
    }
    
    // Basic sanity checks
    if (region->start_coord >= region->end_coord) {
        atlas_manifold_set_error(ATLAS_MANIFOLD_ERROR_BOUNDARY_INVALID);
        return false;
    }
    
    if (region->page_count == 0) {
        atlas_manifold_set_error(ATLAS_MANIFOLD_ERROR_BOUNDARY_INVALID);
        return false;
    }
    
    // Check that coordinate range matches expected page count
    uint32_t coord_range = region->end_coord - region->start_coord;
    uint32_t expected_size = region->page_count * 4096; // Assuming 4KB pages
    
    if (coord_range != expected_size) {
        atlas_manifold_set_error(ATLAS_MANIFOLD_ERROR_BOUNDARY_INVALID);
        return false;
    }
    
    // Check resonance class is valid (0-95 for R96 system)
    if (region->region_class >= 96) {
        atlas_manifold_set_error(ATLAS_MANIFOLD_ERROR_BOUNDARY_INVALID);
        return false;
    }
    
    atlas_manifold_set_error(ATLAS_MANIFOLD_SUCCESS);
    return true;
}

/**
 * Calculate the topological complexity of a boundary region.
 * 
 * @param region Boundary region to analyze
 * @return Complexity measure (higher values indicate more complex topology), or -1.0 on error
 */
double atlas_topology_boundary_complexity(const atlas_boundary_region_t* region) {
    if (!atlas_topology_validate_boundary_region(region)) {
        return -1.0;
    }
    
    double complexity = 0.0;
    
    // Factor 1: Size complexity (larger regions are more complex)
    complexity += log((double)region->page_count + 1.0);
    
    // Factor 2: Coordinate range complexity
    double coord_span = (double)(region->end_coord - region->start_coord);
    complexity += log(coord_span + 1.0) / 10.0;
    
    // Factor 3: Resonance class contribution (some classes are inherently more complex)
    double class_factor = 1.0;
    if (region->region_class % 3 == 0) class_factor = 1.5; // Multiples of 3 are more complex
    if (region->region_class % 7 == 0) class_factor = 2.0; // Multiples of 7 are very complex  
    complexity *= class_factor;
    
    // Factor 4: Conservation status (non-conserved regions add complexity)
    if (!region->is_conserved) {
        complexity *= 1.3;
    }
    
    atlas_manifold_set_error(ATLAS_MANIFOLD_SUCCESS);
    return complexity;
}

// =============================================================================
// Connectivity and Path Analysis
// =============================================================================

/**
 * Check if two points are topologically connected in the manifold.
 * This uses a simplified connectivity check based on curvature similarity.
 * 
 * @param projection Projection containing the manifold
 * @param x1 X coordinate of first point
 * @param y1 Y coordinate of first point
 * @param x2 X coordinate of second point
 * @param y2 Y coordinate of second point
 * @param max_curvature_diff Maximum allowed curvature difference for connectivity
 * @return true if points appear connected, false otherwise
 */
bool atlas_topology_points_connected(const atlas_projection_t projection,
                                    double x1, double y1, double x2, double y2,
                                    double max_curvature_diff) {
    if (projection == NULL || !isfinite(x1) || !isfinite(y1) ||
        !isfinite(x2) || !isfinite(y2) || max_curvature_diff < 0.0) {
        atlas_manifold_set_error(ATLAS_MANIFOLD_ERROR_INVALID_ARGUMENT);
        return false;
    }
    
    // Calculate curvatures at both points
    double curvature1 = atlas_manifold_compute_curvature(projection, x1, y1);
    double curvature2 = atlas_manifold_compute_curvature(projection, x2, y2);
    
    if (!isfinite(curvature1) || !isfinite(curvature2)) {
        atlas_manifold_set_error(ATLAS_MANIFOLD_ERROR_VERIFICATION_FAILED);
        return false;
    }
    
    // Simple connectivity test: similar curvatures suggest connection
    double curvature_diff = fabs(curvature2 - curvature1);
    
    if (curvature_diff <= max_curvature_diff) {
        // Additional test: check intermediate points along straight line path
        double distance = atlas_manifold_euclidean_distance_2d(x1, y1, x2, y2);
        if (distance < 0.0) {
            return false;
        }
        
        int num_samples = (int)(distance / 2.0) + 1; // Sample every 2 units
        if (num_samples > 50) num_samples = 50; // Reasonable limit
        
        for (int i = 1; i < num_samples; i++) {
            double t = (double)i / (double)num_samples;
            double x = x1 + t * (x2 - x1);
            double y = y1 + t * (y2 - y1);
            
            double intermediate_curvature = atlas_manifold_compute_curvature(projection, x, y);
            if (!isfinite(intermediate_curvature)) {
                continue; // Skip problematic points
            }
            
            double diff1 = fabs(intermediate_curvature - curvature1);
            double diff2 = fabs(intermediate_curvature - curvature2);
            
            // If intermediate point is very different from both endpoints, connection is dubious
            if (diff1 > max_curvature_diff * 2.0 && diff2 > max_curvature_diff * 2.0) {
                atlas_manifold_set_error(ATLAS_MANIFOLD_SUCCESS);
                return false;
            }
        }
        
        atlas_manifold_set_error(ATLAS_MANIFOLD_SUCCESS);
        return true;
    }
    
    atlas_manifold_set_error(ATLAS_MANIFOLD_SUCCESS);
    return false;
}