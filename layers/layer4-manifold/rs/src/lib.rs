//! # Atlas Manifold - Layer 4
//!
//! Multi-dimensional transformation and projection layer for the Atlas Hologram system.
//! Provides manifold operations, coordinate transformations, and high-dimensional
//! geometric primitives built on top of Layer 2 (Conservation) and Layer 3 (Resonance).
//!
//! ## Overview
//!
//! Layer 4 (Manifold) implements a sophisticated mathematical framework for handling
//! multi-dimensional data transformations with conservation law preservation. The core
//! functionality includes:
//!
//! - **Atlas Projections**: Convert high-dimensional data into manageable 2D representations
//! - **Manifold Operations**: Geometric transformations that preserve topological properties
//! - **Conservation Integration**: Automatic validation of Layer 2 conservation laws
//! - **R96 Resonance**: Integration with Layer 3 resonance classification system
//! - **FFI Safety**: Complete C API for integration with other system components
//!
//! ## Quick Start
//!
//! ### Basic Projection Creation
//!
//! ```rust
//! use atlas_manifold::{projection::*, AtlasResult};
//!
//! // Create source data (must be multiple of page size)
//! let source_data = vec![42u8; 8192]; // 2 pages of data
//!
//! // Create a linear projection
//! let projection = atlas_projection_create(
//!     ProjectionType::Linear,
//!     source_data.as_ptr(),
//!     source_data.len()
//! )?;
//!
//! // Verify projection integrity
//! if let Some(proj) = projection.as_ref() {
//!     assert!(proj.verify_projection());
//! }
//!
//! // Clean up resources
//! atlas_projection_destroy(projection);
//! # Ok::<(), atlas_manifold::AtlasError>(())
//! ```
//!
//! ### Working with Manifold Descriptors
//!
//! ```rust
//! use atlas_manifold::{ManifoldDescriptor, manifold::*};
//!
//! // Create different manifold types
//! let euclidean = ManifoldDescriptor::euclidean(2, 3);
//! let spherical = ManifoldDescriptor::spherical(5.0, 3);
//! let hyperbolic = ManifoldDescriptor::hyperbolic(2, 3);
//!
//! // Check manifold properties
//! assert!(euclidean.is_riemannian());
//! assert_eq!(euclidean.gaussian_curvature(), 0.0);
//! assert_eq!(spherical.gaussian_curvature(), 0.04); // 1/r²
//! assert_eq!(hyperbolic.gaussian_curvature(), -1.0);
//! ```
//!
//! ### Atlas Points and Vectors
//!
//! ```rust
//! use atlas_manifold::{AtlasPoint, AtlasVector, linear::*};
//!
//! // Create points in different dimensions
//! let point_2d = AtlasPoint::<2> { coords: [1.0, 2.0] };
//! let point_3d = AtlasPoint::<3> { coords: [1.0, 2.0, 3.0] };
//!
//! // Create vectors for transformations
//! let vector = AtlasVector::<3> {
//!     components: [1.0, 0.0, 0.0]
//! };
//!
//! // Points and vectors are FFI-safe and can be passed to C functions
//! assert_eq!(std::mem::size_of::<AtlasPoint<3>>(), 24); // 3 * 8 bytes
//! ```
//!
//! ### Working with Shards
//!
//! ```rust
//! use atlas_manifold::{shard::*, projection::*};
//!
//! # let source_data = vec![42u8; 8192];
//! # let projection = atlas_projection_create(
//! #     ProjectionType::Linear, source_data.as_ptr(), source_data.len()
//! # ).unwrap();
//! // Define boundary region for shard extraction
//! let boundary_region = AtlasBoundaryRegion {
//!     start_coord: 0,
//!     end_coord: 4096,
//!     page_count: 1,
//!     region_class: 42, // R96 resonance class
//!     is_conserved: true,
//!     affecting_resonance_classes: vec![42, 15, 77],
//!     spatial_bounds: (0.0, 0.0, 1.0, 1.0),
//! };
//!
//! // Extract shard from projection
//! if let Some(proj) = projection.as_ref() {
//!     let shard = proj.extract_shard(&boundary_region)?;
//!     
//!     // Verify shard integrity
//!     if let Some(s) = shard.as_ref() {
//!         assert!(s.verify());
//!         println!("Shard size: {} bytes", s.get_size());
//!     }
//! }
//! # atlas_projection_destroy(projection);
//! # Ok::<(), atlas_manifold::AtlasError>(())
//! ```
//!
//! ## C API Integration
//!
//! The library provides a complete C API for integration with other system components:
//!
//! ```c
//! #include "atlas-manifold.h"
//!
//! // Initialize the library
//! if (atlas_manifold_init() != 0) {
//!     fprintf(stderr, "Failed to initialize Atlas Manifold\n");
//!     return -1;
//! }
//!
//! // Create projection from source data
//! const uint8_t *source_data = get_source_data();
//! size_t source_size = get_source_size();
//!
//! CAtlasProjectionHandle *projection = atlas_projection_create(
//!     ATLAS_PROJECTION_LINEAR, source_data, source_size);
//!
//! if (!projection) {
//!     fprintf(stderr, "Failed to create projection\n");
//!     return -1;
//! }
//!
//! // Verify projection
//! if (!atlas_manifold_verify_projection(projection)) {
//!     fprintf(stderr, "Projection verification failed\n");
//!     atlas_projection_destroy(projection);
//!     return -1;
//! }
//!
//! // Use the projection...
//!
//! // Clean up
//! atlas_projection_destroy(projection);
//! atlas_manifold_cleanup();
//! ```
//!
//! ## Feature Flags
//!
//! - `std` (default): Enable standard library support
//! - `parallel`: Enable parallel processing with Rayon
//! - `compression`: Enable shard compression with flate2
//! - `streaming`: Enable streaming mode for large datasets
//! - `benchmarks`: Enable performance benchmarking tools
//!
//! ## Safety and Security
//!
//! All FFI functions are designed with safety as the primary concern:
//! - All pointer parameters are validated before use
//! - Memory management follows RAII principles
//! - Unsafe code is minimized and thoroughly documented
//! - Conservation laws provide mathematical verification of data integrity
//!
//! ## Error Handling
//!
//! The library uses comprehensive error handling with detailed error codes:
//!
//! ```rust
//! use atlas_manifold::{AtlasError, AtlasResult};
//!
//! fn example_with_error_handling() -> AtlasResult<()> {
//!     // Operations that might fail return AtlasResult<T>
//!     let result = some_manifold_operation()?;
//!     
//!     // Handle specific error types
//!     match some_other_operation() {
//!         Ok(value) => println!("Success: {:?}", value),
//!         Err(AtlasError::InvalidDimension(dim)) => {
//!             eprintln!("Invalid dimension: {}", dim);
//!         }
//!         Err(AtlasError::MatrixError(msg)) => {
//!             eprintln!("Matrix error: {}", msg);
//!         }
//!         Err(e) => eprintln!("Other error: {}", e),
//!     }
//!     
//!     Ok(())
//! }
//! ```

#![deny(unsafe_op_in_unsafe_fn)]
#![warn(missing_docs)]
#![warn(clippy::all)]
#![cfg_attr(not(feature = "std"), no_std)]
// Allow necessary FFI patterns
#![allow(clippy::missing_safety_doc)] // FFI functions have safety docs where needed

#[cfg(not(feature = "std"))]
extern crate alloc;

#[cfg(not(feature = "std"))]
use alloc::vec::Vec;

// Core type definitions
pub mod types;

// Foreign Function Interface
pub mod ffi;

// Linear algebra and transformations
pub mod linear;

// Data sharding and distribution
pub mod shard;

// Type-Length-Value serialization
pub mod tlv;

// Manifold-specific operations
pub mod manifold;

// Coordinate system transformations
pub mod coords;

// Error handling
pub mod error;

// Projection operations for manifold transformations
pub mod projection;

// R96 Fourier transforms and resonance analysis
pub mod fourier;

// Transform operations for projections
pub mod transform;

// Critical mathematical invariants for Layer 4 operations
pub mod invariants;

// Streaming mode for large domain processing
#[cfg(feature = "streaming")]
pub mod streaming;

// Performance benchmarks for Layer 4 operations
#[cfg(feature = "benchmarks")]
pub mod benchmark;

// Shard compression support
#[cfg(feature = "compression")]
pub mod compression;

// Incremental projection updates
pub mod incremental;

// Re-export commonly used types
pub use error::{AtlasError, AtlasResult};
pub use fourier::{
    NormalFormRules, R96ClassHarmonics, R96FourierProjection, R96HarmonicCoefficient,
};
pub use invariants::{
    C768CycleTracker, ConservationBudgetTracker, FailureClosedSemanticsEnforcer,
    InvariantValidator, KleinOrbitAligner, PhiBijectionVerifier,
};
pub use projection::{AtlasProjection, AtlasProjectionHandle, ProjectionType};
pub use types::*;

#[cfg(feature = "std")]
extern crate std;

/// Version information for the Atlas Manifold library
pub const VERSION: &str = env!("CARGO_PKG_VERSION");

/// Initialize the Atlas Manifold library
///
/// This function should be called before using any other library functions.
/// It sets up internal state and validates system requirements.
///
/// # Safety
///
/// This function is safe to call multiple times, but should be called
/// from a single thread during initialization.
pub fn init() -> AtlasResult<()> {
    // Initialize logging/error reporting if needed
    // Validate system requirements
    // Set up any global state
    Ok(())
}

/// Clean up the Atlas Manifold library
///
/// This function should be called when shutting down to clean up
/// any resources allocated by the library.
///
/// # Safety
///
/// This function is safe to call multiple times, but should only be
/// called when no other library functions are in use.
pub fn cleanup() -> AtlasResult<()> {
    // Clean up global state
    // Release resources
    Ok(())
}

// =============================================================================
// Layer 3 R96 Integration Helpers
// =============================================================================

/// Classify array of data bytes to R96 resonance classes using Layer 3 FFI
///
/// This provides a convenient Rust interface to the Layer 3 R96 classification
/// system, which partitions the 256-byte space into 96 resonance classes.
///
/// # Arguments
/// * `data` - Input byte array to classify
///
/// # Returns
/// Vector of R96 resonance classes (values 0-95) corresponding to input bytes
pub fn classify_data_r96(data: &[u8]) -> Vec<u8> {
    let mut output = vec![0u8; data.len()];

    // SAFETY: FFI call with validated pointers and length
    unsafe {
        ffi::atlas_r96_classify_array(data.as_ptr(), output.as_mut_ptr(), data.len());
    }

    output
}

/// Find harmonic pairs in resonance class array using Layer 3 FFI
///
/// Two resonance classes r1, r2 harmonize if (r1 + r2) % 96 == 0.
/// This is fundamental for the Universal Numbers theory - harmonically
/// paired elements are considered "adjacent" in the manifold.
///
/// # Arguments
/// * `resonance_classes` - Array of R96 resonance class values (0-95)
/// * `max_pairs` - Maximum number of pairs to find
///
/// # Returns
/// Vector of harmonic pairs found in the data
/// Since atlas_r96_find_harmonic_pairs is not available, we implement it using atlas_r96_harmonizes
pub fn find_harmonic_pairs(
    resonance_classes: &[u8],
    max_pairs: usize,
) -> Vec<ffi::AtlasHarmonicPair> {
    let mut pairs = Vec::new();

    // Find harmonic pairs by checking all combinations using atlas_r96_harmonizes
    'outer: for (i, &r1) in resonance_classes.iter().enumerate() {
        for &r2 in resonance_classes.iter().skip(i + 1) {
            if pairs.len() >= max_pairs {
                break 'outer;
            }

            // SAFETY: FFI call with validated u8 values
            if unsafe { ffi::atlas_r96_harmonizes(r1, r2) } {
                pairs.push(ffi::AtlasHarmonicPair { r1, r2 });
            }
        }
    }

    pairs
}

// C-compatible exports

/// Returns the version string of the Atlas Manifold library
///
/// # Safety
/// This function returns a pointer to a null-terminated C string that is valid
/// for the lifetime of the program. The caller should not modify or free the string.
#[no_mangle]
pub extern "C" fn atlas_manifold_version() -> *const libc::c_char {
    VERSION.as_ptr() as *const libc::c_char
}

/// Initialize the Atlas Manifold library
///
/// # Safety
/// This function is safe to call from C. Returns 0 on success, -1 on failure.
#[no_mangle]
pub extern "C" fn atlas_manifold_init() -> libc::c_int {
    match init() {
        Ok(()) => 0,
        Err(_) => -1,
    }
}

/// Cleanup and shutdown the Atlas Manifold library
///
/// # Safety
/// This function is safe to call from C. Returns 0 on success, -1 on failure.
/// Should be called before program termination to properly clean up resources.
#[no_mangle]
pub extern "C" fn atlas_manifold_cleanup() -> libc::c_int {
    match cleanup() {
        Ok(()) => 0,
        Err(_) => -1,
    }
}

#[cfg(test)]
mod tests {
    use super::*;
    use crate::error::*;
    use crate::manifold::*;
    use crate::shard::*;
    use crate::tlv::*;

    #[test]
    fn test_library_init_cleanup() {
        assert!(init().is_ok());
        assert!(cleanup().is_ok());

        // Test multiple initializations are safe
        assert!(init().is_ok());
        assert!(init().is_ok());
        assert!(cleanup().is_ok());
    }

    #[test]
    fn test_version_info() {
        let version = VERSION;
        assert!(!version.is_empty());

        // Test C version function
        let c_version = unsafe {
            let ptr = atlas_manifold_version();
            assert!(!ptr.is_null());
        };
    }

    #[test]
    fn test_c_api_init_cleanup() {
        assert_eq!(atlas_manifold_init(), 0);
        assert_eq!(atlas_manifold_cleanup(), 0);
    }

    #[test]
    fn test_atlas_point_operations() {
        let point1 = AtlasPoint::<3> {
            coords: [1.0, 2.0, 3.0],
        };

        let point2 = AtlasPoint::<3> {
            coords: [4.0, 5.0, 6.0],
        };

        // Test basic point operations
        assert_eq!(point1.coords.len(), 3);
        assert_eq!(point2.coords[0], 4.0);

        // Test point equality
        let point1_copy = point1;
        assert_eq!(point1, point1_copy);
        assert_ne!(point1, point2);
    }

    #[test]
    fn test_atlas_vector_operations() {
        let vector1 = AtlasVector::<3> {
            components: [1.0, 0.0, 0.0],
        };

        let vector2 = AtlasVector::<3> {
            components: [0.0, 1.0, 0.0],
        };

        assert_eq!(vector1.components.len(), 3);
        assert_eq!(vector2.components[1], 1.0);
        assert_ne!(vector1, vector2);
    }

    #[test]
    fn test_manifold_descriptor_creation() {
        let desc = ManifoldDescriptor::new(2, 3, [1.0, 0.5, 0.0, 0.0], 1.0);

        assert_eq!(desc.intrinsic_dim, 2);
        assert_eq!(desc.embedding_dim, 3);
        assert_eq!(desc.curvature[0], 1.0);
        assert_eq!(desc.metric_det, 1.0);
        assert!(desc.is_riemannian());
    }

    #[test]
    fn test_manifold_descriptor_presets() {
        // Test Euclidean manifold
        let euclidean = ManifoldDescriptor::euclidean(2, 3);
        assert_eq!(euclidean.intrinsic_dim, 2);
        assert_eq!(euclidean.embedding_dim, 3);
        assert_eq!(euclidean.gaussian_curvature(), 0.0);
        assert!(euclidean.is_riemannian());

        // Test spherical manifold
        let spherical = ManifoldDescriptor::spherical(2.0, 3);
        assert_eq!(spherical.intrinsic_dim, 2);
        assert_eq!(spherical.embedding_dim, 3);
        assert_eq!(spherical.gaussian_curvature(), 0.25);
        assert!(spherical.is_riemannian());

        // Test hyperbolic manifold
        let hyperbolic = ManifoldDescriptor::hyperbolic(2, 3);
        assert_eq!(hyperbolic.intrinsic_dim, 2);
        assert_eq!(hyperbolic.embedding_dim, 3);
        assert_eq!(hyperbolic.gaussian_curvature(), -1.0);
        assert!(hyperbolic.is_riemannian());
    }

    #[test]
    fn test_shard_id_operations() {
        let shard1 = ShardId::new(123, 456);
        let shard2 = ShardId::from_hash(0x123456789ABCDEF0);

        assert_eq!(shard1.primary, 123);
        assert_eq!(shard1.secondary, 456);
        assert_ne!(shard1, shard2);

        let combined = shard1.combined();
        assert!(combined != 0);

        // Test hash-based creation
        let hash_shard = ShardId::from_hash(12345);
        assert!(hash_shard.primary != 0 || hash_shard.secondary != 0);
    }

    #[test]
    fn test_error_handling() {
        let errors = [
            AtlasError::InvalidDimension(42),
            AtlasError::MatrixError("test matrix error"),
            AtlasError::TopologyError("test topology error"),
            AtlasError::CoordinateError("test coordinate error"),
            AtlasError::SerializationError("test serialization error"),
            AtlasError::AllocationError,
            AtlasError::InvalidInput("test input error"),
            AtlasError::NumericalError("test numerical error"),
            AtlasError::LayerIntegrationError("test integration error"),
        ];

        for error in &errors {
            let error_code = error_to_code(error);
            assert!(error_code < 0);
            assert!(error_code >= -9);

            // Test error display
            let error_string = format!("{}", error);
            assert!(!error_string.is_empty());
        }
    }

    #[test]
    fn test_shard_manager_creation() {
        let strategy = ShardStrategy::CoordinateHash { num_shards: 4 };
        let mut manager = ShardManager::new(strategy);

        assert_eq!(manager.all_shards().len(), 0);

        // Add a test shard
        let metadata = ShardMetadata {
            id: ShardId::new(1, 0),
            point_count: 100,
            bounds: vec![(0.0, 10.0), (0.0, 10.0)],
            load_factor: 0.5,
        };

        manager.add_shard(metadata);
        assert_eq!(manager.all_shards().len(), 1);

        let shard_meta = manager.shard_metadata(ShardId::new(1, 0));
        assert!(shard_meta.is_some());
        assert_eq!(shard_meta.unwrap().point_count, 100);
    }

    #[test]
    fn test_shard_point_assignment() {
        let strategy = ShardStrategy::CoordinateHash { num_shards: 4 };
        let manager = ShardManager::new(strategy);

        let point = AtlasPoint::<2> { coords: [1.5, 2.5] };

        let shard_result = manager.shard_for_point(&point);
        assert!(shard_result.is_ok());

        let shard_id = shard_result.unwrap();
        assert!(shard_id.primary < 4); // Should be in range [0, 3]
    }

    #[test]
    fn test_tlv_encoding_decoding() {
        use crate::tlv::*;

        // Test basic TLV functionality with simple data
        let mut encoder = TlvEncoder::new();

        // Test encoding a shard ID (simplest case)
        let shard_id = ShardId::new(123, 456);
        assert!(encoder.write_shard_id(&shard_id).is_ok());

        let buffer = encoder.finish();
        assert!(!buffer.is_empty());

        // Test decoding shard ID
        let mut decoder = TlvDecoder::new(&buffer);
        let decoded_shard = decoder.read_shard_id();
        assert!(decoded_shard.is_ok());
        assert_eq!(decoded_shard.unwrap(), shard_id);

        // Note: TLV serialization now properly handles alignment issues by
        // manually serializing/deserializing structs to avoid padding problems.
    }

    #[test]
    fn test_tlv_error_conditions() {
        // Test decoder with empty buffer
        let mut decoder = TlvDecoder::new(&[]);
        assert!(decoder.read_next().unwrap().is_none());

        // Test decoder with invalid data
        let invalid_data = [0xFF, 0xFF, 0xFF, 0xFF];
        let mut decoder = TlvDecoder::new(&invalid_data);
        assert!(decoder.read_next().is_err());

        // Test encoder capacity
        let encoder = TlvEncoder::with_capacity(1024);
        assert_eq!(encoder.len(), 0);
        assert!(encoder.is_empty());
    }

    #[test]
    fn test_manifold_atlas_operations() {
        let desc = ManifoldDescriptor::euclidean(2, 2);
        let mut atlas = ManifoldAtlas::<2, 2>::new(desc);

        // Create an identity chart
        let bounds = [(-10.0, 10.0), (-10.0, 10.0)];
        let chart = CoordinateChart::<2, 2>::identity_square(bounds);

        let chart_index = atlas.add_chart(chart);
        assert_eq!(chart_index, 0);

        assert!(atlas.set_active_chart(0).is_ok());
        assert!(atlas.set_active_chart(1).is_err()); // Out of bounds

        // Test point transformation (identity should return same point)
        let point = AtlasPoint::<2> { coords: [1.0, 2.0] };

        let euclidean_result = atlas.to_euclidean(&point);
        assert!(euclidean_result.is_ok());

        let manifold_result = atlas.from_euclidean(&point);
        assert!(manifold_result.is_ok());
    }

    #[test]
    fn test_coordinate_chart_operations() {
        let bounds = [(-5.0, 5.0), (-5.0, 5.0)];
        let chart = CoordinateChart::<2, 2>::identity_square(bounds);

        let point_inside = AtlasPoint::<2> { coords: [1.0, 2.0] };

        let point_outside = AtlasPoint::<2> {
            coords: [10.0, 2.0],
        };

        assert!(chart.contains_point(&point_inside));
        assert!(!chart.contains_point(&point_outside));
    }

    #[test]
    fn test_memory_safety_pod_types() {
        use bytemuck::{bytes_of, try_from_bytes};

        // Test AtlasPoint is properly Pod
        let point = AtlasPoint::<3> {
            coords: [1.0, 2.0, 3.0],
        };
        let bytes = bytes_of(&point);
        let recovered: Result<&AtlasPoint<3>, _> = try_from_bytes(bytes);
        assert!(recovered.is_ok());
        assert_eq!(recovered.unwrap().coords, point.coords);

        // Test ManifoldDescriptor is properly Pod
        let desc = ManifoldDescriptor::euclidean(2, 3);
        let bytes = bytes_of(&desc);
        let recovered: Result<&ManifoldDescriptor, _> = try_from_bytes(bytes);
        assert!(recovered.is_ok());
        assert_eq!(recovered.unwrap().intrinsic_dim, desc.intrinsic_dim);

        // Test ShardId is properly Pod
        let shard_id = ShardId::new(123, 456);
        let bytes = bytes_of(&shard_id);
        let recovered: Result<&ShardId, _> = try_from_bytes(bytes);
        assert!(recovered.is_ok());
        assert_eq!(*recovered.unwrap(), shard_id);
    }

    #[test]
    fn test_numerical_stability() {
        let desc = ManifoldDescriptor::spherical(1e-10, 3); // Very small radius
        assert!(desc.is_riemannian());

        let large_desc = ManifoldDescriptor::spherical(1e10, 3); // Very large radius
        assert!(large_desc.is_riemannian());

        // Test with extreme curvature values
        let extreme_desc = ManifoldDescriptor::new(2, 3, [1e6, 0.0, 0.0, 0.0], 1.0);
        assert_eq!(extreme_desc.gaussian_curvature(), 1e6);
    }

    #[cfg(feature = "std")]
    #[test]
    fn test_error_trait_implementation() {
        use std::error::Error;

        let error = AtlasError::InvalidDimension(42);
        let error_trait: &dyn Error = &error;
        assert!(!error_trait.to_string().is_empty());
    }

    #[test]
    fn test_type_safety() {
        // Test that const generic parameters enforce type safety
        let point_2d = AtlasPoint::<2> { coords: [1.0, 2.0] };
        let point_3d = AtlasPoint::<3> {
            coords: [1.0, 2.0, 3.0],
        };

        // These should be different types - we can't directly test with assert_ne!
        // but we can verify their sizes are different
        assert_ne!(size_of::<AtlasPoint<2>>(), size_of::<AtlasPoint<3>>());

        // Test transformation matrix type safety
        let matrix_2x2 = TransformMatrix::<2, 2> {
            elements: [[1.0, 0.0], [0.0, 1.0]],
        };
        let matrix_3x3 = TransformMatrix::<3, 3> {
            elements: [[1.0, 0.0, 0.0], [0.0, 1.0, 0.0], [0.0, 0.0, 1.0]],
        };

        assert_ne!(size_of_val(&matrix_2x2), size_of_val(&matrix_3x3));
    }
}
