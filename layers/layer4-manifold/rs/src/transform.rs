//! Transform operations for Atlas manifold projections
//!
//! This module implements geometric transformation operations that preserve conservation laws
//! while applying transformations to manifold projections. All transforms maintain the
//! fundamental invariants required by the Atlas system.

#![allow(clippy::module_name_repetitions)]

use crate::error::*;
use crate::projection::*;

/// Maximum scaling factor to prevent numerical instability
const MAX_SCALE_FACTOR: f64 = 1000.0;

/// Minimum scaling factor to prevent degenerate transformations
const MIN_SCALE_FACTOR: f64 = 0.001;

/// Maximum rotation angle (multiple of 2π is reduced to [0, 2π))
const MAX_ROTATION_ANGLE: f64 = 2.0 * std::f64::consts::PI;

/// Maximum translation offset to prevent overflow
const MAX_TRANSLATION: f64 = 1e6;

/// Apply linear transformation to projection data using a 4x4 matrix
///
/// The transformation matrix is applied in homogeneous coordinates to preserve
/// conservation laws while enabling complex geometric operations.
pub fn apply_linear_transform(
    projection: &mut AtlasProjection,
    matrix: &[f64; 16], // 4x4 matrix in row-major order
) -> AtlasResult<()> {
    // Validate transformation matrix
    validate_transform_matrix(matrix)?;

    // Extract 2x2 transformation matrix from the 4x4 homogeneous matrix
    // We use the upper-left 2x2 for spatial transformation
    let m11 = matrix[0]; // [0,0]
    let m12 = matrix[1]; // [0,1]
    let m21 = matrix[4]; // [1,0]
    let m22 = matrix[5]; // [1,1]

    // Translation components
    let tx = matrix[3]; // [0,3]
    let ty = matrix[7]; // [1,3]

    // Apply transformation to tile bounds while preserving conservation
    for tile in &mut projection.tiles {
        let (min_x, min_y, max_x, max_y) = tile.bounds;

        // Transform the four corners of the bounding box
        let corners = [
            (min_x, min_y),
            (max_x, min_y),
            (min_x, max_y),
            (max_x, max_y),
        ];

        let mut transformed_corners = Vec::with_capacity(4);
        for (x, y) in corners {
            let new_x = m11 * x + m12 * y + tx;
            let new_y = m21 * x + m22 * y + ty;
            transformed_corners.push((new_x, new_y));
        }

        // Find new bounding box
        let new_min_x = transformed_corners.iter().map(|(x, _)| *x).fold(f64::INFINITY, f64::min);
        let new_max_x =
            transformed_corners.iter().map(|(x, _)| *x).fold(f64::NEG_INFINITY, f64::max);
        let new_min_y = transformed_corners.iter().map(|(_, y)| *y).fold(f64::INFINITY, f64::min);
        let new_max_y =
            transformed_corners.iter().map(|(_, y)| *y).fold(f64::NEG_INFINITY, f64::max);

        tile.bounds = (new_min_x, new_min_y, new_max_x, new_max_y);
    }

    // Update transformation parameters for tracking
    let transform_params = if let Some(current) = projection.transform_params {
        // Compose with existing transformation
        TransformationParams {
            scaling_factor: current.scaling_factor * calculate_scale_factor(matrix),
            rotation_angle: (current.rotation_angle + calculate_rotation_angle(matrix))
                % MAX_ROTATION_ANGLE,
            translation_x: current.translation_x + tx,
            translation_y: current.translation_y + ty,
        }
    } else {
        TransformationParams {
            scaling_factor: calculate_scale_factor(matrix),
            rotation_angle: calculate_rotation_angle(matrix),
            translation_x: tx,
            translation_y: ty,
        }
    };

    projection.transform_params = Some(transform_params);

    // Verify conservation laws are still satisfied after transformation
    projection.verify()?;

    Ok(())
}

/// Apply geometric scaling to projection coordinates
pub fn scale_projection(
    projection: &mut AtlasProjection,
    scale_x: f64,
    scale_y: f64,
) -> AtlasResult<()> {
    // Validate scaling factors
    if scale_x <= 0.0 || scale_y <= 0.0 {
        return Err(AtlasError::InvalidInput("scaling factors must be positive"));
    }

    if scale_x > MAX_SCALE_FACTOR
        || scale_y > MAX_SCALE_FACTOR
        || scale_x < MIN_SCALE_FACTOR
        || scale_y < MIN_SCALE_FACTOR
    {
        return Err(AtlasError::InvalidInput(
            "scaling factors out of valid range",
        ));
    }

    // Create scaling transformation matrix
    #[rustfmt::skip]
    let scale_matrix = [
        scale_x, 0.0,     0.0, 0.0,
        0.0,     scale_y, 0.0, 0.0,
        0.0,     0.0,     1.0, 0.0,
        0.0,     0.0,     0.0, 1.0,
    ];

    apply_linear_transform(projection, &scale_matrix)
}

/// Rotate projection coordinates around center point
pub fn rotate_projection(
    projection: &mut AtlasProjection,
    angle: f64,
    center_x: f64,
    center_y: f64,
) -> AtlasResult<()> {
    // Validate rotation parameters
    if !angle.is_finite() || !center_x.is_finite() || !center_y.is_finite() {
        return Err(AtlasError::InvalidInput(
            "rotation parameters must be finite",
        ));
    }

    if center_x.abs() > MAX_TRANSLATION || center_y.abs() > MAX_TRANSLATION {
        return Err(AtlasError::InvalidInput(
            "rotation center too far from origin",
        ));
    }

    // Normalize angle to [0, 2π)
    let normalized_angle = angle % MAX_ROTATION_ANGLE;
    let cos_a = normalized_angle.cos();
    let sin_a = normalized_angle.sin();

    // Create rotation matrix with translation to rotate around center point
    // This combines: translate to origin, rotate, translate back
    #[rustfmt::skip]
    let rotation_matrix = [
        cos_a,  -sin_a, 0.0, center_x - center_x * cos_a + center_y * sin_a,
        sin_a,   cos_a, 0.0, center_y - center_x * sin_a - center_y * cos_a,
        0.0,     0.0,   1.0, 0.0,
        0.0,     0.0,   0.0, 1.0,
    ];

    apply_linear_transform(projection, &rotation_matrix)
}

/// Translate projection coordinates by offset vector
pub fn translate_projection(
    projection: &mut AtlasProjection,
    offset_x: f64,
    offset_y: f64,
) -> AtlasResult<()> {
    // Validate translation parameters
    if !offset_x.is_finite() || !offset_y.is_finite() {
        return Err(AtlasError::InvalidInput(
            "translation offsets must be finite",
        ));
    }

    if offset_x.abs() > MAX_TRANSLATION || offset_y.abs() > MAX_TRANSLATION {
        return Err(AtlasError::InvalidInput("translation offsets too large"));
    }

    // Create translation matrix
    #[rustfmt::skip]
    let translation_matrix = [
        1.0, 0.0, 0.0, offset_x,
        0.0, 1.0, 0.0, offset_y,
        0.0, 0.0, 1.0, 0.0,
        0.0, 0.0, 0.0, 1.0,
    ];

    apply_linear_transform(projection, &translation_matrix)
}

/// Verify transformation parameters for mathematical validity
pub fn verify_transform_params(params: &TransformationParams) -> bool {
    // Check scaling factor
    if !params.scaling_factor.is_finite()
        || params.scaling_factor <= MIN_SCALE_FACTOR
        || params.scaling_factor > MAX_SCALE_FACTOR
    {
        return false;
    }

    // Check rotation angle
    if !params.rotation_angle.is_finite() {
        return false;
    }

    // Check translation parameters
    if !params.translation_x.is_finite()
        || !params.translation_y.is_finite()
        || params.translation_x.abs() > MAX_TRANSLATION
        || params.translation_y.abs() > MAX_TRANSLATION
    {
        return false;
    }

    true
}

/// Validate a 4x4 transformation matrix for numerical stability
fn validate_transform_matrix(matrix: &[f64; 16]) -> AtlasResult<()> {
    // Check for NaN or infinity values
    for &value in matrix {
        if !value.is_finite() {
            return Err(AtlasError::InvalidInput(
                "transformation matrix contains non-finite values",
            ));
        }
    }

    // Check that the matrix is not degenerate by computing determinant of upper-left 2x2
    let det = matrix[0] * matrix[5] - matrix[1] * matrix[4];
    if det.abs() < 1e-12 {
        return Err(AtlasError::InvalidInput(
            "transformation matrix is singular or near-singular",
        ));
    }

    // Check for extreme values that could cause overflow
    for &value in matrix {
        if value.abs() > 1e10 {
            return Err(AtlasError::InvalidInput(
                "transformation matrix values too large",
            ));
        }
    }

    Ok(())
}

/// Calculate the effective scaling factor from a transformation matrix
fn calculate_scale_factor(matrix: &[f64; 16]) -> f64 {
    // Calculate the determinant of the upper-left 2x2 submatrix
    let det = matrix[0] * matrix[5] - matrix[1] * matrix[4];
    det.abs().sqrt() // Square root of absolute determinant gives uniform scaling factor
}

/// Calculate the rotation angle from a transformation matrix
fn calculate_rotation_angle(matrix: &[f64; 16]) -> f64 {
    // Extract rotation angle from the upper-left 2x2 rotation part
    // We use atan2 for proper quadrant handling
    let scale = calculate_scale_factor(matrix);
    if scale < 1e-12 {
        return 0.0; // Degenerate matrix, no rotation
    }

    // Normalize to extract rotation
    let cos_theta = matrix[0] / scale;
    let sin_theta = matrix[4] / scale;

    sin_theta.atan2(cos_theta)
}

/// Apply Fourier transform to R96 projection data
pub fn apply_fourier_transform(projection: &mut AtlasProjection, inverse: bool) -> AtlasResult<()> {
    if projection.projection_type != ProjectionType::R96Fourier {
        return Err(AtlasError::InvalidInput(
            "projection must be R96_FOURIER type",
        ));
    }

    if let Some(ref mut r96_data) = projection.r96_fourier_data {
        if inverse {
            // Apply inverse Fourier transform
            r96_data.apply_inverse_fourier_transform()?;
        } else {
            // Apply forward Fourier transform
            r96_data.apply_fourier_transform()?;
        }

        // Update conservation checksum
        projection.total_conservation_sum = r96_data.conservation_checksum;

        // Verify conservation laws are still satisfied
        projection.verify()?;
    } else {
        return Err(AtlasError::LayerIntegrationError(
            "R96 Fourier data not available",
        ));
    }

    Ok(())
}

#[cfg(test)]
mod tests {
    use super::*;

    /// Create conservation-compliant test data (sum % 96 == 0)
    fn create_conservation_test_data(size: usize) -> Vec<u8> {
        // For now, use all zeros which definitely satisfies Layer 2 conservation
        vec![0u8; size]
    }

    #[test]
    fn test_scale_projection() {
        let source_data = create_conservation_test_data(4096); // One page of test data
        let mut projection = AtlasProjection::new_linear(&source_data).unwrap();

        let result = scale_projection(&mut projection, 2.0, 1.5);
        assert!(result.is_ok());

        // Verify transform params were set
        assert!(projection.transform_params.is_some());
        if let Some(params) = projection.transform_params {
            assert_eq!(params.scaling_factor, (2.0 * 1.5_f64).sqrt()); // Geometric mean
        }

        // Verify projection still passes validation
        assert!(projection.verify().is_ok());
    }

    #[test]
    fn test_rotate_projection() {
        let source_data = create_conservation_test_data(4096);
        let mut projection = AtlasProjection::new_linear(&source_data).unwrap();

        let angle = std::f64::consts::PI / 4.0; // 45 degrees
        let result = rotate_projection(&mut projection, angle, 0.0, 0.0);
        assert!(result.is_ok());

        // Verify transform params were set
        assert!(projection.transform_params.is_some());
        if let Some(params) = projection.transform_params {
            assert!((params.rotation_angle - angle).abs() < 1e-10);
        }

        // Verify projection still passes validation
        assert!(projection.verify().is_ok());
    }

    #[test]
    fn test_translate_projection() {
        let source_data = create_conservation_test_data(4096);
        let mut projection = AtlasProjection::new_linear(&source_data).unwrap();

        let result = translate_projection(&mut projection, 10.0, -5.0);
        assert!(result.is_ok());

        // Verify transform params were set
        assert!(projection.transform_params.is_some());
        if let Some(params) = projection.transform_params {
            assert_eq!(params.translation_x, 10.0);
            assert_eq!(params.translation_y, -5.0);
        }

        // Verify projection still passes validation
        assert!(projection.verify().is_ok());
    }

    #[test]
    fn test_linear_transform_matrix() {
        let source_data = create_conservation_test_data(4096);
        let mut projection = AtlasProjection::new_linear(&source_data).unwrap();

        // Identity matrix should not change anything
        #[rustfmt::skip]
        let identity = [
            1.0, 0.0, 0.0, 0.0,
            0.0, 1.0, 0.0, 0.0,
            0.0, 0.0, 1.0, 0.0,
            0.0, 0.0, 0.0, 1.0,
        ];

        let result = apply_linear_transform(&mut projection, &identity);
        assert!(result.is_ok());

        // Verify projection still passes validation
        assert!(projection.verify().is_ok());
    }

    #[test]
    fn test_invalid_scaling_factors() {
        // Create test data that satisfies conservation laws
        let mut source_data = Vec::new();
        for i in 0..1024 {
            source_data.push((i % 256) as u8);
            source_data.push(((i + 1) % 256) as u8);
            source_data.push(((i + 2) % 256) as u8);
            source_data.push(((i + 3) % 256) as u8);
        }
        let mut projection = AtlasProjection::new_linear(&source_data).unwrap();

        // Zero scaling should fail
        let result = scale_projection(&mut projection, 0.0, 1.0);
        assert!(result.is_err());

        // Negative scaling should fail
        let result = scale_projection(&mut projection, -1.0, 1.0);
        assert!(result.is_err());

        // Too large scaling should fail
        let result = scale_projection(&mut projection, MAX_SCALE_FACTOR * 2.0, 1.0);
        assert!(result.is_err());

        // Too small scaling should fail
        let result = scale_projection(&mut projection, MIN_SCALE_FACTOR / 2.0, 1.0);
        assert!(result.is_err());
    }

    #[test]
    fn test_invalid_transformation_matrix() {
        let source_data = create_conservation_test_data(4096);
        let mut projection = AtlasProjection::new_linear(&source_data).unwrap();

        // Singular matrix (determinant = 0) should fail
        #[rustfmt::skip]
        let singular = [
            1.0, 2.0, 0.0, 0.0,
            2.0, 4.0, 0.0, 0.0,
            0.0, 0.0, 1.0, 0.0,
            0.0, 0.0, 0.0, 1.0,
        ];

        let result = apply_linear_transform(&mut projection, &singular);
        assert!(result.is_err());

        // Matrix with NaN should fail
        #[rustfmt::skip]
        let nan_matrix = [
            f64::NAN, 0.0, 0.0, 0.0,
            0.0, 1.0, 0.0, 0.0,
            0.0, 0.0, 1.0, 0.0,
            0.0, 0.0, 0.0, 1.0,
        ];

        let result = apply_linear_transform(&mut projection, &nan_matrix);
        assert!(result.is_err());
    }

    #[test]
    fn test_verify_transform_params() {
        let valid_params = TransformationParams {
            scaling_factor: 1.5,
            rotation_angle: std::f64::consts::PI / 3.0,
            translation_x: 100.0,
            translation_y: -50.0,
        };

        assert!(verify_transform_params(&valid_params));

        let invalid_params = TransformationParams {
            scaling_factor: 0.0, // Invalid: too small
            rotation_angle: std::f64::consts::PI / 3.0,
            translation_x: 100.0,
            translation_y: -50.0,
        };

        assert!(!verify_transform_params(&invalid_params));
    }

    #[test]
    fn test_combined_transformations() {
        let source_data = create_conservation_test_data(8192); // Two pages
        let mut projection = AtlasProjection::new_linear(&source_data).unwrap();

        // Apply multiple transformations in sequence
        assert!(scale_projection(&mut projection, 1.5, 1.5).is_ok());
        assert!(rotate_projection(&mut projection, std::f64::consts::PI / 6.0, 0.0, 0.0).is_ok());
        assert!(translate_projection(&mut projection, 20.0, 30.0).is_ok());

        // Verify final state
        assert!(projection.verify().is_ok());
        assert!(projection.transform_params.is_some());

        if let Some(params) = projection.transform_params {
            assert!(params.scaling_factor > 1.0); // Should reflect scaling
            assert!(params.translation_x == 20.0);
            assert!(params.translation_y == 30.0);
        }
    }

    #[test]
    fn test_fourier_transform_invalid_type() {
        let source_data = create_conservation_test_data(4096);
        let mut projection = AtlasProjection::new_linear(&source_data).unwrap(); // LINEAR, not R96_FOURIER

        let result = apply_fourier_transform(&mut projection, false);
        assert!(result.is_err());
    }

    #[test]
    fn test_fourier_transform_valid() {
        let source_data = create_conservation_test_data(4096);
        let mut projection = AtlasProjection::new_r96_fourier(&source_data).unwrap();

        // Forward transform should work
        let result = apply_fourier_transform(&mut projection, false);
        assert!(result.is_ok());

        // Inverse transform should work
        let result = apply_fourier_transform(&mut projection, true);
        assert!(result.is_ok());

        // Verify projection still passes validation
        assert!(projection.verify().is_ok());
    }
}
