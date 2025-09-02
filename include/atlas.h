/* atlas.h - Unified C API for Atlas-12288 Layers 2 & 3
 * (c) 2024-2025 UOR Foundation - MIT License
 *
 * This is the stable public interface for Atlas-12288 computational model.
 * Provides Layer 2 (Conservation) and Layer 3 (Resonance) functionality
 * with a stable C ABI for cross-language interoperability.
 */

#ifndef ATLAS_H
#define ATLAS_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* API Version */
#define ATLAS_API_VERSION 1

/* Cross-platform compatibility */
#if defined(_WIN32) || defined(__CYGWIN__)
  #ifdef ATLAS_BUILDING_DLL
    #define ATLAS_API __declspec(dllexport)
  #elif defined(ATLAS_USING_DLL)
    #define ATLAS_API __declspec(dllimport)
  #else
    #define ATLAS_API
  #endif
#else
  #if __GNUC__ >= 4
    #define ATLAS_API __attribute__ ((visibility ("default")))
  #else
    #define ATLAS_API
  #endif
#endif

/* =============================================================================
 * ERROR CODES & VERSION
 * ============================================================================= */

/**
 * Error codes returned by Atlas functions.
 * Success is always 0 (ATLAS_OK), errors are positive integers.
 */
typedef enum {
    ATLAS_OK = 0,              /* Success */
    ATLAS_E_CONSERVATION = 1,  /* Conservation law violation */
    ATLAS_E_WITNESS = 2,       /* Witness verification failed */
    ATLAS_E_BUDGET = 3,        /* Budget allocation failed */
    ATLAS_E_MEMORY = 4,        /* Memory allocation failed */
    ATLAS_E_STATE = 5          /* Invalid state transition */
} atlas_error_t;

/**
 * Get the API version at runtime.
 * @return API version number (currently 1)
 */
ATLAS_API int atlas_get_api_version(void);

/* =============================================================================
 * OPAQUE TYPE DECLARATIONS
 * ============================================================================= */

/**
 * Opaque handle for a 12,288-byte conservation domain.
 * Manages budget tracking and witness generation.
 * Ownership: Created by atlas_domain_create(), destroyed by atlas_domain_destroy().
 */
typedef struct atlas_domain atlas_domain_t;

/**
 * Opaque handle for cryptographic witness data.
 * Immutable after generation, used for data integrity verification.
 * Ownership: Created by atlas_witness_generate(), destroyed by atlas_witness_destroy().
 */
typedef struct atlas_witness atlas_witness_t;

/* =============================================================================
 * LAYER 2 - CONSERVATION (domains, witnesses, budgets)
 * ============================================================================= */

/**
 * Create a new conservation domain.
 * @param bytes Expected size (should be 12288 for standard domains)
 * @param budget_class Initial budget classification (0-95)
 * @return New domain handle, or NULL on allocation failure
 * @ownership Caller owns the returned domain, must call atlas_domain_destroy()
 */
ATLAS_API atlas_domain_t* atlas_domain_create(size_t bytes, uint8_t budget_class);

/**
 * Attach a memory region to a domain for conservation tracking.
 * @param domain Domain handle (must not be NULL)
 * @param base Pointer to memory region (non-owning)
 * @param len Size of memory region in bytes
 * @return ATLAS_OK on success, error code on failure
 * @ownership Domain does not take ownership of the memory region
 */
ATLAS_API int atlas_domain_attach(atlas_domain_t* domain, void* base, size_t len);

/**
 * Verify conservation laws are satisfied for the attached region.
 * @param domain Domain handle (must not be NULL)
 * @return true if conservation laws hold, false otherwise
 */
ATLAS_API bool atlas_domain_verify(const atlas_domain_t* domain);

/**
 * Commit a domain from OPEN to COMMITTED state.
 * Generates witness and finalizes conservation state.
 * @param domain Domain handle (must not be NULL)
 * @return ATLAS_OK on success, ATLAS_E_STATE if already committed, other errors possible
 * @thread_safety This operation uses atomic CAS for thread-safe state transitions
 */
ATLAS_API int atlas_domain_commit(atlas_domain_t* domain);

/**
 * Destroy a domain and free all associated resources.
 * @param domain Domain handle (may be NULL, in which case this is a no-op)
 * @ownership Caller must not use domain handle after this call
 */
ATLAS_API void atlas_domain_destroy(atlas_domain_t* domain);

/**
 * Allocate budget units (mod-96 arithmetic).
 * @param domain Domain handle (must not be NULL)
 * @param amt Amount to allocate (0-95)
 * @return true if allocation succeeded, false if insufficient budget
 * @thread_safety Uses atomic operations for thread-safe budget tracking
 */
ATLAS_API bool atlas_budget_alloc(atlas_domain_t* domain, uint8_t amt);

/**
 * Release budget units back to the pool.
 * @param domain Domain handle (must not be NULL)
 * @param amt Amount to release (0-95)
 * @return true if release succeeded, false if would exceed budget bounds
 * @thread_safety Uses atomic operations for thread-safe budget tracking
 */
ATLAS_API bool atlas_budget_release(atlas_domain_t* domain, uint8_t amt);

/**
 * Generate a cryptographic witness for a memory region.
 * @param base Pointer to data (must not be NULL)
 * @param len Size of data in bytes
 * @return Witness handle, or NULL on failure
 * @ownership Caller owns the returned witness, must call atlas_witness_destroy()
 */
ATLAS_API atlas_witness_t* atlas_witness_generate(const void* base, size_t len);

/**
 * Verify a witness against a memory region.
 * @param witness Witness handle (must not be NULL)
 * @param base Pointer to data to verify (must not be NULL)
 * @param len Size of data in bytes
 * @return true if witness matches data, false otherwise
 */
ATLAS_API bool atlas_witness_verify(const atlas_witness_t* witness, const void* base, size_t len);

/**
 * Destroy a witness and free associated resources.
 * @param witness Witness handle (may be NULL, in which case this is a no-op)
 * @ownership Caller must not use witness handle after this call
 */
ATLAS_API void atlas_witness_destroy(atlas_witness_t* witness);

/**
 * Compute conservation delta between two memory regions.
 * @param before Pointer to "before" state (must not be NULL)
 * @param after Pointer to "after" state (must not be NULL)
 * @param len Size of regions in bytes (both must be same size)
 * @return Delta value (0-127, typically 7-bit range)
 */
ATLAS_API uint8_t atlas_conserved_delta(const void* before, const void* after, size_t len);

/* =============================================================================
 * LAYER 3 - RESONANCE (classification, clustering, scheduling)
 * ============================================================================= */

/**
 * Classify a single 256-byte page into R96 resonance classes.
 * @param in256 Input page data (must be exactly 256 bytes)
 * @param out256 Output classification array (must be 256 bytes)
 * @note Each output[i] contains the R96 class (0-95) for input[i]
 */
ATLAS_API void atlas_r96_classify_page(const uint8_t* in256, uint8_t out256[256]);

/**
 * Generate histogram of R96 resonance classes for a 256-byte page.
 * @param in256 Input page data (must be exactly 256 bytes)
 * @param out96 Output histogram (must be 96 uint16_t elements)
 * @note out96[r] contains count of bytes classified as resonance class r
 */
ATLAS_API void atlas_r96_histogram_page(const uint8_t* in256, uint16_t out96[96]);

/**
 * Compressed Sparse Row (CSR) representation of resonance clusters.
 * @field offsets Array of 97 elements: offsets[r] to offsets[r+1]-1 gives indices for class r
 * @field indices Array of Φ-linearized coordinates (page*256 + offset)
 * @field n Total number of coordinates stored
 * @alignment All pointers are byte-aligned unless otherwise specified
 */
typedef struct {
    const uint32_t* offsets;  /* Array of length 97 */
    const uint32_t* indices;  /* Array of length n */
    uint32_t n;               /* Total coordinate count */
} atlas_cluster_view;

/**
 * Build resonance clusters over multiple pages using CSR format.
 * @param base Pointer to page data (must be pages*256 bytes)
 * @param pages Number of 256-byte pages to process
 * @return Cluster view with CSR data
 * @ownership Caller owns returned data, must call atlas_cluster_destroy()
 * @note Clusters are homogeneous (each contains only one resonance class)
 * @note Indices within each cluster are sorted in ascending order
 */
ATLAS_API atlas_cluster_view atlas_cluster_by_resonance(const uint8_t* base, size_t pages);

/**
 * Destroy cluster data and free associated memory.
 * @param clusters Cluster view (may have NULL pointers, in which case this is a no-op)
 * @ownership Caller must not use cluster data after this call
 */
ATLAS_API void atlas_cluster_destroy(atlas_cluster_view* clusters);

/**
 * Calculate next harmonic window time for a given resonance class.
 * Phase-locked scheduling aligned to 96-unit cycles.
 * @param now Current time reference
 * @param r Resonance class (0-95)
 * @return Next harmonic window time >= now
 * @note Algorithm: now + ((96 - ((now + r) % 96)) % 96)
 */
ATLAS_API uint64_t atlas_next_harmonic_window_from(uint64_t now, uint8_t r);

/**
 * Convenience function: next harmonic window from time 0.
 * @param r Resonance class (0-95)
 * @return Next harmonic window time from epoch
 */
static inline uint64_t atlas_next_harmonic_window(uint8_t r) {
    return atlas_next_harmonic_window_from(0, r);
}

/* =============================================================================
 * METRICS & DEBUGGING (compiled out in release builds)
 * ============================================================================= */

#ifndef NDEBUG
/**
 * Enable or disable internal performance metrics collection.
 * @param enabled 0 to disable, non-zero to enable
 * @note Only available in debug builds, no-op in release builds
 */
ATLAS_API void atlas_enable_metrics(int enabled);
#else
#define atlas_enable_metrics(enabled) ((void)0)
#endif

/* =============================================================================
 * IMPLEMENTATION NOTES
 * ============================================================================= */

/*
 * MEMORY OWNERSHIP:
 * - Functions returning pointers (atlas_domain_create, atlas_witness_generate,
 *   atlas_cluster_by_resonance) transfer ownership to caller
 * - Caller must call corresponding destroy function to avoid leaks
 * - Functions taking const pointers do not take ownership
 * - destroy functions accept NULL pointers safely
 *
 * THREAD SAFETY:
 * - atlas_domain_commit uses atomic CAS for safe state transitions
 * - atlas_budget_* functions use atomic operations for budget tracking
 * - Witness handles are immutable after generation
 * - Cluster operations are not thread-safe, use external synchronization
 *
 * WASM COMPATIBILITY:
 * - Single-threaded builds replace atomics with simple operations
 * - Define ATLAS_SINGLE_THREAD to disable atomic operations
 * - Memory allocation uses standard malloc/free or custom arena
 *
 * ALIGNMENT:
 * - All data is byte-aligned unless otherwise specified
 * - No special alignment requirements for performance-critical paths
 * - SIMD optimizations handled transparently by implementation
 *
 * ERROR HANDLING:
 * - Functions return error codes (atlas_error_t) or bool for success/failure
 * - NULL return values indicate allocation failures
 * - No exceptions thrown across C ABI boundary
 * - Check return values for proper error handling
 */

#ifdef __cplusplus
}
#endif

#endif /* ATLAS_H */