//! Type-Length-Value serialization for Atlas Manifold data structures

use crate::{types::*, error::*};
use bytemuck::{bytes_of, try_from_bytes, cast_slice};

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
#[repr(C, packed)]
#[derive(Debug, Clone, Copy)]
pub struct TlvHeader {
    /// Type tag
    pub tag: u16,
    /// Length of data in bytes
    pub length: u32,
}

unsafe impl bytemuck::Pod for TlvHeader {}
unsafe impl bytemuck::Zeroable for TlvHeader {}

/// TLV encoder for serializing Atlas data structures
pub struct TlvEncoder {
    buffer: Vec<u8>,
}

impl TlvEncoder {
    /// Create a new TLV encoder
    pub fn new() -> Self {
        Self {
            buffer: Vec::new(),
        }
    }
    
    /// Create a new TLV encoder with initial capacity
    pub fn with_capacity(capacity: usize) -> Self {
        Self {
            buffer: Vec::with_capacity(capacity),
        }
    }
    
    /// Write a TLV entry to the buffer
    pub fn write_tlv(&mut self, tag: TlvTag, data: &[u8]) -> AtlasResult<()> {
        let header = TlvHeader {
            tag: tag as u16,
            length: data.len() as u32,
        };
        
        self.buffer.extend_from_slice(bytes_of(&header));
        self.buffer.extend_from_slice(data);
        Ok(())
    }
    
    /// Write an Atlas point
    pub fn write_point<const N: usize>(&mut self, point: &AtlasPoint<N>) -> AtlasResult<()> {
        // Write dimension first
        self.write_tlv(TlvTag::Custom, &(N as u32).to_le_bytes())?;
        // Write point data
        self.write_tlv(TlvTag::AtlasPoint, bytes_of(point))
    }
    
    /// Write an Atlas vector
    pub fn write_vector<const N: usize>(&mut self, vector: &AtlasVector<N>) -> AtlasResult<()> {
        // Write dimension first
        self.write_tlv(TlvTag::Custom, &(N as u32).to_le_bytes())?;
        // Write vector data
        self.write_tlv(TlvTag::AtlasVector, bytes_of(vector))
    }
    
    /// Write a transformation matrix
    pub fn write_matrix<const M: usize, const N: usize>(
        &mut self, 
        matrix: &TransformMatrix<M, N>
    ) -> AtlasResult<()> {
        // Write dimensions
        self.write_tlv(TlvTag::Custom, &(M as u32).to_le_bytes())?;
        self.write_tlv(TlvTag::Custom, &(N as u32).to_le_bytes())?;
        // Write matrix data
        self.write_tlv(TlvTag::TransformMatrix, bytes_of(matrix))
    }
    
    /// Write a manifold descriptor
    pub fn write_manifold_descriptor(&mut self, desc: &ManifoldDescriptor) -> AtlasResult<()> {
        self.write_tlv(TlvTag::ManifoldDescriptor, bytes_of(desc))
    }
    
    /// Write a shard ID
    pub fn write_shard_id(&mut self, shard_id: &ShardId) -> AtlasResult<()> {
        self.write_tlv(TlvTag::ShardId, bytes_of(shard_id))
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
pub struct TlvDecoder<'a> {
    data: &'a [u8],
    position: usize,
}

impl<'a> TlvDecoder<'a> {
    /// Create a new TLV decoder
    pub fn new(data: &'a [u8]) -> Self {
        Self {
            data,
            position: 0,
        }
    }
    
    /// Read the next TLV entry
    pub fn read_next(&mut self) -> AtlasResult<Option<(TlvTag, &'a [u8])>> {
        if self.position >= self.data.len() {
            return Ok(None);
        }
        
        // Read header
        let header_size = core::mem::size_of::<TlvHeader>();
        if self.position + header_size > self.data.len() {
            return Err(AtlasError::SerializationError("incomplete header"));
        }
        
        let header_bytes = &self.data[self.position..self.position + header_size];
        let header: &TlvHeader = try_from_bytes(header_bytes)
            .map_err(|_| AtlasError::SerializationError("invalid header"))?;
        
        self.position += header_size;
        
        // Convert tag
        let tag = match header.tag {
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
        let data_len = header.length as usize;
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
                let point: &AtlasPoint<N> = try_from_bytes(data)
                    .map_err(|_| AtlasError::SerializationError("invalid point data"))?;
                return Ok(*point);
            }
        }
        Err(AtlasError::SerializationError("expected atlas point"))
    }
    
    /// Read an Atlas vector (requires dimension to be read first)
    pub fn read_vector<const N: usize>(&mut self) -> AtlasResult<AtlasVector<N>> {
        if let Some((tag, data)) = self.read_next()? {
            if tag == TlvTag::AtlasVector {
                let vector: &AtlasVector<N> = try_from_bytes(data)
                    .map_err(|_| AtlasError::SerializationError("invalid vector data"))?;
                return Ok(*vector);
            }
        }
        Err(AtlasError::SerializationError("expected atlas vector"))
    }
    
    /// Read a manifold descriptor
    pub fn read_manifold_descriptor(&mut self) -> AtlasResult<ManifoldDescriptor> {
        if let Some((tag, data)) = self.read_next()? {
            if tag == TlvTag::ManifoldDescriptor {
                let desc: &ManifoldDescriptor = try_from_bytes(data)
                    .map_err(|_| AtlasError::SerializationError("invalid manifold descriptor"))?;
                return Ok(*desc);
            }
        }
        Err(AtlasError::SerializationError("expected manifold descriptor"))
    }
    
    /// Read a shard ID
    pub fn read_shard_id(&mut self) -> AtlasResult<ShardId> {
        if let Some((tag, data)) = self.read_next()? {
            if tag == TlvTag::ShardId {
                let shard_id: &ShardId = try_from_bytes(data)
                    .map_err(|_| AtlasError::SerializationError("invalid shard id"))?;
                return Ok(*shard_id);
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