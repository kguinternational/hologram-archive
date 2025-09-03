//! Core type definitions for Atlas Manifold operations

use bytemuck::{Pod, Zeroable};

/// Fundamental floating-point type used throughout the manifold
pub type Float = f64;

/// Integer type for indexing and counts
pub type Index = usize;

/// Dimension type for manifold dimensions
pub type Dimension = u32;

/// Atlas point in N-dimensional space
#[derive(Debug, Clone, Copy, PartialEq)]
#[repr(C)]
pub struct AtlasPoint<const N: usize> {
    /// Coordinates in N-dimensional space
    pub coords: [Float; N],
}

unsafe impl<const N: usize> Pod for AtlasPoint<N> {}
unsafe impl<const N: usize> Zeroable for AtlasPoint<N> {}

/// Transformation matrix for manifold operations
#[derive(Debug, Clone, Copy, PartialEq)]
#[repr(C)]
pub struct TransformMatrix<const M: usize, const N: usize> {
    /// Matrix elements in row-major order
    pub elements: [[Float; N]; M],
}

unsafe impl<const M: usize, const N: usize> Pod for TransformMatrix<M, N> {}
unsafe impl<const M: usize, const N: usize> Zeroable for TransformMatrix<M, N> {}

/// Manifold descriptor containing geometric properties
#[derive(Debug, Clone, Copy)]
#[repr(C)]
pub struct ManifoldDescriptor {
    /// Intrinsic dimension of the manifold
    pub intrinsic_dim: Dimension,
    /// Embedding dimension
    pub embedding_dim: Dimension,
    /// Curvature parameters
    pub curvature: [Float; 4],
    /// Metric tensor determinant
    pub metric_det: Float,
}

unsafe impl Pod for ManifoldDescriptor {}
unsafe impl Zeroable for ManifoldDescriptor {}

/// Shard identifier for distributed operations
#[derive(Debug, Clone, Copy, PartialEq, Eq, Hash)]
#[repr(C)]
pub struct ShardId {
    /// Primary shard identifier
    pub primary: u64,
    /// Secondary identifier for sub-sharding
    pub secondary: u32,
}

unsafe impl Pod for ShardId {}
unsafe impl Zeroable for ShardId {}

/// Atlas vector in N-dimensional space
#[derive(Debug, Clone, Copy, PartialEq)]
#[repr(C)]
pub struct AtlasVector<const N: usize> {
    /// Vector components
    pub components: [Float; N],
}

unsafe impl<const N: usize> Pod for AtlasVector<N> {}
unsafe impl<const N: usize> Zeroable for AtlasVector<N> {}

/// Common manifold operations result
pub type ManifoldResult<T> = Result<T, crate::error::AtlasError>;

/// Constants for common manifold dimensions
pub mod dimensions {
    /// 2D manifold (surface)
    pub const DIM_2D: usize = 2;
    /// 3D manifold (volume)
    pub const DIM_3D: usize = 3;
    /// 4D spacetime manifold
    pub const DIM_4D: usize = 4;
    /// High-dimensional manifold (common choice)
    pub const DIM_HD: usize = 64;
}