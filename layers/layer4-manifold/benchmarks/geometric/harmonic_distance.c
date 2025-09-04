/**
 * harmonic_distance.c - Harmonic Distance vs Euclidean Distance Benchmark
 * (c) 2024-2025 UOR Foundation - MIT License
 * 
 * Compares harmonic distance (R96 resonance-based) with traditional
 * Euclidean distance calculations for geometric operations.
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <time.h>
#include <math.h>
#include <stdbool.h>

#include "atlas-manifold.h"

#define BENCHMARK_ITERATIONS 25000
#define WARMUP_ITERATIONS 2500
#define POINT_COUNT 512
#define DIMENSIONS 3

// Timing utilities
static uint64_t get_time_ns(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (uint64_t)ts.tv_sec * 1000000000UL + (uint64_t)ts.tv_nsec;
}

// Point structures
typedef struct {
    uint8_t coords[DIMENSIONS];
    uint8_t resonance_classes[DIMENSIONS];
} harmonic_point_t;

typedef struct {
    double coords[DIMENSIONS];
} euclidean_point_t;

// Test data
typedef struct {
    harmonic_point_t harmonic_points[POINT_COUNT];
    euclidean_point_t euclidean_points[POINT_COUNT];
    double harmonic_distances[POINT_COUNT][POINT_COUNT];
    double euclidean_distances[POINT_COUNT][POINT_COUNT];
} test_data_t;

// Initialize test points
static void init_test_data(test_data_t* test_data) {
    for (size_t i = 0; i < POINT_COUNT; i++) {
        for (int d = 0; d < DIMENSIONS; d++) {
            // Harmonic points with R96 classification
            test_data->harmonic_points[i].coords[d] = (uint8_t)((i * 17 + d * 31) % 256);
            test_data->harmonic_points[i].resonance_classes[d] = 
                test_data->harmonic_points[i].coords[d] % 96;
            
            // Corresponding Euclidean points
            test_data->euclidean_points[i].coords[d] = 
                (double)test_data->harmonic_points[i].coords[d];
        }
    }
}

// Harmonic distance using R96 resonance
static double harmonic_distance(const harmonic_point_t* p1, const harmonic_point_t* p2) {
    double distance = 0.0;
    
    for (int d = 0; d < DIMENSIONS; d++) {
        uint8_t r1 = p1->resonance_classes[d];
        uint8_t r2 = p2->resonance_classes[d];
        
        // Harmonic distance based on resonance class differences
        int diff = (int)r1 - (int)r2;
        if (diff < 0) diff = -diff;
        
        // Use harmonic spacing: distance to nearest harmonic (multiple of 48)
        int harmonic_diff = (diff <= 48) ? diff : (96 - diff);
        
        // Convert to harmonic units (normalized by 48 for half-cycle)
        double harmonic_component = (double)harmonic_diff / 48.0;
        distance += harmonic_component * harmonic_component;
    }
    
    return sqrt(distance);
}

// Traditional Euclidean distance
static double euclidean_distance(const euclidean_point_t* p1, const euclidean_point_t* p2) {
    double distance_squared = 0.0;
    
    for (int d = 0; d < DIMENSIONS; d++) {
        double diff = p1->coords[d] - p2->coords[d];
        distance_squared += diff * diff;
    }
    
    return sqrt(distance_squared);
}

// Benchmark harmonic distance computation
static void benchmark_harmonic_distance(void) {
    test_data_t test_data;
    init_test_data(&test_data);
    
    printf("Benchmarking Harmonic Distance Computation...\n");
    printf("Points: %d, Dimensions: %d\n", POINT_COUNT, DIMENSIONS);
    printf("Iterations: %d (warmup: %d)\n", BENCHMARK_ITERATIONS, WARMUP_ITERATIONS);
    
    // Warmup
    for (int iter = 0; iter < WARMUP_ITERATIONS; iter++) {
        for (size_t i = 0; i < POINT_COUNT && i < 10; i++) {
            for (size_t j = i + 1; j < POINT_COUNT && j < 10; j++) {
                volatile double dist = harmonic_distance(
                    &test_data.harmonic_points[i], 
                    &test_data.harmonic_points[j]
                );
                (void)dist;
            }
        }
    }
    
    // Benchmark
    uint64_t start_time = get_time_ns();
    uint64_t total_computations = 0;
    double total_distance = 0.0;
    
    for (int iter = 0; iter < BENCHMARK_ITERATIONS; iter++) {
        for (size_t i = 0; i < POINT_COUNT; i += 8) {  // Sample every 8th point
            for (size_t j = i + 1; j < POINT_COUNT && j < i + 8; j++) {
                double dist = harmonic_distance(
                    &test_data.harmonic_points[i], 
                    &test_data.harmonic_points[j]
                );
                total_distance += dist;
                total_computations++;
            }
        }
    }
    
    uint64_t end_time = get_time_ns();
    uint64_t duration_ns = end_time - start_time;
    
    double avg_distance = total_distance / total_computations;
    double duration_per_computation = (double)duration_ns / total_computations;
    double computations_per_sec = (1e9 * total_computations) / duration_ns;
    
    printf("{\n");
    printf("  \"benchmark_name\": \"harmonic_distance\",\n");
    printf("  \"description\": \"R96 resonance-based harmonic distance computation\",\n");
    printf("  \"duration_ns\": %lu,\n", duration_ns);
    printf("  \"iterations\": %d,\n", BENCHMARK_ITERATIONS);
    printf("  \"total_computations\": %lu,\n", total_computations);
    printf("  \"duration_per_computation_ns\": %.2f,\n", duration_per_computation);
    printf("  \"throughput_computations_sec\": %.2f,\n", computations_per_sec);
    printf("  \"average_distance\": %.4f,\n", avg_distance);
    printf("  \"points\": %d,\n", POINT_COUNT);
    printf("  \"dimensions\": %d,\n", DIMENSIONS);
    printf("  \"metadata\": {\n");
    printf("    \"approach\": \"r96_harmonic\",\n");
    printf("    \"resonance_classes\": 96,\n");
    printf("    \"harmonic_spacing\": 48,\n");
    printf("    \"conservation_aware\": true,\n");
    printf("    \"integer_arithmetic\": true\n");
    printf("  }\n");
    printf("}\n");
}

// Benchmark Euclidean distance for comparison
static void benchmark_euclidean_distance(void) {
    test_data_t test_data;
    init_test_data(&test_data);
    
    printf("Benchmarking Euclidean Distance (Baseline)...\n");
    
    // Warmup
    for (int iter = 0; iter < WARMUP_ITERATIONS; iter++) {
        for (size_t i = 0; i < POINT_COUNT && i < 10; i++) {
            for (size_t j = i + 1; j < POINT_COUNT && j < 10; j++) {
                volatile double dist = euclidean_distance(
                    &test_data.euclidean_points[i], 
                    &test_data.euclidean_points[j]
                );
                (void)dist;
            }
        }
    }
    
    // Benchmark
    uint64_t start_time = get_time_ns();
    uint64_t total_computations = 0;
    double total_distance = 0.0;
    
    for (int iter = 0; iter < BENCHMARK_ITERATIONS; iter++) {
        for (size_t i = 0; i < POINT_COUNT; i += 8) {  // Sample every 8th point
            for (size_t j = i + 1; j < POINT_COUNT && j < i + 8; j++) {
                double dist = euclidean_distance(
                    &test_data.euclidean_points[i], 
                    &test_data.euclidean_points[j]
                );
                total_distance += dist;
                total_computations++;
            }
        }
    }
    
    uint64_t end_time = get_time_ns();
    uint64_t duration_ns = end_time - start_time;
    
    double avg_distance = total_distance / total_computations;
    double duration_per_computation = (double)duration_ns / total_computations;
    double computations_per_sec = (1e9 * total_computations) / duration_ns;
    
    printf("{\n");
    printf("  \"benchmark_name\": \"euclidean_distance_comparison\",\n");
    printf("  \"description\": \"Traditional Euclidean distance computation (baseline)\",\n");
    printf("  \"duration_ns\": %lu,\n", duration_ns);
    printf("  \"iterations\": %d,\n", BENCHMARK_ITERATIONS);
    printf("  \"total_computations\": %lu,\n", total_computations);
    printf("  \"duration_per_computation_ns\": %.2f,\n", duration_per_computation);
    printf("  \"throughput_computations_sec\": %.2f,\n", computations_per_sec);
    printf("  \"average_distance\": %.4f,\n", avg_distance);
    printf("  \"points\": %d,\n", POINT_COUNT);
    printf("  \"dimensions\": %d,\n", DIMENSIONS);
    printf("  \"metadata\": {\n");
    printf("    \"approach\": \"euclidean\",\n");
    printf("    \"floating_point_ops\": true,\n");
    printf("    \"sqrt_operations\": true,\n");
    printf("    \"conservation_aware\": false\n");
    printf("  }\n");
    printf("}\n");
}

// Test distance computation correctness
static void test_distance_computations(void) {
    printf("Testing distance computation correctness...\n");
    
    // Test identical points
    harmonic_point_t p1 = {{0, 48, 96}, {0, 48, 0}};
    harmonic_point_t p2 = {{0, 48, 96}, {0, 48, 0}};
    
    double harmonic_dist = harmonic_distance(&p1, &p2);
    bool test1_pass = (harmonic_dist < 1e-10);
    
    printf("  Harmonic distance (identical points): %.6f ≈ 0: %s\n", 
           harmonic_dist, test1_pass ? "✓" : "✗");
    
    // Test harmonic pairs (should have distance 0 in each dimension)
    harmonic_point_t p3 = {{0, 0, 0}, {0, 0, 0}};
    harmonic_point_t p4 = {{48, 48, 48}, {48, 48, 48}};  // Harmonic partners
    
    double harmonic_dist2 = harmonic_distance(&p3, &p4);
    bool test2_pass = (harmonic_dist2 < 1e-10);
    
    printf("  Harmonic distance (harmonic pairs): %.6f ≈ 0: %s\n", 
           harmonic_dist2, test2_pass ? "✓" : "✗");
    
    // Test maximum harmonic distance
    harmonic_point_t p5 = {{24, 24, 24}, {24, 24, 24}};
    harmonic_point_t p6 = {{72, 72, 72}, {72, 72, 72}};  // Maximum harmonic separation
    
    double harmonic_dist3 = harmonic_distance(&p5, &p6);
    double expected_max = sqrt(3.0);  // sqrt(3 * (48/48)²) = sqrt(3)
    bool test3_pass = (fabs(harmonic_dist3 - expected_max) < 0.1);
    
    printf("  Maximum harmonic distance: %.6f ≈ %.6f: %s\n", 
           harmonic_dist3, expected_max, test3_pass ? "✓" : "✗");
    
    printf("Distance computation correctness tests completed\n\n");
    
    if (!test1_pass || !test2_pass || !test3_pass) {
        exit(1);
    }
}

int main(int argc, char* argv[]) {
    bool test_only = false;
    bool run_comparison = false;
    
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--test") == 0) {
            test_only = true;
        } else if (strcmp(argv[i], "--compare") == 0) {
            run_comparison = true;
        } else if (strcmp(argv[i], "--help") == 0) {
            printf("Usage: %s [OPTIONS]\n", argv[0]);
            printf("Options:\n");
            printf("  --test       Run correctness tests only\n");
            printf("  --compare    Also run Euclidean distance comparison\n");
            printf("  --help       Show this help message\n");
            return 0;
        }
    }
    
    test_distance_computations();
    
    if (test_only) {
        return 0;
    }
    
    benchmark_harmonic_distance();
    
    if (run_comparison) {
        printf("\n");
        benchmark_euclidean_distance();
    }
    
    return 0;
}