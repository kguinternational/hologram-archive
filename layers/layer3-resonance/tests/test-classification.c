/* test-layer3.c - Test Layer 3 clustering and scheduling functionality
 * (c) 2024-2025 UOR Foundation - MIT License
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <stdint.h>

#include "atlas-runtime.h"

// Test data: 3 pages of 256 bytes each (total 768 bytes)
#define TEST_PAGES 3
#define PAGE_SIZE 256
#define TOTAL_SIZE (TEST_PAGES * PAGE_SIZE)

int main() {
    printf("Atlas Layer 3 Runtime Test\n");
    printf("==========================\n\n");

    // Allocate test memory aligned to 256-byte pages
    uint8_t* memory = aligned_alloc(256, TOTAL_SIZE);
    if (!memory) {
        fprintf(stderr, "Failed to allocate test memory\n");
        return 1;
    }

    // Initialize test data with patterns
    for (int page = 0; page < TEST_PAGES; page++) {
        uint8_t* page_ptr = memory + (page * PAGE_SIZE);
        for (int i = 0; i < PAGE_SIZE; i++) {
            // Different patterns for each page to get different resonance classes
            page_ptr[i] = (uint8_t)((page * 37 + i * 7) % 256);
        }
    }

    printf("1. Testing R96 page classification...\n");
    
    // Test page classification
    uint8_t classification[PAGE_SIZE];
    atlas_r96_classify_page(memory, classification);
    printf("   ✓ Page classification completed\n");

    // Test histogram generation
    uint16_t histogram[96];
    atlas_r96_histogram_page(memory, histogram);
    
    // Check that histogram sums to 256 (one entry per byte)
    uint32_t total_count = 0;
    for (int i = 0; i < 96; i++) {
        total_count += histogram[i];
    }
    assert(total_count == PAGE_SIZE);
    printf("   ✓ Histogram generation completed (sum=%u)\n", total_count);

    printf("\n2. Testing resonance class determination...\n");
    
    // Test resonance class for each page
    for (int page = 0; page < TEST_PAGES; page++) {
        uint8_t* page_ptr = memory + (page * PAGE_SIZE);
        uint8_t resonance = atlas_page_resonance_class(page_ptr);
        printf("   Page %d: resonance class %u\n", page, resonance);
        assert(resonance < 96);  // Must be in valid range
    }

    printf("\n3. Testing cluster building...\n");
    
    // Build cluster view
    atlas_cluster_view cluster = atlas_cluster_by_resonance(memory, TEST_PAGES);
    assert(cluster.data != NULL);
    printf("   ✓ Cluster view created successfully\n");

    // Validate cluster structure
    assert(atlas_cluster_validate(cluster));
    printf("   ✓ Cluster view structure is valid\n");

    // Get cluster statistics
    size_t total_pages, non_empty_classes, largest_class;
    atlas_cluster_stats(cluster, &total_pages, &non_empty_classes, &largest_class);
    printf("   Total pages: %zu\n", total_pages);
    printf("   Non-empty resonance classes: %zu\n", non_empty_classes);
    printf("   Largest resonance class: %zu pages\n", largest_class);
    
    assert(total_pages == TEST_PAGES);
    assert(non_empty_classes > 0 && non_empty_classes <= 96);

    printf("\n4. Testing cluster access...\n");
    
    // Test accessing pages by resonance class
    for (uint8_t r = 0; r < 96; r++) {
        size_t count = atlas_cluster_count_for_resonance(cluster, r);
        if (count > 0) {
            const uint32_t* page_indices = atlas_cluster_pages_for_resonance(cluster, r, &count);
            assert(page_indices != NULL);
            printf("   Resonance class %u: %zu pages\n", r, count);
            
            // Verify all page indices are valid
            for (size_t i = 0; i < count; i++) {
                assert(page_indices[i] < TEST_PAGES);
            }
        }
    }

    printf("\n5. Testing harmonic scheduling...\n");
    
    // Test scheduling calculations
    uint64_t current_time = 1000;
    for (uint8_t r = 0; r < 8; r++) {  // Test first 8 resonance classes
        uint64_t next_harmonic = atlas_next_harmonic_window_from(current_time, r);
        uint64_t next_simple = atlas_schedule_next_window(current_time, r);
        
        printf("   R=%u: harmonic=%llu, simple=%llu\n", r, 
               (unsigned long long)next_harmonic,
               (unsigned long long)next_simple);
        
        // Both should be >= current_time
        assert(next_harmonic >= current_time);
        assert(next_simple >= current_time);
    }

    printf("\n6. Testing resonance harmonization...\n");
    
    // Test resonance harmonization
    bool harmonic_found = false;
    for (uint8_t r1 = 0; r1 < 16; r1++) {
        for (uint8_t r2 = 0; r2 < 16; r2++) {
            if (atlas_resonance_harmonizes(r1, r2)) {
                printf("   Resonance classes %u and %u harmonize\n", r1, r2);
                harmonic_found = true;
                break;
            }
        }
        if (harmonic_found) break;
    }

    printf("\n7. Testing batch processing...\n");
    
    // Test batch classification
    uint8_t classifications[TEST_PAGES];
    atlas_r96_classify_pages(memory, TEST_PAGES, classifications);
    
    for (int page = 0; page < TEST_PAGES; page++) {
        printf("   Page %d: batch resonance class %u\n", page, classifications[page]);
        assert(classifications[page] < 96);
    }

    // Test batch histogram generation
    uint16_t histograms[TEST_PAGES * 96];
    atlas_r96_histogram_pages(memory, TEST_PAGES, histograms);
    
    for (int page = 0; page < TEST_PAGES; page++) {
        uint16_t* hist = &histograms[page * 96];
        uint32_t page_total = 0;
        for (int i = 0; i < 96; i++) {
            page_total += hist[i];
        }
        assert(page_total == PAGE_SIZE);
        printf("   Page %d: histogram sum = %u ✓\n", page, page_total);
    }

    printf("\n8. Cleanup...\n");
    
    // Clean up
    atlas_cluster_destroy(&cluster);
    assert(cluster.data == NULL);  // Should be cleared
    printf("   ✓ Cluster view destroyed\n");

    free(memory);
    printf("   ✓ Memory freed\n");

    printf("\nAll Layer 3 tests passed! ✅\n");
    return 0;
}