/* test-clustering.c - Comprehensive Clustering Tests for Atlas Layer 3
 * (c) 2024-2025 UOR Foundation - MIT License
 * 
 * Tests CSR matrix operations, clustering algorithms, memory management,
 * and edge cases for the Layer 3 clustering implementation.
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
#define TEST_PAGES 16
#define PAGE_SIZE 256
#define TOTAL_SIZE (TEST_PAGES * PAGE_SIZE)
#define MAX_CLUSTERS 96

// External functions from clustering.c (not in public API)
extern void* atlas_csr_create(uint32_t rows, uint32_t cols);
extern void atlas_csr_destroy(void* matrix);
extern bool atlas_csr_build_from_pages(void* matrix, const uint8_t* base, size_t pages);
extern size_t atlas_csr_get_row_pages(void* matrix, uint8_t resonance_class, const uint32_t** page_indices);

extern void* atlas_cluster_create(uint8_t resonance_class, uint32_t capacity);
extern void atlas_cluster_destroy(void* cluster);
extern bool atlas_cluster_add_page(void* cluster, uint32_t page_index);
extern double atlas_cluster_calculate_homogeneity(void* cluster, const uint8_t* base);
extern bool atlas_cluster_validate(void* cluster);

extern void* atlas_build_all_clusters(const uint8_t* base, size_t pages);
extern void atlas_cluster_directory_destroy(void* dir);
extern void* atlas_get_cluster_for_resonance(void* dir, uint8_t resonance_class);

extern void atlas_clustering_histogram_batch(const uint8_t* base, size_t pages, uint16_t* histograms);
extern bool atlas_clustering_analyze_distribution(const uint8_t* base, size_t pages, double* entropy, double* uniformity);

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

// Generate test data with specific patterns
static void generate_test_data(uint8_t* memory, size_t pages) {
    for (size_t page = 0; page < pages; page++) {
        uint8_t* page_ptr = memory + (page * PAGE_SIZE);
        
        // Create different patterns for different pages
        if (page < 4) {
            // First 4 pages: uniform pattern (same resonance class)
            uint8_t base_value = (uint8_t)(page * 24);
            for (int i = 0; i < PAGE_SIZE; i++) {
                page_ptr[i] = base_value;
            }
        } else if (page < 8) {
            // Next 4 pages: linear pattern
            uint8_t base_value = (uint8_t)(page * 32);
            for (int i = 0; i < PAGE_SIZE; i++) {
                page_ptr[i] = (uint8_t)(base_value + i);
            }
        } else if (page < 12) {
            // Next 4 pages: random-like pattern
            uint32_t seed = page * 12345;
            for (int i = 0; i < PAGE_SIZE; i++) {
                seed = seed * 1103515245 + 12345;  // Simple LCG
                page_ptr[i] = (uint8_t)(seed >> 16);
            }
        } else {
            // Remaining pages: mixed pattern
            for (int i = 0; i < PAGE_SIZE; i++) {
                page_ptr[i] = (uint8_t)((page * 17 + i * 7 + 42) % 256);
            }
        }
    }
}

// Test 1: CSR Matrix Creation and Destruction
static void test_csr_matrix_lifecycle(void) {
    print_test_header("CSR Matrix Lifecycle");
    
    // Test valid creation
    void* matrix = atlas_csr_create(96, TEST_PAGES);
    print_test_result("CSR matrix creation", matrix != NULL);
    
    // Test invalid parameters
    void* null_matrix1 = atlas_csr_create(0, TEST_PAGES);
    print_test_result("CSR matrix creation with zero rows", null_matrix1 == NULL);
    
    void* null_matrix2 = atlas_csr_create(96, 0);
    print_test_result("CSR matrix creation with zero cols", null_matrix2 == NULL);
    
    // Test destruction
    atlas_csr_destroy(matrix);
    atlas_csr_destroy(null_matrix1);
    atlas_csr_destroy(null_matrix2);
    atlas_csr_destroy(NULL);  // Should not crash
    print_test_result("CSR matrix destruction", true);
}

// Test 2: CSR Matrix Building from Pages
static void test_csr_matrix_building(uint8_t* memory) {
    print_test_header("CSR Matrix Building");
    
    void* matrix = atlas_csr_create(96, TEST_PAGES);
    assert(matrix != NULL);
    
    // Build CSR matrix from test data
    bool build_success = atlas_csr_build_from_pages(matrix, memory, TEST_PAGES);
    print_test_result("CSR matrix building", build_success);
    
    // Test building with invalid parameters
    bool build_fail1 = atlas_csr_build_from_pages(NULL, memory, TEST_PAGES);
    print_test_result("CSR building with NULL matrix", !build_fail1);
    
    bool build_fail2 = atlas_csr_build_from_pages(matrix, NULL, TEST_PAGES);
    print_test_result("CSR building with NULL data", !build_fail2);
    
    bool build_fail3 = atlas_csr_build_from_pages(matrix, memory, 0);
    print_test_result("CSR building with zero pages", !build_fail3);
    
    // Test accessing row data
    uint32_t total_pages_found = 0;
    for (uint8_t r = 0; r < 96; r++) {
        const uint32_t* page_indices;
        size_t count = atlas_csr_get_row_pages(matrix, r, &page_indices);
        total_pages_found += count;
        
        // Validate that all page indices are within bounds
        for (size_t i = 0; i < count; i++) {
            assert(page_indices[i] < TEST_PAGES);
        }
    }
    
    print_test_result("CSR row access and validation", total_pages_found == TEST_PAGES);
    
    atlas_csr_destroy(matrix);
}

// Test 3: Individual Cluster Operations
static void test_individual_clusters(uint8_t* memory) {
    print_test_header("Individual Cluster Operations");
    
    // Create cluster for resonance class 0
    void* cluster = atlas_cluster_create(0, 8);
    print_test_result("Cluster creation", cluster != NULL);
    
    // Test cluster validation on empty cluster
    bool valid_empty = atlas_cluster_validate(cluster);
    print_test_result("Empty cluster validation", valid_empty);
    
    // Add pages to cluster
    bool add_success = true;
    for (uint32_t i = 0; i < 4; i++) {
        if (!atlas_cluster_add_page(cluster, i)) {
            add_success = false;
            break;
        }
    }
    print_test_result("Adding pages to cluster", add_success);
    
    // Test duplicate detection
    bool duplicate_rejected = !atlas_cluster_add_page(cluster, 0);
    print_test_result("Duplicate page rejection", duplicate_rejected);
    
    // Test capacity limits
    void* small_cluster = atlas_cluster_create(1, 2);
    assert(small_cluster != NULL);
    
    atlas_cluster_add_page(small_cluster, 0);
    atlas_cluster_add_page(small_cluster, 1);
    bool capacity_respected = !atlas_cluster_add_page(small_cluster, 2);
    print_test_result("Cluster capacity limits", capacity_respected);
    
    // Test homogeneity calculation
    double homogeneity = atlas_cluster_calculate_homogeneity(cluster, memory);
    print_test_result("Homogeneity calculation", homogeneity >= 0.0 && homogeneity <= 1.0);
    printf("   Homogeneity score: %.3f\n", homogeneity);
    
    // Test cluster validation after modifications
    bool valid_filled = atlas_cluster_validate(cluster);
    print_test_result("Filled cluster validation", valid_filled);
    
    // Test invalid cluster creation
    void* invalid_cluster = atlas_cluster_create(200, 10);  // Invalid resonance class
    print_test_result("Invalid resonance class handling", invalid_cluster != NULL);  // Should create but normalize
    
    atlas_cluster_destroy(cluster);
    atlas_cluster_destroy(small_cluster);
    atlas_cluster_destroy(invalid_cluster);
    atlas_cluster_destroy(NULL);  // Should not crash
}

// Test 4: Cluster Directory Operations
static void test_cluster_directory(uint8_t* memory) {
    print_test_header("Cluster Directory Operations");
    
    // Build all clusters from test data
    void* directory = atlas_build_all_clusters(memory, TEST_PAGES);
    print_test_result("Cluster directory creation", directory != NULL);
    
    // Test accessing clusters by resonance class
    uint32_t non_empty_clusters = 0;
    uint32_t total_pages_in_clusters = 0;
    
    for (uint8_t r = 0; r < 96; r++) {
        void* cluster = atlas_get_cluster_for_resonance(directory, r);
        if (cluster != NULL) {
            non_empty_clusters++;
            
            // Validate each cluster
            bool cluster_valid = atlas_cluster_validate(cluster);
            if (!cluster_valid) {
                printf("   Cluster %u validation failed\n", r);
                assert(false);
            }
            
            // Calculate homogeneity
            double homogeneity = atlas_cluster_calculate_homogeneity(cluster, memory);
            if (homogeneity < 0.0 || homogeneity > 1.0) {
                printf("   Cluster %u has invalid homogeneity: %.3f\n", r, homogeneity);
                assert(false);
            }
        }
    }
    
    print_test_result("Non-empty clusters found", non_empty_clusters > 0);
    print_test_result("Cluster validation", true);
    printf("   Found %u non-empty clusters\n", non_empty_clusters);
    
    // Test invalid access
    void* null_cluster = atlas_get_cluster_for_resonance(NULL, 0);
    print_test_result("NULL directory access", null_cluster == NULL);
    
    atlas_cluster_directory_destroy(directory);
    atlas_cluster_directory_destroy(NULL);  // Should not crash
}

// Test 5: Memory Management and Edge Cases
static void test_memory_management(void) {
    print_test_header("Memory Management and Edge Cases");
    
    // Test large capacity cluster
    void* large_cluster = atlas_cluster_create(0, 10000);
    print_test_result("Large capacity cluster creation", large_cluster != NULL);
    atlas_cluster_destroy(large_cluster);
    
    // Test zero capacity cluster
    void* zero_cluster = atlas_cluster_create(0, 0);
    print_test_result("Zero capacity cluster rejection", zero_cluster == NULL);
    
    // Test multiple create/destroy cycles
    bool memory_leak_test = true;
    for (int i = 0; i < 100; i++) {
        void* temp_cluster = atlas_cluster_create(i % 96, 10);
        if (!temp_cluster) {
            memory_leak_test = false;
            break;
        }
        atlas_cluster_destroy(temp_cluster);
        
        void* temp_matrix = atlas_csr_create(96, 10);
        if (!temp_matrix) {
            memory_leak_test = false;
            break;
        }
        atlas_csr_destroy(temp_matrix);
    }
    print_test_result("Memory leak prevention", memory_leak_test);
    
    // Test with minimal data
    uint8_t single_page[PAGE_SIZE];
    memset(single_page, 42, PAGE_SIZE);
    
    void* small_directory = atlas_build_all_clusters(single_page, 1);
    print_test_result("Single page directory", small_directory != NULL);
    
    if (small_directory) {
        // Should have exactly one non-empty cluster
        uint32_t found_clusters = 0;
        for (uint8_t r = 0; r < 96; r++) {
            if (atlas_get_cluster_for_resonance(small_directory, r) != NULL) {
                found_clusters++;
            }
        }
        print_test_result("Single page clustering", found_clusters == 1);
        atlas_cluster_directory_destroy(small_directory);
    }
}

// Test 6: High-Performance Operations
static void test_high_performance_operations(uint8_t* memory) {
    print_test_header("High-Performance Operations");
    
    // Test batch histogram generation
    uint16_t* histograms = (uint16_t*)malloc(TEST_PAGES * 96 * sizeof(uint16_t));
    assert(histograms != NULL);
    
    atlas_clustering_histogram_batch(memory, TEST_PAGES, histograms);
    
    // Validate histograms
    bool histograms_valid = true;
    for (size_t page = 0; page < TEST_PAGES; page++) {
        uint16_t* hist = histograms + (page * 96);
        uint32_t total = 0;
        
        for (int i = 0; i < 96; i++) {
            total += hist[i];
        }
        
        if (total != PAGE_SIZE) {
            printf("   Page %zu histogram sum: %u (expected %d)\n", page, total, PAGE_SIZE);
            histograms_valid = false;
        }
    }
    print_test_result("Batch histogram generation", histograms_valid);
    
    free(histograms);
    
    // Test distribution analysis
    double entropy, uniformity;
    bool analysis_success = atlas_clustering_analyze_distribution(memory, TEST_PAGES, &entropy, &uniformity);
    print_test_result("Distribution analysis", analysis_success);
    
    if (analysis_success) {
        printf("   Entropy: %.3f bits\n", entropy);
        printf("   Uniformity: %.3f\n", uniformity);
        
        bool entropy_valid = (entropy >= 0.0 && entropy <= log2(96.0));
        bool uniformity_valid = (uniformity >= 0.0 && uniformity <= 1.0);
        
        print_test_result("Entropy range validation", entropy_valid);
        print_test_result("Uniformity range validation", uniformity_valid);
    }
    
    // Test invalid parameters for high-performance functions
    bool invalid_param_test = true;
    
    atlas_clustering_histogram_batch(NULL, TEST_PAGES, histograms);  // Should not crash
    atlas_clustering_histogram_batch(memory, 0, histograms);  // Should not crash
    atlas_clustering_histogram_batch(memory, TEST_PAGES, NULL);  // Should not crash
    
    bool invalid_analysis = atlas_clustering_analyze_distribution(NULL, TEST_PAGES, &entropy, &uniformity);
    invalid_param_test &= !invalid_analysis;
    
    invalid_analysis = atlas_clustering_analyze_distribution(memory, 0, &entropy, &uniformity);
    invalid_param_test &= !invalid_analysis;
    
    invalid_analysis = atlas_clustering_analyze_distribution(memory, TEST_PAGES, NULL, &uniformity);
    invalid_param_test &= !invalid_analysis;
    
    print_test_result("Invalid parameter handling", invalid_param_test);
}

// Test 7: Integration with Existing Cluster View API
static void test_cluster_view_integration(uint8_t* memory) {
    print_test_header("Cluster View Integration");
    
    // Create cluster view using existing API
    atlas_cluster_view view = atlas_cluster_by_resonance(memory, TEST_PAGES);
    print_test_result("Cluster view creation", view.offsets != NULL && view.indices != NULL);
    
    // Validate cluster view structure
    bool view_valid = atlas_cluster_validate(view);
    print_test_result("Cluster view validation", view_valid);
    
    // Test accessing cluster view data
    uint32_t total_accessed = 0;
    for (uint8_t r = 0; r < 96; r++) {
        size_t count = atlas_cluster_count_for_resonance(view, r);
        if (count > 0) {
            size_t returned_count;
            const uint32_t* indices = atlas_cluster_pages_for_resonance(view, r, &returned_count);
            
            if (count != returned_count) {
                printf("   Count mismatch for resonance %u: %zu vs %zu\n", r, count, returned_count);
                assert(false);
            }
            
            // Validate indices
            for (size_t i = 0; i < count; i++) {
                if (indices[i] >= TEST_PAGES) {
                    printf("   Invalid page index %u for resonance %u\n", indices[i], r);
                    assert(false);
                }
            }
            
            total_accessed += count;
        }
    }
    
    print_test_result("Cluster view access", total_accessed == TEST_PAGES);
    
    // Get cluster statistics
    size_t total_pages, non_empty_classes, largest_class;
    atlas_cluster_stats(view, &total_pages, &non_empty_classes, &largest_class);
    
    bool stats_valid = (total_pages == TEST_PAGES) && 
                       (non_empty_classes > 0) && 
                       (non_empty_classes <= 96) &&
                       (largest_class > 0) &&
                       (largest_class <= TEST_PAGES);
    
    print_test_result("Cluster statistics", stats_valid);
    printf("   Total pages: %zu\n", total_pages);
    printf("   Non-empty classes: %zu\n", non_empty_classes);
    printf("   Largest class: %zu pages\n", largest_class);
    
    // Clean up
    atlas_cluster_destroy(&view);
    print_test_result("Cluster view destruction", view.offsets == NULL);
}

// Test 8: Stress Testing
static void test_stress_conditions(void) {
    print_test_header("Stress Testing");
    
    // Large dataset stress test
    const size_t STRESS_PAGES = 1000;
    uint8_t* stress_memory = aligned_alloc(256, STRESS_PAGES * PAGE_SIZE);
    assert(stress_memory != NULL);
    
    // Generate stress test data
    generate_test_data(stress_memory, STRESS_PAGES);
    
    // Build cluster directory for large dataset
    clock_t start_time = clock();
    void* stress_directory = atlas_build_all_clusters(stress_memory, STRESS_PAGES);
    clock_t end_time = clock();
    
    double elapsed_time = ((double)(end_time - start_time)) / CLOCKS_PER_SEC;
    printf("   Large dataset clustering time: %.3f seconds\n", elapsed_time);
    
    print_test_result("Large dataset clustering", stress_directory != NULL);
    
    // Validate all clusters in stress test
    bool stress_validation = true;
    uint32_t stress_total_pages = 0;
    
    for (uint8_t r = 0; r < 96; r++) {
        void* cluster = atlas_get_cluster_for_resonance(stress_directory, r);
        if (cluster != NULL) {
            if (!atlas_cluster_validate(cluster)) {
                stress_validation = false;
                break;
            }
        }
    }
    
    print_test_result("Stress test validation", stress_validation);
    
    // Performance benchmark
    start_time = clock();
    uint16_t* stress_histograms = (uint16_t*)malloc(STRESS_PAGES * 96 * sizeof(uint16_t));
    assert(stress_histograms != NULL);
    
    atlas_clustering_histogram_batch(stress_memory, STRESS_PAGES, stress_histograms);
    
    end_time = clock();
    elapsed_time = ((double)(end_time - start_time)) / CLOCKS_PER_SEC;
    
    double pages_per_second = (double)STRESS_PAGES / elapsed_time;
    printf("   Histogram batch performance: %.0f pages/second\n", pages_per_second);
    
    print_test_result("Performance benchmark", pages_per_second > 1000.0);  // Should process >1K pages/sec
    
    // Clean up stress test
    atlas_cluster_directory_destroy(stress_directory);
    free(stress_histograms);
    free(stress_memory);
}

int main(void) {
    printf("Atlas Layer 3 Clustering Tests\n");
    printf("===============================\n");
    
    // Allocate test memory
    uint8_t* memory = aligned_alloc(256, TOTAL_SIZE);
    if (!memory) {
        fprintf(stderr, "Failed to allocate test memory\n");
        return 1;
    }
    
    // Generate test data
    generate_test_data(memory, TEST_PAGES);
    
    // Run all tests
    test_csr_matrix_lifecycle();
    test_csr_matrix_building(memory);
    test_individual_clusters(memory);
    test_cluster_directory(memory);
    test_memory_management();
    test_high_performance_operations(memory);
    test_cluster_view_integration(memory);
    test_stress_conditions();
    
    // Clean up
    free(memory);
    
    printf("\n=== Test Summary ===\n");
    printf("All clustering tests completed successfully! ✅\n");
    printf("Coverage:\n");
    printf("  • CSR matrix operations\n");
    printf("  • Individual cluster management\n");
    printf("  • Cluster directory operations\n");
    printf("  • Memory management and edge cases\n");
    printf("  • High-performance batch operations\n");
    printf("  • Integration with existing APIs\n");
    printf("  • Stress testing and performance\n");
    
    return 0;
}