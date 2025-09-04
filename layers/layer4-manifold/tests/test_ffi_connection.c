#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include "atlas-manifold.h"

int main() {
    printf("Testing Rust FFI connection...\n");
    
    // Test runtime info functions (these should work)
    bool is_optimized = atlas_manifold_is_optimized();
    printf("Is optimized: %s\n", is_optimized ? "true" : "false");
    
    uint32_t supported_projections = atlas_manifold_get_supported_projections();
    printf("Supported projections bitmask: 0x%08x\n", supported_projections);
    
    // Test error handling
    atlas_manifold_error_t error = atlas_manifold_get_last_error();
    printf("Last error: %d (%s)\n", error, atlas_manifold_error_string(error));
    
    printf("Basic FFI connection test completed.\n");
    return 0;
}
