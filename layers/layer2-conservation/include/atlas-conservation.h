/* atlas-conservation.h - Atlas-12288 Layer 2 Conservation C API
 * (c) 2024-2025 UOR Foundation - MIT License
 * 
 * Public C ABI for Atlas-12288 Layer 2 Conservation layer providing:
 * - Domain lifecycle management with atomic state transitions
 * - Budget management with mod-96 arithmetic constraints
 * - Witness operations for cryptographic proofs
 * - Conservation verification and error handling
 */

#ifndef ATLAS_CONSERVATION_H
#define ATLAS_CONSERVATION_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

// =============================================================================
// Version Information
// =============================================================================

#define ATLAS_RUNTIME_VERSION_MAJOR 1
#define ATLAS_RUNTIME_VERSION_MINOR 0
#define ATLAS_RUNTIME_VERSION_PATCH 0

#define ATLAS_RUNTIME_VERSION \
    ((ATLAS_RUNTIME_VERSION_MAJOR << 16) | \
     (ATLAS_RUNTIME_VERSION_MINOR << 8) | \
     (ATLAS_RUNTIME_VERSION_PATCH))

// =============================================================================
// Opaque Type Declarations
// =============================================================================

/* Forward declarations for opaque types */
struct atlas_domain_internal;
struct atlas_witness_internal;

/* Opaque domain handle - actual structure is implementation-defined */
typedef struct atlas_domain_internal* atlas_domain_t;

/* Opaque witness handle - actual structure is implementation-defined */
typedef struct atlas_witness_internal* atlas_witness_t;

// =============================================================================
// Error Handling
// =============================================================================

/* Error codes returned by runtime operations */
typedef enum {
    ATLAS_OK = 0,                      /* Operation completed successfully */
    ATLAS_E_INVALID = 1,               /* Invalid function argument */
    ATLAS_E_MEMORY = 2,                /* Memory allocation failed */
    ATLAS_E_STATE = 3,                 /* Invalid domain state transition */
    ATLAS_E_BUDGET = 4,                /* Insufficient budget for operation */
    ATLAS_E_CONSERVATION = 5,          /* Conservation law violated */
    ATLAS_E_WITNESS = 6,               /* Witness verification failed */
    ATLAS_E_DESTROYED = 7              /* Domain has been destroyed */
} atlas_error_t;

/**
 * Get the last error code from the current thread.
 * 
 * @return Last error code, or ATLAS_OK if no error occurred
 */
atlas_error_t atlas_get_last_error(void);

// =============================================================================
// Domain Lifecycle Functions
// =============================================================================

/**
 * Create a new Atlas computation domain.
 * 
 * Creates a new domain with the specified memory allocation and budget class.
 * The domain starts in CREATED state and must be attached to memory before use.
 * 
 * @param bytes Size of memory to allocate for domain operations (must be > 0)
 * @param budget_class Initial budget class (0..95, mod-96 arithmetic)
 * @return New domain handle, or NULL on error (check atlas_get_last_error())
 * 
 * Thread safety: Safe to call from multiple threads
 * Memory: Caller must call atlas_domain_destroy() to free resources
 * 
 * Example:
 * ```c
 * atlas_domain_t* domain = atlas_domain_create(12288, 42);
 * if (!domain) {
 *     atlas_error_t error = atlas_get_last_error();
 *     // Handle error...
 * }
 * ```
 */
atlas_domain_t* atlas_domain_create(size_t bytes, uint8_t budget_class);

/**
 * Attach memory region to a domain for computation.
 * 
 * Transitions domain from CREATED to ATTACHED state. The memory region becomes
 * the computational space for domain operations. Conservation is tracked.
 * 
 * @param domain Domain handle (must be in CREATED state)
 * @param base Pointer to memory region (must be non-NULL)
 * @param len Length of memory region in bytes (must be > 0)
 * @return 0 on success, -1 on error (check atlas_get_last_error())
 * 
 * Thread safety: Safe to call from multiple threads on different domains
 * Memory: Domain does not take ownership of attached memory
 * 
 * Example:
 * ```c
 * uint8_t buffer[12288];
 * if (atlas_domain_attach(domain, buffer, sizeof(buffer)) != 0) {
 *     // Handle error...
 * }
 * ```
 */
int atlas_domain_attach(atlas_domain_t* domain, void* base, size_t len);

/**
 * Verify domain integrity and conservation laws.
 * 
 * Checks that the domain is in a valid state, attached memory is conserved,
 * and any bound witnesses are still valid. Can be called in ATTACHED,
 * VERIFIED, or COMMITTED states.
 * 
 * @param domain Domain handle (must not be NULL or DESTROYED)
 * @return true if domain is valid and conserved, false otherwise
 * 
 * Thread safety: Safe to call concurrently (read-only operation)
 * Performance: O(n) where n is attached memory size
 * 
 * Example:
 * ```c
 * if (!atlas_domain_verify(domain)) {
 *     atlas_error_t error = atlas_get_last_error();
 *     if (error == ATLAS_ERROR_CONSERVATION_VIOLATION) {
 *         // Conservation laws violated...
 *     }
 * }
 * ```
 */
bool atlas_domain_verify(const atlas_domain_t* domain);

/**
 * Commit domain changes and generate cryptographic witness.
 * 
 * Transitions domain to COMMITTED state, generates a witness for the current
 * memory state if not already present, and finalizes all changes.
 * 
 * @param domain Domain handle (must be in ATTACHED or VERIFIED state)
 * @return 0 on success, -1 on error (check atlas_get_last_error())
 * 
 * Thread safety: Not thread-safe (modifies domain state)
 * Performance: O(n) for witness generation where n is memory size
 * 
 * Example:
 * ```c
 * if (atlas_domain_commit(domain) != 0) {
 *     // Commit failed, domain state unchanged
 * }
 * ```
 */
int atlas_domain_commit(atlas_domain_t* domain);

/**
 * Destroy domain and free all associated resources.
 * 
 * Transitions domain to DESTROYED state, frees witnesses, clears memory,
 * and deallocates domain structure. Domain handle becomes invalid.
 * 
 * @param domain Domain handle (can be NULL - no-op)
 * 
 * Thread safety: Not thread-safe (modifies and frees domain)
 * Memory: Frees all domain resources, attached memory is not freed
 * 
 * Example:
 * ```c
 * atlas_domain_destroy(domain);
 * domain = NULL; // Prevent accidental reuse
 * ```
 */
void atlas_domain_destroy(atlas_domain_t* domain);

// =============================================================================
// Budget Management Functions
// =============================================================================

/**
 * Allocate budget units from domain's available budget.
 * 
 * Atomically reduces domain budget by the specified amount using mod-96
 * arithmetic. Budget represents computational "energy" or permission units.
 * 
 * @param domain Domain handle (must not be DESTROYED)
 * @param amt Amount to allocate (0..95, checked against available budget)
 * @return true on success, false if insufficient budget or error
 * 
 * Thread safety: Atomic operation, safe for concurrent access
 * Arithmetic: All operations use mod-96 arithmetic (0..95 range)
 * 
 * Example:
 * ```c
 * if (atlas_budget_alloc(domain, 10)) {
 *     // Successfully allocated 10 budget units
 *     // Perform operation...
 *     atlas_budget_release(domain, 10); // Return unused budget
 * }
 * ```
 */
bool atlas_budget_alloc(atlas_domain_t* domain, uint8_t amt);

/**
 * Release budget units back to domain's available budget.
 * 
 * Atomically increases domain budget by the specified amount using mod-96
 * arithmetic. Used to return unused budget after operations complete.
 * 
 * @param domain Domain handle (must not be DESTROYED)
 * @param amt Amount to release (0..95, added to current budget mod 96)
 * @return true on success, false on error
 * 
 * Thread safety: Atomic operation, safe for concurrent access
 * Arithmetic: All operations use mod-96 arithmetic (0..95 range)
 * 
 * Example:
 * ```c
 * // Release budget after operation completes
 * atlas_budget_release(domain, unused_budget);
 * ```
 */
bool atlas_budget_release(atlas_domain_t* domain, uint8_t amt);

// =============================================================================
// Witness Operations
// =============================================================================

/**
 * Generate cryptographic witness for memory region.
 * 
 * Creates a cryptographic proof (witness) that captures the current state
 * of the specified memory region. The witness can later verify that memory
 * has not been tampered with or has changed in expected ways.
 * 
 * @param base Pointer to memory region (must be non-NULL)
 * @param len Length of memory region in bytes (must be > 0)
 * @return New witness handle, or NULL on error
 * 
 * Thread safety: Safe to call from multiple threads
 * Memory: Caller must call atlas_witness_destroy() to free witness
 * Performance: O(n) where n is memory size
 * 
 * Example:
 * ```c
 * atlas_witness_t* witness = atlas_witness_generate(data, size);
 * if (!witness) {
 *     // Witness generation failed
 * }
 * ```
 */
atlas_witness_t* atlas_witness_generate(const void* base, size_t len);

/**
 * Verify that memory matches the cryptographic witness.
 * 
 * Checks that the current state of the memory region matches what was
 * captured when the witness was generated. Returns false if memory has
 * been modified in ways that violate conservation laws.
 * 
 * @param witness Witness handle (must be valid)
 * @param base Pointer to memory region (must be non-NULL)
 * @param len Length of memory region in bytes (must match witness)
 * @return true if memory matches witness, false otherwise
 * 
 * Thread safety: Safe to call concurrently (read-only operation)
 * Performance: O(n) where n is memory size
 * 
 * Example:
 * ```c
 * if (!atlas_witness_verify(witness, data, size)) {
 *     atlas_error_t error = atlas_get_last_error();
 *     if (error == ATLAS_ERROR_CONSERVATION_VIOLATION) {
 *         // Memory was modified in violation of conservation
 *     }
 * }
 * ```
 */
bool atlas_witness_verify(const atlas_witness_t* witness, const void* base, size_t len);

/**
 * Destroy witness and free associated resources.
 * 
 * Frees all resources associated with the witness handle. The witness
 * handle becomes invalid and must not be used after this call.
 * 
 * @param witness Witness handle (can be NULL - no-op)
 * 
 * Thread safety: Not thread-safe (modifies and frees witness)
 * Memory: Frees all witness resources
 * 
 * Example:
 * ```c
 * atlas_witness_destroy(witness);
 * witness = NULL; // Prevent accidental reuse
 * ```
 */
void atlas_witness_destroy(atlas_witness_t* witness);

// =============================================================================
// Conservation Functions
// =============================================================================

/**
 * Calculate conservation delta between two memory states.
 * 
 * Computes the change in conservation value between "before" and "after"
 * memory states using mod-96 arithmetic. A delta of 0 indicates perfect
 * conservation (no net change in computational "energy").
 * 
 * @param before Pointer to "before" memory state (must be non-NULL)
 * @param after Pointer to "after" memory state (must be non-NULL)  
 * @param len Length of both memory regions in bytes (must be > 0)
 * @return Conservation delta (0..95), 0 indicates perfect conservation
 * 
 * Thread safety: Safe to call from multiple threads (read-only)
 * Arithmetic: Uses mod-96 arithmetic, handles underflow correctly
 * Performance: O(n) where n is memory size
 * 
 * Example:
 * ```c
 * uint8_t delta = atlas_conserved_delta(old_state, new_state, size);
 * if (delta == 0) {
 *     // Perfect conservation - no energy lost or gained
 * } else {
 *     // Conservation delta of 'delta' units
 * }
 * ```
 */
uint8_t atlas_conserved_delta(const void* before, const void* after, size_t len);

/**
 * Check if a memory buffer satisfies conservation laws.
 * 
 * Verifies that the sum of all bytes in the buffer modulo 96 equals zero,
 * which indicates the memory region is in a conserved state under the
 * Atlas-12288 conservation mathematics.
 * 
 * @param data Pointer to memory buffer (must be non-NULL)
 * @param len Length of memory buffer in bytes (must be > 0)
 * @return true if buffer satisfies conservation laws, false otherwise
 * 
 * Thread safety: Safe to call from multiple threads (read-only)
 * Performance: O(n) where n is memory size, uses SIMD optimizations
 * 
 * Example:
 * ```c
 * if (atlas_conserved_check(buffer, size)) {
 *     // Buffer is conserved (sum % 96 == 0)
 * } else {
 *     // Buffer violates conservation laws
 * }
 * ```
 */
bool atlas_conserved_check(const void* data, size_t len);

/**
 * Check if a memory window satisfies conservation laws.
 * 
 * Specialized conservation check for memory windows with enhanced validation
 * and performance optimizations. Provides additional safety checks compared
 * to the basic conservation check function.
 * 
 * @param data Pointer to memory window (must be non-NULL)
 * @param len Length of memory window in bytes (must be > 0 and <= 1MB)
 * @return true if window satisfies conservation laws, false otherwise
 * 
 * Thread safety: Safe to call from multiple threads (read-only)
 * Performance: O(n) where n is memory size, uses SIMD optimizations
 * Validation: Enhanced bounds checking and memory safety validation
 * 
 * Example:
 * ```c
 * if (atlas_conserved_window_check(window_data, window_size)) {
 *     // Memory window is conserved and valid
 * } else {
 *     // Window fails conservation or validation checks
 * }
 * ```
 */
bool atlas_conserved_window_check(const void* data, size_t len);

/**
 * Calculate raw conservation sum for a memory buffer.
 * 
 * Computes the sum of all bytes in the buffer modulo 96, returning the
 * raw conservation value. A result of 0 indicates perfect conservation.
 * 
 * @param data Pointer to memory buffer (must be non-NULL)
 * @param len Length of memory buffer in bytes (must be > 0)
 * @return Conservation sum modulo 96 (0..95)
 * 
 * Thread safety: Safe to call from multiple threads (read-only)
 * Performance: O(n) where n is memory size, uses SIMD optimizations
 * 
 * Example:
 * ```c
 * uint32_t conservation_sum = atlas_conserved_sum(buffer, size);
 * if (conservation_sum == 0) {
 *     // Perfect conservation
 * } else {
 *     // Conservation value is conservation_sum
 * }
 * ```
 */
uint32_t atlas_conserved_sum(const void* data, size_t len);

// =============================================================================
// Runtime Information Functions  
// =============================================================================

/**
 * Get runtime version information.
 * 
 * @return Runtime version packed as (major << 16) | (minor << 8) | patch
 */
static inline uint32_t atlas_runtime_version(void) {
    return ATLAS_RUNTIME_VERSION;
}

/**
 * Check if runtime was compiled with thread safety support.
 * 
 * @return true if thread-safe, false if single-threaded build
 */
bool atlas_runtime_is_thread_safe(void);

/**
 * Get human-readable error message for error code.
 * 
 * @param error Error code
 * @return Pointer to static error message string
 */
const char* atlas_error_string(atlas_error_t error);

// =============================================================================
// Layer 3 Clustering and Scheduling API
// =============================================================================

/**
 * Cluster view handle for CSR-based resonance clustering.
 * 
 * Encapsulates a Compressed Sparse Row (CSR) representation of pages grouped
 * by their resonance class (0..95). Provides efficient access to pages within
 * specific resonance classes for scheduling and analysis.
 */
typedef struct {
    void* data;  // Opaque pointer to internal cluster view structure
} atlas_cluster_view;

/**
 * Classify page content using R96 resonance classification.
 * 
 * Analyzes a 256-byte page and produces a 256-byte classification output
 * where each byte represents the resonance characteristics of the corresponding
 * input byte under mod-96 arithmetic.
 * 
 * @param in256 Input page data (must be exactly 256 bytes)
 * @param out256 Output classification array (must be exactly 256 bytes)
 * 
 * Thread safety: Safe to call from multiple threads with different data
 * Performance: O(1) - fixed 256-byte processing
 * 
 * Example:
 * ```c
 * uint8_t page[256], classification[256];
 * atlas_r96_classify_page(page, classification);
 * ```
 */
void atlas_r96_classify_page(const uint8_t* in256, uint8_t out256[256]);

/**
 * Generate histogram of resonance classes for a page.
 * 
 * Analyzes a 256-byte page and produces a histogram showing the count
 * of each resonance class (0..95) found within the page. Used for
 * statistical analysis and optimization decisions.
 * 
 * @param in256 Input page data (must be exactly 256 bytes)
 * @param out96 Output histogram array (must be exactly 96 uint16_t elements)
 * 
 * Thread safety: Safe to call from multiple threads with different data
 * Performance: O(1) - fixed 256-byte analysis
 * 
 * Example:
 * ```c
 * uint8_t page[256];
 * uint16_t histogram[96];
 * atlas_r96_histogram_page(page, histogram);
 * // histogram[r] contains count of resonance class r in page
 * ```
 */
void atlas_r96_histogram_page(const uint8_t* in256, uint16_t out96[96]);

/**
 * Build CSR-based cluster view from memory pages.
 * 
 * Scans the provided memory pages and groups them by resonance class
 * using a Compressed Sparse Row format for efficient access. The resulting
 * cluster view allows fast iteration over pages within specific resonance
 * classes for scheduling and batch processing.
 * 
 * @param base Base pointer to memory pages (must be aligned to 256-byte pages)
 * @param pages Number of 256-byte pages to process
 * @return Cluster view handle, or invalid handle on error
 * 
 * Thread safety: Safe to call from multiple threads with different memory
 * Memory: Caller must call atlas_cluster_destroy() to free resources
 * Performance: O(n) where n is number of pages
 * 
 * Example:
 * ```c
 * uint8_t memory[12288];  // 48 pages of 256 bytes each
 * atlas_cluster_view cluster = atlas_cluster_by_resonance(memory, 48);
 * if (cluster.data) {
 *     // Use cluster for scheduling...
 *     atlas_cluster_destroy(&cluster);
 * }
 * ```
 */
atlas_cluster_view atlas_cluster_by_resonance(const uint8_t* base, size_t pages);

/**
 * Destroy cluster view and free all associated resources.
 * 
 * Frees all memory associated with the cluster view including CSR arrays
 * and internal structures. The cluster view handle becomes invalid after
 * this call and must not be used.
 * 
 * @param cluster Pointer to cluster view handle (can be NULL - no-op)
 * 
 * Thread safety: Not thread-safe (modifies and frees cluster)
 * Memory: Frees all cluster resources using WASM-compatible arena
 * 
 * Example:
 * ```c
 * atlas_cluster_destroy(&cluster);
 * cluster.data = NULL; // Prevent accidental reuse
 * ```
 */
void atlas_cluster_destroy(atlas_cluster_view* cluster);

/**
 * Calculate next harmonic scheduling window from current time.
 * 
 * Determines the next optimal scheduling window based on the harmonic
 * properties of the specified resonance class. Uses complex harmonic
 * analysis to ensure proper synchronization with the underlying
 * mathematical structures.
 * 
 * @param now Current time or cycle count
 * @param r Resonance class (0..95, automatically reduced mod 96)
 * @return Next harmonic window time/cycle
 * 
 * Thread safety: Safe to call from multiple threads (pure function)
 * Arithmetic: Uses advanced harmonic analysis via LLVM IR
 * 
 * Example:
 * ```c
 * uint64_t current_cycle = get_current_cycle();
 * uint8_t resonance = 42;
 * uint64_t next_window = atlas_next_harmonic_window_from(current_cycle, resonance);
 * schedule_work_at(next_window);
 * ```
 */
uint64_t atlas_next_harmonic_window_from(uint64_t now, uint8_t r);

// =============================================================================
// Layer 2 Batch Processing API
// =============================================================================

/**
 * Batch buffer descriptor for efficient batch processing.
 */
typedef struct {
    void* data;           /* Pointer to buffer data */
    size_t size;         /* Buffer size in bytes */
    uint8_t status;      /* Processing status (0=pending, 1=success, 2=error) */
    uint8_t reserved[7]; /* Reserved for alignment */
} atlas_batch_buffer_t;

/**
 * Batch delta descriptor for delta computation.
 */
typedef struct {
    const void* before;  /* Pointer to "before" buffer */
    const void* after;   /* Pointer to "after" buffer */
    size_t size;        /* Buffer size in bytes */
    uint8_t delta;      /* Computed delta value (output) */
    uint8_t reserved[7]; /* Reserved for alignment */
} atlas_batch_delta_t;

/**
 * Batch witness descriptor for witness generation.
 */
typedef struct {
    const void* data;   /* Pointer to data buffer */
    size_t size;       /* Data size in bytes */
    atlas_witness_t* witness; /* Generated witness (output) */
    uint32_t status;   /* Generation status (0=error, 1=success) */
    uint8_t reserved[4]; /* Reserved for alignment */
} atlas_batch_witness_t;

/**
 * Batch processing statistics.
 */
typedef struct {
    uint64_t conserved_calls;    /* Number of batch conserved_check calls */
    uint64_t delta_calls;        /* Number of batch delta_compute calls */
    uint64_t witness_calls;      /* Number of batch witness_generate calls */
    uint64_t total_buffers;      /* Total buffers processed across all calls */
} atlas_batch_stats_t;

/**
 * Check multiple buffers for conservation in a single optimized call.
 * 
 * Processes multiple buffers simultaneously using SIMD vectorization and
 * optimized memory access patterns. Provides 2-3x performance improvement
 * over individual calls when processing many small buffers.
 * 
 * @param buffers Array of buffer descriptors to check
 * @param count Number of buffers to process (must be > 0, <= 256)
 * @return Array of results (1 byte per buffer: 1=conserved, 0=not conserved), 
 *         or NULL on error. Caller must free the returned array.
 * 
 * Thread safety: Safe to call from multiple threads with different data
 * Performance: O(n*m) where n=count, m=average buffer size, with 2-3x speedup
 * Optimization: Uses SIMD for 8+ buffers, optimized for uniform buffer sizes
 * 
 * Example:
 * ```c
 * atlas_batch_buffer_t buffers[10];
 * // Initialize buffers...
 * uint8_t* results = atlas_batch_conserved_check(buffers, 10);
 * if (results) {
 *     for (int i = 0; i < 10; i++) {
 *         printf("Buffer %d: %s\n", i, results[i] ? "conserved" : "not conserved");
 *     }
 *     free(results);
 * }
 * ```
 */
uint8_t* atlas_batch_conserved_check(const atlas_batch_buffer_t* buffers, size_t count);

/**
 * Compute conservation deltas for multiple buffer pairs efficiently.
 * 
 * Computes the delta (change in conservation value) between "before" and "after"
 * states for multiple buffer pairs using vectorized operations and optimized
 * memory access patterns.
 * 
 * @param deltas Array of delta descriptors (before/after buffer pairs)
 * @param count Number of delta computations to perform (must be > 0, <= 256)
 * @return 0 on success, -1 on error. Results stored in delta descriptors.
 * 
 * Thread safety: Safe to call from multiple threads with different data
 * Performance: O(n*m) where n=count, m=average buffer size, with 2-3x speedup
 * Arithmetic: Uses mod-96 arithmetic, handles underflow correctly
 * 
 * Example:
 * ```c
 * atlas_batch_delta_t deltas[5];
 * // Initialize deltas with before/after buffer pairs...
 * if (atlas_batch_delta_compute(deltas, 5) == 0) {
 *     for (int i = 0; i < 5; i++) {
 *         printf("Delta %d: %u\n", i, deltas[i].delta);
 *     }
 * }
 * ```
 */
int atlas_batch_delta_compute(atlas_batch_delta_t* deltas, size_t count);

/**
 * Generate cryptographic witnesses for multiple data blocks efficiently.
 * 
 * Creates witnesses for multiple data blocks using pipelined witness generation
 * and optimized memory access patterns. Significantly faster than individual
 * witness generation calls for batch processing scenarios.
 * 
 * @param witnesses Array of witness descriptors
 * @param count Number of witnesses to generate (must be > 0, <= 256)
 * @return 0 on success, -1 on error. Results stored in witness descriptors.
 * 
 * Thread safety: Safe to call from multiple threads with different data
 * Performance: O(n*m) where n=count, m=average buffer size, with 2-3x speedup
 * Memory: Generated witnesses must be freed with atlas_witness_destroy()
 * 
 * Example:
 * ```c
 * atlas_batch_witness_t witnesses[3];
 * // Initialize witnesses with data pointers...
 * if (atlas_batch_witness_generate(witnesses, 3) == 0) {
 *     for (int i = 0; i < 3; i++) {
 *         if (witnesses[i].status == 1) {
 *             printf("Witness %d generated successfully\n", i);
 *             // Use witness...
 *             atlas_witness_destroy(witnesses[i].witness);
 *         }
 *     }
 * }
 * ```
 */
int atlas_batch_witness_generate(atlas_batch_witness_t* witnesses, size_t count);

/**
 * Get batch processing performance statistics.
 * 
 * Returns statistics about batch operation usage and performance for
 * monitoring and optimization purposes.
 * 
 * @param stats Output structure to receive statistics
 * @return true on success, false on error
 * 
 * Thread safety: Safe to call from multiple threads (atomic counters)
 * 
 * Example:
 * ```c
 * atlas_batch_stats_t stats;
 * if (atlas_batch_get_statistics(&stats)) {
 *     printf("Total buffers processed: %llu\n", stats.total_buffers);
 *     printf("Batch conserved calls: %llu\n", stats.conserved_calls);
 * }
 * ```
 */
bool atlas_batch_get_statistics(atlas_batch_stats_t* stats);

/**
 * Reset batch processing performance statistics.
 * 
 * Clears all performance counters for fresh measurement periods.
 * 
 * Thread safety: Safe to call from multiple threads (atomic operations)
 */
void atlas_batch_reset_statistics(void);

/**
 * Get optimal batch size for current system configuration.
 * 
 * Returns the recommended batch size for maximum performance based on
 * CPU cache size, SIMD capabilities, and buffer characteristics.
 * 
 * @param buffer_size Average expected buffer size
 * @return Recommended batch size (number of buffers per batch)
 * 
 * Thread safety: Safe to call from multiple threads (pure function)
 * 
 * Example:
 * ```c
 * size_t optimal_batch = atlas_batch_get_optimal_size(256);
 * // Process buffers in batches of 'optimal_batch' for best performance
 * ```
 */
size_t atlas_batch_get_optimal_size(size_t buffer_size);

// =============================================================================
// Layer 3 Extended Utility Functions
// =============================================================================

/**
 * Get count of pages in a specific resonance class.
 * 
 * @param cluster Valid cluster view handle
 * @param resonance_class Resonance class (0..95)
 * @return Number of pages in the specified class
 */
size_t atlas_cluster_count_for_resonance(atlas_cluster_view cluster, uint8_t resonance_class);

/**
 * Get page indices for a specific resonance class.
 * 
 * @param cluster Valid cluster view handle  
 * @param resonance_class Resonance class (0..95)
 * @param count Output parameter for number of pages returned
 * @return Pointer to array of page indices, or NULL on error
 */
const uint32_t* atlas_cluster_pages_for_resonance(atlas_cluster_view cluster, uint8_t resonance_class, size_t* count);

/**
 * Check if two resonance classes harmonize.
 * 
 * @param r1 First resonance class (0..95)
 * @param r2 Second resonance class (0..95)  
 * @return true if the classes harmonize, false otherwise
 */
bool atlas_resonance_harmonizes(uint8_t r1, uint8_t r2);

/**
 * Get resonance class for a specific page.
 * 
 * @param page256 Pointer to 256-byte page data
 * @return Resonance class (0..95)
 */
uint8_t atlas_page_resonance_class(const uint8_t* page256);

/**
 * Calculate next scheduling window using simple formula.
 * 
 * Uses the formula: next = now + ((96 - ((now + r) % 96)) % 96)
 * for straightforward scheduling without complex harmonic analysis.
 * 
 * @param now Current time or cycle count
 * @param r Resonance class (0..95)
 * @return Next scheduling window
 */
uint64_t atlas_schedule_next_window(uint64_t now, uint8_t r);

/**
 * Validate cluster view structure integrity.
 * 
 * @param cluster Cluster view to validate
 * @return true if cluster is valid, false otherwise
 */
bool atlas_cluster_validate(atlas_cluster_view cluster);

/**
 * Batch classify multiple pages using R96 resonance classification.
 * 
 * @param base Base pointer to memory pages (256-byte aligned)
 * @param pages Number of pages to classify
 * @param classifications Output array to store resonance classes for each page
 */
void atlas_r96_classify_pages(const uint8_t* base, size_t pages, uint8_t* classifications);

/**
 * Generate histograms for multiple pages.
 * 
 * @param base Base pointer to memory pages (256-byte aligned)
 * @param pages Number of pages to process
 * @param histograms Output array to store histograms (pages * 96 uint16_t elements)
 */
void atlas_r96_histogram_pages(const uint8_t* base, size_t pages, uint16_t* histograms);

/**
 * Get cluster view statistics.
 * 
 * @param cluster Valid cluster view handle
 * @param total_pages Output parameter for total number of pages
 * @param non_empty_classes Output parameter for number of non-empty resonance classes
 * @param largest_class Output parameter for size of largest resonance class
 */
void atlas_cluster_stats(atlas_cluster_view cluster, size_t* total_pages, size_t* non_empty_classes, size_t* largest_class);

// =============================================================================
// Compilation and Linking Notes
// =============================================================================

/*
 * Compilation:
 *   gcc -std=c11 -I/path/to/atlas/runtime/include -c your_code.c
 *
 * Linking:
 *   gcc your_code.o -L/path/to/atlas/runtime/lib -latlas-runtime -lm
 *
 * Thread Safety:
 *   - Domain operations are thread-safe for different domains
 *   - Budget operations are atomically thread-safe 
 *   - Witness operations are thread-safe for read-only access
 *   - Single domain should not be modified by multiple threads simultaneously
 *
 * Memory Management:
 *   - All atlas_*_create() functions require matching atlas_*_destroy() calls
 *   - Attached memory is not owned by domains - caller manages lifetime
 *   - Error conditions are indicated by return values and atlas_get_last_error()
 *
 * Performance Notes:
 *   - Verification operations are O(n) in memory size
 *   - Budget operations are O(1) with atomic synchronization
 *   - Witness generation/verification involves cryptographic computation
 */

#ifdef __cplusplus
}
#endif

#endif /* ATLAS_CONSERVATION_H */