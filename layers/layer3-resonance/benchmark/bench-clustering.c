/* bench-clustering.c - Layer 3 Clustering Performance Benchmark Suite
 * (c) 2024-2025 UOR Foundation - MIT License
 * 
 * Comprehensive benchmarks for Atlas-12288 Layer 3 clustering operations:
 * - CSR matrix construction performance
 * - Clustering throughput for different page counts
 * - Memory usage and allocation efficiency
 * - Resonance class distribution analysis
 * - Harmonic analysis performance
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

#include "../include/atlas-resonance.h"

/* Benchmark configuration */
#define ATLAS_PAGE_SIZE 256
#define BENCHMARK_ITERATIONS 1000
#define WARMUP_ITERATIONS 100
#define MEASUREMENT_SAMPLES 10

/* Page count test sizes */
#define PAGES_SMALL  48       // ~12KB
#define PAGES_MEDIUM 1024     // ~256KB  
#define PAGES_LARGE  16384    // ~4MB
#define PAGES_XLARGE 65536    // ~16MB

/* Performance targets */
#define TARGET_CSR_CONSTRUCTION_PAGES_PER_SEC 50000   // Target ≥50k pages/sec for CSR construction
#define TARGET_CLUSTER_ACCESS_NS_PER_CLASS 1000       // Target <1µs per class access
#define TARGET_HARMONIC_ANALYSIS_OPS_PER_SEC 1000000  // Target ≥1M ops/sec for harmonic analysis
#define TARGET_MEMORY_EFFICIENCY_RATIO 0.8             // Target ≥80% memory efficiency

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

/* Data generation utilities */
static void generate_clusterable_data(uint8_t* data, size_t pages, uint32_t seed) {
    srand(seed);
    
    for (size_t page = 0; page < pages; page++) {
        uint8_t* page_ptr = data + (page * ATLAS_PAGE_SIZE);
        
        // Create different clustering patterns based on page index
        switch (page % 5) {
            case 0: {
                // Concentrated pattern - mostly class 0-15
                uint8_t base = rand() % 16;
                for (size_t i = 0; i < ATLAS_PAGE_SIZE; i++) {
                    page_ptr[i] = (base + (rand() % 8)) % 96;
                }
                break;
            }
            case 1: {
                // Sparse pattern - distributed across all classes
                for (size_t i = 0; i < ATLAS_PAGE_SIZE; i++) {
                    page_ptr[i] = rand() % 256;
                }
                break;
            }
            case 2: {
                // Harmonic pattern - pairs of harmonizing classes
                uint8_t r1 = rand() % 48;  // First half
                uint8_t r2 = 96 - r1;      // Harmonic conjugate
                for (size_t i = 0; i < ATLAS_PAGE_SIZE; i++) {
                    page_ptr[i] = (i % 2 == 0) ? r1 : r2;
                }
                break;
            }
            case 3: {
                // Sequential pattern - gradual progression
                uint8_t start = (page * 7) % 96;
                for (size_t i = 0; i < ATLAS_PAGE_SIZE; i++) {
                    page_ptr[i] = (start + i) % 96;
                }
                break;
            }
            case 4: {
                // Block pattern - chunks of same class
                uint8_t chunk_size = 16;
                for (size_t i = 0; i < ATLAS_PAGE_SIZE; i++) {
                    uint8_t class = ((i / chunk_size) + page) % 96;
                    page_ptr[i] = class;
                }
                break;
            }
        }
    }
}

/* Benchmark result structure */
typedef struct {
    const char* name;
    double min_time_ns;
    double max_time_ns;
    double avg_time_ns;
    double median_time_ns;
    double construction_time_ns;
    double access_time_ns;
    double ops_per_sec;
    size_t memory_used;
    size_t memory_theoretical;
    double memory_efficiency;
    bool passed;
    size_t page_count;
} clustering_result_t;

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

/* Memory usage tracking */
static size_t get_current_memory_usage(void) {
#ifdef __linux__
    struct rusage usage;
    if (getrusage(RUSAGE_SELF, &usage) == 0) {
        return usage.ru_maxrss * 1024;  // Convert KB to bytes
    }
#endif
    return 0;
}

/* Benchmark functions */

static clustering_result_t benchmark_csr_construction(size_t num_pages) {
    clustering_result_t result = {0};
    result.name = (num_pages == PAGES_SMALL) ? "CSR construction (48 pages)" :
                  (num_pages == PAGES_MEDIUM) ? "CSR construction (1K pages)" :
                  (num_pages == PAGES_LARGE) ? "CSR construction (16K pages)" :
                  "CSR construction (64K pages)";
    result.page_count = num_pages;
    
    size_t data_size = num_pages * ATLAS_PAGE_SIZE;
    uint8_t* data = aligned_alloc_custom(data_size, 32);
    generate_clusterable_data(data, num_pages, 42);
    
    double construction_times[MEASUREMENT_SAMPLES];
    size_t memory_before = get_current_memory_usage();
    
    /* Warmup */
    for (int i = 0; i < WARMUP_ITERATIONS; i++) {
        atlas_cluster_view cluster = atlas_cluster_by_resonance(data, num_pages);
        atlas_cluster_destroy(&cluster);
    }
    
    /* Measurements */
    for (int sample = 0; sample < MEASUREMENT_SAMPLES; sample++) {
        double start = get_time_ns();
        
        atlas_cluster_view clusters[BENCHMARK_ITERATIONS];
        for (int i = 0; i < BENCHMARK_ITERATIONS; i++) {
            clusters[i] = atlas_cluster_by_resonance(data, num_pages);
        }
        
        double end = get_time_ns();
        construction_times[sample] = (end - start) / BENCHMARK_ITERATIONS;
        
        /* Cleanup */
        for (int i = 0; i < BENCHMARK_ITERATIONS; i++) {
            atlas_cluster_destroy(&clusters[i]);
        }
    }
    
    size_t memory_after = get_current_memory_usage();
    
    /* Create one cluster to analyze memory usage */
    atlas_cluster_view test_cluster = atlas_cluster_by_resonance(data, num_pages);
    size_t theoretical_memory = num_pages * sizeof(uint32_t) + 97 * sizeof(uint32_t);
    
    /* Calculate statistics */
    result.min_time_ns = construction_times[0];
    result.max_time_ns = construction_times[0];
    result.avg_time_ns = 0;
    
    for (int i = 0; i < MEASUREMENT_SAMPLES; i++) {
        if (construction_times[i] < result.min_time_ns) result.min_time_ns = construction_times[i];
        if (construction_times[i] > result.max_time_ns) result.max_time_ns = construction_times[i];
        result.avg_time_ns += construction_times[i];
    }
    result.avg_time_ns /= MEASUREMENT_SAMPLES;
    result.median_time_ns = calculate_median(construction_times, MEASUREMENT_SAMPLES);
    result.construction_time_ns = result.avg_time_ns;
    
    result.ops_per_sec = (num_pages * 1e9) / result.avg_time_ns;
    result.memory_used = memory_after - memory_before;
    result.memory_theoretical = theoretical_memory;
    result.memory_efficiency = (double)theoretical_memory / (double)result.memory_used;
    result.passed = result.ops_per_sec >= TARGET_CSR_CONSTRUCTION_PAGES_PER_SEC;
    
    atlas_cluster_destroy(&test_cluster);
    free(data);
    
    return result;
}

static clustering_result_t benchmark_cluster_access_patterns(size_t num_pages) {
    clustering_result_t result = {0};
    result.name = (num_pages == PAGES_MEDIUM) ? "Cluster access (1K pages)" :
                  "Cluster access (16K pages)";
    result.page_count = num_pages;
    
    size_t data_size = num_pages * ATLAS_PAGE_SIZE;
    uint8_t* data = aligned_alloc_custom(data_size, 32);
    generate_clusterable_data(data, num_pages, 123);
    
    /* Create cluster once */
    atlas_cluster_view cluster = atlas_cluster_by_resonance(data, num_pages);
    
    double access_times[MEASUREMENT_SAMPLES];
    
    /* Warmup */
    for (int i = 0; i < WARMUP_ITERATIONS; i++) {
        for (uint8_t r = 0; r < 96; r++) {
            size_t count;
            const uint32_t* pages = atlas_cluster_pages_for_resonance(cluster, r, &count);
            (void)pages; (void)count;  // Suppress unused variable warnings
        }
    }
    
    /* Measurements */
    for (int sample = 0; sample < MEASUREMENT_SAMPLES; sample++) {
        double start = get_time_ns();
        
        for (int iter = 0; iter < BENCHMARK_ITERATIONS; iter++) {
            for (uint8_t r = 0; r < 96; r++) {
                size_t count = atlas_cluster_count_for_resonance(cluster, r);
                const uint32_t* pages = atlas_cluster_pages_for_resonance(cluster, r, &count);
                
                /* Simulate accessing first few pages in each class */
                for (size_t i = 0; i < count && i < 10; i++) {
                    volatile uint32_t page_idx = pages[i];  // Force memory access
                    (void)page_idx;
                }
            }
        }
        
        double end = get_time_ns();
        access_times[sample] = (end - start) / (BENCHMARK_ITERATIONS * 96);
    }
    
    /* Calculate statistics */
    result.min_time_ns = access_times[0];
    result.max_time_ns = access_times[0];
    result.avg_time_ns = 0;
    
    for (int i = 0; i < MEASUREMENT_SAMPLES; i++) {
        if (access_times[i] < result.min_time_ns) result.min_time_ns = access_times[i];
        if (access_times[i] > result.max_time_ns) result.max_time_ns = access_times[i];
        result.avg_time_ns += access_times[i];
    }
    result.avg_time_ns /= MEASUREMENT_SAMPLES;
    result.median_time_ns = calculate_median(access_times, MEASUREMENT_SAMPLES);
    result.access_time_ns = result.avg_time_ns;
    
    result.passed = result.avg_time_ns < TARGET_CLUSTER_ACCESS_NS_PER_CLASS;
    
    atlas_cluster_destroy(&cluster);
    free(data);
    
    return result;
}

static clustering_result_t benchmark_harmonic_analysis(void) {
    clustering_result_t result = {0};
    result.name = "Harmonic analysis";
    
    double times[MEASUREMENT_SAMPLES];
    
    /* Warmup */
    for (int i = 0; i < WARMUP_ITERATIONS; i++) {
        for (uint8_t r1 = 0; r1 < 96; r1++) {
            for (uint8_t r2 = 0; r2 < 96; r2++) {
                bool harmonizes = atlas_r96_harmonizes(r1, r2);
                (void)harmonizes;
            }
        }
    }
    
    /* Measurements */
    for (int sample = 0; sample < MEASUREMENT_SAMPLES; sample++) {
        double start = get_time_ns();
        
        for (int iter = 0; iter < BENCHMARK_ITERATIONS; iter++) {
            for (uint8_t r1 = 0; r1 < 96; r1++) {
                for (uint8_t r2 = 0; r2 < 96; r2++) {
                    bool harmonizes = atlas_r96_harmonizes(r1, r2);
                    (void)harmonizes;
                }
            }
        }
        
        double end = get_time_ns();
        times[sample] = (end - start) / (BENCHMARK_ITERATIONS * 96 * 96);
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
    
    result.ops_per_sec = 1e9 / result.avg_time_ns;
    result.passed = result.ops_per_sec >= TARGET_HARMONIC_ANALYSIS_OPS_PER_SEC;
    
    return result;
}

static clustering_result_t benchmark_cluster_distribution_analysis(size_t num_pages) {
    clustering_result_t result = {0};
    result.name = (num_pages == PAGES_MEDIUM) ? "Distribution analysis (1K)" :
                  "Distribution analysis (16K)";
    result.page_count = num_pages;
    
    size_t data_size = num_pages * ATLAS_PAGE_SIZE;
    uint8_t* data = aligned_alloc_custom(data_size, 32);
    generate_clusterable_data(data, num_pages, 456);
    
    atlas_cluster_view cluster = atlas_cluster_by_resonance(data, num_pages);
    
    double analysis_times[MEASUREMENT_SAMPLES];
    
    /* Measurements */
    for (int sample = 0; sample < MEASUREMENT_SAMPLES; sample++) {
        double start = get_time_ns();
        
        for (int iter = 0; iter < BENCHMARK_ITERATIONS; iter++) {
            /* Analyze cluster distribution */
            size_t total_pages, non_empty_classes, largest_class;
            atlas_cluster_stats(cluster, &total_pages, &non_empty_classes, &largest_class);
            
            /* Calculate homogeneity metrics */
            double homogeneity = 0.0;
            size_t total_entries = 0;
            
            for (uint8_t r = 0; r < 96; r++) {
                size_t class_size = atlas_cluster_count_for_resonance(cluster, r);
                if (class_size > 0) {
                    double proportion = (double)class_size / (double)total_pages;
                    homogeneity -= proportion * log2(proportion);
                    total_entries += class_size;
                }
            }
            
            /* Validate clustering integrity */
            bool valid = atlas_cluster_validate(cluster);
            (void)valid; (void)homogeneity; (void)total_entries;
        }
        
        double end = get_time_ns();
        analysis_times[sample] = (end - start) / BENCHMARK_ITERATIONS;
    }
    
    /* Calculate statistics */
    result.min_time_ns = analysis_times[0];
    result.max_time_ns = analysis_times[0];
    result.avg_time_ns = 0;
    
    for (int i = 0; i < MEASUREMENT_SAMPLES; i++) {
        if (analysis_times[i] < result.min_time_ns) result.min_time_ns = analysis_times[i];
        if (analysis_times[i] > result.max_time_ns) result.max_time_ns = analysis_times[i];
        result.avg_time_ns += analysis_times[i];
    }
    result.avg_time_ns /= MEASUREMENT_SAMPLES;
    result.median_time_ns = calculate_median(analysis_times, MEASUREMENT_SAMPLES);
    
    result.ops_per_sec = 1e9 / result.avg_time_ns;
    result.passed = result.avg_time_ns < 1000000;  // Target: <1ms per analysis
    
    atlas_cluster_destroy(&cluster);
    free(data);
    
    return result;
}

static clustering_result_t benchmark_memory_efficiency(size_t num_pages) {
    clustering_result_t result = {0};
    result.name = (num_pages == PAGES_LARGE) ? "Memory efficiency (16K)" :
                  "Memory efficiency (64K)";
    result.page_count = num_pages;
    
    size_t data_size = num_pages * ATLAS_PAGE_SIZE;
    uint8_t* data = aligned_alloc_custom(data_size, 32);
    generate_clusterable_data(data, num_pages, 789);
    
    size_t memory_before = get_current_memory_usage();
    
    /* Create multiple clusters to stress memory allocation */
    const int num_clusters = 10;
    atlas_cluster_view clusters[num_clusters];
    
    double start = get_time_ns();
    for (int i = 0; i < num_clusters; i++) {
        clusters[i] = atlas_cluster_by_resonance(data, num_pages);
    }
    double end = get_time_ns();
    
    size_t memory_after = get_current_memory_usage();
    size_t memory_per_cluster = (memory_after - memory_before) / num_clusters;
    size_t theoretical_memory = (num_pages + 97) * sizeof(uint32_t);
    
    result.avg_time_ns = (end - start) / num_clusters;
    result.construction_time_ns = result.avg_time_ns;
    result.memory_used = memory_per_cluster;
    result.memory_theoretical = theoretical_memory;
    result.memory_efficiency = (double)theoretical_memory / (double)memory_per_cluster;
    result.ops_per_sec = (num_pages * 1e9) / result.avg_time_ns;
    result.passed = result.memory_efficiency >= TARGET_MEMORY_EFFICIENCY_RATIO;
    
    /* Cleanup */
    for (int i = 0; i < num_clusters; i++) {
        atlas_cluster_destroy(&clusters[i]);
    }
    free(data);
    
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
    
    FILE* meminfo = fopen("/proc/meminfo", "r");
    if (meminfo) {
        char line[256];
        while (fgets(line, sizeof(line), meminfo)) {
            if (strncmp(line, "MemTotal:", 9) == 0) {
                printf("Memory: %s", strchr(line, ':') + 1);
                break;
            }
        }
        fclose(meminfo);
    }
#endif
    
    printf("Page Size: %d bytes\n", ATLAS_PAGE_SIZE);
    printf("Iterations per test: %d\n", BENCHMARK_ITERATIONS);
    printf("Measurement samples: %d\n", MEASUREMENT_SAMPLES);
    printf("\n");
}

/* Results printing */
static void print_result(const clustering_result_t* result) {
    printf("%-25s | ", result->name);
    printf("Avg: %8.2f ns | ", result->avg_time_ns);
    printf("Med: %8.2f ns | ", result->median_time_ns);
    
    if (result->ops_per_sec > 1000) {
        printf("Rate: %10.0f ops/s | ", result->ops_per_sec);
    } else {
        printf("Time: %10.2f µs | ", result->avg_time_ns / 1000.0);
    }
    
    if (result->memory_efficiency > 0) {
        printf("Mem: %6.1f%% | ", result->memory_efficiency * 100);
    } else {
        printf("             | ");
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

/* Clustering quality analysis */
static void analyze_clustering_quality(void) {
    printf("\n=== Clustering Quality Analysis ===\n");
    
    const size_t test_pages = 1024;
    size_t data_size = test_pages * ATLAS_PAGE_SIZE;
    uint8_t* data = aligned_alloc_custom(data_size, 32);
    generate_clusterable_data(data, test_pages, 999);
    
    atlas_cluster_view cluster = atlas_cluster_by_resonance(data, test_pages);
    
    size_t total_pages, non_empty_classes, largest_class;
    atlas_cluster_stats(cluster, &total_pages, &non_empty_classes, &largest_class);
    
    printf("Total pages clustered: %zu\n", total_pages);
    printf("Non-empty resonance classes: %zu/96\n", non_empty_classes);
    printf("Largest class size: %zu pages (%.1f%%)\n", 
           largest_class, (double)largest_class / total_pages * 100);
    
    /* Calculate distribution statistics */
    double shannon_entropy = 0.0;
    size_t min_class_size = SIZE_MAX;
    size_t max_class_size = 0;
    
    for (uint8_t r = 0; r < 96; r++) {
        size_t class_size = atlas_cluster_count_for_resonance(cluster, r);
        if (class_size > 0) {
            double proportion = (double)class_size / (double)total_pages;
            shannon_entropy -= proportion * log2(proportion);
            
            if (class_size < min_class_size) min_class_size = class_size;
            if (class_size > max_class_size) max_class_size = class_size;
        }
    }
    
    printf("Shannon entropy: %.3f bits\n", shannon_entropy);
    printf("Class size range: %zu - %zu pages\n", 
           min_class_size == SIZE_MAX ? 0 : min_class_size, max_class_size);
    
    /* Test harmonic pairs in clustering */
    size_t harmonic_pairs_found = 0;
    for (uint8_t r1 = 0; r1 < 48; r1++) {
        uint8_t r2 = atlas_r96_harmonic_conjugate(r1);
        size_t count1 = atlas_cluster_count_for_resonance(cluster, r1);
        size_t count2 = atlas_cluster_count_for_resonance(cluster, r2);
        
        if (count1 > 0 && count2 > 0) {
            harmonic_pairs_found++;
        }
    }
    printf("Harmonic pairs present: %zu/48\n", harmonic_pairs_found);
    
    atlas_cluster_destroy(&cluster);
    free(data);
}

int main(void) {
    printf("Atlas-12288 Layer 3 Clustering Benchmark Suite\n");
    printf("===============================================\n\n");
    
    print_system_info();
    
    clustering_result_t results[8];
    int num_passed = 0;
    int test_count = 0;
    
    printf("=== Performance Benchmarks ===\n");
    printf("%-25s | %-15s | %-15s | %-16s | %-12s | Status\n", 
           "Test", "Time (Avg)", "Time (Med)", "Rate/Time", "Memory Eff");
    printf("-------------------------------------------------------------------------------------------\n");
    
    /* CSR Construction Tests */
    results[test_count] = benchmark_csr_construction(PAGES_SMALL);
    print_result(&results[test_count]);
    if (results[test_count].passed) num_passed++;
    test_count++;
    
    results[test_count] = benchmark_csr_construction(PAGES_MEDIUM);
    print_result(&results[test_count]);
    if (results[test_count].passed) num_passed++;
    test_count++;
    
    results[test_count] = benchmark_csr_construction(PAGES_LARGE);
    print_result(&results[test_count]);
    if (results[test_count].passed) num_passed++;
    test_count++;
    
    results[test_count] = benchmark_csr_construction(PAGES_XLARGE);
    print_result(&results[test_count]);
    if (results[test_count].passed) num_passed++;
    test_count++;
    
    /* Cluster Access Tests */
    results[test_count] = benchmark_cluster_access_patterns(PAGES_MEDIUM);
    print_result(&results[test_count]);
    if (results[test_count].passed) num_passed++;
    test_count++;
    
    results[test_count] = benchmark_cluster_access_patterns(PAGES_LARGE);
    print_result(&results[test_count]);
    if (results[test_count].passed) num_passed++;
    test_count++;
    
    /* Harmonic Analysis Test */
    results[test_count] = benchmark_harmonic_analysis();
    print_result(&results[test_count]);
    if (results[test_count].passed) num_passed++;
    test_count++;
    
    /* Distribution Analysis Test */
    results[test_count] = benchmark_cluster_distribution_analysis(PAGES_MEDIUM);
    print_result(&results[test_count]);
    if (results[test_count].passed) num_passed++;
    test_count++;
    
    /* Memory Efficiency Tests */
    printf("\n=== Memory Efficiency Analysis ===\n");
    clustering_result_t mem_result1 = benchmark_memory_efficiency(PAGES_LARGE);
    clustering_result_t mem_result2 = benchmark_memory_efficiency(PAGES_XLARGE);
    
    printf("%-25s | Theoretical: %8zu B | Actual: %8zu B | Efficiency: %5.1f%%\n",
           mem_result1.name, mem_result1.memory_theoretical, 
           mem_result1.memory_used, mem_result1.memory_efficiency * 100);
    printf("%-25s | Theoretical: %8zu B | Actual: %8zu B | Efficiency: %5.1f%%\n",
           mem_result2.name, mem_result2.memory_theoretical, 
           mem_result2.memory_used, mem_result2.memory_efficiency * 100);
    
    if (mem_result1.passed) num_passed++;
    if (mem_result2.passed) num_passed++;
    test_count += 2;
    
    printf("\n=== Performance Targets vs Results ===\n");
    printf("CSR Construction:        Target ≥%.0f pages/s, Best: %.0f pages/s (64K pages)\n", 
           TARGET_CSR_CONSTRUCTION_PAGES_PER_SEC, results[3].ops_per_sec);
    printf("Cluster Access:          Target <%.0f ns/class, Best: %.2f ns/class\n", 
           TARGET_CLUSTER_ACCESS_NS_PER_CLASS, results[4].avg_time_ns);
    printf("Harmonic Analysis:       Target ≥%.0f ops/s, Achieved: %.0f ops/s\n", 
           TARGET_HARMONIC_ANALYSIS_OPS_PER_SEC, results[6].ops_per_sec);
    printf("Memory Efficiency:       Target ≥%.0f%%, Best: %.1f%% (64K pages)\n", 
           TARGET_MEMORY_EFFICIENCY_RATIO * 100, mem_result2.memory_efficiency * 100);
    
    analyze_clustering_quality();
    
    printf("\n=== Memory and Resource Usage ===\n");
    print_memory_usage();
    
    printf("\n=== Summary ===\n");
    printf("Tests passed: %d/%d\n", num_passed, test_count);
    printf("Overall status: %s\n", (num_passed == test_count) ? "ALL TESTS PASSED" : "SOME TESTS FAILED");
    
    /* Output parseable results for automation */
    printf("\n=== Parseable Results (CSV) ===\n");
    printf("test_name,avg_time_ns,ops_per_sec,memory_efficiency,passed,page_count\n");
    for (int i = 0; i < test_count - 2; i++) {  // Exclude memory tests from main results
        printf("%s,%.2f,%.0f,%.3f,%s,%zu\n",
               results[i].name,
               results[i].avg_time_ns,
               results[i].ops_per_sec,
               results[i].memory_efficiency,
               results[i].passed ? "true" : "false",
               results[i].page_count);
    }
    printf("%s,%.2f,%.0f,%.3f,%s,%zu\n",
           mem_result1.name, mem_result1.construction_time_ns, mem_result1.ops_per_sec,
           mem_result1.memory_efficiency, mem_result1.passed ? "true" : "false", mem_result1.page_count);
    printf("%s,%.2f,%.0f,%.3f,%s,%zu\n",
           mem_result2.name, mem_result2.construction_time_ns, mem_result2.ops_per_sec,
           mem_result2.memory_efficiency, mem_result2.passed ? "true" : "false", mem_result2.page_count);
    
    return (num_passed == test_count) ? 0 : 1;
}