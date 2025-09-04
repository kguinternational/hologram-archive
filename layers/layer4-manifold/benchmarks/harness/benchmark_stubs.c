/* benchmark_stubs.c - Atlas Layer 4 Benchmark Stub Implementations
 * (c) 2024-2025 UOR Foundation - MIT License
 * 
 * Provides stub implementations for missing atlas-manifold functions
 * that are referenced in benchmarks but not available in the current library.
 */

#include "atlas-manifold.h"
#include <stdio.h>
#include <stdlib.h>

// =============================================================================
// Missing Function Stubs
// =============================================================================

/**
 * Stub implementation for atlas_reconstruction_destroy
 * 
 * This function is declared in atlas-manifold.h but the library exports
 * atlas_reconstruction_destroy_ptr instead. This stub provides the missing
 * symbol to fix linking issues.
 * 
 * @param ctx Reconstruction context to destroy
 */
void atlas_reconstruction_destroy(atlas_reconstruction_ctx_t* ctx) {
    if (ctx == NULL) {
        return;
    }
    
    // Clean up the context fields
    if (ctx->shards != NULL) {
        // Note: We don't destroy the individual shards here as they are owned by the caller
        // The caller is responsible for managing shard lifecycle
        free(ctx->shards);
        ctx->shards = NULL;
    }
    
    ctx->total_shards = 0;
    ctx->current_shard = 0;
    ctx->shard_capacity = 0;
    ctx->checksum = 0;
    ctx->is_complete = false;
}

// =============================================================================
// End of Stub Implementations
// =============================================================================