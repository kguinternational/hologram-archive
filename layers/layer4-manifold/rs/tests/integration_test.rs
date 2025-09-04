//! Integration tests for Atlas Manifold Layer 4
//!
//! These tests verify complete workflows and interactions between different
//! components of the manifold system.

use atlas_manifold::{
    cleanup,
    coords::{util as coords_util, CoordinateSystem, CoordinateTransform},
    error::*,
    init,
    linear::util,
    manifold::{CoordinateChart, ManifoldAtlas},
    shard::{ShardManager, ShardMetadata, ShardStrategy},
    tlv::{TlvDecoder, TlvEncoder},
    types::*,
};

#[test]
fn test_complete_manifold_workflow() {
    // Initialize library
    assert!(init().is_ok());

    // Create a 2D manifold embedded in 3D space
    let descriptor = ManifoldDescriptor::spherical(2.0, 3);
    let mut atlas = ManifoldAtlas::<2, 3>::new(descriptor);

    // Create a coordinate chart
    let domain_bounds = [
        (-std::f64::consts::PI, std::f64::consts::PI),
        (0.0, std::f64::consts::PI),
    ];
    let codomain_bounds = [(-2.0, 2.0), (-2.0, 2.0), (-2.0, 2.0)];

    // Create transformation matrices (simplified spherical to Cartesian)
    let forward_transform = TransformMatrix::<3, 2>::zero(); // Will be identity-like
    let inverse_transform = TransformMatrix::<2, 3>::zero();

    let chart = CoordinateChart::<2, 3>::new(
        domain_bounds,
        codomain_bounds,
        forward_transform,
        inverse_transform,
    );

    let chart_index = atlas.add_chart(chart);
    assert_eq!(chart_index, 0);

    assert!(atlas.set_active_chart(0).is_ok());

    // Test manifold properties
    assert!(descriptor.is_riemannian());
    assert_eq!(descriptor.gaussian_curvature(), 0.25); // 1/r^2 where r=2

    // Clean up
    assert!(cleanup().is_ok());
}

#[test]
fn test_shard_management_workflow() {
    assert!(init().is_ok());

    // Create a sharding strategy
    let strategy = ShardStrategy::CoordinateHash { num_shards: 8 };
    let mut manager = ShardManager::new(strategy);

    // Create test points and assign them to shards
    let points = vec![
        AtlasPoint::<2> { coords: [0.0, 0.0] },
        AtlasPoint::<2> { coords: [1.0, 1.0] },
        AtlasPoint::<2> {
            coords: [-1.0, -1.0],
        },
        AtlasPoint::<2> {
            coords: [2.0, -2.0],
        },
        AtlasPoint::<2> {
            coords: [-3.0, 3.0],
        },
    ];

    let mut shard_assignments = Vec::new();
    for point in &points {
        let shard_id = manager.shard_for_point(point).unwrap();
        shard_assignments.push(shard_id);
        assert!(shard_id.primary < 8); // Should be in valid range
    }

    // Add shards based on assignments
    for (i, &shard_id) in shard_assignments.iter().enumerate() {
        if manager.shard_metadata(shard_id).is_none() {
            let metadata = ShardMetadata {
                id: shard_id,
                point_count: 1,
                bounds: vec![(-10.0, 10.0), (-10.0, 10.0)],
                load_factor: 0.1,
            };
            manager.add_shard(metadata);
        }
    }

    // Test shard operations
    assert!(manager.all_shards().len() > 0);
    let first_shard = &manager.all_shards()[0];
    assert!(manager.shard_metadata(first_shard.id).is_some());

    // Test rebalancing
    let operations = manager.rebalance().unwrap();
    // Operations list might be empty if shards are already balanced

    assert!(cleanup().is_ok());
}

#[test]
fn test_coordinate_transformation_workflow() {
    assert!(init().is_ok());

    // Test Cartesian to Spherical transformation
    let transform = CoordinateTransform::<3, 3>::new(
        CoordinateSystem::Cartesian,
        CoordinateSystem::Spherical,
        [0.0; 16], // Default parameters
    );

    // Test point at (1, 0, 0) should map to (1, π/2, 0) in spherical
    let cartesian_point = AtlasPoint::<3> {
        coords: [1.0, 0.0, 0.0],
    };
    let spherical_result = transform.transform_point(&cartesian_point);
    assert!(spherical_result.is_ok());

    let spherical_point = spherical_result.unwrap();
    assert!((spherical_point.coords[0] - 1.0).abs() < 1e-10); // r = 1
    assert!((spherical_point.coords[1] - std::f64::consts::FRAC_PI_2).abs() < 1e-10); // θ = π/2
    assert!(spherical_point.coords[2].abs() < 1e-10); // φ = 0

    // Test round-trip transformation
    let reverse_transform = CoordinateTransform::<3, 3>::new(
        CoordinateSystem::Spherical,
        CoordinateSystem::Cartesian,
        [0.0; 16],
    );

    let back_to_cartesian = reverse_transform.transform_point(&spherical_point).unwrap();
    for i in 0..3 {
        assert!(
            (back_to_cartesian.coords[i] - cartesian_point.coords[i]).abs() < 1e-10,
            "Round-trip coordinate transformation failed at dimension {}",
            i
        );
    }

    // Test Jacobian computation
    let jacobian_result = transform.jacobian(&cartesian_point);
    assert!(jacobian_result.is_ok());

    assert!(cleanup().is_ok());
}

#[test]
fn test_serialization_workflow() {
    assert!(init().is_ok());

    // Create complex data structures to serialize
    let manifold_desc = ManifoldDescriptor::hyperbolic(3, 4);
    let point = AtlasPoint::<4> {
        coords: [1.0, 2.0, 3.0, 4.0],
    };
    let vector = AtlasVector::<4> {
        components: [0.5, -0.5, 1.5, -1.5],
    };
    let shard_id = ShardId::new(12345, 67890);

    // Serialize all data
    let mut encoder = TlvEncoder::with_capacity(1024);

    assert!(encoder.write_manifold_descriptor(&manifold_desc).is_ok());
    assert!(encoder.write_point(&point).is_ok());
    assert!(encoder.write_vector(&vector).is_ok());
    assert!(encoder.write_shard_id(&shard_id).is_ok());

    let serialized_data = encoder.finish();
    assert!(!serialized_data.is_empty());

    // Deserialize and verify
    let mut decoder = TlvDecoder::new(&serialized_data);

    let decoded_desc = decoder.read_manifold_descriptor().unwrap();
    assert_eq!(decoded_desc.intrinsic_dim, manifold_desc.intrinsic_dim);
    assert_eq!(decoded_desc.embedding_dim, manifold_desc.embedding_dim);
    assert_eq!(
        decoded_desc.gaussian_curvature(),
        manifold_desc.gaussian_curvature()
    );

    // Skip dimension metadata for point
    let _ = decoder.read_u32();
    let decoded_point = decoder.read_point::<4>().unwrap();
    assert_eq!(decoded_point.coords, point.coords);

    // Skip dimension metadata for vector
    let _ = decoder.read_u32();
    let decoded_vector = decoder.read_vector::<4>().unwrap();
    assert_eq!(decoded_vector.components, vector.components);

    let decoded_shard = decoder.read_shard_id().unwrap();
    assert_eq!(decoded_shard, shard_id);

    assert!(cleanup().is_ok());
}

#[test]
fn test_linear_algebra_workflow() {
    assert!(init().is_ok());

    // Create vectors and test operations
    let v1 = AtlasVector::<3>::new([1.0, 0.0, 0.0]);
    let v2 = AtlasVector::<3>::new([0.0, 1.0, 0.0]);
    let v3 = AtlasVector::<3>::new([0.0, 0.0, 1.0]);

    // Test dot products
    assert_eq!(v1.dot(&v2), 0.0); // Orthogonal vectors
    assert_eq!(v1.dot(&v1), 1.0); // Unit vector with itself

    // Test cross product
    let cross = util::cross_product_3d(&v1, &v2);
    assert!((cross.components[0] - 0.0).abs() < 1e-10);
    assert!((cross.components[1] - 0.0).abs() < 1e-10);
    assert!((cross.components[2] - 1.0).abs() < 1e-10);

    // Test angle computation
    let angle = util::angle_between(&v1, &v2).unwrap();
    assert!((angle - std::f64::consts::FRAC_PI_2).abs() < 1e-10);

    // Test matrix operations
    let identity = TransformMatrix::<3, 3>::identity();
    let transformed = identity.multiply_vector(&v1);
    assert_eq!(transformed.components, v1.components);

    // Test matrix multiplication
    let rotation = coords_util::rotation_z(std::f64::consts::FRAC_PI_2);
    let rotated = rotation.multiply_vector(&v1);

    // After 90° rotation around Z, (1,0,0) should become (0,1,0)
    assert!(rotated.components[0].abs() < 1e-10);
    assert!((rotated.components[1] - 1.0).abs() < 1e-10);
    assert!(rotated.components[2].abs() < 1e-10);

    assert!(cleanup().is_ok());
}

#[test]
fn test_manifold_geometry_workflow() {
    assert!(init().is_ok());

    // Test different manifold types
    let euclidean = ManifoldDescriptor::euclidean(2, 2);
    let spherical = ManifoldDescriptor::spherical(1.0, 3);
    let hyperbolic = ManifoldDescriptor::hyperbolic(2, 3);

    // Create atlases for each
    let mut euclidean_atlas = ManifoldAtlas::<2, 2>::new(euclidean);
    let mut spherical_atlas = ManifoldAtlas::<2, 3>::new(spherical);
    let mut hyperbolic_atlas = ManifoldAtlas::<2, 3>::new(hyperbolic);

    // Add identity charts
    let bounds_2d = [(-10.0, 10.0), (-10.0, 10.0)];
    let bounds_3d = [(-2.0, 2.0), (-2.0, 2.0), (-2.0, 2.0)];

    euclidean_atlas.add_chart(CoordinateChart::<2, 2>::identity_square(bounds_2d));

    // For non-Euclidean manifolds, we'd need proper transformation matrices
    // For now, just test that they can be created
    assert_eq!(euclidean.gaussian_curvature(), 0.0);
    assert_eq!(spherical.gaussian_curvature(), 1.0); // 1/r^2 where r=1
    assert_eq!(hyperbolic.gaussian_curvature(), -1.0);

    // Test geodesic distance computation on Euclidean manifold
    euclidean_atlas.set_active_chart(0).unwrap();
    let p1 = AtlasPoint::<2> { coords: [0.0, 0.0] };
    let p2 = AtlasPoint::<2> { coords: [3.0, 4.0] };

    let distance = euclidean_atlas.geodesic_distance(&p1, &p2).unwrap();
    assert!((distance - 5.0).abs() < 1e-10); // 3-4-5 triangle

    // Test metric tensor computation
    let metric = euclidean_atlas.metric_tensor(&p1).unwrap();
    // For Euclidean space, metric should be identity-like
    assert!((metric.elements[0][0] - 1.0).abs() < 1e-10);
    assert!((metric.elements[1][1] - 1.0).abs() < 1e-10);

    assert!(cleanup().is_ok());
}

#[test]
fn test_error_recovery_workflow() {
    assert!(init().is_ok());

    // Test recovery from various error conditions
    let descriptor = ManifoldDescriptor::euclidean(2, 2);
    let mut atlas = ManifoldAtlas::<2, 2>::new(descriptor);

    // Try to use atlas without any charts
    let point = AtlasPoint::<2> { coords: [1.0, 1.0] };
    let result = atlas.to_euclidean(&point);
    assert!(result.is_err());

    match result.unwrap_err() {
        AtlasError::InvalidInput(msg) => assert_eq!(msg, "no active chart"),
        _ => panic!("Expected InvalidInput error"),
    }

    // Add a chart and try again
    let bounds = [(-5.0, 5.0), (-5.0, 5.0)];
    let chart = CoordinateChart::<2, 2>::identity_square(bounds);
    atlas.add_chart(chart);
    atlas.set_active_chart(0).unwrap();

    // Now it should work
    let result = atlas.to_euclidean(&point);
    assert!(result.is_ok());

    // Test point outside chart domain
    let outside_point = AtlasPoint::<2> {
        coords: [10.0, 10.0],
    };
    let result = atlas.to_euclidean(&outside_point);
    assert!(result.is_err());

    match result.unwrap_err() {
        AtlasError::CoordinateError(msg) => assert_eq!(msg, "point outside chart domain"),
        _ => panic!("Expected CoordinateError"),
    }

    // Test TLV error recovery
    let invalid_data = [0x00, 0x01, 0x02, 0x03]; // Too short for valid TLV
    let mut decoder = TlvDecoder::new(&invalid_data);
    let result = decoder.read_manifold_descriptor();
    assert!(result.is_err());

    // Test shard manager error recovery
    let strategy = ShardStrategy::Spatial {
        bounds: vec![(0.0, 1.0)],
    };
    let manager = ShardManager::new(strategy);

    // Try to shard a point with wrong dimensions
    let wrong_dim_point = AtlasPoint::<3> {
        coords: [0.5, 0.5, 0.5],
    };
    let result = manager.shard_for_point(&wrong_dim_point);
    assert!(result.is_err());

    assert!(cleanup().is_ok());
}

#[test]
fn test_multithreaded_safety() {
    use std::sync::Arc;
    use std::thread;

    assert!(init().is_ok());

    // Create shared data
    let descriptor = Arc::new(ManifoldDescriptor::euclidean(3, 3));
    let points = Arc::new(vec![
        AtlasPoint::<3> {
            coords: [1.0, 0.0, 0.0],
        },
        AtlasPoint::<3> {
            coords: [0.0, 1.0, 0.0],
        },
        AtlasPoint::<3> {
            coords: [0.0, 0.0, 1.0],
        },
        AtlasPoint::<3> {
            coords: [1.0, 1.0, 1.0],
        },
    ]);

    let mut handles = vec![];

    // Spawn multiple threads to perform operations
    for i in 0..4 {
        let desc = descriptor.clone();
        let pts = points.clone();

        let handle = thread::spawn(move || {
            // Each thread creates its own atlas
            let atlas = ManifoldAtlas::<3, 3>::new(*desc);

            // Perform distance calculations
            let mut total_distance = 0.0;
            for j in 0..pts.len() {
                for k in (j + 1)..pts.len() {
                    total_distance += pts[j].distance(&pts[k]);
                }
            }

            // Create and use TLV encoder/decoder
            let mut encoder = TlvEncoder::new();
            encoder.write_manifold_descriptor(&desc).unwrap();
            encoder.write_point(&pts[i % pts.len()]).unwrap();

            let data = encoder.finish();
            let mut decoder = TlvDecoder::new(&data);
            let _ = decoder.read_manifold_descriptor().unwrap();
            let _ = decoder.read_u32(); // Skip dimension
            let _ = decoder.read_point::<3>().unwrap();

            total_distance
        });

        handles.push(handle);
    }

    // Wait for all threads to complete
    for handle in handles {
        let result = handle.join();
        assert!(result.is_ok());
        assert!(result.unwrap() > 0.0); // Should have calculated some distances
    }

    assert!(cleanup().is_ok());
}
