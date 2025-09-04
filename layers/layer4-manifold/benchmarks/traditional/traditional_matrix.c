/* traditional_matrix.c - Traditional O(n³) Matrix Operations for Baseline Comparison
 * (c) 2024-2025 UOR Foundation - MIT License
 * 
 * This file implements standard matrix algorithms without Atlas-specific optimizations
 * to provide baseline performance comparisons against Atlas Universal Number approaches.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <time.h>
#include <assert.h>

// =============================================================================
// Traditional Matrix Data Structures
// =============================================================================

typedef struct {
    double* data;
    int rows;
    int cols;
} traditional_matrix_t;

// =============================================================================
// Basic Matrix Operations
// =============================================================================

/**
 * Create a new matrix with specified dimensions.
 */
traditional_matrix_t* traditional_matrix_create(int rows, int cols) {
    if (rows <= 0 || cols <= 0) return NULL;
    
    traditional_matrix_t* matrix = malloc(sizeof(traditional_matrix_t));
    if (!matrix) return NULL;
    
    matrix->data = calloc(rows * cols, sizeof(double));
    if (!matrix->data) {
        free(matrix);
        return NULL;
    }
    
    matrix->rows = rows;
    matrix->cols = cols;
    return matrix;
}

/**
 * Destroy matrix and free memory.
 */
void traditional_matrix_destroy(traditional_matrix_t* matrix) {
    if (matrix) {
        free(matrix->data);
        free(matrix);
    }
}

/**
 * Set matrix element at (row, col).
 */
void traditional_matrix_set(traditional_matrix_t* matrix, int row, int col, double value) {
    assert(matrix && row >= 0 && row < matrix->rows && col >= 0 && col < matrix->cols);
    matrix->data[row * matrix->cols + col] = value;
}

/**
 * Get matrix element at (row, col).
 */
double traditional_matrix_get(const traditional_matrix_t* matrix, int row, int col) {
    assert(matrix && row >= 0 && row < matrix->rows && col >= 0 && col < matrix->cols);
    return matrix->data[row * matrix->cols + col];
}

/**
 * Fill matrix with random values for testing.
 */
void traditional_matrix_fill_random(traditional_matrix_t* matrix, double min_val, double max_val) {
    assert(matrix);
    double range = max_val - min_val;
    
    for (int i = 0; i < matrix->rows * matrix->cols; i++) {
        matrix->data[i] = min_val + range * ((double)rand() / RAND_MAX);
    }
}

/**
 * Create identity matrix.
 */
void traditional_matrix_identity(traditional_matrix_t* matrix) {
    assert(matrix && matrix->rows == matrix->cols);
    
    memset(matrix->data, 0, matrix->rows * matrix->cols * sizeof(double));
    for (int i = 0; i < matrix->rows; i++) {
        traditional_matrix_set(matrix, i, i, 1.0);
    }
}

// =============================================================================
// Standard O(n³) Matrix Multiplication
// =============================================================================

/**
 * Traditional O(n³) matrix multiplication: C = A * B
 * No optimizations - pure textbook implementation for baseline comparison.
 */
traditional_matrix_t* traditional_matrix_multiply(const traditional_matrix_t* a, 
                                                 const traditional_matrix_t* b) {
    assert(a && b && a->cols == b->rows);
    
    traditional_matrix_t* result = traditional_matrix_create(a->rows, b->cols);
    if (!result) return NULL;
    
    // Standard triple-nested loop - O(n³) complexity
    for (int i = 0; i < a->rows; i++) {
        for (int j = 0; j < b->cols; j++) {
            double sum = 0.0;
            for (int k = 0; k < a->cols; k++) {
                sum += traditional_matrix_get(a, i, k) * traditional_matrix_get(b, k, j);
            }
            traditional_matrix_set(result, i, j, sum);
        }
    }
    
    return result;
}

/**
 * In-place matrix transpose.
 */
int traditional_matrix_transpose_inplace(traditional_matrix_t* matrix) {
    assert(matrix && matrix->rows == matrix->cols);
    
    for (int i = 0; i < matrix->rows; i++) {
        for (int j = i + 1; j < matrix->cols; j++) {
            double temp = traditional_matrix_get(matrix, i, j);
            traditional_matrix_set(matrix, i, j, traditional_matrix_get(matrix, j, i));
            traditional_matrix_set(matrix, j, i, temp);
        }
    }
    
    return 0;
}

// =============================================================================
// LU Decomposition for Determinant Calculation
// =============================================================================

/**
 * LU decomposition using Gaussian elimination with partial pivoting.
 * Returns 0 on success, -1 if matrix is singular.
 */
int traditional_matrix_lu_decompose(traditional_matrix_t* matrix, int* pivot) {
    assert(matrix && matrix->rows == matrix->cols && pivot);
    
    int n = matrix->rows;
    int sign_changes = 0;
    
    // Initialize pivot array
    for (int i = 0; i < n; i++) {
        pivot[i] = i;
    }
    
    for (int k = 0; k < n - 1; k++) {
        // Find pivot element
        double max_val = 0.0;
        int pivot_row = k;
        
        for (int i = k; i < n; i++) {
            double abs_val = fabs(traditional_matrix_get(matrix, i, k));
            if (abs_val > max_val) {
                max_val = abs_val;
                pivot_row = i;
            }
        }
        
        // Check for singular matrix
        if (max_val < 1e-14) {
            return -1; // Matrix is singular
        }
        
        // Swap rows if necessary
        if (pivot_row != k) {
            for (int j = 0; j < n; j++) {
                double temp = traditional_matrix_get(matrix, k, j);
                traditional_matrix_set(matrix, k, j, traditional_matrix_get(matrix, pivot_row, j));
                traditional_matrix_set(matrix, pivot_row, j, temp);
            }
            // Update pivot array
            int temp = pivot[k];
            pivot[k] = pivot[pivot_row];
            pivot[pivot_row] = temp;
            sign_changes++;
        }
        
        // Eliminate column
        for (int i = k + 1; i < n; i++) {
            double factor = traditional_matrix_get(matrix, i, k) / traditional_matrix_get(matrix, k, k);
            traditional_matrix_set(matrix, i, k, factor);
            
            for (int j = k + 1; j < n; j++) {
                double val = traditional_matrix_get(matrix, i, j) - factor * traditional_matrix_get(matrix, k, j);
                traditional_matrix_set(matrix, i, j, val);
            }
        }
    }
    
    return sign_changes;
}

/**
 * Calculate determinant using LU decomposition.
 * This is the traditional O(n³) approach for determinant calculation.
 */
double traditional_matrix_determinant(const traditional_matrix_t* matrix) {
    assert(matrix && matrix->rows == matrix->cols);
    
    int n = matrix->rows;
    
    // Create copy for LU decomposition
    traditional_matrix_t* lu = traditional_matrix_create(n, n);
    if (!lu) return NAN;
    
    memcpy(lu->data, matrix->data, n * n * sizeof(double));
    
    int* pivot = malloc(n * sizeof(int));
    if (!pivot) {
        traditional_matrix_destroy(lu);
        return NAN;
    }
    
    int sign_changes = traditional_matrix_lu_decompose(lu, pivot);
    
    if (sign_changes < 0) {
        // Singular matrix
        free(pivot);
        traditional_matrix_destroy(lu);
        return 0.0;
    }
    
    // Calculate determinant as product of diagonal elements
    double det = (sign_changes % 2 == 0) ? 1.0 : -1.0;
    
    for (int i = 0; i < n; i++) {
        det *= traditional_matrix_get(lu, i, i);
    }
    
    free(pivot);
    traditional_matrix_destroy(lu);
    return det;
}

// =============================================================================
// Matrix Inverse using Gauss-Jordan Elimination
// =============================================================================

/**
 * Calculate matrix inverse using traditional Gauss-Jordan elimination.
 * Returns new matrix containing inverse, or NULL if matrix is singular.
 */
traditional_matrix_t* traditional_matrix_inverse(const traditional_matrix_t* matrix) {
    assert(matrix && matrix->rows == matrix->cols);
    
    int n = matrix->rows;
    
    // Create augmented matrix [A|I]
    traditional_matrix_t* augmented = traditional_matrix_create(n, 2 * n);
    if (!augmented) return NULL;
    
    // Copy original matrix to left half
    for (int i = 0; i < n; i++) {
        for (int j = 0; j < n; j++) {
            traditional_matrix_set(augmented, i, j, traditional_matrix_get(matrix, i, j));
        }
        // Set identity matrix in right half
        traditional_matrix_set(augmented, i, n + i, 1.0);
    }
    
    // Gauss-Jordan elimination
    for (int k = 0; k < n; k++) {
        // Find pivot
        double max_val = 0.0;
        int pivot_row = k;
        
        for (int i = k; i < n; i++) {
            double abs_val = fabs(traditional_matrix_get(augmented, i, k));
            if (abs_val > max_val) {
                max_val = abs_val;
                pivot_row = i;
            }
        }
        
        // Check for singular matrix
        if (max_val < 1e-14) {
            traditional_matrix_destroy(augmented);
            return NULL;
        }
        
        // Swap rows if necessary
        if (pivot_row != k) {
            for (int j = 0; j < 2 * n; j++) {
                double temp = traditional_matrix_get(augmented, k, j);
                traditional_matrix_set(augmented, k, j, traditional_matrix_get(augmented, pivot_row, j));
                traditional_matrix_set(augmented, pivot_row, j, temp);
            }
        }
        
        // Scale pivot row
        double pivot = traditional_matrix_get(augmented, k, k);
        for (int j = 0; j < 2 * n; j++) {
            traditional_matrix_set(augmented, k, j, traditional_matrix_get(augmented, k, j) / pivot);
        }
        
        // Eliminate column
        for (int i = 0; i < n; i++) {
            if (i != k) {
                double factor = traditional_matrix_get(augmented, i, k);
                for (int j = 0; j < 2 * n; j++) {
                    double val = traditional_matrix_get(augmented, i, j) - 
                                factor * traditional_matrix_get(augmented, k, j);
                    traditional_matrix_set(augmented, i, j, val);
                }
            }
        }
    }
    
    // Extract inverse from right half
    traditional_matrix_t* inverse = traditional_matrix_create(n, n);
    if (!inverse) {
        traditional_matrix_destroy(augmented);
        return NULL;
    }
    
    for (int i = 0; i < n; i++) {
        for (int j = 0; j < n; j++) {
            traditional_matrix_set(inverse, i, j, traditional_matrix_get(augmented, i, n + j));
        }
    }
    
    traditional_matrix_destroy(augmented);
    return inverse;
}

// =============================================================================
// Eigenvalue Computation using Power Method
// =============================================================================

/**
 * Find dominant eigenvalue and eigenvector using power method.
 * This is a traditional iterative approach for eigenvalue computation.
 */
int traditional_matrix_dominant_eigenvalue(const traditional_matrix_t* matrix, 
                                          double* eigenvalue, 
                                          traditional_matrix_t* eigenvector,
                                          int max_iterations,
                                          double tolerance) {
    assert(matrix && matrix->rows == matrix->cols && eigenvalue && eigenvector);
    assert(eigenvector->rows == matrix->rows && eigenvector->cols == 1);
    
    int n = matrix->rows;
    
    // Initialize eigenvector with random values
    traditional_matrix_fill_random(eigenvector, -1.0, 1.0);
    
    // Normalize initial vector
    double norm = 0.0;
    for (int i = 0; i < n; i++) {
        double val = traditional_matrix_get(eigenvector, i, 0);
        norm += val * val;
    }
    norm = sqrt(norm);
    
    for (int i = 0; i < n; i++) {
        traditional_matrix_set(eigenvector, i, 0, 
                              traditional_matrix_get(eigenvector, i, 0) / norm);
    }
    
    double prev_eigenvalue = 0.0;
    
    // Power iteration
    for (int iter = 0; iter < max_iterations; iter++) {
        // Multiply matrix by vector
        traditional_matrix_t* temp = traditional_matrix_multiply(matrix, eigenvector);
        if (!temp) return -1;
        
        // Find the largest component for eigenvalue estimate
        double max_component = 0.0;
        int max_index = 0;
        for (int i = 0; i < n; i++) {
            double abs_val = fabs(traditional_matrix_get(temp, i, 0));
            if (abs_val > max_component) {
                max_component = abs_val;
                max_index = i;
            }
        }
        
        *eigenvalue = traditional_matrix_get(temp, max_index, 0) / 
                     traditional_matrix_get(eigenvector, max_index, 0);
        
        // Normalize new vector
        norm = 0.0;
        for (int i = 0; i < n; i++) {
            double val = traditional_matrix_get(temp, i, 0);
            norm += val * val;
        }
        norm = sqrt(norm);
        
        for (int i = 0; i < n; i++) {
            traditional_matrix_set(eigenvector, i, 0, 
                                  traditional_matrix_get(temp, i, 0) / norm);
        }
        
        traditional_matrix_destroy(temp);
        
        // Check convergence
        if (iter > 0 && fabs(*eigenvalue - prev_eigenvalue) < tolerance) {
            return iter + 1; // Return number of iterations
        }
        
        prev_eigenvalue = *eigenvalue;
    }
    
    return max_iterations; // Did not converge
}

// =============================================================================
// Benchmark Functions
// =============================================================================

/**
 * Benchmark matrix multiplication performance.
 */
double traditional_matrix_benchmark_multiply(int size, int iterations) {
    traditional_matrix_t* a = traditional_matrix_create(size, size);
    traditional_matrix_t* b = traditional_matrix_create(size, size);
    
    if (!a || !b) {
        traditional_matrix_destroy(a);
        traditional_matrix_destroy(b);
        return -1.0;
    }
    
    traditional_matrix_fill_random(a, -10.0, 10.0);
    traditional_matrix_fill_random(b, -10.0, 10.0);
    
    clock_t start = clock();
    
    for (int i = 0; i < iterations; i++) {
        traditional_matrix_t* result = traditional_matrix_multiply(a, b);
        traditional_matrix_destroy(result);
    }
    
    clock_t end = clock();
    
    traditional_matrix_destroy(a);
    traditional_matrix_destroy(b);
    
    return ((double)(end - start)) / CLOCKS_PER_SEC / iterations;
}

/**
 * Benchmark determinant calculation performance.
 */
double traditional_matrix_benchmark_determinant(int size, int iterations) {
    traditional_matrix_t* matrix = traditional_matrix_create(size, size);
    if (!matrix) return -1.0;
    
    traditional_matrix_fill_random(matrix, -10.0, 10.0);
    
    clock_t start = clock();
    
    for (int i = 0; i < iterations; i++) {
        double det = traditional_matrix_determinant(matrix);
        (void)det; // Suppress unused variable warning
    }
    
    clock_t end = clock();
    
    traditional_matrix_destroy(matrix);
    
    return ((double)(end - start)) / CLOCKS_PER_SEC / iterations;
}

/**
 * Benchmark matrix inverse calculation performance.
 */
double traditional_matrix_benchmark_inverse(int size, int iterations) {
    traditional_matrix_t* matrix = traditional_matrix_create(size, size);
    if (!matrix) return -1.0;
    
    // Create well-conditioned matrix (identity + small random perturbations)
    traditional_matrix_identity(matrix);
    for (int i = 0; i < size; i++) {
        for (int j = 0; j < size; j++) {
            if (i != j) {
                double perturbation = 0.1 * ((double)rand() / RAND_MAX - 0.5);
                traditional_matrix_set(matrix, i, j, perturbation);
            }
        }
    }
    
    clock_t start = clock();
    
    for (int i = 0; i < iterations; i++) {
        traditional_matrix_t* inverse = traditional_matrix_inverse(matrix);
        traditional_matrix_destroy(inverse);
    }
    
    clock_t end = clock();
    
    traditional_matrix_destroy(matrix);
    
    return ((double)(end - start)) / CLOCKS_PER_SEC / iterations;
}

// =============================================================================
// Main Function for Testing
// =============================================================================

#ifdef TRADITIONAL_MATRIX_MAIN
int main(void) {
    printf("Traditional Matrix Operations Benchmark\n");
    printf("=====================================\n\n");
    
    srand((unsigned int)time(NULL));
    
    // Test basic operations
    printf("Testing 4x4 matrix operations...\n");
    
    traditional_matrix_t* a = traditional_matrix_create(4, 4);
    traditional_matrix_t* b = traditional_matrix_create(4, 4);
    
    traditional_matrix_fill_random(a, -5.0, 5.0);
    traditional_matrix_fill_random(b, -5.0, 5.0);
    
    printf("Matrix multiplication test: ");
    traditional_matrix_t* product = traditional_matrix_multiply(a, b);
    if (product) {
        printf("PASS\n");
        traditional_matrix_destroy(product);
    } else {
        printf("FAIL\n");
    }
    
    printf("Determinant calculation test: ");
    double det = traditional_matrix_determinant(a);
    if (!isnan(det)) {
        printf("PASS (det = %.6f)\n", det);
    } else {
        printf("FAIL\n");
    }
    
    printf("Matrix inverse test: ");
    traditional_matrix_t* inverse = traditional_matrix_inverse(a);
    if (inverse) {
        printf("PASS\n");
        traditional_matrix_destroy(inverse);
    } else {
        printf("FAIL\n");
    }
    
    traditional_matrix_destroy(a);
    traditional_matrix_destroy(b);
    
    // Performance benchmarks
    printf("\nPerformance Benchmarks:\n");
    
    int sizes[] = {8, 16, 32, 64};
    int num_sizes = sizeof(sizes) / sizeof(sizes[0]);
    
    for (int i = 0; i < num_sizes; i++) {
        int size = sizes[i];
        int iterations = (size <= 32) ? 100 : 10;
        
        printf("Matrix size %dx%d:\n", size, size);
        
        double mult_time = traditional_matrix_benchmark_multiply(size, iterations);
        if (mult_time > 0) {
            printf("  Multiplication: %.6f seconds per operation\n", mult_time);
        }
        
        double det_time = traditional_matrix_benchmark_determinant(size, iterations);
        if (det_time > 0) {
            printf("  Determinant:    %.6f seconds per operation\n", det_time);
        }
        
        double inv_time = traditional_matrix_benchmark_inverse(size, iterations / 2);
        if (inv_time > 0) {
            printf("  Inverse:        %.6f seconds per operation\n", inv_time);
        }
        
        printf("\n");
    }
    
    return 0;
}
#endif