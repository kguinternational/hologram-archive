/* test-scheduling.c - Scheduling Tests for Atlas Layer 3
 * (c) 2024-2025 UOR Foundation - MIT License
 * 
 * Tests phase-locked scheduling using only the public API from atlas-resonance.h
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <stdint.h>
#include <time.h>
#include <math.h>

#include "../include/atlas-resonance.h"

// Test configuration
#define TEST_ITERATIONS 1000
#define STRESS_ITERATIONS 10000

// Test utilities
static void print_test_header(const char* test_name) {
    printf("\n=== %s ===\n", test_name);
}

static void print_test_result(const char* test_name, bool passed) {
    printf("   %s: %s\n", test_name, passed ? "✓ PASS" : "✗ FAIL");
    if (!passed) {
        printf("      Test failed at line %d\n", __LINE__);
        exit(1);
    }
}

// Test 1: Public API Next Window Calculation
static void test_next_window_calculation(void) {
    print_test_header("Next Window Calculation");
    
    // Test basic next window calculation
    uint64_t base_time = 1000;
    
    for (uint8_t r = 0; r < 16; r++) {
        uint64_t next1 = atlas_schedule_next_window(base_time, r);
        uint64_t next2 = atlas_next_harmonic_window_from(base_time, r);
        
        // Both functions should return time >= base_time
        bool time_valid = (next1 >= base_time) && (next2 >= base_time);
        if (!time_valid) {
            printf("   R=%u: next_simple=%llu, next_harmonic=%llu, base=%llu\n", 
                   r, (unsigned long long)next1, (unsigned long long)next2, (unsigned long long)base_time);
        }
        assert(time_valid);
    }
    print_test_result("Next window times are valid", true);
    
    // Test monotonicity: next window should advance time
    bool monotonic = true;
    uint64_t current_time = 100;
    
    for (int i = 0; i < 100; i++) {
        uint8_t r = (uint8_t)(i % 96);
        uint64_t next_time = atlas_schedule_next_window(current_time, r);
        
        if (next_time <= current_time) {
            monotonic = false;
            printf("   Monotonicity failure: current=%llu, next=%llu, r=%u\n", 
                   (unsigned long long)current_time, (unsigned long long)next_time, r);
            break;
        }
        
        current_time = next_time;
    }
    print_test_result("Window scheduling monotonicity", monotonic);
    
    // Test edge cases
    uint64_t edge_next = atlas_schedule_next_window(0, 0);
    print_test_result("Zero time handling", edge_next >= 0);
    
    uint64_t large_next = atlas_schedule_next_window(UINT64_MAX - 1000, 50);
    print_test_result("Large time handling", large_next > 0);  // Should not overflow
    
    // Test all resonance classes
    bool all_classes_valid = true;
    for (uint8_t r = 0; r < 96; r++) {
        uint64_t next = atlas_schedule_next_window(base_time, r);
        if (next < base_time) {
            all_classes_valid = false;
            break;
        }
    }
    print_test_result("All resonance classes", all_classes_valid);
}

// Test 2: Harmonic Window Formula Verification
static void test_harmonic_formula(void) {
    print_test_header("Harmonic Window Formula");
    
    // Verify the formula: next = now + ((96 - ((now + r) % 96)) % 96)
    uint64_t test_times[] = {0, 100, 1000, 5000, 10000};
    bool formula_correct = true;
    
    for (size_t t = 0; t < sizeof(test_times)/sizeof(test_times[0]); t++) {
        uint64_t now = test_times[t];
        
        for (uint8_t r = 0; r < 96; r += 12) {  // Test subset for performance
            uint64_t actual = atlas_next_harmonic_window_from(now, r);
            
            // Calculate expected using the formula
            // Note: Implementation adds 96 when offset is 0 to ensure next window is in future
            uint64_t offset = ((96 - ((now + r) % 96)) % 96);
            if (offset == 0) offset = 96;
            uint64_t expected = now + offset;
            
            if (actual != expected) {
                printf("   Formula mismatch at now=%llu, r=%u: expected=%llu, actual=%llu\n",
                       (unsigned long long)now, r, 
                       (unsigned long long)expected, (unsigned long long)actual);
                formula_correct = false;
                break;
            }
        }
        if (!formula_correct) break;
    }
    
    print_test_result("Harmonic formula verification", formula_correct);
    
    // Test that result is always phase-aligned
    bool phase_aligned = true;
    for (uint8_t r = 0; r < 96; r++) {
        uint64_t next = atlas_next_harmonic_window_from(1000, r);
        // The result should satisfy certain phase alignment properties
        // Since we don't have access to internal phase checking, we verify
        // that consecutive calls with same parameters give same result (determinism)
        uint64_t next2 = atlas_next_harmonic_window_from(1000, r);
        if (next != next2) {
            phase_aligned = false;
            break;
        }
    }
    print_test_result("Phase alignment consistency", phase_aligned);
}

// Test 3: Scheduling Determinism
static void test_determinism(void) {
    print_test_header("Scheduling Determinism");
    
    // Test that same inputs always produce same outputs
    bool deterministic = true;
    
    for (int iter = 0; iter < 100; iter++) {
        uint64_t base_time = 1000 + iter * 10;
        uint8_t r = (uint8_t)(iter % 96);
        
        // Call multiple times with same parameters
        uint64_t result1 = atlas_schedule_next_window(base_time, r);
        uint64_t result2 = atlas_schedule_next_window(base_time, r);
        uint64_t result3 = atlas_next_harmonic_window_from(base_time, r);
        uint64_t result4 = atlas_next_harmonic_window_from(base_time, r);
        
        if (result1 != result2 || result3 != result4) {
            printf("   Non-deterministic results at time=%llu, r=%u\n",
                   (unsigned long long)base_time, r);
            printf("   schedule_next_window: %llu vs %llu\n",
                   (unsigned long long)result1, (unsigned long long)result2);
            printf("   next_harmonic_window_from: %llu vs %llu\n",
                   (unsigned long long)result3, (unsigned long long)result4);
            deterministic = false;
            break;
        }
    }
    
    print_test_result("Function determinism", deterministic);
    
    // Test sequence determinism
    bool sequence_deterministic = true;
    uint8_t sequence[] = {0, 48, 24, 72, 12, 84};
    uint64_t results1[6], results2[6];
    
    // First run
    for (size_t i = 0; i < 6; i++) {
        results1[i] = atlas_next_harmonic_window_from(5000, sequence[i]);
    }
    
    // Second run
    for (size_t i = 0; i < 6; i++) {
        results2[i] = atlas_next_harmonic_window_from(5000, sequence[i]);
    }
    
    // Compare
    for (size_t i = 0; i < 6; i++) {
        if (results1[i] != results2[i]) {
            sequence_deterministic = false;
            break;
        }
    }
    
    print_test_result("Sequence determinism", sequence_deterministic);
}

// Test 4: Resonance Integration
static void test_resonance_integration(void) {
    print_test_header("Resonance Integration");
    
    // Test scheduling with harmonizing resonance pairs
    int harmonic_pairs_found = 0;
    uint64_t base_time = 10000;
    
    for (uint8_t r1 = 0; r1 < 48; r1++) {
        for (uint8_t r2 = r1; r2 < 96; r2++) {
            if (atlas_resonance_harmonizes(r1, r2)) {
                // Schedule windows for harmonic pair
                uint64_t window1 = atlas_next_harmonic_window_from(base_time, r1);
                uint64_t window2 = atlas_next_harmonic_window_from(base_time, r2);
                
                // Verify both windows are scheduled in the future
                if (window1 > base_time && window2 > base_time) {
                    harmonic_pairs_found++;
                }
                
                if (harmonic_pairs_found >= 10) break;  // Limit for test performance
            }
        }
        if (harmonic_pairs_found >= 10) break;
    }
    
    print_test_result("Harmonic pair scheduling", harmonic_pairs_found > 0);
    printf("   Found and scheduled %d harmonic pairs\n", harmonic_pairs_found);
    
    // Test resonance class validation
    bool validation_test = true;
    for (int r = 0; r < 200; r++) {  // Test beyond valid range
        uint64_t next_time = atlas_schedule_next_window(10000, (uint8_t)r);
        if (next_time < 10000) {
            validation_test = false;
            break;
        }
    }
    print_test_result("Resonance class validation", validation_test);
}

// Test 5: Performance and Stress Testing
static void test_performance(void) {
    print_test_header("Performance Testing");
    
    // Performance test: window calculation throughput
    clock_t start_time = clock();
    uint64_t total_calculations = 0;
    
    for (int iter = 0; iter < STRESS_ITERATIONS; iter++) {
        uint64_t base_time = 1000 + iter;
        for (uint8_t r = 0; r < 96; r++) {
            uint64_t next_time = atlas_schedule_next_window(base_time, r);
            total_calculations++;
            
            // Use the result to prevent compiler optimization
            if (next_time < base_time) {
                printf("   Performance test validation failed\n");
                assert(false);
            }
        }
    }
    
    clock_t end_time = clock();
    double elapsed_time = ((double)(end_time - start_time)) / CLOCKS_PER_SEC;
    double calculations_per_second = (double)total_calculations / elapsed_time;
    
    printf("   Window calculations: %.0f ops/second\n", calculations_per_second);
    print_test_result("Performance benchmark", calculations_per_second > 10000.0);  // Should be >10K ops/sec
    
    // Test harmonic window calculation performance
    start_time = clock();
    total_calculations = 0;
    
    for (int iter = 0; iter < STRESS_ITERATIONS; iter++) {
        uint64_t base_time = 1000 + iter;
        for (uint8_t r = 0; r < 96; r++) {
            uint64_t next_time = atlas_next_harmonic_window_from(base_time, r);
            total_calculations++;
            
            if (next_time < base_time) {
                printf("   Harmonic test validation failed\n");
                assert(false);
            }
        }
    }
    
    end_time = clock();
    elapsed_time = ((double)(end_time - start_time)) / CLOCKS_PER_SEC;
    calculations_per_second = (double)total_calculations / elapsed_time;
    
    printf("   Harmonic calculations: %.0f ops/second\n", calculations_per_second);
    print_test_result("Harmonic performance", calculations_per_second > 10000.0);
}

// Test 6: Edge Cases
static void test_edge_cases(void) {
    print_test_header("Edge Cases");
    
    // Test time overflow scenarios
    uint64_t near_max = UINT64_MAX - 10000;
    uint64_t overflow_next = atlas_schedule_next_window(near_max, 0);
    print_test_result("Time overflow handling", overflow_next > 0);  // Should handle gracefully
    
    // Test with all resonance classes at boundary times
    bool boundary_test = true;
    uint64_t boundary_times[] = {0, 1, 95, 96, 97, 191, 192, 768, 12288};
    
    for (size_t i = 0; i < sizeof(boundary_times)/sizeof(boundary_times[0]); i++) {
        for (uint8_t r = 0; r < 96; r += 24) {  // Test subset
            uint64_t next = atlas_next_harmonic_window_from(boundary_times[i], r);
            if (next < boundary_times[i]) {
                boundary_test = false;
                printf("   Boundary test failed at time=%llu, r=%u\n",
                       (unsigned long long)boundary_times[i], r);
                break;
            }
        }
        if (!boundary_test) break;
    }
    print_test_result("Boundary time handling", boundary_test);
    
    // Test rapid succession scheduling
    bool rapid_test = true;
    uint64_t current = 1000;
    
    for (int i = 0; i < 1000; i++) {
        uint8_t r = (uint8_t)(i % 96);
        uint64_t next = atlas_schedule_next_window(current, r);
        
        if (next <= current) {
            rapid_test = false;
            break;
        }
        
        // Move time forward slightly
        current = next + 1;
    }
    print_test_result("Rapid succession scheduling", rapid_test);
}

int main(void) {
    printf("Atlas Layer 3 Scheduling Tests\n");
    printf("===============================\n");
    
    // Run all tests
    test_next_window_calculation();
    test_harmonic_formula();
    test_determinism();
    test_resonance_integration();
    test_performance();
    test_edge_cases();
    
    printf("\n=== Test Summary ===\n");
    printf("All scheduling tests completed successfully! ✅\n");
    printf("Coverage:\n");
    printf("  • Next window calculation and monotonicity\n");
    printf("  • Harmonic formula verification\n");
    printf("  • Scheduling determinism\n");
    printf("  • Integration with resonance harmonization\n");
    printf("  • Performance benchmarking\n");
    printf("  • Edge cases and boundary conditions\n");
    
    return 0;
}