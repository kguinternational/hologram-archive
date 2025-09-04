#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>

// Direct Rust FFI declarations
extern bool atlas_manifold_is_optimized(void);
extern uint32_t atlas_manifold_get_supported_projections(void);

int main() {
    printf("Testing direct Rust FFI calls...\n");
    
    bool is_optimized = atlas_manifold_is_optimized();
    printf("Is optimized: %s\n", is_optimized ? "true" : "false");
    
    uint32_t supported_projections = atlas_manifold_get_supported_projections();
    printf("Supported projections bitmask: 0x%08x\n", supported_projections);
    
    printf("Direct Rust FFI test completed successfully!\n");
    return 0;
}
