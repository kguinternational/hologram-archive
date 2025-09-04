/* conservation_verify.c - Atlas Layer 4 Conservation Verification Implementation
 * (c) 2024-2025 UOR Foundation - MIT License
 * 
 * Implementation of conservation verification utilities for Atlas Layer 4 benchmarks.
 */

#include "conservation_verify.h"
#include "atlas-conservation.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <stdatomic.h>

// =============================================================================
// Global State
// =============================================================================

// Conservation benchmark metrics (thread-safe with atomics)
static atomic_uint_least64_t g_total_operations = ATOMIC_VAR_INIT(0);
static atomic_uint_least64_t g_conserved_operations = ATOMIC_VAR_INIT(0);
static atomic_uint_least64_t g_violated_operations = ATOMIC_VAR_INIT(0);
static atomic_uint_least64_t g_total_witnesses = ATOMIC_VAR_INIT(0);
static atomic_uint_least64_t g_verified_witnesses = ATOMIC_VAR_INIT(0);
static atomic_uint_least64_t g_total_bytes_processed = ATOMIC_VAR_INIT(0);

// Global Layer 2 domain for benchmark operations
static void* g_benchmark_domain = NULL;
static atomic_uint_least64_t g_witness_sequence = ATOMIC_VAR_INIT(1);

// =============================================================================
// Core Conservation Verification Functions
// =============================================================================

conservation_result_t verify_conservation(const uint8_t* before, const uint8_t* after, size_t size) {
    conservation_result_t result = {0};
    
    if (!before || !after || size == 0) {
        return result;
    }
    
    // Calculate conservation sums using Atlas Layer 2 functions
    result.before_sum = (uint8_t)(atlas_conserved_sum(before, size) % 96);
    result.after_sum = (uint8_t)(atlas_conserved_sum(after, size) % 96);
    
    // Calculate conservation delta using Atlas Layer 2 function
    result.delta = atlas_conserved_delta(before, after, size);
    
    // Check if conservation is maintained (delta should be 0)
    result.is_conserved = (result.delta == 0);
    
    // Generate witness for verification if we have a domain
    if (g_benchmark_domain) {
        atlas_witness_t* witness = atlas_witness_generate(after, size);
        if (witness) {
            result.witness_verified = atlas_witness_verify(witness, after, size);
            atlas_witness_destroy(witness);
            atomic_fetch_add(&g_total_witnesses, 1);
            if (result.witness_verified) {
                atomic_fetch_add(&g_verified_witnesses, 1);
            }
        }
    }
    
    return result;
}

uint64_t generate_witness_timestamp(witness_timestamp_ctx_t* ctx) {
    if (!ctx) {
        return 0;
    }
    
    // Generate monotonic UN timestamp
    uint64_t sequence = atomic_fetch_add(&g_witness_sequence, 1);
    ctx->sequence_id = sequence;
    ctx->generation_time = (uint64_t)time(NULL);
    
    // Create UN timestamp: high 32 bits = domain_id ^ operation_id, low 32 bits = sequence
    uint64_t timestamp = ((uint64_t)(ctx->domain_id ^ ctx->operation_id) << 32) | (sequence & 0xFFFFFFFF);
    
    return timestamp;
}

bool verify_witness_chain(witness_chain_ctx_t* chain, const uint8_t* data, size_t size) {
    if (!chain || !data || size == 0) {
        return false;
    }
    
    // For now, implement basic witness verification
    // In a full implementation, this would verify the entire chain
    if (chain->witness) {
        atlas_witness_t* witness = (atlas_witness_t*)chain->witness;
        chain->integrity_verified = atlas_witness_verify(witness, data, size);
        return chain->integrity_verified;
    }
    
    return false;
}

// =============================================================================
// Benchmark Integration Functions
// =============================================================================

bool init_conservation_benchmark(size_t domain_size, uint8_t budget_class) {
    // Create Layer 2 conservation domain
    g_benchmark_domain = atlas_domain_create(domain_size, budget_class);
    if (!g_benchmark_domain) {
        fprintf(stderr, "Failed to create Layer 2 conservation domain\n");
        return false;
    }
    
    // Reset metrics
    reset_conservation_metrics();
    
    return true;
}

void cleanup_conservation_benchmark(void) {
    if (g_benchmark_domain) {
        atlas_domain_destroy((atlas_domain_t*)g_benchmark_domain);
        g_benchmark_domain = NULL;
    }
}

void record_conservation_result(const conservation_result_t* result) {
    if (!result) {
        return;
    }
    
    atomic_fetch_add(&g_total_operations, 1);
    
    if (result->is_conserved) {
        atomic_fetch_add(&g_conserved_operations, 1);
    } else {
        atomic_fetch_add(&g_violated_operations, 1);
    }
}

bool get_conservation_metrics(conservation_metrics_t* metrics) {
    if (!metrics) {
        return false;
    }
    
    uint64_t total_ops = atomic_load(&g_total_operations);
    uint64_t conserved_ops = atomic_load(&g_conserved_operations);
    uint64_t violated_ops = atomic_load(&g_violated_operations);
    uint64_t total_witnesses = atomic_load(&g_total_witnesses);
    uint64_t verified_witnesses = atomic_load(&g_verified_witnesses);
    uint64_t total_bytes = atomic_load(&g_total_bytes_processed);
    
    metrics->total_operations = total_ops;
    metrics->conserved_operations = conserved_ops;
    metrics->violated_operations = violated_ops;
    metrics->total_witnesses = total_witnesses;
    metrics->verified_witnesses = verified_witnesses;
    metrics->total_bytes_processed = total_bytes;
    
    // Calculate rates
    if (total_ops > 0) {
        metrics->conservation_rate = (double)conserved_ops / (double)total_ops * 100.0;
    } else {
        metrics->conservation_rate = 0.0;
    }
    
    if (total_witnesses > 0) {
        metrics->witness_verification_rate = (double)verified_witnesses / (double)total_witnesses * 100.0;
    } else {
        metrics->witness_verification_rate = 0.0;
    }
    
    return true;
}

void reset_conservation_metrics(void) {
    atomic_store(&g_total_operations, 0);
    atomic_store(&g_conserved_operations, 0);
    atomic_store(&g_violated_operations, 0);
    atomic_store(&g_total_witnesses, 0);
    atomic_store(&g_verified_witnesses, 0);
    atomic_store(&g_total_bytes_processed, 0);
    atomic_store(&g_witness_sequence, 1);
}

// =============================================================================
// Utility Functions
// =============================================================================

uint8_t calculate_conservation_sum(const uint8_t* data, size_t size) {
    if (!data || size == 0) {
        return 0;
    }
    
    return (uint8_t)(atlas_conserved_sum(data, size) % 96);
}

bool is_conserved(const uint8_t* data, size_t size) {
    if (!data || size == 0) {
        return false;
    }
    
    return atlas_conserved_check(data, size);
}

bool generate_conserved_random_data(uint8_t* buffer, size_t size, uint32_t seed) {
    if (!buffer || size == 0) {
        return false;
    }
    
    srand(seed);
    
    // Generate random data for all but the last byte
    uint32_t sum = 0;
    for (size_t i = 0; i < size - 1; i++) {
        buffer[i] = (uint8_t)(rand() % 256);
        sum += buffer[i];
    }
    
    // Set the last byte to ensure conservation (sum % 96 == 0)
    uint8_t remainder = (uint8_t)(sum % 96);
    if (remainder == 0) {
        buffer[size - 1] = 0;
    } else {
        buffer[size - 1] = (uint8_t)(96 - remainder);
    }
    
    // Verify our work
    return is_conserved(buffer, size);
}

void* create_conservation_domain(size_t bytes, uint8_t budget_class) {
    return atlas_domain_create(bytes, budget_class);
}

void destroy_conservation_domain(void* domain) {
    if (domain) {
        atlas_domain_destroy((atlas_domain_t*)domain);
    }
}

void print_conservation_result(const conservation_result_t* result, const char* prefix) {
    if (!result) {
        return;
    }
    
    const char* pfx = prefix ? prefix : "";
    
    printf("%sConservation Result:\n", pfx);
    printf("%s  Conserved: %s\n", pfx, result->is_conserved ? "YES" : "NO");
    printf("%s  Before Sum: %u (mod 96)\n", pfx, result->before_sum);
    printf("%s  After Sum: %u (mod 96)\n", pfx, result->after_sum);
    printf("%s  Delta: %u\n", pfx, result->delta);
    printf("%s  Timestamp: %lu\n", pfx, result->timestamp);
    printf("%s  Witness Verified: %s\n", pfx, result->witness_verified ? "YES" : "NO");
}

void print_conservation_metrics(const conservation_metrics_t* metrics, const char* title) {
    if (!metrics) {
        return;
    }
    
    const char* t = title ? title : "Conservation Benchmark Metrics";
    
    printf("\n=== %s ===\n", t);
    printf("Total Operations: %lu\n", metrics->total_operations);
    printf("Conserved Operations: %lu\n", metrics->conserved_operations);
    printf("Violated Operations: %lu\n", metrics->violated_operations);
    printf("Conservation Rate: %.2f%%\n", metrics->conservation_rate);
    printf("Total Witnesses: %lu\n", metrics->total_witnesses);
    printf("Verified Witnesses: %lu\n", metrics->verified_witnesses);
    printf("Witness Verification Rate: %.2f%%\n", metrics->witness_verification_rate);
    printf("Total Bytes Processed: %lu\n", metrics->total_bytes_processed);
    printf("==============================\n\n");
}

// =============================================================================
// SIMD-Optimized Conservation Sum (when available)
// =============================================================================

#ifdef __AVX2__
#include <immintrin.h>

static uint8_t calculate_conservation_sum_avx2(const uint8_t* data, size_t size) {
    uint32_t sum = 0;
    size_t simd_end = size - (size % 32);
    
    // Process 32 bytes at a time with AVX2
    __m256i acc = _mm256_setzero_si256();
    for (size_t i = 0; i < simd_end; i += 32) {
        __m256i chunk = _mm256_loadu_si256((const __m256i*)(data + i));
        __m256i unpacked_lo = _mm256_unpacklo_epi8(chunk, _mm256_setzero_si256());
        __m256i unpacked_hi = _mm256_unpackhi_epi8(chunk, _mm256_setzero_si256());
        acc = _mm256_add_epi16(acc, unpacked_lo);
        acc = _mm256_add_epi16(acc, unpacked_hi);
    }
    
    // Horizontal sum of AVX2 register
    __m128i lo = _mm256_extracti128_si256(acc, 0);
    __m128i hi = _mm256_extracti128_si256(acc, 1);
    __m128i sum128 = _mm_add_epi16(lo, hi);
    
    // Horizontal sum within 128-bit register
    for (int i = 0; i < 8; i++) {
        sum += _mm_extract_epi16(sum128, i);
    }
    
    // Process remaining bytes
    for (size_t i = simd_end; i < size; i++) {
        sum += data[i];
    }
    
    return (uint8_t)(sum % 96);
}
#endif

// Override conservation sum calculation if SIMD is available
uint8_t calculate_conservation_sum_optimized(const uint8_t* data, size_t size) {
#ifdef __AVX2__
    if (size >= 32) {
        return calculate_conservation_sum_avx2(data, size);
    }
#endif
    return calculate_conservation_sum(data, size);
}