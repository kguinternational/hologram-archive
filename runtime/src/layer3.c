/* layer3.c - Atlas-12288 Layer 3 Host Runtime with C ABI for Clustering and Scheduling
 * (c) 2024-2025 UOR Foundation - MIT License
 * 
 * Layer 3 runtime implementing CSR-based clustering, harmonic scheduling,
 * spectra & histogram functions, and memory management for cluster views.
 */

#include <stdint.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

// Include the public API header
#include "atlas-runtime.h"

// =============================================================================
// Cluster View Structure
// =============================================================================

typedef struct {
    uint32_t magic;                    // 0xA71A5C1U (ATLASClU)
    
    // CSR format data
    uint32_t* offsets;                 // offsets[97] - start indices for each resonance class
    uint32_t* indices;                 // indices[n] - page indices grouped by resonance
    
    // Metadata
    size_t total_pages;                // Total number of pages in the view
    size_t total_indices;              // Total number of entries in indices array
    const uint8_t* base_ptr;           // Base pointer to original memory
    
    // Memory management
    void* arena_ptr;                   // Simple arena for WASM compatibility
    size_t arena_size;                 // Arena size for cleanup
} atlas_cluster_view_internal;

#define ATLAS_CLUSTER_MAGIC 0xA71A5C1U

// =============================================================================
// LLVM IR Function Declarations for Layer 3
// =============================================================================

// From atlas-12288-c768.ll
extern void atlas_r96_classify_page_llvm(const uint8_t* in256, uint8_t out256[256]);
extern void atlas_r96_histogram_page_llvm(const uint8_t* in256, uint16_t out96[96]);

// From atlas-12288-harmonic.ll  
extern uint64_t atlas_next_harmonic_window_from_llvm(uint64_t now, uint8_t r);
extern bool atlas_resonance_harmonizes_llvm(uint8_t r1, uint8_t r2);

// From atlas-12288-morphisms.ll
extern uint8_t atlas_page_resonance_class_llvm(const uint8_t* page256);

// =============================================================================
// Memory Management Functions (WASM-compatible arena)
// =============================================================================

static void* atlas_arena_alloc(size_t size) {
    // Simple aligned allocation for WASM compatibility
    return aligned_alloc(32, (size + 31) & ~31);
}

static void atlas_arena_free(void* ptr) {
    if (ptr) {
        free(ptr);
    }
}

// =============================================================================
// Layer 3 Public API Functions
// =============================================================================

void atlas_r96_classify_page(const uint8_t* in256, uint8_t out256[256]) {
    // Validate inputs
    if (!in256 || !out256) {
        return;
    }
    
    // Call LLVM IR implementation
    atlas_r96_classify_page_llvm(in256, out256);
}

void atlas_r96_histogram_page(const uint8_t* in256, uint16_t out96[96]) {
    // Validate inputs
    if (!in256 || !out96) {
        return;
    }
    
    // Initialize output array
    memset(out96, 0, 96 * sizeof(uint16_t));
    
    // Call LLVM IR implementation
    atlas_r96_histogram_page_llvm(in256, out96);
}

atlas_cluster_view atlas_cluster_by_resonance(const uint8_t* base, size_t pages) {
    // Validate inputs
    if (!base || pages == 0) {
        return (atlas_cluster_view){ .data = NULL };
    }
    
    // Allocate cluster view structure
    atlas_cluster_view_internal* view = atlas_arena_alloc(sizeof(atlas_cluster_view_internal));
    if (!view) {
        return (atlas_cluster_view){ .data = NULL };
    }
    
    // Initialize structure
    memset(view, 0, sizeof(atlas_cluster_view_internal));
    view->magic = ATLAS_CLUSTER_MAGIC;
    view->total_pages = pages;
    view->base_ptr = base;
    
    // Calculate arena size needed for CSR arrays
    size_t offsets_size = 97 * sizeof(uint32_t);  // 96 resonance classes + 1 terminator
    size_t indices_size = pages * sizeof(uint32_t);  // Worst case: all pages in one class
    view->arena_size = offsets_size + indices_size;
    
    // Allocate arena for CSR data
    view->arena_ptr = atlas_arena_alloc(view->arena_size);
    if (!view->arena_ptr) {
        atlas_arena_free(view);
        return (atlas_cluster_view){ .data = NULL };
    }
    
    // Set up CSR arrays within arena
    view->offsets = (uint32_t*)view->arena_ptr;
    view->indices = (uint32_t*)((uint8_t*)view->arena_ptr + offsets_size);
    
    // Initialize offsets array
    memset(view->offsets, 0, offsets_size);
    
    // First pass: count pages in each resonance class
    uint32_t resonance_counts[96] = {0};
    for (size_t page = 0; page < pages; page++) {
        const uint8_t* page_ptr = base + (page * 256);
        uint8_t resonance_class = atlas_page_resonance_class_llvm(page_ptr);
        resonance_class = resonance_class % 96;  // Ensure within bounds
        resonance_counts[resonance_class]++;
    }
    
    // Build offsets array (prefix sum)
    view->offsets[0] = 0;
    for (int r = 0; r < 96; r++) {
        view->offsets[r + 1] = view->offsets[r] + resonance_counts[r];
    }
    view->total_indices = view->offsets[96];
    
    // Second pass: fill indices array
    uint32_t current_positions[96];
    memcpy(current_positions, view->offsets, 96 * sizeof(uint32_t));
    
    for (size_t page = 0; page < pages; page++) {
        const uint8_t* page_ptr = base + (page * 256);
        uint8_t resonance_class = atlas_page_resonance_class_llvm(page_ptr);
        resonance_class = resonance_class % 96;  // Ensure within bounds
        
        // Store page index in appropriate location
        view->indices[current_positions[resonance_class]++] = (uint32_t)page;
    }
    
    return (atlas_cluster_view){ .data = view };
}

void atlas_cluster_destroy(atlas_cluster_view* cluster) {
    if (!cluster || !cluster->data) {
        return;
    }
    
    atlas_cluster_view_internal* view = (atlas_cluster_view_internal*)cluster->data;
    
    // Validate magic number
    if (view->magic != ATLAS_CLUSTER_MAGIC) {
        return;
    }
    
    // Clear magic to prevent reuse
    view->magic = 0;
    
    // Free arena memory
    atlas_arena_free(view->arena_ptr);
    
    // Free view structure
    atlas_arena_free(view);
    
    // Clear cluster handle
    cluster->data = NULL;
}

uint64_t atlas_next_harmonic_window_from(uint64_t now, uint8_t r) {
    // Validate resonance class
    r = r % 96;  // Ensure within mod-96 bounds
    
    // Use LLVM IR implementation for complex harmonic calculations
    return atlas_next_harmonic_window_from_llvm(now, r);
}

// =============================================================================
// Additional Utility Functions for Layer 3
// =============================================================================

// Get the number of pages in a specific resonance class
size_t atlas_cluster_count_for_resonance(atlas_cluster_view cluster, uint8_t resonance_class) {
    if (!cluster.data) {
        return 0;
    }
    
    atlas_cluster_view_internal* view = (atlas_cluster_view_internal*)cluster.data;
    if (view->magic != ATLAS_CLUSTER_MAGIC) {
        return 0;
    }
    
    resonance_class = resonance_class % 96;
    return view->offsets[resonance_class + 1] - view->offsets[resonance_class];
}

// Get page indices for a specific resonance class
const uint32_t* atlas_cluster_pages_for_resonance(atlas_cluster_view cluster, uint8_t resonance_class, size_t* count) {
    if (!cluster.data || !count) {
        if (count) *count = 0;
        return NULL;
    }
    
    atlas_cluster_view_internal* view = (atlas_cluster_view_internal*)cluster.data;
    if (view->magic != ATLAS_CLUSTER_MAGIC) {
        *count = 0;
        return NULL;
    }
    
    resonance_class = resonance_class % 96;
    uint32_t start = view->offsets[resonance_class];
    uint32_t end = view->offsets[resonance_class + 1];
    
    *count = end - start;
    return view->indices + start;
}

// Check if two resonance classes harmonize
bool atlas_resonance_harmonizes(uint8_t r1, uint8_t r2) {
    // Ensure both are within mod-96 bounds
    r1 = r1 % 96;
    r2 = r2 % 96;
    
    // Use LLVM IR implementation for harmonic analysis
    return atlas_resonance_harmonizes_llvm(r1, r2);
}

// Get resonance class for a specific page
uint8_t atlas_page_resonance_class(const uint8_t* page256) {
    if (!page256) {
        return 0;
    }
    
    return atlas_page_resonance_class_llvm(page256) % 96;
}

// Calculate next scheduling window with validation
uint64_t atlas_schedule_next_window(uint64_t now, uint8_t r) {
    // Validate inputs
    if (r > 95) {
        r = r % 96;
    }
    
    // Calculate using the specified formula: next = now + ((96 - ((now + r) % 96)) % 96)
    uint64_t temp = (now + r) % 96;
    uint64_t offset = (96 - temp) % 96;
    
    // If offset is 0, the next window is 96 units away
    if (offset == 0) {
        offset = 96;
    }
    
    return now + offset;
}

// Batch processing: classify multiple pages
void atlas_r96_classify_pages(const uint8_t* base, size_t pages, uint8_t* classifications) {
    if (!base || !classifications || pages == 0) {
        return;
    }
    
    for (size_t i = 0; i < pages; i++) {
        const uint8_t* page_ptr = base + (i * 256);
        classifications[i] = atlas_page_resonance_class_llvm(page_ptr) % 96;
    }
}

// Batch processing: generate histograms for multiple pages
void atlas_r96_histogram_pages(const uint8_t* base, size_t pages, uint16_t* histograms) {
    if (!base || !histograms || pages == 0) {
        return;
    }
    
    for (size_t i = 0; i < pages; i++) {
        const uint8_t* page_ptr = base + (i * 256);
        uint16_t* hist_ptr = histograms + (i * 96);
        atlas_r96_histogram_page_llvm(page_ptr, hist_ptr);
    }
}

// =============================================================================
// Debug and Validation Functions
// =============================================================================

// Validate cluster view structure
bool atlas_cluster_validate(atlas_cluster_view cluster) {
    if (!cluster.data) {
        return false;
    }
    
    atlas_cluster_view_internal* view = (atlas_cluster_view_internal*)cluster.data;
    
    // Check magic number
    if (view->magic != ATLAS_CLUSTER_MAGIC) {
        return false;
    }
    
    // Check CSR structure consistency
    if (!view->offsets || !view->indices) {
        return false;
    }
    
    // Validate offsets array is monotonically increasing
    for (int r = 0; r < 96; r++) {
        if (view->offsets[r] > view->offsets[r + 1]) {
            return false;
        }
    }
    
    // Check total indices count
    if (view->offsets[96] != view->total_indices) {
        return false;
    }
    
    // Validate all page indices are within bounds
    for (size_t i = 0; i < view->total_indices; i++) {
        if (view->indices[i] >= view->total_pages) {
            return false;
        }
    }
    
    return true;
}

// Get cluster view statistics
void atlas_cluster_stats(atlas_cluster_view cluster, size_t* total_pages, size_t* non_empty_classes, size_t* largest_class) {
    if (!cluster.data || !total_pages || !non_empty_classes || !largest_class) {
        if (total_pages) *total_pages = 0;
        if (non_empty_classes) *non_empty_classes = 0;
        if (largest_class) *largest_class = 0;
        return;
    }
    
    atlas_cluster_view_internal* view = (atlas_cluster_view_internal*)cluster.data;
    if (view->magic != ATLAS_CLUSTER_MAGIC) {
        *total_pages = 0;
        *non_empty_classes = 0;
        *largest_class = 0;
        return;
    }
    
    *total_pages = view->total_pages;
    *non_empty_classes = 0;
    *largest_class = 0;
    
    for (int r = 0; r < 96; r++) {
        size_t class_size = view->offsets[r + 1] - view->offsets[r];
        if (class_size > 0) {
            (*non_empty_classes)++;
            if (class_size > *largest_class) {
                *largest_class = class_size;
            }
        }
    }
}