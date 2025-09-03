/* bench-classification.c - Layer 3 Classification Performance Benchmark Suite
 * (c) 2024-2025 UOR Foundation - MIT License
 * 
 * Comprehensive benchmarks for Atlas-12288 Layer 3 classification operations:
 * - R96 classification throughput (scalar vs SIMD)
 * - Histogram generation performance
 * - Batch processing efficiency
 * - Memory bandwidth utilization
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
#define BENCHMARK_ITERATIONS 10000
#define WARMUP_ITERATIONS 1000
#define MEASUREMENT_SAMPLES 10

/* Data sizes for throughput testing */
#define DATA_SIZE_SMALL  256      // 256B (1 page)
#define DATA_SIZE_MEDIUM 12288    // 12KB (48 pages) 
#define DATA_SIZE_LARGE  1048576  // 1MB (4096 pages)

/* Performance targets */
#define TARGET_R96_CLASSIFY_GB_S 8.0      // Target ≥8 GB/s for classification
#define TARGET_HISTOGRAM_MB_S 500.0       // Target ≥500 MB/s for histogram generation
#define TARGET_BATCH_PAGES_PER_SEC 100000 // Target ≥100k pages/sec for batch processing

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
static void generate_test_data(uint8_t* data, size_t size, uint32_t seed) {
    srand(seed);
    for (size_t i = 0; i < size; i++) {
        // Generate data with good spread across R96 classes
        data[i] = (uint8_t)(rand() % 256);
    }
}

static void generate_page_sequence(uint8_t* data, size_t pages) {
    for (size_t page = 0; page < pages; page++) {
        uint8_t* page_ptr = data + (page * ATLAS_PAGE_SIZE);
        
        // Create pages with varying resonance class distributions
        uint8_t base_class = (page * 7) % 96;  // Rotating base class
        for (size_t i = 0; i < ATLAS_PAGE_SIZE; i++) {
            page_ptr[i] = (base_class + (i * 3)) % 256;
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
    double throughput_gb_s;
    double throughput_mb_s;
    double ops_per_sec;
    uint64_t cycles_per_op;
    bool passed;
    size_t data_size;
} benchmark_result_t;

/* Scalar reference implementation for comparison */
static void r96_classify_scalar(const uint8_t* input, atlas_resonance_t* output, size_t length) {
    for (size_t i = 0; i < length; i++) {
        output[i] = input[i] % 96;
    }
}

/* SIMD optimized implementation (if available) */
#ifdef __AVX2__
#include <immintrin.h>

static void r96_classify_avx2(const uint8_t* input, atlas_resonance_t* output, size_t length) {
    size_t avx2_chunks = length / 32;
    size_t remaining = length % 32;
    
    // Pre-compute mod-96 lookup table
    static uint8_t mod96_table[256];
    static bool table_initialized = false;
    if (!table_initialized) {
        for (int i = 0; i < 256; i++) {
            mod96_table[i] = i % 96;
        }
        table_initialized = true;
    }
    
    // Process 32 bytes at a time
    for (size_t i = 0; i < avx2_chunks; i++) {
        __m256i data = _mm256_loadu_si256((const __m256i*)(input + i * 32));
        
        // Use lookup table for mod operation
        uint8_t temp[32];
        _mm256_storeu_si256((__m256i*)temp, data);
        
        for (int j = 0; j < 32; j++) {
            output[i * 32 + j] = mod96_table[temp[j]];
        }
    }
    
    // Handle remaining bytes
    for (size_t i = avx2_chunks * 32; i < length; i++) {
        output[i] = input[i] % 96;
    }
}
#endif

/* CPU feature detection */
static bool has_avx2(void) {
    uint32_t eax, ebx, ecx, edx;
    __asm__ __volatile__("cpuid"
        : "=a" (eax), "=b" (ebx), "=c" (ecx), "=d" (edx)
        : "a" (7), "c" (0));
    return (ebx & (1 << 5)) != 0;
}

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

/* Benchmark functions */

static benchmark_result_t benchmark_r96_classify_throughput(size_t data_size) {
    benchmark_result_t result = {0};
    result.name = (data_size == DATA_SIZE_SMALL) ? "R96 classify (256B)" :
                  (data_size == DATA_SIZE_MEDIUM) ? "R96 classify (12KB)" :
                  "R96 classify (1MB)";
    result.data_size = data_size;
    
    uint8_t* input = aligned_alloc_custom(data_size, 32);
    atlas_resonance_t* output = aligned_alloc_custom(data_size, 32);
    
    generate_test_data(input, data_size, 42);
    
    double times[MEASUREMENT_SAMPLES];
    uint64_t cycles_start, cycles_end;
    
    /* Warmup */
    for (int i = 0; i < WARMUP_ITERATIONS; i++) {
        r96_classify_scalar(input, output, data_size);
    }
    
    /* Measurements */
    for (int sample = 0; sample < MEASUREMENT_SAMPLES; sample++) {
        cycles_start = rdtsc();
        double start = get_time_ns();
        
        for (int i = 0; i < BENCHMARK_ITERATIONS; i++) {
#ifdef __AVX2__
            if (has_avx2()) {
                r96_classify_avx2(input, output, data_size);
            } else {
                r96_classify_scalar(input, output, data_size);
            }
#else
            r96_classify_scalar(input, output, data_size);
#endif
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
    
    result.throughput_gb_s = (data_size * 1e9) / (result.avg_time_ns * 1e9);
    result.cycles_per_op = (cycles_end - cycles_start) / (BENCHMARK_ITERATIONS * data_size);
    result.passed = result.throughput_gb_s >= TARGET_R96_CLASSIFY_GB_S;
    
    free(input);
    free(output);
    
    return result;
}

static benchmark_result_t benchmark_histogram_generation(size_t num_pages) {
    benchmark_result_t result = {0};
    result.name = (num_pages == 1) ? "Histogram (1 page)" :
                  (num_pages == 48) ? "Histogram (48 pages)" :
                  "Histogram (4096 pages)";
    result.data_size = num_pages * ATLAS_PAGE_SIZE;
    
    size_t data_size = num_pages * ATLAS_PAGE_SIZE;
    uint8_t* input = aligned_alloc_custom(data_size, 32);
    uint16_t* histograms = aligned_alloc_custom(num_pages * 96 * sizeof(uint16_t), 32);
    
    generate_page_sequence(input, num_pages);
    
    double times[MEASUREMENT_SAMPLES];
    
    /* Warmup */
    for (int i = 0; i < WARMUP_ITERATIONS; i++) {
        atlas_r96_histogram_pages(input, num_pages, histograms);
    }
    
    /* Measurements */
    for (int sample = 0; sample < MEASUREMENT_SAMPLES; sample++) {
        double start = get_time_ns();
        
        for (int i = 0; i < BENCHMARK_ITERATIONS; i++) {
            atlas_r96_histogram_pages(input, num_pages, histograms);
        }
        
        double end = get_time_ns();
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
    
    result.throughput_mb_s = (data_size * 1e6) / (result.avg_time_ns * 1e6);
    result.ops_per_sec = (num_pages * 1e9) / result.avg_time_ns;
    result.passed = result.throughput_mb_s >= TARGET_HISTOGRAM_MB_S;
    
    free(input);
    free(histograms);
    
    return result;
}

static benchmark_result_t benchmark_page_classification_batch(size_t num_pages) {
    benchmark_result_t result = {0};
    result.name = (num_pages == 48) ? "Page classify (48 pages)" :
                  (num_pages == 4096) ? "Page classify (4096 pages)" :
                  "Page classify (single)";
    result.data_size = num_pages * ATLAS_PAGE_SIZE;
    
    size_t data_size = num_pages * ATLAS_PAGE_SIZE;
    uint8_t* input = aligned_alloc_custom(data_size, 32);
    uint8_t* classifications = aligned_alloc_custom(num_pages, 32);
    
    generate_page_sequence(input, num_pages);
    
    double times[MEASUREMENT_SAMPLES];
    
    /* Warmup */
    for (int i = 0; i < WARMUP_ITERATIONS; i++) {
        atlas_r96_classify_pages(input, num_pages, classifications);
    }
    
    /* Measurements */
    for (int sample = 0; sample < MEASUREMENT_SAMPLES; sample++) {
        double start = get_time_ns();
        
        for (int i = 0; i < BENCHMARK_ITERATIONS; i++) {
            atlas_r96_classify_pages(input, num_pages, classifications);
        }
        
        double end = get_time_ns();
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
    
    result.ops_per_sec = (num_pages * 1e9) / result.avg_time_ns;
    result.throughput_mb_s = (data_size * 1e6) / (result.avg_time_ns * 1e6);
    result.passed = result.ops_per_sec >= TARGET_BATCH_PAGES_PER_SEC;
    
    free(input);
    free(classifications);
    
    return result;
}

static benchmark_result_t benchmark_scalar_vs_simd_comparison(void) {
    benchmark_result_t result = {0};
    result.name = "Scalar vs SIMD speedup";
    result.data_size = DATA_SIZE_MEDIUM;
    
    uint8_t* input = aligned_alloc_custom(DATA_SIZE_MEDIUM, 32);
    atlas_resonance_t* output = aligned_alloc_custom(DATA_SIZE_MEDIUM, 32);
    
    generate_test_data(input, DATA_SIZE_MEDIUM, 123);
    
    double scalar_times[MEASUREMENT_SAMPLES];
    double simd_times[MEASUREMENT_SAMPLES];
    
    /* Warmup */
    for (int i = 0; i < WARMUP_ITERATIONS; i++) {
        r96_classify_scalar(input, output, DATA_SIZE_MEDIUM);
    }
    
    /* Measure scalar performance */
    for (int sample = 0; sample < MEASUREMENT_SAMPLES; sample++) {
        double start = get_time_ns();
        
        for (int i = 0; i < BENCHMARK_ITERATIONS; i++) {
            r96_classify_scalar(input, output, DATA_SIZE_MEDIUM);
        }
        
        double end = get_time_ns();
        scalar_times[sample] = (end - start) / BENCHMARK_ITERATIONS;
    }
    
#ifdef __AVX2__
    if (has_avx2()) {
        /* Warmup SIMD */
        for (int i = 0; i < WARMUP_ITERATIONS; i++) {
            r96_classify_avx2(input, output, DATA_SIZE_MEDIUM);
        }
        
        /* Measure SIMD performance */
        for (int sample = 0; sample < MEASUREMENT_SAMPLES; sample++) {
            double start = get_time_ns();
            
            for (int i = 0; i < BENCHMARK_ITERATIONS; i++) {
                r96_classify_avx2(input, output, DATA_SIZE_MEDIUM);
            }
            
            double end = get_time_ns();
            simd_times[sample] = (end - start) / BENCHMARK_ITERATIONS;
        }
    }
#endif
    
    /* Calculate scalar statistics */
    double scalar_avg = 0;
    for (int i = 0; i < MEASUREMENT_SAMPLES; i++) {
        scalar_avg += scalar_times[i];
    }
    scalar_avg /= MEASUREMENT_SAMPLES;
    
    result.avg_time_ns = scalar_avg;
    result.throughput_gb_s = (DATA_SIZE_MEDIUM * 1e9) / (scalar_avg * 1e9);
    
#ifdef __AVX2__
    if (has_avx2()) {
        double simd_avg = 0;
        for (int i = 0; i < MEASUREMENT_SAMPLES; i++) {
            simd_avg += simd_times[i];
        }
        simd_avg /= MEASUREMENT_SAMPLES;
        
        double speedup = scalar_avg / simd_avg;
        printf("SIMD Speedup: %.2fx (Scalar: %.2f GB/s, SIMD: %.2f GB/s)\n", 
               speedup,
               (DATA_SIZE_MEDIUM * 1e9) / (scalar_avg * 1e9),
               (DATA_SIZE_MEDIUM * 1e9) / (simd_avg * 1e9));
        
        result.passed = speedup >= 1.5;  // Expect at least 1.5x speedup
    } else {
        printf("AVX2 not available - using scalar implementation\n");
        result.passed = true;
    }
#else
    printf("SIMD not compiled - using scalar implementation\n");
    result.passed = true;
#endif
    
    free(input);
    free(output);
    
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
    
    printf("AVX2 Support: %s\n", has_avx2() ? "Yes" : "No");
    printf("Page Size: %d bytes\n", ATLAS_PAGE_SIZE);
    printf("Iterations per test: %d\n", BENCHMARK_ITERATIONS);
    printf("Measurement samples: %d\n", MEASUREMENT_SAMPLES);
    printf("\n");
}

/* Results printing */
static void print_result(const benchmark_result_t* result) {
    printf("%-25s | ", result->name);
    printf("Avg: %8.2f ns | ", result->avg_time_ns);
    printf("Med: %8.2f ns | ", result->median_time_ns);
    
    if (result->throughput_gb_s > 1.0) {
        printf("Throughput: %6.2f GB/s | ", result->throughput_gb_s);
    } else if (result->throughput_mb_s > 1.0) {
        printf("Throughput: %6.2f MB/s | ", result->throughput_mb_s);
    } else if (result->ops_per_sec > 1.0) {
        printf("Rate: %10.0f ops/s | ", result->ops_per_sec);
    } else {
        printf("Rate: %10.2f ns/op | ", result->avg_time_ns);
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
    printf("Atlas-12288 Layer 3 Classification Benchmark Suite\n");
    printf("==================================================\n\n");
    
    print_system_info();
    
    benchmark_result_t results[8];
    int num_passed = 0;
    int test_count = 0;
    
    printf("=== Performance Benchmarks ===\n");
    printf("%-25s | %-15s | %-15s | %-18s | Status\n", "Test", "Time (Avg)", "Time (Med)", "Throughput/Rate");
    printf("------------------------------------------------------------------------------------\n");
    
    /* R96 Classification Throughput Tests */
    results[test_count] = benchmark_r96_classify_throughput(DATA_SIZE_SMALL);
    print_result(&results[test_count]);
    if (results[test_count].passed) num_passed++;
    test_count++;
    
    results[test_count] = benchmark_r96_classify_throughput(DATA_SIZE_MEDIUM);
    print_result(&results[test_count]);
    if (results[test_count].passed) num_passed++;
    test_count++;
    
    results[test_count] = benchmark_r96_classify_throughput(DATA_SIZE_LARGE);
    print_result(&results[test_count]);
    if (results[test_count].passed) num_passed++;
    test_count++;
    
    /* Histogram Generation Tests */
    results[test_count] = benchmark_histogram_generation(1);
    print_result(&results[test_count]);
    if (results[test_count].passed) num_passed++;
    test_count++;
    
    results[test_count] = benchmark_histogram_generation(48);
    print_result(&results[test_count]);
    if (results[test_count].passed) num_passed++;
    test_count++;
    
    results[test_count] = benchmark_histogram_generation(4096);
    print_result(&results[test_count]);
    if (results[test_count].passed) num_passed++;
    test_count++;
    
    /* Page Classification Batch Tests */
    results[test_count] = benchmark_page_classification_batch(48);
    print_result(&results[test_count]);
    if (results[test_count].passed) num_passed++;
    test_count++;
    
    results[test_count] = benchmark_page_classification_batch(4096);
    print_result(&results[test_count]);
    if (results[test_count].passed) num_passed++;
    test_count++;
    
    printf("\n=== SIMD Analysis ===\n");
    benchmark_scalar_vs_simd_comparison();
    
    printf("\n=== Performance Targets vs Results ===\n");
    printf("R96 Classification:      Target ≥%.0f GB/s, Best: %.2f GB/s (1MB)\n", 
           TARGET_R96_CLASSIFY_GB_S, results[2].throughput_gb_s);
    printf("Histogram Generation:    Target ≥%.0f MB/s, Best: %.2f MB/s (4096 pages)\n", 
           TARGET_HISTOGRAM_MB_S, results[5].throughput_mb_s);
    printf("Batch Page Processing:   Target ≥%.0f pages/s, Best: %.0f pages/s\n", 
           TARGET_BATCH_PAGES_PER_SEC, results[7].ops_per_sec);
    
    printf("\n=== Memory and Resource Usage ===\n");
    print_memory_usage();
    
    printf("\n=== Summary ===\n");
    printf("Tests passed: %d/%d\n", num_passed, test_count);
    printf("Overall status: %s\n", (num_passed == test_count) ? "ALL TESTS PASSED" : "SOME TESTS FAILED");
    
    /* Output parseable results for automation */
    printf("\n=== Parseable Results (CSV) ===\n");
    printf("test_name,avg_time_ns,throughput_gb_s,throughput_mb_s,ops_per_sec,passed\n");
    for (int i = 0; i < test_count; i++) {
        printf("%s,%.2f,%.2f,%.2f,%.0f,%s\n",
               results[i].name,
               results[i].avg_time_ns,
               results[i].throughput_gb_s,
               results[i].throughput_mb_s,
               results[i].ops_per_sec,
               results[i].passed ? "true" : "false");
    }
    
    return (num_passed == test_count) ? 0 : 1;
}