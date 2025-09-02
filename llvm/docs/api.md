# Atlas-12288 C API Reference

## Initialization

### `atlas_init()`
Initialize the Atlas runtime system.
```c
void atlas_init(void);
```

### `atlas_cleanup()`
Clean up and release Atlas runtime resources.
```c
void atlas_cleanup(void);
```

## R96 Classification

### `atlas_r96_classify()`
Classify a single byte to its resonance class.
```c
uint8_t atlas_r96_classify(uint8_t byte);
```
- **Parameters**: `byte` - Input byte to classify
- **Returns**: Resonance class (0-95)

### `atlas_r96_classify_buffer()`
Classify an array of bytes.
```c
void atlas_r96_classify_buffer(const uint8_t* input, uint8_t* output, size_t len);
```

### `atlas_r96_histogram()`
Generate resonance histogram for data block.
```c
void atlas_r96_histogram(const void* data, uint32_t* histogram);
```
- **Parameters**: `histogram` - Array of 96 uint32_t elements for class counts

### `atlas_r96_dominant()`
Find dominant resonance class in data block.
```c
uint8_t atlas_r96_dominant(const void* data);
```

## C768 Triple-Cycle Operations

### `atlas_c768_verify_closure()`
Verify conservation closes over 768-step cycle.
```c
bool atlas_c768_verify_closure(const void* structure, uint64_t window_start);
```

### `atlas_c768_compute_window_sum()`
Compute window sum for C768 verification.
```c
uint64_t atlas_c768_compute_window_sum(const void* data, uint64_t offset, uint64_t size);
```

### `atlas_c768_check_page_rhythm()`
Check 16×48 page rhythm alignment.
```c
bool atlas_c768_check_page_rhythm(const void* structure, uint64_t step);
```

### `atlas_c768_check_byte_rhythm()`
Check 3×256 byte rhythm alignment.
```c
bool atlas_c768_check_byte_rhythm(const void* structure, uint64_t step);
```

## Conservation Operations

### `atlas_conserved_check()`
Check if data satisfies conservation laws.
```c
bool atlas_conserved_check(const void* data, size_t len);
```
- **Returns**: `true` if sum(data) % 96 == 0

### `atlas_conserved_sum()`
Calculate conservation sum of data.
```c
uint32_t atlas_conserved_sum(const void* data, size_t len);
```

### `atlas_conserved_add()`
Conservation-preserving addition.
```c
void atlas_conserved_add(void* dst, const void* src1, const void* src2, size_t len);
```

### `atlas_conserved_delta()`
Compute conservation delta between buffers.
```c
int32_t atlas_conserved_delta(const void* before, const void* after, size_t len);
```

## Witness Operations

### `atlas_witness_generate()`
Generate a witness for data block.
```c
atlas_witness_t atlas_witness_generate(const void* data, size_t len);
```

### `atlas_witness_verify()`
Verify witness against data.
```c
bool atlas_witness_verify(atlas_witness_t witness, const void* data, size_t len);
```

### `atlas_witness_destroy()`
Destroy witness and free resources.
```c
void atlas_witness_destroy(atlas_witness_t witness);
```

### `atlas_witness_chain()`
Create witness chain from current and previous.
```c
atlas_witness_t atlas_witness_chain(atlas_witness_t current, atlas_witness_t previous);
```

### `atlas_witness_merge()`
Merge two witnesses into new handle.
```c
atlas_witness_t atlas_witness_merge(atlas_witness_t w1, atlas_witness_t w2);
```

### `atlas_witness_timestamp()`
Extract timestamp from witness.
```c
uint64_t atlas_witness_timestamp(atlas_witness_t witness);
```

### `atlas_witness_resonance()`
Extract resonance class from witness.
```c
uint8_t atlas_witness_resonance(atlas_witness_t witness);
```

## Boundary Operations

### `atlas_boundary_encode()`
Encode page and offset to boundary coordinate.
```c
uint32_t atlas_boundary_encode(uint16_t page, uint8_t offset);
```

### `atlas_boundary_decode()`
Decode boundary coordinate to page and offset.
```c
void atlas_boundary_decode(uint32_t boundary, uint16_t* page, uint8_t* offset);
```

### `atlas_boundary_transform()`
Transform boundary with resonance.
```c
uint32_t atlas_boundary_transform(uint32_t boundary, uint8_t resonance);
```

### `atlas_boundary_klein()`
Get Klein orbit for boundary coordinate.
```c
uint8_t atlas_boundary_klein(uint32_t boundary);
```

### `atlas_boundary_rotate()`
Rotate boundary coordinate using Klein group.
```c
uint32_t atlas_boundary_rotate(uint32_t boundary, uint8_t klein);
```

## Domain Management

### `atlas_domain_create()`
Create new conservation domain.
```c
atlas_domain_t* atlas_domain_create(uint8_t initial_budget);
```

### `atlas_domain_destroy()`
Destroy domain and free resources.
```c
void atlas_domain_destroy(atlas_domain_t* domain);
```

### `atlas_domain_validate()`
Validate domain integrity.
```c
bool atlas_domain_validate(atlas_domain_t* domain);
```

### `atlas_domain_transfer_budget()`
Transfer budget between domains.
```c
bool atlas_domain_transfer_budget(atlas_domain_t* from, atlas_domain_t* to, uint8_t amount);
```

### `atlas_domain_fork()`
Fork domain with budget allocation.
```c
atlas_domain_t* atlas_domain_fork(atlas_domain_t* parent, uint8_t child_budget);
```

### `atlas_domain_merge()`
Merge two domains.
```c
atlas_domain_t* atlas_domain_merge(atlas_domain_t* d1, atlas_domain_t* d2);
```

## Klein Orbit Operations

### `atlas_klein_get_orbit_id()`
Get Klein orbit ID for coordinate.
```c
uint8_t atlas_klein_get_orbit_id(uint32_t coord);
```

### `atlas_klein_is_privileged_orbit()`
Check if coordinate is in privileged orbit {0,1,48,49}.
```c
bool atlas_klein_is_privileged_orbit(uint32_t coord);
```

### `atlas_klein_canonicalize_coord()`
Canonicalize coordinate to orbit representative.
```c
uint32_t atlas_klein_canonicalize_coord(uint32_t coord);
```

### `atlas_klein_verify_coset_partition()`
Verify V₄ cosets partition the structure.
```c
bool atlas_klein_verify_coset_partition(const void* structure);
```

### `atlas_klein_quick_accept()`
Fast acceptance test using Klein properties.
```c
bool atlas_klein_quick_accept(const void* data, size_t len);
```

## Harmonic Operations

### `atlas_harmonic_compute_window()`
Compute optimal harmonic window.
```c
typedef struct {
    uint32_t start;
    uint32_t length; 
    uint8_t resonance;
} atlas_harmonic_window_t;

atlas_harmonic_window_t atlas_harmonic_compute_window(uint8_t resonance, uint16_t c768_base);
```

### `atlas_harmonic_find_pair()`
Find harmonic pair for resonance.
```c
typedef struct {
    uint8_t a, b;  // a + b ≡ 0 (mod 96)
} atlas_harmonic_pair_t;

atlas_harmonic_pair_t atlas_harmonic_find_pair(uint8_t resonance);
```

### `atlas_harmonic_validate_pair()`
Validate harmonic pair conservation.
```c
bool atlas_harmonic_validate_pair(atlas_harmonic_pair_t pair);
```

### `atlas_harmonic_create_cluster()`
Create resonance cluster by affinity.
```c
typedef struct atlas_harmonic_cluster atlas_harmonic_cluster_t;

atlas_harmonic_cluster_t* atlas_harmonic_create_cluster(uint8_t center_resonance, 
                                                       const uint8_t* resonances, 
                                                       uint32_t count, 
                                                       double threshold);
```

## Morphism Operations

### `atlas_morphism_boundary_auto()`
Apply boundary automorphism.
```c
uint32_t atlas_morphism_boundary_auto(uint32_t coord, uint8_t u48, uint8_t u256);
```

### `atlas_morphism_nf_lift()`
Canonical boundary→bulk lift.
```c
void* atlas_morphism_nf_lift(const void* boundary_trace, size_t trace_len);
```

### `atlas_morphism_nf_project()`
Bulk→boundary projection.
```c
void* atlas_morphism_nf_project(const void* bulk_section);
```

### `atlas_morphism_verify_roundtrip()`
Verify Φ round-trip property.
```c
bool atlas_morphism_verify_roundtrip(const void* original, const void* result);
```

### `atlas_morphism_rl_compose()`
RL-96 semiring composition.
```c
uint8_t atlas_morphism_rl_compose(uint8_t a, uint8_t b);
```

## Validation Operations

### `atlas_validation_calculate_entropy()`
Calculate Shannon entropy for page.
```c
double atlas_validation_calculate_entropy(const void* page);
```

### `atlas_validation_validate_page()`
Comprehensive page validation.
```c
typedef struct {
    bool aligned;
    bool structured;
    double entropy;
    uint32_t anomalies;
} atlas_validation_page_info_t;

atlas_validation_page_info_t atlas_validation_validate_page(const void* page);
```

### `atlas_validation_validate_structure()`
Validate complete Atlas-12288 structure.
```c
typedef struct {
    bool complete;
    bool conserved;
    uint32_t pages;
    uint32_t errors;
    uint32_t repairs;
} atlas_validation_structure_info_t;

atlas_validation_structure_info_t atlas_validation_validate_structure(const void* structure);
```

### `atlas_validation_validate_complete()`
Comprehensive validation with error reporting.
```c
typedef struct {
    bool success;
    uint32_t error_code;
    const char* message;
} atlas_validation_result_t;

atlas_validation_result_t atlas_validation_validate_complete(const void* structure, atlas_witness_t witness);
```

## Memory Operations

### `atlas_alloc_witnessed()`
Allocate witnessed memory with resonance tag.
```c
void* atlas_alloc_witnessed(size_t size, uint8_t resonance);
```

### `atlas_free_witnessed()`
Free witnessed memory.
```c
void atlas_free_witnessed(void* ptr);
```

### `atlas_alloc_pages()`
Allocate page-aligned memory.
```c
void* atlas_alloc_pages(uint32_t count);
```

### `atlas_alloc_resonant()`
Resonance-aware allocation with alignment.
```c
void* atlas_alloc_resonant(size_t size, uint8_t resonance, uint32_t alignment);
```

### `atlas_memcpy_conserved()`
Conservation-preserving memcpy.
```c
void atlas_memcpy_conserved(void* dst, const void* src, size_t len);
```

### `atlas_memset_conserved()`
Conservation-preserving memset.
```c
void atlas_memset_conserved(void* dst, uint8_t val, size_t len);
```

## Extended API

### Budget Operations
```c
uint8_t atlas_budget_add(uint8_t a, uint8_t b);
uint8_t atlas_budget_mul(uint8_t a, uint8_t b);
uint8_t atlas_budget_inv(uint8_t budget);
bool atlas_budget_zero(uint8_t budget);
uint8_t atlas_budget_alloc(uint32_t amount);
void atlas_budget_release(uint8_t budget);
```

### Resonance Operations
```c
uint8_t atlas_resonance_harmonic(uint8_t r1, uint8_t r2);
bool atlas_resonance_harmonizes(uint8_t r1, uint8_t r2);
atlas_harmonic_cluster_t* atlas_resonance_cluster(const uint32_t* coords, uint32_t count);
uint64_t atlas_resonance_schedule(uint8_t resonance);
```

### Memory Statistics
```c
typedef struct {
    uint64_t allocated;
    uint64_t peak;
    uint32_t witness_count;
    uint32_t domain_count;
    uint32_t cluster_count;
} atlas_memory_stats_t;

atlas_memory_stats_t atlas_memory_stats(void);
```

## Error Handling

All functions that can fail return NULL pointers or false values. Check return values before use.

## Thread Safety

- Read operations are thread-safe
- Write operations require external synchronization
- Witness generation/verification is thread-safe
- Memory allocation is thread-safe

## Example Usage

```c
#include "atlas.h"

int main() {
    atlas_init();
    
    uint8_t data[256];
    // Fill data...
    
    if (atlas_conserved_check(data, 256)) {
        atlas_witness_t w = atlas_witness_generate(data, 256);
        // Use witness...
        atlas_witness_destroy(w);
    }
    
    atlas_cleanup();
    return 0;
}
```