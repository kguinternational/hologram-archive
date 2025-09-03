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
#include "../include/atlas-resonance.h"

// Type alias for resonance class
typedef uint8_t atlas_resonance_t;

// =============================================================================
// Cluster View Structure
// =============================================================================

typedef struct {
    uint32_t magic;                    // 0xA71A5C1U (ATLASClU)
    
    // CSR format data
    uint32_t* offsets;                 // offsets[97] - start indices for each resonance class
    uint32_t* indices;                 // indices[n] - Φ-linearized byte coordinates grouped by resonance
    
    // Metadata
    size_t total_pages;                // Total number of pages in the view
    size_t total_indices;              // Total number of entries in indices array (bytes, not pages)
    const uint8_t* base_ptr;           // Base pointer to original memory
    
    // Memory management
    void* arena_ptr;                   // Simple arena for WASM compatibility
    size_t arena_size;                 // Arena size for cleanup
} atlas_cluster_view_internal;

#define ATLAS_CLUSTER_MAGIC 0xA71A5C1U

// =============================================================================
// LLVM IR Function Declarations for Layer 3
// =============================================================================

// From exports.ll - C ABI exports for LLVM IR functions
extern void atlas_r96_classify_page_llvm(const uint8_t* in256, uint8_t out256[256]);
extern void atlas_r96_histogram_page_llvm(const uint8_t* in256, uint16_t out96[96]);
extern bool atlas_resonance_harmonizes_llvm(uint8_t r1, uint8_t r2);
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
        return (atlas_cluster_view){ .offsets = NULL, .indices = NULL, .n = 0 };
    }
    
    // Calculate arena size needed for internal structure + CSR arrays
    size_t view_size = sizeof(atlas_cluster_view_internal);
    size_t offsets_size = 97 * sizeof(uint32_t);  // 96 resonance classes + 1 terminator
    size_t total_bytes = pages * 256;  // Total bytes to classify
    size_t indices_size = total_bytes * sizeof(uint32_t);  // Store Φ-linearized coordinate for each byte
    size_t total_size = view_size + offsets_size + indices_size;
    
    // Allocate single contiguous block for everything
    void* arena = atlas_arena_alloc(total_size);
    if (!arena) {
        return (atlas_cluster_view){ .offsets = NULL, .indices = NULL, .n = 0 };
    }
    
    // Layout: [atlas_cluster_view_internal][offsets array][indices array]
    atlas_cluster_view_internal* view = (atlas_cluster_view_internal*)arena;
    uint32_t* offsets = (uint32_t*)((uint8_t*)arena + view_size);
    uint32_t* indices = (uint32_t*)((uint8_t*)arena + view_size + offsets_size);
    
    // Initialize structure
    memset(view, 0, sizeof(atlas_cluster_view_internal));
    view->magic = ATLAS_CLUSTER_MAGIC;
    view->total_pages = pages;
    view->base_ptr = base;
    view->arena_ptr = arena;  // Points to itself
    view->arena_size = total_size;
    view->offsets = offsets;
    view->indices = indices;
    
    // Initialize offsets array
    memset(view->offsets, 0, offsets_size);
    
    // First pass: count bytes in each resonance class (classify each individual byte)
    uint32_t resonance_counts[96] = {0};
    for (size_t page = 0; page < pages; page++) {
        const uint8_t* page_ptr = base + (page * 256);
        for (size_t byte_offset = 0; byte_offset < 256; byte_offset++) {
            uint8_t byte_value = page_ptr[byte_offset];
            uint8_t resonance_class = atlas_r96_classify(byte_value);
            resonance_counts[resonance_class]++;
        }
    }
    
    // Build offsets array (prefix sum)
    view->offsets[0] = 0;
    for (int r = 0; r < 96; r++) {
        view->offsets[r + 1] = view->offsets[r] + resonance_counts[r];
    }
    view->total_indices = view->offsets[96];
    
    // Second pass: fill indices array with Φ-linearized byte coordinates
    uint32_t current_positions[96];
    memcpy(current_positions, view->offsets, 96 * sizeof(uint32_t));
    
    for (size_t page = 0; page < pages; page++) {
        const uint8_t* page_ptr = base + (page * 256);
        for (size_t byte_offset = 0; byte_offset < 256; byte_offset++) {
            uint8_t byte_value = page_ptr[byte_offset];
            uint8_t resonance_class = atlas_r96_classify(byte_value);
            
            // Store Φ-linearized coordinate: page*256 + byte_offset
            uint32_t phi_coord = (uint32_t)(page * 256 + byte_offset);
            view->indices[current_positions[resonance_class]++] = phi_coord;
        }
    }
    
    // Return the CSR view structure
    // The internal structure is stored at offsets[-sizeof(atlas_cluster_view_internal)/sizeof(uint32_t)]
    return (atlas_cluster_view){ 
        .offsets = offsets,
        .indices = indices,
        .n = view->total_indices
    };
}

void atlas_cluster_destroy(atlas_cluster_view* cluster) {
    if (!cluster || !cluster->offsets) {
        return;
    }
    
    // Recover the internal structure from before the offsets array
    atlas_cluster_view_internal* view = (atlas_cluster_view_internal*)(
        (uint8_t*)cluster->offsets - sizeof(atlas_cluster_view_internal)
    );
    
    // Validate magic number
    if (view->magic != ATLAS_CLUSTER_MAGIC) {
        return;
    }
    
    // Clear magic to prevent reuse
    view->magic = 0;
    
    // Free the single arena allocation
    atlas_arena_free(view->arena_ptr);
    
    // Clear cluster handle
    cluster->offsets = NULL;
    cluster->indices = NULL;
    cluster->n = 0;
}


// =============================================================================
// Additional Utility Functions for Layer 3
// =============================================================================

// Get the number of bytes in a specific resonance class
size_t atlas_cluster_count_for_resonance(atlas_cluster_view cluster, uint8_t resonance_class) {
    if (!cluster.offsets) {
        return 0;
    }
    
    // Recover the internal structure
    atlas_cluster_view_internal* view = (atlas_cluster_view_internal*)(
        (uint8_t*)cluster.offsets - sizeof(atlas_cluster_view_internal)
    );
    if (view->magic != ATLAS_CLUSTER_MAGIC) {
        return 0;
    }
    
    resonance_class = resonance_class % 96;
    return cluster.offsets[resonance_class + 1] - cluster.offsets[resonance_class];
}

// Get Φ-linearized byte coordinates for a specific resonance class
const uint32_t* atlas_cluster_pages_for_resonance(atlas_cluster_view cluster, uint8_t resonance_class, size_t* count) {
    if (!cluster.offsets || !count) {
        if (count) *count = 0;
        return NULL;
    }
    
    // Recover the internal structure
    atlas_cluster_view_internal* view = (atlas_cluster_view_internal*)(
        (uint8_t*)cluster.offsets - sizeof(atlas_cluster_view_internal)
    );
    if (view->magic != ATLAS_CLUSTER_MAGIC) {
        *count = 0;
        return NULL;
    }
    
    resonance_class = resonance_class % 96;
    uint32_t start = cluster.offsets[resonance_class];
    uint32_t end = cluster.offsets[resonance_class + 1];
    
    *count = end - start;
    return cluster.indices + start;
}

// Check if two resonance classes harmonize (moved to Additional Layer 3 API Functions section)

// Get resonance class for a specific page
uint8_t atlas_page_resonance_class(const uint8_t* page256) {
    if (!page256) {
        return 0;
    }
    
    return atlas_page_resonance_class_llvm(page256) % 96;
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
// Additional Layer 3 API Functions
// =============================================================================

// Single byte R96 classification
atlas_resonance_t atlas_r96_classify(uint8_t byte) {
    return byte % 96;
}

// Classify array of bytes to resonance classes
void atlas_r96_classify_array(const uint8_t* input, atlas_resonance_t* output, size_t length) {
    if (!input || !output) {
        return;
    }
    
    for (size_t i = 0; i < length; i++) {
        output[i] = input[i] % 96;
    }
}

// Check if two resonance classes harmonize (compatible with the LLVM function name)
bool atlas_r96_harmonizes(atlas_resonance_t r1, atlas_resonance_t r2) {
    // Ensure both are within mod-96 bounds
    r1 = r1 % 96;
    r2 = r2 % 96;
    
    // Use LLVM IR implementation for harmonic analysis
    return atlas_resonance_harmonizes_llvm(r1, r2);
}

// Find harmonic conjugate of resonance class
atlas_resonance_t atlas_r96_harmonic_conjugate(atlas_resonance_t r1) {
    r1 = r1 % 96;
    if (r1 == 0) {
        return 0;  // 0 is its own conjugate
    }
    return 96 - r1;
}

// Check if two resonance classes harmonize (alias for test compatibility)  
bool atlas_resonance_harmonizes(uint8_t r1, uint8_t r2) {
    // Ensure both are within mod-96 bounds
    r1 = r1 % 96;
    r2 = r2 % 96;
    
    // Use LLVM IR implementation for harmonic analysis
    return atlas_resonance_harmonizes_llvm(r1, r2);
}

// =============================================================================
// Debug and Validation Functions
// =============================================================================

// Validate cluster view structure
bool atlas_cluster_validate(atlas_cluster_view cluster) {
    if (!cluster.offsets || !cluster.indices) {
        return false;
    }
    
    // Recover the internal structure
    atlas_cluster_view_internal* view = (atlas_cluster_view_internal*)(
        (uint8_t*)cluster.offsets - sizeof(atlas_cluster_view_internal)
    );
    
    // Check magic number
    if (view->magic != ATLAS_CLUSTER_MAGIC) {
        return false;
    }
    
    // Validate offsets array is monotonically increasing
    for (int r = 0; r < 96; r++) {
        if (cluster.offsets[r] > cluster.offsets[r + 1]) {
            return false;
        }
    }
    
    // Check total indices count
    if (cluster.offsets[96] != cluster.n) {
        return false;
    }
    
    // Validate all Φ-linearized coordinates are within bounds (0..pages*256-1)
    uint32_t max_coord = (uint32_t)(view->total_pages * 256 - 1);
    for (size_t i = 0; i < cluster.n; i++) {
        if (cluster.indices[i] > max_coord) {
            return false;
        }
    }
    
    return true;
}

// Get cluster view statistics
void atlas_cluster_stats(atlas_cluster_view cluster, size_t* total_pages, size_t* non_empty_classes, size_t* largest_class) {
    if (!cluster.offsets || !total_pages || !non_empty_classes || !largest_class) {
        if (total_pages) *total_pages = 0;
        if (non_empty_classes) *non_empty_classes = 0;
        if (largest_class) *largest_class = 0;
        return;
    }
    
    // Recover the internal structure
    atlas_cluster_view_internal* view = (atlas_cluster_view_internal*)(
        (uint8_t*)cluster.offsets - sizeof(atlas_cluster_view_internal)
    );
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
        size_t class_size = cluster.offsets[r + 1] - cluster.offsets[r];
        if (class_size > 0) {
            (*non_empty_classes)++;
            if (class_size > *largest_class) {
                *largest_class = class_size;
            }
        }
    }
}