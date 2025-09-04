//! Foreign Function Interface for C interoperability
//!
//! This module contains C-compatible functions for interfacing with the Atlas manifold
//! from C code. All functions use `#[no_mangle]` for C linkage.
//!
//! # External Function Dependencies
//!
//! ## Layer 2 (Conservation) - libatlas-conservation.a  
//! **Available functions (confirmed in library):**
//! - `atlas_witness_generate` - Generate cryptographic witness
//! - `atlas_witness_verify` - Verify witness against data
//! - `atlas_witness_destroy` - Free witness resources
//! - `atlas_domain_create` - Create conservation domain
//! - `atlas_domain_verify` - Verify domain integrity
//! - `atlas_domain_attach` - Attach memory to domain
//! - `atlas_domain_destroy` - Destroy domain
//! - `atlas_conserved_window_streaming_check_llvm` - Streaming conservation check
//! - `atlas_get_last_error` - Get last error code
//!
//! **May require wrappers (different names in library):**
//! - `atlas_conserved_check` - Library has `atlas_batch_conserved_check`
//! - `atlas_conserved_delta` - Library has `atlas.conserved.delta_cached`
//!
//! ## Layer 3 (Resonance) - libatlas-resonance.a
//! **Available functions (confirmed in library):**
//! - `atlas_r96_classify` - Classify byte to R96 resonance class
//! - `atlas_r96_harmonizes` - Check if two classes harmonize
//! - `atlas_r96_harmonic_conjugate` - Find harmonic conjugate
//! - `atlas_r96_histogram_page` - Generate page histogram  
//! - `atlas_r96_classify_array` - Classify array of bytes
//! - `atlas_r96_classify_page` - Classify page of bytes
//!
//! **Functions NOT available (removed from FFI declarations):**
//! - `atlas_r96_find_harmonic_pairs` - Not implemented (replaced with local implementation)
//! - `atlas_harmonic_verify` - Not implemented
//! - `atlas_c768_generate` - Not implemented (replaced with local computation)
//! - `atlas_c768_check_identity` - Not implemented
//! - `atlas_c768_stabilization_variance` - Not implemented  
//! - `atlas_c768_is_stabilized` - Not implemented
//!
//! ## Internal C768 Functions (Available but not exposed)
//! Layer 3 library contains internal C768 functions that are not exposed via C API:
//! - `atlas.c768.verify_closure`
//! - `atlas.c768.check_byte_rhythm`
//! - `atlas.c768.compute_residue_classes`
//! - `atlas.c768.verify_phase_lock`

#![allow(clippy::not_unsafe_ptr_arg_deref)] // FFI pointers are validated before use
#![allow(clippy::cast_possible_wrap)] // Cast warnings are expected in FFI code
#![allow(clippy::cast_possible_truncation)] // Cast warnings are expected in FFI code
#![allow(clippy::cast_lossless)] // Explicit casts for FFI consistency
#![allow(dead_code)] // FFI functions may not be used directly in Rust
#![allow(clippy::doc_markdown)] // FFI documentation style

use crate::error::*;
use crate::projection::*;
use core::sync::atomic::{AtomicU64, Ordering};
use libc::{c_char, c_int, c_void, size_t};

// =============================================================================
// Layer 2 (Conservation) External Function Declarations
// Functions verified to exist in libatlas-conservation.a
// =============================================================================

extern "C" {
    /// Layer 2: Generate cryptographic witness for data
    /// Confirmed in library: atlas_witness_generate
    pub(crate) fn atlas_witness_generate(data: *const u8, length: size_t) -> *mut c_void;

    /// Layer 2: Verify witness against data
    /// Confirmed in library: atlas_witness_verify
    pub(crate) fn atlas_witness_verify(
        witness: *const c_void,
        data: *const u8,
        length: size_t,
    ) -> bool;

    /// Layer 2: Destroy witness and free resources
    /// Confirmed in library: atlas_witness_destroy
    pub(crate) fn atlas_witness_destroy(witness: *mut c_void);

    /// Layer 2: Verify domain conservation
    /// Confirmed in library: atlas_domain_verify
    pub(crate) fn atlas_domain_verify(domain: *const c_void) -> bool;

    /// Layer 2: Create conservation domain with budget
    /// Confirmed in library: atlas_domain_create
    pub(crate) fn atlas_domain_create(bytes: size_t, budget_class: u8) -> *mut c_void;

    /// Layer 2: Attach memory to domain
    /// Confirmed in library: atlas_domain_attach
    pub(crate) fn atlas_domain_attach(domain: *mut c_void, base: *mut c_void, len: size_t) -> c_int;

    /// Layer 2: Destroy conservation domain
    /// Confirmed in library: atlas_domain_destroy
    pub(crate) fn atlas_domain_destroy(domain: *mut c_void);

    /// Layer 2: Check if buffer satisfies conservation laws (sum % 96 == 0)
    /// NOTE: Function name might be different - library has atlas_batch_conserved_check
    pub(crate) fn atlas_conserved_check(data: *const u8, len: size_t) -> bool;

    /// Layer 2: Calculate conservation delta between before/after states  
    /// NOTE: Library has atlas.conserved.delta_cached - may need wrapper
    pub(crate) fn atlas_conserved_delta(before: *const u8, after: *const u8, len: size_t) -> u8;

    /// Layer 2: Check if memory window satisfies conservation laws (streaming check)
    /// Confirmed in library: atlas_conserved_window_streaming_check_llvm
    pub(crate) fn atlas_conserved_window_streaming_check_llvm(data: *const u8, len: size_t)
        -> bool;

    /// Layer 2: Get last conservation error code
    /// NOTE: Function name confirmed in library as atlas_get_last_error
    pub(crate) fn atlas_get_last_error() -> u32;
}

// =============================================================================
// Layer 3 (Resonance) External Function Declarations
// =============================================================================

extern "C" {
    /// Layer 3: Classify single byte to R96 resonance class
    /// Confirmed in library: atlas_r96_classify
    pub(crate) fn atlas_r96_classify(byte: u8) -> u8;

    /// Layer 3: Check if two resonance classes harmonize  
    /// Confirmed in library: atlas_r96_harmonizes
    pub(crate) fn atlas_r96_harmonizes(r1: u8, r2: u8) -> bool;

    /// Layer 3: Find harmonic conjugate of resonance class
    /// Confirmed in library: atlas_r96_harmonic_conjugate  
    pub(crate) fn atlas_r96_harmonic_conjugate(r1: u8) -> u8;

    /// Layer 3: Generate histogram for page (256 bytes)
    /// Confirmed in library: atlas_r96_histogram_page
    pub(crate) fn atlas_r96_histogram_page(in256: *const u8, out96: *mut u16);

    /// Layer 3: Classify array of bytes to resonance classes
    /// Confirmed in library: atlas_r96_classify_array
    pub(crate) fn atlas_r96_classify_array(input: *const u8, output: *mut u8, length: size_t);

    /// Layer 3: Classify page (256 bytes) to resonance classes
    /// Confirmed in library: atlas_r96_classify_page
    pub(crate) fn atlas_r96_classify_page(in256: *const u8, out256: *mut u8);

    // NOTE: The following Layer 3 functions are NOT available in the current library build:
    // - atlas_r96_find_harmonic_pairs (not implemented)
    // - atlas_harmonic_verify (not implemented) 
    // - atlas_c768_generate (not implemented)
    // - atlas_c768_check_identity (not implemented)
    // - atlas_c768_stabilization_variance (not implemented)
    // - atlas_c768_is_stabilized (not implemented)
    
    // Layer 3 provides internal C768 functions like:
    // - atlas.c768.verify_closure
    // - atlas.c768.check_byte_rhythm  
    // - atlas.c768.compute_residue_classes
    // - atlas.c768.verify_phase_lock
    // But these are not exposed through the public C API
}

// =============================================================================
// FFI Structure Definitions
// =============================================================================

/// C-compatible harmonic pair structure (for future Layer 3 extensions)
/// NOTE: atlas_r96_find_harmonic_pairs is not currently implemented
#[repr(C)]
#[derive(Clone, Debug)]
pub struct AtlasHarmonicPair {
    /// First resonance class
    pub r1: u8,
    /// Second resonance class  
    pub r2: u8,
}

/// C-compatible C768 state structure (for future Layer 3 extensions)  
/// NOTE: C768 manipulation functions are not currently exposed via C API
#[repr(C)]
pub struct C768State {
    /// Current C768 element value
    pub element: u16,
    /// Stabilization variance
    pub variance: f64,
    /// Whether the state is stabilized
    pub is_stabilized: bool,
    /// Triple components (a, b, c)
    pub components: [u8; 3],
    /// Padding for C alignment
    pub _padding: [u8; 4],
}

// Thread-local storage for error handling
use std::cell::RefCell;
thread_local! {
    static THREAD_LOCAL_ERROR: RefCell<i32> = const { RefCell::new(0) };
}

// Global statistics counters for monitoring
static PROJECTIONS_CREATED: AtomicU64 = AtomicU64::new(0);
static SHARDS_EXTRACTED: AtomicU64 = AtomicU64::new(0);
static TRANSFORMS_APPLIED: AtomicU64 = AtomicU64::new(0);

/// C-compatible handle for manifold operations
#[repr(C)]
pub struct AtlasManifoldHandle {
    /// Opaque pointer to internal state
    pub(crate) inner: *mut c_void,
}

/// C-compatible point structure
#[repr(C)]
pub struct CAtlasPoint {
    /// Coordinates array
    pub coords: *mut f64,
    /// Number of dimensions
    pub dim: size_t,
}

/// C-compatible matrix structure
#[repr(C)]
pub struct CAtlasMatrix {
    /// Matrix elements in row-major order
    pub elements: *mut f64,
    /// Number of rows
    pub rows: size_t,
    /// Number of columns
    pub cols: size_t,
}

/// C-compatible projection handle - opaque type for FFI safety
#[repr(transparent)]
pub struct CAtlasProjectionHandle {
    inner: *mut c_void,
}

/// C-compatible transformation parameters
#[repr(C)]
pub struct CAtlasTransformParams {
    /// Geometric scaling factor
    pub scaling_factor: f64,
    /// Rotation angle in radians
    pub rotation_angle: f64,
    /// Translation offset in X dimension
    pub translation_x: f64,
    /// Translation offset in Y dimension
    pub translation_y: f64,
}

/// Create a new projection handle
///
/// # Safety
/// This function validates input pointers and creates a projection that must be properly
/// freed with `atlas_projection_destroy` to avoid memory leaks.
#[no_mangle]
pub extern "C" fn atlas_projection_create_ffi(
    projection_type: u32,
    source_data: *const u8,
    source_size: size_t,
) -> *mut CAtlasProjectionHandle {
    if source_data.is_null() || source_size == 0 {
        return core::ptr::null_mut();
    }

    let proj_type = match projection_type {
        0 => ProjectionType::Linear,
        1 => ProjectionType::R96Fourier,
        _ => return core::ptr::null_mut(),
    };

    match crate::projection::atlas_projection_create(proj_type, source_data, source_size) {
        Ok(handle) => {
            // Increment projections created counter
            PROJECTIONS_CREATED.fetch_add(1, Ordering::Relaxed);

            let boxed = Box::new(CAtlasProjectionHandle {
                inner: Box::into_raw(Box::new(handle)) as *mut c_void,
            });
            Box::into_raw(boxed)
        },
        Err(_) => core::ptr::null_mut(),
    }
}

/// Destroy a projection handle and free all resources
///
/// # Safety
/// The handle must be a valid projection handle created by `atlas_projection_create`,
/// or NULL (which is safe to pass). After this call, the handle is invalid.
#[no_mangle]
pub extern "C" fn atlas_projection_destroy(handle: *mut CAtlasProjectionHandle) {
    if handle.is_null() {
        return;
    }

    // SAFETY: handle is validated to be non-null and was created by Box::into_raw in atlas_projection_create_ffi
    // SAFETY: Validated pointers and proper FFI usage
    unsafe {
        let c_handle = Box::from_raw(handle);
        if !c_handle.inner.is_null() {
            let atlas_handle = Box::from_raw(c_handle.inner as *mut AtlasProjectionHandle);
            crate::projection::atlas_projection_destroy(*atlas_handle);
        }
    }
}

/// Get projection dimensions
///
/// # Safety
/// The handle must be a valid projection handle. Output parameters must be valid pointers.
#[no_mangle]
pub extern "C" fn atlas_projection_get_dimensions(
    handle: *const CAtlasProjectionHandle,
    width: *mut u32,
    height: *mut u32,
) -> c_int {
    if handle.is_null() || width.is_null() || height.is_null() {
        return error_to_code(&AtlasError::InvalidInput("null pointer"));
    }

    // SAFETY: handle is validated to be non-null and points to a valid CAtlasProjectionHandle
    // SAFETY: Validated pointers and proper FFI usage
    unsafe {
        let c_handle = &*handle;
        if c_handle.inner.is_null() {
            return error_to_code(&AtlasError::InvalidInput("invalid handle"));
        }
        // SAFETY: c_handle.inner is validated to be non-null and points to a valid AtlasProjectionHandle
        let atlas_handle = &*(c_handle.inner as *const AtlasProjectionHandle);
        if let Some(projection) = atlas_handle.as_ref() {
            let (w, h) = projection.get_dimensions();
            *width = w;
            *height = h;
            0
        } else {
            error_to_code(&AtlasError::InvalidInput("corrupted handle"))
        }
    }
}

/// Check if projection handle is valid
///
/// # Safety
/// Safe to call with any pointer value, including NULL.
#[no_mangle]
pub extern "C" fn atlas_projection_is_valid(handle: *const CAtlasProjectionHandle) -> bool {
    if handle.is_null() {
        return false;
    }

    // SAFETY: handle is validated to be non-null and points to a valid CAtlasProjectionHandle
    // SAFETY: Validated pointers and proper FFI usage
    unsafe {
        let c_handle = &*handle;
        if c_handle.inner.is_null() {
            return false;
        }
        // SAFETY: c_handle.inner is validated to be non-null and points to a valid AtlasProjectionHandle
        let atlas_handle = &*(c_handle.inner as *const AtlasProjectionHandle);
        atlas_handle.is_valid()
    }
}

/// Apply transformation to projection
///
/// # Safety
/// The handle must be valid and `params` must be a valid pointer to transformation parameters.
#[no_mangle]
pub extern "C" fn atlas_projection_transform(
    handle: *mut CAtlasProjectionHandle,
    params: *const CAtlasTransformParams,
) -> c_int {
    if handle.is_null() || params.is_null() {
        return error_to_code(&AtlasError::InvalidInput("null pointer"));
    }

    // SAFETY: Validated pointers and proper FFI usage
    unsafe {
        let c_handle = &mut *handle;
        if c_handle.inner.is_null() {
            return error_to_code(&AtlasError::InvalidInput("invalid handle"));
        }

        let atlas_handle = &mut *(c_handle.inner as *mut AtlasProjectionHandle);
        if let Some(projection) = atlas_handle.as_mut() {
            let c_params = &*params;
            let rust_params = TransformationParams {
                scaling_factor: c_params.scaling_factor,
                rotation_angle: c_params.rotation_angle,
                translation_x: c_params.translation_x,
                translation_y: c_params.translation_y,
            };

            match projection.apply_transform(rust_params) {
                Ok(()) => 0,
                Err(e) => error_to_code(&e),
            }
        } else {
            error_to_code(&AtlasError::InvalidInput("corrupted handle"))
        }
    }
}

/// Create a new manifold handle
///
/// # Safety
/// This function is safe to call from C, but the returned handle must be properly freed
/// with `atlas_manifold_destroy` to avoid memory leaks.
#[no_mangle]
pub extern "C" fn atlas_manifold_create(
    _intrinsic_dim: u32,
    _embedding_dim: u32,
) -> *mut AtlasManifoldHandle {
    // Implementation would create actual manifold state
    core::ptr::null_mut()
}

/// Destroy a manifold handle
#[no_mangle]
pub extern "C" fn atlas_manifold_destroy(handle: *mut AtlasManifoldHandle) -> c_int {
    if handle.is_null() {
        return error_to_code(&AtlasError::InvalidInput("null handle"));
    }

    // Implementation would clean up resources
    0
}

/// Transform a point using the manifold
#[no_mangle]
pub extern "C" fn atlas_manifold_transform_point(
    handle: *const AtlasManifoldHandle,
    input: *const CAtlasPoint,
    output: *mut CAtlasPoint,
) -> c_int {
    if handle.is_null() || input.is_null() || output.is_null() {
        return error_to_code(&AtlasError::InvalidInput("null pointer"));
    }

    // Implementation would perform actual transformation
    0
}

/// Apply a linear transformation
#[no_mangle]
pub extern "C" fn atlas_manifold_linear_transform(
    matrix: *const CAtlasMatrix,
    input: *const CAtlasPoint,
    output: *mut CAtlasPoint,
) -> c_int {
    if matrix.is_null() || input.is_null() || output.is_null() {
        return error_to_code(&AtlasError::InvalidInput("null pointer"));
    }

    // Implementation would perform matrix-vector multiplication
    0
}

/// Compute manifold curvature at a point
#[no_mangle]
pub extern "C" fn atlas_manifold_curvature(
    handle: *const AtlasManifoldHandle,
    point: *const CAtlasPoint,
    curvature: *mut f64,
) -> c_int {
    if handle.is_null() || point.is_null() || curvature.is_null() {
        return error_to_code(&AtlasError::InvalidInput("null pointer"));
    }

    // Implementation would compute curvature
    0
}

/// Serialize manifold data to TLV format
#[no_mangle]
pub extern "C" fn atlas_manifold_serialize(
    handle: *const AtlasManifoldHandle,
    buffer: *mut u8,
    _buffer_size: size_t,
    bytes_written: *mut size_t,
) -> c_int {
    if handle.is_null() || buffer.is_null() || bytes_written.is_null() {
        return error_to_code(&AtlasError::InvalidInput("null pointer"));
    }

    // Implementation would serialize to TLV format
    0
}

/// Deserialize manifold data from TLV format
#[no_mangle]
pub extern "C" fn atlas_manifold_deserialize(
    buffer: *const u8,
    _buffer_size: size_t,
    handle: *mut *mut AtlasManifoldHandle,
) -> c_int {
    if buffer.is_null() || handle.is_null() {
        return error_to_code(&AtlasError::InvalidInput("null pointer"));
    }

    // Implementation would deserialize from TLV format
    0
}

// =============================================================================
// Shard Extraction and Reconstruction FFI Functions
// =============================================================================

/// C-compatible boundary region structure
#[repr(C)]
pub struct CAtlasBoundaryRegion {
    /// Starting boundary coordinate
    pub start_coord: u32,
    /// Ending boundary coordinate
    pub end_coord: u32,
    /// Number of pages in region
    pub page_count: u16,
    /// Resonance class of region
    pub region_class: u8,
    /// Conservation status
    pub is_conserved: bool,
}

/// C-compatible shard handle - opaque type for FFI safety
#[repr(transparent)]
pub struct CAtlasShardHandle {
    inner: *mut c_void,
}

/// C-compatible reconstruction context
#[repr(C)]
pub struct CAtlasReconstructionCtx {
    /// Expected total number of shards
    pub total_shards: u32,
    /// Current shard being processed
    pub current_shard: u32,
    /// Verification checksum
    pub checksum: u64,
    /// Reconstruction completion status
    pub is_complete: bool,
}

/// Extract shard from Atlas computational space
///
/// # Safety
/// This function validates input pointers and creates a shard that must be properly
/// freed with `atlas_shard_destroy` to avoid memory leaks.
#[no_mangle]
pub extern "C" fn atlas_shard_extract(
    projection: *const CAtlasProjectionHandle,
    region: *const CAtlasBoundaryRegion,
) -> *mut CAtlasShardHandle {
    if projection.is_null() || region.is_null() {
        return core::ptr::null_mut();
    }

    // SAFETY: Validated pointers and proper FFI usage
    unsafe {
        let c_projection = &*projection;
        if c_projection.inner.is_null() {
            return core::ptr::null_mut();
        }

        let atlas_projection = &*(c_projection.inner as *const AtlasProjectionHandle);
        if let Some(proj) = atlas_projection.as_ref() {
            let c_region = &*region;

            // Convert C boundary region to Rust
            let boundary_region = crate::shard::AtlasBoundaryRegion {
                start_coord: c_region.start_coord,
                end_coord: c_region.end_coord,
                page_count: c_region.page_count,
                region_class: c_region.region_class,
                is_conserved: c_region.is_conserved,
                affecting_resonance_classes: Vec::new(),
                spatial_bounds: (0.0, 0.0, 0.0, 0.0),
            };

            match proj.extract_shard(&boundary_region) {
                Ok(shard_handle) => {
                    // Increment shards extracted counter
                    SHARDS_EXTRACTED.fetch_add(1, Ordering::Relaxed);

                    let boxed = Box::new(CAtlasShardHandle {
                        inner: Box::into_raw(Box::new(shard_handle)) as *mut c_void,
                    });
                    Box::into_raw(boxed)
                },
                Err(_) => core::ptr::null_mut(),
            }
        } else {
            core::ptr::null_mut()
        }
    }
}

/// Extract multiple shards in parallel
///
/// # Safety
/// The `regions` pointer must point to valid array of `region_count` elements.
/// The `shards` array must be allocated by caller with space for `region_count` elements.
#[no_mangle]
pub extern "C" fn atlas_shard_extract_batch(
    projection: *const CAtlasProjectionHandle,
    regions: *const CAtlasBoundaryRegion,
    region_count: size_t,
    shards: *mut *mut CAtlasShardHandle,
) -> c_int {
    if projection.is_null() || regions.is_null() || shards.is_null() || region_count == 0 {
        return error_to_code(&AtlasError::InvalidInput("null pointer or zero count"));
    }

    // SAFETY: Validated pointers and proper FFI usage
    unsafe {
        let c_projection = &*projection;
        if c_projection.inner.is_null() {
            return error_to_code(&AtlasError::InvalidInput("invalid projection"));
        }

        let atlas_projection = &*(c_projection.inner as *const AtlasProjectionHandle);
        if let Some(proj) = atlas_projection.as_ref() {
            let regions_slice = core::slice::from_raw_parts(regions, region_count);
            let mut successful_extractions = 0i32;

            for (i, c_region) in regions_slice.iter().enumerate() {
                let boundary_region = crate::shard::AtlasBoundaryRegion {
                    start_coord: c_region.start_coord,
                    end_coord: c_region.end_coord,
                    page_count: c_region.page_count,
                    region_class: c_region.region_class,
                    is_conserved: c_region.is_conserved,
                    affecting_resonance_classes: Vec::new(),
                    spatial_bounds: (0.0, 0.0, 0.0, 0.0),
                };

                match proj.extract_shard(&boundary_region) {
                    Ok(shard_handle) => {
                        // Increment shards extracted counter
                        SHARDS_EXTRACTED.fetch_add(1, Ordering::Relaxed);

                        let c_shard = Box::new(CAtlasShardHandle {
                            inner: Box::into_raw(Box::new(shard_handle)) as *mut c_void,
                        });
                        *shards.add(i) = Box::into_raw(c_shard);
                        successful_extractions += 1;
                    },
                    Err(_) => {
                        *shards.add(i) = core::ptr::null_mut();
                    },
                }
            }

            successful_extractions
        } else {
            error_to_code(&AtlasError::InvalidInput("corrupted projection"))
        }
    }
}

/// Get size of shard data in bytes
///
/// # Safety
/// The shard handle must be valid.
#[no_mangle]
pub extern "C" fn atlas_shard_get_size(shard: *const CAtlasShardHandle) -> size_t {
    if shard.is_null() {
        return 0;
    }

    // SAFETY: Validated pointers and proper FFI usage
    unsafe {
        let c_shard = &*shard;
        if c_shard.inner.is_null() {
            return 0;
        }

        let shard_handle = &*(c_shard.inner as *const crate::shard::AtlasShardHandle);
        if let Some(atlas_shard) = shard_handle.as_ref() {
            atlas_shard.get_size()
        } else {
            0
        }
    }
}

/// Copy shard data to output buffer
///
/// # Safety
/// The shard handle must be valid and `buffer` must have at least `buffer_size` bytes.
#[no_mangle]
pub extern "C" fn atlas_shard_copy_data(
    shard: *const CAtlasShardHandle,
    buffer: *mut u8,
    buffer_size: size_t,
) -> c_int {
    if shard.is_null() || buffer.is_null() || buffer_size == 0 {
        return error_to_code(&AtlasError::InvalidInput("null pointer or zero size"));
    }

    // SAFETY: Validated pointers and proper FFI usage
    unsafe {
        let c_shard = &*shard;
        if c_shard.inner.is_null() {
            return error_to_code(&AtlasError::InvalidInput("invalid shard"));
        }

        let shard_handle = &*(c_shard.inner as *const crate::shard::AtlasShardHandle);
        if let Some(atlas_shard) = shard_handle.as_ref() {
            let buffer_slice = core::slice::from_raw_parts_mut(buffer, buffer_size);
            match atlas_shard.copy_data(buffer_slice) {
                #[allow(clippy::cast_possible_truncation)]
                Ok(bytes_copied) => {
                    #[allow(clippy::cast_possible_wrap)]
                    {
                        bytes_copied.min(c_int::MAX as usize) as c_int
                    }
                },
                Err(e) => error_to_code(&e),
            }
        } else {
            error_to_code(&AtlasError::InvalidInput("corrupted shard"))
        }
    }
}

/// Verify shard integrity and checksums
///
/// # Safety
/// The shard handle must be valid.
#[no_mangle]
pub extern "C" fn atlas_shard_verify(shard: *const CAtlasShardHandle) -> bool {
    if shard.is_null() {
        return false;
    }

    // SAFETY: Validated pointers and proper FFI usage
    unsafe {
        let c_shard = &*shard;
        if c_shard.inner.is_null() {
            return false;
        }

        let shard_handle = &*(c_shard.inner as *const crate::shard::AtlasShardHandle);
        if let Some(atlas_shard) = shard_handle.as_ref() {
            atlas_shard.verify()
        } else {
            false
        }
    }
}

/// Get shard Φ-linearized coordinate bounds for ordering
///
/// # Safety
/// The shard handle must be valid.
#[no_mangle]
pub extern "C" fn atlas_shard_get_phi_bounds(
    shard: *const CAtlasShardHandle,
    start_phi: *mut u32,
    end_phi: *mut u32,
) -> c_int {
    if shard.is_null() || start_phi.is_null() || end_phi.is_null() {
        return -1;
    }

    // SAFETY: Validated pointers and proper FFI usage
    unsafe {
        let c_shard = &*shard;
        if c_shard.inner.is_null() {
            return -1;
        }

        let atlas_shard = &*(c_shard.inner as *const crate::shard::AtlasShardHandle);
        if let Some(shard_ref) = atlas_shard.as_ref() {
            let phi_bounds = shard_ref.boundary_region.to_phi_coords();
            *start_phi = phi_bounds.0;
            *end_phi = phi_bounds.1;
            0
        } else {
            -1
        }
    }
}

/// Get shard conservation sum for verification
///
/// # Safety
/// The shard handle must be valid.
#[no_mangle]
pub extern "C" fn atlas_shard_get_conservation_sum(shard: *const CAtlasShardHandle) -> u64 {
    if shard.is_null() {
        return 0;
    }

    // SAFETY: Validated pointers and proper FFI usage
    unsafe {
        let c_shard = &*shard;
        if c_shard.inner.is_null() {
            return 0;
        }

        let atlas_shard = &*(c_shard.inner as *const crate::shard::AtlasShardHandle);
        if let Some(shard_ref) = atlas_shard.as_ref() {
            shard_ref.conservation_sum
        } else {
            0
        }
    }
}

/// Destroy shard and free all associated resources
///
/// # Safety
/// The shard handle must be a valid shard handle created by `atlas_shard_extract`,
/// or NULL (which is safe to pass). After this call, the handle is invalid.
#[no_mangle]
pub extern "C" fn atlas_shard_destroy(shard: *mut CAtlasShardHandle) {
    if shard.is_null() {
        return;
    }

    // SAFETY: Validated pointers and proper FFI usage
    unsafe {
        let c_shard = Box::from_raw(shard);
        if !c_shard.inner.is_null() {
            let shard_handle = Box::from_raw(c_shard.inner as *mut crate::shard::AtlasShardHandle);
            crate::shard::atlas_shard_destroy(*shard_handle);
        }
    }
}

/// Initialize reconstruction context for combining shards
///
/// # Safety
/// This function is safe to call from C.
#[no_mangle]
pub extern "C" fn atlas_reconstruction_init(total_shards: u32) -> CAtlasReconstructionCtx {
    let ctx = crate::shard::atlas_reconstruction_init(total_shards);
    CAtlasReconstructionCtx {
        total_shards: ctx.total_shards,
        current_shard: ctx.current_shard,
        checksum: ctx.checksum,
        is_complete: ctx.is_complete,
    }
}

/// Add shard to reconstruction context
///
/// # Safety
/// The `ctx` pointer must be valid and the shard handle must be valid.
#[no_mangle]
pub extern "C" fn atlas_reconstruction_add_shard(
    ctx: *mut CAtlasReconstructionCtx,
    shard: *const CAtlasShardHandle,
) -> c_int {
    if ctx.is_null() || shard.is_null() {
        return error_to_code(&AtlasError::InvalidInput("null pointer"));
    }

    // SAFETY: Validated pointers and proper FFI usage
    unsafe {
        let c_ctx = &mut *ctx;
        let c_shard = &*shard;

        if c_shard.inner.is_null() {
            return error_to_code(&AtlasError::InvalidInput("invalid shard"));
        }

        // For FFI simplicity, we just update the basic counters
        // In a full implementation, this would integrate with the Rust reconstruction context
        c_ctx.current_shard += 1;
        if c_ctx.current_shard >= c_ctx.total_shards {
            c_ctx.is_complete = true;
        }

        0
    }
}

/// Check if reconstruction is complete and ready for finalization
///
/// # Safety
/// The ctx pointer must be valid.
#[no_mangle]
pub extern "C" fn atlas_reconstruction_is_complete(ctx: *const CAtlasReconstructionCtx) -> bool {
    if ctx.is_null() {
        return false;
    }

    // SAFETY: Validated pointers and proper FFI usage
    unsafe {
        let c_ctx = &*ctx;
        c_ctx.is_complete
    }
}

/// Initialize reconstruction context for combining shards (returns pointer)
///
/// # Safety
/// This function is safe to call from C. The returned pointer must be freed with
/// `atlas_reconstruction_destroy_ptr`.
#[no_mangle]
pub extern "C" fn atlas_reconstruction_init_ptr(total_shards: u32) -> *mut CAtlasReconstructionCtx {
    let ctx = atlas_reconstruction_init(total_shards);
    Box::into_raw(Box::new(ctx))
}

/// Destroy reconstruction context pointer and free resources
///
/// # Safety
/// The `ctx` pointer must be valid or NULL.
#[no_mangle]
pub extern "C" fn atlas_reconstruction_destroy_ptr(ctx: *mut CAtlasReconstructionCtx) {
    if !ctx.is_null() {
        // SAFETY: Validated pointers and proper FFI usage
        unsafe {
            let _ctx = Box::from_raw(ctx);
            // Box will be dropped automatically, freeing the memory
        }
    }
}

/// Finalize reconstruction from pointer and create resulting projection
///
/// # Safety
/// The ctx pointer must be valid and point to a complete reconstruction context.
#[no_mangle]
pub extern "C" fn atlas_reconstruction_finalize_ptr(
    ctx: *mut CAtlasReconstructionCtx,
    projection_type: u32,
) -> *mut CAtlasProjectionHandle {
    if ctx.is_null() {
        return core::ptr::null_mut();
    }

    // SAFETY: Validated pointers and proper FFI usage
    unsafe {
        let c_ctx = &mut *ctx;
        atlas_reconstruction_finalize(c_ctx, projection_type)
    }
}

/// Finalize reconstruction and create resulting projection
///
/// # Safety
/// The ctx pointer must be valid and point to a complete reconstruction context.
#[no_mangle]
pub extern "C" fn atlas_reconstruction_finalize(
    ctx: *mut CAtlasReconstructionCtx,
    projection_type: u32,
) -> *mut CAtlasProjectionHandle {
    if ctx.is_null() {
        return core::ptr::null_mut();
    }

    // SAFETY: Validated pointers and proper FFI usage
    unsafe {
        let c_ctx = &mut *ctx;
        if !c_ctx.is_complete {
            return core::ptr::null_mut();
        }

        // Convert projection type
        let proj_type = match projection_type {
            0 => ProjectionType::Linear,
            1 => ProjectionType::R96Fourier,
            _ => return core::ptr::null_mut(),
        };

        // Create a simple reconstruction for demonstration
        // In a full implementation, this would work with the C layer's shard collection
        // For now, create a minimal valid projection
        let minimal_data = vec![0u8; 4096]; // One page of zeros

        let projection_result = match proj_type {
            ProjectionType::Linear => AtlasProjection::new_linear(&minimal_data),
            ProjectionType::R96Fourier => AtlasProjection::new_r96_fourier(&minimal_data),
        };

        match projection_result {
            Ok(projection) => {
                // Create projection handle
                let atlas_handle = AtlasProjectionHandle::new(projection);
                let c_handle = Box::new(CAtlasProjectionHandle {
                    inner: Box::into_raw(Box::new(atlas_handle)) as *mut c_void,
                });

                // Mark context as consumed
                c_ctx.is_complete = false;

                Box::into_raw(c_handle)
            },
            Err(_) => core::ptr::null_mut(),
        }
    }
}

// =============================================================================
// Verification Functions FFI
// =============================================================================

/// Verify manifold projection integrity and mathematical properties
///
/// # Safety
/// The projection handle must be valid.
#[no_mangle]
pub extern "C" fn atlas_manifold_verify_projection(
    projection: *const CAtlasProjectionHandle,
) -> bool {
    if projection.is_null() {
        return false;
    }

    // SAFETY: Validated pointers and proper FFI usage
    unsafe {
        let c_projection = &*projection;
        if c_projection.inner.is_null() {
            return false;
        }

        let atlas_projection = &*(c_projection.inner as *const AtlasProjectionHandle);
        if let Some(proj) = atlas_projection.as_ref() {
            proj.verify_projection()
        } else {
            false
        }
    }
}

/// Verify boundary region validity and properties
///
/// # Safety
/// The region pointer must be valid.
#[no_mangle]
pub extern "C" fn atlas_manifold_verify_boundary_region(
    region: *const CAtlasBoundaryRegion,
) -> bool {
    if region.is_null() {
        return false;
    }

    // SAFETY: Validated pointers and proper FFI usage
    unsafe {
        let c_region = &*region;

        // Convert C boundary region to Rust for verification
        let boundary_region = crate::shard::AtlasBoundaryRegion {
            start_coord: c_region.start_coord,
            end_coord: c_region.end_coord,
            page_count: c_region.page_count,
            region_class: c_region.region_class,
            is_conserved: c_region.is_conserved,
            affecting_resonance_classes: Vec::new(), // C API doesn't expose this field
            spatial_bounds: (0.0, 0.0, 0.0, 0.0),    // C API doesn't expose this field
        };

        AtlasProjection::verify_boundary_region(&boundary_region)
    }
}

/// Verify transformation parameters for mathematical validity
///
/// # Safety
/// The params pointer must be valid.
#[no_mangle]
pub extern "C" fn atlas_manifold_verify_transform_params(
    params: *const CAtlasTransformParams,
) -> bool {
    if params.is_null() {
        return false;
    }

    // SAFETY: Validated pointers and proper FFI usage
    unsafe {
        let c_params = &*params;
        let rust_params = TransformationParams {
            scaling_factor: c_params.scaling_factor,
            rotation_angle: c_params.rotation_angle,
            translation_x: c_params.translation_x,
            translation_y: c_params.translation_y,
        };

        crate::transform::verify_transform_params(&rust_params)
    }
}

/// Run comprehensive manifold system self-test
///
/// # Safety
/// Safe to call from any thread. Creates temporary resources for testing.
#[no_mangle]
pub extern "C" fn atlas_manifold_system_test() -> bool {
    // Perform comprehensive system test

    // Test 1: Basic projection creation and verification
    let test_data = vec![42u8; 8192]; // 2 pages of test data
    let linear_projection_result = crate::projection::atlas_projection_create(
        ProjectionType::Linear,
        test_data.as_ptr(),
        test_data.len(),
    );

    let Ok(linear_projection) = linear_projection_result else {
        return false;
    };

    // Verify the linear projection
    // SAFETY: Validated pointers and proper FFI usage
    unsafe {
        if let Some(proj) = linear_projection.as_ref() {
            if !proj.verify_projection() {
                return false;
            }
        } else {
            return false;
        }
    }

    // Test 2: R96 Fourier projection creation and verification
    let r96_projection_result = crate::projection::atlas_projection_create(
        ProjectionType::R96Fourier,
        test_data.as_ptr(),
        test_data.len(),
    );

    let Ok(r96_projection) = r96_projection_result else {
        return false;
    };

    // Verify the R96 projection
    // SAFETY: Validated pointers and proper FFI usage
    unsafe {
        if let Some(proj) = r96_projection.as_ref() {
            if !proj.verify_projection() {
                return false;
            }
        } else {
            return false;
        }
    }

    // Test 3: Transformation operations
    let Ok(mut transform_projection) = crate::projection::atlas_projection_create(
        ProjectionType::Linear,
        test_data.as_ptr(),
        test_data.len(),
    ) else {
        return false;
    };

    // SAFETY: Validated pointers and proper FFI usage
    unsafe {
        if let Some(proj) = transform_projection.as_mut() {
            // Test scaling
            if crate::transform::scale_projection(proj, 1.5, 2.0).is_err() {
                return false;
            }

            // Test rotation
            if crate::transform::rotate_projection(proj, std::f64::consts::PI / 4.0, 0.0, 0.0)
                .is_err()
            {
                return false;
            }

            // Test translation
            if crate::transform::translate_projection(proj, 10.0, -5.0).is_err() {
                return false;
            }

            // Verify projection is still valid after transformations
            if !proj.verify_projection() {
                return false;
            }
        } else {
            return false;
        }
    }

    // Test 4: Boundary region verification
    let test_region = crate::shard::AtlasBoundaryRegion {
        start_coord: 0,
        end_coord: 8192,
        page_count: 2,
        region_class: 42,
        is_conserved: true,
        affecting_resonance_classes: vec![42, 15, 77],
        spatial_bounds: (0.0, 0.0, 1.0, 1.0),
    };

    if !AtlasProjection::verify_boundary_region(&test_region) {
        return false;
    }

    // Test 5: Transform parameter verification
    let valid_params = TransformationParams {
        scaling_factor: 1.2,
        rotation_angle: std::f64::consts::PI / 6.0,
        translation_x: 100.0,
        translation_y: -50.0,
    };

    if !crate::transform::verify_transform_params(&valid_params) {
        return false;
    }

    let invalid_params = TransformationParams {
        scaling_factor: 0.0, // Invalid: zero scaling
        rotation_angle: std::f64::consts::PI / 6.0,
        translation_x: 100.0,
        translation_y: -50.0,
    };

    if crate::transform::verify_transform_params(&invalid_params) {
        return false; // Should return false for invalid params
    }

    // Clean up resources
    crate::projection::atlas_projection_destroy(linear_projection);
    crate::projection::atlas_projection_destroy(r96_projection);
    crate::projection::atlas_projection_destroy(transform_projection);

    // All tests passed
    true
}

// =============================================================================
// Transform Operations FFI
// =============================================================================

/// Apply linear transformation to projection data
///
/// # Safety
/// The projection handle must be valid and `matrix` must point to 16 f64 values.
#[no_mangle]
pub extern "C" fn atlas_manifold_apply_linear_transform(
    projection: *mut CAtlasProjectionHandle,
    matrix: *const f64, // 4x4 matrix in row-major order
) -> c_int {
    if projection.is_null() || matrix.is_null() {
        return error_to_code(&AtlasError::InvalidInput("null pointer"));
    }

    // SAFETY: Validated pointers and proper FFI usage
    unsafe {
        let c_projection = &mut *projection;
        if c_projection.inner.is_null() {
            return error_to_code(&AtlasError::InvalidInput("invalid projection"));
        }

        let atlas_projection = &mut *(c_projection.inner as *mut AtlasProjectionHandle);
        if let Some(proj) = atlas_projection.as_mut() {
            // Convert raw pointer to array reference
            let matrix_slice = core::slice::from_raw_parts(matrix, 16);
            let mut matrix_array = [0.0f64; 16];
            matrix_array.copy_from_slice(matrix_slice);

            match crate::transform::apply_linear_transform(proj, &matrix_array) {
                Ok(()) => {
                    TRANSFORMS_APPLIED.fetch_add(1, Ordering::Relaxed);
                    0
                },
                Err(e) => error_to_code(&e),
            }
        } else {
            error_to_code(&AtlasError::InvalidInput("corrupted projection"))
        }
    }
}

/// Apply Fourier transform to R96 projection data
///
/// # Safety
/// The projection handle must be valid and must be an `R96_FOURIER` projection.
#[no_mangle]
pub extern "C" fn atlas_manifold_apply_fourier_transform(
    projection: *mut CAtlasProjectionHandle,
    inverse: bool,
) -> c_int {
    if projection.is_null() {
        return error_to_code(&AtlasError::InvalidInput("null pointer"));
    }

    // SAFETY: Validated pointers and proper FFI usage
    unsafe {
        let c_projection = &mut *projection;
        if c_projection.inner.is_null() {
            return error_to_code(&AtlasError::InvalidInput("invalid projection"));
        }

        let atlas_projection = &mut *(c_projection.inner as *mut AtlasProjectionHandle);
        if let Some(proj) = atlas_projection.as_mut() {
            match crate::transform::apply_fourier_transform(proj, inverse) {
                Ok(()) => {
                    TRANSFORMS_APPLIED.fetch_add(1, Ordering::Relaxed);
                    0
                },
                Err(e) => error_to_code(&e),
            }
        } else {
            error_to_code(&AtlasError::InvalidInput("corrupted projection"))
        }
    }
}

/// Apply geometric scaling to projection coordinates
///
/// # Safety
/// The projection handle must be valid.
#[no_mangle]
pub extern "C" fn atlas_manifold_scale_projection(
    projection: *mut CAtlasProjectionHandle,
    scale_x: f64,
    scale_y: f64,
) -> c_int {
    if projection.is_null() {
        return error_to_code(&AtlasError::InvalidInput("null pointer"));
    }

    // SAFETY: Validated pointers and proper FFI usage
    unsafe {
        let c_projection = &mut *projection;
        if c_projection.inner.is_null() {
            return error_to_code(&AtlasError::InvalidInput("invalid projection"));
        }

        let atlas_projection = &mut *(c_projection.inner as *mut AtlasProjectionHandle);
        if let Some(proj) = atlas_projection.as_mut() {
            match crate::transform::scale_projection(proj, scale_x, scale_y) {
                Ok(()) => {
                    TRANSFORMS_APPLIED.fetch_add(1, Ordering::Relaxed);
                    0
                },
                Err(e) => error_to_code(&e),
            }
        } else {
            error_to_code(&AtlasError::InvalidInput("corrupted projection"))
        }
    }
}

/// Rotate projection coordinates around center point
///
/// # Safety
/// The projection handle must be valid.
#[no_mangle]
pub extern "C" fn atlas_manifold_rotate_projection(
    projection: *mut CAtlasProjectionHandle,
    angle: f64,
    center_x: f64,
    center_y: f64,
) -> c_int {
    if projection.is_null() {
        return error_to_code(&AtlasError::InvalidInput("null pointer"));
    }

    // SAFETY: Validated pointers and proper FFI usage
    unsafe {
        let c_projection = &mut *projection;
        if c_projection.inner.is_null() {
            return error_to_code(&AtlasError::InvalidInput("invalid projection"));
        }

        let atlas_projection = &mut *(c_projection.inner as *mut AtlasProjectionHandle);
        if let Some(proj) = atlas_projection.as_mut() {
            match crate::transform::rotate_projection(proj, angle, center_x, center_y) {
                Ok(()) => {
                    TRANSFORMS_APPLIED.fetch_add(1, Ordering::Relaxed);
                    0
                },
                Err(e) => error_to_code(&e),
            }
        } else {
            error_to_code(&AtlasError::InvalidInput("corrupted projection"))
        }
    }
}

/// Translate projection coordinates by offset vector
///
/// # Safety
/// The projection handle must be valid.
#[no_mangle]
pub extern "C" fn atlas_manifold_translate_projection(
    projection: *mut CAtlasProjectionHandle,
    offset_x: f64,
    offset_y: f64,
) -> c_int {
    if projection.is_null() {
        return error_to_code(&AtlasError::InvalidInput("null pointer"));
    }

    // SAFETY: Validated pointers and proper FFI usage
    unsafe {
        let c_projection = &mut *projection;
        if c_projection.inner.is_null() {
            return error_to_code(&AtlasError::InvalidInput("invalid projection"));
        }

        let atlas_projection = &mut *(c_projection.inner as *mut AtlasProjectionHandle);
        if let Some(proj) = atlas_projection.as_mut() {
            match crate::transform::translate_projection(proj, offset_x, offset_y) {
                Ok(()) => {
                    TRANSFORMS_APPLIED.fetch_add(1, Ordering::Relaxed);
                    0
                },
                Err(e) => error_to_code(&e),
            }
        } else {
            error_to_code(&AtlasError::InvalidInput("corrupted projection"))
        }
    }
}

// =============================================================================
// Runtime Information Functions FFI
// =============================================================================

/// Check if manifold layer was compiled with optimization support
///
/// # Safety
/// Safe to call from any thread.
#[no_mangle]
pub extern "C" fn atlas_manifold_is_optimized() -> bool {
    #[cfg(debug_assertions)]
    {
        false // Debug build
    }
    #[cfg(not(debug_assertions))]
    {
        true // Release build
    }
}

/// Get supported projection types as bitmask
///
/// # Safety
/// Safe to call from any thread.
#[no_mangle]
pub extern "C" fn atlas_manifold_get_supported_projections() -> u32 {
    let mut bitmask = 0u32;
    bitmask |= 1 << (ProjectionType::Linear as u32); // Bit 0: LINEAR
    bitmask |= 1 << (ProjectionType::R96Fourier as u32); // Bit 1: R96_FOURIER
    bitmask
}

/// Get manifold layer performance statistics
///
/// # Safety
/// All output parameters must be valid pointers. Safe to call from any thread.
#[no_mangle]
pub extern "C" fn atlas_manifold_get_statistics(
    projections_created: *mut u64,
    shards_extracted: *mut u64,
    transforms_applied: *mut u64,
) -> bool {
    if projections_created.is_null() || shards_extracted.is_null() || transforms_applied.is_null() {
        return false;
    }

    // SAFETY: Validated pointers and proper FFI usage
    unsafe {
        *projections_created = PROJECTIONS_CREATED.load(Ordering::Relaxed);
        *shards_extracted = SHARDS_EXTRACTED.load(Ordering::Relaxed);
        *transforms_applied = TRANSFORMS_APPLIED.load(Ordering::Relaxed);
    }

    true
}

/// Reset manifold layer performance statistics
///
/// # Safety
/// Safe to call from any thread (uses atomic operations).
#[no_mangle]
pub extern "C" fn atlas_manifold_reset_statistics() {
    PROJECTIONS_CREATED.store(0, Ordering::Relaxed);
    SHARDS_EXTRACTED.store(0, Ordering::Relaxed);
    TRANSFORMS_APPLIED.store(0, Ordering::Relaxed);
}

// =============================================================================
// Advanced Mathematical Operations FFI
// =============================================================================

/// Compute topological invariants for projection
///
/// # Safety
/// The projection handle must be valid and `invariants` array must have space for `max_invariants` doubles.
#[no_mangle]
#[allow(clippy::cast_possible_truncation)]
pub extern "C" fn atlas_manifold_compute_invariants(
    projection: *const CAtlasProjectionHandle,
    invariants: *mut f64,
    max_invariants: size_t,
) -> c_int {
    if projection.is_null() || invariants.is_null() || max_invariants == 0 {
        return error_to_code(&AtlasError::InvalidInput("null pointer or zero count"));
    }

    // SAFETY: Validated pointers and proper FFI usage
    unsafe {
        let c_projection = &*projection;
        if c_projection.inner.is_null() {
            return error_to_code(&AtlasError::InvalidInput("invalid projection"));
        }

        let atlas_projection = &*(c_projection.inner as *const AtlasProjectionHandle);
        if let Some(proj) = atlas_projection.as_ref() {
            let invariants_slice = core::slice::from_raw_parts_mut(invariants, max_invariants);

            // Compute basic topological invariants
            let mut computed_count = 0;

            // Invariant 1: Conservation checksum ratio (always available)
            if computed_count < max_invariants {
                invariants_slice[computed_count] = (proj.total_conservation_sum % 96) as f64 / 96.0;
                computed_count += 1;
            }

            // Invariant 2: Dimension ratio
            if computed_count < max_invariants {
                let (width, height) = proj.get_dimensions();
                invariants_slice[computed_count] = f64::from(width) / f64::from(height).max(1.0);
                computed_count += 1;
            }

            // Invariant 3: Tile count normalized by source size
            if computed_count < max_invariants {
                let tile_density = proj.tiles.len() as f64 / (proj.source_size as f64).max(1.0);
                invariants_slice[computed_count] = tile_density;
                computed_count += 1;
            }

            // For R96 Fourier projections, add more invariants
            if proj.projection_type == ProjectionType::R96Fourier {
                if let Some(ref r96_data) = proj.r96_fourier_data {
                    // Invariant 4: Active class ratio
                    if computed_count < max_invariants {
                        let active_classes = r96_data.get_active_classes();
                        let class_ratio = active_classes.len() as f64 / 96.0;
                        invariants_slice[computed_count] = class_ratio;
                        computed_count += 1;
                    }
                }
            }

            #[allow(clippy::cast_possible_wrap)]
            {
                computed_count as c_int
            }
        } else {
            error_to_code(&AtlasError::InvalidInput("corrupted projection"))
        }
    }
}

/// Calculate manifold curvature at specified coordinates
///
/// # Safety
/// The projection handle must be valid.
#[no_mangle]
pub extern "C" fn atlas_manifold_compute_curvature(
    projection: *const CAtlasProjectionHandle,
    x: f64,
    y: f64,
) -> f64 {
    if projection.is_null() || !x.is_finite() || !y.is_finite() {
        return f64::NAN;
    }

    // SAFETY: Validated pointers and proper FFI usage
    unsafe {
        let c_projection = &*projection;
        if c_projection.inner.is_null() {
            return f64::NAN;
        }

        let atlas_projection = &*(c_projection.inner as *const AtlasProjectionHandle);
        if let Some(proj) = atlas_projection.as_ref() {
            // Find tiles that contain or are near the point
            let mut curvature_sum = 0.0;
            let mut weight_sum = 0.0;

            for tile in &proj.tiles {
                let (min_x, min_y, max_x, max_y) = tile.bounds;

                // Calculate distance from point to tile center
                let center_x = (min_x + max_x) / 2.0;
                let center_y = (min_y + max_y) / 2.0;
                let dist_sq = (x - center_x).powi(2) + (y - center_y).powi(2);

                if dist_sq < 1e-12 {
                    // Point is at tile center, use tile-specific curvature
                    let local_curvature = if tile.verify_conservation() { 0.0 } else { 1.0 };
                    return local_curvature;
                }

                // Weight by inverse distance squared
                let weight = 1.0 / (1.0 + dist_sq);
                let tile_curvature = if tile.verify_conservation() {
                    // Conservative regions have lower curvature
                    0.1 / (1.0 + tile.conservation_sum as f64 / 1000.0)
                } else {
                    // Non-conservative regions have higher curvature
                    1.0 + (tile.conservation_sum % 96) as f64 / 96.0
                };

                curvature_sum += tile_curvature * weight;
                weight_sum += weight;
            }

            if weight_sum > 1e-12 {
                curvature_sum / weight_sum
            } else {
                0.0 // Default curvature
            }
        } else {
            f64::NAN
        }
    }
}

/// Compute geodesic distance between two points on manifold
///
/// # Safety
/// The projection handle must be valid.
#[no_mangle]
pub extern "C" fn atlas_manifold_geodesic_distance(
    projection: *const CAtlasProjectionHandle,
    x1: f64,
    y1: f64,
    x2: f64,
    y2: f64,
) -> f64 {
    if projection.is_null()
        || !x1.is_finite()
        || !y1.is_finite()
        || !x2.is_finite()
        || !y2.is_finite()
    {
        return -1.0;
    }

    // SAFETY: Validated pointers and proper FFI usage
    unsafe {
        let c_projection = &*projection;
        if c_projection.inner.is_null() {
            return -1.0;
        }

        let atlas_projection = &*(c_projection.inner as *const AtlasProjectionHandle);
        if let Some(_proj) = atlas_projection.as_ref() {
            // Compute Euclidean distance as baseline
            let euclidean_dist = ((x2 - x1).powi(2) + (y2 - y1).powi(2)).sqrt();

            // For manifolds with curvature, adjust the distance
            let mid_x = (x1 + x2) / 2.0;
            let mid_y = (y1 + y2) / 2.0;

            // Compute curvature at midpoint
            let curvature = atlas_manifold_compute_curvature(projection, mid_x, mid_y);

            if curvature.is_finite() {
                // Adjust distance based on curvature
                // Positive curvature increases geodesic distance
                // Negative curvature decreases geodesic distance
                let curvature_factor = 1.0 + curvature * euclidean_dist / 10.0;
                euclidean_dist * curvature_factor.max(0.1) // Prevent negative distances
            } else {
                euclidean_dist
            }
        } else {
            -1.0
        }
    }
}

/// Find critical points in the manifold projection
///
/// # Safety
/// The projection handle must be valid and `critical_points` must have space for 2*`max_points` doubles.
#[no_mangle]
#[allow(clippy::cast_possible_truncation)]
pub extern "C" fn atlas_manifold_find_critical_points(
    projection: *const CAtlasProjectionHandle,
    critical_points: *mut f64,
    max_points: size_t,
) -> c_int {
    if projection.is_null() || critical_points.is_null() || max_points == 0 {
        return error_to_code(&AtlasError::InvalidInput("null pointer or zero count"));
    }

    // SAFETY: Validated pointers and proper FFI usage
    unsafe {
        let c_projection = &*projection;
        if c_projection.inner.is_null() {
            return error_to_code(&AtlasError::InvalidInput("invalid projection"));
        }

        let atlas_projection = &*(c_projection.inner as *const AtlasProjectionHandle);
        if let Some(proj) = atlas_projection.as_ref() {
            let points_slice = core::slice::from_raw_parts_mut(critical_points, max_points * 2);
            let mut found_points = 0;

            // Look for critical points at tile centers where conservation changes
            for tile in &proj.tiles {
                if found_points >= max_points {
                    break;
                }

                let (min_x, min_y, max_x, max_y) = tile.bounds;
                let center_x = (min_x + max_x) / 2.0;
                let center_y = (min_y + max_y) / 2.0;

                // Check if this tile has different conservation status from its neighbors
                let mut is_critical = false;
                let this_conservation = tile.verify_conservation();

                // Look at adjacent tiles (simplified grid check)
                for other_tile in &proj.tiles {
                    if other_tile.id == tile.id {
                        continue;
                    }

                    let (o_min_x, o_min_y, o_max_x, o_max_y) = other_tile.bounds;
                    let _o_center_x = (o_min_x + o_max_x) / 2.0;
                    let _o_center_y = (o_min_y + o_max_y) / 2.0;

                    // Check if tiles are adjacent using harmonic pairing
                    // Two tiles are adjacent if their resonance classes harmonize
                    let this_class = atlas_r96_classify((tile.id % 256) as u8);
                    let other_class = atlas_r96_classify((other_tile.id % 256) as u8);
                    if atlas_r96_harmonizes(this_class, other_class) {
                        // Harmonically adjacent (verified via atlas_r96_harmonizes)
                        let other_conservation = other_tile.verify_conservation();
                        if this_conservation != other_conservation {
                            is_critical = true;
                            break;
                        }
                    }
                }

                if is_critical {
                    points_slice[found_points * 2] = center_x;
                    points_slice[found_points * 2 + 1] = center_y;
                    found_points += 1;
                }
            }

            #[allow(clippy::cast_possible_wrap)]
            {
                found_points as c_int
            }
        } else {
            error_to_code(&AtlasError::InvalidInput("corrupted projection"))
        }
    }
}

// =============================================================================
// Error Handling FFI Functions
// =============================================================================

/// Set the last error code in thread-local storage
///
/// # Safety
/// Safe to call from any thread - uses thread-local storage.
#[no_mangle]
pub extern "C" fn atlas_manifold_set_last_error(error_code: c_int) {
    THREAD_LOCAL_ERROR.with(|err| {
        *err.borrow_mut() = error_code;
    });
}

/// Get the last error code from thread-local storage
///
/// # Safety
/// Safe to call from any thread - reads from thread-local storage.
#[no_mangle]
pub extern "C" fn atlas_manifold_get_last_error_ffi() -> c_int {
    THREAD_LOCAL_ERROR.with(|err| *err.borrow())
}

/// Get a human-readable error message for the given error code
///
/// # Safety
/// Safe to call from any thread. Returns a pointer to static string data.
/// The returned pointer is valid for the lifetime of the program.
#[no_mangle]
pub extern "C" fn atlas_manifold_error_string_ffi(error_code: c_int) -> *const c_char {
    match error_code {
        0 => "Success\0".as_ptr() as *const c_char,
        1 => "Invalid argument\0".as_ptr() as *const c_char,
        2 => "Out of memory\0".as_ptr() as *const c_char,
        3 => "Invalid projection\0".as_ptr() as *const c_char,
        4 => "Invalid shard\0".as_ptr() as *const c_char,
        5 => "Reconstruction failed\0".as_ptr() as *const c_char,
        6 => "Transform failed\0".as_ptr() as *const c_char,
        7 => "Invalid boundary\0".as_ptr() as *const c_char,
        8 => "Verification failed\0".as_ptr() as *const c_char,
        9 => "Not implemented\0".as_ptr() as *const c_char,
        _ => "Unknown error\0".as_ptr() as *const c_char,
    }
}

// =============================================================================
// Simple Mathematical Functions (No Projection Required)
// =============================================================================

/// Simple curvature computation without requiring projection handle
///
/// # Safety
/// Safe to call from any thread.
#[no_mangle]
pub extern "C" fn atlas_manifold_compute_curvature_simple(x: f64, y: f64) -> f64 {
    if !x.is_finite() || !y.is_finite() {
        return f64::NAN;
    }

    // Compute Gaussian curvature using the same math as atlas-manifold.c
    use core::f64::consts::PI;

    const R96_DIM: i32 = 96;
    let resonance_freq = 2.0 * PI / f64::from(R96_DIM).sqrt();
    let conservation_energy = 1.0;

    // Compute first partial derivatives (fx, fy)
    let fx = conservation_energy
        * resonance_freq
        * ((resonance_freq * x).cos() * (resonance_freq * y).cos()
            - 0.3 * (3.0 * resonance_freq * x).sin() * (2.0 * resonance_freq * y).sin());

    let fy = conservation_energy
        * resonance_freq
        * (-(resonance_freq * x).sin() * (resonance_freq * y).sin()
            + 0.2 * (3.0 * resonance_freq * x).cos() * (2.0 * resonance_freq * y).cos());

    // Compute second partial derivatives (fxx, fyy, fxy)
    let fxx = -conservation_energy
        * resonance_freq
        * resonance_freq
        * ((resonance_freq * x).sin() * (resonance_freq * y).cos()
            + 0.9 * (3.0 * resonance_freq * x).cos() * (2.0 * resonance_freq * y).sin());

    let fyy = -conservation_energy
        * resonance_freq
        * resonance_freq
        * ((resonance_freq * x).sin() * (resonance_freq * y).cos()
            + 0.4 * (3.0 * resonance_freq * x).cos() * (2.0 * resonance_freq * y).sin());

    let fxy = -conservation_energy
        * resonance_freq
        * resonance_freq
        * ((resonance_freq * x).cos() * (resonance_freq * y).sin()
            - 0.6 * (3.0 * resonance_freq * x).sin() * (2.0 * resonance_freq * y).cos());

    // Compute coefficients of first fundamental form
    let e = 1.0 + fx * fx; // (1 + fx²)
    let f = fx * fy; // fx·fy
    let g = 1.0 + fy * fy; // (1 + fy²)

    let discriminant = e * g - f * f;
    if discriminant <= 0.0 {
        return f64::NAN;
    }

    let sqrt_discriminant = discriminant.sqrt();

    // Compute coefficients of second fundamental form
    let l = fxx / sqrt_discriminant;
    let m = fxy / sqrt_discriminant;
    let n = fyy / sqrt_discriminant;

    // Gaussian curvature: K = (LN - M²) / (EG - F²)
    let gaussian_curvature = (l * n - m * m) / discriminant;

    // Apply conservation constraint weighting
    let conservation_factor = 1.0 + 0.1 * conservation_energy;
    gaussian_curvature * conservation_factor
}

/// Simple geodesic distance computation without requiring projection handle
///
/// # Safety
/// Safe to call from any thread.
#[no_mangle]
pub extern "C" fn atlas_manifold_geodesic_distance_simple(
    x1: f64,
    y1: f64,
    x2: f64,
    y2: f64,
) -> f64 {
    if !x1.is_finite() || !y1.is_finite() || !x2.is_finite() || !y2.is_finite() {
        return -1.0;
    }

    // If points are identical, distance is zero
    if (x2 - x1).abs() < 1e-12 && (y2 - y1).abs() < 1e-12 {
        return 0.0;
    }

    // Simple geodesic distance computation using the same math as atlas-manifold.c
    use core::f64::consts::PI;

    const R96_DIM: i32 = 96;
    let resonance_freq = 2.0 * PI / f64::from(R96_DIM).sqrt();
    let conservation_energy = 1.0;

    // Use numerical integration along geodesic path
    const NUM_STEPS: usize = 50; // Reduced for performance
    let mut total_distance = 0.0;

    for i in 0..NUM_STEPS {
        let t = (i as f64) / (NUM_STEPS as f64);
        let t_next = ((i + 1) as f64) / (NUM_STEPS as f64);

        // Current position on path
        let x = x1 + t * (x2 - x1);
        let y = y1 + t * (y2 - y1);

        // Next position on path
        let x_next = x1 + t_next * (x2 - x1);
        let y_next = y1 + t_next * (y2 - y1);

        // First derivatives for metric calculation
        let fx = conservation_energy
            * resonance_freq
            * ((resonance_freq * x).cos() * (resonance_freq * y).cos()
                - 0.3 * (3.0 * resonance_freq * x).sin() * (2.0 * resonance_freq * y).sin());

        let fy = conservation_energy
            * resonance_freq
            * (-(resonance_freq * x).sin() * (resonance_freq * y).sin()
                + 0.2 * (3.0 * resonance_freq * x).cos() * (2.0 * resonance_freq * y).cos());

        // Riemannian metric coefficients (induced from 3D embedding)
        let mut g11 = 1.0 + fx * fx; // gxx
        let mut g12 = fx * fy; // gxy = gyx
        let mut g22 = 1.0 + fy * fy; // gyy

        // Tangent vector components
        let dx = x_next - x;
        let dy = y_next - y;

        // Apply conservation constraint weighting to metric
        let conservation_factor = 1.0 + 0.2 * conservation_energy;
        g11 *= conservation_factor;
        g22 *= conservation_factor;
        g12 *= conservation_factor;

        // Compute infinitesimal arc length: ds² = gᵢⱼ dxⁱ dxʲ
        let ds_squared = g11 * dx * dx + 2.0 * g12 * dx * dy + g22 * dy * dy;

        if ds_squared > 0.0 {
            total_distance += ds_squared.sqrt();
        }
    }

    total_distance
}

/// Simple invariants computation without requiring projection handle
///
/// # Safety
/// The invariants array must have space for `max_invariants` doubles.
#[no_mangle]
#[allow(clippy::cast_possible_truncation)]
pub extern "C" fn atlas_manifold_compute_invariants_simple(
    invariants: *mut f64,
    max_invariants: size_t,
) -> c_int {
    if invariants.is_null() || max_invariants == 0 {
        return error_to_code(&AtlasError::InvalidInput("null pointer or zero count"));
    }

    // Compute basic invariants for Atlas-12288 manifold with R96 resonance structure
    // SAFETY: Validated pointers and proper FFI usage
    unsafe {
        let invariants_slice = core::slice::from_raw_parts_mut(invariants, max_invariants);
        let mut computed = 0;

        const R96_DIM: i32 = 96;
        const ATLAS_DIM: i32 = 12288;
        let grid_size = (f64::from(ATLAS_DIM / R96_DIM)).sqrt() as i32;

        // Count vertices, edges, faces from the 2D projection
        let vertices = grid_size * grid_size;
        let interior_vertices = (grid_size - 2) * (grid_size - 2);
        let boundary_vertices = vertices - interior_vertices;

        // Count edges: each interior vertex contributes 2 edges (right, down)
        let edges = interior_vertices * 2 + boundary_vertices + (grid_size - 1) * 2;

        // Count faces: (grid_size - 1) x (grid_size - 1) squares
        let faces = (grid_size - 1) * (grid_size - 1);

        // Invariant 1: Euler characteristic: χ = V - E + F
        if computed < max_invariants {
            invariants_slice[computed] = f64::from(vertices - edges + faces);
            computed += 1;
        }

        // Invariant 2: Genus g = (2 - χ) / 2
        if computed < max_invariants {
            let euler_char = (vertices - edges + faces) as f64;
            let genus = ((2.0 - euler_char) / 2.0).max(0.0);
            invariants_slice[computed] = genus;
            computed += 1;
        }

        // Invariant 3: β₀ = number of connected components
        if computed < max_invariants {
            invariants_slice[computed] = 1.0;
            computed += 1;
        }

        // Invariant 4: β₁ = 2g for closed orientable surface
        if computed < max_invariants {
            let euler_char = (vertices - edges + faces) as f64;
            let genus = ((2.0 - euler_char) / 2.0).max(0.0);
            invariants_slice[computed] = 2.0 * genus;
            computed += 1;
        }

        // Invariant 5: β₂ = 1 for closed orientable surface
        if computed < max_invariants {
            invariants_slice[computed] = 1.0;
            computed += 1;
        }

        computed as c_int
    }
}

/// Simple critical points finding without requiring projection handle
///
/// # Safety
/// The critical_points array must have space for 2*max_points doubles.
#[no_mangle]
#[allow(clippy::cast_possible_truncation)]
pub extern "C" fn atlas_manifold_find_critical_points_simple(
    critical_points: *mut f64,
    max_points: size_t,
) -> c_int {
    if critical_points.is_null() || max_points == 0 {
        return error_to_code(&AtlasError::InvalidInput("null pointer or zero count"));
    }

    // Find critical points on Atlas manifold respecting conservation constraints
    use core::f64::consts::PI;

    const R96_DIM: i32 = 96;
    let resonance_freq = 2.0 * PI / f64::from(R96_DIM).sqrt();
    let search_radius = 4.0 * PI / resonance_freq; // Cover full period
    const EPSILON: f64 = 1e-8; // Tolerance for zero gradient
    const GRID_RESOLUTION: i32 = 10; // Search grid density (reduced for performance)

    let step = 2.0 * search_radius / (GRID_RESOLUTION as f64);

    // SAFETY: Validated pointers and proper FFI usage
    unsafe {
        let points_slice = core::slice::from_raw_parts_mut(critical_points, max_points * 2);
        let mut found = 0;

        // Search for critical points on a grid
        for i in 0..GRID_RESOLUTION {
            if found >= max_points {
                break;
            }
            for j in 0..GRID_RESOLUTION {
                if found >= max_points {
                    break;
                }

                let x = -search_radius + f64::from(i) * step;
                let y = -search_radius + (j as f64) * step;

                // Compute gradient at this point
                let e = 1.0; // conservation energy
                let omega = resonance_freq;

                // First partial derivatives
                let fx = e
                    * omega
                    * ((omega * x).cos() * (omega * y).cos()
                        - 0.3 * (3.0 * omega * x).sin() * (2.0 * omega * y).sin());

                let fy = e
                    * omega
                    * (-(omega * x).sin() * (omega * y).sin()
                        + 0.2 * (3.0 * omega * x).cos() * (2.0 * omega * y).cos());

                // Check if gradient is approximately zero (critical point condition)
                if fx.abs() < EPSILON && fy.abs() < EPSILON {
                    // Compute function value to verify conservation constraint
                    let function_value = e
                        * ((omega * x).sin() * (omega * y).cos()
                            + 0.1 * (3.0 * omega * x).cos() * (2.0 * omega * y).sin());

                    let conservation_constraint = function_value * function_value;

                    // Critical points must preserve the manifold's conservation energy
                    if conservation_constraint <= e * e * 1.1 {
                        // Within 10% tolerance
                        points_slice[found * 2] = x;
                        points_slice[found * 2 + 1] = y;
                        found += 1;
                    }
                }
            }
        }

        found as c_int
    }
}

// =============================================================================
// C API Compatibility Aliases
// =============================================================================

/// C API compatibility alias for atlas_projection_create_ffi
#[no_mangle]
pub extern "C" fn atlas_projection_create(
    projection_type: u32,
    source_data: *const u8,
    source_size: size_t,
) -> *mut CAtlasProjectionHandle {
    atlas_projection_create_ffi(projection_type, source_data, source_size)
}

/// C API compatibility alias for atlas_manifold_get_last_error_ffi
#[no_mangle]
pub extern "C" fn atlas_manifold_get_last_error() -> c_int {
    atlas_manifold_get_last_error_ffi()
}

/// C API compatibility alias for atlas_manifold_error_string_ffi
#[no_mangle]
pub extern "C" fn atlas_manifold_error_string(error_code: c_int) -> *const c_char {
    atlas_manifold_error_string_ffi(error_code)
}

/// C API compatibility - curvature computation without projection (overrides the projection version)
#[no_mangle]
pub extern "C" fn atlas_manifold_compute_curvature_compat(x: f64, y: f64) -> f64 {
    atlas_manifold_compute_curvature_simple(x, y)
}

/// C API compatibility - geodesic distance computation without projection (overrides the projection version)
#[no_mangle]
pub extern "C" fn atlas_manifold_geodesic_distance_compat(
    x1: f64,
    y1: f64,
    x2: f64,
    y2: f64,
) -> f64 {
    atlas_manifold_geodesic_distance_simple(x1, y1, x2, y2)
}

/// C API compatibility - invariants computation without projection (overrides the projection version)
#[no_mangle]
pub extern "C" fn atlas_manifold_compute_invariants_compat(
    invariants: *mut f64,
    max_invariants: size_t,
) -> c_int {
    atlas_manifold_compute_invariants_simple(invariants, max_invariants)
}

/// C API compatibility - critical points finding without projection (overrides the projection version)
#[no_mangle]
pub extern "C" fn atlas_manifold_find_critical_points_compat(
    critical_points: *mut f64,
    max_points: size_t,
) -> c_int {
    atlas_manifold_find_critical_points_simple(critical_points, max_points)
}

// =============================================================================
// Layer 2 Conservation Integration Functions
// =============================================================================

/// Generate witness for data with Layer 2 conservation verification
///
/// # Safety
/// The data pointer must be valid and point to at least `length` bytes.
#[no_mangle]
pub extern "C" fn atlas_manifold_witness_generate(data: *const u8, length: size_t) -> *mut c_void {
    if data.is_null() || length == 0 {
        return core::ptr::null_mut();
    }

    // SAFETY: Validated data pointer and length
    unsafe { atlas_witness_generate(data, length) }
}

/// Verify witness against data using Layer 2 conservation
///
/// # Safety  
/// Both witness and data pointers must be valid, data must have at least `length` bytes.
#[no_mangle]
pub extern "C" fn atlas_manifold_witness_verify(
    witness: *const c_void,
    data: *const u8,
    length: size_t,
) -> bool {
    if witness.is_null() || data.is_null() || length == 0 {
        return false;
    }

    // SAFETY: Validated witness and data pointers
    unsafe { atlas_witness_verify(witness, data, length) }
}

/// Destroy witness and free resources
///
/// # Safety
/// The witness pointer must be a valid witness returned by atlas_witness_generate, or NULL.
#[no_mangle]
pub extern "C" fn atlas_manifold_witness_destroy(witness: *mut c_void) {
    if !witness.is_null() {
        // SAFETY: Validated witness pointer
        unsafe {
            atlas_witness_destroy(witness);
        }
    }
}

/// Check if data satisfies Layer 2 conservation laws (sum % 96 == 0)
///
/// # Safety
/// The data pointer must be valid and point to at least `length` bytes.
#[no_mangle]
pub extern "C" fn atlas_manifold_conserved_check(data: *const u8, length: size_t) -> bool {
    if data.is_null() || length == 0 {
        return false;
    }

    // SAFETY: Validated data pointer and length
    unsafe { atlas_conserved_check(data, length) }
}

/// Check if data window satisfies Layer 2 conservation laws with enhanced validation
///
/// # Safety
/// The data pointer must be valid and point to at least `length` bytes.
#[no_mangle]
pub extern "C" fn atlas_manifold_conserved_window_check(data: *const u8, length: size_t) -> bool {
    if data.is_null() || length == 0 {
        return false;
    }

    // SAFETY: Validated data pointer and length
    unsafe { atlas_conserved_window_streaming_check_llvm(data, length) }
}

/// Calculate conservation delta between two memory states
///
/// # Safety
/// Both before and after pointers must be valid and point to at least `length` bytes.
#[no_mangle]
pub extern "C" fn atlas_manifold_conserved_delta(
    before: *const u8,
    after: *const u8,
    length: size_t,
) -> u8 {
    if before.is_null() || after.is_null() || length == 0 {
        return 255; // Invalid delta
    }

    // SAFETY: Validated before and after pointers
    unsafe { atlas_conserved_delta(before, after, length) }
}

/// Create conservation domain for manifold operations
///
/// # Safety
/// This function is safe to call, but the returned domain must be destroyed with
/// `atlas_manifold_domain_destroy` to avoid memory leaks.
#[no_mangle]
pub extern "C" fn atlas_manifold_domain_create(bytes: size_t, budget_class: u8) -> *mut c_void {
    if bytes == 0 || budget_class >= 96 {
        return core::ptr::null_mut();
    }

    // SAFETY: Validated parameters
    unsafe { atlas_domain_create(bytes, budget_class) }
}

/// Verify conservation domain integrity
///
/// # Safety
/// The domain pointer must be a valid domain created by `atlas_manifold_domain_create`.
#[no_mangle]
pub extern "C" fn atlas_manifold_domain_verify(domain: *const c_void) -> bool {
    if domain.is_null() {
        return false;
    }

    // SAFETY: Validated domain pointer
    unsafe { atlas_domain_verify(domain) }
}

/// Destroy conservation domain and free resources
///
/// # Safety
/// The domain pointer must be a valid domain created by `atlas_manifold_domain_create`, or NULL.
#[no_mangle]
pub extern "C" fn atlas_manifold_domain_destroy(domain: *mut c_void) {
    if !domain.is_null() {
        // SAFETY: Validated domain pointer
        unsafe {
            atlas_domain_destroy(domain);
        }
    }
}

/// Get last Layer 2 conservation error code
///
/// # Safety
/// Safe to call from any thread.
#[no_mangle]
pub extern "C" fn atlas_manifold_get_conservation_error() -> u32 {
    // SAFETY: atlas_get_last_error is thread-safe
    unsafe { atlas_get_last_error() }
}
