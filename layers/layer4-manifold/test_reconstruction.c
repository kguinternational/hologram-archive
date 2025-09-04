#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include "include/atlas-manifold.h"

int main() {
    printf("Testing Atlas Reconstruction Implementation...\n");
    
    // Create test data (4096 bytes = 1 page)
    size_t test_data_size = 4096;
    uint8_t* test_data = malloc(test_data_size);
    if (test_data == NULL) {
        printf("ERROR: Failed to allocate test data\n");
        return 1;
    }
    
    // Fill with test pattern
    for (size_t i = 0; i < test_data_size; i++) {
        test_data[i] = (uint8_t)(i % 256);
    }
    
    // Create a projection from test data
    atlas_projection_t projection = atlas_projection_create(ATLAS_PROJECTION_LINEAR, test_data, test_data_size);
    if (projection == NULL) {
        printf("ERROR: Failed to create projection\n");
        free(test_data);
        return 1;
    }
    
    printf("✓ Created projection from test data\n");
    
    // Create boundary regions for sharding (split into 2 shards)
    atlas_boundary_region_t region1 = {
        .start_coord = 0,
        .end_coord = 2047,      // First half
        .page_count = 1,
        .region_class = 0
    };
    
    atlas_boundary_region_t region2 = {
        .start_coord = 2048,
        .end_coord = 4095,      // Second half  
        .page_count = 1,
        .region_class = 1
    };
    
    // Extract shards
    atlas_shard_t shard1 = atlas_shard_extract(projection, &region1);
    atlas_shard_t shard2 = atlas_shard_extract(projection, &region2);
    
    if (shard1 == NULL || shard2 == NULL) {
        printf("ERROR: Failed to extract shards\n");
        if (shard1) atlas_shard_destroy(shard1);
        if (shard2) atlas_shard_destroy(shard2);
        atlas_projection_destroy(projection);
        free(test_data);
        return 1;
    }
    
    printf("✓ Extracted 2 shards from projection\n");
    
    // Verify shards
    if (!atlas_shard_verify(shard1) || !atlas_shard_verify(shard2)) {
        printf("ERROR: Shard verification failed\n");
        atlas_shard_destroy(shard1);
        atlas_shard_destroy(shard2);
        atlas_projection_destroy(projection);
        free(test_data);
        return 1;
    }
    
    printf("✓ Shards verified successfully\n");
    
    // Initialize reconstruction context
    atlas_reconstruction_ctx_t ctx = atlas_reconstruction_init(2);
    if (ctx.shards == NULL) {
        printf("ERROR: Failed to initialize reconstruction context\n");
        atlas_shard_destroy(shard1);
        atlas_shard_destroy(shard2);
        atlas_projection_destroy(projection);
        free(test_data);
        return 1;
    }
    
    printf("✓ Initialized reconstruction context\n");
    
    // Add shards to reconstruction context (in reverse order to test Φ-ordering)
    if (atlas_reconstruction_add_shard(&ctx, shard2) != 0 ||
        atlas_reconstruction_add_shard(&ctx, shard1) != 0) {
        printf("ERROR: Failed to add shards to reconstruction context\n");
        atlas_reconstruction_destroy(&ctx);
        atlas_shard_destroy(shard1);
        atlas_shard_destroy(shard2);
        atlas_projection_destroy(projection);
        free(test_data);
        return 1;
    }
    
    printf("✓ Added shards to reconstruction context\n");
    
    // Check if reconstruction is complete
    if (!atlas_reconstruction_is_complete(&ctx)) {
        printf("ERROR: Reconstruction context should be complete\n");
        atlas_reconstruction_destroy(&ctx);
        atlas_shard_destroy(shard1);
        atlas_shard_destroy(shard2);
        atlas_projection_destroy(projection);
        free(test_data);
        return 1;
    }
    
    printf("✓ Reconstruction context is complete\n");
    
    // Finalize reconstruction
    atlas_projection_t reconstructed = atlas_reconstruction_finalize(&ctx, ATLAS_PROJECTION_LINEAR);
    if (reconstructed == NULL) {
        printf("ERROR: Failed to finalize reconstruction\n");
        atlas_shard_destroy(shard1);
        atlas_shard_destroy(shard2);
        atlas_projection_destroy(projection);
        free(test_data);
        return 1;
    }
    
    printf("✓ Reconstructed projection successfully\n");
    
    // Verify reconstructed projection
    if (!atlas_manifold_verify_projection(reconstructed)) {
        printf("ERROR: Reconstructed projection failed verification\n");
        atlas_projection_destroy(reconstructed);
        atlas_shard_destroy(shard1);
        atlas_shard_destroy(shard2);
        atlas_projection_destroy(projection);
        free(test_data);
        return 1;
    }
    
    printf("✓ Reconstructed projection passed verification\n");
    
    // Clean up
    atlas_projection_destroy(reconstructed);
    atlas_shard_destroy(shard1);
    atlas_shard_destroy(shard2);
    atlas_projection_destroy(projection);
    free(test_data);
    
    printf("✅ All tests passed! Reconstruction implementation working correctly.\n");
    return 0;
}