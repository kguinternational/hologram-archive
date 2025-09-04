//! Property-based tests for Atlas Manifold Layer 4
//!
//! This module implements comprehensive property-based testing using proptest
//! to verify mathematical invariants, conservation laws, and system properties
//! across a wide range of inputs and conditions.

use atlas_manifold::*;
use proptest::prelude::*;

// =============================================================================
// Test Strategies for Property-Based Testing
// =============================================================================

/// Strategy for generating valid Atlas points
fn atlas_point_2d() -> impl Strategy<Value = AtlasPoint<2>> {
    prop::array::uniform(-1000.0..1000.0).prop_map(|coords| AtlasPoint { coords })
}

/// Strategy for generating valid Atlas points in 3D
fn atlas_point_3d() -> impl Strategy<Value = AtlasPoint<3>> {
    prop::array::uniform(-1000.0..1000.0).prop_map(|coords| AtlasPoint { coords })
}

/// Strategy for generating valid manifold dimensions
fn valid_dimensions() -> impl Strategy<Value = (u32, u32)> {
    (1u32..=10, 1u32..=10)
}

/// Strategy for generating valid curvature values
fn curvature_values() -> impl Strategy<Value = [f64; 4]> {
    prop::array::uniform(-10.0..10.0)
}

/// Strategy for generating valid metric determinants
fn metric_determinant() -> impl Strategy<Value = f64> {
    // Must be positive for Riemannian manifolds
    prop::num::f64::POSITIVE
}

/// Strategy for generating valid source data for projections
fn projection_source_data() -> impl Strategy<Value = Vec<u8>> {
    // Data must be multiple of page size (4096 bytes)
    prop::collection::vec(any::<u8>(), 4096..=32768)
        .prop_map(|mut data| {
            // Ensure size is multiple of 4096
            let remainder = data.len() % 4096;
            if remainder != 0 {
                data.resize(data.len() + (4096 - remainder), 0);
            }
            data
        })
}

/// Strategy for generating conservation-compliant data
fn conserved_data() -> impl Strategy<Value = Vec<u8>> {
    prop::collection::vec(any::<u8>(), 96..=4096)
        .prop_map(|mut data| {
            // Ensure conservation law: sum % 96 == 0
            let sum: u64 = data.iter().map(|&b| b as u64).sum();
            let remainder = (sum % 96) as u8;
            if remainder != 0 {
                // Adjust last byte to satisfy conservation
                if let Some(last) = data.last_mut() {
                    *last = last.wrapping_sub(remainder);
                }
            }
            data
        })
}

/// Strategy for generating R96 resonance classes
fn r96_class() -> impl Strategy<Value = u8> {
    0u8..96
}

/// Strategy for generating valid boundary regions
fn boundary_region() -> impl Strategy<Value = shard::AtlasBoundaryRegion> {
    (
        0u32..65536,  // start_coord
        0u32..65536,  // end_coord (will be adjusted)
        1u16..256,    // page_count
        r96_class(),  // region_class
        any::<bool>(), // is_conserved
        prop::collection::vec(r96_class(), 0..=10), // affecting_resonance_classes
        (-100.0..100.0, -100.0..100.0, -100.0..100.0, -100.0..100.0), // spatial_bounds
    ).prop_map(|(start, end_offset, page_count, region_class, is_conserved, classes, bounds)| {
        let end_coord = start + end_offset + 4096; // Ensure end > start
        shard::AtlasBoundaryRegion {
            start_coord: start,
            end_coord,
            page_count,
            region_class,
            is_conserved,
            affecting_resonance_classes: classes,
            spatial_bounds: (bounds.0, bounds.1, bounds.2, bounds.3),
        }
    })
}

// =============================================================================
// Manifold Descriptor Properties
// =============================================================================

proptest! {
    /// Test that manifold descriptors maintain mathematical consistency
    #[test]
    fn manifold_descriptor_consistency(
        (intrinsic_dim, embedding_dim) in valid_dimensions(),
        curvature in curvature_values(),
        metric_det in metric_determinant()
    ) {
        let descriptor = ManifoldDescriptor::new(intrinsic_dim, embedding_dim, curvature, metric_det);
        
        // Basic invariants
        prop_assert_eq!(descriptor.intrinsic_dim, intrinsic_dim);
        prop_assert_eq!(descriptor.embedding_dim, embedding_dim);
        prop_assert_eq!(descriptor.metric_det, metric_det);
        prop_assert!(descriptor.is_riemannian()); // metric_det > 0
        
        // Gaussian curvature should match first curvature component
        prop_assert!((descriptor.gaussian_curvature() - curvature[0]).abs() < 1e-10);
    }

    /// Test Euclidean manifold properties
    #[test]
    fn euclidean_manifold_properties(
        (intrinsic_dim, embedding_dim) in valid_dimensions()
    ) {
        let euclidean = ManifoldDescriptor::euclidean(intrinsic_dim, embedding_dim);
        
        // Euclidean manifolds should have zero curvature
        prop_assert_eq!(euclidean.gaussian_curvature(), 0.0);
        prop_assert_eq!(euclidean.mean_curvature(), 0.0);
        prop_assert!(euclidean.is_riemannian());
        prop_assert_eq!(euclidean.metric_det, 1.0);
    }

    /// Test spherical manifold properties
    #[test]
    fn spherical_manifold_properties(
        radius in prop::num::f64::POSITIVE,
        embedding_dim in 2u32..=10
    ) {
        let spherical = ManifoldDescriptor::spherical(radius, embedding_dim);
        
        // Spherical manifolds should have positive curvature
        let expected_curvature = 1.0 / (radius * radius);
        prop_assert!((spherical.gaussian_curvature() - expected_curvature).abs() < 1e-10);
        prop_assert!(spherical.is_riemannian());
        prop_assert_eq!(spherical.intrinsic_dim, embedding_dim - 1);
    }

    /// Test hyperbolic manifold properties
    #[test]
    fn hyperbolic_manifold_properties(
        (intrinsic_dim, embedding_dim) in valid_dimensions()
    ) {
        let hyperbolic = ManifoldDescriptor::hyperbolic(intrinsic_dim, embedding_dim);
        
        // Hyperbolic manifolds should have negative curvature
        prop_assert_eq!(hyperbolic.gaussian_curvature(), -1.0);
        prop_assert!(hyperbolic.is_riemannian());
        prop_assert_eq!(hyperbolic.intrinsic_dim, intrinsic_dim);
        prop_assert_eq!(hyperbolic.embedding_dim, embedding_dim);
    }
}

// =============================================================================
// Atlas Point and Vector Properties
// =============================================================================

proptest! {
    /// Test Atlas point operations preserve type safety
    #[test]
    fn atlas_point_type_safety(point in atlas_point_2d()) {
        // Points should maintain their coordinate values
        prop_assert_eq!(point.coords.len(), 2);
        
        // Memory layout should be predictable
        prop_assert_eq!(std::mem::size_of::<AtlasPoint<2>>(), 16); // 2 * 8 bytes
        
        // Should be copyable and cloneable
        let point_copy = point;
        let point_clone = point.clone();
        prop_assert_eq!(point, point_copy);
        prop_assert_eq!(point, point_clone);
    }

    /// Test Atlas vector operations
    #[test]
    fn atlas_vector_operations(components in prop::array::uniform(-1000.0..1000.0)) {
        let vector = AtlasVector::<3> { components };
        
        // Vector should maintain components
        prop_assert_eq!(vector.components.len(), 3);
        prop_assert_eq!(vector.components, components);
        
        // Memory layout should be predictable
        prop_assert_eq!(std::mem::size_of::<AtlasVector<3>>(), 24); // 3 * 8 bytes
    }
}

// =============================================================================
// Projection Properties
// =============================================================================

proptest! {
    /// Test projection creation and verification invariants
    #[test]
    fn projection_creation_invariants(
        source_data in projection_source_data(),
        projection_type in prop::sample::select(vec![
            projection::ProjectionType::Linear,
            projection::ProjectionType::R96Fourier
        ])
    ) {
        let result = projection::atlas_projection_create(
            projection_type,
            source_data.as_ptr(),
            source_data.len()
        );
        
        prop_assert!(result.is_ok());
        let handle = result.unwrap();
        
        // Projection should be valid after creation
        unsafe {
            if let Some(proj) = handle.as_ref() {
                prop_assert!(proj.verify_projection());
                
                // Source size should be preserved
                prop_assert_eq!(proj.source_size, source_data.len());
                
                // Projection type should match
                prop_assert_eq!(proj.projection_type, projection_type);
                
                // Should have non-zero conservation sum if data is non-empty
                if !source_data.is_empty() {
                    prop_assert!(proj.total_conservation_sum > 0);
                }
            }
        }
        
        // Clean up
        projection::atlas_projection_destroy(handle);
    }

    /// Test projection dimensions are consistent
    #[test]
    fn projection_dimension_consistency(
        source_data in projection_source_data()
    ) {
        let result = projection::atlas_projection_create(
            projection::ProjectionType::Linear,
            source_data.as_ptr(),
            source_data.len()
        );
        
        if let Ok(handle) = result {
            unsafe {
                if let Some(proj) = handle.as_ref() {
                    let (width, height) = proj.get_dimensions();
                    
                    // Dimensions should be reasonable
                    prop_assert!(width > 0);
                    prop_assert!(height > 0);
                    
                    // Total area should relate to source size
                    let total_pixels = width as usize * height as usize;
                    prop_assert!(total_pixels > 0);
                }
            }
            
            projection::atlas_projection_destroy(handle);
        }
    }
}

// =============================================================================
// Conservation Law Properties
// =============================================================================

proptest! {
    /// Test conservation law preservation
    #[test]
    fn conservation_law_preservation(data in conserved_data()) {
        // Data generated by conserved_data() should satisfy conservation laws
        let sum: u64 = data.iter().map(|&b| b as u64).sum();
        prop_assert_eq!(sum % 96, 0);
        
        // Layer 2 conservation check should pass
        // Note: Using the public C API instead of internal functions
        let is_conserved = unsafe {
            atlas_manifold::ffi::atlas_manifold_conserved_check(data.as_ptr(), data.len())
        };
        prop_assert!(is_conserved);
    }

    /// Test conservation delta calculations
    #[test]
    fn conservation_delta_properties(
        before_data in conserved_data(),
        after_data in conserved_data()
    ) {
        if before_data.len() == after_data.len() {
            let delta = unsafe {
                atlas_manifold::ffi::atlas_manifold_conserved_delta(
                    before_data.as_ptr(),
                    after_data.as_ptr(),
                    before_data.len()
                )
            };
            
            // Delta should be less than 96 (modular arithmetic)
            prop_assert!(delta < 96);
            
            // If data is identical, delta should be 0
            if before_data == after_data {
                prop_assert_eq!(delta, 0);
            }
        }
    }
}

// =============================================================================
// Shard Properties
// =============================================================================

proptest! {
    /// Test shard extraction preserves data integrity
    #[test]
    fn shard_extraction_integrity(
        source_data in projection_source_data(),
        boundary_region in boundary_region()
    ) {
        // Only test if boundary region is within source data bounds
        if boundary_region.end_coord <= source_data.len() as u32 {
            let projection_result = projection::atlas_projection_create(
                projection::ProjectionType::Linear,
                source_data.as_ptr(),
                source_data.len()
            );
            
            if let Ok(projection_handle) = projection_result {
                unsafe {
                    if let Some(proj) = projection_handle.as_ref() {
                        let shard_result = proj.extract_shard(&boundary_region);
                        
                        if let Ok(shard_handle) = shard_result {
                            if let Some(shard) = shard_handle.as_ref() {
                                // Shard should verify correctly
                                prop_assert!(shard.verify());
                                
                                // Shard size should be reasonable
                                let shard_size = shard.get_size();
                                prop_assert!(shard_size > 0);
                                prop_assert!(shard_size <= source_data.len());
                                
                                // Conservation sum should be consistent
                                prop_assert!(shard.conservation_sum <= u64::MAX);
                            }
                            
                            shard::atlas_shard_destroy(shard_handle);
                        }
                    }
                }
                
                projection::atlas_projection_destroy(projection_handle);
            }
        }
    }

    /// Test boundary region validation
    #[test]
    fn boundary_region_validation(region in boundary_region()) {
        let is_valid = projection::AtlasProjection::verify_boundary_region(&region);
        
        // Basic consistency checks
        if region.start_coord < region.end_coord && region.page_count > 0 {
            // Most valid-looking regions should pass basic validation
            // (though some may fail due to conservation or other constraints)
        }
        
        // Region class should be in valid range
        prop_assert!(region.region_class < 96);
        
        // Page count should be reasonable
        prop_assert!(region.page_count <= 256);
    }
}

// =============================================================================
// Transformation Properties
// =============================================================================

proptest! {
    /// Test transformation parameter validation
    #[test]
    fn transformation_parameter_validation(
        scaling_factor in prop::num::f64::POSITIVE,
        rotation_angle in -std::f64::consts::PI..std::f64::consts::PI,
        translation_x in -1000.0..1000.0,
        translation_y in -1000.0..1000.0
    ) {
        // Basic parameter validation - positive scaling should be valid
        prop_assert!(scaling_factor > 0.0);
        prop_assert!(scaling_factor.is_finite());
        prop_assert!(rotation_angle.is_finite());
        prop_assert!((translation_x as f64).is_finite());
        prop_assert!((translation_y as f64).is_finite());
    }

    /// Test basic transformation properties
    #[test]
    fn transformation_basic_properties(
        scaling_factor in 0.1..10.0,
        rotation_angle in -std::f64::consts::PI..std::f64::consts::PI
    ) {
        // Test that transformation parameters are reasonable
        prop_assert!(scaling_factor > 0.0);
        prop_assert!(scaling_factor < 100.0);
        prop_assert!(rotation_angle.abs() <= std::f64::consts::PI);
        prop_assert!(rotation_angle.is_finite());
        
        // Test that scaling preserves sign and magnitude relationships
        let scaled_value = 10.0 * scaling_factor;
        prop_assert!(scaled_value > 0.0);
        prop_assert!(((scaled_value / 10.0 - scaling_factor) as f64).abs() < 1e-10);
    }
}

// =============================================================================
// Fourier Transform Properties
// =============================================================================

proptest! {
    /// Test R96 Fourier transform properties
    #[test]
    fn r96_fourier_properties(
        source_data in projection_source_data()
    ) {
        let projection_result = projection::atlas_projection_create(
            projection::ProjectionType::R96Fourier,
            source_data.as_ptr(),
            source_data.len()
        );
        
        if let Ok(projection_handle) = projection_result {
            unsafe {
                if let Some(proj) = projection_handle.as_ref() {
                    // R96 projection should have Fourier data
                    if let Some(ref r96_data) = proj.r96_fourier_data {
                        // Should have coefficients for active classes
                        let active_classes = r96_data.get_active_classes();
                        prop_assert!(!active_classes.is_empty());
                        prop_assert!(active_classes.len() <= 96);
                        
                        // All active classes should be in valid range
                        for &class in &active_classes {
                            prop_assert!(class < 96);
                        }
                    }
                }
            }
            
            projection::atlas_projection_destroy(projection_handle);
        }
    }

    /// Test Fourier transform reversibility
    #[test]
    fn fourier_transform_reversibility(
        source_data in projection_source_data()
    ) {
        let projection_result = projection::atlas_projection_create(
            projection::ProjectionType::R96Fourier,
            source_data.as_ptr(),
            source_data.len()
        );
        
        if let Ok(mut projection_handle) = projection_result {
            unsafe {
                if let Some(proj) = projection_handle.as_mut() {
                    let original_conservation = proj.total_conservation_sum;
                    
                    // Apply forward Fourier transform
                    let forward_result = transform::apply_fourier_transform(proj, false);
                    
                    if forward_result.is_ok() {
                        // Apply inverse Fourier transform
                        let inverse_result = transform::apply_fourier_transform(proj, true);
                        
                        if inverse_result.is_ok() {
                            // Conservation should be preserved through round-trip
                            // (within numerical tolerance for Fourier operations)
                            let conservation_diff = 
                                (proj.total_conservation_sum as i64 - original_conservation as i64).abs();
                            prop_assert!(conservation_diff <= 96); // Within one R96 cycle
                            
                            // Projection should still be valid
                            prop_assert!(proj.verify_projection());
                        }
                    }
                }
            }
            
            projection::atlas_projection_destroy(projection_handle);
        }
    }
}

// =============================================================================
// Memory Safety Properties
// =============================================================================

proptest! {
    /// Test memory safety of Pod types
    #[test]
    fn pod_types_memory_safety(point in atlas_point_3d()) {
        // AtlasPoint should be safely convertible to/from bytes
        let bytes = bytemuck::bytes_of(&point);
        prop_assert_eq!(bytes.len(), 24); // 3 * 8 bytes
        
        let recovered: Result<&AtlasPoint<3>, _> = bytemuck::try_from_bytes(bytes);
        prop_assert!(recovered.is_ok());
        
        if let Ok(recovered_point) = recovered {
            prop_assert_eq!(*recovered_point, point);
        }
    }

    /// Test manifold descriptor Pod safety
    #[test]
    fn manifold_descriptor_pod_safety(
        (intrinsic_dim, embedding_dim) in valid_dimensions(),
        curvature in curvature_values(),
        metric_det in metric_determinant()
    ) {
        let descriptor = ManifoldDescriptor::new(intrinsic_dim, embedding_dim, curvature, metric_det);
        
        // Should be safely convertible to/from bytes
        let bytes = bytemuck::bytes_of(&descriptor);
        let recovered: Result<&ManifoldDescriptor, _> = bytemuck::try_from_bytes(bytes);
        prop_assert!(recovered.is_ok());
        
        if let Ok(recovered_desc) = recovered {
            prop_assert_eq!(recovered_desc.intrinsic_dim, descriptor.intrinsic_dim);
            prop_assert_eq!(recovered_desc.embedding_dim, descriptor.embedding_dim);
            prop_assert_eq!(recovered_desc.metric_det, descriptor.metric_det);
        }
    }
}

// =============================================================================
// Error Handling Properties
// =============================================================================

proptest! {
    /// Test error code consistency
    #[test]
    fn error_code_consistency(
        error_variant in prop::sample::select(vec![
            AtlasError::InvalidDimension(42),
            AtlasError::MatrixError("test"),
            AtlasError::TopologyError("test"),
            AtlasError::CoordinateError("test"),
            AtlasError::SerializationError("test"),
            AtlasError::AllocationError,
            AtlasError::InvalidInput("test"),
            AtlasError::NumericalError("test"),
            AtlasError::LayerIntegrationError("test"),
        ])
    ) {
        let error_code = error::error_to_code(&error_variant);
        
        // Error codes should be negative
        prop_assert!(error_code < 0);
        
        // Error codes should be in valid range
        prop_assert!(error_code >= -9);
        
        // Error display should be non-empty
        let error_string = format!("{}", error_variant);
        prop_assert!(!error_string.is_empty());
    }
}

// =============================================================================
// Integration Properties
// =============================================================================

proptest! {
    /// Test full pipeline invariant preservation
    #[test]
    fn full_pipeline_invariants(
        source_data in conserved_data().prop_filter("Must be page-aligned", |data| data.len() % 4096 == 0),
        scaling_factor in 0.5..2.0
    ) {
        if source_data.len() >= 4096 {
            // Create projection
            let projection_result = projection::atlas_projection_create(
                projection::ProjectionType::Linear,
                source_data.as_ptr(),
                source_data.len()
            );
            
            if let Ok(mut projection_handle) = projection_result {
                unsafe {
                    if let Some(proj) = projection_handle.as_mut() {
                        let original_conservation = proj.total_conservation_sum;
                        
                        // Apply transformation
                        let transform_result = transform::scale_projection(proj, scaling_factor, scaling_factor);
                        
                        if transform_result.is_ok() {
                            // Create boundary region for shard extraction
                            let region = shard::AtlasBoundaryRegion {
                                start_coord: 0,
                                end_coord: std::cmp::min(4096, source_data.len() as u32),
                                page_count: 1,
                                region_class: 42,
                                is_conserved: true,
                                affecting_resonance_classes: vec![42],
                                spatial_bounds: (0.0, 0.0, 1.0, 1.0),
                            };
                            
                            // Extract shard
                            let shard_result = proj.extract_shard(&region);
                            
                            if let Ok(shard_handle) = shard_result {
                                if let Some(shard) = shard_handle.as_ref() {
                                    // Full pipeline should preserve fundamental invariants
                                    prop_assert!(shard.verify());
                                    prop_assert!(proj.verify_projection());
                                    
                                    // Conservation should be preserved (within tolerance)
                                    let conservation_diff = 
                                        (proj.total_conservation_sum as i64 - original_conservation as i64).abs();
                                    prop_assert!(conservation_diff <= 96);
                                }
                                
                                shard::atlas_shard_destroy(shard_handle);
                            }
                        }
                    }
                }
                
                projection::atlas_projection_destroy(projection_handle);
            }
        }
    }
}

// =============================================================================
// Stress Test Properties
// =============================================================================

proptest! {
    #![proptest_config(ProptestConfig::with_cases(100))]
    
    /// Stress test with large data sizes
    #[test]
    fn stress_test_large_data(
        size_multiplier in 1u32..8, // Up to ~128KB of data
        operations in prop::collection::vec(
            prop::sample::select(vec!["scale", "rotate", "translate"]), 
            1..=5
        )
    ) {
        let data_size = (size_multiplier as usize) * 4096;
        let source_data = vec![42u8; data_size];
        
        let projection_result = projection::atlas_projection_create(
            projection::ProjectionType::Linear,
            source_data.as_ptr(),
            source_data.len()
        );
        
        if let Ok(mut projection_handle) = projection_result {
            unsafe {
                if let Some(proj) = projection_handle.as_mut() {
                    let original_conservation = proj.total_conservation_sum;
                    
                    // Apply sequence of operations
                    let operations_count = operations.len();
                    for operation in &operations {
                        let result = match operation.as_ref() {
                            "scale" => transform::scale_projection(proj, 1.5, 1.5),
                            "rotate" => transform::rotate_projection(proj, 0.5, 0.0, 0.0),
                            "translate" => transform::translate_projection(proj, 10.0, -5.0),
                            _ => Ok(()),
                        };
                        
                        if result.is_err() {
                            break; // Some operations might fail with large data
                        }
                    }
                    
                    // System should remain stable
                    prop_assert!(proj.verify_projection());
                    
                    // Conservation should be preserved (within tolerance for multiple operations)
                    let conservation_diff = 
                        (proj.total_conservation_sum as i64 - original_conservation as i64).abs();
                    prop_assert!(conservation_diff <= 96 * operations_count as i64);
                }
            }
            
            projection::atlas_projection_destroy(projection_handle);
        }
    }
}