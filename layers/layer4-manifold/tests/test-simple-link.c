/* Simple test to verify basic linking works */

#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

// Test just the symbols we know exist
extern int atlas_manifold_get_supported_projections(void);
extern int atlas_manifold_is_optimized(void);

int main(void) {
    printf("=== Simple Link Test ===\n");
    
    int supported = atlas_manifold_get_supported_projections();
    printf("Supported projections: 0x%08X\n", supported);
    
    int optimized = atlas_manifold_is_optimized();
    printf("Is optimized: %s\n", optimized ? "yes" : "no");
    
    printf("✓ Basic linking works\n");
    return 0;
}