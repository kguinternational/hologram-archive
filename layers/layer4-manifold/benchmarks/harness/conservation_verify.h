/* conservation_verify.h - Atlas Layer 4 Conservation Verification Harness
 * (c) 2024-2025 UOR Foundation - MIT License
 * 
 * Provides conservation verification utilities for Atlas Layer 4 manifold benchmarks.
 * Ensures all operations maintain the fundamental conservation law: sum(bytes) % 96 == constant.
 */

#ifndef CONSERVATION_VERIFY_H
#define CONSERVATION_VERIFY_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <time.h>

#ifdef __cplusplus
extern "C" {
#endif

// =============================================================================
// Conservation Verification Types
// =============================================================================

/**
 * Conservation verification result structure.
 */
typedef struct {
    bool is_conserved;              // True if conservation is maintained
    uint8_t before_sum;            // Conservation sum before operation (mod 96)
    uint8_t after_sum;             // Conservation sum after operation (mod 96)  
    uint8_t delta;                 // Conservation delta (0 = perfect conservation)
    uint64_t timestamp;            // UN timestamp from witness
    bool witness_verified;         // True if witness verification passed
} conservation_result_t;

/**
 * Universal Number timestamp context for witness generation.
 */
typedef struct {
    uint64_t sequence_id;          // Monotonic sequence counter
    uint32_t domain_id;           // Domain identifier for witness
    uint32_t operation_id;        // Operation type identifier
    uint64_t generation_time;     // System time when generated
    void* layer2_domain;          // Layer 2 domain handle (opaque)
} witness_timestamp_ctx_t;

/**
 * Witness chain verification context.
 */
typedef struct {
    void* witness;                // Atlas witness handle
    uint64_t timestamp;           // UN timestamp
    uint32_t chain_length;        // Number of witnesses in chain
    bool integrity_verified;      // True if chain integrity is valid
} witness_chain_ctx_t;

/**
 * Conservation benchmark metrics.
 */
typedef struct {
    uint64_t total_operations;     // Total operations performed
    uint64_t conserved_operations; // Operations that maintained conservation
    uint64_t violated_operations;  // Operations that violated conservation
    double conservation_rate;      // Percentage of conserved operations
    uint64_t total_witnesses;      // Total witnesses generated
    uint64_t verified_witnesses;   // Successfully verified witnesses
    double witness_verification_rate; // Percentage of verified witnesses
    uint64_t total_bytes_processed; // Total bytes processed across all operations
} conservation_metrics_t;

// =============================================================================
// Core Conservation Verification Functions
// =============================================================================

/**
 * Verify conservation between before and after memory states.
 * 
 * Checks that the conservation law sum(bytes) % 96 is preserved across
 * an operation. Returns detailed verification results including delta
 * calculation and witness verification if applicable.
 * 
 * @param before Pointer to "before" memory state (must be non-NULL)
 * @param after Pointer to "after" memory state (must be non-NULL)
 * @param size Size of both memory regions in bytes (must be > 0)
 * @return Conservation verification result
 * 
 * Thread safety: Safe to call from multiple threads (read-only operation)
 * Performance: O(n) where n is memory size
 */
conservation_result_t verify_conservation(const uint8_t* before, const uint8_t* after, size_t size);

/**
 * Generate Universal Number timestamp for witness operations.
 * 
 * Creates a monotonic UN timestamp that serves as a verifiable certificate
 * for computation ordering. Uses Layer 2's witness infrastructure to ensure
 * conservation-preserving timestamp generation.
 * 
 * @param ctx Witness timestamp context (must be initialized)
 * @return UN timestamp value, or 0 on error
 * 
 * Thread safety: Safe to call from multiple threads (atomic sequence generation)
 * Monotonicity: Guarantees monotonic increasing sequence within same domain
 */
uint64_t generate_witness_timestamp(witness_timestamp_ctx_t* ctx);

/**
 * Verify witness chain integrity and conservation properties.
 * 
 * Validates that a chain of witnesses maintains proper ordering,
 * conservation laws, and cryptographic integrity. Ensures that
 * the computational trail is verifiable and conservation-preserving.
 * 
 * @param chain Witness chain context (must be valid)
 * @param data Pointer to data being verified
 * @param size Size of data in bytes
 * @return true if witness chain is valid and conserved, false otherwise
 * 
 * Thread safety: Safe to call concurrently (read-only operation)
 * Performance: O(n*m) where n is chain length, m is data size
 */
bool verify_witness_chain(witness_chain_ctx_t* chain, const uint8_t* data, size_t size);

// =============================================================================
// Benchmark Integration Functions
// =============================================================================

/**
 * Initialize conservation verification for benchmark run.
 * 
 * Sets up conservation tracking infrastructure including Layer 2 domain
 * creation, witness timestamp contexts, and metric collection systems.
 * Must be called before running benchmarks that require conservation.
 * 
 * @param domain_size Size of Layer 2 domain to allocate
 * @param budget_class Budget class for domain operations (0..95)
 * @return true on successful initialization, false on error
 * 
 * Thread safety: Not thread-safe (call once before benchmark threads start)
 * Memory: Caller must call cleanup_conservation_benchmark() to free resources
 */
bool init_conservation_benchmark(size_t domain_size, uint8_t budget_class);

/**
 * Clean up conservation verification resources after benchmark run.
 * 
 * Destroys Layer 2 domains, witness contexts, and frees all allocated
 * resources used for conservation verification during benchmarking.
 * 
 * Thread safety: Not thread-safe (call once after all benchmark threads complete)
 * Memory: Frees all resources allocated by init_conservation_benchmark()
 */
void cleanup_conservation_benchmark(void);

/**
 * Record conservation verification result for benchmark metrics.
 * 
 * Adds a conservation verification result to the running benchmark
 * metrics for later analysis and reporting. Updates counters and
 * calculates running averages.
 * 
 * @param result Conservation verification result to record
 * 
 * Thread safety: Safe to call from multiple threads (atomic updates)
 */
void record_conservation_result(const conservation_result_t* result);

/**
 * Get current conservation benchmark metrics.
 * 
 * Returns a snapshot of the current conservation verification metrics
 * including success rates, violation counts, and performance data.
 * 
 * @param metrics Output structure to receive metrics
 * @return true on success, false on error
 * 
 * Thread safety: Safe to call from multiple threads (atomic reads)
 */
bool get_conservation_metrics(conservation_metrics_t* metrics);

/**
 * Reset conservation benchmark metrics.
 * 
 * Clears all metric counters and resets the benchmark tracking state
 * for a fresh measurement period.
 * 
 * Thread safety: Safe to call from multiple threads (atomic operations)
 */
void reset_conservation_metrics(void);

// =============================================================================
// Utility Functions
// =============================================================================

/**
 * Calculate conservation sum for a memory buffer.
 * 
 * Computes sum(bytes) % 96 for the given memory region using
 * optimized SIMD operations where available.
 * 
 * @param data Pointer to memory buffer (must be non-NULL)
 * @param size Size of memory buffer in bytes (must be > 0)
 * @return Conservation sum modulo 96 (0..95)
 * 
 * Thread safety: Safe to call from multiple threads (read-only operation)
 * Performance: O(n) with SIMD optimizations
 */
uint8_t calculate_conservation_sum(const uint8_t* data, size_t size);

/**
 * Check if data satisfies Atlas conservation laws.
 * 
 * Verifies that sum(bytes) % 96 == 0, indicating perfect conservation.
 * 
 * @param data Pointer to memory buffer (must be non-NULL)
 * @param size Size of memory buffer in bytes (must be > 0)
 * @return true if data is conserved, false otherwise
 * 
 * Thread safety: Safe to call from multiple threads (read-only operation)
 */
bool is_conserved(const uint8_t* data, size_t size);

/**
 * Generate conservation-preserving random data for testing.
 * 
 * Creates random data that satisfies conservation laws (sum % 96 == 0)
 * for use in benchmark testing and validation.
 * 
 * @param buffer Output buffer for random data (must be pre-allocated)
 * @param size Size of buffer in bytes (must be > 0)
 * @param seed Random seed value
 * @return true on success, false on error
 * 
 * Thread safety: Safe to call from multiple threads with different buffers
 */
bool generate_conserved_random_data(uint8_t* buffer, size_t size, uint32_t seed);

/**
 * Create Layer 2 domain for conservation operations.
 * 
 * Creates and initializes a Layer 2 conservation domain for use in
 * benchmark operations that require domain-based conservation tracking.
 * 
 * @param bytes Size of domain memory allocation
 * @param budget_class Budget class for domain (0..95)
 * @return Opaque domain handle, or NULL on error
 * 
 * Thread safety: Safe to call from multiple threads
 * Memory: Caller must call destroy_conservation_domain() to free domain
 */
void* create_conservation_domain(size_t bytes, uint8_t budget_class);

/**
 * Destroy Layer 2 conservation domain and free resources.
 * 
 * @param domain Domain handle (can be NULL - no-op)
 * 
 * Thread safety: Not thread-safe (modifies and frees domain)
 * Memory: Frees all domain resources
 */
void destroy_conservation_domain(void* domain);

/**
 * Print conservation verification result in human-readable format.
 * 
 * @param result Conservation result to print
 * @param prefix Optional prefix string for output formatting
 */
void print_conservation_result(const conservation_result_t* result, const char* prefix);

/**
 * Print conservation benchmark metrics in human-readable format.
 * 
 * @param metrics Conservation metrics to print
 * @param title Optional title string for output formatting
 */
void print_conservation_metrics(const conservation_metrics_t* metrics, const char* title);

// =============================================================================
// Benchmark Macros
// =============================================================================

/**
 * Macro to wrap a benchmark operation with conservation verification.
 * 
 * Usage:
 * ```c
 * VERIFY_CONSERVATION_OP(
 *     data, size,                    // Memory region to verify
 *     {                              // Operation block
 *         result = my_operation(data, params);
 *     },
 *     "My Operation"                 // Operation name for reporting
 * );
 * ```
 */
#define VERIFY_CONSERVATION_OP(data_ptr, data_size, operation_block, op_name) \
    do { \
        uint8_t* _before_copy = malloc(data_size); \
        if (_before_copy) { \
            memcpy(_before_copy, data_ptr, data_size); \
            { operation_block } \
            conservation_result_t _result = verify_conservation(_before_copy, data_ptr, data_size); \
            record_conservation_result(&_result); \
            if (!_result.is_conserved) { \
                fprintf(stderr, "CONSERVATION VIOLATION in %s: delta=%u\n", op_name, _result.delta); \
            } \
            free(_before_copy); \
        } \
    } while(0)

/**
 * Macro to verify conservation with witness generation.
 * 
 * Usage:
 * ```c
 * VERIFY_CONSERVATION_WITH_WITNESS(
 *     data, size,                    // Memory region  
 *     witness_ctx,                   // Witness context
 *     {                              // Operation block
 *         result = my_operation(data, params);
 *     },
 *     "My Operation"                 // Operation name
 * );
 * ```
 */
#define VERIFY_CONSERVATION_WITH_WITNESS(data_ptr, data_size, witness_ctx, operation, op_name) \
    do { \
        uint8_t* _before_copy = malloc(data_size); \
        if (_before_copy) { \
            memcpy(_before_copy, data_ptr, data_size); \
            uint64_t _timestamp = generate_witness_timestamp(&witness_ctx); \
            operation; \
            conservation_result_t _result = verify_conservation(_before_copy, data_ptr, data_size); \
            _result.timestamp = _timestamp; \
            record_conservation_result(&_result); \
            if (!_result.is_conserved) { \
                fprintf(stderr, "CONSERVATION VIOLATION in %s: delta=%u, timestamp=%llu\n", \
                        op_name, _result.delta, _result.timestamp); \
            } \
            free(_before_copy); \
        } \
    } while(0)

#ifdef __cplusplus
}
#endif

#endif /* CONSERVATION_VERIFY_H */