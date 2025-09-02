/* atlas-c-api.h - Extended C API wrapper for Atlas-12288
 * (c) 2024-2025 UOR Foundation - MIT License
 */

#ifndef ATLAS_C_API_H
#define ATLAS_C_API_H

#include "atlas.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Extended API for advanced operations */

/* Budget operations */
uint8_t atlas_budget_add(uint8_t a, uint8_t b);
uint8_t atlas_budget_mul(uint8_t a, uint8_t b);
uint8_t atlas_budget_inv(uint8_t budget);
bool atlas_budget_zero(uint8_t budget);

/* Resonance operations */
uint8_t atlas_resonance_harmonic(uint8_t r1, uint8_t r2);
bool atlas_resonance_harmonizes(uint8_t r1, uint8_t r2);

/* Pool management */
typedef void* atlas_pool_t;
atlas_pool_t atlas_pool_create(uint32_t pages);
void* atlas_pool_alloc(atlas_pool_t pool, size_t size);
void atlas_pool_destroy(atlas_pool_t pool);

/* Statistics */
typedef struct {
    uint64_t allocated;
    uint64_t peak;
    uint32_t witness_count;
} atlas_memory_stats_t;

atlas_memory_stats_t atlas_memory_stats(void);

/* Debug operations */
void atlas_debug_conservation(const void* data, size_t len);
bool atlas_debug_validate(const void* structure);

#ifdef __cplusplus
}
#endif

#endif /* ATLAS_C_API_H */