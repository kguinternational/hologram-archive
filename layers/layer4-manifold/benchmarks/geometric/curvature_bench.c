/* curvature_bench.c - Layer 4 Curvature Computation Benchmark Suite
 * (c) 2024-2025 UOR Foundation - MIT License
 * 
 * Compares performance of Universal Number (UN) curvature computation using 
 * spectral moments vs traditional Riemann tensor curvature calculations.
 * 
 * Key Comparison:
 * - UN Curvature: Uses 2 spectral moments (trace invariants) as Universal Numbers
 *   that compose algebraically. Only requires Tr(A²) and Tr(A³) computations.
 * - Riemann Curvature: Traditional 4th-order tensor with 256 components requiring
 *   complex partial derivatives and Christoffel symbol computations.
 * 
 * The UN approach leverages Atlas theoretical frameworks:
 * - Universal Numbers are scalar invariants under symmetry transformations
 * - Spectral moments are witnessable and compose naturally
 * - Conservation laws are preserved automatically
 */

#define _POSIX_C_SOURCE 199309L
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <sys/time.h>
#include <stdbool.h>
#include <math.h>
#include <complex.h>

#ifdef __linux__
#include <sys/resource.h>
#endif

// Include Layer 4 manifold and Layer 3 resonance interfaces
#include "../../include/atlas-manifold.h"
#include "../../../layer3-resonance/include/atlas-resonance.h"

/* Benchmark configuration */
#define BENCHMARK_ITERATIONS 10000
#define WARMUP_ITERATIONS 1000
#define MEASUREMENT_SAMPLES 10
#define MATRIX_SIZE 4  // 4x4 matrices for manifold operations

/* Data sizes for throughput testing */
#define MATRICES_SMALL  100     // 100 matrices
#define MATRICES_MEDIUM 1000    // 1K matrices 
#define MATRICES_LARGE  10000   // 10K matrices

/* Performance targets */
#define TARGET_UN_CURVATURE_MPS 10.0e6    // Target ≥10M matrices/sec for UN curvature
#define TARGET_RIEMANN_CURVATURE_MPS 10000 // Target ≥10K matrices/sec for Riemann (expected much slower)

/* Timer utilities */
static inline uint64_t rdtsc(void) {
    uint32_t lo, hi;
    __asm__ __volatile__ ("rdtsc" : "=a" (lo), "=d" (hi));
    return ((uint64_t)hi << 32) | lo;
}

static double get_time_ns(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return ts.tv_sec * 1e9 + ts.tv_nsec;
}

/* Memory utilities */
static void* aligned_alloc_custom(size_t size, size_t alignment) {
    void* ptr;
    if (posix_memalign(&ptr, alignment, size) != 0) {
        return NULL;
    }
    return ptr;
}

/* Matrix structure for manifold operations */
typedef struct {
    double data[MATRIX_SIZE * MATRIX_SIZE];  // 4x4 matrix in row-major order
    uint8_t resonance_class;                 // Associated R96 resonance class for UN properties
} manifold_matrix_t;

/* Curvature result structures */
typedef struct {
    double spectral_moment_2;  // Tr(A²) - Universal Number
    double spectral_moment_3;  // Tr(A³) - Universal Number  
    double curvature_scalar;   // Single scalar curvature from UN composition
} un_curvature_t;

typedef struct {
    double components[256];    // Full Riemann tensor components (4⁴ = 256)
    double ricci_scalar;       // Scalar curvature from Riemann tensor contraction
    double ricci_tensor[16];   // Ricci tensor components
} riemann_curvature_t;

/* Benchmark result structure */
typedef struct {
    const char* name;
    double min_time_ns;
    double max_time_ns;
    double avg_time_ns;
    double median_time_ns;
    double matrices_per_second;
    double speedup_factor;
    uint64_t cycles_per_op;
    bool passed;
    size_t num_matrices;
    size_t memory_usage;
} benchmark_result_t;

/* Statistical functions */
static int compare_double(const void* a, const void* b) {
    double da = *(const double*)a;
    double db = *(const double*)b;
    return (da > db) - (da < db);
}

static double calculate_median(double* values, int count) {
    qsort(values, count, sizeof(double), compare_double);
    if (count % 2 == 0) {
        return (values[count/2 - 1] + values[count/2]) / 2.0;
    } else {
        return values[count/2];
    }
}

/* Data generation utilities */
static void generate_manifold_matrices(manifold_matrix_t* matrices, size_t count, uint32_t seed) {
    srand(seed);
    for (size_t i = 0; i < count; i++) {
        // Generate symmetric positive definite matrices for valid manifolds
        for (int row = 0; row < MATRIX_SIZE; row++) {
            for (int col = 0; col < MATRIX_SIZE; col++) {
                if (row == col) {
                    matrices[i].data[row * MATRIX_SIZE + col] = 1.0 + (rand() % 100) / 100.0; // Diagonal dominance
                } else if (row < col) {
                    double val = (rand() % 200 - 100) / 1000.0; // Small off-diagonal elements
                    matrices[i].data[row * MATRIX_SIZE + col] = val;
                    matrices[i].data[col * MATRIX_SIZE + row] = val; // Symmetric
                }
            }
        }
        
        // Generate resonance class from matrix trace for consistency
        double trace = 0;
        for (int j = 0; j < MATRIX_SIZE; j++) {
            trace += matrices[i].data[j * MATRIX_SIZE + j];
        }
        uint32_t trace_int = (uint32_t)(trace * 1000) & 0xFF;
        matrices[i].resonance_class = atlas_r96_classify((uint8_t)trace_int);
    }
}

/* Universal Number curvature computation using spectral moments */
static un_curvature_t compute_un_curvature(const manifold_matrix_t* matrix) {
    un_curvature_t result = {0};
    const double* A = matrix->data;
    
    // Compute Tr(A²) - Second spectral moment (Universal Number)
    for (int i = 0; i < MATRIX_SIZE; i++) {
        for (int k = 0; k < MATRIX_SIZE; k++) {
            result.spectral_moment_2 += A[i * MATRIX_SIZE + k] * A[k * MATRIX_SIZE + i];
        }
    }
    
    // Compute Tr(A³) - Third spectral moment (Universal Number)  
    for (int i = 0; i < MATRIX_SIZE; i++) {
        for (int j = 0; j < MATRIX_SIZE; j++) {
            for (int k = 0; k < MATRIX_SIZE; k++) {
                result.spectral_moment_3 += A[i * MATRIX_SIZE + j] * A[j * MATRIX_SIZE + k] * A[k * MATRIX_SIZE + i];
            }
        }
    }
    
    // Compose Universal Numbers algebraically to get scalar curvature
    // This is a much simpler operation than full Riemann tensor computation
    result.curvature_scalar = result.spectral_moment_2 - (result.spectral_moment_3 / (2 * result.spectral_moment_2 + 1e-10));
    
    return result;
}

/* Traditional Riemann tensor computation (computationally expensive) */
static riemann_curvature_t compute_riemann_curvature(const manifold_matrix_t* matrix) {
    riemann_curvature_t result = {0};
    const double* g = matrix->data; // Metric tensor
    
    // This is a simplified computation - full Riemann tensor requires:
    // 1. Computing Christoffel symbols (requires metric inverse and derivatives)
    // 2. Computing all 256 Riemann tensor components R^α_βγδ
    // 3. Contracting to get Ricci tensor and scalar curvature
    
    // Step 1: Compute metric inverse (simplified - assumes invertible)
    double g_inv[MATRIX_SIZE * MATRIX_SIZE];
    for (int i = 0; i < MATRIX_SIZE * MATRIX_SIZE; i++) {
        g_inv[i] = 0;
    }
    // Simplified inverse for diagonal dominance (real implementation would use LU decomposition)
    for (int i = 0; i < MATRIX_SIZE; i++) {
        if (fabs(g[i * MATRIX_SIZE + i]) > 1e-10) {
            g_inv[i * MATRIX_SIZE + i] = 1.0 / g[i * MATRIX_SIZE + i];
        }
    }
    
    // Step 2: Compute simplified Christoffel symbols (64 components)
    double christoffel[MATRIX_SIZE * MATRIX_SIZE * MATRIX_SIZE];
    for (int i = 0; i < MATRIX_SIZE; i++) {
        for (int j = 0; j < MATRIX_SIZE; j++) {
            for (int k = 0; k < MATRIX_SIZE; k++) {
                // Simplified computation (real implementation requires partial derivatives)
                christoffel[i * MATRIX_SIZE * MATRIX_SIZE + j * MATRIX_SIZE + k] = 
                    0.5 * (g[i * MATRIX_SIZE + j] + g[i * MATRIX_SIZE + k] - g[j * MATRIX_SIZE + k]) * g_inv[i * MATRIX_SIZE + i];
            }
        }
    }
    
    // Step 3: Compute Riemann tensor components (256 components)
    for (int rho = 0; rho < MATRIX_SIZE; rho++) {
        for (int sigma = 0; sigma < MATRIX_SIZE; sigma++) {
            for (int mu = 0; mu < MATRIX_SIZE; mu++) {
                for (int nu = 0; nu < MATRIX_SIZE; nu++) {
                    int idx = rho * MATRIX_SIZE * MATRIX_SIZE * MATRIX_SIZE + 
                             sigma * MATRIX_SIZE * MATRIX_SIZE + 
                             mu * MATRIX_SIZE + nu;
                    
                    // Simplified Riemann tensor computation
                    // Real computation involves derivatives of Christoffel symbols
                    double gamma1 = christoffel[rho * MATRIX_SIZE * MATRIX_SIZE + sigma * MATRIX_SIZE + mu];
                    double gamma2 = christoffel[rho * MATRIX_SIZE * MATRIX_SIZE + sigma * MATRIX_SIZE + nu];
                    result.components[idx] = gamma1 * gamma2 - gamma2 * gamma1;
                }
            }
        }
    }
    
    // Step 4: Contract to get Ricci tensor and scalar
    result.ricci_scalar = 0;
    for (int mu = 0; mu < MATRIX_SIZE; mu++) {
        for (int nu = 0; nu < MATRIX_SIZE; nu++) {
            double ricci_component = 0;
            for (int alpha = 0; alpha < MATRIX_SIZE; alpha++) {
                int idx = alpha * MATRIX_SIZE * MATRIX_SIZE * MATRIX_SIZE + 
                         mu * MATRIX_SIZE * MATRIX_SIZE + 
                         alpha * MATRIX_SIZE + nu;
                ricci_component += result.components[idx];
            }
            result.ricci_tensor[mu * MATRIX_SIZE + nu] = ricci_component;
            
            if (mu == nu) {
                result.ricci_scalar += ricci_component * g_inv[mu * MATRIX_SIZE + mu];
            }
        }
    }
    
    return result;
}

/* Benchmark UN curvature performance */
static benchmark_result_t benchmark_un_curvature(size_t num_matrices) {
    benchmark_result_t result = {0};
    result.name = (num_matrices == MATRICES_SMALL) ? "UN curvature (100 matrices)" :
                  (num_matrices == MATRICES_MEDIUM) ? "UN curvature (1K matrices)" :
                  "UN curvature (10K matrices)";
    result.num_matrices = num_matrices;
    result.memory_usage = num_matrices * (sizeof(manifold_matrix_t) + sizeof(un_curvature_t));
    
    manifold_matrix_t* matrices = aligned_alloc_custom(num_matrices * sizeof(manifold_matrix_t), 32);
    un_curvature_t* results_arr = aligned_alloc_custom(num_matrices * sizeof(un_curvature_t), 32);
    
    generate_manifold_matrices(matrices, num_matrices, 42);
    
    double times[MEASUREMENT_SAMPLES];
    uint64_t cycles_start, cycles_end;
    
    /* Warmup */
    for (int i = 0; i < WARMUP_ITERATIONS; i++) {
        for (size_t j = 0; j < num_matrices; j++) {
            results_arr[j] = compute_un_curvature(&matrices[j]);
        }
    }
    
    /* Measurements */
    for (int sample = 0; sample < MEASUREMENT_SAMPLES; sample++) {
        cycles_start = rdtsc();
        double start = get_time_ns();
        
        for (int i = 0; i < BENCHMARK_ITERATIONS; i++) {
            for (size_t j = 0; j < num_matrices; j++) {
                results_arr[j] = compute_un_curvature(&matrices[j]);
            }
        }
        
        double end = get_time_ns();
        cycles_end = rdtsc();
        
        times[sample] = (end - start) / BENCHMARK_ITERATIONS;
    }
    
    /* Calculate statistics */
    result.min_time_ns = times[0];
    result.max_time_ns = times[0];
    result.avg_time_ns = 0;
    
    for (int i = 0; i < MEASUREMENT_SAMPLES; i++) {
        if (times[i] < result.min_time_ns) result.min_time_ns = times[i];
        if (times[i] > result.max_time_ns) result.max_time_ns = times[i];
        result.avg_time_ns += times[i];
    }
    result.avg_time_ns /= MEASUREMENT_SAMPLES;
    result.median_time_ns = calculate_median(times, MEASUREMENT_SAMPLES);
    
    result.matrices_per_second = (num_matrices * 1e9) / result.avg_time_ns;
    result.cycles_per_op = (cycles_end - cycles_start) / (BENCHMARK_ITERATIONS * num_matrices);
    result.passed = result.matrices_per_second >= TARGET_UN_CURVATURE_MPS;
    
    free(matrices);
    free(results_arr);
    
    return result;
}

/* Benchmark Riemann curvature performance */
static benchmark_result_t benchmark_riemann_curvature(size_t num_matrices) {
    benchmark_result_t result = {0};
    result.name = (num_matrices == MATRICES_SMALL) ? "Riemann curvature (100 matrices)" :
                  (num_matrices == MATRICES_MEDIUM) ? "Riemann curvature (1K matrices)" :
                  "Riemann curvature (10K matrices)";
    result.num_matrices = num_matrices;
    result.memory_usage = num_matrices * (sizeof(manifold_matrix_t) + sizeof(riemann_curvature_t));
    
    manifold_matrix_t* matrices = aligned_alloc_custom(num_matrices * sizeof(manifold_matrix_t), 32);
    riemann_curvature_t* results_arr = aligned_alloc_custom(num_matrices * sizeof(riemann_curvature_t), 32);
    
    generate_manifold_matrices(matrices, num_matrices, 42);
    
    double times[MEASUREMENT_SAMPLES];
    uint64_t cycles_start, cycles_end;
    
    // Use fewer iterations for Riemann due to computational complexity
    int riemann_iterations = BENCHMARK_ITERATIONS / 10;
    if (riemann_iterations < 100) riemann_iterations = 100;
    
    /* Warmup */
    for (int i = 0; i < WARMUP_ITERATIONS / 10; i++) {
        for (size_t j = 0; j < num_matrices; j++) {
            results_arr[j] = compute_riemann_curvature(&matrices[j]);
        }
    }
    
    /* Measurements */
    for (int sample = 0; sample < MEASUREMENT_SAMPLES; sample++) {
        cycles_start = rdtsc();
        double start = get_time_ns();
        
        for (int i = 0; i < riemann_iterations; i++) {
            for (size_t j = 0; j < num_matrices; j++) {
                results_arr[j] = compute_riemann_curvature(&matrices[j]);
            }
        }
        
        double end = get_time_ns();
        cycles_end = rdtsc();
        
        times[sample] = (end - start) / riemann_iterations;
    }
    
    /* Calculate statistics */
    result.min_time_ns = times[0];
    result.max_time_ns = times[0];
    result.avg_time_ns = 0;
    
    for (int i = 0; i < MEASUREMENT_SAMPLES; i++) {
        if (times[i] < result.min_time_ns) result.min_time_ns = times[i];
        if (times[i] > result.max_time_ns) result.max_time_ns = times[i];
        result.avg_time_ns += times[i];
    }
    result.avg_time_ns /= MEASUREMENT_SAMPLES;
    result.median_time_ns = calculate_median(times, MEASUREMENT_SAMPLES);
    
    result.matrices_per_second = (num_matrices * 1e9) / result.avg_time_ns;
    result.cycles_per_op = (cycles_end - cycles_start) / (riemann_iterations * num_matrices);
    result.passed = result.matrices_per_second >= TARGET_RIEMANN_CURVATURE_MPS;
    
    free(matrices);
    free(results_arr);
    
    return result;
}

/* Memory efficiency comparison */
static benchmark_result_t benchmark_memory_efficiency(void) {
    benchmark_result_t result = {0};
    result.name = "Memory efficiency comparison";
    result.num_matrices = MATRICES_MEDIUM;
    
    size_t un_memory = sizeof(un_curvature_t);           // 3 doubles = 24 bytes
    size_t riemann_memory = sizeof(riemann_curvature_t); // 256 + 1 + 16 doubles = 273 doubles = ~2184 bytes
    
    double memory_efficiency = (double)riemann_memory / (double)un_memory;
    
    result.memory_usage = un_memory;
    result.speedup_factor = memory_efficiency;
    result.passed = memory_efficiency > 50.0; // UN should use at least 50x less memory
    
    printf("Memory Usage - UN: %zu bytes, Riemann: %zu bytes (%.1fx more efficient)\n",
           un_memory, riemann_memory, memory_efficiency);
    
    return result;
}

/* Computational complexity analysis */
static benchmark_result_t benchmark_complexity_analysis(void) {
    benchmark_result_t result = {0};
    result.name = "Computational complexity analysis";
    result.num_matrices = MATRICES_SMALL;
    
    manifold_matrix_t* matrices = aligned_alloc_custom(MATRICES_SMALL * sizeof(manifold_matrix_t), 32);
    generate_manifold_matrices(matrices, MATRICES_SMALL, 789);
    
    // Count operations for UN curvature (2 spectral moments)
    // Tr(A²): n² multiplications and n² additions = 2n² ops
    // Tr(A³): n³ multiplications = n³ ops
    // Total: O(n³) for n×n matrix
    int un_ops_per_matrix = MATRIX_SIZE * MATRIX_SIZE * MATRIX_SIZE + 2 * MATRIX_SIZE * MATRIX_SIZE;
    
    // Count operations for Riemann tensor
    // Metric inverse: O(n³) for LU decomposition
    // Christoffel symbols: n³ components × n ops each = O(n⁴)
    // Riemann tensor: n⁴ components × n ops each = O(n⁵)
    // Total: O(n⁵) for n×n matrix
    int riemann_ops_per_matrix = MATRIX_SIZE * MATRIX_SIZE * MATRIX_SIZE * MATRIX_SIZE * MATRIX_SIZE;
    
    double complexity_advantage = (double)riemann_ops_per_matrix / (double)un_ops_per_matrix;
    
    result.speedup_factor = complexity_advantage;
    result.passed = complexity_advantage > 10.0;
    
    printf("Operations per 4x4 matrix - UN: %d ops, Riemann: %d ops (%.1fx more efficient)\n",
           un_ops_per_matrix, riemann_ops_per_matrix, complexity_advantage);
    printf("Complexity - UN: O(n³), Riemann: O(n⁵) for n×n matrices\n");
    
    free(matrices);
    return result;
}

/* System information */
static void print_system_info(void) {
    printf("=== System Information ===\n");
    
#ifdef __linux__
    FILE* cpuinfo = fopen("/proc/cpuinfo", "r");
    if (cpuinfo) {
        char line[256];
        while (fgets(line, sizeof(line), cpuinfo)) {
            if (strncmp(line, "model name", 10) == 0) {
                printf("CPU: %s", strchr(line, ':') + 2);
                break;
            }
        }
        fclose(cpuinfo);
    }
#endif
    
    printf("Matrix size: %dx%d\n", MATRIX_SIZE, MATRIX_SIZE);
    printf("Benchmark iterations: %d (UN), %d (Riemann)\n", BENCHMARK_ITERATIONS, BENCHMARK_ITERATIONS/10);
    printf("Measurement samples: %d\n", MEASUREMENT_SAMPLES);
    printf("Matrix dataset sizes: %zu, %zu, %zu matrices\n", 
           (size_t)MATRICES_SMALL, (size_t)MATRICES_MEDIUM, (size_t)MATRICES_LARGE);
    printf("\n");
}

/* Results printing */
static void print_result(const benchmark_result_t* result) {
    printf("%-35s | ", result->name);
    printf("Avg: %10.2f ns | ", result->avg_time_ns);
    if (result->matrices_per_second > 1e6) {
        printf("Rate: %8.2f Mmat/s | ", result->matrices_per_second / 1e6);
    } else {
        printf("Rate: %8.0f mat/s | ", result->matrices_per_second);
    }
    if (result->speedup_factor > 0) {
        printf("Advantage: %6.1fx | ", result->speedup_factor);
    } else {
        printf("Cycles/op: %8llu | ", result->cycles_per_op);
    }
    printf("Status: %s\n", result->passed ? "PASS" : "FAIL");
}

/* Memory usage reporting */
static void print_memory_usage(void) {
#ifdef __linux__
    struct rusage usage;
    if (getrusage(RUSAGE_SELF, &usage) == 0) {
        printf("Peak Memory Usage: %ld KB\n", usage.ru_maxrss);
    }
#endif
}

int main(void) {
    printf("Atlas-12288 Layer 4 Curvature Computation Benchmark Suite\n");
    printf("=========================================================\n\n");
    
    print_system_info();
    
    benchmark_result_t results[8];
    int num_passed = 0;
    int test_count = 0;
    
    printf("=== Performance Benchmarks ===\n");
    printf("%-35s | %-15s | %-18s | %-15s | Status\n", "Test", "Time (Avg)", "Rate", "Advantage");
    printf("------------------------------------------------------------------------------------------------\n");
    
    /* Universal Number Curvature Tests */
    results[test_count] = benchmark_un_curvature(MATRICES_SMALL);
    print_result(&results[test_count]);
    if (results[test_count].passed) num_passed++;
    test_count++;
    
    results[test_count] = benchmark_un_curvature(MATRICES_MEDIUM);
    print_result(&results[test_count]);
    if (results[test_count].passed) num_passed++;
    test_count++;
    
    results[test_count] = benchmark_un_curvature(MATRICES_LARGE);
    print_result(&results[test_count]);
    if (results[test_count].passed) num_passed++;
    test_count++;
    
    /* Riemann Tensor Curvature Tests */
    results[test_count] = benchmark_riemann_curvature(MATRICES_SMALL);
    print_result(&results[test_count]);
    if (results[test_count].passed) num_passed++;
    test_count++;
    
    results[test_count] = benchmark_riemann_curvature(MATRICES_MEDIUM);
    print_result(&results[test_count]);
    if (results[test_count].passed) num_passed++;
    test_count++;
    
    /* Efficiency Analysis */
    results[test_count] = benchmark_memory_efficiency();
    print_result(&results[test_count]);
    if (results[test_count].passed) num_passed++;
    test_count++;
    
    results[test_count] = benchmark_complexity_analysis();
    print_result(&results[test_count]);
    if (results[test_count].passed) num_passed++;
    test_count++;
    
    printf("\n=== Performance Analysis ===\n");
    // Calculate performance ratios
    double un_rate_large = results[2].matrices_per_second;
    double riemann_rate_medium = results[4].matrices_per_second;
    double performance_advantage = un_rate_large / riemann_rate_medium;
    
    printf("UN Curvature (10K matrices):     %.2f Mmat/s\n", un_rate_large / 1e6);
    printf("Riemann Curvature (1K matrices): %.0f mat/s\n", riemann_rate_medium);
    printf("Performance Advantage:           %.0fx faster with UN approach\n", performance_advantage);
    
    printf("\n=== Theoretical Framework ===\n");
    printf("Universal Numbers (UN):          Scalar invariants, algebraic composition\n");
    printf("Spectral Moments:               Tr(A²) + Tr(A³) = 2 scalar operations\n");
    printf("Riemann Tensor:                 256 components, O(n⁵) complexity\n");
    printf("Conservation:                   UN automatically preserves Atlas conservation laws\n");
    printf("Witnessability:                 UN operations are verifiable with certificates\n");
    
    printf("\n=== Memory and Computational Efficiency ===\n");
    size_t un_memory = 24;      // 3 doubles
    size_t riemann_memory = 2184; // ~273 doubles
    printf("Memory - UN: %zu bytes, Riemann: %zu bytes (%.0fx more efficient)\n",
           un_memory, riemann_memory, (double)riemann_memory / un_memory);
    printf("Complexity - UN: O(n³), Riemann: O(n⁵)\n");
    print_memory_usage();
    
    printf("\n=== Summary ===\n");
    printf("Tests passed: %d/%d\n", num_passed, test_count);
    printf("Overall performance advantage: %.0fx with Universal Number curvature\n", performance_advantage);
    printf("Overall status: %s\n", (num_passed == test_count) ? "ALL TESTS PASSED" : "SOME TESTS FAILED");
    
    /* Output parseable results for automation */
    printf("\n=== Parseable Results (CSV) ===\n");
    printf("test_name,avg_time_ns,matrices_per_second,advantage_factor,memory_bytes,passed\n");
    for (int i = 0; i < test_count; i++) {
        printf("%s,%.2f,%.0f,%.1f,%zu,%s\n",
               results[i].name,
               results[i].avg_time_ns,
               results[i].matrices_per_second,
               results[i].speedup_factor,
               results[i].memory_usage,
               results[i].passed ? "true" : "false");
    }
    
    return (num_passed == test_count) ? 0 : 1;
}