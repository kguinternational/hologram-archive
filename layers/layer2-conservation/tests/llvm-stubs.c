/* llvm-stubs.c - LLVM function stubs for testing
 * (c) 2024-2025 UOR Foundation - MIT License
 * 
 * Simple stubs for LLVM functions to allow testing without full LLVM linkage.
 * These provide basic functionality for testing the C runtime layer.
 */

#include <stdint.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <stdio.h>

// =============================================================================
// LLVM Domain Function Stubs
// =============================================================================

void* atlas_domain_create_llvm(uint8_t initial_budget) {
    // Simple stub - return a dummy pointer
    (void)initial_budget;
    return malloc(64); // Small allocation for testing
}

void atlas_domain_destroy_llvm(void* domain) {
    if (domain) {
        free(domain);
    }
}

bool atlas_domain_validate_llvm(void* domain) {
    return domain != NULL;
}

void atlas_domain_reserve_budget_llvm(void* domain, uint8_t amount) {
    (void)domain;
    (void)amount;
    // No-op for testing
}

void atlas_domain_release_budget_llvm(void* domain, uint8_t amount) {
    (void)domain;
    (void)amount;
    // No-op for testing
}

uint8_t atlas_domain_available_budget_llvm(void* domain) {
    (void)domain;
    return 50; // Return a reasonable default
}

bool atlas_domain_can_afford_llvm(void* domain, uint8_t cost) {
    (void)domain;
    return cost <= 50; // Simple check against default budget
}

// =============================================================================
// LLVM Memory Function Stubs
// =============================================================================

void* atlas_alloc_aligned(size_t size) {
    // Simple aligned allocation
    void* ptr = malloc(size);
    return ptr;
}

bool atlas_conserved_check(const void* data, size_t length) {
    if (!data || length == 0) return false;
    
    // Simple conservation check: sum of all bytes mod 96 should be 0
    uint32_t sum = 0;
    const uint8_t* bytes = (const uint8_t*)data;
    
    for (size_t i = 0; i < length; i++) {
        sum += bytes[i];
    }
    
    return (sum % 96) == 0;
}

uint32_t atlas_conserved_sum(const void* data, size_t length) {
    if (!data || length == 0) return 0;
    
    uint32_t sum = 0;
    const uint8_t* bytes = (const uint8_t*)data;
    
    for (size_t i = 0; i < length; i++) {
        sum += bytes[i];
    }
    
    return sum;
}

uint8_t atlas_conserved_delta(const void* before, const void* after, size_t length) {
    if (!before || !after || length == 0) return 0;
    
    const uint8_t* b_bytes = (const uint8_t*)before;
    const uint8_t* a_bytes = (const uint8_t*)after;
    
    int32_t delta = 0;
    for (size_t i = 0; i < length; i++) {
        delta += (int32_t)a_bytes[i] - (int32_t)b_bytes[i];
    }
    
    // Handle negative deltas with mod 96
    while (delta < 0) delta += 96;
    return (uint8_t)(delta % 96);
}

// =============================================================================
// LLVM Witness Function Stubs
// =============================================================================

typedef struct {
    uint32_t magic;
    size_t data_length;
    uint8_t hash[32]; // Simple hash storage
} llvm_witness_stub_t;

void* atlas_witness_generate_llvm(const void* data, size_t length) {
    if (!data || length == 0) return NULL;
    
    llvm_witness_stub_t* witness = malloc(sizeof(llvm_witness_stub_t));
    if (!witness) return NULL;
    
    witness->magic = 0xDEADBEEF;
    witness->data_length = length;
    
    // Simple hash: XOR all bytes
    memset(witness->hash, 0, sizeof(witness->hash));
    const uint8_t* bytes = (const uint8_t*)data;
    for (size_t i = 0; i < length; i++) {
        witness->hash[i % 32] ^= bytes[i];
    }
    
    return witness;
}

bool atlas_witness_verify_llvm(void* witness, const void* data, size_t length) {
    if (!witness || !data || length == 0) return false;
    
    llvm_witness_stub_t* w = (llvm_witness_stub_t*)witness;
    if (w->magic != 0xDEADBEEF || w->data_length != length) {
        return false;
    }
    
    // Recompute hash and compare
    uint8_t current_hash[32];
    memset(current_hash, 0, sizeof(current_hash));
    const uint8_t* bytes = (const uint8_t*)data;
    for (size_t i = 0; i < length; i++) {
        current_hash[i % 32] ^= bytes[i];
    }
    
    return memcmp(w->hash, current_hash, sizeof(current_hash)) == 0;
}

void atlas_witness_destroy_llvm(void* witness) {
    if (witness) {
        llvm_witness_stub_t* w = (llvm_witness_stub_t*)witness;
        w->magic = 0; // Clear magic
        free(witness);
    }
}

// =============================================================================
// LLVM Batch Processing Function Stubs
// =============================================================================

void atlas_batch_conserved_check_llvm(const void* buffers, uint32_t count, uint8_t* results) {
    if (!buffers || count == 0 || !results) return;
    
    // Cast to LLVM batch buffer structure (matches ABI conversion)
    typedef struct {
        void* data;
        uint64_t size;      // uint64_t for LLVM ABI
        uint32_t status;
        uint8_t reserved[4];
    } llvm_batch_buffer_desc;
    
    const llvm_batch_buffer_desc* batch_buffers = (const llvm_batch_buffer_desc*)buffers;
    
    // Check each buffer individually
    for (uint32_t i = 0; i < count; i++) {
        if (!batch_buffers[i].data || batch_buffers[i].size == 0) {
            results[i] = 0;
            continue;
        }
        
        // Calculate sum of bytes mod 96
        uint32_t sum = 0;
        const uint8_t* bytes = (const uint8_t*)batch_buffers[i].data;
        for (uint64_t j = 0; j < batch_buffers[i].size; j++) {
            sum += bytes[j];
        }
        
        // Conservation check: sum % 96 == 0
        results[i] = (sum % 96) == 0 ? 1 : 0;
        
        // Debug output
        if (getenv("DEBUG_BATCH")) {
            printf("   Stub: buffer[%u] ptr=%p size=%lu sum=%u mod96=%u result=%u\n", 
                   i, batch_buffers[i].data, batch_buffers[i].size, sum, sum % 96, results[i]);
        }
    }
}

void* atlas_batch_delta_compute_llvm(void* deltas, uint32_t count) {
    (void)deltas;
    (void)count;
    // Return non-NULL to indicate success
    return (void*)0x1;
}

void* atlas_batch_witness_generate_llvm(void* witnesses, uint32_t count) {
    (void)witnesses;
    (void)count;
    // Return non-NULL to indicate success
    return (void*)0x1;
}

void atlas_batch_get_statistics_llvm(uint64_t* stats) {
    if (stats) {
        stats[0] = 0; // conserved_calls
        stats[1] = 0; // delta_calls
        stats[2] = 0; // witness_calls
        stats[3] = 0; // total_buffers
    }
}

void atlas_batch_reset_statistics_llvm(void) {
    // No-op for testing
}