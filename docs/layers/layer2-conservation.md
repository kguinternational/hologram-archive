# Atlas-12288 Layer 2 Contract Documentation

**Version:** 1.0.0  
**API Version:** 1  
**Layer:** Conservation (Layer 2)  
**Generated:** 2025-09-02  

## Table of Contents

1. [Overview](#overview)
2. [API Contract Specification](#api-contract-specification)
3. [Conservation Laws and Invariants](#conservation-laws-and-invariants)
4. [Error Handling](#error-handling)
5. [Domain State Management](#domain-state-management)
6. [Budget Management](#budget-management)
7. [Witness Operations](#witness-operations)
8. [Thread Safety](#thread-safety)
9. [Memory Management](#memory-management)
10. [Performance Guarantees](#performance-guarantees)
11. [Integration Requirements](#integration-requirements)
12. [Usage Examples](#usage-examples)

---

## Overview

Layer 2 (Conservation) provides the fundamental computational domain management for the Atlas-12288 system. It enforces conservation laws, manages computational budgets through mod-96 arithmetic, and provides cryptographic witness generation for state verification.

### Core Responsibilities

- **Domain Lifecycle:** Create, attach, verify, commit, and destroy computational domains
- **Conservation Enforcement:** Maintain energy conservation through mod-96 arithmetic
- **Budget Management:** Atomic allocation and release of computational units (0-95 range)
- **Witness Generation:** Cryptographic proofs for domain state integrity
- **Thread Safety:** Atomic operations for concurrent access across domains

### Key Design Principles

- **Zero-Copy:** Pass raw pointers with lengths, no ownership transfer unless explicit
- **Deterministic:** Pure functions with consistent outputs for identical inputs
- **Atomic State:** Thread-safe state transitions using compare-and-swap operations
- **Failure-Closed:** All errors return specific codes, no undefined behavior

---

## API Contract Specification

### Core Types

```c
// Opaque handles - implementation details hidden
typedef struct atlas_domain_internal* atlas_domain_t;
typedef struct atlas_witness_internal* atlas_witness_t;

// Error codes returned by all operations
typedef enum {
    ATLAS_SUCCESS = 0,                    // Operation completed successfully
    ATLAS_ERROR_INVALID_ARGUMENT = 1,     // Invalid function argument
    ATLAS_ERROR_OUT_OF_MEMORY = 2,        // Memory allocation failed
    ATLAS_ERROR_INVALID_STATE = 3,        // Invalid domain state transition
    ATLAS_ERROR_BUDGET_INSUFFICIENT = 4,  // Insufficient budget for operation
    ATLAS_ERROR_CONSERVATION_VIOLATION = 5, // Conservation law violated
    ATLAS_ERROR_WITNESS_INVALID = 6,      // Witness verification failed
    ATLAS_ERROR_DOMAIN_DESTROYED = 7      // Domain has been destroyed
} atlas_error_t;
```

### Domain Lifecycle Functions

#### atlas_domain_create

```c
atlas_domain_t* atlas_domain_create(size_t bytes, uint8_t budget_class);
```

**Purpose:** Create a new Atlas computation domain with specified memory allocation and budget class.

**Parameters:**
- `bytes`: Size of memory to allocate (must be > 0, typically 12288)
- `budget_class`: Initial budget class (0-95, mod-96 arithmetic)

**Returns:** New domain handle, or NULL on error

**State Transition:** None → CREATED

**Error Conditions:**
- `ATLAS_ERROR_INVALID_ARGUMENT`: bytes == 0 or budget_class > 95
- `ATLAS_ERROR_OUT_OF_MEMORY`: Memory allocation failed

**Thread Safety:** Safe to call concurrently

**Contract:**
- Domain starts in CREATED state
- Budget initialized to `budget_class`
- Unique domain ID assigned atomically
- Must call `atlas_domain_destroy()` to prevent leaks

#### atlas_domain_attach

```c
int atlas_domain_attach(atlas_domain_t* domain, void* base, size_t len);
```

**Purpose:** Attach memory region to domain for computation.

**Parameters:**
- `domain`: Domain handle (must be in CREATED state)
- `base`: Pointer to memory region (must be non-NULL)
- `len`: Length of memory region in bytes (must be > 0)

**Returns:** 0 on success, -1 on error

**State Transition:** CREATED → ATTACHED

**Error Conditions:**
- `ATLAS_ERROR_INVALID_ARGUMENT`: domain is NULL/invalid, base is NULL, or len == 0
- `ATLAS_ERROR_INVALID_STATE`: domain not in CREATED state

**Thread Safety:** Safe for different domains, not thread-safe for same domain

**Contract:**
- Memory becomes computational space for domain
- Conservation sum calculated and stored
- Domain does not take ownership of attached memory
- Transition is atomic using compare-and-swap

#### atlas_domain_verify

```c
bool atlas_domain_verify(const atlas_domain_t* domain);
```

**Purpose:** Verify domain integrity and conservation laws.

**Parameters:**
- `domain`: Domain handle (must not be NULL or DESTROYED)

**Returns:** true if valid and conserved, false otherwise

**State Requirements:** ATTACHED, VERIFIED, or COMMITTED

**Error Conditions:**
- `ATLAS_ERROR_INVALID_ARGUMENT`: domain is NULL or invalid
- `ATLAS_ERROR_INVALID_STATE`: domain not in valid state
- `ATLAS_ERROR_CONSERVATION_VIOLATION`: conservation laws violated
- `ATLAS_ERROR_WITNESS_INVALID`: bound witness verification failed

**Thread Safety:** Safe to call concurrently (read-only)

**Contract:**
- Checks conservation sum against current memory state
- Verifies witness if present
- O(n) complexity where n is memory size
- Does not modify domain state

#### atlas_domain_commit

```c
int atlas_domain_commit(atlas_domain_t* domain);
```

**Purpose:** Commit domain changes and generate cryptographic witness.

**Parameters:**
- `domain`: Domain handle (must be ATTACHED or VERIFIED)

**Returns:** 0 on success, -1 on error

**State Transition:** ATTACHED/VERIFIED → COMMITTED

**Error Conditions:**
- `ATLAS_ERROR_INVALID_ARGUMENT`: domain is NULL or invalid
- `ATLAS_ERROR_INVALID_STATE`: domain not in attachable state or already committed
- `ATLAS_ERROR_CONSERVATION_VIOLATION`: verification failed before commit
- `ATLAS_ERROR_OUT_OF_MEMORY`: witness generation failed

**Thread Safety:** Not thread-safe (modifies domain state)

**Contract:**
- Verifies domain before committing
- Generates witness if not present
- Atomic state transition using compare-and-swap
- Idempotent on already committed domains (returns error code 5)

#### atlas_domain_destroy

```c
void atlas_domain_destroy(atlas_domain_t* domain);
```

**Purpose:** Destroy domain and free all associated resources.

**Parameters:**
- `domain`: Domain handle (can be NULL - no-op)

**Returns:** void

**State Transition:** Any → DESTROYED

**Thread Safety:** Not thread-safe (modifies and frees domain)

**Contract:**
- Destroys bound witness
- Frees domain structure
- Does not free attached memory
- Safe to call with NULL pointer
- Domain handle becomes invalid

### Budget Management Functions

#### atlas_budget_alloc

```c
bool atlas_budget_alloc(atlas_domain_t* domain, uint8_t amt);
```

**Purpose:** Atomically allocate budget units from domain's available budget.

**Parameters:**
- `domain`: Domain handle (must not be DESTROYED)
- `amt`: Amount to allocate (0-95, checked against available budget)

**Returns:** true on success, false if insufficient budget or error

**Error Conditions:**
- `ATLAS_ERROR_INVALID_ARGUMENT`: domain invalid or amt > 95
- `ATLAS_ERROR_DOMAIN_DESTROYED`: domain has been destroyed
- `ATLAS_ERROR_BUDGET_INSUFFICIENT`: not enough budget available

**Thread Safety:** Atomic operation, safe for concurrent access

**Contract:**
- Uses mod-96 arithmetic: `new_budget = (current_budget - amt) % 96`
- Atomic compare-and-swap loop for thread safety
- Fails if current_budget < amt
- Budget represents computational "energy" units

#### atlas_budget_release

```c
bool atlas_budget_release(atlas_domain_t* domain, uint8_t amt);
```

**Purpose:** Atomically release budget units back to domain's available budget.

**Parameters:**
- `domain`: Domain handle (must not be DESTROYED)
- `amt`: Amount to release (0-95, added to current budget mod 96)

**Returns:** true on success, false on error

**Error Conditions:**
- `ATLAS_ERROR_INVALID_ARGUMENT`: domain invalid or amt > 95
- `ATLAS_ERROR_DOMAIN_DESTROYED`: domain has been destroyed

**Thread Safety:** Atomic operation, safe for concurrent access

**Contract:**
- Uses mod-96 arithmetic: `new_budget = (current_budget + amt) % 96`
- Atomic compare-and-swap for thread safety
- Always succeeds if domain and amount are valid
- Used to return unused budget after operations

### Witness Operations

#### atlas_witness_generate

```c
atlas_witness_t* atlas_witness_generate(const void* base, size_t len);
```

**Purpose:** Generate cryptographic witness for memory region.

**Parameters:**
- `base`: Pointer to memory region (must be non-NULL)
- `len`: Length of memory region in bytes (must be > 0)

**Returns:** New witness handle, or NULL on error

**Error Conditions:**
- `ATLAS_ERROR_INVALID_ARGUMENT`: base is NULL or len == 0
- `ATLAS_ERROR_OUT_OF_MEMORY`: witness allocation failed

**Thread Safety:** Safe to call concurrently

**Contract:**
- Captures current state of memory region
- Calculates resonance class using mod-96 arithmetic
- Generates LLVM-based cryptographic proof
- O(n) complexity where n is memory size
- Must call `atlas_witness_destroy()` to prevent leaks

#### atlas_witness_verify

```c
bool atlas_witness_verify(const atlas_witness_t* witness, const void* base, size_t len);
```

**Purpose:** Verify that memory matches the cryptographic witness.

**Parameters:**
- `witness`: Witness handle (must be valid)
- `base`: Pointer to memory region (must be non-NULL)
- `len`: Length of memory region (must match witness length)

**Returns:** true if memory matches witness, false otherwise

**Error Conditions:**
- `ATLAS_ERROR_INVALID_ARGUMENT`: witness/base invalid or len mismatch
- `ATLAS_ERROR_WITNESS_INVALID`: witness verification failed
- `ATLAS_ERROR_CONSERVATION_VIOLATION`: resonance class mismatch

**Thread Safety:** Safe to call concurrently (read-only)

**Contract:**
- Verifies cryptographic proof using LLVM witness
- Checks resonance class consistency
- O(n) complexity where n is memory size
- Does not modify witness or memory

#### atlas_witness_destroy

```c
void atlas_witness_destroy(atlas_witness_t* witness);
```

**Purpose:** Destroy witness and free associated resources.

**Parameters:**
- `witness`: Witness handle (can be NULL - no-op)

**Returns:** void

**Thread Safety:** Not thread-safe (modifies and frees witness)

**Contract:**
- Frees LLVM witness resources
- Clears magic number to prevent reuse
- Safe to call with NULL pointer
- Witness handle becomes invalid

### Conservation Functions

#### atlas_conserved_delta

```c
uint8_t atlas_conserved_delta(const void* before, const void* after, size_t len);
```

**Purpose:** Calculate conservation delta between two memory states.

**Parameters:**
- `before`: Pointer to "before" memory state (must be non-NULL)
- `after`: Pointer to "after" memory state (must be non-NULL)
- `len`: Length of both memory regions (must be > 0)

**Returns:** Conservation delta (0-95), 0 indicates perfect conservation

**Thread Safety:** Safe to call concurrently (read-only)

**Contract:**
- Uses mod-96 arithmetic for all calculations
- Formula: `delta = (Σ(after) - Σ(before)) % 96`
- Handles underflow correctly in modular arithmetic
- O(n) complexity where n is memory size
- Delta of 0 means perfect conservation (no energy change)

---

## Conservation Laws and Invariants

### Primary Conservation Law

**Energy Conservation Principle:** The total computational "energy" in a system is conserved under valid operations.

```
∀ operation: (Σ_before + expected_delta) % 96 ≡ Σ_after % 96
```

Where:
- `Σ_before` = sum of all bytes before operation
- `Σ_after` = sum of all bytes after operation  
- `expected_delta` = anticipated energy change from operation
- All arithmetic performed mod 96

### Invariants

1. **Budget Invariant:** `∀ domain: 0 ≤ budget ≤ 95`
2. **State Transition Invariant:** State changes follow valid sequences only
3. **Witness Immutability:** Once generated, witnesses do not change for unchanging data
4. **Domain Isolation:** Each domain has unique ID and isolation proof
5. **Atomic Operations:** Budget and state changes are atomic

### Valid State Transitions

```
CREATED → ATTACHED → VERIFIED → COMMITTED → DESTROYED
    ↓         ↓         ↓         ↓
    ↘---------↘---------↘---------↘→ DESTROYED
```

**Invalid Transitions:**
- COMMITTED → ATTACHED (cannot reattach after commit)
- DESTROYED → any state (destroyed domains cannot transition)
- Any → CREATED (cannot return to created state)

---

## Error Handling

### Error Code Meanings

| Code | Name | Description | Recovery Strategy |
|------|------|-------------|------------------|
| 0 | `ATLAS_SUCCESS` | Operation completed successfully | Continue normal execution |
| 1 | `ATLAS_ERROR_INVALID_ARGUMENT` | Invalid function parameter | Check parameters, fix caller |
| 2 | `ATLAS_ERROR_OUT_OF_MEMORY` | Memory allocation failed | Reduce memory usage, retry |
| 3 | `ATLAS_ERROR_INVALID_STATE` | Invalid state transition | Check domain state, fix sequence |
| 4 | `ATLAS_ERROR_BUDGET_INSUFFICIENT` | Not enough budget | Release budget, reduce operation scope |
| 5 | `ATLAS_ERROR_CONSERVATION_VIOLATION` | Conservation law violated | Check memory integrity, debug data corruption |
| 6 | `ATLAS_ERROR_WITNESS_INVALID` | Witness verification failed | Regenerate witness, check memory |
| 7 | `ATLAS_ERROR_DOMAIN_DESTROYED` | Domain has been destroyed | Create new domain, fix lifecycle management |

### Error Propagation

- All functions set thread-local error state via `atlas_get_last_error()`
- Functions return success/failure indication (bool/int/pointer)
- Check error state immediately after failed operations
- Error state persists until next operation or explicit clear

### Error Handling Pattern

```c
atlas_domain_t* domain = atlas_domain_create(12288, 42);
if (!domain) {
    atlas_error_t error = atlas_get_last_error();
    switch (error) {
        case ATLAS_ERROR_INVALID_ARGUMENT:
            // Fix parameters and retry
            break;
        case ATLAS_ERROR_OUT_OF_MEMORY:
            // Reduce memory usage
            break;
        default:
            // Handle unexpected error
            break;
    }
}
```

---

## Domain State Management

### State Definitions

#### CREATED (0)
- **Description:** Domain allocated and initialized
- **Operations Allowed:** attach, destroy
- **Operations Forbidden:** verify, commit, budget operations
- **Memory:** No attached memory
- **Witnesses:** None

#### ATTACHED (1)
- **Description:** Memory region attached to domain
- **Operations Allowed:** verify, commit, budget operations, destroy
- **Operations Forbidden:** attach (already attached)
- **Memory:** Memory attached, conservation sum calculated
- **Witnesses:** None (generated on commit)

#### VERIFIED (2)
- **Description:** Domain verified and conservation confirmed
- **Operations Allowed:** commit, verify, budget operations, destroy
- **Operations Forbidden:** attach
- **Memory:** Memory verified as conserved
- **Witnesses:** May have witness from previous operations

#### COMMITTED (3)
- **Description:** Domain committed with witness generated
- **Operations Allowed:** verify, budget operations, destroy
- **Operations Forbidden:** attach, commit (idempotent failure)
- **Memory:** Memory state frozen with witness
- **Witnesses:** Cryptographic witness present

#### DESTROYED (4)
- **Description:** Domain destroyed, resources freed
- **Operations Allowed:** None (all operations fail)
- **Operations Forbidden:** All operations
- **Memory:** Domain structure invalid
- **Witnesses:** All witnesses destroyed

### State Transition Safety

- **Atomic Transitions:** All state changes use compare-and-swap operations
- **Race Protection:** Multiple threads cannot transition same domain simultaneously  
- **Failure Atomicity:** Failed transitions leave domain in original state
- **Idempotent Destroy:** Safe to destroy domain multiple times

---

## Budget Management

### Budget Arithmetic

All budget operations use **mod-96 arithmetic** with the range [0, 95].

#### Allocation Formula
```
new_budget = (current_budget - amount) % 96
success = (current_budget >= amount)
```

#### Release Formula  
```
new_budget = (current_budget + amount) % 96
success = true  // Always succeeds if domain/amount valid
```

### Budget Semantics

- **Budget Units:** Represent computational "energy" or permission units
- **Initial Budget:** Set at domain creation (0-95)
- **Allocation:** Consumes budget for operations (may fail if insufficient)
- **Release:** Returns unused budget after operations complete
- **Thread Safety:** All budget operations are atomic

### Budget Lifecycle

1. **Initialization:** Budget set to `budget_class` at domain creation
2. **Allocation:** Budget consumed before expensive operations
3. **Operation:** Work performed using allocated budget
4. **Release:** Unused budget returned to domain
5. **Conservation:** Total budget changes follow mod-96 arithmetic

### Budget Best Practices

```c
// Allocate budget before operation
if (!atlas_budget_alloc(domain, operation_cost)) {
    // Handle insufficient budget
    return false;
}

// Perform operation
bool success = perform_operation();

// Release unused budget
if (!success) {
    atlas_budget_release(domain, operation_cost);  // Full refund
} else {
    atlas_budget_release(domain, unused_portion);  // Partial refund
}
```

---

## Witness Operations

### Witness Generation

Witnesses provide cryptographic proof of memory state integrity using:

1. **LLVM IR Witness:** Low-level cryptographic proof generated by LLVM runtime
2. **Resonance Class:** Memory classification using mod-96 sum for quick validation
3. **Length Binding:** Witness tied to specific memory size for integrity

### Witness Verification Process

1. **Structure Validation:** Check witness handle and magic number
2. **Parameter Validation:** Verify memory pointer and length match witness
3. **LLVM Verification:** Cryptographic proof verification via LLVM runtime
4. **Resonance Check:** Current memory resonance class matches witness
5. **Conservation Check:** Memory state consistent with conservation laws

### Witness Immutability

- **Content Immutable:** Witnesses do not change after generation
- **Verification Stable:** Same data produces same verification result
- **Cross Verification:** Multiple witnesses for same data are equivalent
- **Tamper Detection:** Modified memory fails witness verification

### Witness Performance

- **Generation:** O(n) complexity, expensive cryptographic operation
- **Verification:** O(n) complexity, faster than generation
- **Memory:** Witness size proportional to memory size
- **Caching:** Witnesses can be cached for repeated verification

---

## Thread Safety

### Thread Safety Guarantees

#### Domain Operations
- **Different Domains:** Fully thread-safe, no synchronization required
- **Same Domain:** Budget operations are atomic, state changes are not thread-safe
- **Destruction:** Not thread-safe, must synchronize with other operations

#### Budget Operations
- **Allocation/Release:** Atomic operations using compare-and-swap loops
- **Concurrent Access:** Multiple threads can safely allocate/release on same domain
- **Consistency:** Budget invariants maintained under concurrent access

#### Witness Operations
- **Generation:** Thread-safe, can generate multiple witnesses concurrently
- **Verification:** Thread-safe, read-only operation
- **Destruction:** Not thread-safe, synchronize with verification

### Single-Threaded Builds

When compiled with `ATLAS_SINGLE_THREAD`:
- Atomic operations replaced with simple assignments
- Thread-local storage becomes static storage
- Performance improved, thread safety removed
- Use `atlas_runtime_is_thread_safe()` to detect build configuration

### Synchronization Patterns

#### Per-Domain Mutex
```c
// One mutex per domain for state changes
pthread_mutex_t domain_mutex;

pthread_mutex_lock(&domain_mutex);
atlas_domain_attach(domain, memory, size);
atlas_domain_commit(domain);
pthread_mutex_unlock(&domain_mutex);

// Budget operations don't need mutex (atomic)
atlas_budget_alloc(domain, 10);  // Thread-safe
```

#### Work Queue Pattern
```c
// Worker threads operate on different domains
void* worker_thread(void* arg) {
    atlas_domain_t* domain = create_worker_domain();
    
    while (work_available()) {
        process_work_item(domain);  // No synchronization needed
    }
    
    atlas_domain_destroy(domain);
    return NULL;
}
```

---

## Memory Management

### Ownership Model

#### Domain Memory
- **Creation:** Atlas allocates and owns domain structure
- **Destruction:** Atlas frees domain structure
- **Attached Memory:** Atlas does NOT own attached memory
- **Lifetime:** Domain structure lifetime managed by create/destroy calls

#### Witness Memory
- **Generation:** Atlas allocates and owns witness structure
- **Destruction:** Atlas frees witness structure  
- **LLVM Handles:** Atlas manages internal LLVM witness objects
- **Lifetime:** Witness lifetime managed by generate/destroy calls

#### Memory Safety Rules
1. **No Double-Free:** Safe to destroy NULL pointers
2. **No Use-After-Free:** Magic numbers prevent accidental reuse
3. **No Memory Leaks:** All create operations have matching destroy
4. **No Buffer Overruns:** All operations validate length parameters

### Memory Layout

#### Domain Structure (Internal)
```c
struct atlas_domain_internal {
    uint32_t magic;                    // Validation: 0xA71A5D0C
    _Atomic(atlas_domain_state_t) state;  // Current state
    _Atomic(uint8_t) budget;          // Available budget (0-95)
    void* base_ptr;                   // Attached memory (not owned)
    size_t allocated_bytes;           // Expected memory size
    size_t attached_length;           // Actual attached size
    uint32_t conservation_sum;        // Conservation tracking
    void* witness_handle;            // Bound witness (if any)
    uint64_t domain_id;              // Unique identifier
    uint64_t isolation_proof;        // Domain isolation proof
};
```

#### Witness Structure (Internal)
```c
struct atlas_witness_internal {
    uint32_t magic;                   // Validation: 0xA71A5117  
    void* llvm_witness_ptr;          // LLVM witness handle
    size_t data_length;              // Witnessed memory length
    uint8_t resonance_class;         // Memory resonance (sum % 96)
};
```

### Memory Alignment

- **No Special Alignment:** All operations work with byte-aligned data
- **Platform Natural:** Structures use platform natural alignment
- **SIMD Friendly:** LLVM operations may benefit from aligned data
- **Conservative:** No alignment assumptions in API contract

---

## Performance Guarantees

### Complexity Guarantees

| Operation | Time Complexity | Space Complexity | Notes |
|-----------|----------------|------------------|--------|
| `atlas_domain_create` | O(1) | O(1) | Constant time allocation |
| `atlas_domain_attach` | O(n) | O(1) | Conservation sum calculation |
| `atlas_domain_verify` | O(n) | O(1) | Memory validation scan |
| `atlas_domain_commit` | O(n) | O(n) | Witness generation |
| `atlas_domain_destroy` | O(n) | O(-n) | Witness cleanup |
| `atlas_budget_alloc` | O(1) | O(1) | Atomic operation |
| `atlas_budget_release` | O(1) | O(1) | Atomic operation |
| `atlas_witness_generate` | O(n) | O(n) | Cryptographic computation |
| `atlas_witness_verify` | O(n) | O(1) | Cryptographic verification |
| `atlas_conserved_delta` | O(n) | O(1) | Memory comparison |

Where n = memory size in bytes.

### Performance Targets

#### Throughput Targets (Indicative)
- **AVX2 (x86-64):** ≥ 25 GB/s for conserved memcpy operations
- **NEON (AArch64):** ≥ 15 GB/s for conserved memcpy operations  
- **WASM SIMD:** ≥ 5 GB/s for conserved memcpy operations

#### Latency Targets
- **Budget Operations:** < 50ns per allocation/release
- **Domain Creation:** < 1μs for typical domain  
- **Conservation Delta:** < 10ns per byte on aligned data
- **Witness Generation:** < 100μs for 12KB data

### Optimization Notes

#### Memory Access Patterns
- **Sequential Access:** LLVM IR optimized for sequential memory scans
- **Cache Friendly:** Operations minimize cache misses where possible
- **SIMD Utilization:** LLVM backend automatically vectorizes loops
- **Branch Prediction:** Optimized control flow in hot paths

#### Scalability Characteristics
- **Multiple Domains:** Operations scale linearly with domain count
- **Large Memory:** Memory operations scale linearly with size
- **Thread Count:** Budget operations scale with thread count
- **NUMA Aware:** Uses thread-local storage for error state

---

## Integration Requirements

### Layer Dependencies

#### Layer 0 (Atlas Core) Dependencies
- **LLVM IR Functions:** Conservation, witness, and memory operations
- **Atomic Primitives:** Platform-specific atomic operation support
- **Memory Allocation:** Basic memory management via `atlas_alloc_aligned`

#### Layer 1 (Boundary) Dependencies  
- **Φ-linearization:** Coordinate transformation for memory addressing
- **Klein Bottle Operations:** Boundary manipulation and morphisms
- **Domain Isolation:** Mathematical isolation proofs

### Header Dependencies

```c
#include <stdint.h>     // Standard integer types
#include <stddef.h>     // size_t and NULL definitions
#include <stdbool.h>    // Boolean type support
#include <stdatomic.h>  // Atomic operations (if not single-threaded)
```

### Linking Requirements

#### Static Linking
```bash
gcc your_code.o -L/path/to/atlas -latlas-layer2 -latlas-layer0 -lm
```

#### Dynamic Linking
```bash
gcc your_code.o -L/path/to/atlas -latlas-layer2 -Wl,-rpath,/path/to/atlas
```

### Platform Requirements

#### Minimum Requirements
- **C11 Compiler:** Support for `_Atomic` types and thread-local storage
- **64-bit Platform:** Required for domain IDs and isolation proofs
- **IEEE 754 Arithmetic:** Standard floating-point representation

#### Supported Platforms
- **x86-64:** SSE2/AVX2/AVX-512 SIMD support
- **AArch64:** NEON SIMD support
- **WASM:** WebAssembly SIMD (with single-threaded fallback)

### Compilation Flags

#### Release Build
```bash
-DNDEBUG -O3 -march=native -flto
```

#### Debug Build
```bash
-DDEBUG -O0 -g -fsanitize=address -fsanitize=undefined
```

#### Single-Threaded Build
```bash
-DATLAS_SINGLE_THREAD -O3 -march=native
```

---

## Usage Examples

### Basic Domain Lifecycle

```c
#include "atlas-conservation.h"

int main() {
    // Create domain with 12KB memory and budget class 42
    atlas_domain_t* domain = atlas_domain_create(12288, 42);
    if (!domain) {
        fprintf(stderr, "Domain creation failed: %s\n", 
                atlas_error_string(atlas_get_last_error()));
        return 1;
    }
    
    // Allocate memory for computation
    uint8_t* memory = calloc(12288, 1);
    if (!memory) {
        atlas_domain_destroy(domain);
        return 1;
    }
    
    // Attach memory to domain
    if (atlas_domain_attach(domain, memory, 12288) != 0) {
        fprintf(stderr, "Attach failed: %s\n",
                atlas_error_string(atlas_get_last_error()));
        free(memory);
        atlas_domain_destroy(domain);
        return 1;
    }
    
    // Verify domain integrity
    if (!atlas_domain_verify(domain)) {
        fprintf(stderr, "Verification failed: %s\n",
                atlas_error_string(atlas_get_last_error()));
        free(memory);
        atlas_domain_destroy(domain);
        return 1;
    }
    
    // Commit domain changes
    if (atlas_domain_commit(domain) != 0) {
        fprintf(stderr, "Commit failed: %s\n",
                atlas_error_string(atlas_get_last_error()));
        free(memory);
        atlas_domain_destroy(domain);
        return 1;
    }
    
    printf("Domain successfully created and committed\n");
    
    // Cleanup
    free(memory);
    atlas_domain_destroy(domain);
    return 0;
}
```

### Budget Management

```c
void demonstrate_budget_management(atlas_domain_t* domain) {
    const uint8_t operation_cost = 10;
    
    // Check if we can afford the operation
    if (atlas_budget_alloc(domain, operation_cost)) {
        printf("Budget allocated successfully\n");
        
        // Simulate operation
        bool operation_success = perform_some_operation();
        
        if (!operation_success) {
            // Refund full budget on failure
            atlas_budget_release(domain, operation_cost);
            printf("Operation failed, budget refunded\n");
        } else {
            // Return unused portion (assume we used 7 units)
            uint8_t unused = 3;
            atlas_budget_release(domain, unused);
            printf("Operation succeeded, unused budget refunded\n");
        }
    } else {
        atlas_error_t error = atlas_get_last_error();
        if (error == ATLAS_ERROR_BUDGET_INSUFFICIENT) {
            printf("Insufficient budget for operation\n");
        } else {
            printf("Budget allocation error: %s\n", atlas_error_string(error));
        }
    }
}
```

### Witness Operations

```c
bool verify_memory_integrity(const uint8_t* memory, size_t size) {
    // Generate witness for current memory state
    atlas_witness_t* witness = atlas_witness_generate(memory, size);
    if (!witness) {
        fprintf(stderr, "Witness generation failed: %s\n",
                atlas_error_string(atlas_get_last_error()));
        return false;
    }
    
    // Simulate some operations on memory
    // ... modify memory in conservation-preserving ways ...
    
    // Verify memory still matches witness
    bool is_valid = atlas_witness_verify(witness, memory, size);
    if (!is_valid) {
        fprintf(stderr, "Witness verification failed: %s\n",
                atlas_error_string(atlas_get_last_error()));
        atlas_witness_destroy(witness);
        return false;
    }
    
    printf("Memory integrity verified\n");
    atlas_witness_destroy(witness);
    return true;
}
```

### Conservation Checking

```c
void demonstrate_conservation_checking(void) {
    const size_t size = 1024;
    uint8_t before[1024], after[1024];
    
    // Initialize before state
    memset(before, 42, size);  // All bytes = 42
    
    // Create after state with known delta
    memcpy(after, before, size);
    after[0] = 43;  // Change one byte: 42 -> 43, delta = 1
    
    // Calculate conservation delta
    uint8_t delta = atlas_conserved_delta(before, after, size);
    printf("Conservation delta: %u (expected: 1)\n", delta);
    
    // Test perfect conservation (no change)
    uint8_t zero_delta = atlas_conserved_delta(before, before, size);
    printf("Self-delta: %u (expected: 0)\n", zero_delta);
    
    // Verify conservation law: (sum_before + delta) % 96 == sum_after % 96
    uint32_t sum_before = 0, sum_after = 0;
    for (size_t i = 0; i < size; i++) {
        sum_before += before[i];
        sum_after += after[i];
    }
    
    uint8_t expected = (uint8_t)((sum_after - sum_before) % 96);
    printf("Conservation law check: calculated=%u, expected=%u, %s\n",
           delta, expected, (delta == expected) ? "PASS" : "FAIL");
}
```

### Error Handling Patterns

```c
atlas_error_t safe_domain_operation(atlas_domain_t* domain, 
                                   uint8_t* memory, size_t size) {
    // Pattern 1: Check operation preconditions
    if (!domain || !memory || size == 0) {
        return ATLAS_ERROR_INVALID_ARGUMENT;
    }
    
    // Pattern 2: Attempt operation with error checking
    if (atlas_domain_attach(domain, memory, size) != 0) {
        atlas_error_t error = atlas_get_last_error();
        return error;  // Propagate specific error
    }
    
    // Pattern 3: Verify operation succeeded
    if (!atlas_domain_verify(domain)) {
        atlas_error_t error = atlas_get_last_error();
        // Domain is in inconsistent state, cannot proceed
        return error;
    }
    
    // Pattern 4: Atomic commit with rollback capability
    if (atlas_domain_commit(domain) != 0) {
        atlas_error_t error = atlas_get_last_error();
        // Domain remains in ATTACHED state, can retry
        return error;
    }
    
    return ATLAS_SUCCESS;
}
```

### Multi-threaded Usage

```c
#include <pthread.h>

typedef struct {
    atlas_domain_t* domain;
    uint8_t* memory;
    size_t offset;
    size_t chunk_size;
    int thread_id;
} worker_context_t;

void* worker_thread(void* arg) {
    worker_context_t* ctx = (worker_context_t*)arg;
    
    // Each thread works on different domain - fully thread-safe
    atlas_domain_t* local_domain = atlas_domain_create(ctx->chunk_size, 
                                                       ctx->thread_id % 96);
    if (!local_domain) {
        return NULL;
    }
    
    // Attach local memory chunk
    uint8_t* local_memory = ctx->memory + ctx->offset;
    if (atlas_domain_attach(local_domain, local_memory, ctx->chunk_size) != 0) {
        atlas_domain_destroy(local_domain);
        return NULL;
    }
    
    // Budget operations are atomic - safe on shared domain
    if (atlas_budget_alloc(ctx->domain, 5)) {
        // Perform work with allocated budget
        process_memory_chunk(local_memory, ctx->chunk_size);
        
        // Return unused budget
        atlas_budget_release(ctx->domain, 2);  // Used 3, return 2
    }
    
    atlas_domain_destroy(local_domain);
    return NULL;
}
```

---

This documentation provides a comprehensive contract specification for Atlas-12288 Layer 2 (Conservation). It serves as the definitive reference for developers integrating with the Layer 2 API, covering all aspects from basic usage to advanced concurrency patterns and performance considerations.