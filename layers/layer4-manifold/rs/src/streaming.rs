//! Streaming mode for large domain processing in Layer 4
//!
//! This module implements streaming capabilities for processing large manifold domains
//! that don't fit in memory, enabling chunk-by-chunk processing with conservation guarantees.

#![allow(clippy::module_name_repetitions)]

use crate::error::*;
use crate::projection::*;

#[cfg(feature = "std")]
use std::vec::Vec;

#[cfg(not(feature = "std"))]
use alloc::vec::Vec;

#[cfg(feature = "streaming")]
use memmap2::MmapOptions;
#[cfg(feature = "streaming")]
use std::fs::File;
#[cfg(feature = "streaming")]
use std::path::Path;

/// Default chunk size for streaming processing (16 MB)
const DEFAULT_CHUNK_SIZE: usize = 16 * 1024 * 1024;

/// Minimum chunk size to maintain efficiency (64 KB)
const MIN_CHUNK_SIZE: usize = 64 * 1024;

/// Maximum chunk size to avoid memory pressure (256 MB)
const MAX_CHUNK_SIZE: usize = 256 * 1024 * 1024;

/// Streaming configuration for large domain processing
#[derive(Debug, Clone)]
pub struct StreamingConfig {
    /// Size of each processing chunk in bytes
    pub chunk_size: usize,
    /// Number of chunks to process in parallel (when parallel feature enabled)
    pub parallel_chunks: usize,
    /// Overlap size between chunks for continuity
    pub chunk_overlap: usize,
    /// Enable compression for intermediate results
    pub enable_compression: bool,
    /// Buffer size for I/O operations
    pub io_buffer_size: usize,
}

impl Default for StreamingConfig {
    fn default() -> Self {
        Self {
            chunk_size: DEFAULT_CHUNK_SIZE,
            parallel_chunks: 4,
            chunk_overlap: 4096, // One page overlap
            enable_compression: false,
            io_buffer_size: 64 * 1024,
        }
    }
}

/// Streaming chunk for processing large domains
#[derive(Debug)]
pub struct StreamingChunk {
    /// Chunk identifier
    pub id: u32,
    /// Chunk data
    pub data: Vec<u8>,
    /// Starting offset in the original data
    pub start_offset: u64,
    /// Conservation sum for this chunk
    pub conservation_sum: u64,
    /// Overlap region with next chunk
    pub overlap_region: Option<Vec<u8>>,
}

/// Context for streaming large domain processing
#[derive(Debug)]
pub struct StreamingContext {
    /// Configuration for streaming
    pub config: StreamingConfig,
    /// Total size of the domain being processed
    pub total_size: u64,
    /// Current processing position
    pub current_position: u64,
    /// Number of chunks processed
    pub processed_chunks: u32,
    /// Accumulated conservation sum
    pub total_conservation_sum: u64,
    /// Intermediate results storage
    pub intermediate_results: Vec<AtlasProjectionHandle>,
    /// Memory-mapped file handle (when streaming from file)
    #[cfg(feature = "streaming")]
    pub mmap: Option<memmap2::Mmap>,
}

impl StreamingConfig {
    /// Create a new streaming configuration with validation
    pub fn new(chunk_size: usize, parallel_chunks: usize) -> AtlasResult<Self> {
        if chunk_size < MIN_CHUNK_SIZE {
            return Err(AtlasError::InvalidInput("chunk size too small"));
        }
        
        if chunk_size > MAX_CHUNK_SIZE {
            return Err(AtlasError::InvalidInput("chunk size too large"));
        }
        
        if parallel_chunks == 0 {
            return Err(AtlasError::InvalidInput("parallel chunks must be > 0"));
        }

        Ok(Self {
            chunk_size,
            parallel_chunks,
            chunk_overlap: 4096,
            enable_compression: false,
            io_buffer_size: 64 * 1024,
        })
    }

    /// Enable compression for intermediate results
    pub fn with_compression(mut self, enable: bool) -> Self {
        self.enable_compression = enable;
        self
    }

    /// Set the chunk overlap size
    pub fn with_overlap(mut self, overlap: usize) -> Self {
        self.chunk_overlap = overlap;
        self
    }

    /// Set the I/O buffer size
    pub fn with_io_buffer_size(mut self, size: usize) -> Self {
        self.io_buffer_size = size;
        self
    }
}

impl StreamingChunk {
    /// Create a new streaming chunk
    pub fn new(id: u32, data: Vec<u8>, start_offset: u64) -> Self {
        let conservation_sum = data.iter().map(|&b| u64::from(b)).sum();
        
        Self {
            id,
            data,
            start_offset,
            conservation_sum,
            overlap_region: None,
        }
    }

    /// Set overlap region for this chunk
    pub fn set_overlap(&mut self, overlap_data: Vec<u8>) {
        self.overlap_region = Some(overlap_data);
    }

    /// Get chunk size including overlap
    pub fn total_size(&self) -> usize {
        self.data.len() + self.overlap_region.as_ref().map_or(0, |o| o.len())
    }

    /// Verify chunk conservation
    pub fn verify_conservation(&self) -> bool {
        let calculated_sum: u64 = self.data.iter().map(|&b| u64::from(b)).sum();
        calculated_sum == self.conservation_sum
    }
}

impl StreamingContext {
    /// Create a new streaming context for in-memory data
    pub fn new(config: StreamingConfig, total_size: u64) -> Self {
        Self {
            config,
            total_size,
            current_position: 0,
            processed_chunks: 0,
            total_conservation_sum: 0,
            intermediate_results: Vec::new(),
            #[cfg(feature = "streaming")]
            mmap: None,
        }
    }

    /// Create a streaming context for file-based processing
    #[cfg(feature = "streaming")]
    pub fn from_file<P: AsRef<Path>>(config: StreamingConfig, file_path: P) -> AtlasResult<Self> {
        let file = File::open(file_path.as_ref())
            .map_err(|_| AtlasError::InvalidInput("failed to open file"))?;
        
        let mmap = unsafe {
            MmapOptions::new()
                .map(&file)
                .map_err(|_| AtlasError::InvalidInput("failed to memory map file"))?
        };
        
        let total_size = mmap.len() as u64;
        
        Ok(Self {
            config,
            total_size,
            current_position: 0,
            processed_chunks: 0,
            total_conservation_sum: 0,
            intermediate_results: Vec::new(),
            mmap: Some(mmap),
        })
    }

    /// Get the next chunk for processing
    pub fn next_chunk(&mut self, source_data: Option<&[u8]>) -> AtlasResult<Option<StreamingChunk>> {
        if self.current_position >= self.total_size {
            return Ok(None);
        }

        let chunk_end = (self.current_position + self.config.chunk_size as u64).min(self.total_size);
        let _chunk_size = chunk_end - self.current_position;

        let chunk_data = if let Some(data) = source_data {
            // In-memory processing
            let start = self.current_position as usize;
            let end = chunk_end as usize;
            data[start..end].to_vec()
        } else {
            #[cfg(feature = "streaming")]
            {
                if let Some(ref mmap) = self.mmap {
                    let start = self.current_position as usize;
                    let end = chunk_end as usize;
                    mmap[start..end].to_vec()
                } else {
                    return Err(AtlasError::InvalidInput("no data source available"));
                }
            }
            #[cfg(not(feature = "streaming"))]
            {
                return Err(AtlasError::InvalidInput("streaming feature not enabled"));
            }
        };

        let mut chunk = StreamingChunk::new(self.processed_chunks, chunk_data, self.current_position);

        // Add overlap region if not the last chunk
        if chunk_end < self.total_size && self.config.chunk_overlap > 0 {
            let overlap_end = (chunk_end + self.config.chunk_overlap as u64).min(self.total_size);
            let _overlap_size = overlap_end - chunk_end;

            let overlap_data = if let Some(data) = source_data {
                let start = chunk_end as usize;
                let end = overlap_end as usize;
                data[start..end].to_vec()
            } else {
                #[cfg(feature = "streaming")]
                {
                    if let Some(ref mmap) = self.mmap {
                        let start = chunk_end as usize;
                        let end = overlap_end as usize;
                        mmap[start..end].to_vec()
                    } else {
                        Vec::new()
                    }
                }
                #[cfg(not(feature = "streaming"))]
                {
                    Vec::new()
                }
            };

            chunk.set_overlap(overlap_data);
        }

        self.current_position = chunk_end;
        self.processed_chunks += 1;
        self.total_conservation_sum = self.total_conservation_sum.wrapping_add(chunk.conservation_sum);

        Ok(Some(chunk))
    }

    /// Process a chunk and create a projection
    pub fn process_chunk(&mut self, chunk: StreamingChunk, projection_type: ProjectionType) -> AtlasResult<AtlasProjectionHandle> {
        let projection = match projection_type {
            ProjectionType::Linear => AtlasProjection::new_linear(&chunk.data)?,
            ProjectionType::R96Fourier => AtlasProjection::new_r96_fourier(&chunk.data)?,
        };

        let handle = AtlasProjectionHandle::new(projection);
        self.intermediate_results.push(handle);

        // Return the last added handle (clone the inner pointer)
        // SAFETY: We just added this handle, so we know it's valid
        let last_handle = unsafe {
            let inner = self.intermediate_results.last().unwrap().inner;
            AtlasProjectionHandle { inner }
        };

        Ok(last_handle)
    }

    /// Process all chunks in streaming mode
    pub fn process_streaming(&mut self, source_data: Option<&[u8]>, projection_type: ProjectionType) -> AtlasResult<Vec<AtlasProjectionHandle>> {
        let mut results = Vec::new();

        while let Some(chunk) = self.next_chunk(source_data)? {
            let handle = self.process_chunk(chunk, projection_type)?;
            results.push(handle);
        }

        Ok(results)
    }

    /// Get processing progress (0.0 to 1.0)
    pub fn progress(&self) -> f64 {
        if self.total_size == 0 {
            1.0
        } else {
            self.current_position as f64 / self.total_size as f64
        }
    }

    /// Check if streaming is complete
    pub fn is_complete(&self) -> bool {
        self.current_position >= self.total_size
    }

    /// Get total conservation sum across all processed chunks
    pub fn total_conservation(&self) -> u64 {
        self.total_conservation_sum
    }

    /// Merge intermediate results into a single projection
    pub fn merge_results(&self) -> AtlasResult<AtlasProjectionHandle> {
        if self.intermediate_results.is_empty() {
            return Err(AtlasError::InvalidInput("no intermediate results to merge"));
        }

        // For now, return the first result as the merged result
        // In a more sophisticated implementation, this would combine all projections
        // SAFETY: We checked that intermediate_results is not empty
        let first_handle = unsafe {
            let inner = self.intermediate_results[0].inner;
            AtlasProjectionHandle { inner }
        };

        Ok(first_handle)
    }
}

/// Stream large domain projections with chunked processing
pub fn stream_large_domain_projection(
    source_data: &[u8],
    projection_type: ProjectionType,
    config: StreamingConfig,
) -> AtlasResult<AtlasProjectionHandle> {
    if source_data.len() < config.chunk_size {
        // Data is small enough to process normally
        let projection = match projection_type {
            ProjectionType::Linear => AtlasProjection::new_linear(source_data)?,
            ProjectionType::R96Fourier => AtlasProjection::new_r96_fourier(source_data)?,
        };
        return Ok(AtlasProjectionHandle::new(projection));
    }

    let mut context = StreamingContext::new(config, source_data.len() as u64);
    let _results = context.process_streaming(Some(source_data), projection_type)?;
    
    context.merge_results()
}

/// Stream large domain projections from file
#[cfg(feature = "streaming")]
pub fn stream_file_projection<P: AsRef<Path>>(
    file_path: P,
    projection_type: ProjectionType,
    config: StreamingConfig,
) -> AtlasResult<AtlasProjectionHandle> {
    let mut context = StreamingContext::from_file(config, file_path)?;
    let _results = context.process_streaming(None, projection_type)?;
    
    context.merge_results()
}

#[cfg(test)]
mod tests {
    use super::*;

    fn create_test_data(size: usize) -> Vec<u8> {
        let mut data = vec![0u8; size];
        for (i, byte) in data.iter_mut().enumerate() {
            *byte = (i % 256) as u8;
        }
        data
    }

    #[test]
    fn test_streaming_config_creation() {
        let config = StreamingConfig::default();
        assert_eq!(config.chunk_size, DEFAULT_CHUNK_SIZE);
        assert_eq!(config.parallel_chunks, 4);
        
        let custom_config = StreamingConfig::new(1024 * 1024, 2).unwrap();
        assert_eq!(custom_config.chunk_size, 1024 * 1024);
        assert_eq!(custom_config.parallel_chunks, 2);
    }

    #[test]
    fn test_streaming_config_validation() {
        // Too small chunk size
        assert!(StreamingConfig::new(1024, 2).is_err());
        
        // Zero parallel chunks
        assert!(StreamingConfig::new(1024 * 1024, 0).is_err());
        
        // Too large chunk size
        assert!(StreamingConfig::new(512 * 1024 * 1024, 2).is_err());
    }

    #[test]
    fn test_streaming_chunk() {
        let test_data = create_test_data(1000);
        let chunk = StreamingChunk::new(0, test_data.clone(), 0);
        
        assert_eq!(chunk.id, 0);
        assert_eq!(chunk.start_offset, 0);
        assert_eq!(chunk.data.len(), 1000);
        assert!(chunk.verify_conservation());
        
        let expected_sum: u64 = test_data.iter().map(|&b| u64::from(b)).sum();
        assert_eq!(chunk.conservation_sum, expected_sum);
    }

    #[test]
    fn test_streaming_context() {
        let config = StreamingConfig::new(1000, 2).unwrap();
        let mut context = StreamingContext::new(config, 5000);
        
        assert_eq!(context.total_size, 5000);
        assert_eq!(context.current_position, 0);
        assert!(!context.is_complete());
        assert_eq!(context.progress(), 0.0);
        
        let test_data = create_test_data(5000);
        
        // Process first chunk
        let chunk1 = context.next_chunk(Some(&test_data)).unwrap().unwrap();
        assert_eq!(chunk1.id, 0);
        assert_eq!(chunk1.data.len(), 1000);
        assert_eq!(context.progress(), 0.2); // 1000/5000
        
        // Process until completion
        while let Some(_chunk) = context.next_chunk(Some(&test_data)).unwrap() {
            // Process chunk
        }
        
        assert!(context.is_complete());
        assert_eq!(context.progress(), 1.0);
    }

    #[test]
    fn test_small_data_streaming() {
        let test_data = create_test_data(1000); // Smaller than default chunk size
        let config = StreamingConfig::default();
        
        let result = stream_large_domain_projection(&test_data, ProjectionType::Linear, config);
        assert!(result.is_ok());
        
        let handle = result.unwrap();
        assert!(handle.is_valid());
    }

    #[test]
    fn test_large_data_streaming() {
        let test_data = create_test_data(50 * 1024 * 1024); // 50 MB - larger than default chunk
        let config = StreamingConfig::new(10 * 1024 * 1024, 2).unwrap(); // 10 MB chunks
        
        let result = stream_large_domain_projection(&test_data, ProjectionType::Linear, config);
        assert!(result.is_ok());
        
        let handle = result.unwrap();
        assert!(handle.is_valid());
    }
}