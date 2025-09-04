//! Core manifold operations and geometric primitives

use crate::{error::*, ffi, types::*};

/// Type alias for coordinate bounds arrays
type DomainBounds<const N: usize> = [(Float, Float); N];
type CodomainBounds<const M: usize> = [(Float, Float); M];

// =============================================================================
// Universal Numbers Helper Functions
// =============================================================================

/// Compute spectral moment Tr(M^k) which is a Universal Number
///
/// Spectral moments are invariant under similarity transformations
/// and compose algebraically, making them ideal Universal Numbers.
fn compute_spectral_moment<const N: usize>(matrix: &TransformMatrix<N, N>, k: u32) -> Float {
    match k {
        1 => {
            // Tr(M) = sum of diagonal elements
            (0..N).map(|i| matrix.elements[i][i]).sum()
        },
        2 => {
            // Tr(M^2) = sum of squared elements (Frobenius norm squared)
            (0..N)
                .map(|i| {
                    (0..N).map(|j| matrix.elements[i][j] * matrix.elements[j][i]).sum::<Float>()
                })
                .sum()
        },
        3 => {
            // Tr(M^3) - simplified approximation for computational efficiency
            let trace = (0..N).map(|i| matrix.elements[i][i]).sum::<Float>();
            let trace_squared = (0..N)
                .map(|i| {
                    (0..N).map(|j| matrix.elements[i][j] * matrix.elements[j][i]).sum::<Float>()
                })
                .sum::<Float>();
            // Use Newton's identities: tr(M^3) ≈ tr(M)^3 - 3*tr(M)*tr(M^2) + 3*det(M) for approximation
            trace.powi(3) - 3.0 * trace * trace_squared
        },
        _ => {
            // For higher powers, use trace as approximation
            (0..N).map(|i| matrix.elements[i][i]).sum::<Float>().powf(k as Float)
        },
    }
}

/// Compute harmonic coupling between matrix elements based on R96 resonance
///
/// Uses the Layer 3 harmonization rule: two elements are coupled if their
/// resonance classes r1, r2 satisfy (r1 + r2) % 96 == 0
fn compute_harmonic_coupling<const N: usize>(
    matrix: &TransformMatrix<N, N>,
    i: usize,
    j: usize,
) -> Float {
    if i >= N || j >= N {
        return 0.0;
    }

    // Classify matrix elements to R96 resonance classes
    // Use fixed-point representation to convert float to byte for classification
    let elem_i = (matrix.elements[i][i].abs() * 255.0).min(255.0) as u8;
    let elem_j = (matrix.elements[j][j].abs() * 255.0).min(255.0) as u8;

    // SAFETY: FFI calls with single bytes are always safe
    let class_i = unsafe { ffi::atlas_r96_classify(elem_i) };
    let class_j = unsafe { ffi::atlas_r96_classify(elem_j) };

    // Check if classes harmonize using Layer 3 FFI
    let harmonizes = unsafe { ffi::atlas_r96_harmonizes(class_i, class_j) };

    if harmonizes {
        // Harmonic coupling strength based on conservation energy
        let coupling_strength = (matrix.elements[i][j] + matrix.elements[j][i]) * 0.5;
        coupling_strength * 0.1 // Scale down for stability
    } else {
        0.0
    }
}

#[cfg(feature = "std")]
use std::vec::Vec;

#[cfg(not(feature = "std"))]
use alloc::vec::Vec;

impl ManifoldDescriptor {
    /// Create a new manifold descriptor
    pub fn new(
        intrinsic_dim: Dimension,
        embedding_dim: Dimension,
        curvature: [Float; 4],
        metric_det: Float,
    ) -> Self {
        Self {
            intrinsic_dim,
            embedding_dim,
            curvature,
            metric_det,
        }
    }

    /// Create a flat Euclidean manifold
    pub fn euclidean(intrinsic_dim: Dimension, embedding_dim: Dimension) -> Self {
        Self::new(
            intrinsic_dim,
            embedding_dim,
            [0.0, 0.0, 0.0, 0.0], // Zero curvature
            1.0,                  // Unit metric determinant
        )
    }

    /// Create a spherical manifold with given radius
    pub fn spherical(radius: Float, embedding_dim: Dimension) -> Self {
        let curvature = 1.0 / (radius * radius);
        Self::new(
            embedding_dim - 1, // Sphere is one dimension less than embedding
            embedding_dim,
            [curvature, 0.0, 0.0, 0.0],
            radius.powi(i32::try_from(embedding_dim).unwrap_or(1) - 1),
        )
    }

    /// Create a hyperbolic manifold
    pub fn hyperbolic(intrinsic_dim: Dimension, embedding_dim: Dimension) -> Self {
        Self::new(
            intrinsic_dim,
            embedding_dim,
            [-1.0, 0.0, 0.0, 0.0], // Negative curvature
            1.0,
        )
    }

    /// Check if this is a Riemannian manifold
    pub fn is_riemannian(&self) -> bool {
        self.metric_det > 0.0
    }

    /// Get the Gaussian curvature
    pub fn gaussian_curvature(&self) -> Float {
        self.curvature[0]
    }

    /// Get the mean curvature
    pub fn mean_curvature(&self) -> Float {
        self.curvature[1]
    }
}

/// Manifold atlas containing coordinate charts
pub struct ManifoldAtlas<const N: usize, const M: usize> {
    /// Manifold descriptor
    descriptor: ManifoldDescriptor,
    /// Collection of coordinate charts
    charts: Vec<CoordinateChart<N, M>>,
    /// Active chart for current operations
    active_chart: usize,
}

/// Coordinate chart mapping between manifold and Euclidean space
pub struct CoordinateChart<const N: usize, const M: usize> {
    /// Domain bounds in manifold coordinates
    domain: DomainBounds<N>,
    /// Codomain bounds in Euclidean coordinates  
    codomain: CodomainBounds<M>,
    /// Forward transformation (manifold to Euclidean)
    forward_transform: TransformMatrix<M, N>,
    /// Inverse transformation (Euclidean to manifold)
    inverse_transform: TransformMatrix<N, M>,
}

impl<const N: usize, const M: usize> ManifoldAtlas<N, M> {
    /// Create a new manifold atlas
    pub fn new(descriptor: ManifoldDescriptor) -> Self {
        Self {
            descriptor,
            charts: Vec::new(),
            active_chart: 0,
        }
    }

    /// Add a coordinate chart to the atlas
    pub fn add_chart(&mut self, chart: CoordinateChart<N, M>) -> usize {
        self.charts.push(chart);
        self.charts.len() - 1
    }

    /// Set the active coordinate chart
    pub fn set_active_chart(&mut self, index: usize) -> AtlasResult<()> {
        if index >= self.charts.len() {
            return Err(AtlasError::InvalidInput("chart index out of bounds"));
        }
        self.active_chart = index;
        Ok(())
    }

    /// Transform a point from manifold to Euclidean coordinates
    pub fn to_euclidean(&self, point: &AtlasPoint<N>) -> AtlasResult<AtlasPoint<M>> {
        let chart = self
            .charts
            .get(self.active_chart)
            .ok_or(AtlasError::InvalidInput("no active chart"))?;

        // Check if point is in chart domain
        for (i, &coord) in point.coords.iter().enumerate() {
            let (min, max) = chart.domain[i];
            if coord < min || coord > max {
                return Err(AtlasError::CoordinateError("point outside chart domain"));
            }
        }

        Ok(chart.forward_transform.transform_point(point))
    }

    /// Transform a point from Euclidean to manifold coordinates
    pub fn from_euclidean(&self, point: &AtlasPoint<M>) -> AtlasResult<AtlasPoint<N>> {
        let chart = self
            .charts
            .get(self.active_chart)
            .ok_or(AtlasError::InvalidInput("no active chart"))?;

        // Check if point is in chart codomain
        for (i, &coord) in point.coords.iter().enumerate() {
            let (min, max) = chart.codomain[i];
            if coord < min || coord > max {
                return Err(AtlasError::CoordinateError("point outside chart codomain"));
            }
        }

        Ok(chart.inverse_transform.transform_point(point))
    }

    /// Compute the metric tensor at a point using Universal Numbers
    pub fn metric_tensor(&self, point: &AtlasPoint<N>) -> AtlasResult<TransformMatrix<N, N>> {
        // Simplified Universal Numbers approach: use scaled identity matrix as base
        // with spectral moments (which are Universal Numbers) for corrections

        let mut metric = TransformMatrix::identity();
        let scale = self.descriptor.metric_det.powf(1.0 / N as Float);

        // Apply base scaling from manifold descriptor
        for i in 0..N {
            metric.elements[i][i] = scale;
        }

        // Add spectral moment corrections (Universal Numbers that compose algebraically)
        let coord_matrix = self.point_to_matrix(point);
        let trace_m = compute_spectral_moment(&coord_matrix, 1); // Tr(M)
        let trace_m2 = compute_spectral_moment(&coord_matrix, 2); // Tr(M^2)

        // Apply trace invariants as Universal Number corrections
        let trace_correction = trace_m / (N as Float);
        let norm_correction = (trace_m2 / (N as Float)).sqrt();

        for i in 0..N {
            // Diagonal elements: base scale + trace correction
            metric.elements[i][i] = scale * (1.0 + trace_correction * 0.1);

            // Off-diagonal elements: harmonic coupling based on R96 pairing
            for j in 0..N {
                if i != j {
                    metric.elements[i][j] =
                        compute_harmonic_coupling(&coord_matrix, i, j) * norm_correction * 0.05;
                }
            }
        }

        Ok(metric)
    }

    /// Convert point coordinates to matrix for spectral analysis
    fn point_to_matrix(&self, point: &AtlasPoint<N>) -> TransformMatrix<N, N> {
        let mut matrix = TransformMatrix::zero();

        // Create matrix from point coordinates using cyclic structure
        for i in 0..N {
            for j in 0..N {
                if i == j {
                    matrix.elements[i][j] = point.coords[i];
                } else {
                    // Off-diagonal elements based on coordinate interactions
                    let idx1 = i % point.coords.len();
                    let idx2 = j % point.coords.len();
                    matrix.elements[i][j] = point.coords[idx1] * point.coords[idx2] * 0.1;
                }
            }
        }

        matrix
    }

    /// Compute determinant for NxN matrix (simplified for small N)
    fn compute_determinant_nxn(&self, matrix: &TransformMatrix<N, N>) -> AtlasResult<Float> {
        match N {
            1 => Ok(matrix.elements[0][0]),
            2 => Ok(matrix.elements[0][0] * matrix.elements[1][1]
                - matrix.elements[0][1] * matrix.elements[1][0]),
            3 => {
                // 3x3 determinant using rule of Sarrus
                let a = matrix.elements[0][0];
                let b = matrix.elements[0][1];
                let c = matrix.elements[0][2];
                let d = matrix.elements[1][0];
                let e = matrix.elements[1][1];
                let f = matrix.elements[1][2];
                let g = matrix.elements[2][0];
                let h = matrix.elements[2][1];
                let i = matrix.elements[2][2];

                Ok(a * (e * i - f * h) - b * (d * i - f * g) + c * (d * h - e * g))
            },
            _ => {
                // For larger matrices, use LU decomposition or return approximate value
                // For now, return the product of diagonal elements as approximation
                let mut det = 1.0;
                for i in 0..N {
                    det *= matrix.elements[i][i];
                }
                Ok(det)
            },
        }
    }

    /// Compute geodesic distance between two points
    pub fn geodesic_distance(&self, p1: &AtlasPoint<N>, p2: &AtlasPoint<N>) -> AtlasResult<Float> {
        match self.descriptor.gaussian_curvature() {
            0.0 => {
                // Euclidean distance for flat manifolds
                Ok(p1.distance(p2))
            },
            curvature if curvature > 0.0 => {
                // Spherical distance
                Self::spherical_distance(p1, p2)
            },
            _ => {
                // Hyperbolic distance
                Self::hyperbolic_distance(p1, p2)
            },
        }
    }

    /// Compute curvature at a specific point using spectral moments (Universal Numbers)
    pub fn curvature_at_point(&self, point: &AtlasPoint<N>) -> AtlasResult<[Float; 4]> {
        // Simplified approach using spectral moments of the metric tensor
        // This replaces complex Christoffel symbols and Riemann tensor computation

        let metric = self.metric_tensor(point)?;

        // Compute spectral moments which are Universal Numbers
        let tr_m = compute_spectral_moment(&metric, 1); // Tr(M)
        let tr_m2 = compute_spectral_moment(&metric, 2); // Tr(M^2)
        let tr_m3 = compute_spectral_moment(&metric, 3); // Tr(M^3)

        let mut curvature = [0.0; 4];

        // Gaussian curvature from spectral moments
        // For 2D: K = (tr²M - trM²) / det(M)
        let det_approx = tr_m * tr_m - tr_m2;
        if det_approx.abs() > 1e-10 {
            curvature[0] = (tr_m * tr_m - tr_m2) / det_approx.abs();
        } else {
            curvature[0] = 0.0;
        }

        // Mean curvature from spectral moments
        // H = tr(M) / (2 * sqrt(det(M)))
        if det_approx > 1e-10 {
            curvature[1] = tr_m / (2.0 * det_approx.sqrt());
        } else {
            curvature[1] = 0.0;
        }

        // Principal curvatures from characteristic polynomial
        // Using Newton's identities with spectral moments
        let discriminant = curvature[1] * curvature[1] - curvature[0];
        if discriminant >= 0.0 {
            let sqrt_disc = discriminant.sqrt();
            curvature[2] = curvature[1] + sqrt_disc; // k1 (max)
            curvature[3] = curvature[1] - sqrt_disc; // k2 (min)
        } else {
            // Use higher order spectral moment for complex case
            let higher_order_correction = tr_m3 / (tr_m2 * N as Float);
            curvature[2] = curvature[1] + higher_order_correction;
            curvature[3] = curvature[1] - higher_order_correction;
        }

        // Apply conservation scaling (Universal Numbers must preserve conservation)
        let conservation_scale = self.descriptor.metric_det.sqrt();
        for c in &mut curvature {
            *c *= conservation_scale;
        }

        Ok(curvature)
    }

    /// Project a vector onto the tangent space at a point
    pub fn project_to_tangent_space(
        &self,
        point: &AtlasPoint<N>,
        vector: &AtlasVector<N>,
    ) -> AtlasResult<AtlasVector<N>> {
        // For a general manifold embedded in Euclidean space,
        // projection requires knowing the normal vectors

        // Simplified implementation: assume the manifold is nearly flat locally
        let metric = self.metric_tensor(point)?;
        let projected = metric.multiply_vector(vector);

        Ok(projected)
    }

    // Helper methods for specific geometries

    fn spherical_distance(p1: &AtlasPoint<N>, p2: &AtlasPoint<N>) -> AtlasResult<Float> {
        if N < 2 {
            return Err(AtlasError::InvalidDimension(u32::try_from(N).unwrap_or(0)));
        }

        // Convert to unit sphere coordinates and compute great circle distance
        let r1 = p1.coords.iter().map(|&x| x * x).sum::<Float>().sqrt();
        let r2 = p2.coords.iter().map(|&x| x * x).sum::<Float>().sqrt();

        if r1 == 0.0 || r2 == 0.0 {
            return Err(AtlasError::NumericalError("zero radius point"));
        }

        let mut dot_product = 0.0;
        for i in 0..N {
            dot_product += (p1.coords[i] / r1) * (p2.coords[i] / r2);
        }

        let angle = dot_product.clamp(-1.0, 1.0).acos();
        let radius = (r1 + r2) / 2.0; // Average radius

        Ok(radius * angle)
    }

    fn hyperbolic_distance(p1: &AtlasPoint<N>, p2: &AtlasPoint<N>) -> AtlasResult<Float> {
        // Simplified hyperbolic distance in the Poincaré disk model
        // This assumes the manifold is embedded in the unit disk

        let d_euclidean = p1.distance(p2);
        let r1_squared = p1.coords.iter().map(|&x| x * x).sum::<Float>();
        let r2_squared = p2.coords.iter().map(|&x| x * x).sum::<Float>();

        if r1_squared >= 1.0 || r2_squared >= 1.0 {
            return Err(AtlasError::CoordinateError(
                "points outside hyperbolic disk",
            ));
        }

        let numerator = 2.0 * d_euclidean * d_euclidean;
        let denominator = (1.0 - r1_squared) * (1.0 - r2_squared);

        if denominator <= 0.0 {
            return Err(AtlasError::NumericalError("invalid hyperbolic distance"));
        }

        Ok((1.0 + numerator / denominator).sqrt().ln() * 2.0)
    }
}

impl<const N: usize, const M: usize> CoordinateChart<N, M> {
    /// Create a new coordinate chart
    pub fn new(
        domain: DomainBounds<N>,
        codomain: CodomainBounds<M>,
        forward_transform: TransformMatrix<M, N>,
        inverse_transform: TransformMatrix<N, M>,
    ) -> Self {
        Self {
            domain,
            codomain,
            forward_transform,
            inverse_transform,
        }
    }

    /// Create an identity chart (for square matrices where M == N)
    pub fn identity_square(bounds: DomainBounds<N>) -> CoordinateChart<N, N> {
        CoordinateChart::<N, N>::new(
            bounds,
            bounds,
            TransformMatrix::<N, N>::identity(),
            TransformMatrix::<N, N>::identity(),
        )
    }

    /// Check if a point is in the chart's domain
    pub fn contains_point(&self, point: &AtlasPoint<N>) -> bool {
        for (i, &coord) in point.coords.iter().enumerate() {
            let (min, max) = self.domain[i];
            if coord < min || coord > max {
                return false;
            }
        }
        true
    }
}
