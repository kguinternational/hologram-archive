/* clustering.c - Atlas-12288 Layer 3 Clustering Runtime
 * (c) 2024-2025 UOR Foundation - MIT License
 * 
 * Layer 3 clustering implementation using CSR (Compressed Sparse Row) format
 * for efficient resonance-based clustering operations with LLVM backend.
 */

#include <stdint.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <assert.h>
#include <math.h>

#include "../include/atlas-resonance.h"

// =============================================================================
// CSR Matrix Structure for Clustering
// =============================================================================

typedef struct {
    uint32_t magic;                    // 0xC5B12345 (CSR Matrix)
    uint32_t rows;                     // Number of rows (96 resonance classes)
    uint32_t cols;                     // Number of columns (pages)
    uint32_t nnz;                      // Number of non-zero entries
    
    // CSR format arrays
    uint32_t* row_offsets;             // row_offsets[rows+1] - start/end indices
    uint32_t* col_indices;             // col_indices[nnz] - column indices
    uint8_t* values;                   // values[nnz] - resonance values (optional)
    
    // Memory management
    void* arena_ptr;                   // Arena memory for cleanup
    size_t arena_size;                 // Total arena size
} atlas_csr_matrix_t;

#define ATLAS_CSR_MAGIC 0xC5B12345U

// =============================================================================
// Cluster Management Structure
// =============================================================================

typedef struct {
    uint32_t magic;                    // 0xC1U572B0 (ClUsteR)
    uint8_t resonance_class;           // Target resonance class [0, 95]
    uint32_t capacity;                 // Maximum pages that can be stored
    uint32_t count;                    // Current number of pages
    uint32_t* page_indices;            // Array of page indices
    
    // Statistics
    double homogeneity_score;          // Cluster homogeneity (0.0-1.0)
    uint32_t last_update_time;         // Last update timestamp
    
    void* arena_ptr;                   // Arena memory
    size_t arena_size;                 // Arena size
} atlas_cluster_t;

#define ATLAS_CLUSTER_MAGIC 0xC1572B0U

// =============================================================================
// LLVM IR Function Declarations
// =============================================================================

// From exports.ll
extern uint8_t atlas_page_resonance_class_llvm(const uint8_t* page256);

// =============================================================================
// Memory Management
// =============================================================================

static void* atlas_clustering_alloc(size_t size) {
    return aligned_alloc(32, (size + 31) & ~31);
}

static void atlas_clustering_free(void* ptr) {
    if (ptr) {
        free(ptr);
    }
}

// =============================================================================
// CSR Matrix Operations
// =============================================================================

atlas_csr_matrix_t* atlas_csr_create(uint32_t rows, uint32_t cols) {
    if (rows == 0 || cols == 0) {
        return NULL;
    }
    
    // Calculate memory requirements
    size_t matrix_size = sizeof(atlas_csr_matrix_t);
    size_t offsets_size = (rows + 1) * sizeof(uint32_t);
    size_t indices_size = cols * sizeof(uint32_t);  // Worst case: dense matrix
    size_t values_size = cols * sizeof(uint8_t);
    size_t total_size = matrix_size + offsets_size + indices_size + values_size;
    
    // Allocate arena
    void* arena = atlas_clustering_alloc(total_size);
    if (!arena) {
        return NULL;
    }
    
    // Layout memory
    atlas_csr_matrix_t* matrix = (atlas_csr_matrix_t*)arena;
    matrix->row_offsets = (uint32_t*)((uint8_t*)arena + matrix_size);
    matrix->col_indices = (uint32_t*)((uint8_t*)arena + matrix_size + offsets_size);
    matrix->values = (uint8_t*)((uint8_t*)arena + matrix_size + offsets_size + indices_size);
    
    // Initialize structure
    matrix->magic = ATLAS_CSR_MAGIC;
    matrix->rows = rows;
    matrix->cols = cols;
    matrix->nnz = 0;
    matrix->arena_ptr = arena;
    matrix->arena_size = total_size;
    
    // Initialize offsets to zero
    memset(matrix->row_offsets, 0, (rows + 1) * sizeof(uint32_t));
    
    return matrix;
}

void atlas_csr_destroy(atlas_csr_matrix_t* matrix) {
    if (!matrix || matrix->magic != ATLAS_CSR_MAGIC) {
        return;
    }
    
    matrix->magic = 0;  // Clear magic
    atlas_clustering_free(matrix->arena_ptr);
}

bool atlas_csr_build_from_pages(atlas_csr_matrix_t* matrix, const uint8_t* base, size_t pages) {
    if (!matrix || !base || pages == 0 || matrix->magic != ATLAS_CSR_MAGIC) {
        return false;
    }
    
    if (pages > matrix->cols) {
        return false;  // Not enough columns
    }
    
    // Reset matrix
    matrix->nnz = 0;
    memset(matrix->row_offsets, 0, (matrix->rows + 1) * sizeof(uint32_t));
    
    // Count entries per resonance class (row)
    uint32_t row_counts[96] = {0};
    
    for (size_t page = 0; page < pages; page++) {
        const uint8_t* page_ptr = base + (page * 256);
        uint8_t resonance = atlas_page_resonance_class_llvm(page_ptr) % 96;
        row_counts[resonance]++;
    }
    
    // Build row offsets (prefix sum)
    matrix->row_offsets[0] = 0;
    for (uint32_t r = 0; r < matrix->rows; r++) {
        matrix->row_offsets[r + 1] = matrix->row_offsets[r] + row_counts[r];
    }
    matrix->nnz = matrix->row_offsets[matrix->rows];
    
    // Fill column indices and values
    uint32_t current_positions[96];
    memcpy(current_positions, matrix->row_offsets, 96 * sizeof(uint32_t));
    
    for (size_t page = 0; page < pages; page++) {
        const uint8_t* page_ptr = base + (page * 256);
        uint8_t resonance = atlas_page_resonance_class_llvm(page_ptr) % 96;
        
        uint32_t pos = current_positions[resonance]++;
        matrix->col_indices[pos] = (uint32_t)page;
        matrix->values[pos] = resonance;
    }
    
    return true;
}

size_t atlas_csr_get_row_pages(atlas_csr_matrix_t* matrix, uint8_t resonance_class, 
                               const uint32_t** page_indices) {
    if (!matrix || !page_indices || matrix->magic != ATLAS_CSR_MAGIC) {
        if (page_indices) *page_indices = NULL;
        return 0;
    }
    
    resonance_class = resonance_class % 96;
    if (resonance_class >= matrix->rows) {
        *page_indices = NULL;
        return 0;
    }
    
    uint32_t start = matrix->row_offsets[resonance_class];
    uint32_t end = matrix->row_offsets[resonance_class + 1];
    
    *page_indices = matrix->col_indices + start;
    return end - start;
}

// =============================================================================
// Cluster Operations
// =============================================================================

atlas_cluster_t* atlas_cluster_create(uint8_t resonance_class, uint32_t capacity) {
    if (capacity == 0) {
        return NULL;
    }
    
    // Calculate memory requirements
    size_t cluster_size = sizeof(atlas_cluster_t);
    size_t indices_size = capacity * sizeof(uint32_t);
    size_t total_size = cluster_size + indices_size;
    
    // Allocate arena
    void* arena = atlas_clustering_alloc(total_size);
    if (!arena) {
        return NULL;
    }
    
    // Layout memory
    atlas_cluster_t* cluster = (atlas_cluster_t*)arena;
    cluster->page_indices = (uint32_t*)((uint8_t*)arena + cluster_size);
    
    // Initialize structure
    cluster->magic = ATLAS_CLUSTER_MAGIC;
    cluster->resonance_class = resonance_class % 96;
    cluster->capacity = capacity;
    cluster->count = 0;
    cluster->homogeneity_score = 0.0;
    cluster->last_update_time = 0;
    cluster->arena_ptr = arena;
    cluster->arena_size = total_size;
    
    return cluster;
}

void atlas_cluster_destroy_individual(atlas_cluster_t* cluster) {
    if (!cluster || cluster->magic != ATLAS_CLUSTER_MAGIC) {
        return;
    }
    
    cluster->magic = 0;  // Clear magic
    atlas_clustering_free(cluster->arena_ptr);
}


bool atlas_cluster_add_page(atlas_cluster_t* cluster, uint32_t page_index) {
    if (!cluster || cluster->magic != ATLAS_CLUSTER_MAGIC) {
        return false;
    }
    
    if (cluster->count >= cluster->capacity) {
        return false;  // Cluster is full
    }
    
    // Check for duplicates
    for (uint32_t i = 0; i < cluster->count; i++) {
        if (cluster->page_indices[i] == page_index) {
            return false;  // Already exists
        }
    }
    
    cluster->page_indices[cluster->count++] = page_index;
    return true;
}

double atlas_cluster_calculate_homogeneity(atlas_cluster_t* cluster, const uint8_t* base) {
    if (!cluster || !base || cluster->magic != ATLAS_CLUSTER_MAGIC || cluster->count == 0) {
        return 0.0;
    }
    
    uint32_t target_resonance = cluster->resonance_class;
    uint32_t matching_pages = 0;
    
    for (uint32_t i = 0; i < cluster->count; i++) {
        uint32_t page_index = cluster->page_indices[i];
        const uint8_t* page_ptr = base + (page_index * 256);
        uint8_t page_resonance = atlas_page_resonance_class_llvm(page_ptr) % 96;
        
        if (page_resonance == target_resonance) {
            matching_pages++;
        }
    }
    
    cluster->homogeneity_score = (double)matching_pages / (double)cluster->count;
    return cluster->homogeneity_score;
}

bool atlas_cluster_validate_individual(atlas_cluster_t* cluster) {
    if (!cluster || cluster->magic != ATLAS_CLUSTER_MAGIC) {
        return false;
    }
    
    if (cluster->count > cluster->capacity) {
        return false;
    }
    
    if (cluster->resonance_class >= 96) {
        return false;
    }
    
    if (cluster->homogeneity_score < 0.0 || cluster->homogeneity_score > 1.0) {
        return false;
    }
    
    // Check for duplicate page indices
    for (uint32_t i = 0; i < cluster->count; i++) {
        for (uint32_t j = i + 1; j < cluster->count; j++) {
            if (cluster->page_indices[i] == cluster->page_indices[j]) {
                return false;  // Duplicate found
            }
        }
    }
    
    return true;
}


// =============================================================================
// Advanced Clustering Operations
// =============================================================================

typedef struct {
    uint32_t magic;                    // 0xC1U5723D (ClUsteRD)
    atlas_cluster_t* clusters[96];     // One cluster per resonance class
    uint32_t active_clusters;          // Number of non-empty clusters
    size_t total_pages;                // Total pages across all clusters
    void* arena_ptr;                   // Arena memory
    size_t arena_size;                 // Arena size
} atlas_cluster_directory_t;

#define ATLAS_CLUSTER_DIR_MAGIC 0xC15723DU

atlas_cluster_directory_t* atlas_build_all_clusters(const uint8_t* base, size_t pages) {
    if (!base || pages == 0) {
        return NULL;
    }
    
    // Allocate directory
    size_t dir_size = sizeof(atlas_cluster_directory_t);
    atlas_cluster_directory_t* dir = (atlas_cluster_directory_t*)atlas_clustering_alloc(dir_size);
    if (!dir) {
        return NULL;
    }
    
    // Initialize directory
    memset(dir, 0, sizeof(atlas_cluster_directory_t));
    dir->magic = ATLAS_CLUSTER_DIR_MAGIC;
    dir->total_pages = pages;
    dir->arena_ptr = dir;
    dir->arena_size = dir_size;
    
    // Count pages per resonance class
    uint32_t resonance_counts[96] = {0};
    for (size_t page = 0; page < pages; page++) {
        const uint8_t* page_ptr = base + (page * 256);
        uint8_t resonance = atlas_page_resonance_class_llvm(page_ptr) % 96;
        resonance_counts[resonance]++;
    }
    
    // Create clusters for non-empty resonance classes
    dir->active_clusters = 0;
    for (uint8_t r = 0; r < 96; r++) {
        if (resonance_counts[r] > 0) {
            dir->clusters[r] = atlas_cluster_create(r, resonance_counts[r]);
            if (dir->clusters[r]) {
                dir->active_clusters++;
            }
        }
    }
    
    // Fill clusters with page indices
    for (size_t page = 0; page < pages; page++) {
        const uint8_t* page_ptr = base + (page * 256);
        uint8_t resonance = atlas_page_resonance_class_llvm(page_ptr) % 96;
        
        if (dir->clusters[resonance]) {
            atlas_cluster_add_page(dir->clusters[resonance], (uint32_t)page);
        }
    }
    
    // Calculate homogeneity for all clusters
    for (uint8_t r = 0; r < 96; r++) {
        if (dir->clusters[r]) {
            atlas_cluster_calculate_homogeneity(dir->clusters[r], base);
        }
    }
    
    return dir;
}

void atlas_cluster_directory_destroy(atlas_cluster_directory_t* dir) {
    if (!dir || dir->magic != ATLAS_CLUSTER_DIR_MAGIC) {
        return;
    }
    
    // Destroy all clusters
    for (uint8_t r = 0; r < 96; r++) {
        if (dir->clusters[r]) {
            atlas_cluster_destroy_individual(dir->clusters[r]);
            dir->clusters[r] = NULL;
        }
    }
    
    dir->magic = 0;
    atlas_clustering_free(dir);
}

atlas_cluster_t* atlas_get_cluster_for_resonance(atlas_cluster_directory_t* dir, uint8_t resonance_class) {
    if (!dir || dir->magic != ATLAS_CLUSTER_DIR_MAGIC) {
        return NULL;
    }
    
    return dir->clusters[resonance_class % 96];
}

// =============================================================================
// High-Performance Clustering Operations
// =============================================================================

void atlas_clustering_histogram_batch(const uint8_t* base, size_t pages, uint16_t* histograms) {
    if (!base || !histograms || pages == 0) {
        return;
    }
    
    // Use SIMD-optimized LLVM implementation for each page
    for (size_t page = 0; page < pages; page++) {
        const uint8_t* page_ptr = base + (page * 256);
        uint16_t* hist_ptr = histograms + (page * 96);
        
        // Clear histogram
        memset(hist_ptr, 0, 96 * sizeof(uint16_t));
        
        // Call LLVM SIMD histogram function
        // Use simple histogram generation instead of SIMD version
        for (int byte_idx = 0; byte_idx < 256; byte_idx++) {
            uint8_t byte = page_ptr[byte_idx];
            uint8_t resonance = byte % 96;
            hist_ptr[resonance]++;
        }
    }
}

bool atlas_clustering_analyze_distribution(const uint8_t* base, size_t pages, 
                                           double* entropy, double* uniformity) {
    if (!base || pages == 0 || !entropy || !uniformity) {
        return false;
    }
    
    // Count total resonance class distribution
    uint32_t resonance_counts[96] = {0};
    uint32_t total_pages = 0;
    
    for (size_t page = 0; page < pages; page++) {
        const uint8_t* page_ptr = base + (page * 256);
        uint8_t resonance = atlas_page_resonance_class_llvm(page_ptr) % 96;
        resonance_counts[resonance]++;
        total_pages++;
    }
    
    // Calculate entropy: H = -Σ(p_i * log2(p_i))
    double entropy_sum = 0.0;
    uint32_t non_empty_classes = 0;
    
    for (uint8_t r = 0; r < 96; r++) {
        if (resonance_counts[r] > 0) {
            double probability = (double)resonance_counts[r] / (double)total_pages;
            entropy_sum -= probability * (log(probability) / log(2.0));
            non_empty_classes++;
        }
    }
    
    *entropy = entropy_sum;
    
    // Calculate uniformity: 1 - (max_count - min_count) / total_pages
    uint32_t max_count = 0, min_count = UINT32_MAX;
    for (uint8_t r = 0; r < 96; r++) {
        if (resonance_counts[r] > 0) {
            if (resonance_counts[r] > max_count) max_count = resonance_counts[r];
            if (resonance_counts[r] < min_count) min_count = resonance_counts[r];
        }
    }
    
    if (non_empty_classes > 0) {
        *uniformity = 1.0 - (double)(max_count - min_count) / (double)total_pages;
    } else {
        *uniformity = 0.0;
    }
    
    return true;
}

// =============================================================================
// Memory-Efficient Cluster Views (Integration with existing API)
// =============================================================================

bool atlas_clustering_merge_with_cluster_view(atlas_cluster_directory_t* dir, 
                                               atlas_cluster_view* view) {
    if (!dir || !view || dir->magic != ATLAS_CLUSTER_DIR_MAGIC) {
        return false;
    }
    
    // Validate that both represent the same data structure
    if (!view->offsets || !view->indices) {
        return false;
    }
    
    // Cross-validate that the cluster directory and cluster view are consistent
    for (uint8_t r = 0; r < 96; r++) {
        atlas_cluster_t* cluster = dir->clusters[r];
        size_t view_count = (r < 95) ? 
            (view->offsets[r + 1] - view->offsets[r]) : 
            (view->n - view->offsets[r]);
        
        if (cluster) {
            if (cluster->count != view_count) {
                return false;  // Inconsistent counts
            }
        } else {
            if (view_count != 0) {
                return false;  // Cluster should exist but doesn't
            }
        }
    }
    
    return true;
}