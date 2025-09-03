//! Core manifold operations and geometric primitives

use crate::{types::*, error::*};

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
            1.0, // Unit metric determinant
        )
    }
    
    /// Create a spherical manifold with given radius
    pub fn spherical(radius: Float, embedding_dim: Dimension) -> Self {
        let curvature = 1.0 / (radius * radius);
        Self::new(
            embedding_dim - 1, // Sphere is one dimension less than embedding
            embedding_dim,
            [curvature, 0.0, 0.0, 0.0],
            radius.powi(embedding_dim as i32 - 1),
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
    domain: [(Float, Float); N],
    /// Codomain bounds in Euclidean coordinates  
    codomain: [(Float, Float); M],
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
        let chart = self.charts.get(self.active_chart)
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
        let chart = self.charts.get(self.active_chart)
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
    
    /// Compute the metric tensor at a point
    pub fn metric_tensor(&self, point: &AtlasPoint<N>) -> AtlasResult<TransformMatrix<N, N>> {
        // For a general manifold, the metric tensor computation requires
        // knowledge of the embedding. This is a simplified implementation.
        let mut metric = TransformMatrix::zero();
        
        // Use the manifold's metric determinant to scale an identity-like metric
        let scale = self.descriptor.metric_det.powf(1.0 / N as Float);
        for i in 0..N {
            metric.elements[i][i] = scale;
        }
        
        // Apply curvature corrections
        let curvature = self.descriptor.gaussian_curvature();
        if curvature != 0.0 {
            let curvature_correction = 1.0 + curvature * point.coords[0] * point.coords[0];
            for i in 0..N {
                metric.elements[i][i] *= curvature_correction;
            }
        }
        
        Ok(metric)
    }
    
    /// Compute geodesic distance between two points
    pub fn geodesic_distance(&self, p1: &AtlasPoint<N>, p2: &AtlasPoint<N>) -> AtlasResult<Float> {
        match self.descriptor.gaussian_curvature() {
            curvature if curvature == 0.0 => {
                // Euclidean distance for flat manifolds
                Ok(p1.distance(p2))
            },
            curvature if curvature > 0.0 => {
                // Spherical distance
                self.spherical_distance(p1, p2)
            },
            _ => {
                // Hyperbolic distance
                self.hyperbolic_distance(p1, p2)
            }
        }
    }
    
    /// Compute curvature at a specific point
    pub fn curvature_at_point(&self, point: &AtlasPoint<N>) -> AtlasResult<[Float; 4]> {
        let mut curvature = self.descriptor.curvature;
        
        // Apply position-dependent curvature corrections
        let position_factor = point.coords.iter().map(|&x| x * x).sum::<Float>().sqrt();
        
        // This is a simplified model - real curvature computation requires
        // second derivatives of the metric tensor
        curvature[0] *= 1.0 + 0.1 * position_factor;
        curvature[1] *= 1.0 + 0.05 * position_factor;
        
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
    
    fn spherical_distance(&self, p1: &AtlasPoint<N>, p2: &AtlasPoint<N>) -> AtlasResult<Float> {
        if N < 2 {
            return Err(AtlasError::InvalidDimension(N as u32));
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
    
    fn hyperbolic_distance(&self, p1: &AtlasPoint<N>, p2: &AtlasPoint<N>) -> AtlasResult<Float> {
        // Simplified hyperbolic distance in the Poincaré disk model
        // This assumes the manifold is embedded in the unit disk
        
        let d_euclidean = p1.distance(p2);
        let r1_squared = p1.coords.iter().map(|&x| x * x).sum::<Float>();
        let r2_squared = p2.coords.iter().map(|&x| x * x).sum::<Float>();
        
        if r1_squared >= 1.0 || r2_squared >= 1.0 {
            return Err(AtlasError::CoordinateError("points outside hyperbolic disk"));
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
        domain: [(Float, Float); N],
        codomain: [(Float, Float); M],
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
    pub fn identity_square(bounds: [(Float, Float); N]) -> CoordinateChart<N, N> {
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