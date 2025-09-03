/* atlas-resonance.h - Atlas-12288 Layer 3 Resonance Interface
 * (c) 2024-2025 UOR Foundation - MIT License
 * 
 * Public API for Atlas-12288 Layer 3 (Resonance Layer) providing:
 * - R96 resonance classification
 * - Harmonic analysis and pairing
 * - C768 triple-cycle operations
 * - High-performance SIMD kernels
 */

#ifndef ATLAS_RESONANCE_H
#define ATLAS_RESONANCE_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

// =============================================================================
// Version Information
// =============================================================================

#define ATLAS_RESONANCE_VERSION_MAJOR 1
#define ATLAS_RESONANCE_VERSION_MINOR 0
#define ATLAS_RESONANCE_VERSION_PATCH 0

// =============================================================================
// Type Definitions
// =============================================================================

/* Resonance class [0, 95] */
typedef uint8_t atlas_resonance_t;

/* C768 element */
typedef uint16_t atlas_c768_t;

/* Harmonic pair structure */
typedef struct {
    atlas_resonance_t r1;    /* First resonance */
    atlas_resonance_t r2;    /* Second resonance */
} atlas_harmonic_pair_t;

/* R96 histogram (96 bins of 16-bit counters) */
typedef struct {
    uint16_t bins[96];       /* Histogram bins */
    uint32_t total_count;    /* Total classified bytes */
} atlas_r96_histogram_t;

/* Cluster view for CSR clustering - matches main atlas.h API */
typedef struct {
    const uint32_t* offsets;  /* Array of length 97 */
    const uint32_t* indices;  /* Array of Φ-linearized byte coordinates (coord = page*256 + offset) */
    uint32_t n;               /* Total coordinate count (= pages * 256) */
} atlas_cluster_view;

// =============================================================================
// R96 Classification
// =============================================================================

/**
 * Classify single byte to R96 resonance class.
 * 
 * @param byte Input byte
 * @return Resonance class [0, 95]
 */
atlas_resonance_t atlas_r96_classify(uint8_t byte);

/**
 * Classify array of bytes to resonance classes.
 * 
 * @param input Input byte array
 * @param output Output resonance array (same length as input)
 * @param length Number of bytes to classify
 */
void atlas_r96_classify_array(const uint8_t* input, atlas_resonance_t* output, size_t length);

/**
 * Classify page (256 bytes) and generate histogram.
 * 
 * @param page_data Input page data (256 bytes)
 * @param histogram Output histogram structure
 */
void atlas_r96_classify_page_histogram(const uint8_t* page_data, atlas_r96_histogram_t* histogram);

/**
 * Check if page contains exactly 96 distinct resonance classes.
 * 
 * @param page_data Input page data (256 bytes)
 * @return true if R96 conformant, false otherwise
 */
bool atlas_r96_check_conformance(const uint8_t* page_data);

// =============================================================================
// Harmonic Analysis
// =============================================================================

/**
 * Check if two resonance classes harmonize.
 * Harmonic condition: (r1 + r2) % 96 == 0
 * 
 * @param r1 First resonance class
 * @param r2 Second resonance class
 * @return true if harmonizing pair, false otherwise
 */
bool atlas_r96_harmonizes(atlas_resonance_t r1, atlas_resonance_t r2);

/**
 * Find harmonic conjugate of resonance class.
 * Returns r2 such that (r1 + r2) % 96 == 0
 * 
 * @param r1 Input resonance class
 * @return Harmonic conjugate
 */
atlas_resonance_t atlas_r96_harmonic_conjugate(atlas_resonance_t r1);

/**
 * Find all harmonic pairs in resonance array.
 * 
 * @param resonances Input resonance array
 * @param length Array length
 * @param pairs Output harmonic pairs (allocated by caller)
 * @param max_pairs Maximum number of pairs to find
 * @return Number of harmonic pairs found
 */
size_t atlas_r96_find_harmonic_pairs(const atlas_resonance_t* resonances, size_t length,
                                      atlas_harmonic_pair_t* pairs, size_t max_pairs);

// =============================================================================
// C768 Operations
// =============================================================================

/**
 * Generate C768 element from triple components.
 * 
 * @param a First component
 * @param b Second component
 * @param c Third component
 * @return C768 element
 */
atlas_c768_t atlas_c768_generate(uint8_t a, uint8_t b, uint8_t c);

/**
 * Check C768 triple identity: a³ + b³ + c³ ≡ 3abc (mod 768)
 * 
 * @param a First component
 * @param b Second component
 * @param c Third component
 * @return true if identity holds, false otherwise
 */
bool atlas_c768_check_identity(uint8_t a, uint8_t b, uint8_t c);

/**
 * Compute C768 stabilization window variance.
 * 
 * @param data Input data window
 * @param length Window length
 * @return Stabilization variance
 */
double atlas_c768_stabilization_variance(const uint8_t* data, size_t length);

/**
 * Check if C768 window has stabilized (variance < threshold).
 * 
 * @param data Input data window  
 * @param length Window length
 * @param threshold Stabilization threshold
 * @return true if stabilized, false otherwise
 */
bool atlas_c768_is_stabilized(const uint8_t* data, size_t length, double threshold);

// =============================================================================
// Performance and Optimization
// =============================================================================

/**
 * Get R96 classification throughput in bytes/second.
 * 
 * @return Classification throughput
 */
uint64_t atlas_r96_get_throughput(void);

/**
 * Run R96 classification performance benchmark.
 * 
 * @param data Test data
 * @param length Data length
 * @param iterations Number of iterations
 * @return Average throughput in bytes/second
 */
uint64_t atlas_r96_benchmark_throughput(const uint8_t* data, size_t length, size_t iterations);

// =============================================================================
// Error Handling
// =============================================================================

/* Error codes specific to resonance operations */
typedef enum {
    ATLAS_RESONANCE_SUCCESS = 0,
    ATLAS_RESONANCE_ERROR_INVALID_ARGUMENT = 1,
    ATLAS_RESONANCE_ERROR_OUT_OF_RANGE = 2,
    ATLAS_RESONANCE_ERROR_NOT_CONFORMANT = 3,
    ATLAS_RESONANCE_ERROR_PERFORMANCE = 4
} atlas_resonance_error_t;

/**
 * Get last resonance operation error.
 * 
 * @return Last error code
 */
atlas_resonance_error_t atlas_resonance_get_last_error(void);

// =============================================================================
// Cluster Operations (CSR Clustering)
// =============================================================================

/**
 * Create cluster view by resonance class.
 * 
 * @param base Base pointer to data
 * @param pages Number of pages
 * @return Cluster view structure
 */
atlas_cluster_view atlas_cluster_by_resonance(const uint8_t* base, size_t pages);

/**
 * Destroy cluster view and free resources.
 * 
 * @param cluster Cluster view to destroy
 */
void atlas_cluster_destroy(atlas_cluster_view* cluster);

/**
 * Count bytes for specific resonance class.
 * 
 * @param cluster Cluster view
 * @param resonance_class Target resonance class
 * @return Number of bytes with this resonance class
 */
size_t atlas_cluster_count_for_resonance(atlas_cluster_view cluster, uint8_t resonance_class);

/**
 * Get Φ-linearized byte coordinates for specific resonance class.
 * 
 * @param cluster Cluster view
 * @param resonance_class Target resonance class  
 * @param count Output count of bytes
 * @return Array of Φ-linearized coordinates (coord = page*256 + offset, caller should not free)
 */
const uint32_t* atlas_cluster_pages_for_resonance(atlas_cluster_view cluster, uint8_t resonance_class, size_t* count);

/**
 * Validate cluster integrity.
 * 
 * @param cluster Cluster view
 * @return true if cluster is valid
 */
bool atlas_cluster_validate(atlas_cluster_view cluster);

/**
 * Get cluster statistics.
 * 
 * @param cluster Cluster view
 * @param total_pages Output total pages
 * @param non_empty_classes Output number of non-empty classes
 * @param largest_class Output size of largest class (in bytes)
 */
void atlas_cluster_stats(atlas_cluster_view cluster, size_t* total_pages, size_t* non_empty_classes, size_t* largest_class);

// =============================================================================
// Layer 3 Runtime Functions
// =============================================================================

/**
 * Classify page (256 bytes) to resonance classes.
 * 
 * @param in256 Input page data (256 bytes)
 * @param out256 Output classification array (256 bytes)
 */
void atlas_r96_classify_page(const uint8_t* in256, uint8_t out256[256]);

/**
 * Generate histogram for page (256 bytes).
 * 
 * @param in256 Input page data (256 bytes)  
 * @param out96 Output histogram (96 bins)
 */
void atlas_r96_histogram_page(const uint8_t* in256, uint16_t out96[96]);

/**
 * Get dominant resonance class for page.
 * 
 * @param page256 Input page data (256 bytes)
 * @return Dominant resonance class
 */
uint8_t atlas_page_resonance_class(const uint8_t* page256);

/**
 * Calculate next harmonic window from current time.
 * 
 * @param now Current time
 * @param r Resonance class
 * @return Next harmonic window time
 */
uint64_t atlas_next_harmonic_window_from(uint64_t now, uint8_t r);

/**
 * Calculate next scheduling window.
 * 
 * @param now Current time
 * @param r Resonance class
 * @return Next scheduling window time
 */
uint64_t atlas_schedule_next_window(uint64_t now, uint8_t r);

/**
 * Check if two resonance classes harmonize (alias for compatibility).
 * 
 * @param r1 First resonance class
 * @param r2 Second resonance class
 * @return true if harmonizing pair
 */
bool atlas_resonance_harmonizes(uint8_t r1, uint8_t r2);

/**
 * Batch classify multiple pages.
 * 
 * @param base Base pointer to data
 * @param pages Number of pages to classify
 * @param classifications Output array of resonance classes
 */
void atlas_r96_classify_pages(const uint8_t* base, size_t pages, uint8_t* classifications);

/**
 * Batch generate histograms for multiple pages.
 * 
 * @param base Base pointer to data  
 * @param pages Number of pages to process
 * @param histograms Output histogram array (pages * 96 elements)
 */
void atlas_r96_histogram_pages(const uint8_t* base, size_t pages, uint16_t* histograms);

#ifdef __cplusplus
}
#endif

#endif /* ATLAS_RESONANCE_H */