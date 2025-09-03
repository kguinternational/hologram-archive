/**
 * projection_seed.c - Example demonstrating L2/L3 integration for L4 unblocking
 * 
 * This example shows how to:
 * 1. Create a conservation domain
 * 2. Generate resonance histogram
 * 3. Build clusters
 * 4. Emit shard metadata with witness ID
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>
#include <time.h>
#include "../include/atlas.h"

// JSON output helper
void emit_json_shard(const char* witness_id, 
                     const uint16_t histogram[96],
                     const atlas_cluster_view* clusters,
                     FILE* out) {
    fprintf(out, "{\n");
    fprintf(out, "  \"witness_id\": \"%s\",\n", witness_id);
    fprintf(out, "  \"timestamp\": %ld,\n", time(NULL));
    fprintf(out, "  \"conservation\": {\n");
    fprintf(out, "    \"verified\": true,\n");
    fprintf(out, "    \"domain_size\": 12288\n");
    fprintf(out, "  },\n");
    
    // Histogram data
    fprintf(out, "  \"histogram\": [\n");
    for (int i = 0; i < 96; i++) {
        fprintf(out, "    %u%s\n", histogram[i], i < 95 ? "," : "");
    }
    fprintf(out, "  ],\n");
    
    // Cluster metadata
    fprintf(out, "  \"clusters\": {\n");
    fprintf(out, "    \"total_elements\": %u,\n", clusters->n);
    fprintf(out, "    \"resonance_counts\": [\n");
    for (int r = 0; r < 96; r++) {
        uint32_t count = clusters->offsets[r+1] - clusters->offsets[r];
        fprintf(out, "      {\"resonance\": %d, \"count\": %u}%s\n", 
                r, count, r < 95 ? "," : "");
    }
    fprintf(out, "  ]},\n");
    
    // Scheduling hints
    fprintf(out, "  \"scheduling\": {\n");
    fprintf(out, "    \"next_windows\": [\n");
    for (int r = 0; r < 8; r++) {  // First 8 resonances as example
        uint64_t next = atlas_next_harmonic_window(r);
        fprintf(out, "      {\"resonance\": %d, \"window\": %lu}%s\n",
                r, next, r < 7 ? "," : "");
    }
    fprintf(out, "    ]\n");
    fprintf(out, "  }\n");
    fprintf(out, "}\n");
}

// Generate witness ID from witness pointer (mock implementation)
const char* generate_witness_id(const atlas_witness_t* witness) {
    static char id[65];
    // In real implementation, this would extract hash from witness
    snprintf(id, sizeof(id), "wit_%016lx", (uintptr_t)witness);
    return id;
}

// Initialize test data with known pattern
void init_test_data(uint8_t* data, size_t size) {
    // Create a pattern that satisfies conservation
    for (size_t i = 0; i < size; i++) {
        data[i] = (uint8_t)((i * 7 + i/256 * 13) % 256);
    }
    
    // Ensure conservation (sum % 96 == 0)
    uint32_t sum = 0;
    for (size_t i = 0; i < size; i++) {
        sum += data[i];
    }
    uint8_t deficit = sum % 96;
    if (deficit > 0 && size > 0) {
        // Adjust last byte to satisfy conservation
        data[size-1] = (data[size-1] + 96 - deficit) % 256;
    }
}

int main(int argc, char* argv[]) {
    printf("Atlas Projection Seed Example\n");
    printf("=============================\n\n");
    
    // Parse output file argument
    const char* output_file = (argc > 1) ? argv[1] : "shard_metadata.json";
    
    // 1. Create conservation domain
    printf("1. Creating conservation domain...\n");
    atlas_domain_t* domain = atlas_domain_create(12288, 50);  // Budget class 50
    if (!domain) {
        fprintf(stderr, "Failed to create domain\n");
        return 1;
    }
    
    // Allocate and initialize data
    uint8_t* atlas_data = (uint8_t*)malloc(12288);
    if (!atlas_data) {
        fprintf(stderr, "Failed to allocate memory\n");
        atlas_domain_destroy(domain);
        return 1;
    }
    init_test_data(atlas_data, 12288);
    
    // Attach memory to domain
    int err = atlas_domain_attach(domain, atlas_data, 12288);
    if (err != ATLAS_OK) {
        fprintf(stderr, "Failed to attach memory: error %d\n", err);
        free(atlas_data);
        atlas_domain_destroy(domain);
        return 1;
    }
    
    // Verify conservation
    if (!atlas_domain_verify(domain)) {
        fprintf(stderr, "Conservation verification failed\n");
        free(atlas_data);
        atlas_domain_destroy(domain);
        return 1;
    }
    printf("   ✓ Domain created and verified\n");
    
    // 2. Generate resonance data
    printf("\n2. Generating resonance histogram...\n");
    uint16_t total_histogram[96] = {0};
    
    // Process each page (48 pages total)
    for (int page = 0; page < 48; page++) {
        uint16_t page_histogram[96];
        atlas_r96_histogram_page(&atlas_data[page * 256], page_histogram);
        
        // Accumulate into total
        for (int i = 0; i < 96; i++) {
            total_histogram[i] += page_histogram[i];
        }
    }
    
    // Verify histogram totals
    uint32_t total = 0;
    for (int i = 0; i < 96; i++) {
        total += total_histogram[i];
    }
    printf("   ✓ Histogram generated (total: %u elements)\n", total);
    
    // 3. Build clusters
    printf("\n3. Building resonance clusters...\n");
    atlas_cluster_view clusters = atlas_cluster_by_resonance(atlas_data, 48);
    if (clusters.n == 0) {
        fprintf(stderr, "Failed to build clusters\n");
        free(atlas_data);
        atlas_domain_destroy(domain);
        return 1;
    }
    
    // Count non-empty clusters
    int non_empty = 0;
    for (int r = 0; r < 96; r++) {
        if (clusters.offsets[r+1] > clusters.offsets[r]) {
            non_empty++;
        }
    }
    printf("   ✓ Clusters built (%d non-empty resonance classes)\n", non_empty);
    
    // 4. Commit domain and generate witness
    printf("\n4. Committing domain and generating witness...\n");
    err = atlas_domain_commit(domain);
    if (err != ATLAS_OK) {
        fprintf(stderr, "Failed to commit domain: error %d\n", err);
        atlas_cluster_destroy(&clusters);
        free(atlas_data);
        atlas_domain_destroy(domain);
        return 1;
    }
    
    // Generate witness for the data
    atlas_witness_t* witness = atlas_witness_generate(atlas_data, 12288);
    if (!witness) {
        fprintf(stderr, "Failed to generate witness\n");
        atlas_cluster_destroy(&clusters);
        free(atlas_data);
        atlas_domain_destroy(domain);
        return 1;
    }
    
    const char* witness_id = generate_witness_id(witness);
    printf("   ✓ Witness generated: %s\n", witness_id);
    
    // 5. Emit shard metadata
    printf("\n5. Emitting shard metadata to %s...\n", output_file);
    FILE* out = fopen(output_file, "w");
    if (!out) {
        fprintf(stderr, "Failed to open output file\n");
        atlas_witness_destroy(witness);
        atlas_cluster_destroy(&clusters);
        free(atlas_data);
        atlas_domain_destroy(domain);
        return 1;
    }
    
    emit_json_shard(witness_id, total_histogram, &clusters, out);
    fclose(out);
    printf("   ✓ Metadata written successfully\n");
    
    // 6. Verify witness (demonstration)
    printf("\n6. Verifying witness integrity...\n");
    if (atlas_witness_verify(witness, atlas_data, 12288)) {
        printf("   ✓ Witness verification passed\n");
    } else {
        printf("   ✗ Witness verification failed\n");
    }
    
    // Clean up
    atlas_witness_destroy(witness);
    atlas_cluster_destroy(&clusters);
    free(atlas_data);
    atlas_domain_destroy(domain);
    
    printf("\n✅ Projection seed completed successfully!\n");
    printf("   Output: %s\n", output_file);
    printf("\nThis data can now be used by Layer 4 for:\n");
    printf("  • Holographic projections\n");
    printf("  • Shard generation\n");
    printf("  • Manifold reconstruction\n");
    
    return 0;
}