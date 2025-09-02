/* layer2.c - Atlas-12288 Layer 2 Host Runtime with C ABI
 * (c) 2024-2025 UOR Foundation - MIT License
 * 
 * Layer 2 runtime implementing domain lifecycle, budget management,
 * witness operations, and error handling with thread-safe state transitions.
 */

#include <stdint.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <errno.h>

// Include the public API header
#include "atlas-runtime.h"

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
    static atlas_error_t last_error = ATLAS_SUCCESS;
#else
    static _Thread_local atlas_error_t last_error = ATLAS_SUCCESS;
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

typedef enum {
    ATLAS_DOMAIN_CREATED = 0,
    ATLAS_DOMAIN_ATTACHED = 1,
    ATLAS_DOMAIN_VERIFIED = 2,
    ATLAS_DOMAIN_COMMITTED = 3,
    ATLAS_DOMAIN_DESTROYED = 4
} atlas_domain_state_t;

typedef struct atlas_domain_internal {
    // Header for validation
    uint32_t magic;                    // 0xA71A5D0C (ATLASDoC)
    
    // State management (atomic for thread safety)
#ifdef ATLAS_SINGLE_THREAD
    atlas_domain_state_t state;
    uint8_t budget;                    // Current budget (0..95)
#else
    _Atomic(atlas_domain_state_t) state;
    _Atomic(uint8_t) budget;          // Current budget (0..95)
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
} atlas_domain_internal_t;

typedef struct atlas_witness_internal {
    uint32_t magic;                   // 0xA71A5W17 (ATLASWit)
    void* llvm_witness_ptr;          // LLVM witness handle
    size_t data_length;              // Length of witnessed data
    uint8_t resonance_class;         // Resonance classification
} atlas_witness_internal_t;

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

// From atlas-12288-domains.ll
extern void* atlas_domain_create_llvm(uint8_t initial_budget);
extern void atlas_domain_destroy_llvm(void* domain);
extern bool atlas_domain_validate_llvm(void* domain);
extern void atlas_domain_reserve_budget_llvm(void* domain, uint8_t amount);
extern void atlas_domain_release_budget_llvm(void* domain, uint8_t amount);
extern uint8_t atlas_domain_available_budget_llvm(void* domain);
extern bool atlas_domain_can_afford_llvm(void* domain, uint8_t cost);

// From atlas-12288-memory.ll  
extern void* atlas_alloc_aligned(size_t size);
extern bool atlas_conserved_check(const void* data, size_t length);
extern uint32_t atlas_conserved_sum(const void* data, size_t length);
extern uint8_t atlas_conserved_delta(const void* before, const void* after, size_t length);

// From atlas-12288-intrinsics.ll
extern void* atlas_witness_generate_llvm(const void* data, size_t length);
extern bool atlas_witness_verify_llvm(void* witness, const void* data, size_t length);
extern void atlas_witness_destroy_llvm(void* witness);

// =============================================================================
// Domain Lifecycle Functions
// =============================================================================

atlas_domain_t* atlas_domain_create(size_t bytes, uint8_t budget_class) {
    // Validate inputs
    if (bytes == 0 || budget_class > 95) {
        atlas_set_error(ATLAS_ERROR_INVALID_ARGUMENT);
        return NULL;
    }
    
    // Allocate domain structure
    atlas_domain_internal_t* domain = malloc(sizeof(atlas_domain_internal_t));
    if (!domain) {
        atlas_set_error(ATLAS_ERROR_OUT_OF_MEMORY);
        return NULL;
    }
    
    // Initialize domain structure
    memset(domain, 0, sizeof(atlas_domain_internal_t));
    domain->magic = ATLAS_DOMAIN_MAGIC;
    
    // Generate unique domain ID
#ifdef ATLAS_SINGLE_THREAD
    domain->domain_id = next_domain_id++;
#else
    domain->domain_id = ATLAS_ATOMIC_FETCH_ADD(&next_domain_id, 1);
#endif
    
    // Set initial state and budget
    ATLAS_ATOMIC_STORE(&domain->state, ATLAS_DOMAIN_CREATED);
    ATLAS_ATOMIC_STORE(&domain->budget, budget_class);
    
    // Store allocation parameters
    domain->allocated_bytes = bytes;
    domain->initial_budget_class = budget_class;
    
    // Generate isolation proof (simplified: hash of domain ID and timestamp)
    domain->isolation_proof = domain->domain_id * 0x9E3779B9U; // Golden ratio hash
    
    atlas_set_error(ATLAS_SUCCESS);
    return (atlas_domain_t*)domain;
}

int atlas_domain_attach(atlas_domain_t* domain_opaque, void* base, size_t len) {
    atlas_domain_internal_t* domain = (atlas_domain_internal_t*)domain_opaque;
    
    // Validate domain
    if (!domain || domain->magic != ATLAS_DOMAIN_MAGIC) {
        atlas_set_error(ATLAS_ERROR_INVALID_ARGUMENT);
        return -1;
    }
    
    // Check state transition
    atlas_domain_state_t current_state = ATLAS_ATOMIC_LOAD(&domain->state);
    if (current_state != ATLAS_DOMAIN_CREATED) {
        atlas_set_error(ATLAS_ERROR_INVALID_STATE);
        return -1;
    }
    
    // Validate memory parameters
    if (!base || len == 0) {
        atlas_set_error(ATLAS_ERROR_INVALID_ARGUMENT);
        return -1;
    }
    
    // Atomically transition to attached state
    atlas_domain_state_t expected = ATLAS_DOMAIN_CREATED;
    if (!ATLAS_ATOMIC_CAS(&domain->state, expected, ATLAS_DOMAIN_ATTACHED)) {
        atlas_set_error(ATLAS_ERROR_INVALID_STATE);
        return -1;
    }
    
    // Attach memory
    domain->base_ptr = base;
    domain->attached_length = len;
    
    // Compute initial conservation sum
    domain->conservation_sum = atlas_conserved_sum(base, len);
    
    atlas_set_error(ATLAS_SUCCESS);
    return 0;
}

bool atlas_domain_verify(const atlas_domain_t* domain_opaque) {
    const atlas_domain_internal_t* domain = (const atlas_domain_internal_t*)domain_opaque;
    
    // Validate domain structure
    if (!domain || domain->magic != ATLAS_DOMAIN_MAGIC) {
        atlas_set_error(ATLAS_ERROR_INVALID_ARGUMENT);
        return false;
    }
    
    // Check state
    atlas_domain_state_t current_state = ATLAS_ATOMIC_LOAD(&domain->state);
    if (current_state < ATLAS_DOMAIN_ATTACHED || current_state >= ATLAS_DOMAIN_DESTROYED) {
        atlas_set_error(ATLAS_ERROR_INVALID_STATE);
        return false;
    }
    
    // Verify memory is attached
    if (!domain->base_ptr || domain->attached_length == 0) {
        atlas_set_error(ATLAS_ERROR_INVALID_STATE);
        return false;
    }
    
    // Check conservation
    bool conserved = atlas_conserved_check(domain->base_ptr, domain->attached_length);
    if (!conserved) {
        atlas_set_error(ATLAS_ERROR_CONSERVATION_VIOLATION);
        return false;
    }
    
    // Verify conservation sum hasn't changed
    uint32_t current_sum = atlas_conserved_sum(domain->base_ptr, domain->attached_length);
    if (current_sum != domain->conservation_sum) {
        atlas_set_error(ATLAS_ERROR_CONSERVATION_VIOLATION);
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
            atlas_set_error(ATLAS_ERROR_WITNESS_INVALID);
            return false;
        }
    }
    
    atlas_set_error(ATLAS_SUCCESS);
    return true;
}

int atlas_domain_commit(atlas_domain_t* domain_opaque) {
    atlas_domain_internal_t* domain = (atlas_domain_internal_t*)domain_opaque;
    
    // Validate domain
    if (!domain || domain->magic != ATLAS_DOMAIN_MAGIC) {
        atlas_set_error(ATLAS_ERROR_INVALID_ARGUMENT);
        return -1;
    }
    
    // Verify before committing
    if (!atlas_domain_verify((const atlas_domain_t*)domain)) {
        // Error already set by verify
        return -1;
    }
    
    // Check state transition
    atlas_domain_state_t current_state = ATLAS_ATOMIC_LOAD(&domain->state);
    if (current_state != ATLAS_DOMAIN_ATTACHED && current_state != ATLAS_DOMAIN_VERIFIED) {
        atlas_set_error(ATLAS_ERROR_INVALID_STATE);
        return -1;
    }
    
    // Generate witness if not present
    if (!domain->witness_handle) {
        domain->witness_handle = atlas_witness_generate_llvm(
            domain->base_ptr,
            domain->attached_length
        );
        if (!domain->witness_handle) {
            atlas_set_error(ATLAS_ERROR_OUT_OF_MEMORY);
            return -1;
        }
    }
    
    // Atomically transition to committed state
    atlas_domain_state_t expected = current_state;
    if (!ATLAS_ATOMIC_CAS(&domain->state, expected, ATLAS_DOMAIN_COMMITTED)) {
        atlas_set_error(ATLAS_ERROR_INVALID_STATE);
        return -1;
    }
    
    atlas_set_error(ATLAS_SUCCESS);
    return 0;
}

void atlas_domain_destroy(atlas_domain_t* domain_opaque) {
    atlas_domain_internal_t* domain = (atlas_domain_internal_t*)domain_opaque;
    
    if (!domain || domain->magic != ATLAS_DOMAIN_MAGIC) {
        atlas_set_error(ATLAS_ERROR_INVALID_ARGUMENT);
        return;
    }
    
    // Mark as destroyed atomically
    ATLAS_ATOMIC_STORE(&domain->state, ATLAS_DOMAIN_DESTROYED);
    
    // Clean up witness
    if (domain->witness_handle) {
        atlas_witness_destroy_llvm(domain->witness_handle);
        domain->witness_handle = NULL;
    }
    
    // Clear magic to prevent reuse
    domain->magic = 0;
    
    // Free domain structure
    free(domain);
    
    atlas_set_error(ATLAS_SUCCESS);
}

// =============================================================================
// Budget Management Functions
// =============================================================================

bool atlas_budget_alloc(atlas_domain_t* domain_opaque, uint8_t amt) {
    atlas_domain_internal_t* domain = (atlas_domain_internal_t*)domain_opaque;
    
    // Validate domain
    if (!domain || domain->magic != ATLAS_DOMAIN_MAGIC) {
        atlas_set_error(ATLAS_ERROR_INVALID_ARGUMENT);
        return false;
    }
    
    // Check domain state
    atlas_domain_state_t current_state = ATLAS_ATOMIC_LOAD(&domain->state);
    if (current_state >= ATLAS_DOMAIN_DESTROYED) {
        atlas_set_error(ATLAS_ERROR_DOMAIN_DESTROYED);
        return false;
    }
    
    // Validate amount (must be within mod-96 range)
    if (amt > 95) {
        atlas_set_error(ATLAS_ERROR_INVALID_ARGUMENT);
        return false;
    }
    
    // Atomic budget deduction with mod-96 arithmetic
    uint8_t current_budget, new_budget;
    do {
        current_budget = ATLAS_ATOMIC_LOAD(&domain->budget);
        
        // Check if we have sufficient budget
        if (current_budget < amt) {
            atlas_set_error(ATLAS_ERROR_BUDGET_INSUFFICIENT);
            return false;
        }
        
        // Calculate new budget with mod-96 arithmetic
        new_budget = (current_budget - amt) % 96;
        
    } while (!ATLAS_ATOMIC_CAS(&domain->budget, current_budget, new_budget));
    
    atlas_set_error(ATLAS_SUCCESS);
    return true;
}

bool atlas_budget_release(atlas_domain_t* domain_opaque, uint8_t amt) {
    atlas_domain_internal_t* domain = (atlas_domain_internal_t*)domain_opaque;
    
    // Validate domain
    if (!domain || domain->magic != ATLAS_DOMAIN_MAGIC) {
        atlas_set_error(ATLAS_ERROR_INVALID_ARGUMENT);
        return false;
    }
    
    // Check domain state
    atlas_domain_state_t current_state = ATLAS_ATOMIC_LOAD(&domain->state);
    if (current_state >= ATLAS_DOMAIN_DESTROYED) {
        atlas_set_error(ATLAS_ERROR_DOMAIN_DESTROYED);
        return false;
    }
    
    // Validate amount (must be within mod-96 range)
    if (amt > 95) {
        atlas_set_error(ATLAS_ERROR_INVALID_ARGUMENT);
        return false;
    }
    
    // Atomic budget addition with mod-96 arithmetic
    uint8_t current_budget, new_budget;
    do {
        current_budget = ATLAS_ATOMIC_LOAD(&domain->budget);
        
        // Calculate new budget with mod-96 arithmetic
        new_budget = (current_budget + amt) % 96;
        
    } while (!ATLAS_ATOMIC_CAS(&domain->budget, current_budget, new_budget));
    
    atlas_set_error(ATLAS_SUCCESS);
    return true;
}

// =============================================================================
// Witness Operations
// =============================================================================

atlas_witness_t* atlas_witness_generate(const void* base, size_t len) {
    // Validate inputs
    if (!base || len == 0) {
        atlas_set_error(ATLAS_ERROR_INVALID_ARGUMENT);
        return NULL;
    }
    
    // Allocate witness structure
    atlas_witness_internal_t* witness = malloc(sizeof(atlas_witness_internal_t));
    if (!witness) {
        atlas_set_error(ATLAS_ERROR_OUT_OF_MEMORY);
        return NULL;
    }
    
    // Initialize witness structure
    memset(witness, 0, sizeof(atlas_witness_internal_t));
    witness->magic = ATLAS_WITNESS_MAGIC;
    witness->data_length = len;
    
    // Generate LLVM witness
    witness->llvm_witness_ptr = atlas_witness_generate_llvm(base, len);
    if (!witness->llvm_witness_ptr) {
        free(witness);
        atlas_set_error(ATLAS_ERROR_OUT_OF_MEMORY);
        return NULL;
    }
    
    // Calculate resonance class (simplified: sum mod 96)
    uint32_t sum = atlas_conserved_sum(base, len);
    witness->resonance_class = (uint8_t)(sum % 96);
    
    atlas_set_error(ATLAS_SUCCESS);
    return (atlas_witness_t*)witness;
}

bool atlas_witness_verify(const atlas_witness_t* witness_opaque, const void* base, size_t len) {
    const atlas_witness_internal_t* witness = (const atlas_witness_internal_t*)witness_opaque;
    
    // Validate witness structure
    if (!witness || witness->magic != ATLAS_WITNESS_MAGIC) {
        atlas_set_error(ATLAS_ERROR_INVALID_ARGUMENT);
        return false;
    }
    
    // Validate inputs
    if (!base || len == 0) {
        atlas_set_error(ATLAS_ERROR_INVALID_ARGUMENT);
        return false;
    }
    
    // Check length consistency
    if (witness->data_length != len) {
        atlas_set_error(ATLAS_ERROR_INVALID_ARGUMENT);
        return false;
    }
    
    // Verify using LLVM witness
    bool valid = atlas_witness_verify_llvm(witness->llvm_witness_ptr, base, len);
    if (!valid) {
        atlas_set_error(ATLAS_ERROR_WITNESS_INVALID);
        return false;
    }
    
    // Verify resonance class
    uint32_t current_sum = atlas_conserved_sum(base, len);
    uint8_t current_resonance = (uint8_t)(current_sum % 96);
    if (current_resonance != witness->resonance_class) {
        atlas_set_error(ATLAS_ERROR_CONSERVATION_VIOLATION);
        return false;
    }
    
    atlas_set_error(ATLAS_SUCCESS);
    return true;
}

void atlas_witness_destroy(atlas_witness_t* witness_opaque) {
    atlas_witness_internal_t* witness = (atlas_witness_internal_t*)witness_opaque;
    
    if (!witness || witness->magic != ATLAS_WITNESS_MAGIC) {
        atlas_set_error(ATLAS_ERROR_INVALID_ARGUMENT);
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
    
    atlas_set_error(ATLAS_SUCCESS);
}

// =============================================================================
// Conservation Functions
// =============================================================================

uint8_t atlas_conserved_delta(const void* before, const void* after, size_t len) {
    // Validate inputs
    if (!before || !after || len == 0) {
        atlas_set_error(ATLAS_ERROR_INVALID_ARGUMENT);
        return 0;
    }
    
    // Calculate conservation sums
    uint32_t sum_before = atlas_conserved_sum(before, len);
    uint32_t sum_after = atlas_conserved_sum(after, len);
    
    // Calculate delta with mod-96 arithmetic
    uint8_t delta;
    if (sum_after >= sum_before) {
        delta = (uint8_t)((sum_after - sum_before) % 96);
    } else {
        // Handle underflow with modular arithmetic
        delta = (uint8_t)((96 - ((sum_before - sum_after) % 96)) % 96);
    }
    
    atlas_set_error(ATLAS_SUCCESS);
    return delta;
}

// =============================================================================
// Additional Runtime Information Functions
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
        case ATLAS_SUCCESS:
            return "Operation completed successfully";
        case ATLAS_ERROR_INVALID_ARGUMENT:
            return "Invalid function argument";
        case ATLAS_ERROR_OUT_OF_MEMORY:
            return "Memory allocation failed";
        case ATLAS_ERROR_INVALID_STATE:
            return "Invalid domain state transition";
        case ATLAS_ERROR_BUDGET_INSUFFICIENT:
            return "Insufficient budget for operation";
        case ATLAS_ERROR_CONSERVATION_VIOLATION:
            return "Conservation law violated";
        case ATLAS_ERROR_WITNESS_INVALID:
            return "Witness verification failed";
        case ATLAS_ERROR_DOMAIN_DESTROYED:
            return "Domain has been destroyed";
        default:
            return "Unknown error";
    }
}