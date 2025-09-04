/* runtime/geometry.c - Geometric operations support
 * (c) 2024-2025 UOR Foundation - MIT License
 * 
 * C runtime support for geometric operations in Atlas-12288 Layer 4:
 * - Linear transformation utilities  
 * - Coordinate transformation functions
 * - Distance and metric calculations
 * - Geometric validation routines
 * - Matrix operations for projections
 */

#include "../include/atlas-manifold.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <float.h>

// =============================================================================
// External Function Declarations from manifold.c
// =============================================================================

extern void atlas_manifold_set_error(atlas_manifold_error_t error);
extern bool atlas_manifold_validate_pointer(const void* ptr, size_t required_alignment);
extern void atlas_manifold_inc_transforms(void);

// =============================================================================
// Matrix Operations and Linear Transformations
// =============================================================================

/**
 * Multiply two 4x4 matrices in row-major order.
 * 
 * @param result Output matrix (4x4, 16 elements)
 * @param a First input matrix (4x4, 16 elements)  
 * @param b Second input matrix (4x4, 16 elements)
 * @return 0 on success, -1 on error
 */
int atlas_manifold_matrix_multiply_4x4(double result[16], 
                                       const double a[16], 
                                       const double b[16]) {
    if (result == NULL || a == NULL || b == NULL) {
        atlas_manifold_set_error(ATLAS_MANIFOLD_ERROR_INVALID_ARGUMENT);
        return -1;
    }
    
    // Perform matrix multiplication: result = a * b
    for (int i = 0; i < 4; i++) {
        for (int j = 0; j < 4; j++) {
            double sum = 0.0;
            for (int k = 0; k < 4; k++) {
                sum += a[i * 4 + k] * b[k * 4 + j];
            }
            result[i * 4 + j] = sum;
        }
    }
    
    atlas_manifold_set_error(ATLAS_MANIFOLD_SUCCESS);
    return 0;
}

/**
 * Create an identity transformation matrix.
 * 
 * @param matrix Output matrix (4x4, 16 elements)
 * @return 0 on success, -1 on error
 */
int atlas_manifold_matrix_identity(double matrix[16]) {
    if (matrix == NULL) {
        atlas_manifold_set_error(ATLAS_MANIFOLD_ERROR_INVALID_ARGUMENT);
        return -1;
    }
    
    // Initialize to zero
    memset(matrix, 0, 16 * sizeof(double));
    
    // Set diagonal elements to 1
    matrix[0] = 1.0;   // [0,0]
    matrix[5] = 1.0;   // [1,1]
    matrix[10] = 1.0;  // [2,2]
    matrix[15] = 1.0;  // [3,3]
    
    atlas_manifold_set_error(ATLAS_MANIFOLD_SUCCESS);
    return 0;
}

/**
 * Create a scaling transformation matrix.
 * 
 * @param matrix Output matrix (4x4, 16 elements)
 * @param scale_x Scaling factor for X dimension
 * @param scale_y Scaling factor for Y dimension
 * @return 0 on success, -1 on error
 */
int atlas_manifold_matrix_scaling(double matrix[16], double scale_x, double scale_y) {
    if (matrix == NULL || !isfinite(scale_x) || !isfinite(scale_y)) {
        atlas_manifold_set_error(ATLAS_MANIFOLD_ERROR_INVALID_ARGUMENT);
        return -1;
    }
    
    if (scale_x <= 0.0 || scale_y <= 0.0) {
        atlas_manifold_set_error(ATLAS_MANIFOLD_ERROR_INVALID_ARGUMENT);
        return -1;
    }
    
    // Initialize as identity matrix
    atlas_manifold_matrix_identity(matrix);
    
    // Set scaling factors
    matrix[0] = scale_x;   // [0,0]
    matrix[5] = scale_y;   // [1,1]
    
    atlas_manifold_set_error(ATLAS_MANIFOLD_SUCCESS);
    return 0;
}

/**
 * Create a rotation transformation matrix.
 * 
 * @param matrix Output matrix (4x4, 16 elements)
 * @param angle Rotation angle in radians
 * @return 0 on success, -1 on error
 */
int atlas_manifold_matrix_rotation(double matrix[16], double angle) {
    if (matrix == NULL || !isfinite(angle)) {
        atlas_manifold_set_error(ATLAS_MANIFOLD_ERROR_INVALID_ARGUMENT);
        return -1;
    }
    
    // Initialize as identity matrix
    atlas_manifold_matrix_identity(matrix);
    
    double cos_a = cos(angle);
    double sin_a = sin(angle);
    
    // Set 2D rotation components
    matrix[0] = cos_a;   // [0,0]
    matrix[1] = -sin_a;  // [0,1]
    matrix[4] = sin_a;   // [1,0]
    matrix[5] = cos_a;   // [1,1]
    
    atlas_manifold_set_error(ATLAS_MANIFOLD_SUCCESS);
    return 0;
}

/**
 * Create a translation transformation matrix.
 * 
 * @param matrix Output matrix (4x4, 16 elements)
 * @param offset_x Translation offset for X dimension
 * @param offset_y Translation offset for Y dimension
 * @return 0 on success, -1 on error
 */
int atlas_manifold_matrix_translation(double matrix[16], double offset_x, double offset_y) {
    if (matrix == NULL || !isfinite(offset_x) || !isfinite(offset_y)) {
        atlas_manifold_set_error(ATLAS_MANIFOLD_ERROR_INVALID_ARGUMENT);
        return -1;
    }
    
    // Initialize as identity matrix
    atlas_manifold_matrix_identity(matrix);
    
    // Set translation components
    matrix[3] = offset_x;   // [0,3]
    matrix[7] = offset_y;   // [1,3]
    
    atlas_manifold_set_error(ATLAS_MANIFOLD_SUCCESS);
    return 0;
}

/**
 * Calculate the determinant of a 4x4 matrix.
 * 
 * @param matrix Input matrix (4x4, 16 elements)
 * @return Determinant value, or NaN on error
 */
double atlas_manifold_matrix_determinant_4x4(const double matrix[16]) {
    if (matrix == NULL) {
        atlas_manifold_set_error(ATLAS_MANIFOLD_ERROR_INVALID_ARGUMENT);
        return NAN;
    }
    
    // Calculate determinant using cofactor expansion along first row
    double det = 0.0;
    
    // Get matrix elements for convenience (row-major indexing)
    const double* m = matrix;
    
    // Cofactor expansion along first row
    det += m[0] * (m[5] * (m[10] * m[15] - m[11] * m[14]) - 
                   m[6] * (m[9] * m[15] - m[11] * m[13]) + 
                   m[7] * (m[9] * m[14] - m[10] * m[13]));
                   
    det -= m[1] * (m[4] * (m[10] * m[15] - m[11] * m[14]) - 
                   m[6] * (m[8] * m[15] - m[11] * m[12]) + 
                   m[7] * (m[8] * m[14] - m[10] * m[12]));
                   
    det += m[2] * (m[4] * (m[9] * m[15] - m[11] * m[13]) - 
                   m[5] * (m[8] * m[15] - m[11] * m[12]) + 
                   m[7] * (m[8] * m[13] - m[9] * m[12]));
                   
    det -= m[3] * (m[4] * (m[9] * m[14] - m[10] * m[13]) - 
                   m[5] * (m[8] * m[14] - m[10] * m[12]) + 
                   m[6] * (m[8] * m[13] - m[9] * m[12]));
    
    atlas_manifold_set_error(ATLAS_MANIFOLD_SUCCESS);
    return det;
}

// =============================================================================
// Coordinate System Transformations
// =============================================================================

/**
 * Transform a 2D point using a transformation matrix.
 * 
 * @param result_x Output X coordinate
 * @param result_y Output Y coordinate
 * @param x Input X coordinate
 * @param y Input Y coordinate
 * @param matrix Transformation matrix (4x4, 16 elements)
 * @return 0 on success, -1 on error
 */
int atlas_manifold_transform_point_2d(double* result_x, double* result_y,
                                      double x, double y,
                                      const double matrix[16]) {
    if (result_x == NULL || result_y == NULL || matrix == NULL ||
        !isfinite(x) || !isfinite(y)) {
        atlas_manifold_set_error(ATLAS_MANIFOLD_ERROR_INVALID_ARGUMENT);
        return -1;
    }
    
    // Homogeneous coordinates: [x, y, 0, 1]
    double homogeneous[4] = {x, y, 0.0, 1.0};
    double transformed[4] = {0.0, 0.0, 0.0, 0.0};
    
    // Matrix-vector multiplication
    for (int i = 0; i < 4; i++) {
        for (int j = 0; j < 4; j++) {
            transformed[i] += matrix[i * 4 + j] * homogeneous[j];
        }
    }
    
    // Check for degenerate transformation (w = 0)
    if (fabs(transformed[3]) < DBL_EPSILON) {
        atlas_manifold_set_error(ATLAS_MANIFOLD_ERROR_TRANSFORM_FAILED);
        return -1;
    }
    
    // Convert back to Cartesian coordinates
    *result_x = transformed[0] / transformed[3];
    *result_y = transformed[1] / transformed[3];
    
    // Verify results are finite
    if (!isfinite(*result_x) || !isfinite(*result_y)) {
        atlas_manifold_set_error(ATLAS_MANIFOLD_ERROR_TRANSFORM_FAILED);
        return -1;
    }
    
    atlas_manifold_set_error(ATLAS_MANIFOLD_SUCCESS);
    return 0;
}

/**
 * Apply composed transformation: scale, then rotate, then translate.
 * 
 * @param result_x Output X coordinate  
 * @param result_y Output Y coordinate
 * @param x Input X coordinate
 * @param y Input Y coordinate
 * @param params Transformation parameters
 * @return 0 on success, -1 on error
 */
int atlas_manifold_transform_composed(double* result_x, double* result_y,
                                     double x, double y,
                                     const atlas_transform_params_t* params) {
    if (result_x == NULL || result_y == NULL || params == NULL ||
        !isfinite(x) || !isfinite(y)) {
        atlas_manifold_set_error(ATLAS_MANIFOLD_ERROR_INVALID_ARGUMENT);
        return -1;
    }
    
    // Create individual transformation matrices
    double scale_matrix[16], rotation_matrix[16], translation_matrix[16];
    double temp_matrix[16], final_matrix[16];
    
    if (atlas_manifold_matrix_scaling(scale_matrix, 
                                     params->scaling_factor, params->scaling_factor) != 0) {
        return -1;
    }
    
    if (atlas_manifold_matrix_rotation(rotation_matrix, params->rotation_angle) != 0) {
        return -1;
    }
    
    if (atlas_manifold_matrix_translation(translation_matrix, 
                                         params->translation_x, params->translation_y) != 0) {
        return -1;
    }
    
    // Compose transformations: T * R * S (applied right to left)
    if (atlas_manifold_matrix_multiply_4x4(temp_matrix, rotation_matrix, scale_matrix) != 0) {
        return -1;
    }
    
    if (atlas_manifold_matrix_multiply_4x4(final_matrix, translation_matrix, temp_matrix) != 0) {
        return -1;
    }
    
    // Apply the composed transformation
    return atlas_manifold_transform_point_2d(result_x, result_y, x, y, final_matrix);
}

// =============================================================================
// Distance and Metric Calculations
// =============================================================================

/**
 * Calculate Euclidean distance between two 2D points.
 * 
 * @param x1 X coordinate of first point
 * @param y1 Y coordinate of first point  
 * @param x2 X coordinate of second point
 * @param y2 Y coordinate of second point
 * @return Distance value, or negative on error
 */
double atlas_manifold_euclidean_distance_2d(double x1, double y1, double x2, double y2) {
    if (!isfinite(x1) || !isfinite(y1) || !isfinite(x2) || !isfinite(y2)) {
        atlas_manifold_set_error(ATLAS_MANIFOLD_ERROR_INVALID_ARGUMENT);
        return -1.0;
    }
    
    double dx = x2 - x1;
    double dy = y2 - y1;
    
    double distance = sqrt(dx * dx + dy * dy);
    
    if (!isfinite(distance)) {
        atlas_manifold_set_error(ATLAS_MANIFOLD_ERROR_TRANSFORM_FAILED);
        return -1.0;
    }
    
    atlas_manifold_set_error(ATLAS_MANIFOLD_SUCCESS);
    return distance;
}

/**
 * Calculate Manhattan (L1) distance between two 2D points.
 * 
 * @param x1 X coordinate of first point
 * @param y1 Y coordinate of first point
 * @param x2 X coordinate of second point  
 * @param y2 Y coordinate of second point
 * @return Distance value, or negative on error
 */
double atlas_manifold_manhattan_distance_2d(double x1, double y1, double x2, double y2) {
    if (!isfinite(x1) || !isfinite(y1) || !isfinite(x2) || !isfinite(y2)) {
        atlas_manifold_set_error(ATLAS_MANIFOLD_ERROR_INVALID_ARGUMENT);
        return -1.0;
    }
    
    double distance = fabs(x2 - x1) + fabs(y2 - y1);
    
    if (!isfinite(distance)) {
        atlas_manifold_set_error(ATLAS_MANIFOLD_ERROR_TRANSFORM_FAILED);
        return -1.0;
    }
    
    atlas_manifold_set_error(ATLAS_MANIFOLD_SUCCESS);
    return distance;
}

/**
 * Calculate the angle between two vectors from origin.
 * 
 * @param x1 X component of first vector
 * @param y1 Y component of first vector
 * @param x2 X component of second vector
 * @param y2 Y component of second vector
 * @return Angle in radians, or NaN on error
 */
double atlas_manifold_vector_angle(double x1, double y1, double x2, double y2) {
    if (!isfinite(x1) || !isfinite(y1) || !isfinite(x2) || !isfinite(y2)) {
        atlas_manifold_set_error(ATLAS_MANIFOLD_ERROR_INVALID_ARGUMENT);
        return NAN;
    }
    
    // Calculate magnitudes
    double mag1 = sqrt(x1 * x1 + y1 * y1);
    double mag2 = sqrt(x2 * x2 + y2 * y2);
    
    // Check for zero vectors
    if (mag1 < DBL_EPSILON || mag2 < DBL_EPSILON) {
        atlas_manifold_set_error(ATLAS_MANIFOLD_ERROR_INVALID_ARGUMENT);
        return NAN;
    }
    
    // Calculate dot product and normalize
    double dot_product = (x1 * x2 + y1 * y2) / (mag1 * mag2);
    
    // Clamp to [-1, 1] to handle numerical errors
    if (dot_product > 1.0) dot_product = 1.0;
    if (dot_product < -1.0) dot_product = -1.0;
    
    double angle = acos(dot_product);
    
    if (!isfinite(angle)) {
        atlas_manifold_set_error(ATLAS_MANIFOLD_ERROR_TRANSFORM_FAILED);
        return NAN;
    }
    
    atlas_manifold_set_error(ATLAS_MANIFOLD_SUCCESS);
    return angle;
}

// =============================================================================
// Geometric Validation and Utilities
// =============================================================================

/**
 * Check if a point lies within a rectangular boundary.
 * 
 * @param x X coordinate of point
 * @param y Y coordinate of point
 * @param min_x Minimum X coordinate of boundary
 * @param min_y Minimum Y coordinate of boundary
 * @param max_x Maximum X coordinate of boundary
 * @param max_y Maximum Y coordinate of boundary
 * @return true if point is within boundary, false otherwise
 */
bool atlas_manifold_point_in_bounds(double x, double y,
                                   double min_x, double min_y,
                                   double max_x, double max_y) {
    if (!isfinite(x) || !isfinite(y) || 
        !isfinite(min_x) || !isfinite(min_y) ||
        !isfinite(max_x) || !isfinite(max_y)) {
        atlas_manifold_set_error(ATLAS_MANIFOLD_ERROR_INVALID_ARGUMENT);
        return false;
    }
    
    if (min_x > max_x || min_y > max_y) {
        atlas_manifold_set_error(ATLAS_MANIFOLD_ERROR_INVALID_ARGUMENT);
        return false;
    }
    
    atlas_manifold_set_error(ATLAS_MANIFOLD_SUCCESS);
    return (x >= min_x && x <= max_x && y >= min_y && y <= max_y);
}

/**
 * Calculate the area of a rectangular region.
 * 
 * @param min_x Minimum X coordinate
 * @param min_y Minimum Y coordinate
 * @param max_x Maximum X coordinate
 * @param max_y Maximum Y coordinate
 * @return Area value, or negative on error
 */
double atlas_manifold_rectangle_area(double min_x, double min_y, 
                                    double max_x, double max_y) {
    if (!isfinite(min_x) || !isfinite(min_y) ||
        !isfinite(max_x) || !isfinite(max_y)) {
        atlas_manifold_set_error(ATLAS_MANIFOLD_ERROR_INVALID_ARGUMENT);
        return -1.0;
    }
    
    if (min_x > max_x || min_y > max_y) {
        atlas_manifold_set_error(ATLAS_MANIFOLD_ERROR_INVALID_ARGUMENT);
        return -1.0;
    }
    
    double area = (max_x - min_x) * (max_y - min_y);
    
    if (!isfinite(area) || area < 0.0) {
        atlas_manifold_set_error(ATLAS_MANIFOLD_ERROR_TRANSFORM_FAILED);
        return -1.0;
    }
    
    atlas_manifold_set_error(ATLAS_MANIFOLD_SUCCESS);
    return area;
}

/**
 * Normalize a 2D vector to unit length.
 * 
 * @param result_x Output normalized X component
 * @param result_y Output normalized Y component  
 * @param x Input X component
 * @param y Input Y component
 * @return 0 on success, -1 on error
 */
int atlas_manifold_normalize_vector_2d(double* result_x, double* result_y,
                                      double x, double y) {
    if (result_x == NULL || result_y == NULL ||
        !isfinite(x) || !isfinite(y)) {
        atlas_manifold_set_error(ATLAS_MANIFOLD_ERROR_INVALID_ARGUMENT);
        return -1;
    }
    
    double magnitude = sqrt(x * x + y * y);
    
    if (magnitude < DBL_EPSILON) {
        atlas_manifold_set_error(ATLAS_MANIFOLD_ERROR_INVALID_ARGUMENT);
        return -1;
    }
    
    *result_x = x / magnitude;
    *result_y = y / magnitude;
    
    if (!isfinite(*result_x) || !isfinite(*result_y)) {
        atlas_manifold_set_error(ATLAS_MANIFOLD_ERROR_TRANSFORM_FAILED);
        return -1;
    }
    
    atlas_manifold_set_error(ATLAS_MANIFOLD_SUCCESS);
    return 0;
}

// =============================================================================
// Advanced Geometric Operations  
// =============================================================================

/**
 * Calculate the centroid of a set of 2D points.
 * 
 * @param centroid_x Output X coordinate of centroid
 * @param centroid_y Output Y coordinate of centroid
 * @param points Array of points (x1, y1, x2, y2, ...)
 * @param num_points Number of points
 * @return 0 on success, -1 on error
 */
int atlas_manifold_calculate_centroid_2d(double* centroid_x, double* centroid_y,
                                        const double* points, size_t num_points) {
    if (centroid_x == NULL || centroid_y == NULL || points == NULL || num_points == 0) {
        atlas_manifold_set_error(ATLAS_MANIFOLD_ERROR_INVALID_ARGUMENT);
        return -1;
    }
    
    double sum_x = 0.0, sum_y = 0.0;
    
    for (size_t i = 0; i < num_points; i++) {
        double x = points[i * 2];
        double y = points[i * 2 + 1];
        
        if (!isfinite(x) || !isfinite(y)) {
            atlas_manifold_set_error(ATLAS_MANIFOLD_ERROR_INVALID_ARGUMENT);
            return -1;
        }
        
        sum_x += x;
        sum_y += y;
    }
    
    *centroid_x = sum_x / (double)num_points;
    *centroid_y = sum_y / (double)num_points;
    
    if (!isfinite(*centroid_x) || !isfinite(*centroid_y)) {
        atlas_manifold_set_error(ATLAS_MANIFOLD_ERROR_TRANSFORM_FAILED);
        return -1;
    }
    
    atlas_manifold_set_error(ATLAS_MANIFOLD_SUCCESS);
    return 0;
}