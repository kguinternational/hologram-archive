//! Core type definitions for Atlas Manifold operations
//!
//! This module contains fundamental data structures used throughout the manifold system,
//! including FFI-safe representations that use unsafe traits for byte-level compatibility.

#![allow(clippy::module_name_repetitions)] // Allow repetitive names for FFI clarity

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

// SAFETY: AtlasPoint is #[repr(C)] with only Float (f64) elements, which are Pod-safe
unsafe impl<const N: usize> Pod for AtlasPoint<N> {}

// SAFETY: AtlasPoint can be safely zero-initialized as all f64 values can be zero
unsafe impl<const N: usize> Zeroable for AtlasPoint<N> {}

/// Transformation matrix for manifold operations
#[derive(Debug, Clone, Copy, PartialEq)]
#[repr(C)]
pub struct TransformMatrix<const M: usize, const N: usize> {
    /// Matrix elements in row-major order
    pub elements: [[Float; N]; M],
}

// SAFETY: TransformMatrix is #[repr(C)] with only Float (f64) arrays, which are Pod-safe
unsafe impl<const M: usize, const N: usize> Pod for TransformMatrix<M, N> {}

// SAFETY: TransformMatrix can be safely zero-initialized as f64 arrays can be zero
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

// SAFETY: ManifoldDescriptor is #[repr(C)] with only primitive types (u32, f64 arrays)
unsafe impl Pod for ManifoldDescriptor {}

// SAFETY: ManifoldDescriptor fields can be safely zero-initialized
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

// SAFETY: ShardId is #[repr(C)] with only primitive integer types (u64, u32)
unsafe impl Pod for ShardId {}

// SAFETY: ShardId can be safely zero-initialized as integer types can be zero
unsafe impl Zeroable for ShardId {}

/// Atlas vector in N-dimensional space
#[derive(Debug, Clone, Copy, PartialEq)]
#[repr(C)]
pub struct AtlasVector<const N: usize> {
    /// Vector components
    pub components: [Float; N],
}

// SAFETY: AtlasVector is #[repr(C)] with only Float (f64) array components, which are Pod-safe
unsafe impl<const N: usize> Pod for AtlasVector<N> {}

// SAFETY: AtlasVector can be safely zero-initialized as f64 arrays can be zero
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
