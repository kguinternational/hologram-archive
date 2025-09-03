/* atlas-manifold.h - Atlas-12288 Layer 4 Manifold Interface
 * (c) 2024-2025 UOR Foundation - MIT License
 * 
 * Public API for Atlas-12288 Layer 4 (Manifold Layer) providing:
 * - Manifold projections and transformations
 * - Atlas shard extraction and reconstruction  
 * - Geometric operations and topology analysis
 * - Advanced mathematical operations on computational space
 */

#ifndef ATLAS_MANIFOLD_H
#define ATLAS_MANIFOLD_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

// =============================================================================
// Version Information
// =============================================================================

#define ATLAS_MANIFOLD_VERSION_MAJOR 1
#define ATLAS_MANIFOLD_VERSION_MINOR 0
#define ATLAS_MANIFOLD_VERSION_PATCH 0

// =============================================================================
// Type Definitions
// =============================================================================

/* Forward declarations for opaque types */
struct atlas_projection_internal;
struct atlas_shard_internal;

/* Opaque projection handle - actual structure is implementation-defined */
typedef struct atlas_projection_internal* atlas_projection_t;

/* Opaque shard handle - actual structure is implementation-defined */  
typedef struct atlas_shard_internal* atlas_shard_t;

/* Projection types supported by Layer 4 */
typedef enum {
    ATLAS_PROJECTION_LINEAR = 0,      /* Linear projection mapping */
    ATLAS_PROJECTION_R96_FOURIER = 1  /* R96 Fourier transform projection */
} atlas_projection_type_t;

/* Boundary region structure for manifold operations */
typedef struct {
    uint32_t start_coord;    /* Starting boundary coordinate */
    uint32_t end_coord;      /* Ending boundary coordinate */
    uint16_t page_count;     /* Number of pages in region */
    uint8_t  region_class;   /* Resonance class of region */
    bool     is_conserved;   /* Conservation status */
} atlas_boundary_region_t;

/* Manifold transformation parameters */
typedef struct {
    double scaling_factor;   /* Geometric scaling parameter */
    double rotation_angle;   /* Rotation in radians */
    double translation_x;    /* X-axis translation */
    double translation_y;    /* Y-axis translation */
} atlas_transform_params_t;

/* Shard reconstruction context */
typedef struct {
    uint32_t total_shards;   /* Total number of shards */
    uint32_t current_shard;  /* Current shard being processed */
    uint64_t checksum;       /* Verification checksum */
    bool     is_complete;    /* Reconstruction completion status */
} atlas_reconstruction_ctx_t;

// =============================================================================
// Error Handling
// =============================================================================

/* Error codes specific to Layer 4 manifold operations */
typedef enum {
    ATLAS_MANIFOLD_SUCCESS = 0,
    ATLAS_MANIFOLD_ERROR_INVALID_ARGUMENT = 1,
    ATLAS_MANIFOLD_ERROR_OUT_OF_MEMORY = 2,
    ATLAS_MANIFOLD_ERROR_INVALID_PROJECTION = 3,
    ATLAS_MANIFOLD_ERROR_INVALID_SHARD = 4,
    ATLAS_MANIFOLD_ERROR_RECONSTRUCTION_FAILED = 5,
    ATLAS_MANIFOLD_ERROR_TRANSFORM_FAILED = 6,
    ATLAS_MANIFOLD_ERROR_BOUNDARY_INVALID = 7,
    ATLAS_MANIFOLD_ERROR_VERIFICATION_FAILED = 8,
    ATLAS_MANIFOLD_ERROR_NOT_IMPLEMENTED = 9
} atlas_manifold_error_t;

/**
 * Get the last manifold operation error.
 * 
 * @return Last error code from manifold operations
 */
atlas_manifold_error_t atlas_manifold_get_last_error(void);

/**
 * Get human-readable error message for manifold error code.
 * 
 * @param error Error code
 * @return Pointer to static error message string
 */
const char* atlas_manifold_error_string(atlas_manifold_error_t error);

// =============================================================================
// Projection Management Functions
// =============================================================================

/**
 * Create a new manifold projection.
 * 
 * Creates a projection handle for the specified type that can be used
 * to transform Atlas computational space into different coordinate systems
 * or representations.
 * 
 * @param type Projection type (LINEAR or R96_FOURIER)
 * @param source_data Pointer to source data for projection
 * @param source_size Size of source data in bytes
 * @return New projection handle, or NULL on error
 * 
 * Thread safety: Safe to call from multiple threads
 * Memory: Caller must call atlas_projection_destroy() to free resources
 */
atlas_projection_t atlas_projection_create(atlas_projection_type_t type,
                                          const void* source_data,
                                          size_t source_size);

/**
 * Apply transformation parameters to a projection.
 * 
 * Modifies the projection using the specified transformation parameters
 * for geometric operations like scaling, rotation, and translation.
 * 
 * @param projection Projection handle (must be valid)
 * @param params Transformation parameters
 * @return 0 on success, -1 on error
 * 
 * Thread safety: Not thread-safe (modifies projection state)
 */
int atlas_projection_transform(atlas_projection_t projection,
                              const atlas_transform_params_t* params);

/**
 * Get the dimensions of a projection's output space.
 * 
 * @param projection Projection handle (must be valid)
 * @param width Output parameter for projection width
 * @param height Output parameter for projection height
 * @return 0 on success, -1 on error
 * 
 * Thread safety: Safe to call concurrently (read-only operation)
 */
int atlas_projection_get_dimensions(const atlas_projection_t projection,
                                   uint32_t* width, uint32_t* height);

/**
 * Check if projection is valid and ready for use.
 * 
 * @param projection Projection handle to verify
 * @return true if projection is valid, false otherwise
 * 
 * Thread safety: Safe to call concurrently (read-only operation)
 */
bool atlas_projection_is_valid(const atlas_projection_t projection);

/**
 * Destroy projection and free all associated resources.
 * 
 * @param projection Projection handle (can be NULL - no-op)
 * 
 * Thread safety: Not thread-safe (modifies and frees projection)
 * Memory: Frees all projection resources
 */
void atlas_projection_destroy(atlas_projection_t projection);

// =============================================================================
// Shard Extraction Functions
// =============================================================================

/**
 * Extract shard from Atlas computational space.
 * 
 * Creates a shard by extracting a portion of the Atlas space defined
 * by the boundary region. Shards can be used for distributed processing
 * or hierarchical decomposition of problems.
 * 
 * @param projection Source projection for extraction
 * @param region Boundary region defining shard extent
 * @return New shard handle, or NULL on error
 * 
 * Thread safety: Safe to call from multiple threads with different projections
 * Memory: Caller must call atlas_shard_destroy() to free resources
 */
atlas_shard_t atlas_shard_extract(const atlas_projection_t projection,
                                 const atlas_boundary_region_t* region);

/**
 * Extract multiple shards in parallel.
 * 
 * Efficiently extracts multiple shards from the same projection using
 * optimized parallel processing.
 * 
 * @param projection Source projection for extraction
 * @param regions Array of boundary regions
 * @param region_count Number of regions to extract
 * @param shards Output array for shard handles (allocated by caller)
 * @return Number of shards successfully extracted, or -1 on error
 * 
 * Thread safety: Safe to call from multiple threads with different projections
 * Memory: Caller must call atlas_shard_destroy() on each extracted shard
 */
int atlas_shard_extract_batch(const atlas_projection_t projection,
                             const atlas_boundary_region_t* regions,
                             size_t region_count,
                             atlas_shard_t* shards);

/**
 * Get size of shard data in bytes.
 * 
 * @param shard Shard handle (must be valid)
 * @return Size of shard data, or 0 on error
 * 
 * Thread safety: Safe to call concurrently (read-only operation)
 */
size_t atlas_shard_get_size(const atlas_shard_t shard);

/**
 * Copy shard data to output buffer.
 * 
 * @param shard Shard handle (must be valid)
 * @param buffer Output buffer (must be large enough)
 * @param buffer_size Size of output buffer
 * @return Number of bytes copied, or -1 on error
 * 
 * Thread safety: Safe to call concurrently (read-only operation)
 */
int atlas_shard_copy_data(const atlas_shard_t shard,
                         void* buffer, size_t buffer_size);

/**
 * Verify shard integrity and checksums.
 * 
 * @param shard Shard handle (must be valid)
 * @return true if shard is valid and intact, false otherwise
 * 
 * Thread safety: Safe to call concurrently (read-only operation)
 */
bool atlas_shard_verify(const atlas_shard_t shard);

/**
 * Destroy shard and free all associated resources.
 * 
 * @param shard Shard handle (can be NULL - no-op)
 * 
 * Thread safety: Not thread-safe (modifies and frees shard)
 * Memory: Frees all shard resources
 */
void atlas_shard_destroy(atlas_shard_t shard);

// =============================================================================
// Reconstruction Functions
// =============================================================================

/**
 * Initialize reconstruction context for combining shards.
 * 
 * Creates a context for reconstructing a projection from multiple shards.
 * The context tracks progress and maintains integrity checks during
 * the reconstruction process.
 * 
 * @param total_shards Expected number of shards to reconstruct
 * @return New reconstruction context, or invalid context on error
 * 
 * Thread safety: Safe to call from multiple threads
 */
atlas_reconstruction_ctx_t atlas_reconstruction_init(uint32_t total_shards);

/**
 * Add shard to reconstruction context.
 * 
 * Incorporates a shard into the ongoing reconstruction process.
 * Shards can be added in any order and the context will track
 * completion automatically.
 * 
 * @param ctx Pointer to reconstruction context
 * @param shard Shard to add to reconstruction
 * @return 0 on success, -1 on error
 * 
 * Thread safety: Not thread-safe (modifies context state)
 */
int atlas_reconstruction_add_shard(atlas_reconstruction_ctx_t* ctx,
                                  const atlas_shard_t shard);

/**
 * Check if reconstruction is complete and ready for finalization.
 * 
 * @param ctx Reconstruction context to check
 * @return true if all shards have been added, false otherwise
 * 
 * Thread safety: Safe to call concurrently (read-only operation)
 */
bool atlas_reconstruction_is_complete(const atlas_reconstruction_ctx_t* ctx);

/**
 * Finalize reconstruction and create resulting projection.
 * 
 * Completes the reconstruction process and creates a new projection
 * from the assembled shards. The reconstruction context is consumed
 * by this operation.
 * 
 * @param ctx Pointer to reconstruction context (will be invalidated)
 * @param type Desired projection type for result
 * @return New projection handle, or NULL on error
 * 
 * Thread safety: Not thread-safe (modifies and consumes context)
 * Memory: Caller must call atlas_projection_destroy() on result
 */
atlas_projection_t atlas_reconstruction_finalize(atlas_reconstruction_ctx_t* ctx,
                                                atlas_projection_type_t type);

// =============================================================================
// Verification Functions
// =============================================================================

/**
 * Verify manifold projection integrity and mathematical properties.
 * 
 * Performs comprehensive verification of a projection including:
 * - Data integrity checks
 * - Mathematical property validation  
 * - Conservation law compliance
 * - Topological invariant verification
 * 
 * @param projection Projection to verify
 * @return true if projection passes all verification checks, false otherwise
 * 
 * Thread safety: Safe to call concurrently (read-only operation)
 * Performance: O(n) where n is projection size
 */
bool atlas_manifold_verify_projection(const atlas_projection_t projection);

/**
 * Verify boundary region validity and properties.
 * 
 * Checks that a boundary region is well-formed and satisfies
 * the mathematical constraints required for manifold operations.
 * 
 * @param region Boundary region to verify
 * @return true if region is valid, false otherwise
 * 
 * Thread safety: Safe to call concurrently (read-only operation)
 */
bool atlas_manifold_verify_boundary_region(const atlas_boundary_region_t* region);

/**
 * Verify transformation parameters for mathematical validity.
 * 
 * Checks that transformation parameters are within valid ranges
 * and will not cause mathematical instabilities or invalid results.
 * 
 * @param params Transformation parameters to verify
 * @return true if parameters are valid, false otherwise
 * 
 * Thread safety: Safe to call concurrently (read-only operation)  
 */
bool atlas_manifold_verify_transform_params(const atlas_transform_params_t* params);

/**
 * Run comprehensive manifold system self-test.
 * 
 * Performs extensive testing of all manifold operations including:
 * - Projection creation and transformation
 * - Shard extraction and reconstruction
 * - Mathematical property verification
 * - Performance benchmarking
 * 
 * @return true if all tests pass, false if any test fails
 * 
 * Thread safety: Safe to call from multiple threads (creates temporary resources)
 * Performance: This is a comprehensive test that may take significant time
 */
bool atlas_manifold_system_test(void);

// =============================================================================
// Transform Operations
// =============================================================================

/**
 * Apply linear transformation to projection data.
 * 
 * Applies a linear transformation matrix to the projection space,
 * supporting operations like scaling, rotation, shearing, and reflection.
 * 
 * @param projection Projection to transform (modified in-place)
 * @param matrix 4x4 transformation matrix (row-major order)
 * @return 0 on success, -1 on error
 * 
 * Thread safety: Not thread-safe (modifies projection state)
 * Performance: O(n) where n is projection size
 */
int atlas_manifold_apply_linear_transform(atlas_projection_t projection,
                                         const double matrix[16]);

/**
 * Apply Fourier transform to R96 projection data.
 * 
 * Transforms R96 projection data into frequency domain using optimized
 * Fourier transforms specifically designed for Atlas resonance patterns.
 * 
 * @param projection R96_FOURIER projection to transform
 * @param inverse true for inverse transform, false for forward transform
 * @return 0 on success, -1 on error
 * 
 * Thread safety: Not thread-safe (modifies projection state)
 * Performance: O(n log n) where n is projection size
 */
int atlas_manifold_apply_fourier_transform(atlas_projection_t projection,
                                          bool inverse);

/**
 * Apply geometric scaling to projection coordinates.
 * 
 * Scales the projection space by the specified factors in each dimension.
 * Maintains aspect ratio if uniform scaling factors are used.
 * 
 * @param projection Projection to scale (modified in-place)
 * @param scale_x Scaling factor for X dimension (must be > 0)
 * @param scale_y Scaling factor for Y dimension (must be > 0)
 * @return 0 on success, -1 on error
 * 
 * Thread safety: Not thread-safe (modifies projection state)
 */
int atlas_manifold_scale_projection(atlas_projection_t projection,
                                   double scale_x, double scale_y);

/**
 * Rotate projection coordinates around center point.
 * 
 * Applies rotation transformation to projection space around the
 * specified center point by the given angle in radians.
 * 
 * @param projection Projection to rotate (modified in-place)
 * @param angle Rotation angle in radians
 * @param center_x X coordinate of rotation center
 * @param center_y Y coordinate of rotation center
 * @return 0 on success, -1 on error
 * 
 * Thread safety: Not thread-safe (modifies projection state)
 */
int atlas_manifold_rotate_projection(atlas_projection_t projection,
                                    double angle, double center_x, double center_y);

/**
 * Translate projection coordinates by offset vector.
 * 
 * Applies translation transformation to move the entire projection
 * space by the specified offset amounts.
 * 
 * @param projection Projection to translate (modified in-place)
 * @param offset_x Translation offset for X dimension
 * @param offset_y Translation offset for Y dimension
 * @return 0 on success, -1 on error
 * 
 * Thread safety: Not thread-safe (modifies projection state)
 */
int atlas_manifold_translate_projection(atlas_projection_t projection,
                                       double offset_x, double offset_y);

// =============================================================================
// Advanced Mathematical Operations
// =============================================================================

/**
 * Compute topological invariants for projection.
 * 
 * Calculates topological properties that remain unchanged under
 * continuous transformations, useful for classification and analysis.
 * 
 * @param projection Projection to analyze
 * @param invariants Output array for invariant values (allocated by caller)
 * @param max_invariants Maximum number of invariants to compute
 * @return Number of invariants computed, or -1 on error
 * 
 * Thread safety: Safe to call concurrently (read-only operation)
 * Performance: O(n²) where n is projection complexity
 */
int atlas_manifold_compute_invariants(const atlas_projection_t projection,
                                     double* invariants, size_t max_invariants);

/**
 * Calculate manifold curvature at specified coordinates.
 * 
 * Computes the local curvature of the manifold at the given point,
 * providing insight into the geometric properties of the space.
 * 
 * @param projection Projection containing the manifold
 * @param x X coordinate for curvature calculation
 * @param y Y coordinate for curvature calculation
 * @return Curvature value, or NaN on error
 * 
 * Thread safety: Safe to call concurrently (read-only operation)
 * Performance: O(1) with local neighborhood sampling
 */
double atlas_manifold_compute_curvature(const atlas_projection_t projection,
                                       double x, double y);

/**
 * Compute geodesic distance between two points on manifold.
 * 
 * Calculates the shortest path distance along the manifold surface
 * between two specified points, accounting for the manifold's geometry.
 * 
 * @param projection Projection containing the manifold
 * @param x1 X coordinate of first point
 * @param y1 Y coordinate of first point
 * @param x2 X coordinate of second point
 * @param y2 Y coordinate of second point
 * @return Geodesic distance, or negative value on error
 * 
 * Thread safety: Safe to call concurrently (read-only operation)
 * Performance: O(n) where n depends on manifold complexity
 */
double atlas_manifold_geodesic_distance(const atlas_projection_t projection,
                                       double x1, double y1, double x2, double y2);

/**
 * Find critical points in the manifold projection.
 * 
 * Locates points where the gradient vanishes, indicating local extrema,
 * saddle points, or other mathematically significant features.
 * 
 * @param projection Projection to analyze
 * @param critical_points Output array for critical point coordinates
 * @param max_points Maximum number of critical points to find
 * @return Number of critical points found, or -1 on error
 * 
 * Thread safety: Safe to call concurrently (read-only operation)
 * Memory: critical_points array must have space for 2*max_points doubles
 */
int atlas_manifold_find_critical_points(const atlas_projection_t projection,
                                       double* critical_points, size_t max_points);

// =============================================================================
// Runtime Information Functions
// =============================================================================

/**
 * Get manifold layer version information.
 * 
 * @return Version packed as (major << 16) | (minor << 8) | patch
 */
static inline uint32_t atlas_manifold_version(void) {
    return (ATLAS_MANIFOLD_VERSION_MAJOR << 16) |
           (ATLAS_MANIFOLD_VERSION_MINOR << 8) |
           (ATLAS_MANIFOLD_VERSION_PATCH);
}

/**
 * Check if manifold layer was compiled with optimization support.
 * 
 * @return true if optimized build, false if debug build
 */
bool atlas_manifold_is_optimized(void);

/**
 * Get supported projection types as bitmask.
 * 
 * Returns a bitmask indicating which projection types are available
 * in the current build configuration.
 * 
 * @return Bitmask of supported atlas_projection_type_t values
 */
uint32_t atlas_manifold_get_supported_projections(void);

/**
 * Get manifold layer performance statistics.
 * 
 * Returns runtime performance metrics for monitoring and optimization.
 * 
 * @param projections_created Output parameter for projection count
 * @param shards_extracted Output parameter for shard extraction count  
 * @param transforms_applied Output parameter for transform operation count
 * @return true on success, false on error
 * 
 * Thread safety: Safe to call concurrently (atomic counters)
 */
bool atlas_manifold_get_statistics(uint64_t* projections_created,
                                  uint64_t* shards_extracted,
                                  uint64_t* transforms_applied);

/**
 * Reset manifold layer performance statistics.
 * 
 * Thread safety: Safe to call concurrently (atomic operations)
 */
void atlas_manifold_reset_statistics(void);

#ifdef __cplusplus
}
#endif

#endif /* ATLAS_MANIFOLD_H */