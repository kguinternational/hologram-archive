/* traditional_geometry.c - Traditional Euclidean Geometry for Baseline Comparison
 * (c) 2024-2025 UOR Foundation - MIT License
 * 
 * This file implements standard Euclidean geometry algorithms without Atlas-specific
 * optimizations to provide baseline performance comparisons against Atlas Universal
 * Number approaches using R96 harmonic relationships.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <time.h>
#include <assert.h>
#include <stdbool.h>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

// =============================================================================
// Traditional Geometric Data Structures
// =============================================================================

typedef struct {
    double x, y, z;
} traditional_point3d_t;

typedef struct {
    double x, y;
} traditional_point2d_t;

typedef struct {
    traditional_point3d_t origin;
    traditional_point3d_t direction;
} traditional_line3d_t;

typedef struct {
    traditional_point3d_t center;
    double radius;
} traditional_sphere_t;

typedef struct {
    traditional_point3d_t vertices[3];
} traditional_triangle_t;

typedef struct {
    double elements[16]; // 4x4 matrix in row-major order
} traditional_transform_matrix_t;

// Riemann tensor structure for curvature calculations
typedef struct {
    double components[256]; // 4x4x4x4 tensor
    int dimension;
} traditional_riemann_tensor_t;

// =============================================================================
// Basic Distance and Vector Operations
// =============================================================================

/**
 * Calculate Euclidean distance between two 2D points.
 * This is the standard sqrt(dx² + dy²) calculation.
 */
double traditional_distance_2d(const traditional_point2d_t* p1, const traditional_point2d_t* p2) {
    assert(p1 && p2);
    
    double dx = p2->x - p1->x;
    double dy = p2->y - p1->y;
    
    return sqrt(dx * dx + dy * dy);
}

/**
 * Calculate Euclidean distance between two 3D points.
 * This is the standard sqrt(dx² + dy² + dz²) calculation.
 */
double traditional_distance_3d(const traditional_point3d_t* p1, const traditional_point3d_t* p2) {
    assert(p1 && p2);
    
    double dx = p2->x - p1->x;
    double dy = p2->y - p1->y;
    double dz = p2->z - p1->z;
    
    return sqrt(dx * dx + dy * dy + dz * dz);
}

/**
 * Calculate squared distance (avoids sqrt for performance when comparing).
 */
double traditional_distance_squared_3d(const traditional_point3d_t* p1, const traditional_point3d_t* p2) {
    assert(p1 && p2);
    
    double dx = p2->x - p1->x;
    double dy = p2->y - p1->y;
    double dz = p2->z - p1->z;
    
    return dx * dx + dy * dy + dz * dz;
}

/**
 * Calculate vector dot product.
 */
double traditional_dot_product(const traditional_point3d_t* v1, const traditional_point3d_t* v2) {
    assert(v1 && v2);
    return v1->x * v2->x + v1->y * v2->y + v1->z * v2->z;
}

/**
 * Calculate vector cross product.
 */
traditional_point3d_t traditional_cross_product(const traditional_point3d_t* v1, const traditional_point3d_t* v2) {
    assert(v1 && v2);
    
    traditional_point3d_t result;
    result.x = v1->y * v2->z - v1->z * v2->y;
    result.y = v1->z * v2->x - v1->x * v2->z;
    result.z = v1->x * v2->y - v1->y * v2->x;
    
    return result;
}

/**
 * Calculate vector magnitude.
 */
double traditional_vector_magnitude(const traditional_point3d_t* v) {
    assert(v);
    return sqrt(v->x * v->x + v->y * v->y + v->z * v->z);
}

/**
 * Normalize vector to unit length.
 */
traditional_point3d_t traditional_vector_normalize(const traditional_point3d_t* v) {
    assert(v);
    
    double mag = traditional_vector_magnitude(v);
    traditional_point3d_t result;
    
    if (mag < 1e-14) {
        result.x = result.y = result.z = 0.0;
    } else {
        result.x = v->x / mag;
        result.y = v->y / mag;
        result.z = v->z / mag;
    }
    
    return result;
}

// =============================================================================
// Traditional Transformation Matrices
// =============================================================================

/**
 * Create identity transformation matrix.
 */
traditional_transform_matrix_t traditional_matrix_identity(void) {
    traditional_transform_matrix_t matrix;
    memset(matrix.elements, 0, sizeof(matrix.elements));
    
    // Set diagonal elements to 1
    matrix.elements[0] = 1.0;   // [0,0]
    matrix.elements[5] = 1.0;   // [1,1]
    matrix.elements[10] = 1.0;  // [2,2]
    matrix.elements[15] = 1.0;  // [3,3]
    
    return matrix;
}

/**
 * Create translation matrix.
 */
traditional_transform_matrix_t traditional_matrix_translation(double tx, double ty, double tz) {
    traditional_transform_matrix_t matrix = traditional_matrix_identity();
    
    matrix.elements[3] = tx;   // [0,3]
    matrix.elements[7] = ty;   // [1,3]
    matrix.elements[11] = tz;  // [2,3]
    
    return matrix;
}

/**
 * Create rotation matrix around X-axis.
 */
traditional_transform_matrix_t traditional_matrix_rotation_x(double angle) {
    traditional_transform_matrix_t matrix = traditional_matrix_identity();
    
    double c = cos(angle);
    double s = sin(angle);
    
    matrix.elements[5] = c;   // [1,1]
    matrix.elements[6] = -s;  // [1,2]
    matrix.elements[9] = s;   // [2,1]
    matrix.elements[10] = c;  // [2,2]
    
    return matrix;
}

/**
 * Create rotation matrix around Y-axis.
 */
traditional_transform_matrix_t traditional_matrix_rotation_y(double angle) {
    traditional_transform_matrix_t matrix = traditional_matrix_identity();
    
    double c = cos(angle);
    double s = sin(angle);
    
    matrix.elements[0] = c;   // [0,0]
    matrix.elements[2] = s;   // [0,2]
    matrix.elements[8] = -s;  // [2,0]
    matrix.elements[10] = c;  // [2,2]
    
    return matrix;
}

/**
 * Create rotation matrix around Z-axis.
 */
traditional_transform_matrix_t traditional_matrix_rotation_z(double angle) {
    traditional_transform_matrix_t matrix = traditional_matrix_identity();
    
    double c = cos(angle);
    double s = sin(angle);
    
    matrix.elements[0] = c;   // [0,0]
    matrix.elements[1] = -s;  // [0,1]
    matrix.elements[4] = s;   // [1,0]
    matrix.elements[5] = c;   // [1,1]
    
    return matrix;
}

/**
 * Create scaling matrix.
 */
traditional_transform_matrix_t traditional_matrix_scaling(double sx, double sy, double sz) {
    traditional_transform_matrix_t matrix = traditional_matrix_identity();
    
    matrix.elements[0] = sx;   // [0,0]
    matrix.elements[5] = sy;   // [1,1]
    matrix.elements[10] = sz;  // [2,2]
    
    return matrix;
}

/**
 * Multiply two 4x4 transformation matrices.
 */
traditional_transform_matrix_t traditional_matrix_multiply(const traditional_transform_matrix_t* a, 
                                                          const traditional_transform_matrix_t* b) {
    assert(a && b);
    
    traditional_transform_matrix_t result;
    
    for (int i = 0; i < 4; i++) {
        for (int j = 0; j < 4; j++) {
            double sum = 0.0;
            for (int k = 0; k < 4; k++) {
                sum += a->elements[i * 4 + k] * b->elements[k * 4 + j];
            }
            result.elements[i * 4 + j] = sum;
        }
    }
    
    return result;
}

/**
 * Transform a 3D point using a 4x4 matrix.
 */
traditional_point3d_t traditional_transform_point(const traditional_transform_matrix_t* matrix, 
                                                 const traditional_point3d_t* point) {
    assert(matrix && point);
    
    // Homogeneous coordinates: [x, y, z, 1]
    double x = matrix->elements[0] * point->x + matrix->elements[1] * point->y + 
               matrix->elements[2] * point->z + matrix->elements[3];
    double y = matrix->elements[4] * point->x + matrix->elements[5] * point->y + 
               matrix->elements[6] * point->z + matrix->elements[7];
    double z = matrix->elements[8] * point->x + matrix->elements[9] * point->y + 
               matrix->elements[10] * point->z + matrix->elements[11];
    double w = matrix->elements[12] * point->x + matrix->elements[13] * point->y + 
               matrix->elements[14] * point->z + matrix->elements[15];
    
    traditional_point3d_t result;
    if (fabs(w) > 1e-14) {
        result.x = x / w;
        result.y = y / w;
        result.z = z / w;
    } else {
        result.x = x;
        result.y = y;
        result.z = z;
    }
    
    return result;
}

// =============================================================================
// Traditional Riemann Curvature Tensor Computation
// =============================================================================

/**
 * Initialize Riemann tensor structure.
 */
traditional_riemann_tensor_t* traditional_riemann_create(int dimension) {
    assert(dimension > 0 && dimension <= 4);
    
    traditional_riemann_tensor_t* tensor = malloc(sizeof(traditional_riemann_tensor_t));
    if (!tensor) return NULL;
    
    tensor->dimension = dimension;
    memset(tensor->components, 0, sizeof(tensor->components));
    
    return tensor;
}

/**
 * Destroy Riemann tensor.
 */
void traditional_riemann_destroy(traditional_riemann_tensor_t* tensor) {
    if (tensor) {
        free(tensor);
    }
}

/**
 * Get Riemann tensor component R^μ_νρσ.
 */
double traditional_riemann_get_component(const traditional_riemann_tensor_t* tensor,
                                        int mu, int nu, int rho, int sigma) {
    assert(tensor && mu >= 0 && mu < tensor->dimension && 
           nu >= 0 && nu < tensor->dimension &&
           rho >= 0 && rho < tensor->dimension && 
           sigma >= 0 && sigma < tensor->dimension);
    
    int index = mu * tensor->dimension * tensor->dimension * tensor->dimension +
                nu * tensor->dimension * tensor->dimension +
                rho * tensor->dimension + sigma;
    
    return tensor->components[index];
}

/**
 * Set Riemann tensor component R^μ_νρσ.
 */
void traditional_riemann_set_component(traditional_riemann_tensor_t* tensor,
                                      int mu, int nu, int rho, int sigma,
                                      double value) {
    assert(tensor && mu >= 0 && mu < tensor->dimension && 
           nu >= 0 && nu < tensor->dimension &&
           rho >= 0 && rho < tensor->dimension && 
           sigma >= 0 && sigma < tensor->dimension);
    
    int index = mu * tensor->dimension * tensor->dimension * tensor->dimension +
                nu * tensor->dimension * tensor->dimension +
                rho * tensor->dimension + sigma;
    
    tensor->components[index] = value;
}

/**
 * Compute Riemann curvature tensor from metric derivatives.
 * This is a simplified implementation for demonstration - full implementation
 * would require proper metric tensor and Christoffel symbols.
 */
int traditional_compute_riemann_curvature(traditional_riemann_tensor_t* tensor,
                                         double metric[4][4],
                                         double metric_derivatives[4][4][4]) {
    assert(tensor && metric && metric_derivatives);
    
    int dim = tensor->dimension;
    
    // This is a simplified computation - real Riemann tensor calculation
    // involves computing Christoffel symbols and their derivatives
    for (int mu = 0; mu < dim; mu++) {
        for (int nu = 0; nu < dim; nu++) {
            for (int rho = 0; rho < dim; rho++) {
                for (int sigma = 0; sigma < dim; sigma++) {
                    // Simplified computation using metric derivatives
                    double curvature = 0.0;
                    
                    for (int lambda = 0; lambda < dim; lambda++) {
                        curvature += 0.5 * metric[mu][lambda] *
                                   (metric_derivatives[nu][lambda][sigma] + 
                                    metric_derivatives[lambda][sigma][nu] -
                                    metric_derivatives[nu][sigma][lambda]);
                    }
                    
                    traditional_riemann_set_component(tensor, mu, nu, rho, sigma, curvature);
                }
            }
        }
    }
    
    return 0;
}

/**
 * Calculate scalar curvature from Riemann tensor.
 */
double traditional_compute_scalar_curvature(const traditional_riemann_tensor_t* tensor,
                                           double metric_inverse[4][4]) {
    assert(tensor && metric_inverse);
    
    double scalar_curvature = 0.0;
    int dim = tensor->dimension;
    
    // R = g^μν R_μν where R_μν is the Ricci tensor
    for (int mu = 0; mu < dim; mu++) {
        for (int nu = 0; nu < dim; nu++) {
            double ricci_component = 0.0;
            
            // R_μν = R^λ_μλν (contraction of Riemann tensor)
            for (int lambda = 0; lambda < dim; lambda++) {
                ricci_component += traditional_riemann_get_component(tensor, lambda, mu, lambda, nu);
            }
            
            scalar_curvature += metric_inverse[mu][nu] * ricci_component;
        }
    }
    
    return scalar_curvature;
}

// =============================================================================
// Geometric Primitive Operations
// =============================================================================

/**
 * Calculate distance from point to line in 3D.
 */
double traditional_point_to_line_distance(const traditional_point3d_t* point,
                                         const traditional_line3d_t* line) {
    assert(point && line);
    
    // Vector from line origin to point
    traditional_point3d_t to_point;
    to_point.x = point->x - line->origin.x;
    to_point.y = point->y - line->origin.y;
    to_point.z = point->z - line->origin.z;
    
    // Cross product of vectors
    traditional_point3d_t cross = traditional_cross_product(&to_point, &line->direction);
    double cross_magnitude = traditional_vector_magnitude(&cross);
    double direction_magnitude = traditional_vector_magnitude(&line->direction);
    
    if (direction_magnitude < 1e-14) {
        return traditional_distance_3d(point, &line->origin);
    }
    
    return cross_magnitude / direction_magnitude;
}

/**
 * Calculate distance from point to sphere surface.
 */
double traditional_point_to_sphere_distance(const traditional_point3d_t* point,
                                           const traditional_sphere_t* sphere) {
    assert(point && sphere);
    
    double center_distance = traditional_distance_3d(point, &sphere->center);
    return fabs(center_distance - sphere->radius);
}

/**
 * Check if point is inside sphere.
 */
bool traditional_point_in_sphere(const traditional_point3d_t* point,
                                const traditional_sphere_t* sphere) {
    assert(point && sphere);
    
    double distance_squared = traditional_distance_squared_3d(point, &sphere->center);
    return distance_squared <= (sphere->radius * sphere->radius);
}

/**
 * Calculate area of triangle using Heron's formula.
 */
double traditional_triangle_area(const traditional_triangle_t* triangle) {
    assert(triangle);
    
    // Calculate side lengths
    double a = traditional_distance_3d(&triangle->vertices[0], &triangle->vertices[1]);
    double b = traditional_distance_3d(&triangle->vertices[1], &triangle->vertices[2]);
    double c = traditional_distance_3d(&triangle->vertices[2], &triangle->vertices[0]);
    
    // Semi-perimeter
    double s = (a + b + c) / 2.0;
    
    // Heron's formula
    double area_squared = s * (s - a) * (s - b) * (s - c);
    
    if (area_squared < 0) return 0.0;
    return sqrt(area_squared);
}

/**
 * Calculate triangle normal vector.
 */
traditional_point3d_t traditional_triangle_normal(const traditional_triangle_t* triangle) {
    assert(triangle);
    
    // Two edge vectors
    traditional_point3d_t edge1, edge2;
    edge1.x = triangle->vertices[1].x - triangle->vertices[0].x;
    edge1.y = triangle->vertices[1].y - triangle->vertices[0].y;
    edge1.z = triangle->vertices[1].z - triangle->vertices[0].z;
    
    edge2.x = triangle->vertices[2].x - triangle->vertices[0].x;
    edge2.y = triangle->vertices[2].y - triangle->vertices[0].y;
    edge2.z = triangle->vertices[2].z - triangle->vertices[0].z;
    
    // Cross product gives normal
    traditional_point3d_t normal = traditional_cross_product(&edge1, &edge2);
    return traditional_vector_normalize(&normal);
}

// =============================================================================
// Intersection and Collision Detection
// =============================================================================

/**
 * Ray-sphere intersection test.
 */
bool traditional_ray_sphere_intersection(const traditional_line3d_t* ray,
                                        const traditional_sphere_t* sphere,
                                        double* t_near, double* t_far) {
    assert(ray && sphere);
    
    // Vector from ray origin to sphere center
    traditional_point3d_t to_center;
    to_center.x = sphere->center.x - ray->origin.x;
    to_center.y = sphere->center.y - ray->origin.y;
    to_center.z = sphere->center.z - ray->origin.z;
    
    // Quadratic equation coefficients
    double a = traditional_dot_product(&ray->direction, &ray->direction);
    double b = -2.0 * traditional_dot_product(&to_center, &ray->direction);
    double c = traditional_dot_product(&to_center, &to_center) - 
               sphere->radius * sphere->radius;
    
    // Discriminant
    double discriminant = b * b - 4 * a * c;
    
    if (discriminant < 0) {
        return false; // No intersection
    }
    
    double sqrt_discriminant = sqrt(discriminant);
    double t1 = (-b - sqrt_discriminant) / (2 * a);
    double t2 = (-b + sqrt_discriminant) / (2 * a);
    
    if (t_near) *t_near = fmin(t1, t2);
    if (t_far) *t_far = fmax(t1, t2);
    
    return true;
}

/**
 * Ray-triangle intersection using Möller-Trumbore algorithm.
 */
bool traditional_ray_triangle_intersection(const traditional_line3d_t* ray,
                                          const traditional_triangle_t* triangle,
                                          double* t, double* u, double* v) {
    assert(ray && triangle);
    
    const double EPSILON = 1e-8;
    
    // Edge vectors
    traditional_point3d_t edge1, edge2;
    edge1.x = triangle->vertices[1].x - triangle->vertices[0].x;
    edge1.y = triangle->vertices[1].y - triangle->vertices[0].y;
    edge1.z = triangle->vertices[1].z - triangle->vertices[0].z;
    
    edge2.x = triangle->vertices[2].x - triangle->vertices[0].x;
    edge2.y = triangle->vertices[2].y - triangle->vertices[0].y;
    edge2.z = triangle->vertices[2].z - triangle->vertices[0].z;
    
    // Calculate determinant
    traditional_point3d_t h = traditional_cross_product(&ray->direction, &edge2);
    double det = traditional_dot_product(&edge1, &h);
    
    if (det > -EPSILON && det < EPSILON) {
        return false; // Ray is parallel to triangle
    }
    
    double inv_det = 1.0 / det;
    
    // Calculate distance from vertex 0 to ray origin
    traditional_point3d_t s;
    s.x = ray->origin.x - triangle->vertices[0].x;
    s.y = ray->origin.y - triangle->vertices[0].y;
    s.z = ray->origin.z - triangle->vertices[0].z;
    
    // Calculate u parameter
    double u_param = inv_det * traditional_dot_product(&s, &h);
    if (u_param < 0.0 || u_param > 1.0) {
        return false;
    }
    
    // Calculate v parameter
    traditional_point3d_t q = traditional_cross_product(&s, &edge1);
    double v_param = inv_det * traditional_dot_product(&ray->direction, &q);
    if (v_param < 0.0 || u_param + v_param > 1.0) {
        return false;
    }
    
    // Calculate t parameter (distance along ray)
    double t_param = inv_det * traditional_dot_product(&edge2, &q);
    
    if (t_param > EPSILON) {
        if (t) *t = t_param;
        if (u) *u = u_param;
        if (v) *v = v_param;
        return true;
    }
    
    return false;
}

// =============================================================================
// Benchmark Functions
// =============================================================================

/**
 * Benchmark distance calculations.
 */
double traditional_benchmark_distances(int num_points, int iterations) {
    // Generate random points
    traditional_point3d_t* points = malloc(num_points * sizeof(traditional_point3d_t));
    if (!points) return -1.0;
    
    for (int i = 0; i < num_points; i++) {
        points[i].x = ((double)rand() / RAND_MAX) * 100.0 - 50.0;
        points[i].y = ((double)rand() / RAND_MAX) * 100.0 - 50.0;
        points[i].z = ((double)rand() / RAND_MAX) * 100.0 - 50.0;
    }
    
    clock_t start = clock();
    
    double total_distance = 0.0;
    for (int iter = 0; iter < iterations; iter++) {
        for (int i = 0; i < num_points - 1; i++) {
            total_distance += traditional_distance_3d(&points[i], &points[i + 1]);
        }
    }
    
    clock_t end = clock();
    
    free(points);
    
    // Prevent compiler optimization
    (void)total_distance;
    
    return ((double)(end - start)) / CLOCKS_PER_SEC / iterations;
}

/**
 * Benchmark matrix transformations.
 */
double traditional_benchmark_transforms(int num_points, int iterations) {
    // Generate random points and transformation matrices
    traditional_point3d_t* points = malloc(num_points * sizeof(traditional_point3d_t));
    if (!points) return -1.0;
    
    for (int i = 0; i < num_points; i++) {
        points[i].x = ((double)rand() / RAND_MAX) * 10.0 - 5.0;
        points[i].y = ((double)rand() / RAND_MAX) * 10.0 - 5.0;
        points[i].z = ((double)rand() / RAND_MAX) * 10.0 - 5.0;
    }
    
    traditional_transform_matrix_t rotation = traditional_matrix_rotation_y(M_PI / 4);
    traditional_transform_matrix_t translation = traditional_matrix_translation(1.0, 2.0, 3.0);
    traditional_transform_matrix_t transform = traditional_matrix_multiply(&rotation, &translation);
    
    clock_t start = clock();
    
    for (int iter = 0; iter < iterations; iter++) {
        for (int i = 0; i < num_points; i++) {
            traditional_point3d_t transformed = traditional_transform_point(&transform, &points[i]);
            points[i] = transformed; // Update in place to prevent optimization
        }
    }
    
    clock_t end = clock();
    
    free(points);
    
    return ((double)(end - start)) / CLOCKS_PER_SEC / iterations;
}

/**
 * Benchmark curvature calculations.
 */
double traditional_benchmark_curvature(int grid_size, int iterations) {
    // Create simplified metric for flat space
    double metric[4][4];
    double metric_derivatives[4][4][4];
    double metric_inverse[4][4];
    
    // Initialize to flat Minkowski metric
    memset(metric, 0, sizeof(metric));
    memset(metric_derivatives, 0, sizeof(metric_derivatives));
    memset(metric_inverse, 0, sizeof(metric_inverse));
    
    metric[0][0] = -1.0; metric_inverse[0][0] = -1.0;
    metric[1][1] = 1.0;  metric_inverse[1][1] = 1.0;
    metric[2][2] = 1.0;  metric_inverse[2][2] = 1.0;
    metric[3][3] = 1.0;  metric_inverse[3][3] = 1.0;
    
    traditional_riemann_tensor_t* tensor = traditional_riemann_create(4);
    if (!tensor) return -1.0;
    
    clock_t start = clock();
    
    for (int iter = 0; iter < iterations; iter++) {
        traditional_compute_riemann_curvature(tensor, metric, metric_derivatives);
        double curvature = traditional_compute_scalar_curvature(tensor, metric_inverse);
        (void)curvature; // Prevent optimization
    }
    
    clock_t end = clock();
    
    traditional_riemann_destroy(tensor);
    
    return ((double)(end - start)) / CLOCKS_PER_SEC / iterations;
}

// =============================================================================
// Main Function for Testing
// =============================================================================

#ifdef TRADITIONAL_GEOMETRY_MAIN
int main(void) {
    printf("Traditional Geometry Operations Benchmark\n");
    printf("=======================================\n\n");
    
    srand((unsigned int)time(NULL));
    
    // Test basic operations
    printf("Testing basic geometric operations...\n");
    
    traditional_point3d_t p1 = {1.0, 2.0, 3.0};
    traditional_point3d_t p2 = {4.0, 6.0, 8.0};
    
    double distance = traditional_distance_3d(&p1, &p2);
    printf("3D distance test: %.6f (expected ~7.07)\n", distance);
    
    traditional_point3d_t cross = traditional_cross_product(&p1, &p2);
    printf("Cross product test: (%.3f, %.3f, %.3f)\n", cross.x, cross.y, cross.z);
    
    traditional_transform_matrix_t rotation = traditional_matrix_rotation_z(M_PI / 2);
    traditional_point3d_t rotated = traditional_transform_point(&rotation, &p1);
    printf("Rotation test: (%.3f, %.3f, %.3f) -> (%.3f, %.3f, %.3f)\n", 
           p1.x, p1.y, p1.z, rotated.x, rotated.y, rotated.z);
    
    // Intersection tests
    traditional_sphere_t sphere = {{0.0, 0.0, 0.0}, 5.0};
    traditional_line3d_t ray = {{-10.0, 0.0, 0.0}, {1.0, 0.0, 0.0}};
    
    double t_near, t_far;
    bool intersection = traditional_ray_sphere_intersection(&ray, &sphere, &t_near, &t_far);
    printf("Ray-sphere intersection test: %s (t_near=%.3f, t_far=%.3f)\n", 
           intersection ? "HIT" : "MISS", t_near, t_far);
    
    printf("\nPerformance Benchmarks:\n");
    
    // Distance benchmark
    double dist_time = traditional_benchmark_distances(10000, 100);
    if (dist_time > 0) {
        printf("Distance calculations: %.6f seconds per iteration\n", dist_time);
    }
    
    // Transform benchmark
    double transform_time = traditional_benchmark_transforms(10000, 100);
    if (transform_time > 0) {
        printf("Transform operations: %.6f seconds per iteration\n", transform_time);
    }
    
    // Curvature benchmark
    double curvature_time = traditional_benchmark_curvature(50, 10);
    if (curvature_time > 0) {
        printf("Curvature calculations: %.6f seconds per iteration\n", curvature_time);
    }
    
    return 0;
}
#endif