/* layer2-bench.c - Comprehensive Layer 2 Operations Benchmark Suite
 * (c) 2024-2025 UOR Foundation - MIT License
 */

#define _POSIX_C_SOURCE 199309L
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <sys/time.h>
#include <immintrin.h>

#ifdef __linux__
#include <sys/resource.h>
#endif

#include "../llvm/include/atlas.h"
#include "../llvm/include/atlas-c-api.h"

/* Benchmark configuration */
#define ATLAS_PAGE_SIZE 12288
#define BENCHMARK_ITERATIONS 1000
#define WARMUP_ITERATIONS 100
#define MEASUREMENT_SAMPLES 10

/* Performance targets */
#define TARGET_AVX2_MEMCPY_GB_S 25.0
#define TARGET_AVX2_MEMSET_GB_S 30.0
#define TARGET_WITNESS_GB_S 1.0
#define TARGET_DELTA_NS_BYTE 10.0

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

/* CPU feature detection */
static bool has_avx2(void) {
    uint32_t eax, ebx, ecx, edx;
    __asm__ __volatile__("cpuid"
        : "=a" (eax), "=b" (ebx), "=c" (ecx), "=d" (edx)
        : "a" (7), "c" (0));
    return (ebx & (1 << 5)) != 0;
}

static bool has_avx512(void) {
    uint32_t eax, ebx, ecx, edx;
    __asm__ __volatile__("cpuid"
        : "=a" (eax), "=b" (ebx), "=c" (ecx), "=d" (edx)
        : "a" (7), "c" (0));
    return (ebx & (1 << 16)) != 0;
}

/* Conserved memcpy implementations */
static void conserved_memcpy_scalar(void* dest, const void* src, size_t n) {
    const uint8_t* s = (const uint8_t*)src;
    uint8_t* d = (uint8_t*)dest;
    uint32_t sum = 0;
    
    for (size_t i = 0; i < n; i++) {
        d[i] = s[i];
        sum += s[i];
    }
    
    /* Verify conservation */
    if (sum % 256 != 0) {
        /* Handle conservation violation */
    }
}

#ifdef __AVX2__
static void conserved_memcpy_avx2(void* dest, const void* src, size_t n) {
    const uint8_t* s = (const uint8_t*)src;
    uint8_t* d = (uint8_t*)dest;
    size_t avx2_chunks = n / 32;
    size_t remaining = n % 32;
    
    __m256i sum_vec = _mm256_setzero_si256();
    
    for (size_t i = 0; i < avx2_chunks; i++) {
        __m256i data = _mm256_loadu_si256((const __m256i*)(s + i * 32));
        _mm256_storeu_si256((__m256i*)(d + i * 32), data);
        
        /* Accumulate for conservation check */
        __m256i lo = _mm256_unpacklo_epi8(data, _mm256_setzero_si256());
        __m256i hi = _mm256_unpackhi_epi8(data, _mm256_setzero_si256());
        sum_vec = _mm256_add_epi16(sum_vec, lo);
        sum_vec = _mm256_add_epi16(sum_vec, hi);
    }
    
    /* Handle remaining bytes */
    uint32_t scalar_sum = 0;
    for (size_t i = avx2_chunks * 32; i < n; i++) {
        d[i] = s[i];
        scalar_sum += s[i];
    }
    
    /* Verify conservation */
    uint16_t sums[16];
    _mm256_storeu_si256((__m256i*)sums, sum_vec);
    uint32_t total_sum = scalar_sum;
    for (int i = 0; i < 16; i++) {
        total_sum += sums[i];
    }
    
    if (total_sum % 256 != 0) {
        /* Handle conservation violation */
    }
}
#endif

/* Conserved memset implementations */
static void conserved_memset_scalar(void* dest, int c, size_t n) {
    uint8_t* d = (uint8_t*)dest;
    uint8_t value = (uint8_t)c;
    
    /* Ensure conservation */
    if ((n * value) % 256 != 0) {
        value = (256 - (n % 256)) % 256;
    }
    
    for (size_t i = 0; i < n; i++) {
        d[i] = value;
    }
}

#ifdef __AVX2__
static void conserved_memset_avx2(void* dest, int c, size_t n) {
    uint8_t* d = (uint8_t*)dest;
    uint8_t value = (uint8_t)c;
    
    /* Ensure conservation */
    if ((n * value) % 256 != 0) {
        value = (256 - (n % 256)) % 256;
    }
    
    __m256i pattern = _mm256_set1_epi8(value);
    size_t avx2_chunks = n / 32;
    
    for (size_t i = 0; i < avx2_chunks; i++) {
        _mm256_storeu_si256((__m256i*)(d + i * 32), pattern);
    }
    
    /* Handle remaining bytes */
    for (size_t i = avx2_chunks * 32; i < n; i++) {
        d[i] = value;
    }
}
#endif

/* Delta computation benchmark */
static uint64_t delta_compute_scalar(const void* a, const void* b, size_t n) {
    const uint8_t* data_a = (const uint8_t*)a;
    const uint8_t* data_b = (const uint8_t*)b;
    uint64_t delta = 0;
    
    for (size_t i = 0; i < n; i++) {
        delta += abs((int)data_a[i] - (int)data_b[i]);
    }
    
    return delta;
}

#ifdef __AVX2__
static uint64_t delta_compute_avx2(const void* a, const void* b, size_t n) {
    const uint8_t* data_a = (const uint8_t*)a;
    const uint8_t* data_b = (const uint8_t*)b;
    
    __m256i sum_vec = _mm256_setzero_si256();
    size_t avx2_chunks = n / 32;
    
    for (size_t i = 0; i < avx2_chunks; i++) {
        __m256i va = _mm256_loadu_si256((const __m256i*)(data_a + i * 32));
        __m256i vb = _mm256_loadu_si256((const __m256i*)(data_b + i * 32));
        
        /* Compute absolute differences */
        __m256i diff = _mm256_or_si256(
            _mm256_subs_epu8(va, vb),
            _mm256_subs_epu8(vb, va)
        );
        
        /* Accumulate */
        __m256i lo = _mm256_unpacklo_epi8(diff, _mm256_setzero_si256());
        __m256i hi = _mm256_unpackhi_epi8(diff, _mm256_setzero_si256());
        sum_vec = _mm256_add_epi16(sum_vec, lo);
        sum_vec = _mm256_add_epi16(sum_vec, hi);
    }
    
    /* Sum the vector components */
    uint16_t sums[16];
    _mm256_storeu_si256((__m256i*)sums, sum_vec);
    uint64_t total = 0;
    for (int i = 0; i < 16; i++) {
        total += sums[i];
    }
    
    /* Handle remaining bytes */
    for (size_t i = avx2_chunks * 32; i < n; i++) {
        total += abs((int)data_a[i] - (int)data_b[i]);
    }
    
    return total;
}
#endif

/* Benchmark structure */
typedef struct {
    const char* name;
    double min_time_ns;
    double max_time_ns;
    double avg_time_ns;
    double throughput_gb_s;
    double throughput_mb_s;
    double ops_per_sec;
    uint64_t cycles;
    bool passed;
} benchmark_result_t;

/* Benchmark functions */
static benchmark_result_t benchmark_conserved_memcpy(void) {
    benchmark_result_t result = {0};
    result.name = "Conserved memcpy";
    
    void* src = aligned_alloc_custom(ATLAS_PAGE_SIZE, 32);
    void* dest = aligned_alloc_custom(ATLAS_PAGE_SIZE, 32);
    
    /* Initialize with conserved data */
    memset(src, 0, ATLAS_PAGE_SIZE);
    
    double times[MEASUREMENT_SAMPLES];
    uint64_t cycles_start, cycles_end;
    
    /* Warmup */
    for (int i = 0; i < WARMUP_ITERATIONS; i++) {
        conserved_memcpy_scalar(dest, src, ATLAS_PAGE_SIZE);
    }
    
    /* Measurements */
    for (int sample = 0; sample < MEASUREMENT_SAMPLES; sample++) {
        cycles_start = rdtsc();
        double start = get_time_ns();
        
        for (int i = 0; i < BENCHMARK_ITERATIONS; i++) {
#ifdef __AVX2__
            if (has_avx2()) {
                conserved_memcpy_avx2(dest, src, ATLAS_PAGE_SIZE);
            } else {
                conserved_memcpy_scalar(dest, src, ATLAS_PAGE_SIZE);
            }
#else
            conserved_memcpy_scalar(dest, src, ATLAS_PAGE_SIZE);
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
    
    result.throughput_gb_s = (ATLAS_PAGE_SIZE * 1e9) / (result.avg_time_ns * 1e9);
    result.cycles = (cycles_end - cycles_start) / BENCHMARK_ITERATIONS;
    result.passed = result.throughput_gb_s >= TARGET_AVX2_MEMCPY_GB_S;
    
    free(src);
    free(dest);
    
    return result;
}

static benchmark_result_t benchmark_conserved_memset(void) {
    benchmark_result_t result = {0};
    result.name = "Conserved memset";
    
    void* dest = aligned_alloc_custom(ATLAS_PAGE_SIZE, 32);
    double times[MEASUREMENT_SAMPLES];
    
    /* Warmup */
    for (int i = 0; i < WARMUP_ITERATIONS; i++) {
        conserved_memset_scalar(dest, 0, ATLAS_PAGE_SIZE);
    }
    
    /* Measurements */
    for (int sample = 0; sample < MEASUREMENT_SAMPLES; sample++) {
        double start = get_time_ns();
        
        for (int i = 0; i < BENCHMARK_ITERATIONS; i++) {
#ifdef __AVX2__
            if (has_avx2()) {
                conserved_memset_avx2(dest, 0, ATLAS_PAGE_SIZE);
            } else {
                conserved_memset_scalar(dest, 0, ATLAS_PAGE_SIZE);
            }
#else
            conserved_memset_scalar(dest, 0, ATLAS_PAGE_SIZE);
#endif
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
    
    result.throughput_gb_s = (ATLAS_PAGE_SIZE * 1e9) / (result.avg_time_ns * 1e9);
    result.passed = result.throughput_gb_s >= TARGET_AVX2_MEMSET_GB_S;
    
    free(dest);
    
    return result;
}

static benchmark_result_t benchmark_witness_operations(void) {
    benchmark_result_t result = {0};
    result.name = "Witness generate/verify";
    
    void* data = aligned_alloc_custom(ATLAS_PAGE_SIZE, 32);
    memset(data, 0, ATLAS_PAGE_SIZE);
    
    double gen_times[MEASUREMENT_SAMPLES];
    double verify_times[MEASUREMENT_SAMPLES];
    
    /* Warmup */
    for (int i = 0; i < WARMUP_ITERATIONS; i++) {
        atlas_witness_t witness = atlas_witness_generate(data, ATLAS_PAGE_SIZE);
        atlas_witness_verify(witness, data, ATLAS_PAGE_SIZE);
        atlas_witness_destroy(witness);
    }
    
    /* Measurements */
    for (int sample = 0; sample < MEASUREMENT_SAMPLES; sample++) {
        /* Generation benchmark */
        double start = get_time_ns();
        atlas_witness_t witnesses[BENCHMARK_ITERATIONS];
        
        for (int i = 0; i < BENCHMARK_ITERATIONS; i++) {
            witnesses[i] = atlas_witness_generate(data, ATLAS_PAGE_SIZE);
        }
        
        double end = get_time_ns();
        gen_times[sample] = (end - start) / BENCHMARK_ITERATIONS;
        
        /* Verification benchmark */
        start = get_time_ns();
        
        for (int i = 0; i < BENCHMARK_ITERATIONS; i++) {
            atlas_witness_verify(witnesses[i], data, ATLAS_PAGE_SIZE);
        }
        
        end = get_time_ns();
        verify_times[sample] = (end - start) / BENCHMARK_ITERATIONS;
        
        /* Cleanup */
        for (int i = 0; i < BENCHMARK_ITERATIONS; i++) {
            atlas_witness_destroy(witnesses[i]);
        }
    }
    
    /* Calculate statistics for combined operation */
    result.min_time_ns = gen_times[0] + verify_times[0];
    result.max_time_ns = gen_times[0] + verify_times[0];
    result.avg_time_ns = 0;
    
    for (int i = 0; i < MEASUREMENT_SAMPLES; i++) {
        double combined = gen_times[i] + verify_times[i];
        if (combined < result.min_time_ns) result.min_time_ns = combined;
        if (combined > result.max_time_ns) result.max_time_ns = combined;
        result.avg_time_ns += combined;
    }
    result.avg_time_ns /= MEASUREMENT_SAMPLES;
    
    result.throughput_mb_s = (ATLAS_PAGE_SIZE * 1e6) / (result.avg_time_ns * 1e6);
    result.throughput_gb_s = result.throughput_mb_s / 1024.0;
    result.passed = result.throughput_gb_s >= TARGET_WITNESS_GB_S;
    
    free(data);
    
    return result;
}

static benchmark_result_t benchmark_delta_computation(void) {
    benchmark_result_t result = {0};
    result.name = "Delta computation";
    
    void* data_a = aligned_alloc_custom(ATLAS_PAGE_SIZE, 32);
    void* data_b = aligned_alloc_custom(ATLAS_PAGE_SIZE, 32);
    
    /* Initialize with different patterns */
    memset(data_a, 0x55, ATLAS_PAGE_SIZE);
    memset(data_b, 0xAA, ATLAS_PAGE_SIZE);
    
    double times[MEASUREMENT_SAMPLES];
    
    /* Warmup */
    for (int i = 0; i < WARMUP_ITERATIONS; i++) {
        delta_compute_scalar(data_a, data_b, ATLAS_PAGE_SIZE);
    }
    
    /* Measurements */
    for (int sample = 0; sample < MEASUREMENT_SAMPLES; sample++) {
        double start = get_time_ns();
        
        for (int i = 0; i < BENCHMARK_ITERATIONS; i++) {
#ifdef __AVX2__
            if (has_avx2()) {
                delta_compute_avx2(data_a, data_b, ATLAS_PAGE_SIZE);
            } else {
                delta_compute_scalar(data_a, data_b, ATLAS_PAGE_SIZE);
            }
#else
            delta_compute_scalar(data_a, data_b, ATLAS_PAGE_SIZE);
#endif
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
    
    double ns_per_byte = result.avg_time_ns / ATLAS_PAGE_SIZE;
    result.passed = ns_per_byte < TARGET_DELTA_NS_BYTE;
    
    free(data_a);
    free(data_b);
    
    return result;
}

static benchmark_result_t benchmark_budget_operations(void) {
    benchmark_result_t result = {0};
    result.name = "Budget operations";
    
    double times[MEASUREMENT_SAMPLES];
    
    /* Warmup */
    for (int i = 0; i < WARMUP_ITERATIONS; i++) {
        for (uint8_t a = 0; a < 16; a++) {
            for (uint8_t b = 0; b < 16; b++) {
                atlas_budget_add(a, b);
                atlas_budget_mul(a, b);
                atlas_budget_inv(a);
                atlas_budget_zero(a);
            }
        }
    }
    
    /* Measurements */
    for (int sample = 0; sample < MEASUREMENT_SAMPLES; sample++) {
        double start = get_time_ns();
        
        for (int iter = 0; iter < BENCHMARK_ITERATIONS; iter++) {
            for (uint8_t a = 0; a < 16; a++) {
                for (uint8_t b = 0; b < 16; b++) {
                    atlas_budget_add(a, b);
                    atlas_budget_mul(a, b);
                    atlas_budget_inv(a);
                    atlas_budget_zero(a);
                }
            }
        }
        
        double end = get_time_ns();
        times[sample] = (end - start) / (BENCHMARK_ITERATIONS * 16 * 16 * 4);
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
    
    result.ops_per_sec = 1e9 / result.avg_time_ns;
    result.passed = result.ops_per_sec > 1e6; /* Target: >1M ops/sec */
    
    return result;
}

static benchmark_result_t benchmark_domain_lifecycle(void) {
    benchmark_result_t result = {0};
    result.name = "Domain lifecycle";
    
    double times[MEASUREMENT_SAMPLES];
    
    /* Measurements */
    for (int sample = 0; sample < MEASUREMENT_SAMPLES; sample++) {
        double start = get_time_ns();
        
        for (int i = 0; i < BENCHMARK_ITERATIONS; i++) {
            atlas_pool_t pool = atlas_pool_create(1);
            void* ptr = atlas_pool_alloc(pool, ATLAS_PAGE_SIZE);
            (void)ptr; /* Suppress unused variable warning */
            atlas_pool_destroy(pool);
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
    
    result.ops_per_sec = 1e9 / result.avg_time_ns;
    result.passed = result.ops_per_sec > 10000; /* Target: >10k lifecycle ops/sec */
    
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
    printf("AVX-512 Support: %s\n", has_avx512() ? "Yes" : "No");
    printf("Page Size: %d bytes\n", ATLAS_PAGE_SIZE);
    printf("Iterations per test: %d\n", BENCHMARK_ITERATIONS);
    printf("Measurement samples: %d\n", MEASUREMENT_SAMPLES);
    printf("\n");
}

/* Results printing */
static void print_result(const benchmark_result_t* result) {
    printf("%-25s | ", result->name);
    printf("Avg: %8.2f ns | ", result->avg_time_ns);
    
    if (result->throughput_gb_s > 0) {
        printf("Throughput: %6.2f GB/s | ", result->throughput_gb_s);
    } else if (result->throughput_mb_s > 0) {
        printf("Throughput: %6.2f MB/s | ", result->throughput_mb_s);
    } else if (result->ops_per_sec > 0) {
        printf("Rate: %10.0f ops/s | ", result->ops_per_sec);
    } else {
        printf("Rate: %10.2f ns/byte | ", result->avg_time_ns / ATLAS_PAGE_SIZE);
    }
    
    printf("Status: %s\n", result->passed ? "PASS" : "FAIL");
}

int main(void) {
    printf("Atlas-12288 Layer 2 Operations Benchmark Suite\n");
    printf("==============================================\n\n");
    
    print_system_info();
    
    /* Initialize Atlas runtime */
    atlas_init();
    
    benchmark_result_t results[6];
    int num_passed = 0;
    
    printf("=== Performance Benchmarks ===\n");
    printf("%-25s | %-15s | %-18s | Status\n", "Test", "Time", "Throughput/Rate");
    printf("--------------------------------------------------------------------------\n");
    
    /* Run benchmarks */
    results[0] = benchmark_conserved_memcpy();
    print_result(&results[0]);
    if (results[0].passed) num_passed++;
    
    results[1] = benchmark_conserved_memset();
    print_result(&results[1]);
    if (results[1].passed) num_passed++;
    
    results[2] = benchmark_witness_operations();
    print_result(&results[2]);
    if (results[2].passed) num_passed++;
    
    results[3] = benchmark_delta_computation();
    print_result(&results[3]);
    if (results[3].passed) num_passed++;
    
    results[4] = benchmark_budget_operations();
    print_result(&results[4]);
    if (results[4].passed) num_passed++;
    
    results[5] = benchmark_domain_lifecycle();
    print_result(&results[5]);
    if (results[5].passed) num_passed++;
    
    printf("\n=== Performance Targets vs Results ===\n");
    printf("Conserved memcpy (AVX2): Target ≥%.0f GB/s, Achieved %.2f GB/s\n", 
           TARGET_AVX2_MEMCPY_GB_S, results[0].throughput_gb_s);
    printf("Conserved memset (AVX2): Target ≥%.0f GB/s, Achieved %.2f GB/s\n", 
           TARGET_AVX2_MEMSET_GB_S, results[1].throughput_gb_s);
    printf("Witness operations:      Target ≥%.0f GB/s, Achieved %.2f GB/s\n", 
           TARGET_WITNESS_GB_S, results[2].throughput_gb_s);
    printf("Delta computation:       Target <%.0f ns/byte, Achieved %.2f ns/byte\n", 
           TARGET_DELTA_NS_BYTE, results[3].avg_time_ns / ATLAS_PAGE_SIZE);
    
    printf("\n=== Summary ===\n");
    printf("Tests passed: %d/6\n", num_passed);
    printf("Overall status: %s\n", (num_passed == 6) ? "ALL TESTS PASSED" : "SOME TESTS FAILED");
    
    /* Cleanup */
    atlas_cleanup();
    
    return (num_passed == 6) ? 0 : 1;
}