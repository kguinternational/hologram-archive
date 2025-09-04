//! Type-Length-Value serialization for Atlas Manifold data structures

#![allow(clippy::needless_range_loop)] // Index loops needed for tensor operations

use crate::{error::*, types::*};

/// Type alias for TLV decoder result
type TlvEntry<'a> = (TlvTag, &'a [u8]);
type TlvResult<'a> = AtlasResult<Option<TlvEntry<'a>>>;
use bytemuck::{bytes_of, cast_slice};

#[cfg(feature = "std")]
use std::vec::Vec;

#[cfg(not(feature = "std"))]
use alloc::vec::Vec;

/// TLV tag types for different data structures
#[repr(u16)]
#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub enum TlvTag {
    /// Atlas point data
    AtlasPoint = 0x1001,
    /// Atlas vector data
    AtlasVector = 0x1002,
    /// Transformation matrix
    TransformMatrix = 0x1003,
    /// Manifold descriptor
    ManifoldDescriptor = 0x1004,
    /// Shard identifier
    ShardId = 0x1005,
    /// Shard metadata
    ShardMetadata = 0x1006,
    /// Custom user-defined type
    Custom = 0x2000,
    /// Array of elements
    Array = 0x3000,
    /// End of data marker
    End = 0xFFFF,
}

/// TLV header structure
#[repr(C)]
#[derive(Debug, Clone, Copy)]
pub struct TlvHeader {
    /// Type tag
    pub tag: u16,
    /// Length of data in bytes
    pub length: u32,
}

// SAFETY: TlvHeader is #[repr(C)] with only primitive integer types (u16, u32)
// SAFETY: Type contains only primitive data with no pointers
unsafe impl bytemuck::Pod for TlvHeader {}

// SAFETY: TlvHeader can be safely zero-initialized as integer fields can be zero
// SAFETY: Type contains only primitive data with no pointers
unsafe impl bytemuck::Zeroable for TlvHeader {}

/// TLV encoder for serializing Atlas data structures
///
/// This encoder is thread-safe when used in single-threaded contexts
/// or when each thread has its own encoder instance.
pub struct TlvEncoder {
    buffer: Vec<u8>,
}

// SAFETY: TlvEncoder contains only a Vec<u8> which is Send + Sync
// SAFETY: Type contains only primitive data with no pointers
unsafe impl Send for TlvEncoder {}
// SAFETY: Type contains only primitive data with no pointers
unsafe impl Sync for TlvEncoder {}

impl TlvEncoder {
    /// Create a new TLV encoder
    pub fn new() -> Self {
        Self { buffer: Vec::new() }
    }

    /// Create a new TLV encoder with initial capacity
    pub fn with_capacity(capacity: usize) -> Self {
        Self {
            buffer: Vec::with_capacity(capacity),
        }
    }

    /// Write a TLV entry to the buffer
    pub fn write_tlv(&mut self, tag: TlvTag, data: &[u8]) -> AtlasResult<()> {
        let tag_bytes = (tag as u16).to_le_bytes();
        let length = u32::try_from(data.len())
            .map_err(|_| AtlasError::SerializationError("data too large"))?;
        let length_bytes = length.to_le_bytes();

        // Write header as individual fields to avoid padding issues
        self.buffer.extend_from_slice(&tag_bytes);
        self.buffer.extend_from_slice(&length_bytes);
        self.buffer.extend_from_slice(data);
        Ok(())
    }

    /// Write an Atlas point
    pub fn write_point<const N: usize>(&mut self, point: &AtlasPoint<N>) -> AtlasResult<()> {
        // Write dimension first
        let dim_bytes = u32::try_from(N)
            .map_err(|_| AtlasError::SerializationError("dimension too large"))?
            .to_le_bytes();
        self.write_tlv(TlvTag::Custom, &dim_bytes)?;
        // Write point data manually to ensure consistent format
        let mut data = Vec::with_capacity(N * 8);
        for coord in &point.coords {
            data.extend_from_slice(&coord.to_le_bytes());
        }
        self.write_tlv(TlvTag::AtlasPoint, &data)
    }

    /// Write an Atlas vector
    pub fn write_vector<const N: usize>(&mut self, vector: &AtlasVector<N>) -> AtlasResult<()> {
        // Write dimension first
        let dim_bytes = u32::try_from(N)
            .map_err(|_| AtlasError::SerializationError("dimension too large"))?
            .to_le_bytes();
        self.write_tlv(TlvTag::Custom, &dim_bytes)?;
        // Write vector data manually to ensure consistent format
        let mut data = Vec::with_capacity(N * 8);
        for component in &vector.components {
            data.extend_from_slice(&component.to_le_bytes());
        }
        self.write_tlv(TlvTag::AtlasVector, &data)
    }

    /// Write a transformation matrix
    pub fn write_matrix<const M: usize, const N: usize>(
        &mut self,
        matrix: &TransformMatrix<M, N>,
    ) -> AtlasResult<()> {
        // Write dimensions
        let m_bytes = u32::try_from(M)
            .map_err(|_| AtlasError::SerializationError("dimension too large"))?
            .to_le_bytes();
        let n_bytes = u32::try_from(N)
            .map_err(|_| AtlasError::SerializationError("dimension too large"))?
            .to_le_bytes();
        self.write_tlv(TlvTag::Custom, &m_bytes)?;
        self.write_tlv(TlvTag::Custom, &n_bytes)?;
        // Write matrix data
        self.write_tlv(TlvTag::TransformMatrix, bytes_of(matrix))
    }

    /// Write a manifold descriptor
    pub fn write_manifold_descriptor(&mut self, desc: &ManifoldDescriptor) -> AtlasResult<()> {
        // Manually serialize to ensure consistent layout across platforms
        let mut data = Vec::with_capacity(48); // 2*u32 + 4*f64 + f64 = 8 + 32 + 8 = 48 bytes
        data.extend_from_slice(&desc.intrinsic_dim.to_le_bytes());
        data.extend_from_slice(&desc.embedding_dim.to_le_bytes());
        for curvature in &desc.curvature {
            data.extend_from_slice(&curvature.to_le_bytes());
        }
        data.extend_from_slice(&desc.metric_det.to_le_bytes());
        self.write_tlv(TlvTag::ManifoldDescriptor, &data)
    }

    /// Write a shard ID
    pub fn write_shard_id(&mut self, shard_id: &ShardId) -> AtlasResult<()> {
        // Manually serialize to avoid padding issues
        let mut data = Vec::with_capacity(12); // u64 + u32 without padding
        data.extend_from_slice(&shard_id.primary.to_le_bytes());
        data.extend_from_slice(&shard_id.secondary.to_le_bytes());
        self.write_tlv(TlvTag::ShardId, &data)
    }

    /// Write an array of items
    pub fn write_array<T: bytemuck::Pod>(&mut self, items: &[T]) -> AtlasResult<()> {
        let data = cast_slice(items);
        self.write_tlv(TlvTag::Array, data)
    }

    /// Write end marker
    pub fn write_end(&mut self) -> AtlasResult<()> {
        self.write_tlv(TlvTag::End, &[])
    }

    /// Get the encoded buffer
    pub fn finish(mut self) -> Vec<u8> {
        let _ = self.write_end();
        self.buffer
    }

    /// Get the current buffer size
    pub fn len(&self) -> usize {
        self.buffer.len()
    }

    /// Check if the buffer is empty
    pub fn is_empty(&self) -> bool {
        self.buffer.is_empty()
    }
}

impl Default for TlvEncoder {
    fn default() -> Self {
        Self::new()
    }
}

/// TLV decoder for deserializing Atlas data structures
///
/// This decoder is thread-safe when used in single-threaded contexts
/// or when each thread has its own decoder instance.
pub struct TlvDecoder<'a> {
    data: &'a [u8],
    position: usize,
}

// SAFETY: TlvDecoder contains only a reference and usize which are Send + Sync
// SAFETY: Type contains only primitive data with no pointers
unsafe impl<'a> Send for TlvDecoder<'a> {}
// SAFETY: Type contains only primitive data with no pointers
unsafe impl<'a> Sync for TlvDecoder<'a> {}

impl<'a> TlvDecoder<'a> {
    /// Create a new TLV decoder
    pub fn new(data: &'a [u8]) -> Self {
        Self { data, position: 0 }
    }

    /// Read the next TLV entry
    pub fn read_next(&mut self) -> TlvResult<'a> {
        if self.position >= self.data.len() {
            return Ok(None);
        }

        // Read header fields individually to avoid padding issues
        if self.position + 6 > self.data.len() {
            // u16 + u32 = 6 bytes
            return Err(AtlasError::SerializationError("incomplete header"));
        }

        let tag_bytes = [self.data[self.position], self.data[self.position + 1]];
        let length_bytes = [
            self.data[self.position + 2],
            self.data[self.position + 3],
            self.data[self.position + 4],
            self.data[self.position + 5],
        ];

        let tag_value = u16::from_le_bytes(tag_bytes);
        let length = u32::from_le_bytes(length_bytes);

        self.position += 6;

        // Convert tag
        let tag = match tag_value {
            0x1001 => TlvTag::AtlasPoint,
            0x1002 => TlvTag::AtlasVector,
            0x1003 => TlvTag::TransformMatrix,
            0x1004 => TlvTag::ManifoldDescriptor,
            0x1005 => TlvTag::ShardId,
            0x1006 => TlvTag::ShardMetadata,
            0x2000 => TlvTag::Custom,
            0x3000 => TlvTag::Array,
            0xFFFF => TlvTag::End,
            _ => return Err(AtlasError::SerializationError("unknown tag")),
        };

        // Read data
        let data_len = length as usize;
        if self.position + data_len > self.data.len() {
            return Err(AtlasError::SerializationError("incomplete data"));
        }

        let data = &self.data[self.position..self.position + data_len];
        self.position += data_len;

        Ok(Some((tag, data)))
    }

    /// Read an Atlas point (requires dimension to be read first)
    pub fn read_point<const N: usize>(&mut self) -> AtlasResult<AtlasPoint<N>> {
        if let Some((tag, data)) = self.read_next()? {
            if tag == TlvTag::AtlasPoint {
                // Manually deserialize to ensure consistent format
                if data.len() != N * 8 {
                    return Err(AtlasError::SerializationError("invalid point data length"));
                }
                let mut coords = [0.0f64; N];
                for i in 0..N {
                    let start = i * 8;
                    coords[i] = f64::from_le_bytes([
                        data[start],
                        data[start + 1],
                        data[start + 2],
                        data[start + 3],
                        data[start + 4],
                        data[start + 5],
                        data[start + 6],
                        data[start + 7],
                    ]);
                }
                return Ok(AtlasPoint { coords });
            }
        }
        Err(AtlasError::SerializationError("expected atlas point"))
    }

    /// Read an Atlas vector (requires dimension to be read first)
    pub fn read_vector<const N: usize>(&mut self) -> AtlasResult<AtlasVector<N>> {
        if let Some((tag, data)) = self.read_next()? {
            if tag == TlvTag::AtlasVector {
                // Manually deserialize to ensure consistent format
                if data.len() != N * 8 {
                    return Err(AtlasError::SerializationError("invalid vector data length"));
                }
                let mut components = [0.0f64; N];
                for i in 0..N {
                    let start = i * 8;
                    components[i] = f64::from_le_bytes([
                        data[start],
                        data[start + 1],
                        data[start + 2],
                        data[start + 3],
                        data[start + 4],
                        data[start + 5],
                        data[start + 6],
                        data[start + 7],
                    ]);
                }
                return Ok(AtlasVector { components });
            }
        }
        Err(AtlasError::SerializationError("expected atlas vector"))
    }

    /// Read a manifold descriptor
    pub fn read_manifold_descriptor(&mut self) -> AtlasResult<ManifoldDescriptor> {
        if let Some((tag, data)) = self.read_next()? {
            if tag == TlvTag::ManifoldDescriptor {
                // Manually deserialize to ensure consistent layout
                if data.len() != 48 {
                    return Err(AtlasError::SerializationError(
                        "invalid manifold descriptor length",
                    ));
                }

                let intrinsic_dim = u32::from_le_bytes([data[0], data[1], data[2], data[3]]);
                let embedding_dim = u32::from_le_bytes([data[4], data[5], data[6], data[7]]);

                let mut curvature = [0.0f64; 4];
                for i in 0..4 {
                    let start = 8 + i * 8;
                    curvature[i] = f64::from_le_bytes([
                        data[start],
                        data[start + 1],
                        data[start + 2],
                        data[start + 3],
                        data[start + 4],
                        data[start + 5],
                        data[start + 6],
                        data[start + 7],
                    ]);
                }

                let metric_det = f64::from_le_bytes([
                    data[40], data[41], data[42], data[43], data[44], data[45], data[46], data[47],
                ]);

                return Ok(ManifoldDescriptor {
                    intrinsic_dim,
                    embedding_dim,
                    curvature,
                    metric_det,
                });
            }
        }
        Err(AtlasError::SerializationError(
            "expected manifold descriptor",
        ))
    }

    /// Read a shard ID
    pub fn read_shard_id(&mut self) -> AtlasResult<ShardId> {
        if let Some((tag, data)) = self.read_next()? {
            if tag == TlvTag::ShardId {
                // Manually deserialize to avoid padding issues
                if data.len() != 12 {
                    return Err(AtlasError::SerializationError("invalid shard id length"));
                }
                let primary = u64::from_le_bytes([
                    data[0], data[1], data[2], data[3], data[4], data[5], data[6], data[7],
                ]);
                let secondary = u32::from_le_bytes([data[8], data[9], data[10], data[11]]);
                return Ok(ShardId { primary, secondary });
            }
        }
        Err(AtlasError::SerializationError("expected shard id"))
    }

    /// Read a custom u32 value
    pub fn read_u32(&mut self) -> AtlasResult<u32> {
        if let Some((tag, data)) = self.read_next()? {
            if tag == TlvTag::Custom && data.len() == 4 {
                return Ok(u32::from_le_bytes([data[0], data[1], data[2], data[3]]));
            }
        }
        Err(AtlasError::SerializationError("expected u32 custom data"))
    }

    /// Check if we've reached the end
    pub fn is_at_end(&self) -> bool {
        self.position >= self.data.len()
    }

    /// Get current position
    pub fn position(&self) -> usize {
        self.position
    }
}
