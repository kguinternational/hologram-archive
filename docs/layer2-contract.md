# Layer 2 (Conservation) Contract

## Overview

Layer 2 provides domain isolation, conservation verification, witness generation, and budget management for the Atlas-12288 computational model. All operations preserve the fundamental conservation law: `sum(elements) % 96 == 0`.

## Core Invariants

1. **Conservation Law**: The sum of all bytes in a domain modulo 96 must equal 0
2. **Budget Constraint**: Budget values are always in range [0, 95]
3. **Witness Immutability**: Once generated, witnesses cannot be modified
4. **Atomic Commit**: Domain state transitions are atomic (OPEN → COMMITTED)
5. **Failure-Closed**: Invalid operations fail safely without corrupting state

## API Signatures

### Error Codes
```c
typedef enum {
    ATLAS_OK = 0,               // Success
    ATLAS_E_CONSERVATION = 1,   // Conservation law violated
    ATLAS_E_WITNESS = 2,        // Witness verification failed
    ATLAS_E_BUDGET = 3,         // Budget constraint violated
    ATLAS_E_MEMORY = 4,         // Memory allocation failed
    ATLAS_E_STATE = 5           // Invalid state transition
} atlas_error_t;
```

### Domain Management
```c
// Create a new domain with specified budget
atlas_domain_t* atlas_domain_create(size_t bytes, uint8_t budget_class);

// Attach memory region to domain (non-owning)
int atlas_domain_attach(atlas_domain_t* domain, void* base, size_t len);

// Verify domain conservation laws
bool atlas_domain_verify(const atlas_domain_t* domain);

// Commit domain (OPEN → COMMITTED), generates witness
int atlas_domain_commit(atlas_domain_t* domain);

// Destroy domain and free resources
void atlas_domain_destroy(atlas_domain_t* domain);
```

### Budget Operations
```c
// Allocate budget (fails if insufficient)
bool atlas_budget_alloc(atlas_domain_t* domain, uint8_t amt);

// Release budget back to domain
bool atlas_budget_release(atlas_domain_t* domain, uint8_t amt);
```

### Witness Operations
```c
// Generate witness for memory region
atlas_witness_t* atlas_witness_generate(const void* base, size_t len);

// Verify witness against memory
bool atlas_witness_verify(const atlas_witness_t* w, const void* base, size_t len);

// Destroy witness and free resources
void atlas_witness_destroy(atlas_witness_t* w);
```

### Conservation Utilities
```c
// Compute conservation delta between two buffers
uint8_t atlas_conserved_delta(const void* before, const void* after, size_t len);
```

## LLVM IR Kernels (Hot Paths)

```llvm
; Compute delta with mod-96 arithmetic
declare i7 @atlas.conserved.delta(ptr before, ptr after, i64 len)

; Check if window sums to 0 mod 96
declare i1 @atlas.conserved.window.check(ptr data, i64 len)

; Streaming conservation update
declare void @atlas.conserved.update(ptr state, ptr chunk, i64 n)
```

## Memory Model

- **Alignment**: Byte-aligned (no special alignment required)
- **Ownership**: Domains do not own attached memory; caller manages lifetime
- **Witnesses**: Immutable once generated, contain checksums and timestamps
- **Thread Safety**: Budget operations use atomics; commit is race-free

## Performance Characteristics

| Operation | Target Throughput | Notes |
|-----------|------------------|-------|
| Conserved memcpy | ≥25 GB/s (AVX2) | Aligned 12,288-byte windows |
| Conserved memset | ≥30 GB/s (AVX2) | With fixup for conservation |
| Witness generate | ≥1 GB/s | SHA-256 based |
| Witness verify | ≥1 GB/s | Constant-time comparison |
| Delta computation | <10 ns/byte | SIMD optimized |

## Usage Examples

### Basic Domain Lifecycle
```c
// Create domain with budget class 50
atlas_domain_t* domain = atlas_domain_create(12288, 50);

// Attach memory
uint8_t buffer[12288];
atlas_domain_attach(domain, buffer, 12288);

// Verify conservation
if (atlas_domain_verify(domain)) {
    // Commit changes
    atlas_domain_commit(domain);
}

// Clean up
atlas_domain_destroy(domain);
```

### Budget Management
```c
// Allocate 10 units of budget
if (atlas_budget_alloc(domain, 10)) {
    // Perform operations...
    
    // Release 5 units back
    atlas_budget_release(domain, 5);
}
```

### Witness Verification
```c
// Generate witness
atlas_witness_t* w = atlas_witness_generate(buffer, 12288);

// Later: verify integrity
if (atlas_witness_verify(w, buffer, 12288)) {
    printf("Buffer unchanged\n");
}

atlas_witness_destroy(w);
```

## Error Handling

All functions that can fail return error codes or NULL:
- Check return values before proceeding
- Use `atlas_get_last_error()` for detailed error messages
- NULL pointers to destroy functions are safe (no-op)

## Thread Safety

- **Domain State**: Atomic transitions, one successful commit per domain
- **Budget Operations**: Thread-safe via atomic compare-and-swap
- **Witness Generation**: Thread-safe, can generate multiple witnesses
- **Memory Access**: Caller must synchronize access to attached memory

## WASM Compatibility

When compiled with `ATLAS_SINGLE_THREAD`:
- Atomics replaced with simple operations
- No thread-local storage
- Simplified error handling
- Same API signatures maintained

## Conformance Requirements

An implementation is conformant if:
1. All conservation laws are preserved
2. Budget arithmetic is mod-96
3. Witnesses are cryptographically secure
4. State transitions are atomic
5. All error conditions are handled safely
6. Memory is never leaked