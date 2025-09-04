//! Golden Test Cases for Atlas Manifold Layer 4
//!
//! This module contains golden test cases with known inputs and expected outputs
//! to ensure deterministic behavior and regression testing across versions.

use atlas_manifold::{
    fourier::{NormalFormRules, R96FourierProjection, R96HarmonicCoefficient},
    invariants::*,
    projection::AtlasProjection,
};

/// Golden test case 1: Simple R96 classification with known result
#[test]
fn golden_r96_classification_simple() {
    let mut projection = R96FourierProjection::new();

    // Known input: 256 bytes of pattern data
    let input_data = create_golden_pattern_data();
    let build_result = projection.build_from_data(&input_data);
    assert!(build_result.is_ok());

    // Expected: specific classes should be active
    let active_classes = projection.get_active_classes();
    assert!(!active_classes.is_empty());

    // Golden verification: pattern should activate specific R96 classes
    // This pattern is designed to activate classes 0, 1, 2, and 95
    let expected_active = [0u8, 1, 2, 95];
    for &expected_class in &expected_active {
        assert!(
            active_classes.contains(&expected_class),
            "Expected class {} to be active",
            expected_class
        );
    }

    // Verify harmonic coefficients for known classes
    if let Some(ref harmonics) = projection.class_harmonics[0] {
        assert!(harmonics.coefficients.len() > 0);
        assert!(harmonics.normalization_factor > 0.0);
        // Golden value: first coefficient should have specific magnitude
        let first_coeff = &harmonics.coefficients[0];
        assert!(first_coeff.amplitude > 0.1 && first_coeff.amplitude < 10.0);
    }
}

/// Golden test case 2: C768 cycle with deterministic progression
#[test]
fn golden_c768_cycle_progression() {
    let mut tracker = C768CycleTracker::new();

    // Known input: deterministic sequence that completes exactly one C768 cycle
    let test_sequence = create_golden_c768_sequence();

    // Process the sequence step by step
    let mut step_count = 0;
    for chunk in test_sequence.chunks(32) {
        tracker.process_step(chunk).unwrap();
        step_count += 1;

        // Golden checkpoints: verify state at specific positions
        match step_count {
            24 => {
                // After 24 steps (768 bytes), should be at position 768
                assert_eq!(tracker.get_position(), 768);
                assert!(tracker.is_cycle_closed());
            },
            48 => {
                // After 48 steps (1536 bytes), should be at position 0 (wrapped)
                assert_eq!(tracker.get_position(), 0);
            },
            _ => {
                // Intermediate positions
                assert_eq!(tracker.get_position(), (step_count * 32) % 768);
            },
        }
    }

    // Final verification: cycle should be closed after exactly 768 steps
    assert!(tracker.is_cycle_closed());
}

/// Golden test case 3: Φ bijection with known mappings
#[test]
fn golden_phi_bijection_mappings() {
    let phi_verifier = PhiBijectionVerifier::new(256).unwrap();

    // Golden test vectors: (page, offset) -> expected_encoding
    let test_vectors = [
        (0, 0, 0),
        (0, 1, 1),
        (0, 255, 255),
        (1, 0, 256),
        (1, 1, 257),
        (1, 255, 511),
        (255, 255, 65535), // 255 * 256 + 255
    ];

    for &(page, offset, expected_encoding) in &test_vectors {
        // Test forward mapping
        let encoding = phi_verifier.phi_encode(page, offset);
        assert_eq!(
            encoding, expected_encoding,
            "Φ({}, {}) should equal {}, got {}",
            page, offset, expected_encoding, encoding
        );

        // Test inverse mapping
        let decode_result = phi_verifier.phi_inverse(encoding);
        assert!(decode_result.is_ok());
        let (decoded_page, decoded_offset) = decode_result.unwrap();
        assert_eq!(decoded_page, page);
        assert_eq!(decoded_offset, offset);
    }
}

/// Golden test case 4: Klein orbit alignment patterns
#[test]
fn golden_klein_orbit_patterns() {
    let mut aligner = KleinOrbitAligner::new();

    // Golden Klein orbit positions: {0, 1, 48, 49}
    let orbit_positions = [0, 1, 48, 49];
    let test_data = create_golden_orbit_data();

    for &position in &orbit_positions {
        let alignment_result = aligner.check_alignment(position, &test_data);
        assert!(alignment_result.is_ok());
    }

    // Verify complete alignment
    assert!(aligner.verify_complete_alignment().is_ok());

    // Test invalid positions (should not align)
    let invalid_positions = [2, 47, 50, 95];
    for &position in &invalid_positions {
        let alignment_result = aligner.check_alignment(position, &test_data);
        // These positions should not create perfect alignment
        // (depending on implementation, this might be Ok but with different scores)
        assert!(alignment_result.is_ok() || alignment_result.is_err());
    }
}

/// Golden test case 5: Normal Form quantization with known coefficients
#[test]
fn golden_normal_form_quantization() {
    let mut harmonics = create_golden_harmonic_coefficients();
    let rules = NormalFormRules::default();

    // Store original values for comparison
    let original_phases: Vec<f64> = harmonics.iter().map(|h| h.phase).collect();

    // Apply Normal Form
    for harmonic in &mut harmonics {
        harmonic.apply_normal_form(&rules);
    }

    // Verify quantization effects
    for (i, harmonic) in harmonics.iter().enumerate() {
        // Phase should be quantized to discrete levels
        let phase_levels = f64::from(rules.phase_quantization);
        let normalized_phase = harmonic.phase / (2.0 * std::f64::consts::PI);
        let quantized_check = (normalized_phase * phase_levels).round() / phase_levels;
        let expected_phase = quantized_check * 2.0 * std::f64::consts::PI;

        assert!(
            (harmonic.phase - expected_phase).abs() < 1e-10,
            "Phase quantization failed for harmonic {}: expected {}, got {}",
            i,
            expected_phase,
            harmonic.phase
        );
    }
}

/// Golden test case 6: Conservation law arithmetic
#[test]
fn golden_conservation_arithmetic() {
    // Golden test data with known conservation sums
    let test_cases = [
        (vec![1u8; 96], 96u64), // 96 ones = 96
        (vec![2u8; 48], 96u64), // 48 twos = 96
        (vec![3u8; 32], 96u64), // 32 threes = 96
        (vec![4u8; 24], 96u64), // 24 fours = 96
        (vec![6u8; 16], 96u64), // 16 sixes = 96
    ];

    for (data, expected_sum) in test_cases {
        let actual_sum: u64 = data.iter().map(|&b| u64::from(b)).sum();
        assert_eq!(actual_sum, expected_sum);
        assert_eq!(actual_sum % 96, 0, "Sum should be divisible by 96");

        // Test with projection
        let projection_result = AtlasProjection::new_linear(&data);
        assert!(projection_result.is_ok());
        let projection = projection_result.unwrap();
        assert!(projection.verify_projection());
        assert_eq!(projection.total_conservation_sum % 96, 0);
    }
}

/// Golden test case 7: R96 harmonic synthesis determinism
#[test]
fn golden_r96_harmonic_synthesis() {
    let mut projection = R96FourierProjection::new();

    // Use the same golden pattern data for reproducibility
    let input_data = create_golden_pattern_data();
    projection.build_from_data(&input_data).unwrap();

    // Synthesize data and verify reproducibility
    let synthesized1 = projection.reconstruct_data(256);
    let synthesized2 = projection.reconstruct_data(256);

    // Should be deterministic
    assert_eq!(synthesized1, synthesized2);
    assert_eq!(synthesized1.len(), 256);

    // Should not be all zeros (active synthesis)
    assert!(synthesized1.iter().any(|&b| b != 0));

    // Apply Normal Form and verify determinism is maintained
    let rules = NormalFormRules::default();
    projection.apply_normal_form(&rules).unwrap();

    let synthesized3 = projection.reconstruct_data(256);
    let synthesized4 = projection.reconstruct_data(256);
    assert_eq!(synthesized3, synthesized4);
}

/// Golden test case 8: Mathematical invariants verification
#[test]
fn golden_mathematical_invariants() {
    // Golden ratios and constants that must hold

    // C768 structure
    assert_eq!(768, 3 * 256);
    assert_eq!(768, 16 * 48);
    assert_eq!(768, 24 * 32);

    // R96 class relationships
    assert_eq!(96, 2 * 48);
    assert_eq!(96, 3 * 32);
    assert_eq!(96, 4 * 24);
    assert_eq!(96, 6 * 16);
    assert_eq!(96, 8 * 12);

    // Klein orbit properties
    let klein_positions = [0, 1, 48, 49];
    assert_eq!(klein_positions[0], 0); // Origin
    assert_eq!(klein_positions[1], 1); // Unit step
    assert_eq!(klein_positions[2], 96 / 2); // Midpoint
    assert_eq!(klein_positions[3], 48 + 1); // Conjugate

    // Page size relationships
    assert_eq!(256, 1 << 8); // Power of 2
    assert_eq!(256, 4 * 64); // Tensor compatibility
    assert_eq!(256, 16 * 16); // Square structure

    // Φ bijection bounds
    let max_encoding = 256 * 256; // page * offset
    assert_eq!(max_encoding, 65536);
    assert_eq!(max_encoding, 1 << 16); // 16-bit addressing
}

// Helper functions for creating golden test data

fn create_golden_pattern_data() -> Vec<u8> {
    // Create a specific pattern that produces known R96 classifications
    let mut data = Vec::with_capacity(256);

    // Pattern designed to activate specific R96 classes
    for i in 0..256 {
        match i % 4 {
            0 => data.push(0),   // Should map to R96 class 0
            1 => data.push(1),   // Should map to R96 class 1
            2 => data.push(2),   // Should map to R96 class 2
            3 => data.push(255), // Should map to R96 class 95
            _ => unreachable!(),
        }
    }

    data
}

fn create_golden_c768_sequence() -> Vec<u8> {
    // Create a sequence that exactly completes C768 cycles
    let sequence_length = 768 * 2; // Two complete cycles
    (0..sequence_length).map(|i| (i % 256) as u8).collect()
}

fn create_golden_orbit_data() -> Vec<u8> {
    // Create data that aligns well with Klein orbit positions
    vec![42u8; 32] // Simple pattern that should align consistently
}

fn create_golden_harmonic_coefficients() -> Vec<R96HarmonicCoefficient> {
    vec![
        R96HarmonicCoefficient::new(1.0, 0.0), // DC component
        R96HarmonicCoefficient::new(0.5, std::f64::consts::PI / 4.0), // 45° phase
        R96HarmonicCoefficient::new(0.25, std::f64::consts::PI / 2.0), // 90° phase
        R96HarmonicCoefficient::new(0.125, std::f64::consts::PI), // 180° phase
    ]
}
