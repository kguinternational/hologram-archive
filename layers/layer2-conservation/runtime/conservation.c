/* conservation.c - Atlas-12288 Layer 2 Conservation Runtime Implementation
 * (c) 2024-2025 UOR Foundation - MIT License
 * 
 * Layer 2 runtime implementing domain lifecycle, budget management,
 * witness operations, and conservation verification with atomic state transitions.
 */

#include <stdint.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <errno.h>

// Include the public API header
#include "../include/atlas-conservation.h"

// WASM compatibility macros
#ifdef ATLAS_SINGLE_THREAD
    #define ATLAS_ATOMIC_LOAD(ptr) (*(ptr))
    #define ATLAS_ATOMIC_STORE(ptr, val) (*(ptr) = (val))
    #define ATLAS_ATOMIC_CAS(ptr, expected, desired) \
        ((*(ptr) == (expected)) ? (*(ptr) = (desired), true) : false)
    #define ATLAS_ATOMIC_FETCH_ADD(ptr, val) \
        ({ __typeof__(*(ptr)) _old = *(ptr); *(ptr) += (val); _old; })
    #define ATLAS_ATOMIC_FETCH_SUB(ptr, val) \
        ({ __typeof__(*(ptr)) _old = *(ptr); *(ptr) -= (val); _old; })
#else
    #include <stdatomic.h>
    #define ATLAS_ATOMIC_LOAD(ptr) atomic_load(ptr)
    #define ATLAS_ATOMIC_STORE(ptr, val) atomic_store(ptr, val)
    #define ATLAS_ATOMIC_CAS(ptr, expected, desired) \
        atomic_compare_exchange_strong(ptr, &(expected), desired)
    #define ATLAS_ATOMIC_FETCH_ADD(ptr, val) atomic_fetch_add(ptr, val)
    #define ATLAS_ATOMIC_FETCH_SUB(ptr, val) atomic_fetch_sub(ptr, val)
#endif

// =============================================================================
// Error Handling  
// =============================================================================

// Thread-local error state
#ifdef ATLAS_SINGLE_THREAD
    static atlas_error_t last_error = ATLAS_OK;
#else
    static _Thread_local atlas_error_t last_error = ATLAS_OK;
#endif

static void atlas_set_error(atlas_error_t error) {
    last_error = error;
}

atlas_error_t atlas_get_last_error(void) {
    return last_error;
}

// =============================================================================
// Internal Structures
// =============================================================================

// Domain states as specified
typedef enum {
    DOM_OPEN = 0,          // Open state (default)
    DOM_COMMITTED = 1      // Committed state (atomic transition)
} atlas_domain_state_t;

// SHA-256 hash size for witnesses
#define SHA256_HASH_SIZE 32

struct atlas_domain_internal {
    // Header for validation
    uint32_t magic;                    // 0xA71A5D0C (ATLASDoC)
    
    // State management (atomic for thread safety)
#ifdef ATLAS_SINGLE_THREAD
    atlas_domain_state_t state;
    uint32_t budget;                   // Current budget (lower 7 bits for mod-96)
#else
    _Atomic(atlas_domain_state_t) state;
    _Atomic(uint32_t) budget;         // Current budget (lower 7 bits for mod-96)
#endif
    
    // Memory management
    void* base_ptr;                   // Attached memory base
    size_t allocated_bytes;           // Initial allocation size
    size_t attached_length;           // Length of attached memory
    uint8_t initial_budget_class;     // Budget class at creation
    
    // Conservation tracking
    uint32_t conservation_sum;        // Current conservation sum
    
    // Witness binding
    void* witness_handle;            // Opaque witness pointer
    
    // Domain isolation
    uint64_t domain_id;              // Unique domain identifier
    uint64_t isolation_proof;        // Proof of isolation
};

struct atlas_witness_internal {
    uint32_t magic;                   // 0xA71A5117 (ATLASWit)
    void* llvm_witness_ptr;          // LLVM witness handle
    size_t data_length;              // Length of witnessed data
    uint8_t sha256_hash[SHA256_HASH_SIZE]; // SHA-256 hash of data
    uint8_t resonance_class;         // Resonance classification
};

// Static domain ID counter for uniqueness
#ifdef ATLAS_SINGLE_THREAD
    static uint64_t next_domain_id = 1;
#else
    static _Atomic(uint64_t) next_domain_id = 1;
#endif

// Validation magic numbers
#define ATLAS_DOMAIN_MAGIC 0xA71A5D0C
#define ATLAS_WITNESS_MAGIC 0xA71A5117

// =============================================================================
// LLVM IR Function Declarations
// =============================================================================

// From conservation LLVM modules
extern void* atlas_domain_create_llvm(uint8_t initial_budget);
extern void atlas_domain_destroy_llvm(void* domain);
extern bool atlas_domain_validate_llvm(void* domain);
extern void atlas_domain_reserve_budget_llvm(void* domain, uint8_t amount);
extern void atlas_domain_release_budget_llvm(void* domain, uint8_t amount);
extern uint8_t atlas_domain_available_budget_llvm(void* domain);
extern bool atlas_domain_can_afford_llvm(void* domain, uint8_t cost);

extern void* atlas_alloc_aligned(size_t size);
extern bool atlas_conserved_check(const void* data, size_t length);
extern uint32_t atlas_conserved_sum(const void* data, size_t length);
extern uint8_t atlas_conserved_delta(const void* before, const void* after, size_t length);

extern void* atlas_witness_generate_llvm(const void* data, size_t length);
extern bool atlas_witness_verify_llvm(void* witness, const void* data, size_t length);
extern void atlas_witness_destroy_llvm(void* witness);

// Batch processing functions
extern uint8_t* atlas_batch_conserved_check_llvm(const void* buffers, uint32_t count);
extern void* atlas_batch_delta_compute_llvm(void* deltas, uint32_t count);
extern void* atlas_batch_witness_generate_llvm(void* witnesses, uint32_t count);
extern void atlas_batch_get_statistics_llvm(uint64_t* stats);
extern void atlas_batch_reset_statistics_llvm(void);

// =============================================================================
// Domain Lifecycle Functions
// =============================================================================

atlas_domain_t* atlas_domain_create(size_t bytes, uint8_t budget_class) {
    // Validate inputs
    if (bytes == 0 || budget_class > 95) {
        atlas_set_error(ATLAS_E_INVALID);
        return NULL;
    }
    
    // Allocate domain structure
    struct atlas_domain_internal* domain = malloc(sizeof(struct atlas_domain_internal));
    if (!domain) {
        atlas_set_error(ATLAS_E_MEMORY);
        return NULL;
    }
    
    // Initialize domain structure
    memset(domain, 0, sizeof(struct atlas_domain_internal));
    domain->magic = ATLAS_DOMAIN_MAGIC;
    
    // Generate unique domain ID
#ifdef ATLAS_SINGLE_THREAD
    domain->domain_id = next_domain_id++;
#else
    domain->domain_id = ATLAS_ATOMIC_FETCH_ADD(&next_domain_id, 1);
#endif
    
    // Set initial state and budget (DOM_OPEN = 0)
    ATLAS_ATOMIC_STORE(&domain->state, DOM_OPEN);
    ATLAS_ATOMIC_STORE(&domain->budget, (uint32_t)budget_class);
    
    // Store allocation parameters
    domain->allocated_bytes = bytes;
    domain->initial_budget_class = budget_class;
    
    // Generate isolation proof (simplified: hash of domain ID and timestamp)
    domain->isolation_proof = domain->domain_id * 0x9E3779B9U; // Golden ratio hash
    
    atlas_set_error(ATLAS_OK);
    return (atlas_domain_t*)domain;
}

int atlas_domain_attach(atlas_domain_t* dom, void* base, size_t len) {
    struct atlas_domain_internal* domain = (struct atlas_domain_internal*)dom;
    
    // Validate domain
    if (!domain || domain->magic != ATLAS_DOMAIN_MAGIC) {
        atlas_set_error(ATLAS_E_INVALID);
        return -1;
    }
    
    // Validate memory parameters
    if (!base || len == 0) {
        atlas_set_error(ATLAS_E_INVALID);
        return -1;
    }
    
    // Check state (must be open)
    atlas_domain_state_t current_state = ATLAS_ATOMIC_LOAD(&domain->state);
    if (current_state != DOM_OPEN) {
        atlas_set_error(ATLAS_E_STATE);
        return -1;
    }
    
    // Check if memory is already attached
    if (domain->base_ptr != NULL || domain->attached_length != 0) {
        atlas_set_error(ATLAS_E_STATE);
        return -1;
    }
    
    // Attach memory
    domain->base_ptr = base;
    domain->attached_length = len;
    
    // Compute initial conservation sum
    domain->conservation_sum = atlas_conserved_sum(base, len);
    
    atlas_set_error(ATLAS_OK);
    return 0;
}

bool atlas_domain_verify(const atlas_domain_t* dom) {
    const struct atlas_domain_internal* domain = (const struct atlas_domain_internal*)dom;
    
    // Validate domain structure
    if (!domain || domain->magic != ATLAS_DOMAIN_MAGIC) {
        atlas_set_error(ATLAS_E_INVALID);
        return false;
    }
    
    // Verify memory is attached
    if (!domain->base_ptr || domain->attached_length == 0) {
        atlas_set_error(ATLAS_E_STATE);
        return false;
    }
    
    // Check conservation
    bool conserved = atlas_conserved_check(domain->base_ptr, domain->attached_length);
    if (!conserved) {
        atlas_set_error(ATLAS_E_CONSERVATION);
        return false;
    }
    
    // Verify conservation sum hasn't changed
    uint32_t current_sum = atlas_conserved_sum(domain->base_ptr, domain->attached_length);
    if (current_sum != domain->conservation_sum) {
        atlas_set_error(ATLAS_E_CONSERVATION);
        return false;
    }
    
    // Verify witness if present
    if (domain->witness_handle) {
        bool witness_valid = atlas_witness_verify_llvm(
            domain->witness_handle, 
            domain->base_ptr, 
            domain->attached_length
        );
        if (!witness_valid) {
            atlas_set_error(ATLAS_E_WITNESS);
            return false;
        }
    }
    
    atlas_set_error(ATLAS_OK);
    return true;
}

int atlas_domain_commit(atlas_domain_t* dom) {
    struct atlas_domain_internal* domain = (struct atlas_domain_internal*)dom;
    
    // Validate domain
    if (!domain || domain->magic != ATLAS_DOMAIN_MAGIC) {
        atlas_set_error(ATLAS_E_INVALID);
        return -1;
    }
    
    // Verify before committing
    if (!atlas_domain_verify((const atlas_domain_t*)domain)) {
        // Error already set by verify
        return -1;
    }
    
    // Atomic transition from OPEN to COMMITTED
    atlas_domain_state_t expected = DOM_OPEN;
    if (!ATLAS_ATOMIC_CAS(&domain->state, expected, DOM_COMMITTED)) {
        atlas_set_error(ATLAS_E_STATE);
        return -1;
    }
    
    // Generate witness if not present
    if (!domain->witness_handle) {
        domain->witness_handle = atlas_witness_generate_llvm(
            domain->base_ptr,
            domain->attached_length
        );
        if (!domain->witness_handle) {
            // Rollback state change
            ATLAS_ATOMIC_STORE(&domain->state, DOM_OPEN);
            atlas_set_error(ATLAS_E_MEMORY);
            return -1;
        }
    }
    
    atlas_set_error(ATLAS_OK);
    return 0;
}

void atlas_domain_destroy(atlas_domain_t* dom) {
    struct atlas_domain_internal* domain = (struct atlas_domain_internal*)dom;
    
    if (!domain || domain->magic != ATLAS_DOMAIN_MAGIC) {
        atlas_set_error(ATLAS_E_INVALID);
        return;
    }
    
    // Clean up witness
    if (domain->witness_handle) {
        atlas_witness_destroy_llvm(domain->witness_handle);
        domain->witness_handle = NULL;
    }
    
    // Clear magic to prevent reuse
    domain->magic = 0;
    
    // Free domain structure
    free(domain);
    
    atlas_set_error(ATLAS_OK);
}

// =============================================================================
// Budget Management Functions (mod-96 with atomics)
// =============================================================================

bool atlas_budget_alloc(atlas_domain_t* dom, uint8_t amt) {
    struct atlas_domain_internal* domain = (struct atlas_domain_internal*)dom;
    
    // Validate domain
    if (!domain || domain->magic != ATLAS_DOMAIN_MAGIC) {
        atlas_set_error(ATLAS_E_INVALID);
        return false;
    }
    
    // Validate amount (must be within mod-96 range)
    if (amt > 95) {
        atlas_set_error(ATLAS_E_INVALID);
        return false;
    }
    
    // Atomic budget deduction with mod-96 arithmetic
    uint32_t current_budget, new_budget;
    do {
        current_budget = ATLAS_ATOMIC_LOAD(&domain->budget);
        uint8_t current_mod96 = (uint8_t)(current_budget & 0x7F); // Lower 7 bits for mod-96
        
        // Check if we have sufficient budget
        if (current_mod96 < amt) {
            atlas_set_error(ATLAS_E_BUDGET);
            return false;
        }
        
        // Calculate new budget with mod-96 arithmetic
        uint8_t new_mod96 = (current_mod96 - amt) % 96;
        new_budget = (current_budget & ~0x7F) | new_mod96;
        
    } while (!ATLAS_ATOMIC_CAS(&domain->budget, current_budget, new_budget));
    
    atlas_set_error(ATLAS_OK);
    return true;
}

bool atlas_budget_release(atlas_domain_t* dom, uint8_t amt) {
    struct atlas_domain_internal* domain = (struct atlas_domain_internal*)dom;
    
    // Validate domain
    if (!domain || domain->magic != ATLAS_DOMAIN_MAGIC) {
        atlas_set_error(ATLAS_E_INVALID);
        return false;
    }
    
    // Validate amount (must be within mod-96 range)
    if (amt > 95) {
        atlas_set_error(ATLAS_E_INVALID);
        return false;
    }
    
    // Atomic budget addition with mod-96 arithmetic
    uint32_t current_budget, new_budget;
    do {
        current_budget = ATLAS_ATOMIC_LOAD(&domain->budget);
        uint8_t current_mod96 = (uint8_t)(current_budget & 0x7F); // Lower 7 bits for mod-96
        
        // Calculate new budget with mod-96 arithmetic
        uint8_t new_mod96 = (current_mod96 + amt) % 96;
        new_budget = (current_budget & ~0x7F) | new_mod96;
        
    } while (!ATLAS_ATOMIC_CAS(&domain->budget, current_budget, new_budget));
    
    atlas_set_error(ATLAS_OK);
    return true;
}

// =============================================================================
// Witness Operations
// =============================================================================

// Simple SHA-256 implementation for witness hashing
static void sha256_simple(const uint8_t* data, size_t len, uint8_t hash[SHA256_HASH_SIZE]) {
    // Simplified SHA-256 - for production use proper crypto library
    // This is a placeholder that produces deterministic hashes
    uint32_t state[8] = {
        0x6a09e667, 0xbb67ae85, 0x3c6ef372, 0xa54ff53a,
        0x510e527f, 0x9b05688c, 0x1f83d9ab, 0x5be0cd19
    };
    
    for (size_t i = 0; i < len; i++) {
        uint32_t byte_val = data[i];
        for (int j = 0; j < 8; j++) {
            state[j] ^= byte_val;
            state[j] = ((state[j] << 7) | (state[j] >> 25)) ^ (i + 1);
            byte_val = (byte_val << 1) ^ (byte_val >> 7);
        }
    }
    
    // Pack state into hash
    for (int i = 0; i < 8; i++) {
        hash[i*4 + 0] = (uint8_t)(state[i] >> 24);
        hash[i*4 + 1] = (uint8_t)(state[i] >> 16);
        hash[i*4 + 2] = (uint8_t)(state[i] >> 8);
        hash[i*4 + 3] = (uint8_t)(state[i]);
    }
}

atlas_witness_t* atlas_witness_generate(const void* base, size_t len) {
    // Validate inputs
    if (!base || len == 0) {
        atlas_set_error(ATLAS_E_INVALID);
        return NULL;
    }
    
    // Allocate witness structure
    struct atlas_witness_internal* witness = malloc(sizeof(struct atlas_witness_internal));
    if (!witness) {
        atlas_set_error(ATLAS_E_MEMORY);
        return NULL;
    }
    
    // Initialize witness structure
    memset(witness, 0, sizeof(struct atlas_witness_internal));
    witness->magic = ATLAS_WITNESS_MAGIC;
    witness->data_length = len;
    
    // Generate SHA-256 hash
    sha256_simple((const uint8_t*)base, len, witness->sha256_hash);
    
    // Generate LLVM witness
    witness->llvm_witness_ptr = atlas_witness_generate_llvm(base, len);
    if (!witness->llvm_witness_ptr) {
        free(witness);
        atlas_set_error(ATLAS_E_MEMORY);
        return NULL;
    }
    
    // Calculate resonance class (simplified: sum mod 96)
    uint32_t sum = atlas_conserved_sum(base, len);
    witness->resonance_class = (uint8_t)(sum % 96);
    
    atlas_set_error(ATLAS_OK);
    return (atlas_witness_t*)witness;
}

bool atlas_witness_verify(const atlas_witness_t* w, const void* base, size_t len) {
    const struct atlas_witness_internal* witness = (const struct atlas_witness_internal*)w;
    
    // Validate witness structure
    if (!witness || witness->magic != ATLAS_WITNESS_MAGIC) {
        atlas_set_error(ATLAS_E_INVALID);
        return false;
    }
    
    // Validate inputs
    if (!base || len == 0) {
        atlas_set_error(ATLAS_E_INVALID);
        return false;
    }
    
    // Check length consistency
    if (witness->data_length != len) {
        atlas_set_error(ATLAS_E_INVALID);
        return false;
    }
    
    // Verify SHA-256 hash
    uint8_t current_hash[SHA256_HASH_SIZE];
    sha256_simple((const uint8_t*)base, len, current_hash);
    if (memcmp(witness->sha256_hash, current_hash, SHA256_HASH_SIZE) != 0) {
        atlas_set_error(ATLAS_E_WITNESS);
        return false;
    }
    
    // Verify using LLVM witness
    bool valid = atlas_witness_verify_llvm(witness->llvm_witness_ptr, base, len);
    if (!valid) {
        atlas_set_error(ATLAS_E_WITNESS);
        return false;
    }
    
    // Verify resonance class
    uint32_t current_sum = atlas_conserved_sum(base, len);
    uint8_t current_resonance = (uint8_t)(current_sum % 96);
    if (current_resonance != witness->resonance_class) {
        atlas_set_error(ATLAS_E_CONSERVATION);
        return false;
    }
    
    atlas_set_error(ATLAS_OK);
    return true;
}

void atlas_witness_destroy(atlas_witness_t* w) {
    struct atlas_witness_internal* witness = (struct atlas_witness_internal*)w;
    
    if (!witness || witness->magic != ATLAS_WITNESS_MAGIC) {
        atlas_set_error(ATLAS_E_INVALID);
        return;
    }
    
    // Destroy LLVM witness
    if (witness->llvm_witness_ptr) {
        atlas_witness_destroy_llvm(witness->llvm_witness_ptr);
        witness->llvm_witness_ptr = NULL;
    }
    
    // Clear magic to prevent reuse
    witness->magic = 0;
    
    // Free witness structure
    free(witness);
    
    atlas_set_error(ATLAS_OK);
}

// =============================================================================
// Conservation Helper (delegated to LLVM)
// =============================================================================

// atlas_conserved_delta is provided by LLVM layer exports

// =============================================================================
// Runtime Information Functions
// =============================================================================

bool atlas_runtime_is_thread_safe(void) {
#ifdef ATLAS_SINGLE_THREAD
    return false;
#else
    return true;
#endif
}

const char* atlas_error_string(atlas_error_t error) {
    switch (error) {
        case ATLAS_OK:
            return "Operation completed successfully";
        case ATLAS_E_INVALID:
            return "Invalid function argument";
        case ATLAS_E_MEMORY:
            return "Memory allocation failed";
        case ATLAS_E_STATE:
            return "Invalid domain state transition";
        case ATLAS_E_BUDGET:
            return "Insufficient budget for operation";
        case ATLAS_E_CONSERVATION:
            return "Conservation law violated";
        case ATLAS_E_WITNESS:
            return "Witness verification failed";
        case ATLAS_E_DESTROYED:
            return "Domain has been destroyed";
        default:
            return "Unknown error";
    }
}

// =============================================================================
// Batch Processing Implementation (ABI bridge to LLVM)
// =============================================================================

// LLVM batch operation structures (mirror C structures for ABI compatibility)
typedef struct {
    void* data;
    uint64_t size;
    uint8_t status;
    uint8_t reserved[7];
} llvm_batch_buffer_desc;

typedef struct {
    void* before;
    void* after;
    uint64_t size;
    uint8_t delta_result;
    uint8_t reserved[7];
} llvm_batch_delta_desc;

typedef struct {
    void* data;
    uint64_t size;
    void* witness;
    uint32_t status;
    uint8_t reserved[4];
} llvm_batch_witness_desc;

// Helper functions for ABI conversion
static void convert_to_llvm_batch_buffers(const atlas_batch_buffer_t* c_buffers, 
                                         llvm_batch_buffer_desc* llvm_buffers, 
                                         size_t count) {
    for (size_t i = 0; i < count; i++) {
        llvm_buffers[i].data = c_buffers[i].data;
        llvm_buffers[i].size = (uint64_t)c_buffers[i].size;
        llvm_buffers[i].status = c_buffers[i].status;
        memset(llvm_buffers[i].reserved, 0, sizeof(llvm_buffers[i].reserved));
    }
}

static void convert_to_llvm_batch_deltas(const atlas_batch_delta_t* c_deltas,
                                        llvm_batch_delta_desc* llvm_deltas,
                                        size_t count) {
    for (size_t i = 0; i < count; i++) {
        llvm_deltas[i].before = (void*)c_deltas[i].before;
        llvm_deltas[i].after = (void*)c_deltas[i].after;
        llvm_deltas[i].size = (uint64_t)c_deltas[i].size;
        llvm_deltas[i].delta_result = c_deltas[i].delta;
        memset(llvm_deltas[i].reserved, 0, sizeof(llvm_deltas[i].reserved));
    }
}

static void convert_from_llvm_batch_deltas(const llvm_batch_delta_desc* llvm_deltas,
                                          atlas_batch_delta_t* c_deltas,
                                          size_t count) {
    for (size_t i = 0; i < count; i++) {
        c_deltas[i].delta = llvm_deltas[i].delta_result;
    }
}

static void convert_to_llvm_batch_witnesses(const atlas_batch_witness_t* c_witnesses,
                                           llvm_batch_witness_desc* llvm_witnesses,
                                           size_t count) {
    for (size_t i = 0; i < count; i++) {
        llvm_witnesses[i].data = (void*)c_witnesses[i].data;
        llvm_witnesses[i].size = (uint64_t)c_witnesses[i].size;
        llvm_witnesses[i].witness = c_witnesses[i].witness;
        llvm_witnesses[i].status = c_witnesses[i].status;
        memset(llvm_witnesses[i].reserved, 0, sizeof(llvm_witnesses[i].reserved));
    }
}

static void convert_from_llvm_batch_witnesses(const llvm_batch_witness_desc* llvm_witnesses,
                                             atlas_batch_witness_t* c_witnesses,
                                             size_t count) {
    for (size_t i = 0; i < count; i++) {
        c_witnesses[i].witness = (atlas_witness_t*)llvm_witnesses[i].witness;
        c_witnesses[i].status = llvm_witnesses[i].status;
    }
}

uint8_t* atlas_batch_conserved_check(const atlas_batch_buffer_t* buffers, size_t count) {
    // Validate inputs
    if (!buffers || count == 0 || count > 256) {
        atlas_set_error(ATLAS_E_INVALID);
        return NULL;
    }
    
    // Validate individual buffer descriptors
    for (size_t i = 0; i < count; i++) {
        if (!buffers[i].data || buffers[i].size == 0) {
            atlas_set_error(ATLAS_E_INVALID);
            return NULL;
        }
    }
    
    // Allocate LLVM-compatible buffer descriptors
    llvm_batch_buffer_desc* llvm_buffers = malloc(count * sizeof(llvm_batch_buffer_desc));
    if (!llvm_buffers) {
        atlas_set_error(ATLAS_E_MEMORY);
        return NULL;
    }
    
    // Convert to LLVM format
    convert_to_llvm_batch_buffers(buffers, llvm_buffers, count);
    
    // Call LLVM batch function
    uint8_t* results = atlas_batch_conserved_check_llvm(llvm_buffers, (uint32_t)count);
    
    // Clean up temporary buffer
    free(llvm_buffers);
    
    if (!results) {
        atlas_set_error(ATLAS_E_MEMORY);
        return NULL;
    }
    
    atlas_set_error(ATLAS_OK);
    return results;
}

int atlas_batch_delta_compute(atlas_batch_delta_t* deltas, size_t count) {
    // Validate inputs
    if (!deltas || count == 0 || count > 256) {
        atlas_set_error(ATLAS_E_INVALID);
        return -1;
    }
    
    // Validate individual delta descriptors
    for (size_t i = 0; i < count; i++) {
        if (!deltas[i].before || !deltas[i].after || deltas[i].size == 0) {
            atlas_set_error(ATLAS_E_INVALID);
            return -1;
        }
    }
    
    // Allocate LLVM-compatible delta descriptors
    llvm_batch_delta_desc* llvm_deltas = malloc(count * sizeof(llvm_batch_delta_desc));
    if (!llvm_deltas) {
        atlas_set_error(ATLAS_E_MEMORY);
        return -1;
    }
    
    // Convert to LLVM format
    convert_to_llvm_batch_deltas(deltas, llvm_deltas, count);
    
    // Call LLVM batch function
    void* result = atlas_batch_delta_compute_llvm(llvm_deltas, (uint32_t)count);
    
    if (result) {
        // Convert results back to C format
        convert_from_llvm_batch_deltas(llvm_deltas, deltas, count);
        atlas_set_error(ATLAS_OK);
    } else {
        atlas_set_error(ATLAS_E_MEMORY);
    }
    
    // Clean up temporary buffer
    free(llvm_deltas);
    
    return result ? 0 : -1;
}

int atlas_batch_witness_generate(atlas_batch_witness_t* witnesses, size_t count) {
    // Validate inputs
    if (!witnesses || count == 0 || count > 256) {
        atlas_set_error(ATLAS_E_INVALID);
        return -1;
    }
    
    // Validate individual witness descriptors
    for (size_t i = 0; i < count; i++) {
        if (!witnesses[i].data || witnesses[i].size == 0) {
            atlas_set_error(ATLAS_E_INVALID);
            return -1;
        }
        // Initialize output fields
        witnesses[i].witness = NULL;
        witnesses[i].status = 0;
    }
    
    // Allocate LLVM-compatible witness descriptors
    llvm_batch_witness_desc* llvm_witnesses = malloc(count * sizeof(llvm_batch_witness_desc));
    if (!llvm_witnesses) {
        atlas_set_error(ATLAS_E_MEMORY);
        return -1;
    }
    
    // Convert to LLVM format
    convert_to_llvm_batch_witnesses(witnesses, llvm_witnesses, count);
    
    // Call LLVM batch function
    void* result = atlas_batch_witness_generate_llvm(llvm_witnesses, (uint32_t)count);
    
    if (result) {
        // Convert results back to C format
        convert_from_llvm_batch_witnesses(llvm_witnesses, witnesses, count);
        atlas_set_error(ATLAS_OK);
    } else {
        atlas_set_error(ATLAS_E_MEMORY);
    }
    
    // Clean up temporary buffer
    free(llvm_witnesses);
    
    return result ? 0 : -1;
}

bool atlas_batch_get_statistics(atlas_batch_stats_t* stats) {
    if (!stats) {
        atlas_set_error(ATLAS_E_INVALID);
        return false;
    }
    
    // Create temporary array for LLVM function
    uint64_t llvm_stats[4];
    
    // Call LLVM function
    atlas_batch_get_statistics_llvm(llvm_stats);
    
    // Convert results
    stats->conserved_calls = llvm_stats[0];
    stats->delta_calls = llvm_stats[1];
    stats->witness_calls = llvm_stats[2];
    stats->total_buffers = llvm_stats[3];
    
    atlas_set_error(ATLAS_OK);
    return true;
}

void atlas_batch_reset_statistics(void) {
    atlas_batch_reset_statistics_llvm();
    atlas_set_error(ATLAS_OK);
}

size_t atlas_batch_get_optimal_size(size_t buffer_size) {
    // Calculate optimal batch size based on buffer size and system characteristics
    // This is a heuristic based on cache size and SIMD capabilities
    
    const size_t L1_CACHE_SIZE = 32 * 1024;  // Assume 32KB L1 cache
    const size_t SIMD_THRESHOLD = 8;         // Minimum for SIMD benefits
    const size_t MAX_BATCH_SIZE = 256;       // Maximum supported batch size
    
    // For very small buffers, use larger batches to amortize function call overhead
    if (buffer_size <= 64) {
        return MAX_BATCH_SIZE;
    }
    
    // For medium buffers, calculate based on cache efficiency
    if (buffer_size <= 1024) {
        size_t optimal = L1_CACHE_SIZE / (buffer_size * 4);  // Leave room for prefetching
        optimal = (optimal < SIMD_THRESHOLD) ? SIMD_THRESHOLD : optimal;
        optimal = (optimal > MAX_BATCH_SIZE) ? MAX_BATCH_SIZE : optimal;
        return optimal;
    }
    
    // For large buffers, use smaller batches to avoid cache thrashing
    if (buffer_size <= 16 * 1024) {
        return 16;
    }
    
    // For very large buffers, minimal batching is optimal
    return SIMD_THRESHOLD;
}