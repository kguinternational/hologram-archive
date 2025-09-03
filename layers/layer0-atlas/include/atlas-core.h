/* atlas.h - C API for Atlas-12288 computational model
 * (c) 2024-2025 UOR Foundation - MIT License
 */

#ifndef ATLAS_H
#define ATLAS_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Type definitions */
typedef void* atlas_witness_t;
typedef void* atlas_jit_t;

/* Runtime initialization */
void atlas_init(void);
void atlas_cleanup(void);

/* R96 Classification */
uint8_t atlas_r96_classify(uint8_t byte);
void atlas_r96_classify_buffer(const uint8_t* input, uint8_t* output, size_t len);

/* Conservation operations */
bool atlas_conserved_check(const void* data, size_t len);
uint32_t atlas_conserved_sum(const void* data, size_t len);

/* Witness operations */
atlas_witness_t atlas_witness_generate(const void* data, size_t len);
bool atlas_witness_verify(atlas_witness_t witness, const void* data, size_t len);
void atlas_witness_destroy(atlas_witness_t witness);

/* Boundary operations */
uint32_t atlas_boundary_encode(uint16_t page, uint8_t offset);
void atlas_boundary_decode(uint32_t boundary, uint16_t* page, uint8_t* offset);

/* Memory operations */
void* atlas_alloc_witnessed(size_t size, uint8_t resonance);
void atlas_free_witnessed(void* ptr);
void* atlas_alloc_pages(uint32_t count);

/* JIT operations (optional) */
#ifdef ATLAS_JIT_ENABLED
atlas_jit_t* atlas_jit_create(void);
void atlas_jit_destroy(atlas_jit_t* jit);
int atlas_jit_compile(atlas_jit_t* jit, const char* function_name);
void* atlas_jit_get_function(atlas_jit_t* jit, const char* function_name);
#endif

#ifdef __cplusplus
}
#endif

#endif /* ATLAS_H */