//! Linear algebra operations for manifold transformations

use crate::{types::*, error::*};

impl<const N: usize> AtlasPoint<N> {
    /// Create a new point with all coordinates set to zero
    pub fn zero() -> Self {
        Self {
            coords: [0.0; N],
        }
    }
    
    /// Create a new point from coordinates
    pub fn new(coords: [Float; N]) -> Self {
        Self { coords }
    }
    
    /// Get the squared Euclidean distance to another point
    pub fn distance_squared(&self, other: &Self) -> Float {
        self.coords
            .iter()
            .zip(other.coords.iter())
            .map(|(a, b)| (a - b) * (a - b))
            .sum()
    }
    
    /// Get the Euclidean distance to another point
    pub fn distance(&self, other: &Self) -> Float {
        self.distance_squared(other).sqrt()
    }
}

impl<const N: usize> AtlasVector<N> {
    /// Create a new zero vector
    pub fn zero() -> Self {
        Self {
            components: [0.0; N],
        }
    }
    
    /// Create a new vector from components
    pub fn new(components: [Float; N]) -> Self {
        Self { components }
    }
    
    /// Compute dot product with another vector
    pub fn dot(&self, other: &Self) -> Float {
        self.components
            .iter()
            .zip(other.components.iter())
            .map(|(a, b)| a * b)
            .sum()
    }
    
    /// Compute the squared magnitude of the vector
    pub fn magnitude_squared(&self) -> Float {
        self.dot(self)
    }
    
    /// Compute the magnitude of the vector
    pub fn magnitude(&self) -> Float {
        self.magnitude_squared().sqrt()
    }
    
    /// Normalize the vector to unit length
    pub fn normalize(&mut self) -> AtlasResult<()> {
        let mag = self.magnitude();
        if mag == 0.0 {
            return Err(AtlasError::NumericalError("cannot normalize zero vector"));
        }
        
        for component in &mut self.components {
            *component /= mag;
        }
        
        Ok(())
    }
    
    /// Get a normalized copy of the vector
    pub fn normalized(&self) -> AtlasResult<Self> {
        let mut result = *self;
        result.normalize()?;
        Ok(result)
    }
}

impl<const M: usize, const N: usize> TransformMatrix<M, N> {
    /// Create a new matrix with all elements set to zero
    pub fn zero() -> Self {
        Self {
            elements: [[0.0; N]; M],
        }
    }
    
    /// Create an identity matrix (only valid for square matrices where M == N)
    pub fn identity() -> Self {
        let mut matrix = Self::zero();
        let size = core::cmp::min(M, N);
        for i in 0..size {
            matrix.elements[i][i] = 1.0;
        }
        matrix
    }
    
    /// Multiply this matrix by a vector
    pub fn multiply_vector(&self, vector: &AtlasVector<N>) -> AtlasVector<M> {
        let mut result = AtlasVector::zero();
        
        for i in 0..M {
            for j in 0..N {
                result.components[i] += self.elements[i][j] * vector.components[j];
            }
        }
        
        result
    }
    
    /// Transform a point using this matrix
    pub fn transform_point(&self, point: &AtlasPoint<N>) -> AtlasPoint<M> {
        let vector = AtlasVector::new(point.coords);
        let transformed = self.multiply_vector(&vector);
        AtlasPoint::new(transformed.components)
    }
    
    /// Get the transpose of this matrix
    pub fn transpose(&self) -> TransformMatrix<N, M> {
        let mut result = TransformMatrix::zero();
        
        for i in 0..M {
            for j in 0..N {
                result.elements[j][i] = self.elements[i][j];
            }
        }
        
        result
    }
}

impl<const M: usize, const N: usize> TransformMatrix<M, N> {
    /// Multiply this matrix by another matrix with compatible dimensions
    pub fn multiply_matrix<const P: usize>(&self, other: &TransformMatrix<N, P>) -> TransformMatrix<M, P> {
        let mut result = TransformMatrix::zero();
        
        for i in 0..M {
            for j in 0..P {
                for k in 0..N {
                    result.elements[i][j] += self.elements[i][k] * other.elements[k][j];
                }
            }
        }
        
        result
    }
}

/// Utility functions for common linear algebra operations
pub mod util {
    use super::*;
    
    /// Compute the cross product of two 3D vectors
    pub fn cross_product_3d(a: &AtlasVector<3>, b: &AtlasVector<3>) -> AtlasVector<3> {
        AtlasVector::new([
            a.components[1] * b.components[2] - a.components[2] * b.components[1],
            a.components[2] * b.components[0] - a.components[0] * b.components[2],
            a.components[0] * b.components[1] - a.components[1] * b.components[0],
        ])
    }
    
    /// Compute the angle between two vectors in radians
    pub fn angle_between<const N: usize>(a: &AtlasVector<N>, b: &AtlasVector<N>) -> AtlasResult<Float> {
        let dot_product = a.dot(b);
        let magnitude_product = a.magnitude() * b.magnitude();
        
        if magnitude_product == 0.0 {
            return Err(AtlasError::NumericalError("cannot compute angle with zero vector"));
        }
        
        let cos_angle = (dot_product / magnitude_product).clamp(-1.0, 1.0);
        Ok(cos_angle.acos())
    }
}