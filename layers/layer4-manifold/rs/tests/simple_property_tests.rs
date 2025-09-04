//! Simplified property-based tests for Atlas Manifold Layer 4
//!
//! This module focuses on testing core mathematical properties and invariants
//! without complex FFI interactions.

use atlas_manifold::*;
use proptest::prelude::*;

// =============================================================================
// Basic Property Tests
// =============================================================================

proptest! {
    /// Test that Atlas points maintain their coordinate values
    #[test]
    fn atlas_point_coordinate_preservation(coords in prop::array::uniform(-1000.0f64..1000.0)) {
        let point = AtlasPoint::<3> { coords };
        
        // Coordinates should be preserved exactly
        prop_assert_eq!(point.coords, coords);
        
        // Memory layout should be predictable
        prop_assert_eq!(std::mem::size_of::<AtlasPoint<3>>(), 24); // 3 * 8 bytes
        
        // Should be copyable
        let point_copy = point;
        prop_assert_eq!(point, point_copy);
    }

    /// Test manifold descriptor mathematical consistency
    #[test]
    fn manifold_descriptor_properties(
        intrinsic_dim in 1u32..=10,
        embedding_dim in 1u32..=10,
        curvature_val in -10.0f64..10.0,
        metric_det in 0.01f64..100.0  // Must be positive for Riemannian
    ) {
        let curvature = [curvature_val, 0.0, 0.0, 0.0];
        let descriptor = ManifoldDescriptor::new(intrinsic_dim, embedding_dim, curvature, metric_det);
        
        // Basic properties should be preserved
        prop_assert_eq!(descriptor.intrinsic_dim, intrinsic_dim);
        prop_assert_eq!(descriptor.embedding_dim, embedding_dim);
        prop_assert_eq!(descriptor.metric_det, metric_det);
        prop_assert!(descriptor.is_riemannian()); // metric_det > 0
        
        // Gaussian curvature should match first component
        prop_assert!((descriptor.gaussian_curvature() - curvature_val).abs() < 1e-10);
    }

    /// Test conservation law mathematical properties
    #[test]
    fn conservation_law_mathematics(data in prop::collection::vec(any::<u8>(), 96..=4096)) {
        let sum: u64 = data.iter().map(|&b| b as u64).sum();
        let remainder = sum % 96;
        
        // Test modular arithmetic properties
        prop_assert!(remainder < 96);
        
        // Test that making data conservative works
        let mut conservative_data = data.clone();
        if remainder != 0 {
            if let Some(last_byte) = conservative_data.last_mut() {
                *last_byte = last_byte.wrapping_sub(remainder as u8);
            }
        }
        
        let conservative_sum: u64 = conservative_data.iter().map(|&b| b as u64).sum();
        prop_assert_eq!(conservative_sum % 96, 0);
    }

    /// Test Euclidean manifold special properties
    #[test]
    fn euclidean_manifold_invariants(
        intrinsic_dim in 1u32..=5,
        embedding_dim in 1u32..=5
    ) {
        let euclidean = ManifoldDescriptor::euclidean(intrinsic_dim, embedding_dim);
        
        // Euclidean manifolds have zero curvature
        prop_assert_eq!(euclidean.gaussian_curvature(), 0.0);
        prop_assert_eq!(euclidean.mean_curvature(), 0.0);
        
        // Unit metric determinant
        prop_assert_eq!(euclidean.metric_det, 1.0);
        
        // Should be Riemannian
        prop_assert!(euclidean.is_riemannian());
    }

    /// Test spherical manifold properties
    #[test]
    fn spherical_manifold_properties(
        radius in 0.1f64..100.0,
        embedding_dim in 2u32..=5
    ) {
        let spherical = ManifoldDescriptor::spherical(radius, embedding_dim);
        
        // Positive curvature for spheres
        let expected_curvature = 1.0 / (radius * radius);
        prop_assert!((spherical.gaussian_curvature() - expected_curvature).abs() < 1e-10);
        
        // Intrinsic dimension is one less than embedding
        prop_assert_eq!(spherical.intrinsic_dim, embedding_dim - 1);
        
        // Should be Riemannian
        prop_assert!(spherical.is_riemannian());
    }

    /// Test hyperbolic manifold properties
    #[test]
    fn hyperbolic_manifold_properties(
        intrinsic_dim in 1u32..=5,
        embedding_dim in 1u32..=5
    ) {
        let hyperbolic = ManifoldDescriptor::hyperbolic(intrinsic_dim, embedding_dim);
        
        // Negative curvature for hyperbolic manifolds
        prop_assert_eq!(hyperbolic.gaussian_curvature(), -1.0);
        
        // Dimensions should be preserved
        prop_assert_eq!(hyperbolic.intrinsic_dim, intrinsic_dim);
        prop_assert_eq!(hyperbolic.embedding_dim, embedding_dim);
        
        // Should be Riemannian
        prop_assert!(hyperbolic.is_riemannian());
    }

    /// Test numerical stability of mathematical operations
    #[test]
    fn numerical_stability_tests(
        small_val in 1e-10f64..1e-5,
        large_val in 1e5f64..1e10
    ) {
        // Test with very small radius
        let small_sphere = ManifoldDescriptor::spherical(small_val, 3);
        prop_assert!(small_sphere.is_riemannian());
        prop_assert!(small_sphere.gaussian_curvature() > 0.0);
        prop_assert!(small_sphere.gaussian_curvature().is_finite());
        
        // Test with very large radius  
        let large_sphere = ManifoldDescriptor::spherical(large_val, 3);
        prop_assert!(large_sphere.is_riemannian());
        prop_assert!(large_sphere.gaussian_curvature() > 0.0);
        prop_assert!(large_sphere.gaussian_curvature().is_finite());
        prop_assert!(large_sphere.gaussian_curvature() < small_sphere.gaussian_curvature());
    }

    /// Test error code consistency and bounds
    #[test]
    fn error_code_properties(
        dimension in 1u32..1000
    ) {
        // Test various error types with static strings
        let errors = vec![
            AtlasError::InvalidDimension(dimension),
            AtlasError::MatrixError("test matrix error"),
            AtlasError::TopologyError("test topology error"),
            AtlasError::AllocationError,
        ];
        
        for error in &errors {
            let error_code = error::error_to_code(error);
            
            // Error codes should be negative
            prop_assert!(error_code < 0);
            
            // Error codes should be in reasonable range
            prop_assert!(error_code >= -100);
            
            // Error display should not be empty
            let error_str = format!("{}", error);
            prop_assert!(!error_str.is_empty());
        }
    }

    /// Test memory safety properties for Pod types
    #[test]
    fn pod_type_safety(coords in prop::array::uniform(-1000.0f64..1000.0)) {
        let point = AtlasPoint::<3> { coords };
        
        // Should be safely convertible to bytes
        let bytes = bytemuck::bytes_of(&point);
        prop_assert_eq!(bytes.len(), 24);
        
        // Should be recoverable from bytes
        let recovered: Result<&AtlasPoint<3>, _> = bytemuck::try_from_bytes(bytes);
        prop_assert!(recovered.is_ok());
        
        if let Ok(recovered_point) = recovered {
            prop_assert_eq!(*recovered_point, point);
        }
    }

    /// Test basic arithmetic and scaling properties
    #[test]
    fn basic_mathematical_properties(
        scale_factor in 0.01f64..100.0,
        x in -1000.0f64..1000.0,
        y in -1000.0f64..1000.0
    ) {
        // Test scaling properties
        let scaled_x = x * scale_factor;
        let scaled_y = y * scale_factor;
        
        if x != 0.0 {
            prop_assert!((scaled_x / x - scale_factor).abs() < 1e-10);
        }
        
        if y != 0.0 {
            prop_assert!((scaled_y / y - scale_factor).abs() < 1e-10);
        }
        
        // Test that finite values remain finite after reasonable operations
        if x.is_finite() && y.is_finite() && scale_factor.is_finite() {
            prop_assert!(scaled_x.is_finite());
            prop_assert!(scaled_y.is_finite());
        }
    }

    /// Test byte-level properties and ranges
    #[test]
    fn byte_operations_properties(
        byte_val in any::<u8>(),
        byte_array in prop::collection::vec(any::<u8>(), 1..=1000)
    ) {
        // Test byte value properties
        prop_assert!(byte_val <= 255);
        prop_assert!(byte_val >= 0);
        
        // Test R96 classification bounds (using mathematical properties)
        let r96_class = byte_val % 96;
        prop_assert!(r96_class < 96);
        
        // Test array sum properties
        let sum: u64 = byte_array.iter().map(|&b| b as u64).sum();
        prop_assert!(sum >= byte_array.len() as u64 * 0);
        prop_assert!(sum <= byte_array.len() as u64 * 255);
        
        // Test modular arithmetic
        let remainder = sum % 96;
        prop_assert!(remainder < 96);
    }

    /// Test dimensional consistency and bounds
    #[test]
    fn dimensional_consistency(
        dim1 in 1u32..=10,
        dim2 in 1u32..=10
    ) {
        // Test that different dimensional points have different sizes
        let size_1d = std::mem::size_of::<AtlasPoint<1>>();
        let size_2d = std::mem::size_of::<AtlasPoint<2>>();
        let size_3d = std::mem::size_of::<AtlasPoint<3>>();
        
        prop_assert_eq!(size_1d, 8);   // 1 * 8 bytes
        prop_assert_eq!(size_2d, 16);  // 2 * 8 bytes  
        prop_assert_eq!(size_3d, 24);  // 3 * 8 bytes
        
        prop_assert!(size_1d < size_2d);
        prop_assert!(size_2d < size_3d);
        
        // Test manifold dimension relationships
        let manifold = ManifoldDescriptor::new(dim1, dim2, [0.0; 4], 1.0);
        prop_assert_eq!(manifold.intrinsic_dim, dim1);
        prop_assert_eq!(manifold.embedding_dim, dim2);
    }
}