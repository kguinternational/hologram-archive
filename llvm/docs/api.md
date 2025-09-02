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

## Extended API

### Budget Operations
```c
uint8_t atlas_budget_add(uint8_t a, uint8_t b);
uint8_t atlas_budget_mul(uint8_t a, uint8_t b);
uint8_t atlas_budget_inv(uint8_t budget);
bool atlas_budget_zero(uint8_t budget);
```

### Resonance Operations
```c
uint8_t atlas_resonance_harmonic(uint8_t r1, uint8_t r2);
bool atlas_resonance_harmonizes(uint8_t r1, uint8_t r2);
```

### Memory Statistics
```c
typedef struct {
    uint64_t allocated;
    uint64_t peak;
    uint32_t witness_count;
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