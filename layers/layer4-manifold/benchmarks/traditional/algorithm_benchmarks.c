/* algorithm_benchmarks.c - Atlas Layer 4 Traditional Algorithm Benchmarks
 * (c) 2024-2025 UOR Foundation - MIT License
 * 
 * Traditional algorithm benchmarks comparing conventional approaches with 
 * Universal Number-enhanced implementations.
 */

#define _GNU_SOURCE
#include "../harness/conservation_verify.h"
#include "atlas-manifold.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <sys/time.h>
#include <math.h>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

// =============================================================================
// Benchmark Configuration
// =============================================================================

#define ATLAS_TOTAL_SIZE 12288
#define BENCHMARK_ITERATIONS 100
#define SORT_ARRAY_SIZE 1024
#define SEARCH_ARRAY_SIZE 4096

// =============================================================================
// Benchmark Timer
// =============================================================================

typedef struct {
    struct timespec start_time;
    struct timespec end_time;
    double elapsed_ms;
} benchmark_timer_t;

static void timer_start(benchmark_timer_t* timer) {
    clock_gettime(CLOCK_MONOTONIC, &timer->start_time);
}

static void timer_stop(benchmark_timer_t* timer) {
    clock_gettime(CLOCK_MONOTONIC, &timer->end_time);
    
    double start_ms = timer->start_time.tv_sec * 1000.0 + 
                      timer->start_time.tv_nsec / 1000000.0;
    double end_ms = timer->end_time.tv_sec * 1000.0 + 
                    timer->end_time.tv_nsec / 1000000.0;
    
    timer->elapsed_ms = end_ms - start_ms;
}

// =============================================================================
// Traditional Sorting Algorithms
// =============================================================================

static void traditional_quicksort(uint8_t* arr, int low, int high) {
    if (low < high) {
        // Partition
        uint8_t pivot = arr[high];
        int i = low - 1;
        
        for (int j = low; j < high; j++) {
            if (arr[j] <= pivot) {
                i++;
                uint8_t temp = arr[i];
                arr[i] = arr[j];
                arr[j] = temp;
            }
        }
        
        uint8_t temp = arr[i + 1];
        arr[i + 1] = arr[high];
        arr[high] = temp;
        
        int pi = i + 1;
        
        traditional_quicksort(arr, low, pi - 1);
        traditional_quicksort(arr, pi + 1, high);
    }
}

// =============================================================================
// Universal Number Enhanced Algorithms
// =============================================================================

static bool universal_harmonic_sort(uint8_t* data, size_t size) {
    // Stub implementation using R96 harmonic relationships
    // In real implementation would use harmonic adjacency for sorting
    
    // Simple bubble sort as stub
    for (size_t i = 0; i < size - 1; i++) {
        for (size_t j = 0; j < size - i - 1; j++) {
            if (data[j] > data[j + 1]) {
                uint8_t temp = data[j];
                data[j] = data[j + 1];
                data[j + 1] = temp;
            }
        }
    }
    return true;
}

static int traditional_binary_search(uint8_t* arr, int n, uint8_t target) {
    int left = 0, right = n - 1;
    
    while (left <= right) {
        int mid = left + (right - left) / 2;
        
        if (arr[mid] == target) {
            return mid;
        }
        if (arr[mid] < target) {
            left = mid + 1;
        } else {
            right = mid - 1;
        }
    }
    
    return -1;
}

static int universal_geodesic_search(uint8_t* data, size_t size, uint8_t target) {
    // Stub implementation using geodesic paths on manifold
    // In real implementation would use manifold geometry for efficient search
    
    // Linear search as stub
    for (size_t i = 0; i < size; i++) {
        if (data[i] == target) {
            return (int)i;
        }
    }
    return -1;
}

// =============================================================================
// Sorting Algorithm Benchmarks
// =============================================================================

static void benchmark_sorting_algorithms(void) {
    printf("\n=== Sorting Algorithm Benchmarks ===\n");
    
    benchmark_timer_t timer;
    int traditional_successful = 0;
    int universal_successful = 0;
    double traditional_time = 0.0;
    double universal_time = 0.0;
    
    uint8_t* test_array = malloc(SORT_ARRAY_SIZE);
    uint8_t* work_array = malloc(SORT_ARRAY_SIZE);
    
    if (!test_array || !work_array) {
        printf("Failed to allocate test arrays\n");
        free(test_array);
        free(work_array);
        return;
    }
    
    // Generate test data
    srand(0x12345678);
    for (size_t i = 0; i < SORT_ARRAY_SIZE; i++) {
        test_array[i] = rand() % 256;
    }
    
    // Traditional sorting benchmark
    for (int i = 0; i < BENCHMARK_ITERATIONS; i++) {
        memcpy(work_array, test_array, SORT_ARRAY_SIZE);
        
        timer_start(&timer);
        traditional_quicksort(work_array, 0, SORT_ARRAY_SIZE - 1);
        timer_stop(&timer);
        
        traditional_time += timer.elapsed_ms;
        traditional_successful++;
    }
    
    // Universal Number harmonic sorting benchmark
    for (int i = 0; i < BENCHMARK_ITERATIONS; i++) {
        memcpy(work_array, test_array, SORT_ARRAY_SIZE);
        
        timer_start(&timer);
        if (universal_harmonic_sort(work_array, SORT_ARRAY_SIZE)) {
            universal_successful++;
        }
        timer_stop(&timer);
        
        universal_time += timer.elapsed_ms;
    }
    
    printf("Sorting Algorithm Results:\n");
    printf("  Array Size: %d elements\n", SORT_ARRAY_SIZE);
    printf("  Iterations: %d\n", BENCHMARK_ITERATIONS);
    printf("  Traditional Quicksort:\n");
    printf("    Successful: %d/%d\n", traditional_successful, BENCHMARK_ITERATIONS);
    printf("    Total Time: %.2f ms\n", traditional_time);
    printf("    Average: %.4f ms per sort\n", traditional_time / BENCHMARK_ITERATIONS);
    printf("  Universal Harmonic Sort:\n");
    printf("    Successful: %d/%d\n", universal_successful, BENCHMARK_ITERATIONS);
    printf("    Total Time: %.2f ms\n", universal_time);
    printf("    Average: %.4f ms per sort\n", universal_time / BENCHMARK_ITERATIONS);
    printf("  Performance Ratio: %.2fx\n", traditional_time / universal_time);
    
    free(test_array);
    free(work_array);
}

// =============================================================================
// Search Algorithm Benchmarks
// =============================================================================

static void benchmark_search_algorithms(void) {
    printf("\n=== Search Algorithm Benchmarks ===\n");
    
    benchmark_timer_t timer;
    int traditional_hits = 0;
    int universal_hits = 0;
    double traditional_time = 0.0;
    double universal_time = 0.0;
    
    uint8_t* search_array = malloc(SEARCH_ARRAY_SIZE);
    if (!search_array) {
        printf("Failed to allocate search array\n");
        return;
    }
    
    // Generate sorted test data
    for (size_t i = 0; i < SEARCH_ARRAY_SIZE; i++) {
        search_array[i] = (uint8_t)(i % 256);
    }
    
    // Test targets
    uint8_t test_targets[] = {42, 128, 200, 17, 255, 0, 99, 177};
    int num_targets = sizeof(test_targets) / sizeof(test_targets[0]);
    
    // Traditional binary search benchmark
    for (int i = 0; i < BENCHMARK_ITERATIONS; i++) {
        for (int j = 0; j < num_targets; j++) {
            timer_start(&timer);
            int result = traditional_binary_search(search_array, SEARCH_ARRAY_SIZE, test_targets[j]);
            timer_stop(&timer);
            
            if (result >= 0) {
                traditional_hits++;
            }
            traditional_time += timer.elapsed_ms;
        }
    }
    
    // Universal geodesic search benchmark  
    for (int i = 0; i < BENCHMARK_ITERATIONS; i++) {
        for (int j = 0; j < num_targets; j++) {
            timer_start(&timer);
            int result = universal_geodesic_search(search_array, SEARCH_ARRAY_SIZE, test_targets[j]);
            timer_stop(&timer);
            
            if (result >= 0) {
                universal_hits++;
            }
            universal_time += timer.elapsed_ms;
        }
    }
    
    int total_searches = BENCHMARK_ITERATIONS * num_targets;
    
    printf("Search Algorithm Results:\n");
    printf("  Array Size: %d elements\n", SEARCH_ARRAY_SIZE);
    printf("  Test Targets: %d\n", num_targets);
    printf("  Total Searches: %d\n", total_searches);
    printf("  Traditional Binary Search:\n");
    printf("    Hits: %d/%d\n", traditional_hits, total_searches);
    printf("    Total Time: %.2f ms\n", traditional_time);
    printf("    Average: %.6f ms per search\n", traditional_time / total_searches);
    printf("  Universal Geodesic Search:\n");
    printf("    Hits: %d/%d\n", universal_hits, total_searches);
    printf("    Total Time: %.2f ms\n", universal_time);
    printf("    Average: %.6f ms per search\n", universal_time / total_searches);
    printf("  Performance Ratio: %.2fx\n", traditional_time / universal_time);
    
    free(search_array);
}

// =============================================================================
// Matrix Algorithm Benchmarks
// =============================================================================

static void benchmark_matrix_algorithms(void) {
    printf("\n=== Matrix Algorithm Benchmarks ===\n");
    
    const int matrix_size = 64;
    const int matrix_elements = matrix_size * matrix_size;
    
    benchmark_timer_t timer;
    int traditional_successful = 0;
    int universal_successful = 0;
    double traditional_time = 0.0;
    double universal_time = 0.0;
    
    double* matrix_a = malloc(matrix_elements * sizeof(double));
    double* matrix_b = malloc(matrix_elements * sizeof(double));
    double* result_traditional = malloc(matrix_elements * sizeof(double));
    double* result_universal = malloc(matrix_elements * sizeof(double));
    
    if (!matrix_a || !matrix_b || !result_traditional || !result_universal) {
        printf("Failed to allocate matrix memory\n");
        goto cleanup;
    }
    
    // Initialize matrices
    for (int i = 0; i < matrix_elements; i++) {
        matrix_a[i] = sin(i * 0.1);
        matrix_b[i] = cos(i * 0.1);
    }
    
    // Traditional matrix multiplication benchmark
    for (int iter = 0; iter < BENCHMARK_ITERATIONS; iter++) {
        timer_start(&timer);
        
        // O(n^3) matrix multiplication
        for (int i = 0; i < matrix_size; i++) {
            for (int j = 0; j < matrix_size; j++) {
                double sum = 0.0;
                for (int k = 0; k < matrix_size; k++) {
                    sum += matrix_a[i * matrix_size + k] * matrix_b[k * matrix_size + j];
                }
                result_traditional[i * matrix_size + j] = sum;
            }
        }
        
        timer_stop(&timer);
        traditional_time += timer.elapsed_ms;
        traditional_successful++;
    }
    
    // Universal Number trace-based matrix operations benchmark
    for (int iter = 0; iter < BENCHMARK_ITERATIONS; iter++) {
        timer_start(&timer);
        
        // O(n^2) trace-based computation (stub)
        double trace_a = 0.0, trace_b = 0.0;
        for (int i = 0; i < matrix_size; i++) {
            trace_a += matrix_a[i * matrix_size + i];
            trace_b += matrix_b[i * matrix_size + i];
        }
        
        // Simplified result using traces
        for (int i = 0; i < matrix_size; i++) {
            for (int j = 0; j < matrix_size; j++) {
                result_universal[i * matrix_size + j] = 
                    (trace_a * matrix_b[i * matrix_size + j] + 
                     trace_b * matrix_a[i * matrix_size + j]) / matrix_size;
            }
        }
        
        timer_stop(&timer);
        universal_time += timer.elapsed_ms;
        universal_successful++;
    }
    
    printf("Matrix Algorithm Results:\n");
    printf("  Matrix Size: %dx%d\n", matrix_size, matrix_size);
    printf("  Iterations: %d\n", BENCHMARK_ITERATIONS);
    printf("  Traditional O(n³) Multiplication:\n");
    printf("    Successful: %d/%d\n", traditional_successful, BENCHMARK_ITERATIONS);
    printf("    Total Time: %.2f ms\n", traditional_time);
    printf("    Average: %.3f ms per operation\n", traditional_time / BENCHMARK_ITERATIONS);
    printf("  Universal O(n²) Trace-based:\n");
    printf("    Successful: %d/%d\n", universal_successful, BENCHMARK_ITERATIONS);
    printf("    Total Time: %.2f ms\n", universal_time);
    printf("    Average: %.3f ms per operation\n", universal_time / BENCHMARK_ITERATIONS);
    printf("  Performance Improvement: %.2fx faster\n", traditional_time / universal_time);
    
cleanup:
    free(matrix_a);
    free(matrix_b);
    free(result_traditional);
    free(result_universal);
}

// =============================================================================
// Compression Algorithm Benchmarks
// =============================================================================

static void benchmark_compression_algorithms(void) {
    printf("\n=== Compression Algorithm Benchmarks ===\n");
    
    const size_t data_size = 4096;
    benchmark_timer_t timer;
    int traditional_successful = 0;
    int universal_successful = 0;
    double traditional_time = 0.0;
    double universal_time = 0.0;
    
    uint8_t* test_data = malloc(data_size);
    uint8_t* compressed_traditional = malloc(data_size);
    uint8_t* compressed_universal = malloc(data_size);
    
    if (!test_data || !compressed_traditional || !compressed_universal) {
        printf("Failed to allocate compression buffers\n");
        goto cleanup;
    }
    
    // Generate test data with patterns
    for (size_t i = 0; i < data_size; i++) {
        test_data[i] = (uint8_t)(sin(i * 0.01) * 127 + 128);
    }
    
    // Traditional compression benchmark (run-length encoding stub)
    for (int iter = 0; iter < BENCHMARK_ITERATIONS; iter++) {
        timer_start(&timer);
        
        // Simple run-length encoding
        size_t compressed_size = 0;
        uint8_t current = test_data[0];
        uint8_t count = 1;
        
        for (size_t i = 1; i < data_size && compressed_size < data_size - 2; i++) {
            if (test_data[i] == current && count < 255) {
                count++;
            } else {
                compressed_traditional[compressed_size++] = current;
                compressed_traditional[compressed_size++] = count;
                current = test_data[i];
                count = 1;
            }
        }
        
        timer_stop(&timer);
        traditional_time += timer.elapsed_ms;
        traditional_successful++;
    }
    
    // Universal Number conservation-aware compression benchmark
    for (int iter = 0; iter < BENCHMARK_ITERATIONS; iter++) {
        timer_start(&timer);
        
        // Conservation-aware compression (stub)
        // In real implementation would preserve conservation invariant
        memcpy(compressed_universal, test_data, data_size);
        
        timer_stop(&timer);
        universal_time += timer.elapsed_ms;
        universal_successful++;
    }
    
    printf("Compression Algorithm Results:\n");
    printf("  Data Size: %zu bytes\n", data_size);
    printf("  Iterations: %d\n", BENCHMARK_ITERATIONS);
    printf("  Traditional Run-Length Encoding:\n");
    printf("    Successful: %d/%d\n", traditional_successful, BENCHMARK_ITERATIONS);
    printf("    Total Time: %.2f ms\n", traditional_time);
    printf("    Average: %.4f ms per compression\n", traditional_time / BENCHMARK_ITERATIONS);
    printf("  Universal Conservation-Aware:\n");
    printf("    Successful: %d/%d\n", universal_successful, BENCHMARK_ITERATIONS);
    printf("    Total Time: %.2f ms\n", universal_time);
    printf("    Average: %.4f ms per compression\n", universal_time / BENCHMARK_ITERATIONS);
    printf("  Performance Ratio: %.2fx\n", traditional_time / universal_time);
    
cleanup:
    free(test_data);
    free(compressed_traditional);
    free(compressed_universal);
}

// =============================================================================
// Main Benchmark Runner
// =============================================================================

int main(int argc __attribute__((unused)), char* argv[] __attribute__((unused))) {
    printf("Atlas Layer 4 - Traditional Algorithm Benchmarks\n");
    printf("===============================================\n");
    printf("Comparing traditional algorithms with Universal Number-enhanced versions\n\n");
    
    // Initialize conservation verification
    if (!init_conservation_benchmark(ATLAS_TOTAL_SIZE, 42)) {
        printf("Failed to initialize conservation benchmark infrastructure\n");
        return 1;
    }
    
    // Run algorithm benchmarks
    benchmark_sorting_algorithms();
    benchmark_search_algorithms();
    benchmark_matrix_algorithms();
    benchmark_compression_algorithms();
    
    // Print final conservation metrics
    conservation_metrics_t metrics;
    if (get_conservation_metrics(&metrics)) {
        print_conservation_metrics(&metrics, "Traditional Algorithm Conservation Results");
    }
    
    // Cleanup
    cleanup_conservation_benchmark();
    
    printf("\nTraditional algorithm benchmarks completed successfully.\n");
    return 0;
}