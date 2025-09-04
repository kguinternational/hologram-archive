//! Coordinate system transformations and conversions

use crate::{error::*, types::*};

/// Coordinate system types supported by the manifold layer
#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub enum CoordinateSystem {
    /// Cartesian coordinates (x, y, z, ...)
    Cartesian,
    /// Spherical coordinates (r, θ, φ, ...)
    Spherical,
    /// Cylindrical coordinates (ρ, φ, z)
    Cylindrical,
    /// Polar coordinates (r, θ)
    Polar,
    /// Homogeneous coordinates
    Homogeneous,
    /// Barycentric coordinates
    Barycentric,
    /// Custom coordinate system
    Custom(&'static str),
}

/// Coordinate transformation context
pub struct CoordinateTransform<const FROM: usize, const TO: usize> {
    /// Source coordinate system
    from_system: CoordinateSystem,
    /// Target coordinate system
    to_system: CoordinateSystem,
    /// Transformation parameters (scale, offset, rotation, etc.)
    #[allow(dead_code)]
    parameters: [Float; 16],
}

impl<const FROM: usize, const TO: usize> CoordinateTransform<FROM, TO> {
    /// Create a new coordinate transformation
    pub fn new(
        from_system: CoordinateSystem,
        to_system: CoordinateSystem,
        parameters: [Float; 16],
    ) -> Self {
        Self {
            from_system,
            to_system,
            parameters,
        }
    }

    /// Transform a point from source to target coordinates
    pub fn transform_point(&self, point: &AtlasPoint<FROM>) -> AtlasResult<AtlasPoint<TO>> {
        match (self.from_system, self.to_system) {
            (CoordinateSystem::Cartesian, CoordinateSystem::Spherical) => {
                Self::cartesian_to_spherical(point)
            },
            (CoordinateSystem::Spherical, CoordinateSystem::Cartesian) => {
                Self::spherical_to_cartesian(point)
            },
            (CoordinateSystem::Cartesian, CoordinateSystem::Cylindrical) => {
                Self::cartesian_to_cylindrical(point)
            },
            (CoordinateSystem::Cylindrical, CoordinateSystem::Cartesian) => {
                Self::cylindrical_to_cartesian(point)
            },
            (CoordinateSystem::Cartesian, CoordinateSystem::Polar) => {
                Self::cartesian_to_polar(point)
            },
            (CoordinateSystem::Polar, CoordinateSystem::Cartesian) => {
                Self::polar_to_cartesian(self, point)
            },
            (CoordinateSystem::Cartesian, CoordinateSystem::Homogeneous) => {
                Self::cartesian_to_homogeneous(self, point)
            },
            (CoordinateSystem::Homogeneous, CoordinateSystem::Cartesian) => {
                Self::homogeneous_to_cartesian(self, point)
            },
            _ => Err(AtlasError::CoordinateError(
                "unsupported coordinate transformation",
            )),
        }
    }

    /// Transform a vector from source to target coordinates
    pub fn transform_vector(&self, vector: &AtlasVector<FROM>) -> AtlasResult<AtlasVector<TO>> {
        // Vector transformation is similar to point transformation but without translation
        let point = AtlasPoint::new(vector.components);
        let transformed_point = self.transform_point(&point)?;
        Ok(AtlasVector::new(transformed_point.coords))
    }

    /// Get the Jacobian matrix of the transformation at a given point
    pub fn jacobian(&self, point: &AtlasPoint<FROM>) -> AtlasResult<TransformMatrix<TO, FROM>> {
        // Compute numerical Jacobian using finite differences
        let epsilon = 1e-8;
        let mut jacobian = TransformMatrix::zero();

        let base_transformed = self.transform_point(point)?;

        for i in 0..FROM {
            let mut perturbed_point = *point;
            perturbed_point.coords[i] += epsilon;

            let perturbed_transformed = self.transform_point(&perturbed_point)?;

            for j in 0..TO {
                jacobian.elements[j][i] =
                    (perturbed_transformed.coords[j] - base_transformed.coords[j]) / epsilon;
            }
        }

        Ok(jacobian)
    }

    // Specific coordinate transformations

    fn cartesian_to_spherical(point: &AtlasPoint<FROM>) -> AtlasResult<AtlasPoint<TO>> {
        if FROM < 2 || TO < 2 {
            return Err(AtlasError::InvalidDimension(
                core::cmp::min(FROM, TO).try_into().unwrap_or(u32::MAX),
            ));
        }

        let x = point.coords[0];
        let y = point.coords[1];
        let z = if FROM > 2 { point.coords[2] } else { 0.0 };

        let r = (x * x + y * y + z * z).sqrt();
        let theta = if r > 0.0 { (z / r).acos() } else { 0.0 };
        let phi = y.atan2(x);

        let mut result_coords = [0.0; TO];
        result_coords[0] = r;
        if TO > 1 {
            result_coords[1] = theta;
        }
        if TO > 2 {
            result_coords[2] = phi;
        }

        // Copy remaining coordinates if any
        let copy_end = core::cmp::min(FROM, TO);
        if copy_end > 3 {
            result_coords[3..copy_end].copy_from_slice(&point.coords[3..copy_end]);
        }

        Ok(AtlasPoint::new(result_coords))
    }

    fn spherical_to_cartesian(point: &AtlasPoint<FROM>) -> AtlasResult<AtlasPoint<TO>> {
        if FROM < 2 || TO < 2 {
            return Err(AtlasError::InvalidDimension(
                core::cmp::min(FROM, TO).try_into().unwrap_or(u32::MAX),
            ));
        }

        let r = point.coords[0];
        let theta = if FROM > 1 { point.coords[1] } else { 0.0 };
        let phi = if FROM > 2 { point.coords[2] } else { 0.0 };

        let sin_theta = theta.sin();
        let cos_theta = theta.cos();
        let sin_phi = phi.sin();
        let cos_phi = phi.cos();

        let mut result_coords = [0.0; TO];
        result_coords[0] = r * sin_theta * cos_phi;
        if TO > 1 {
            result_coords[1] = r * sin_theta * sin_phi;
        }
        if TO > 2 {
            result_coords[2] = r * cos_theta;
        }

        // Copy remaining coordinates if any
        let copy_end = core::cmp::min(FROM, TO);
        if copy_end > 3 {
            result_coords[3..copy_end].copy_from_slice(&point.coords[3..copy_end]);
        }

        Ok(AtlasPoint::new(result_coords))
    }

    fn cartesian_to_cylindrical(point: &AtlasPoint<FROM>) -> AtlasResult<AtlasPoint<TO>> {
        if FROM < 3 || TO < 3 {
            return Err(AtlasError::InvalidDimension(
                core::cmp::min(FROM, TO).try_into().unwrap_or(u32::MAX),
            ));
        }

        let x = point.coords[0];
        let y = point.coords[1];
        let z = point.coords[2];

        let rho = (x * x + y * y).sqrt();
        let phi = y.atan2(x);

        let mut result_coords = [0.0; TO];
        result_coords[0] = rho;
        result_coords[1] = phi;
        result_coords[2] = z;

        // Copy remaining coordinates if any
        let copy_end = core::cmp::min(FROM, TO);
        if copy_end > 3 {
            result_coords[3..copy_end].copy_from_slice(&point.coords[3..copy_end]);
        }

        Ok(AtlasPoint::new(result_coords))
    }

    fn cylindrical_to_cartesian(point: &AtlasPoint<FROM>) -> AtlasResult<AtlasPoint<TO>> {
        if FROM < 3 || TO < 3 {
            return Err(AtlasError::InvalidDimension(
                core::cmp::min(FROM, TO).try_into().unwrap_or(u32::MAX),
            ));
        }

        let rho = point.coords[0];
        let phi = point.coords[1];
        let z = point.coords[2];

        let x = rho * phi.cos();
        let y = rho * phi.sin();

        let mut result_coords = [0.0; TO];
        result_coords[0] = x;
        result_coords[1] = y;
        result_coords[2] = z;

        // Copy remaining coordinates if any
        let copy_end = core::cmp::min(FROM, TO);
        if copy_end > 3 {
            result_coords[3..copy_end].copy_from_slice(&point.coords[3..copy_end]);
        }

        Ok(AtlasPoint::new(result_coords))
    }

    fn cartesian_to_polar(point: &AtlasPoint<FROM>) -> AtlasResult<AtlasPoint<TO>> {
        if FROM < 2 || TO < 2 {
            return Err(AtlasError::InvalidDimension(
                core::cmp::min(FROM, TO).try_into().unwrap_or(u32::MAX),
            ));
        }

        let x = point.coords[0];
        let y = point.coords[1];

        let r = (x * x + y * y).sqrt();
        let theta = y.atan2(x);

        let mut result_coords = [0.0; TO];
        result_coords[0] = r;
        result_coords[1] = theta;

        // Copy remaining coordinates if any
        let copy_end = core::cmp::min(FROM, TO);
        if copy_end > 2 {
            result_coords[2..copy_end].copy_from_slice(&point.coords[2..copy_end]);
        }

        Ok(AtlasPoint::new(result_coords))
    }

    fn polar_to_cartesian(_: &Self, point: &AtlasPoint<FROM>) -> AtlasResult<AtlasPoint<TO>> {
        if FROM < 2 || TO < 2 {
            return Err(AtlasError::InvalidDimension(
                core::cmp::min(FROM, TO).try_into().unwrap_or(u32::MAX),
            ));
        }

        let r = point.coords[0];
        let theta = point.coords[1];

        let x = r * theta.cos();
        let y = r * theta.sin();

        let mut result_coords = [0.0; TO];
        result_coords[0] = x;
        result_coords[1] = y;

        // Copy remaining coordinates if any
        let copy_end = core::cmp::min(FROM, TO);
        if copy_end > 2 {
            result_coords[2..copy_end].copy_from_slice(&point.coords[2..copy_end]);
        }

        Ok(AtlasPoint::new(result_coords))
    }

    fn cartesian_to_homogeneous(_: &Self, point: &AtlasPoint<FROM>) -> AtlasResult<AtlasPoint<TO>> {
        if TO != FROM + 1 {
            return Err(AtlasError::InvalidDimension(
                TO.try_into().unwrap_or(u32::MAX),
            ));
        }

        let mut result_coords = [0.0; TO];

        // Copy all Cartesian coordinates
        result_coords[..FROM].copy_from_slice(&point.coords[..FROM]);

        // Set homogeneous coordinate to 1
        result_coords[FROM] = 1.0;

        Ok(AtlasPoint::new(result_coords))
    }

    fn homogeneous_to_cartesian(_: &Self, point: &AtlasPoint<FROM>) -> AtlasResult<AtlasPoint<TO>> {
        if FROM != TO + 1 {
            return Err(AtlasError::InvalidDimension(
                FROM.try_into().unwrap_or(u32::MAX),
            ));
        }

        let w = point.coords[FROM - 1];
        if w == 0.0 {
            return Err(AtlasError::NumericalError("homogeneous coordinate is zero"));
        }

        let mut result_coords = [0.0; TO];

        // Divide by homogeneous coordinate
        for (i, coord) in result_coords.iter_mut().enumerate().take(TO) {
            *coord = point.coords[i] / w;
        }

        Ok(AtlasPoint::new(result_coords))
    }
}

/// Utility functions for coordinate operations
pub mod util {
    use super::*;

    /// Create a rotation matrix around the X-axis (3D)
    pub fn rotation_x(angle: Float) -> TransformMatrix<3, 3> {
        let cos_a = angle.cos();
        let sin_a = angle.sin();

        let mut matrix = TransformMatrix::zero();
        matrix.elements[0][0] = 1.0;
        matrix.elements[1][1] = cos_a;
        matrix.elements[1][2] = -sin_a;
        matrix.elements[2][1] = sin_a;
        matrix.elements[2][2] = cos_a;

        matrix
    }

    /// Create a rotation matrix around the Y-axis (3D)
    pub fn rotation_y(angle: Float) -> TransformMatrix<3, 3> {
        let cos_a = angle.cos();
        let sin_a = angle.sin();

        let mut matrix = TransformMatrix::zero();
        matrix.elements[0][0] = cos_a;
        matrix.elements[0][2] = sin_a;
        matrix.elements[1][1] = 1.0;
        matrix.elements[2][0] = -sin_a;
        matrix.elements[2][2] = cos_a;

        matrix
    }

    /// Create a rotation matrix around the Z-axis (3D)
    pub fn rotation_z(angle: Float) -> TransformMatrix<3, 3> {
        let cos_a = angle.cos();
        let sin_a = angle.sin();

        let mut matrix = TransformMatrix::zero();
        matrix.elements[0][0] = cos_a;
        matrix.elements[0][1] = -sin_a;
        matrix.elements[1][0] = sin_a;
        matrix.elements[1][1] = cos_a;
        matrix.elements[2][2] = 1.0;

        matrix
    }

    /// Create a scaling transformation matrix
    pub fn scaling<const N: usize>(factors: &[Float; N]) -> TransformMatrix<N, N> {
        let mut matrix = TransformMatrix::zero();
        for (i, &factor) in factors.iter().enumerate().take(N) {
            matrix.elements[i][i] = factor;
        }
        matrix
    }

    /// Create a translation transformation matrix (homogeneous coordinates)
    /// Returns a matrix of size (N+1) x (N+1)
    pub fn translation_3d(offset: &[Float; 3]) -> TransformMatrix<4, 4> {
        let mut matrix = TransformMatrix::identity();
        for (i, &value) in offset.iter().enumerate().take(3) {
            matrix.elements[i][3] = value;
        }
        matrix
    }

    /// Create a 2D translation transformation matrix (homogeneous coordinates)
    pub fn translation_2d(offset: &[Float; 2]) -> TransformMatrix<3, 3> {
        let mut matrix = TransformMatrix::identity();
        for (i, &value) in offset.iter().enumerate().take(2) {
            matrix.elements[i][2] = value;
        }
        matrix
    }

    /// Create a 2D rotation transformation matrix
    pub fn rotation_2d(angle: Float) -> TransformMatrix<2, 2> {
        let cos_a = angle.cos();
        let sin_a = angle.sin();

        let mut matrix = TransformMatrix::zero();
        matrix.elements[0][0] = cos_a;
        matrix.elements[0][1] = -sin_a;
        matrix.elements[1][0] = sin_a;
        matrix.elements[1][1] = cos_a;
        matrix
    }

    /// Create a 2D rotation transformation matrix (homogeneous coordinates)
    pub fn rotation_2d_homogeneous(angle: Float) -> TransformMatrix<3, 3> {
        let cos_a = angle.cos();
        let sin_a = angle.sin();

        let mut matrix = TransformMatrix::identity();
        matrix.elements[0][0] = cos_a;
        matrix.elements[0][1] = -sin_a;
        matrix.elements[1][0] = sin_a;
        matrix.elements[1][1] = cos_a;
        matrix
    }
}
