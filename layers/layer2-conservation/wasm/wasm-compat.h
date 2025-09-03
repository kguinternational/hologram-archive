/* wasm-compat.h - WebAssembly Compatibility Layer for Atlas Conservation
 * (c) 2024-2025 UOR Foundation - MIT License
 * 
 * WASM-compatible single-threaded macros and compatibility layer for the
 * Atlas-12288 Layer 2 Conservation runtime. Provides non-atomic operations
 * when ATLAS_SINGLE_THREAD is defined (for WASM environments).
 */

#ifndef ATLAS_WASM_COMPAT_H
#define ATLAS_WASM_COMPAT_H

#ifdef __cplusplus
extern "C" {
#endif

// Define ATLAS_SINGLE_THREAD for WASM builds
#ifndef ATLAS_SINGLE_THREAD
    #ifdef __EMSCRIPTEN__
        #define ATLAS_SINGLE_THREAD 1
    #endif
    #ifdef WASM
        #define ATLAS_SINGLE_THREAD 1
    #endif
    #ifdef __wasm__
        #define ATLAS_SINGLE_THREAD 1
    #endif
    #ifdef __wasm32__
        #define ATLAS_SINGLE_THREAD 1
    #endif
#endif

// Single-threaded atomic operation macros
#ifdef ATLAS_SINGLE_THREAD
    // Simple direct memory operations for single-threaded environments
    #define ATLAS_ATOMIC_LOAD(ptr) (*(ptr))
    #define ATLAS_ATOMIC_STORE(ptr, val) (*(ptr) = (val))
    
    // Compare-and-swap simulation
    #define ATLAS_ATOMIC_CAS(ptr, expected, desired) \
        ((*(ptr) == (expected)) ? (*(ptr) = (desired), true) : false)
    
    // Fetch-and-add/sub operations
    #define ATLAS_ATOMIC_FETCH_ADD(ptr, val) \
        ({ __typeof__(*(ptr)) _old = *(ptr); *(ptr) += (val); _old; })
    #define ATLAS_ATOMIC_FETCH_SUB(ptr, val) \
        ({ __typeof__(*(ptr)) _old = *(ptr); *(ptr) -= (val); _old; })
    
    // Memory barriers (no-op in single-threaded)
    #define ATLAS_MEMORY_BARRIER() ((void)0)
    #define ATLAS_ACQUIRE_BARRIER() ((void)0)
    #define ATLAS_RELEASE_BARRIER() ((void)0)
    
    // Thread-local storage simulation (global variables in single-threaded)
    #define ATLAS_THREAD_LOCAL static
    
#else
    // Multi-threaded atomic operations using C11 atomics
    #include <stdatomic.h>
    
    #define ATLAS_ATOMIC_LOAD(ptr) atomic_load(ptr)
    #define ATLAS_ATOMIC_STORE(ptr, val) atomic_store(ptr, val)
    #define ATLAS_ATOMIC_CAS(ptr, expected, desired) \
        atomic_compare_exchange_strong(ptr, &(expected), desired)
    #define ATLAS_ATOMIC_FETCH_ADD(ptr, val) atomic_fetch_add(ptr, val)
    #define ATLAS_ATOMIC_FETCH_SUB(ptr, val) atomic_fetch_sub(ptr, val)
    
    // Real memory barriers
    #define ATLAS_MEMORY_BARRIER() atomic_thread_fence(memory_order_seq_cst)
    #define ATLAS_ACQUIRE_BARRIER() atomic_thread_fence(memory_order_acquire)
    #define ATLAS_RELEASE_BARRIER() atomic_thread_fence(memory_order_release)
    
    // Thread-local storage
    #define ATLAS_THREAD_LOCAL _Thread_local
    
#endif

// Platform detection and capability flags
#ifdef ATLAS_SINGLE_THREAD
    #define ATLAS_HAS_ATOMICS 0
    #define ATLAS_HAS_THREADING 0
    #define ATLAS_IS_WASM 1
#else
    #define ATLAS_HAS_ATOMICS 1
    #define ATLAS_HAS_THREADING 1
    #define ATLAS_IS_WASM 0
#endif

// WASM-specific memory management hints
#ifdef ATLAS_SINGLE_THREAD
    // Memory allocation hints for WASM
    #define ATLAS_WASM_MEMORY_GROW(pages) \
        (__builtin_wasm_memory_grow(0, pages))
    #define ATLAS_WASM_MEMORY_SIZE() \
        (__builtin_wasm_memory_size(0))
    
    // Stack size limits in WASM
    #define ATLAS_WASM_MAX_STACK_DEPTH 1000
    #define ATLAS_WASM_RECURSION_LIMIT 100
    
    // Disable features not available in WASM
    #define ATLAS_NO_SIGNALS 1
    #define ATLAS_NO_FILESYSTEM 1
    #define ATLAS_NO_NETWORK 1
#endif

// Performance tuning for WASM
#ifdef ATLAS_SINGLE_THREAD
    // Smaller batch sizes for WASM due to memory constraints
    #define ATLAS_WASM_MAX_BATCH_SIZE 64
    #define ATLAS_WASM_PREFERRED_BATCH_SIZE 16
    
    // Reduced cache assumptions for WASM
    #define ATLAS_WASM_L1_CACHE_SIZE (16 * 1024)  // Conservative 16KB
    #define ATLAS_WASM_PAGE_SIZE 65536            // WASM page size
#endif

// WASM export macros
#ifdef __EMSCRIPTEN__
    #include <emscripten.h>
    #define ATLAS_WASM_EXPORT EMSCRIPTEN_KEEPALIVE
    #define ATLAS_WASM_IMPORT extern
#else
    #define ATLAS_WASM_EXPORT __attribute__((visibility("default")))
    #define ATLAS_WASM_IMPORT extern
#endif

// Error handling adaptations for WASM
#ifdef ATLAS_SINGLE_THREAD
    // No errno in WASM, use simple error tracking
    #define ATLAS_ERRNO_SUPPORT 0
    #define ATLAS_SET_ERRNO(val) ((void)0)
    #define ATLAS_GET_ERRNO() (0)
#else
    #define ATLAS_ERRNO_SUPPORT 1
    #define ATLAS_SET_ERRNO(val) (errno = (val))
    #define ATLAS_GET_ERRNO() (errno)
#endif

// Logging and debugging for WASM
#ifdef ATLAS_SINGLE_THREAD
    #ifdef __EMSCRIPTEN__
        // Use Emscripten console logging
        #define ATLAS_LOG_DEBUG(msg) emscripten_console_log("[ATLAS-DEBUG] " msg)
        #define ATLAS_LOG_INFO(msg) emscripten_console_log("[ATLAS-INFO] " msg)
        #define ATLAS_LOG_ERROR(msg) emscripten_console_error("[ATLAS-ERROR] " msg)
    #else
        // Fallback to no-op logging
        #define ATLAS_LOG_DEBUG(msg) ((void)0)
        #define ATLAS_LOG_INFO(msg) ((void)0)
        #define ATLAS_LOG_ERROR(msg) ((void)0)
    #endif
#else
    // Standard logging
    #include <stdio.h>
    #define ATLAS_LOG_DEBUG(msg) fprintf(stderr, "[ATLAS-DEBUG] " msg "\n")
    #define ATLAS_LOG_INFO(msg) fprintf(stderr, "[ATLAS-INFO] " msg "\n")
    #define ATLAS_LOG_ERROR(msg) fprintf(stderr, "[ATLAS-ERROR] " msg "\n")
#endif

// Type size verification for WASM compatibility
_Static_assert(sizeof(void*) <= 8, "WASM supports max 64-bit pointers");
_Static_assert(sizeof(size_t) <= 8, "size_t must fit in 64 bits for WASM");
_Static_assert(sizeof(uint64_t) == 8, "uint64_t must be exactly 64 bits");
_Static_assert(sizeof(uint32_t) == 4, "uint32_t must be exactly 32 bits");
_Static_assert(sizeof(uint8_t) == 1, "uint8_t must be exactly 8 bits");

// Runtime capability detection
static inline int atlas_runtime_is_wasm(void) {
#ifdef ATLAS_SINGLE_THREAD
    return 1;
#else
    return 0;
#endif
}

static inline int atlas_runtime_has_threads(void) {
#ifdef ATLAS_SINGLE_THREAD
    return 0;
#else
    return 1;
#endif
}

static inline int atlas_runtime_has_atomics(void) {
#ifdef ATLAS_SINGLE_THREAD
    return 0;
#else
    return 1;
#endif
}

// WASM memory management helpers
#ifdef ATLAS_SINGLE_THREAD
static inline size_t atlas_wasm_memory_pages(void) {
    return ATLAS_WASM_MEMORY_SIZE();
}

static inline size_t atlas_wasm_memory_bytes(void) {
    return atlas_wasm_memory_pages() * ATLAS_WASM_PAGE_SIZE;
}

static inline int atlas_wasm_grow_memory(size_t additional_pages) {
    return ATLAS_WASM_MEMORY_GROW(additional_pages) != (size_t)-1;
}
#endif

#ifdef __cplusplus
}
#endif

#endif /* ATLAS_WASM_COMPAT_H */