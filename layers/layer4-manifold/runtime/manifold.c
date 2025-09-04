/* runtime/manifold.c - Core manifold runtime operations
 * (c) 2024-2025 UOR Foundation - MIT License
 * 
 * C runtime support for Atlas-12288 Layer 4 (Manifold Layer) providing:
 * - Core manifold management utilities
 * - Runtime initialization and cleanup
 * - Thread-safe error handling
 * - Performance monitoring and statistics
 * - Memory management helpers
 */

#define _GNU_SOURCE
#define _POSIX_C_SOURCE 200809L

#include "../include/atlas-manifold.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <math.h>
#include <errno.h>
#include <stdatomic.h>
#include <time.h>
#include <unistd.h>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

// =============================================================================
// Internal State and Error Management
// =============================================================================

/* Thread-local storage for last error */
static _Thread_local atlas_manifold_error_t g_last_error = ATLAS_MANIFOLD_SUCCESS;

/* Runtime statistics - using atomic operations for thread safety */
static atomic_uint_fast64_t g_projections_created = 0;
static atomic_uint_fast64_t g_shards_extracted = 0; 
static atomic_uint_fast64_t g_transforms_applied = 0;

/* Global lock for non-atomic operations */
static pthread_mutex_t g_manifold_mutex = PTHREAD_MUTEX_INITIALIZER;

/* Runtime initialization flag */
static atomic_bool g_runtime_initialized = false;

// =============================================================================
// Error Handling Implementation
// =============================================================================

atlas_manifold_error_t atlas_manifold_get_last_error(void) {
    return g_last_error;
}

const char* atlas_manifold_error_string(atlas_manifold_error_t error) {
    switch (error) {
        case ATLAS_MANIFOLD_SUCCESS:
            return "Success";
        case ATLAS_MANIFOLD_ERROR_INVALID_ARGUMENT:
            return "Invalid argument provided";
        case ATLAS_MANIFOLD_ERROR_OUT_OF_MEMORY:
            return "Out of memory";
        case ATLAS_MANIFOLD_ERROR_INVALID_PROJECTION:
            return "Invalid projection handle";
        case ATLAS_MANIFOLD_ERROR_INVALID_SHARD:
            return "Invalid shard handle";
        case ATLAS_MANIFOLD_ERROR_RECONSTRUCTION_FAILED:
            return "Shard reconstruction failed";
        case ATLAS_MANIFOLD_ERROR_TRANSFORM_FAILED:
            return "Transformation operation failed";
        case ATLAS_MANIFOLD_ERROR_BOUNDARY_INVALID:
            return "Boundary region is invalid";
        case ATLAS_MANIFOLD_ERROR_VERIFICATION_FAILED:
            return "Verification checks failed";
        case ATLAS_MANIFOLD_ERROR_NOT_IMPLEMENTED:
            return "Feature not yet implemented";
        default:
            return "Unknown error";
    }
}

/**
 * Set the thread-local error state.
 * 
 * @param error Error code to set
 */
static void atlas_manifold_set_error(atlas_manifold_error_t error) {
    g_last_error = error;
}

// =============================================================================
// Runtime Initialization and Management
// =============================================================================

/**
 * Initialize the manifold runtime system.
 * This function is called automatically but can be called explicitly for
 * early initialization. Thread-safe.
 * 
 * @return 0 on success, -1 on error
 */
int atlas_manifold_init_runtime(void) {
    bool expected = false;
    if (atomic_compare_exchange_strong(&g_runtime_initialized, &expected, true)) {
        // First initialization
        
        // Reset all statistics
        atomic_store(&g_projections_created, 0);
        atomic_store(&g_shards_extracted, 0);
        atomic_store(&g_transforms_applied, 0);
        
        // Initialize mutex
        if (pthread_mutex_init(&g_manifold_mutex, NULL) != 0) {
            atlas_manifold_set_error(ATLAS_MANIFOLD_ERROR_OUT_OF_MEMORY);
            atomic_store(&g_runtime_initialized, false);
            return -1;
        }
        
        // Clear error state
        atlas_manifold_set_error(ATLAS_MANIFOLD_SUCCESS);
        
        return 0;
    }
    
    // Already initialized
    return 0;
}

/**
 * Cleanup the manifold runtime system.
 * This should be called at program termination for clean shutdown.
 * Thread-safe.
 */
void atlas_manifold_cleanup_runtime(void) {
    bool expected = true;
    if (atomic_compare_exchange_strong(&g_runtime_initialized, &expected, false)) {
        // Cleanup mutex
        pthread_mutex_destroy(&g_manifold_mutex);
        
        // Clear error state
        atlas_manifold_set_error(ATLAS_MANIFOLD_SUCCESS);
    }
}

/**
 * Check if the runtime system is properly initialized.
 * 
 * @return true if initialized, false otherwise
 */
bool atlas_manifold_is_runtime_initialized(void) {
    return atomic_load(&g_runtime_initialized);
}

// =============================================================================
// Memory Management Utilities
// =============================================================================

/**
 * Allocate aligned memory for manifold operations.
 * 
 * @param size Size in bytes to allocate
 * @param alignment Alignment requirement (must be power of 2)
 * @return Pointer to allocated memory, or NULL on failure
 */
void* atlas_manifold_aligned_alloc(size_t size, size_t alignment) {
    if (size == 0 || alignment == 0 || (alignment & (alignment - 1)) != 0) {
        atlas_manifold_set_error(ATLAS_MANIFOLD_ERROR_INVALID_ARGUMENT);
        return NULL;
    }
    
    void* ptr;
    int result = posix_memalign(&ptr, alignment, size);
    
    if (result != 0) {
        atlas_manifold_set_error(ATLAS_MANIFOLD_ERROR_OUT_OF_MEMORY);
        return NULL;
    }
    
    // Zero the memory for security
    memset(ptr, 0, size);
    
    atlas_manifold_set_error(ATLAS_MANIFOLD_SUCCESS);
    return ptr;
}

/**
 * Free memory allocated by atlas_manifold_aligned_alloc.
 * 
 * @param ptr Pointer to memory to free (can be NULL)
 */
void atlas_manifold_aligned_free(void* ptr) {
    if (ptr != NULL) {
        free(ptr);
    }
}

/**
 * Secure memory clear - ensures memory is actually zeroed.
 * 
 * @param ptr Pointer to memory to clear
 * @param size Size in bytes to clear
 */
void atlas_manifold_secure_clear(void* ptr, size_t size) {
    if (ptr != NULL && size > 0) {
        // Use volatile to prevent compiler optimization
        volatile unsigned char* volatile_ptr = (volatile unsigned char*)ptr;
        for (size_t i = 0; i < size; i++) {
            volatile_ptr[i] = 0;
        }
    }
}

// =============================================================================
// Validation and Utility Functions
// =============================================================================

/**
 * Validate that a pointer is non-null and properly aligned.
 * 
 * @param ptr Pointer to validate
 * @param required_alignment Required alignment (must be power of 2)
 * @return true if valid, false otherwise
 */
bool atlas_manifold_validate_pointer(const void* ptr, size_t required_alignment) {
    if (ptr == NULL) {
        atlas_manifold_set_error(ATLAS_MANIFOLD_ERROR_INVALID_ARGUMENT);
        return false;
    }
    
    if (required_alignment > 0 && (required_alignment & (required_alignment - 1)) == 0) {
        uintptr_t addr = (uintptr_t)ptr;
        if ((addr & (required_alignment - 1)) != 0) {
            atlas_manifold_set_error(ATLAS_MANIFOLD_ERROR_INVALID_ARGUMENT);
            return false;
        }
    }
    
    return true;
}

/**
 * Validate transformation parameters for mathematical soundness.
 * 
 * @param params Transformation parameters to validate
 * @return true if valid, false otherwise
 */
bool atlas_manifold_validate_transform_params_internal(const atlas_transform_params_t* params) {
    if (!atlas_manifold_validate_pointer(params, sizeof(double))) {
        return false;
    }
    
    // Check for NaN or infinite values
    if (!isfinite(params->scaling_factor) || !isfinite(params->rotation_angle) ||
        !isfinite(params->translation_x) || !isfinite(params->translation_y)) {
        atlas_manifold_set_error(ATLAS_MANIFOLD_ERROR_INVALID_ARGUMENT);
        return false;
    }
    
    // Scaling factor must be positive and not too extreme
    if (params->scaling_factor <= 0.0 || params->scaling_factor > 1000.0 || 
        params->scaling_factor < 1e-6) {
        atlas_manifold_set_error(ATLAS_MANIFOLD_ERROR_INVALID_ARGUMENT);
        return false;
    }
    
    // Rotation angle should be normalized to [-2π, 2π] for stability
    if (fabs(params->rotation_angle) > 4.0 * M_PI) {
        atlas_manifold_set_error(ATLAS_MANIFOLD_ERROR_INVALID_ARGUMENT);
        return false;
    }
    
    // Translation values should be reasonable
    if (fabs(params->translation_x) > 1e6 || fabs(params->translation_y) > 1e6) {
        atlas_manifold_set_error(ATLAS_MANIFOLD_ERROR_INVALID_ARGUMENT);
        return false;
    }
    
    atlas_manifold_set_error(ATLAS_MANIFOLD_SUCCESS);
    return true;
}

/**
 * Calculate a 32-bit hash from data for integrity checking.
 * 
 * @param data Pointer to data to hash
 * @param size Size of data in bytes
 * @return 32-bit hash value
 */
uint32_t atlas_manifold_hash32(const void* data, size_t size) {
    if (data == NULL || size == 0) {
        return 0;
    }
    
    const unsigned char* bytes = (const unsigned char*)data;
    uint32_t hash = 0x811c9dc5; // FNV offset basis
    
    for (size_t i = 0; i < size; i++) {
        hash ^= bytes[i];
        hash *= 0x01000193; // FNV prime
    }
    
    return hash;
}

// =============================================================================
// Runtime Information and Statistics Implementation
// =============================================================================

bool atlas_manifold_is_optimized(void) {
#ifdef NDEBUG
    return true;
#else
    return false;
#endif
}

uint32_t atlas_manifold_get_supported_projections(void) {
    uint32_t bitmask = 0;
    bitmask |= (1u << ATLAS_PROJECTION_LINEAR);      // Bit 0: Linear projection
    bitmask |= (1u << ATLAS_PROJECTION_R96_FOURIER); // Bit 1: R96 Fourier projection
    return bitmask;
}

bool atlas_manifold_get_statistics(uint64_t* projections_created,
                                  uint64_t* shards_extracted,
                                  uint64_t* transforms_applied) {
    if (projections_created == NULL || shards_extracted == NULL || 
        transforms_applied == NULL) {
        atlas_manifold_set_error(ATLAS_MANIFOLD_ERROR_INVALID_ARGUMENT);
        return false;
    }
    
    *projections_created = atomic_load(&g_projections_created);
    *shards_extracted = atomic_load(&g_shards_extracted);
    *transforms_applied = atomic_load(&g_transforms_applied);
    
    atlas_manifold_set_error(ATLAS_MANIFOLD_SUCCESS);
    return true;
}

void atlas_manifold_reset_statistics(void) {
    atomic_store(&g_projections_created, 0);
    atomic_store(&g_shards_extracted, 0);
    atomic_store(&g_transforms_applied, 0);
    
    atlas_manifold_set_error(ATLAS_MANIFOLD_SUCCESS);
}

/**
 * Internal function to increment projection counter.
 * Called by projection creation functions.
 */
void atlas_manifold_inc_projections(void) {
    atomic_fetch_add(&g_projections_created, 1);
}

/**
 * Internal function to increment shard extraction counter.
 * Called by shard extraction functions.
 */
void atlas_manifold_inc_shards(void) {
    atomic_fetch_add(&g_shards_extracted, 1);
}

/**
 * Internal function to increment transform counter.
 * Called by transformation functions.
 */
void atlas_manifold_inc_transforms(void) {
    atomic_fetch_add(&g_transforms_applied, 1);
}

// =============================================================================
// Performance and Monitoring Utilities
// =============================================================================

/**
 * Get current time in microseconds for performance measurement.
 * 
 * @return Current time in microseconds since epoch
 */
uint64_t atlas_manifold_get_time_us(void) {
    struct timespec ts;
    if (clock_gettime(CLOCK_MONOTONIC, &ts) == 0) {
        return ((uint64_t)ts.tv_sec * 1000000ULL) + ((uint64_t)ts.tv_nsec / 1000ULL);
    }
    return 0;
}

/**
 * Simple performance timer for measuring operation duration.
 */
typedef struct {
    uint64_t start_time;
    const char* operation_name;
    bool is_running;
} atlas_perf_timer_t;

/**
 * Start a performance timer.
 * 
 * @param timer Timer structure to initialize
 * @param operation_name Name of the operation being timed
 */
void atlas_manifold_timer_start(atlas_perf_timer_t* timer, const char* operation_name) {
    if (timer != NULL) {
        timer->start_time = atlas_manifold_get_time_us();
        timer->operation_name = operation_name;
        timer->is_running = true;
    }
}

/**
 * Stop a performance timer and optionally print the result.
 * 
 * @param timer Timer structure to stop
 * @param print_result Whether to print timing results to stdout
 * @return Duration in microseconds, or 0 on error
 */
uint64_t atlas_manifold_timer_stop(atlas_perf_timer_t* timer, bool print_result) {
    if (timer == NULL || !timer->is_running) {
        return 0;
    }
    
    uint64_t end_time = atlas_manifold_get_time_us();
    uint64_t duration = end_time - timer->start_time;
    
    timer->is_running = false;
    
    if (print_result && timer->operation_name != NULL) {
        printf("[PERF] %s: %llu us\n", timer->operation_name, 
               (unsigned long long)duration);
    }
    
    return duration;
}

// =============================================================================
// Initialization Hook
// =============================================================================

/**
 * Constructor function to automatically initialize runtime when library is loaded.
 */
__attribute__((constructor))
static void atlas_manifold_auto_init(void) {
    atlas_manifold_init_runtime();
}

/**
 * Destructor function to automatically cleanup runtime when library is unloaded.
 */
__attribute__((destructor)) 
static void atlas_manifold_auto_cleanup(void) {
    atlas_manifold_cleanup_runtime();
}