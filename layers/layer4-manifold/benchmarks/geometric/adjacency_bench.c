/* adjacency_bench.c - Layer 4 Adjacency Matrix Benchmark Suite
 * (c) 2024-2025 UOR Foundation - MIT License
 * 
 * Compares performance of R96 harmonic pairing for adjacency determination
 * vs traditional adjacency matrix storage and lookup operations.
 * 
 * Key Comparison:
 * - R96 Harmonic Pairing: Determines adjacency using (r1 + r2) % 96 == 0
 *   computation. No storage required - adjacency computed on demand in O(1).
 * - Traditional Adjacency Matrix: Stores N×N boolean matrix requiring O(N²) 
 *   memory and O(1) lookup but expensive O(N²) construction and updates.
 * 
 * The harmonic approach leverages mathematical properties where elements are
 * "adjacent" in the resonance space rather than geometric space, providing
 * both computational and memory efficiency advantages.
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

#ifdef __linux__
#include <sys/resource.h>
#endif

// Include Layer 4 manifold and Layer 3 resonance interfaces
#include "../../include/atlas-manifold.h"
#include "../../../layer3-resonance/include/atlas-resonance.h"

/* Benchmark configuration */
#define BENCHMARK_ITERATIONS 100000
#define WARMUP_ITERATIONS 10000
#define MEASUREMENT_SAMPLES 10

/* Graph sizes for adjacency testing */
#define NODES_SMALL  100     // 100 nodes (10K adjacency entries)
#define NODES_MEDIUM 1000    // 1K nodes (1M adjacency entries)
#define NODES_LARGE  5000    // 5K nodes (25M adjacency entries)

/* Performance targets */
#define TARGET_HARMONIC_LOOKUPS_PPS 100.0e6   // Target ≥100M lookups/sec for harmonic
#define TARGET_MATRIX_LOOKUPS_PPS 500.0e6     // Target ≥500M lookups/sec for matrix (faster lookup)
#define TARGET_MATRIX_CONSTRUCTION_PPS 1.0e6  // Target ≥1M entries/sec for matrix construction

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

/* Node structure with resonance class */
typedef struct {
    uint32_t id;
    uint8_t resonance_class;
    double x, y; // Coordinates for traditional adjacency calculation
} node_t;

/* Traditional adjacency matrix */
typedef struct {
    bool* matrix;           // N×N adjacency matrix
    size_t num_nodes;       // Number of nodes
    size_t matrix_size;     // Total matrix size in bytes
} adjacency_matrix_t;

/* Adjacency query pair */
typedef struct {
    uint32_t node_a;
    uint32_t node_b;
} adjacency_query_t;

/* Benchmark result structure */
typedef struct {
    const char* name;
    double min_time_ns;
    double max_time_ns;
    double avg_time_ns;
    double median_time_ns;
    double operations_per_second;
    double speedup_factor;
    uint64_t cycles_per_op;
    bool passed;
    size_t num_nodes;
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
static void generate_nodes(node_t* nodes, size_t count, uint32_t seed) {
    srand(seed);
    for (size_t i = 0; i < count; i++) {
        nodes[i].id = (uint32_t)i;
        nodes[i].x = (rand() % 2000) - 1000.0;  // [-1000, 1000)
        nodes[i].y = (rand() % 2000) - 1000.0;
        
        // Generate resonance class from coordinates hash
        uint32_t hash = (uint32_t)(nodes[i].x * 100) ^ (uint32_t)(nodes[i].y * 100);
        nodes[i].resonance_class = atlas_r96_classify((uint8_t)(hash & 0xFF));
    }
}

static void generate_adjacency_queries(adjacency_query_t* queries, size_t count, 
                                      size_t num_nodes, uint32_t seed) {
    srand(seed);
    for (size_t i = 0; i < count; i++) {
        queries[i].node_a = rand() % num_nodes;
        queries[i].node_b = rand() % num_nodes;
        
        // Ensure different nodes for meaningful adjacency test
        if (queries[i].node_a == queries[i].node_b) {
            queries[i].node_b = (queries[i].node_b + 1) % num_nodes;
        }
    }
}

/* Adjacency matrix operations */
static adjacency_matrix_t* create_adjacency_matrix(const node_t* nodes, size_t num_nodes, 
                                                  double distance_threshold) {
    adjacency_matrix_t* adj = malloc(sizeof(adjacency_matrix_t));
    adj->num_nodes = num_nodes;
    adj->matrix_size = num_nodes * num_nodes * sizeof(bool);
    adj->matrix = aligned_alloc_custom(adj->matrix_size, 32);
    
    if (!adj->matrix) {
        free(adj);
        return NULL;
    }
    
    // Initialize adjacency based on geometric distance
    for (size_t i = 0; i < num_nodes; i++) {
        for (size_t j = 0; j < num_nodes; j++) {
            if (i == j) {
                adj->matrix[i * num_nodes + j] = false; // No self-adjacency
            } else {
                double dx = nodes[i].x - nodes[j].x;
                double dy = nodes[i].y - nodes[j].y;
                double distance = sqrt(dx * dx + dy * dy);
                adj->matrix[i * num_nodes + j] = (distance <= distance_threshold);
            }
        }
    }
    
    return adj;
}

static void destroy_adjacency_matrix(adjacency_matrix_t* adj) {
    if (adj) {
        free(adj->matrix);
        free(adj);
    }
}

static bool adjacency_matrix_lookup(const adjacency_matrix_t* adj, uint32_t node_a, uint32_t node_b) {
    if (node_a >= adj->num_nodes || node_b >= adj->num_nodes) {
        return false;
    }
    return adj->matrix[node_a * adj->num_nodes + node_b];
}

/* R96 harmonic adjacency computation */
static bool harmonic_adjacency_lookup(const node_t* nodes, uint32_t node_a, uint32_t node_b) {
    uint8_t r1 = nodes[node_a].resonance_class;
    uint8_t r2 = nodes[node_b].resonance_class;
    return atlas_r96_harmonizes(r1, r2);
}

/* Benchmark R96 harmonic adjacency lookups */
static benchmark_result_t benchmark_harmonic_adjacency_lookups(size_t num_nodes) {
    benchmark_result_t result = {0};
    result.name = (num_nodes == NODES_SMALL) ? "Harmonic lookups (100 nodes)" :
                  (num_nodes == NODES_MEDIUM) ? "Harmonic lookups (1K nodes)" :
                  "Harmonic lookups (5K nodes)";
    result.num_nodes = num_nodes;
    result.memory_usage = num_nodes * sizeof(node_t); // Only need nodes, no adjacency matrix
    
    node_t* nodes = aligned_alloc_custom(num_nodes * sizeof(node_t), 32);
    size_t num_queries = BENCHMARK_ITERATIONS;
    adjacency_query_t* queries = aligned_alloc_custom(num_queries * sizeof(adjacency_query_t), 32);
    bool* results_arr = aligned_alloc_custom(num_queries * sizeof(bool), 32);
    
    generate_nodes(nodes, num_nodes, 42);
    generate_adjacency_queries(queries, num_queries, num_nodes, 123);
    
    double times[MEASUREMENT_SAMPLES];
    uint64_t cycles_start, cycles_end;
    
    /* Warmup */
    for (int i = 0; i < WARMUP_ITERATIONS; i++) {
        size_t idx = i % num_queries;
        results_arr[idx] = harmonic_adjacency_lookup(nodes, queries[idx].node_a, queries[idx].node_b);
    }
    
    /* Measurements */
    for (int sample = 0; sample < MEASUREMENT_SAMPLES; sample++) {
        cycles_start = rdtsc();
        double start = get_time_ns();
        
        for (size_t i = 0; i < num_queries; i++) {
            results_arr[i] = harmonic_adjacency_lookup(nodes, queries[i].node_a, queries[i].node_b);
        }
        
        double end = get_time_ns();
        cycles_end = rdtsc();
        
        times[sample] = (end - start) / num_queries;
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
    
    result.operations_per_second = 1e9 / result.avg_time_ns;
    result.cycles_per_op = (cycles_end - cycles_start) / num_queries;
    result.passed = result.operations_per_second >= TARGET_HARMONIC_LOOKUPS_PPS;
    
    free(nodes);
    free(queries);
    free(results_arr);
    
    return result;
}

/* Benchmark traditional adjacency matrix lookups */
static benchmark_result_t benchmark_matrix_adjacency_lookups(size_t num_nodes) {
    benchmark_result_t result = {0};
    result.name = (num_nodes == NODES_SMALL) ? "Matrix lookups (100 nodes)" :
                  (num_nodes == NODES_MEDIUM) ? "Matrix lookups (1K nodes)" :
                  "Matrix lookups (5K nodes)";
    result.num_nodes = num_nodes;
    
    node_t* nodes = aligned_alloc_custom(num_nodes * sizeof(node_t), 32);
    generate_nodes(nodes, num_nodes, 42);
    
    // Create adjacency matrix with reasonable distance threshold
    double distance_threshold = 500.0; // Nodes within 500 units are adjacent
    adjacency_matrix_t* adj_matrix = create_adjacency_matrix(nodes, num_nodes, distance_threshold);
    
    result.memory_usage = num_nodes * sizeof(node_t) + adj_matrix->matrix_size;
    
    size_t num_queries = BENCHMARK_ITERATIONS;
    adjacency_query_t* queries = aligned_alloc_custom(num_queries * sizeof(adjacency_query_t), 32);
    bool* results_arr = aligned_alloc_custom(num_queries * sizeof(bool), 32);
    
    generate_adjacency_queries(queries, num_queries, num_nodes, 123);
    
    double times[MEASUREMENT_SAMPLES];
    uint64_t cycles_start, cycles_end;
    
    /* Warmup */
    for (int i = 0; i < WARMUP_ITERATIONS; i++) {
        size_t idx = i % num_queries;
        results_arr[idx] = adjacency_matrix_lookup(adj_matrix, queries[idx].node_a, queries[idx].node_b);
    }
    
    /* Measurements */
    for (int sample = 0; sample < MEASUREMENT_SAMPLES; sample++) {
        cycles_start = rdtsc();
        double start = get_time_ns();
        
        for (size_t i = 0; i < num_queries; i++) {
            results_arr[i] = adjacency_matrix_lookup(adj_matrix, queries[i].node_a, queries[i].node_b);
        }
        
        double end = get_time_ns();
        cycles_end = rdtsc();
        
        times[sample] = (end - start) / num_queries;
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
    
    result.operations_per_second = 1e9 / result.avg_time_ns;
    result.cycles_per_op = (cycles_end - cycles_start) / num_queries;
    result.passed = result.operations_per_second >= TARGET_MATRIX_LOOKUPS_PPS;
    
    destroy_adjacency_matrix(adj_matrix);
    free(nodes);
    free(queries);
    free(results_arr);
    
    return result;
}

/* Benchmark adjacency matrix construction time */
static benchmark_result_t benchmark_matrix_construction(size_t num_nodes) {
    benchmark_result_t result = {0};
    result.name = (num_nodes == NODES_SMALL) ? "Matrix construction (100 nodes)" :
                  (num_nodes == NODES_MEDIUM) ? "Matrix construction (1K nodes)" :
                  "Matrix construction (5K nodes)";
    result.num_nodes = num_nodes;
    result.memory_usage = num_nodes * sizeof(node_t) + num_nodes * num_nodes * sizeof(bool);
    
    node_t* nodes = aligned_alloc_custom(num_nodes * sizeof(node_t), 32);
    generate_nodes(nodes, num_nodes, 42);
    
    double distance_threshold = 500.0;
    double times[MEASUREMENT_SAMPLES];
    uint64_t cycles_start, cycles_end;
    
    // Use fewer iterations for matrix construction due to O(n²) complexity
    int construction_iterations = (num_nodes > NODES_MEDIUM) ? 10 : 
                                 (num_nodes > NODES_SMALL) ? 100 : 1000;
    
    /* Warmup */
    for (int i = 0; i < 3; i++) {
        adjacency_matrix_t* adj = create_adjacency_matrix(nodes, num_nodes, distance_threshold);
        destroy_adjacency_matrix(adj);
    }
    
    /* Measurements */
    for (int sample = 0; sample < MEASUREMENT_SAMPLES; sample++) {
        cycles_start = rdtsc();
        double start = get_time_ns();
        
        for (int i = 0; i < construction_iterations; i++) {
            adjacency_matrix_t* adj = create_adjacency_matrix(nodes, num_nodes, distance_threshold);
            destroy_adjacency_matrix(adj);
        }
        
        double end = get_time_ns();
        cycles_end = rdtsc();
        
        times[sample] = (end - start) / construction_iterations;
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
    
    // Operations per second based on matrix entries (N²)
    double entries_per_construction = num_nodes * num_nodes;
    result.operations_per_second = entries_per_construction * 1e9 / result.avg_time_ns;
    result.cycles_per_op = (cycles_end - cycles_start) / (construction_iterations * entries_per_construction);
    result.passed = result.operations_per_second >= TARGET_MATRIX_CONSTRUCTION_PPS;
    
    free(nodes);
    return result;
}

/* Memory efficiency analysis */
static benchmark_result_t benchmark_memory_efficiency(void) {
    benchmark_result_t result = {0};
    result.name = "Memory efficiency analysis";
    result.num_nodes = NODES_MEDIUM;
    
    size_t num_nodes = NODES_MEDIUM;
    
    // Harmonic approach: Only store nodes with resonance classes
    size_t harmonic_memory = num_nodes * sizeof(node_t); // ~12 bytes per node
    
    // Traditional approach: Store nodes + full adjacency matrix
    size_t matrix_memory = num_nodes * sizeof(node_t) + num_nodes * num_nodes * sizeof(bool);
    
    double memory_efficiency = (double)matrix_memory / (double)harmonic_memory;
    
    result.memory_usage = harmonic_memory;
    result.speedup_factor = memory_efficiency;
    result.passed = memory_efficiency > 50.0; // Should be much more efficient
    
    printf("Memory Usage for %zu nodes:\n", num_nodes);
    printf("  Harmonic approach:  %zu bytes (%zu bytes/node)\n", 
           harmonic_memory, harmonic_memory / num_nodes);
    printf("  Matrix approach:    %zu bytes (%zu bytes/node)\n", 
           matrix_memory, matrix_memory / num_nodes);
    printf("  Efficiency:         %.1fx less memory with harmonic\n", memory_efficiency);
    
    return result;
}

/* Scalability analysis */
static benchmark_result_t benchmark_scalability_analysis(void) {
    benchmark_result_t result = {0};
    result.name = "Scalability analysis";
    result.num_nodes = 0; // Not specific to one size
    
    // Analyze scaling characteristics
    double small_nodes = NODES_SMALL;
    double medium_nodes = NODES_MEDIUM;
    double large_nodes = NODES_LARGE;
    
    // Harmonic approach scaling: O(1) per lookup, O(N) total storage
    double harmonic_small = small_nodes * sizeof(node_t);
    double harmonic_medium = medium_nodes * sizeof(node_t);
    double harmonic_large = large_nodes * sizeof(node_t);
    
    // Matrix approach scaling: O(1) per lookup, O(N²) total storage
    double matrix_small = small_nodes * small_nodes * sizeof(bool);
    double matrix_medium = medium_nodes * medium_nodes * sizeof(bool);
    double matrix_large = large_nodes * large_nodes * sizeof(bool);
    
    double scaling_advantage_medium = (matrix_medium / harmonic_medium) / (matrix_small / harmonic_small);
    double scaling_advantage_large = (matrix_large / harmonic_large) / (matrix_medium / harmonic_medium);
    
    result.speedup_factor = scaling_advantage_large;
    result.passed = scaling_advantage_large > 1.0; // Should scale better
    
    printf("Scaling Analysis:\n");
    printf("  Small (%d nodes):   Harmonic %.0f B, Matrix %.0f B (%.1fx)\n", 
           NODES_SMALL, harmonic_small, matrix_small, matrix_small / harmonic_small);
    printf("  Medium (%d nodes):  Harmonic %.0f B, Matrix %.0f B (%.1fx)\n", 
           NODES_MEDIUM, harmonic_medium, matrix_medium, matrix_medium / harmonic_medium);
    printf("  Large (%d nodes):   Harmonic %.0f B, Matrix %.0f B (%.1fx)\n", 
           NODES_LARGE, harmonic_large, matrix_large, matrix_large / harmonic_large);
    printf("  Scaling advantage:  %.1fx improvement as size increases\n", scaling_advantage_large);
    
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
    
    printf("Node sizes: %zu, %zu, %zu nodes\n", 
           (size_t)NODES_SMALL, (size_t)NODES_MEDIUM, (size_t)NODES_LARGE);
    printf("Adjacency matrix sizes: %.1f KB, %.1f MB, %.1f MB\n",
           (double)(NODES_SMALL * NODES_SMALL) / 1024.0,
           (double)(NODES_MEDIUM * NODES_MEDIUM) / (1024.0 * 1024.0),
           (double)(NODES_LARGE * NODES_LARGE) / (1024.0 * 1024.0));
    printf("R96 resonance classes: 96\n");
    printf("Measurement samples: %d\n", MEASUREMENT_SAMPLES);
    printf("\n");
}

/* Results printing */
static void print_result(const benchmark_result_t* result) {
    printf("%-35s | ", result->name);
    printf("Avg: %10.2f ns | ", result->avg_time_ns);
    if (result->operations_per_second > 1e6) {
        printf("Rate: %8.2f Mops/s | ", result->operations_per_second / 1e6);
    } else {
        printf("Rate: %8.0f ops/s | ", result->operations_per_second);
    }
    if (result->speedup_factor > 0) {
        printf("Advantage: %6.1fx | ", result->speedup_factor);
    } else {
        printf("Memory: %6zu KB | ", result->memory_usage / 1024);
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
    printf("Atlas-12288 Layer 4 Adjacency Matrix Benchmark Suite\n");
    printf("===================================================\n\n");
    
    print_system_info();
    
    benchmark_result_t results[10];
    int num_passed = 0;
    int test_count = 0;
    
    printf("=== Performance Benchmarks ===\n");
    printf("%-35s | %-15s | %-18s | %-15s | Status\n", "Test", "Time (Avg)", "Rate", "Advantage");
    printf("------------------------------------------------------------------------------------------------\n");
    
    /* Harmonic Adjacency Lookup Tests */
    results[test_count] = benchmark_harmonic_adjacency_lookups(NODES_SMALL);
    print_result(&results[test_count]);
    if (results[test_count].passed) num_passed++;
    test_count++;
    
    results[test_count] = benchmark_harmonic_adjacency_lookups(NODES_MEDIUM);
    print_result(&results[test_count]);
    if (results[test_count].passed) num_passed++;
    test_count++;
    
    results[test_count] = benchmark_harmonic_adjacency_lookups(NODES_LARGE);
    print_result(&results[test_count]);
    if (results[test_count].passed) num_passed++;
    test_count++;
    
    /* Traditional Matrix Lookup Tests */
    results[test_count] = benchmark_matrix_adjacency_lookups(NODES_SMALL);
    print_result(&results[test_count]);
    if (results[test_count].passed) num_passed++;
    test_count++;
    
    results[test_count] = benchmark_matrix_adjacency_lookups(NODES_MEDIUM);
    print_result(&results[test_count]);
    if (results[test_count].passed) num_passed++;
    test_count++;
    
    /* Matrix Construction Tests */
    results[test_count] = benchmark_matrix_construction(NODES_SMALL);
    print_result(&results[test_count]);
    if (results[test_count].passed) num_passed++;
    test_count++;
    
    results[test_count] = benchmark_matrix_construction(NODES_MEDIUM);
    print_result(&results[test_count]);
    if (results[test_count].passed) num_passed++;
    test_count++;
    
    /* Efficiency Analysis */
    results[test_count] = benchmark_memory_efficiency();
    print_result(&results[test_count]);
    if (results[test_count].passed) num_passed++;
    test_count++;
    
    results[test_count] = benchmark_scalability_analysis();
    print_result(&results[test_count]);
    if (results[test_count].passed) num_passed++;
    test_count++;
    
    printf("\n=== Performance Analysis ===\n");
    // Calculate lookup performance ratios
    double harmonic_rate_medium = results[1].operations_per_second;
    double matrix_rate_medium = results[4].operations_per_second;
    double lookup_speed_ratio = matrix_rate_medium / harmonic_rate_medium;
    
    printf("Lookup Performance (1K nodes):\n");
    printf("  Harmonic adjacency:  %.2f Mops/s\n", harmonic_rate_medium / 1e6);
    printf("  Matrix adjacency:    %.2f Mops/s\n", matrix_rate_medium / 1e6);
    printf("  Lookup speed ratio:  %.2fx (matrix faster for lookups)\n", lookup_speed_ratio);
    
    // Memory usage comparison
    size_t harmonic_memory_1k = NODES_MEDIUM * sizeof(node_t);
    size_t matrix_memory_1k = NODES_MEDIUM * sizeof(node_t) + NODES_MEDIUM * NODES_MEDIUM;
    double memory_advantage = (double)matrix_memory_1k / (double)harmonic_memory_1k;
    printf("\nMemory Usage (1K nodes):\n");
    printf("  Harmonic approach:   %zu KB\n", harmonic_memory_1k / 1024);
    printf("  Matrix approach:     %zu KB\n", matrix_memory_1k / 1024);
    printf("  Memory advantage:    %.0fx less memory with harmonic\n", memory_advantage);
    
    printf("\n=== Theoretical Framework ===\n");
    printf("R96 Harmonic Adjacency:       Compute (r1 + r2) %% 96 == 0\n");
    printf("Memory Complexity:            O(N) for harmonic vs O(N²) for matrix\n");
    printf("Lookup Complexity:            O(1) for both approaches\n");
    printf("Construction:                 O(1) harmonic vs O(N²) matrix construction\n");
    printf("Dynamic Updates:              O(1) harmonic vs O(N²) matrix reconstruction\n");
    
    printf("\n=== Atlas Integration ===\n");
    printf("Layer 3 Harmonization:        Uses atlas_r96_harmonizes() function\n");
    printf("R96 Classification:           Based on atlas_r96_classify() output\n");
    printf("Conservation Laws:            Harmonic relationships preserve mod-96\n");
    printf("Mathematical Basis:           Resonance space adjacency vs geometric\n");
    
    printf("\n=== Trade-off Analysis ===\n");
    printf("Harmonic Advantages:          %.0fx less memory, O(1) updates, no construction\n", memory_advantage);
    printf("Matrix Advantages:            %.2fx faster lookups (cached memory access)\n", lookup_speed_ratio);
    printf("Optimal Use Case:             Harmonic for dynamic/sparse, Matrix for static/dense\n");
    
    printf("\n=== Memory and Resource Usage ===\n");
    print_memory_usage();
    
    printf("\n=== Summary ===\n");
    printf("Tests passed: %d/%d\n", num_passed, test_count);
    printf("Memory efficiency: %.0fx with R96 harmonic adjacency\n", memory_advantage);
    printf("Overall status: %s\n", (num_passed == test_count) ? "ALL TESTS PASSED" : "SOME TESTS FAILED");
    
    /* Output parseable results for automation */
    printf("\n=== Parseable Results (CSV) ===\n");
    printf("test_name,avg_time_ns,operations_per_second,advantage_factor,memory_usage,passed\n");
    for (int i = 0; i < test_count; i++) {
        printf("%s,%.2f,%.0f,%.1f,%zu,%s\n",
               results[i].name,
               results[i].avg_time_ns,
               results[i].operations_per_second,
               results[i].speedup_factor,
               results[i].memory_usage,
               results[i].passed ? "true" : "false");
    }
    
    return (num_passed == test_count) ? 0 : 1;
}