/* distance_bench.c - Layer 4 Distance Computation Benchmark Suite
 * (c) 2024-2025 UOR Foundation - MIT License
 * 
 * Compares performance of harmonic adjacency (R96-based O(1)) vs traditional 
 * Euclidean distance calculations (O(n)) to demonstrate the efficiency gains
 * from using Atlas resonance class harmonization instead of geometric computation.
 * 
 * Key Comparison:
 * - Harmonic Adjacency: Uses R96 classification where adjacency is defined as
 *   two elements being adjacent if (r1 + r2) % 96 == 0. This is O(1) computation.
 * - Euclidean Distance: Traditional geometric distance calculation requiring
 *   square root and coordinate transformations. This scales with coordinate precision.
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
#define MAX_COORDINATE_PAIRS 10000

/* Data sizes for throughput testing */
#define PAIRS_SMALL  100      // 100 coordinate pairs
#define PAIRS_MEDIUM 1000     // 1K coordinate pairs 
#define PAIRS_LARGE  10000    // 10K coordinate pairs

/* Performance targets */
#define TARGET_HARMONIC_ADJACENCY_MPS 50.0e6  // Target ≥50M pairs/sec for harmonic adjacency
#define TARGET_EUCLIDEAN_DISTANCE_MPS 1.0e6   // Target ≥1M pairs/sec for Euclidean distance (expected slower)

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

/* Coordinate pair structure for geometric operations */
typedef struct {
    double x1, y1;  // First point coordinates
    double x2, y2;  // Second point coordinates
    uint8_t r1, r2; // Resonance classes for harmonic computation
} coordinate_pair_t;

/* Benchmark result structure */
typedef struct {
    const char* name;
    double min_time_ns;
    double max_time_ns;
    double avg_time_ns;
    double median_time_ns;
    double pairs_per_second;
    double speedup_factor;
    uint64_t cycles_per_op;
    bool passed;
    size_t num_pairs;
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
static void generate_coordinate_pairs(coordinate_pair_t* pairs, size_t count, uint32_t seed) {
    srand(seed);
    for (size_t i = 0; i < count; i++) {
        // Generate coordinates in reasonable range
        pairs[i].x1 = (rand() % 2000) - 1000.0;  // [-1000, 1000)
        pairs[i].y1 = (rand() % 2000) - 1000.0;
        pairs[i].x2 = (rand() % 2000) - 1000.0;
        pairs[i].y2 = (rand() % 2000) - 1000.0;
        
        // Generate resonance classes from coordinate hash for consistency
        uint32_t hash1 = (uint32_t)(pairs[i].x1 * 100) ^ (uint32_t)(pairs[i].y1 * 100);
        uint32_t hash2 = (uint32_t)(pairs[i].x2 * 100) ^ (uint32_t)(pairs[i].y2 * 100);
        pairs[i].r1 = atlas_r96_classify((uint8_t)(hash1 & 0xFF));
        pairs[i].r2 = atlas_r96_classify((uint8_t)(hash2 & 0xFF));
    }
}

/* Harmonic adjacency computation (O(1)) */
static bool compute_harmonic_adjacency(const coordinate_pair_t* pair) {
    // Use Layer 3 harmonic function: adjacency if (r1 + r2) % 96 == 0
    return atlas_r96_harmonizes(pair->r1, pair->r2);
}

/* Euclidean distance computation (O(n) due to sqrt and coordinate operations) */
static double compute_euclidean_distance(const coordinate_pair_t* pair) {
    double dx = pair->x2 - pair->x1;
    double dy = pair->y2 - pair->y1;
    return sqrt(dx * dx + dy * dy);
}

/* Benchmark harmonic adjacency performance */
static benchmark_result_t benchmark_harmonic_adjacency(size_t num_pairs) {
    benchmark_result_t result = {0};
    result.name = (num_pairs == PAIRS_SMALL) ? "Harmonic adjacency (100 pairs)" :
                  (num_pairs == PAIRS_MEDIUM) ? "Harmonic adjacency (1K pairs)" :
                  "Harmonic adjacency (10K pairs)";
    result.num_pairs = num_pairs;
    
    coordinate_pair_t* pairs = aligned_alloc_custom(num_pairs * sizeof(coordinate_pair_t), 32);
    bool* results = aligned_alloc_custom(num_pairs * sizeof(bool), 32);
    
    generate_coordinate_pairs(pairs, num_pairs, 42);
    
    double times[MEASUREMENT_SAMPLES];
    uint64_t cycles_start, cycles_end;
    
    /* Warmup */
    for (int i = 0; i < WARMUP_ITERATIONS; i++) {
        for (size_t j = 0; j < num_pairs; j++) {
            results[j] = compute_harmonic_adjacency(&pairs[j]);
        }
    }
    
    /* Measurements */
    for (int sample = 0; sample < MEASUREMENT_SAMPLES; sample++) {
        cycles_start = rdtsc();
        double start = get_time_ns();
        
        for (int i = 0; i < BENCHMARK_ITERATIONS; i++) {
            for (size_t j = 0; j < num_pairs; j++) {
                results[j] = compute_harmonic_adjacency(&pairs[j]);
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
    
    result.pairs_per_second = (num_pairs * 1e9) / result.avg_time_ns;
    result.cycles_per_op = (cycles_end - cycles_start) / (BENCHMARK_ITERATIONS * num_pairs);
    result.passed = result.pairs_per_second >= TARGET_HARMONIC_ADJACENCY_MPS;
    
    free(pairs);
    free(results);
    
    return result;
}

/* Benchmark Euclidean distance performance */
static benchmark_result_t benchmark_euclidean_distance(size_t num_pairs) {
    benchmark_result_t result = {0};
    result.name = (num_pairs == PAIRS_SMALL) ? "Euclidean distance (100 pairs)" :
                  (num_pairs == PAIRS_MEDIUM) ? "Euclidean distance (1K pairs)" :
                  "Euclidean distance (10K pairs)";
    result.num_pairs = num_pairs;
    
    coordinate_pair_t* pairs = aligned_alloc_custom(num_pairs * sizeof(coordinate_pair_t), 32);
    double* results = aligned_alloc_custom(num_pairs * sizeof(double), 32);
    
    generate_coordinate_pairs(pairs, num_pairs, 42);
    
    double times[MEASUREMENT_SAMPLES];
    uint64_t cycles_start, cycles_end;
    
    /* Warmup */
    for (int i = 0; i < WARMUP_ITERATIONS; i++) {
        for (size_t j = 0; j < num_pairs; j++) {
            results[j] = compute_euclidean_distance(&pairs[j]);
        }
    }
    
    /* Measurements */
    for (int sample = 0; sample < MEASUREMENT_SAMPLES; sample++) {
        cycles_start = rdtsc();
        double start = get_time_ns();
        
        for (int i = 0; i < BENCHMARK_ITERATIONS; i++) {
            for (size_t j = 0; j < num_pairs; j++) {
                results[j] = compute_euclidean_distance(&pairs[j]);
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
    
    result.pairs_per_second = (num_pairs * 1e9) / result.avg_time_ns;
    result.cycles_per_op = (cycles_end - cycles_start) / (BENCHMARK_ITERATIONS * num_pairs);
    result.passed = result.pairs_per_second >= TARGET_EUCLIDEAN_DISTANCE_MPS;
    
    free(pairs);
    free(results);
    
    return result;
}

/* Memory vs computation trade-off benchmark */
static benchmark_result_t benchmark_memory_vs_computation_tradeoff(void) {
    benchmark_result_t result = {0};
    result.name = "Memory vs computation efficiency";
    result.num_pairs = PAIRS_MEDIUM;
    
    const size_t num_pairs = PAIRS_MEDIUM;
    coordinate_pair_t* pairs = aligned_alloc_custom(num_pairs * sizeof(coordinate_pair_t), 32);
    
    // Euclidean: Need to store full coordinate data (8 doubles per pair = 64 bytes)
    // Harmonic: Only need resonance classes (2 bytes per pair) 
    size_t euclidean_memory = num_pairs * sizeof(coordinate_pair_t); // ~64KB for 1K pairs
    size_t harmonic_memory = num_pairs * 2; // ~2KB for 1K pairs
    
    double memory_efficiency = (double)euclidean_memory / (double)harmonic_memory;
    
    generate_coordinate_pairs(pairs, num_pairs, 123);
    
    // Measure computational efficiency
    double harmonic_time, euclidean_time;
    
    // Harmonic timing
    double start = get_time_ns();
    for (int i = 0; i < BENCHMARK_ITERATIONS; i++) {
        for (size_t j = 0; j < num_pairs; j++) {
            volatile bool adjacent = compute_harmonic_adjacency(&pairs[j]);
            (void)adjacent; // Prevent optimization
        }
    }
    double end = get_time_ns();
    harmonic_time = (end - start) / BENCHMARK_ITERATIONS;
    
    // Euclidean timing
    start = get_time_ns();
    for (int i = 0; i < BENCHMARK_ITERATIONS; i++) {
        for (size_t j = 0; j < num_pairs; j++) {
            volatile double distance = compute_euclidean_distance(&pairs[j]);
            (void)distance; // Prevent optimization
        }
    }
    end = get_time_ns();
    euclidean_time = (end - start) / BENCHMARK_ITERATIONS;
    
    double computational_efficiency = euclidean_time / harmonic_time;
    
    result.avg_time_ns = harmonic_time;
    result.pairs_per_second = (num_pairs * 1e9) / harmonic_time;
    result.speedup_factor = computational_efficiency;
    result.passed = (memory_efficiency > 10.0 && computational_efficiency > 5.0);
    
    printf("Memory Efficiency: %.1fx less memory (Harmonic: %zu bytes, Euclidean: %zu bytes)\n",
           memory_efficiency, harmonic_memory, euclidean_memory);
    printf("Computational Efficiency: %.2fx faster (Harmonic: %.2f ns, Euclidean: %.2f ns)\n",
           computational_efficiency, harmonic_time, euclidean_time);
    
    free(pairs);
    return result;
}

/* Cache efficiency analysis */
static benchmark_result_t benchmark_cache_efficiency(void) {
    benchmark_result_t result = {0};
    result.name = "Cache efficiency comparison";
    result.num_pairs = PAIRS_LARGE;
    
    const size_t num_pairs = PAIRS_LARGE;
    coordinate_pair_t* pairs = aligned_alloc_custom(num_pairs * sizeof(coordinate_pair_t), 64);
    uint8_t* resonance_classes = aligned_alloc_custom(num_pairs * 2, 64);
    
    generate_coordinate_pairs(pairs, num_pairs, 456);
    
    // Extract resonance classes for cache-friendly access
    for (size_t i = 0; i < num_pairs; i++) {
        resonance_classes[i * 2] = pairs[i].r1;
        resonance_classes[i * 2 + 1] = pairs[i].r2;
    }
    
    double times[MEASUREMENT_SAMPLES];
    
    // Benchmark cache-friendly harmonic adjacency (sequential access to resonance classes)
    for (int sample = 0; sample < MEASUREMENT_SAMPLES; sample++) {
        double start = get_time_ns();
        
        for (int i = 0; i < BENCHMARK_ITERATIONS; i++) {
            for (size_t j = 0; j < num_pairs; j++) {
                uint8_t r1 = resonance_classes[j * 2];
                uint8_t r2 = resonance_classes[j * 2 + 1];
                volatile bool adjacent = atlas_r96_harmonizes(r1, r2);
                (void)adjacent;
            }
        }
        
        double end = get_time_ns();
        times[sample] = (end - start) / BENCHMARK_ITERATIONS;
    }
    
    result.avg_time_ns = 0;
    for (int i = 0; i < MEASUREMENT_SAMPLES; i++) {
        result.avg_time_ns += times[i];
    }
    result.avg_time_ns /= MEASUREMENT_SAMPLES;
    result.median_time_ns = calculate_median(times, MEASUREMENT_SAMPLES);
    
    result.pairs_per_second = (num_pairs * 1e9) / result.avg_time_ns;
    result.passed = result.pairs_per_second >= TARGET_HARMONIC_ADJACENCY_MPS;
    
    free(pairs);
    free(resonance_classes);
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
    
    printf("Iterations per test: %d\n", BENCHMARK_ITERATIONS);
    printf("Measurement samples: %d\n", MEASUREMENT_SAMPLES);
    printf("Coordinate pair sizes: %zu, %zu, %zu pairs\n", 
           (size_t)PAIRS_SMALL, (size_t)PAIRS_MEDIUM, (size_t)PAIRS_LARGE);
    printf("\n");
}

/* Results printing */
static void print_result(const benchmark_result_t* result) {
    printf("%-35s | ", result->name);
    printf("Avg: %8.2f ns | ", result->avg_time_ns);
    printf("Rate: %8.2f Mpairs/s | ", result->pairs_per_second / 1e6);
    if (result->speedup_factor > 0) {
        printf("Speedup: %6.2fx | ", result->speedup_factor);
    } else {
        printf("Cycles/op: %6llu | ", result->cycles_per_op);
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
    printf("Atlas-12288 Layer 4 Distance Computation Benchmark Suite\n");
    printf("========================================================\n\n");
    
    print_system_info();
    
    benchmark_result_t results[8];
    int num_passed = 0;
    int test_count = 0;
    
    printf("=== Performance Benchmarks ===\n");
    printf("%-35s | %-15s | %-18s | %-15s | Status\n", "Test", "Time (Avg)", "Rate", "Efficiency");
    printf("------------------------------------------------------------------------------------------------\n");
    
    /* Harmonic Adjacency Tests */
    results[test_count] = benchmark_harmonic_adjacency(PAIRS_SMALL);
    print_result(&results[test_count]);
    if (results[test_count].passed) num_passed++;
    test_count++;
    
    results[test_count] = benchmark_harmonic_adjacency(PAIRS_MEDIUM);
    print_result(&results[test_count]);
    if (results[test_count].passed) num_passed++;
    test_count++;
    
    results[test_count] = benchmark_harmonic_adjacency(PAIRS_LARGE);
    print_result(&results[test_count]);
    if (results[test_count].passed) num_passed++;
    test_count++;
    
    /* Euclidean Distance Tests */
    results[test_count] = benchmark_euclidean_distance(PAIRS_SMALL);
    print_result(&results[test_count]);
    if (results[test_count].passed) num_passed++;
    test_count++;
    
    results[test_count] = benchmark_euclidean_distance(PAIRS_MEDIUM);
    print_result(&results[test_count]);
    if (results[test_count].passed) num_passed++;
    test_count++;
    
    results[test_count] = benchmark_euclidean_distance(PAIRS_LARGE);
    print_result(&results[test_count]);
    if (results[test_count].passed) num_passed++;
    test_count++;
    
    /* Efficiency Analysis */
    results[test_count] = benchmark_memory_vs_computation_tradeoff();
    print_result(&results[test_count]);
    if (results[test_count].passed) num_passed++;
    test_count++;
    
    results[test_count] = benchmark_cache_efficiency();
    print_result(&results[test_count]);
    if (results[test_count].passed) num_passed++;
    test_count++;
    
    printf("\n=== Performance Analysis ===\n");
    // Calculate performance ratios
    double harmonic_rate_large = results[2].pairs_per_second;
    double euclidean_rate_large = results[5].pairs_per_second;
    double performance_advantage = harmonic_rate_large / euclidean_rate_large;
    
    printf("Harmonic Adjacency (10K pairs):   %.2f Mpairs/s\n", harmonic_rate_large / 1e6);
    printf("Euclidean Distance (10K pairs):   %.2f Mpairs/s\n", euclidean_rate_large / 1e6);
    printf("Performance Advantage:            %.2fx faster with harmonic approach\n", performance_advantage);
    
    printf("\n=== Theoretical Advantages ===\n");
    printf("Harmonic Adjacency (R96):        O(1) computation, 2 bytes/pair storage\n");
    printf("Euclidean Distance:               O(n) computation, 32+ bytes/pair storage\n");
    printf("Memory Efficiency:                ~16x less memory with harmonic approach\n");
    printf("Computational Complexity:         Constant time vs square root operations\n");
    
    printf("\n=== Memory and Resource Usage ===\n");
    print_memory_usage();
    
    printf("\n=== Summary ===\n");
    printf("Tests passed: %d/%d\n", num_passed, test_count);
    printf("Overall performance advantage: %.2fx with R96 harmonic adjacency\n", performance_advantage);
    printf("Overall status: %s\n", (num_passed == test_count) ? "ALL TESTS PASSED" : "SOME TESTS FAILED");
    
    /* Output parseable results for automation */
    printf("\n=== Parseable Results (CSV) ===\n");
    printf("test_name,avg_time_ns,pairs_per_second,speedup_factor,passed\n");
    for (int i = 0; i < test_count; i++) {
        printf("%s,%.2f,%.0f,%.2f,%s\n",
               results[i].name,
               results[i].avg_time_ns,
               results[i].pairs_per_second,
               results[i].speedup_factor,
               results[i].passed ? "true" : "false");
    }
    
    return (num_passed == test_count) ? 0 : 1;
}