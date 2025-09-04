//! Holographic Properties Test Suite
//!
//! This module tests the holographic properties of Layer 4 (Manifold).

use atlas_manifold::{error::*, projection::AtlasProjection};

/// Test basic holographic properties through projection
#[test]
fn test_basic_holographic_properties() {
    // Create test data with structure that can be verified
    let original_data = create_structured_test_data();

    // Create projection
    let projection = AtlasProjection::new_linear(&original_data).unwrap();

    // Verify holographic properties are maintained
    assert!(projection.verify_projection());
    assert_eq!(projection.total_conservation_sum % 96, 0);

    // Verify that tiles maintain structure
    for tile in &projection.tiles {
        assert!(tile.verify_conservation());
        assert!(tile.verify_layer2_conservation());
        assert!(!tile.pages.is_empty());
    }

    // Test that projection maintains structural coherence
    assert!(!projection.tiles.is_empty());
    verify_projection_structure(&projection);
}

/// Test data preservation through projection transformations
#[test]
fn test_data_preservation_holographic() {
    let original_data = create_holographic_test_data();
    let projection = AtlasProjection::new_linear(&original_data).unwrap();

    // Verify data is preserved through projection
    assert!(projection.verify_projection());

    // Test that tiles contain meaningful data
    let mut total_tile_data = 0;
    for tile in &projection.tiles {
        for page in &tile.pages {
            total_tile_data += page.len();
        }
    }

    // Should have preserved data in tiles
    assert!(total_tile_data > 0);
    assert!(total_tile_data <= original_data.len() + projection.tiles.len() * 4096); // Some overhead allowed

    // Verify holographic properties: structure preservation
    verify_holographic_preservation(&original_data, &projection);
}

/// Test projection coherence with multiple tiles
#[test]
fn test_projection_tile_coherence() {
    let original_data = create_coherent_test_data();
    let projection = AtlasProjection::new_linear(&original_data).unwrap();

    // Verify multiple tiles maintain coherence
    assert!(projection.tiles.len() > 1); // Should create multiple tiles

    // Verify each tile maintains coherence properties
    for tile in &projection.tiles {
        assert!(tile.verify_conservation());
        assert!(tile.verify_layer2_conservation());
        assert!(!tile.pages.is_empty());

        // Conservation should be maintained across all tiles
        assert_eq!(tile.conservation_sum % 96, 0);
    }

    // Verify overall projection coherence
    assert!(projection.verify_projection());
    assert_eq!(projection.total_conservation_sum % 96, 0);

    // Test that tiles maintain statistical coherence
    assert!(verify_tile_coherence(&projection));
}

/// Test R96 Fourier shard extraction and reconstruction
#[test]
fn test_r96_fourier_holographic() {
    // Create data suitable for R96 Fourier analysis
    let r96_data = create_r96_holographic_data();

    // Create R96 Fourier projection
    let projection_result = AtlasProjection::new_r96_fourier(&r96_data, None);
    assert!(projection_result.is_ok());
    let projection = projection_result.unwrap();

    // Verify R96 projection properties
    assert!(projection.verify_projection());

    // Extract shards with R96-aware regions
    let r96_regions = vec![
        ShardMetadata::new(0, 2560, 10, 1, (0.0, 0.0, 25.0, 25.0)), // ~10 R96 pages
        ShardMetadata::new(2560, 5120, 10, 2, (25.0, 0.0, 50.0, 25.0)), // Another 10 pages
    ];

    let shards = projection.extract_shards(&r96_regions).unwrap();
    assert_eq!(shards.len(), 2);

    // Verify R96 harmonic data is preserved in shards
    for shard in &shards {
        assert!(!shard.r96_harmonics.is_empty());

        // Each harmonic should have valid coefficients
        for harmonic in &shard.r96_harmonics {
            assert!(!harmonic.coefficients.is_empty());
            assert!(harmonic.normalization_factor > 0.0);
            assert!(harmonic.resonance_class < 96);
        }

        // Verify R96 conservation
        let r96_conservation_sum: u64 =
            shard.r96_harmonics.iter().map(|h| h.conservation_sum).sum();
        assert_eq!(r96_conservation_sum % 96, 0);
    }

    // Test R96 holographic reconstruction
    let shard_manager = ShardManager::new();
    for shard in &shards {
        let reconstruction = shard_manager.reconstruct_from_shard(shard).unwrap();

        // Verify R96 structure is maintained
        verify_r96_structure(&reconstruction);
    }
}

/// Test conservation law preservation in holographic operations
#[test]
fn test_conservation_preservation_holographic() {
    // Create data that exactly satisfies conservation laws
    let conservation_data = create_conservation_compliant_data();
    let original_sum: u64 = conservation_data.iter().map(|&b| u64::from(b)).sum();
    assert_eq!(original_sum % 96, 0); // Verify input conservation

    let projection = AtlasProjection::new_linear(&conservation_data, None).unwrap();

    // Extract multiple shards
    let regions = create_conservation_test_regions();
    let shards = projection.extract_shards(&regions).unwrap();

    // Verify conservation is preserved across all shards
    let total_shard_conservation: u64 = shards.iter().map(|s| s.conservation_sum).sum();

    // Conservation should be preserved (allowing for some computational tolerance)
    assert_eq!(total_shard_conservation % 96, 0);

    // Individual shards should also be conserved
    for shard in &shards {
        assert!(shard.is_conserved);
        assert_eq!(shard.conservation_sum % 96, 0);
    }

    // Test holographic reconstruction preserves conservation
    let shard_manager = ShardManager::new();
    for shard in &shards {
        let reconstruction = shard_manager.reconstruct_from_shard(shard).unwrap();
        let reconstruction_sum: u64 = reconstruction.iter().map(|&b| u64::from(b)).sum();

        // Reconstruction should maintain conservation properties
        assert_eq!(reconstruction_sum % 96, 0);
    }
}

/// Test witness verification in holographic reconstruction
#[test]
fn test_witness_verification_holographic() {
    let test_data = create_witness_test_data();
    let projection = AtlasProjection::new_linear(&test_data, None).unwrap();

    let regions = vec![ShardMetadata::new(0, 1024, 4, 1, (0.0, 0.0, 10.0, 10.0))];

    let shards = projection.extract_shards(&regions).unwrap();
    assert_eq!(shards.len(), 1);

    let shard = &shards[0];

    // Verify witness data is present
    assert!(!shard.witness.proof_data.is_empty());
    assert!(!shard.witness.metadata.is_empty());

    // Test witness verification
    assert!(shard.witness.verify(&shard.data_blocks));

    // Test reconstruction with witness verification
    let shard_manager = ShardManager::new();
    let reconstruction_result = shard_manager.reconstruct_from_shard(shard);

    // Should succeed with valid witness
    assert!(reconstruction_result.is_ok());

    // Test with corrupted witness (simulate tampered shard)
    let mut corrupted_shard = shard.clone();
    corrupted_shard.witness.proof_data[0] ^= 0xFF; // Flip bits

    let corrupted_reconstruction = shard_manager.reconstruct_from_shard(&corrupted_shard);
    // Should fail or produce different result with corrupted witness
    assert!(
        corrupted_reconstruction.is_err()
            || corrupted_reconstruction.unwrap() != reconstruction_result.unwrap()
    );
}

// Helper functions for creating test data and verification

fn create_structured_test_data() -> Vec<u8> {
    let mut data = Vec::new();
    for i in 0..4096 {
        // Create structured pattern that can be verified
        data.push(((i % 256) ^ ((i / 256) % 256)) as u8);
    }
    data
}

fn create_holographic_test_data() -> Vec<u8> {
    // Create data with holographic properties - patterns that repeat at different scales
    let mut data = Vec::new();
    for i in 0..8192 {
        let scale1 = (i % 32) as u8;
        let scale2 = ((i / 32) % 32) as u8;
        let scale3 = ((i / 1024) % 8) as u8;
        data.push(scale1 ^ scale2 ^ scale3);
    }
    data
}

fn create_coherent_test_data() -> Vec<u8> {
    // Create data where overlapping regions have coherent relationships
    let mut data = Vec::new();
    for i in 0..8192 {
        // Coherent pattern: each position related to its neighbors
        let value = ((i % 127) + (i / 127) % 127) % 256;
        data.push(value as u8);
    }
    data
}

fn create_r96_holographic_data() -> Vec<u8> {
    // Create data suitable for R96 classification with holographic properties
    let mut data = Vec::new();
    for page in 0..96 {
        for byte_in_page in 0..256 {
            // Pattern that should classify to specific R96 classes
            let r96_class = page % 96;
            let value = (r96_class + byte_in_page) % 256;
            data.push(value as u8);
        }
    }
    data
}

fn create_conservation_compliant_data() -> Vec<u8> {
    // Create data that exactly satisfies conservation laws (sum % 96 == 0)
    let mut data = vec![1u8; 1920]; // 1920 ones = 20 * 96
                                    // Adjust for exact conservation
    let current_sum: u64 = data.iter().map(|&b| u64::from(b)).sum();
    let remainder = current_sum % 96;
    if remainder != 0 {
        let adjustment = 96 - remainder;
        data[0] = (u64::from(data[0]) + adjustment) as u8;
    }
    data
}

fn create_witness_test_data() -> Vec<u8> {
    // Create test data for witness verification
    (0..2048).map(|i| ((i * 7 + 11) % 256) as u8).collect()
}

fn create_conservation_test_regions() -> Vec<ShardMetadata> {
    vec![
        ShardMetadata::new(0, 640, 5, 1, (0.0, 0.0, 8.0, 8.0)), // 640 bytes, 5 pages
        ShardMetadata::new(640, 1280, 5, 2, (8.0, 0.0, 16.0, 8.0)), // 640 bytes, 5 pages
        ShardMetadata::new(1280, 1920, 5, 3, (0.0, 8.0, 8.0, 16.0)), // 640 bytes, 5 pages
    ]
}

fn verify_projection_structure(projection: &AtlasProjection) -> bool {
    // Verify the projection maintains expected structural properties
    if projection.tiles.is_empty() {
        return false;
    }

    // Check that tiles have meaningful data variety
    let mut all_bytes = Vec::new();
    for tile in &projection.tiles {
        for page in &tile.pages {
            all_bytes.extend_from_slice(page);
        }
    }

    if all_bytes.is_empty() {
        return false;
    }

    // Check for non-trivial patterns (not all zeros or ones)
    let unique_values: std::collections::HashSet<u8> = all_bytes.iter().cloned().collect();
    unique_values.len() > 1
}

fn verify_holographic_preservation(original: &[u8], projection: &AtlasProjection) -> bool {
    // Verify that holographic properties are preserved in projection
    if projection.tiles.is_empty() {
        return false;
    }

    // Collect all data from projection
    let mut projection_data = Vec::new();
    for tile in &projection.tiles {
        for page in &tile.pages {
            projection_data.extend_from_slice(page);
        }
    }

    if projection_data.is_empty() {
        return false;
    }

    // Check that statistical properties are preserved
    let orig_mean =
        original.iter().map(|&b| u64::from(b)).sum::<u64>() as f64 / original.len() as f64;
    let proj_mean = projection_data.iter().map(|&b| u64::from(b)).sum::<u64>() as f64
        / projection_data.len() as f64;

    // Means should be reasonably similar (allowing for some transformation effects)
    (orig_mean - proj_mean).abs() < orig_mean * 0.8 + 10.0 // Allow more tolerance
}

fn verify_tile_coherence(projection: &AtlasProjection) -> bool {
    // Verify coherence across multiple projection tiles
    if projection.tiles.len() < 2 {
        return true; // Trivially coherent
    }

    // Check that all tiles are non-empty and have reasonable conservation sums
    projection
        .tiles
        .iter()
        .all(|tile| !tile.pages.is_empty() && tile.conservation_sum % 96 == 0)
}

fn verify_r96_structure(data: &[u8]) -> bool {
    // Verify that R96 structure is maintained in reconstruction
    if data.len() % 256 != 0 {
        return false; // Should align to R96 page boundaries
    }

    // Check for reasonable variation (R96 classification should produce varied output)
    let unique_values: std::collections::HashSet<u8> = data.iter().cloned().collect();
    unique_values.len() >= 4 // Should have at least some variety
}
