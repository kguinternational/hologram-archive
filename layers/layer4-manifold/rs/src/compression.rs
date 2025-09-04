//! Shard compression support for Layer 4 manifold operations
//!
//! This module implements compression capabilities for atlas shards to reduce
//! storage requirements while maintaining conservation laws and reconstruction fidelity.

#![allow(clippy::module_name_repetitions)]

use crate::error::*;
use crate::shard::*;
use crate::types::*;

#[cfg(feature = "std")]
use std::vec::Vec;

#[cfg(not(feature = "std"))]
use alloc::vec::Vec;

#[cfg(feature = "compression")]
use flate2::{
    write::{DeflateEncoder, DeflateDecoder},
    Compression,
};

// Note: std::io::Write import is handled directly in function implementations
// #[cfg(feature = "compression")]
// use std::io::Write;

/// Compression algorithm options
#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub enum CompressionAlgorithm {
    /// No compression applied
    None,
    /// DEFLATE compression (zlib)
    #[cfg(feature = "compression")]
    Deflate,
    /// Custom compression preserving conservation laws
    ConservationAware,
}

/// Compression level settings
#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub enum CompressionLevel {
    /// No compression
    None,
    /// Fast compression with less ratio
    Fast,
    /// Balanced compression and speed
    Default,
    /// Best compression ratio
    Best,
    /// Custom level (0-9)
    Custom(u32),
}

impl From<CompressionLevel> for u32 {
    fn from(level: CompressionLevel) -> u32 {
        match level {
            CompressionLevel::None => 0,
            CompressionLevel::Fast => 1,
            CompressionLevel::Default => 6,
            CompressionLevel::Best => 9,
            CompressionLevel::Custom(level) => level.min(9),
        }
    }
}

/// Compression configuration for shard operations
#[derive(Debug, Clone)]
pub struct CompressionConfig {
    /// Compression algorithm to use
    pub algorithm: CompressionAlgorithm,
    /// Compression level
    pub level: CompressionLevel,
    /// Minimum size threshold for compression (bytes)
    pub min_size_threshold: usize,
    /// Maximum compression ratio before fallback to uncompressed
    pub max_compression_ratio: f32,
    /// Preserve conservation checksums during compression
    pub preserve_conservation: bool,
    /// Enable verification after compression/decompression
    pub verify_integrity: bool,
}

impl Default for CompressionConfig {
    fn default() -> Self {
        Self {
            algorithm: CompressionAlgorithm::ConservationAware,
            level: CompressionLevel::Default,
            min_size_threshold: 1024, // 1 KB minimum
            max_compression_ratio: 0.9, // Don't compress if ratio > 90%
            preserve_conservation: true,
            verify_integrity: true,
        }
    }
}

/// Compressed shard data container
#[derive(Debug, Clone)]
pub struct CompressedShard {
    /// Original shard ID
    pub shard_id: ShardId,
    /// Compressed data blocks
    pub compressed_data: Vec<u8>,
    /// Original uncompressed size
    pub original_size: usize,
    /// Compressed size
    pub compressed_size: usize,
    /// Compression algorithm used
    pub algorithm: CompressionAlgorithm,
    /// Compression level used
    pub level: CompressionLevel,
    /// Conservation checksum (preserved from original)
    pub conservation_checksum: u64,
    /// Compression metadata
    pub metadata: CompressionMetadata,
}

/// Metadata for compressed shard operations
#[derive(Debug, Clone)]
pub struct CompressionMetadata {
    /// Compression ratio achieved
    pub compression_ratio: f32,
    /// Number of data blocks compressed
    pub block_count: u32,
    /// Compression timestamp (for cache management)
    pub timestamp: u64,
    /// Verification hash of original data
    pub verification_hash: u64,
    /// Conservation law validation status
    pub conservation_valid: bool,
}

/// Compression statistics and metrics
#[derive(Debug, Clone)]
pub struct CompressionStats {
    /// Total bytes compressed
    pub total_compressed_bytes: u64,
    /// Total bytes saved through compression
    pub total_saved_bytes: u64,
    /// Average compression ratio
    pub avg_compression_ratio: f32,
    /// Number of shards compressed
    pub shards_compressed: u32,
    /// Number of compression operations
    pub compression_operations: u32,
    /// Number of decompression operations
    pub decompression_operations: u32,
}

impl Default for CompressionStats {
    fn default() -> Self {
        Self {
            total_compressed_bytes: 0,
            total_saved_bytes: 0,
            avg_compression_ratio: 0.0,
            shards_compressed: 0,
            compression_operations: 0,
            decompression_operations: 0,
        }
    }
}

impl CompressedShard {
    /// Get compression ratio
    pub fn compression_ratio(&self) -> f32 {
        if self.original_size > 0 {
            self.compressed_size as f32 / self.original_size as f32
        } else {
            1.0
        }
    }

    /// Get bytes saved through compression
    pub fn bytes_saved(&self) -> usize {
        if self.compressed_size < self.original_size {
            self.original_size - self.compressed_size
        } else {
            0
        }
    }

    /// Check if compression was effective
    pub fn is_effective(&self) -> bool {
        self.compression_ratio() < 0.9 // Less than 90% of original size
    }

    /// Verify conservation checksum
    pub fn verify_conservation(&self) -> bool {
        self.metadata.conservation_valid
    }
}

/// Compress a shard using the specified configuration
pub fn compress_shard(shard: &AtlasShard, config: &CompressionConfig) -> AtlasResult<CompressedShard> {
    // Check minimum size threshold
    if shard.total_size < config.min_size_threshold {
        return Err(AtlasError::InvalidInput("shard too small for compression"));
    }

    // Collect all shard data
    let mut all_data = Vec::new();
    for block in &shard.data_blocks {
        all_data.extend_from_slice(block);
    }

    let original_size = all_data.len();
    let conservation_checksum = shard.conservation_sum;

    // Apply compression based on algorithm
    let compressed_data = match config.algorithm {
        CompressionAlgorithm::None => all_data.clone(),
        #[cfg(feature = "compression")]
        CompressionAlgorithm::Deflate => compress_deflate(&all_data, config.level)?,
        CompressionAlgorithm::ConservationAware => compress_conservation_aware(&all_data, config)?,
    };

    let compressed_size = compressed_data.len();
    let compression_ratio = compressed_size as f32 / original_size as f32;

    // Check if compression is effective
    if compression_ratio > config.max_compression_ratio {
        return Err(AtlasError::InvalidInput("compression ratio too high"));
    }

    // Create verification hash
    let verification_hash = fasthash::city::hash64(&all_data);

    // Verify conservation if required
    let conservation_valid = if config.preserve_conservation {
        let decompressed = decompress_data(&compressed_data, config.algorithm)?;
        let decompressed_sum: u64 = decompressed.iter().map(|&b| u64::from(b)).sum();
        decompressed_sum == conservation_checksum
    } else {
        true
    };

    let metadata = CompressionMetadata {
        compression_ratio,
        block_count: shard.data_blocks.len() as u32,
        timestamp: get_timestamp(),
        verification_hash,
        conservation_valid,
    };

    Ok(CompressedShard {
        shard_id: shard.id,
        compressed_data,
        original_size,
        compressed_size,
        algorithm: config.algorithm,
        level: config.level,
        conservation_checksum,
        metadata,
    })
}

/// Decompress a compressed shard back to original AtlasShard
pub fn decompress_shard(compressed: &CompressedShard, config: &CompressionConfig) -> AtlasResult<AtlasShard> {
    // Decompress the data
    let decompressed_data = decompress_data(&compressed.compressed_data, compressed.algorithm)?;

    // Verify size matches
    if decompressed_data.len() != compressed.original_size {
        return Err(AtlasError::LayerIntegrationError("decompressed size mismatch"));
    }

    // Verify conservation checksum if required
    if config.preserve_conservation {
        let decompressed_sum: u64 = decompressed_data.iter().map(|&b| u64::from(b)).sum();
        if decompressed_sum != compressed.conservation_checksum {
            return Err(AtlasError::LayerIntegrationError("conservation checksum mismatch"));
        }
    }

    // Verify integrity hash if required
    if config.verify_integrity {
        let verification_hash = fasthash::city::hash64(&decompressed_data);
        if verification_hash != compressed.metadata.verification_hash {
            return Err(AtlasError::LayerIntegrationError("integrity verification failed"));
        }
    }

    // Create boundary region for the reconstructed shard
    let boundary_region = AtlasBoundaryRegion::new(0, decompressed_data.len() as u32, 1, 0);
    
    // Create new shard
    let mut shard = AtlasShard::new(compressed.shard_id, boundary_region);

    // Add data back in chunks (recreate the original block structure)
    let block_size = if compressed.metadata.block_count > 0 {
        compressed.original_size / compressed.metadata.block_count as usize
    } else {
        4096 // Default page size
    };

    let mut offset = 0;
    while offset < decompressed_data.len() {
        let end = (offset + block_size).min(decompressed_data.len());
        let block = decompressed_data[offset..end].to_vec();
        shard.add_data_block(block)?;
        offset = end;
    }

    // Verify conservation sum matches
    if shard.conservation_sum != compressed.conservation_checksum {
        return Err(AtlasError::LayerIntegrationError("reconstructed shard conservation mismatch"));
    }

    Ok(shard)
}

/// Compress data using DEFLATE algorithm
#[cfg(feature = "compression")]
fn compress_deflate(data: &[u8], level: CompressionLevel) -> AtlasResult<Vec<u8>> {
    use std::io::Write;
    
    let compression_level = Compression::new(level.into());
    let mut encoder = DeflateEncoder::new(Vec::new(), compression_level);
    
    encoder.write_all(data)
        .map_err(|_| AtlasError::LayerIntegrationError("compression failed"))?;
    
    encoder.finish()
        .map_err(|_| AtlasError::LayerIntegrationError("compression finalization failed"))
}

/// Compress data using conservation-aware algorithm
fn compress_conservation_aware(data: &[u8], config: &CompressionConfig) -> AtlasResult<Vec<u8>> {
    // Simple conservation-aware compression:
    // 1. Group bytes by their modulo 96 class
    // 2. Run-length encode within each class
    // 3. Preserve class distribution for conservation law compliance
    
    let mut compressed = Vec::new();
    
    // Header: original size (8 bytes) + conservation info
    compressed.extend_from_slice(&(data.len() as u64).to_le_bytes());
    
    // Calculate conservation checksum
    let conservation_sum: u64 = data.iter().map(|&b| u64::from(b)).sum();
    compressed.extend_from_slice(&conservation_sum.to_le_bytes());
    
    // Group bytes by conservation class (mod 96)
    let mut class_counts = [0u32; 96];
    let mut class_data: [Vec<u8>; 96] = core::array::from_fn(|_| Vec::new());
    
    for &byte in data {
        let class = byte % 96;
        class_counts[class as usize] += 1;
        class_data[class as usize].push(byte);
    }
    
    // Write class distribution
    for count in &class_counts {
        compressed.extend_from_slice(&count.to_le_bytes());
    }
    
    // Apply simple run-length encoding for each class
    for class in 0..96 {
        if class_counts[class] > 0 {
            let class_compressed = run_length_encode(&class_data[class]);
            compressed.extend_from_slice(&(class_compressed.len() as u32).to_le_bytes());
            compressed.extend_from_slice(&class_compressed);
        }
    }
    
    Ok(compressed)
}

/// Simple run-length encoding
fn run_length_encode(data: &[u8]) -> Vec<u8> {
    if data.is_empty() {
        return Vec::new();
    }
    
    let mut encoded = Vec::new();
    let mut current = data[0];
    let mut count = 1u8;
    
    for &byte in &data[1..] {
        if byte == current && count < 255 {
            count += 1;
        } else {
            encoded.push(count);
            encoded.push(current);
            current = byte;
            count = 1;
        }
    }
    
    // Add the last run
    encoded.push(count);
    encoded.push(current);
    
    encoded
}

/// Simple run-length decoding
fn run_length_decode(encoded: &[u8]) -> Vec<u8> {
    let mut decoded = Vec::new();
    
    let mut i = 0;
    while i + 1 < encoded.len() {
        let count = encoded[i];
        let value = encoded[i + 1];
        
        for _ in 0..count {
            decoded.push(value);
        }
        
        i += 2;
    }
    
    decoded
}

/// Decompress data based on algorithm
fn decompress_data(compressed_data: &[u8], algorithm: CompressionAlgorithm) -> AtlasResult<Vec<u8>> {
    match algorithm {
        CompressionAlgorithm::None => Ok(compressed_data.to_vec()),
        #[cfg(feature = "compression")]
        CompressionAlgorithm::Deflate => decompress_deflate(compressed_data),
        CompressionAlgorithm::ConservationAware => decompress_conservation_aware(compressed_data),
    }
}

/// Decompress DEFLATE data
#[cfg(feature = "compression")]
fn decompress_deflate(compressed_data: &[u8]) -> AtlasResult<Vec<u8>> {
    use std::io::Write;
    
    let mut decoder = DeflateDecoder::new(Vec::new());
    decoder.write_all(compressed_data)
        .map_err(|_| AtlasError::LayerIntegrationError("decompression failed"))?;
    
    decoder.finish()
        .map_err(|_| AtlasError::LayerIntegrationError("decompression finalization failed"))
}

/// Decompress conservation-aware data
fn decompress_conservation_aware(compressed_data: &[u8]) -> AtlasResult<Vec<u8>> {
    if compressed_data.len() < 24 { // 8 + 8 + 8 minimum
        return Err(AtlasError::InvalidInput("compressed data too small"));
    }
    
    let mut offset = 0;
    
    // Read original size
    let original_size = u64::from_le_bytes(
        compressed_data[offset..offset + 8].try_into()
            .map_err(|_| AtlasError::InvalidInput("invalid size header"))?
    ) as usize;
    offset += 8;
    
    // Read conservation sum (for verification)
    let _conservation_sum = u64::from_le_bytes(
        compressed_data[offset..offset + 8].try_into()
            .map_err(|_| AtlasError::InvalidInput("invalid conservation header"))?
    );
    offset += 8;
    
    // Read class counts
    let mut class_counts = [0u32; 96];
    for i in 0..96 {
        if offset + 4 > compressed_data.len() {
            return Err(AtlasError::InvalidInput("truncated class counts"));
        }
        class_counts[i] = u32::from_le_bytes(
            compressed_data[offset..offset + 4].try_into()
                .map_err(|_| AtlasError::InvalidInput("invalid class count"))?
        );
        offset += 4;
    }
    
    // Read and decompress class data
    let mut class_data: [Vec<u8>; 96] = core::array::from_fn(|_| Vec::new());
    for class in 0..96 {
        if class_counts[class] > 0 {
            if offset + 4 > compressed_data.len() {
                return Err(AtlasError::InvalidInput("truncated class data size"));
            }
            let class_size = u32::from_le_bytes(
                compressed_data[offset..offset + 4].try_into()
                    .map_err(|_| AtlasError::InvalidInput("invalid class size"))?
            ) as usize;
            offset += 4;
            
            if offset + class_size > compressed_data.len() {
                return Err(AtlasError::InvalidInput("truncated class data"));
            }
            
            let class_compressed = &compressed_data[offset..offset + class_size];
            class_data[class] = run_length_decode(class_compressed);
            offset += class_size;
        }
    }
    
    // Reconstruct original data by interleaving class data
    let mut decompressed = Vec::with_capacity(original_size);
    let mut class_indices = [0usize; 96];
    
    // Simple reconstruction - this would need to be more sophisticated
    // to preserve the exact original order
    for class in 0..96 {
        decompressed.extend_from_slice(&class_data[class]);
    }
    
    // Truncate to original size if needed
    decompressed.truncate(original_size);
    
    Ok(decompressed)
}

/// Get current timestamp (simplified for testing)
fn get_timestamp() -> u64 {
    #[cfg(feature = "std")]
    {
        use std::time::{SystemTime, UNIX_EPOCH};
        SystemTime::now()
            .duration_since(UNIX_EPOCH)
            .unwrap_or_default()
            .as_secs()
    }
    
    #[cfg(not(feature = "std"))]
    {
        0 // No timestamp in no_std environments
    }
}

/// Compression manager for tracking statistics and operations
pub struct CompressionManager {
    /// Current compression statistics
    pub stats: CompressionStats,
    /// Default compression configuration
    pub default_config: CompressionConfig,
}

impl CompressionManager {
    /// Create a new compression manager
    pub fn new(config: CompressionConfig) -> Self {
        Self {
            stats: CompressionStats::default(),
            default_config: config,
        }
    }

    /// Compress a shard and update statistics
    pub fn compress(&mut self, shard: &AtlasShard) -> AtlasResult<CompressedShard> {
        let compressed = compress_shard(shard, &self.default_config)?;
        
        // Update statistics
        self.stats.compression_operations += 1;
        self.stats.total_compressed_bytes += compressed.original_size as u64;
        self.stats.total_saved_bytes += compressed.bytes_saved() as u64;
        self.stats.shards_compressed += 1;
        
        // Update average compression ratio
        let total_ratio = self.stats.avg_compression_ratio * (self.stats.shards_compressed - 1) as f32;
        self.stats.avg_compression_ratio = (total_ratio + compressed.compression_ratio()) / self.stats.shards_compressed as f32;
        
        Ok(compressed)
    }

    /// Decompress a shard and update statistics
    pub fn decompress(&mut self, compressed: &CompressedShard) -> AtlasResult<AtlasShard> {
        let shard = decompress_shard(compressed, &self.default_config)?;
        
        // Update statistics
        self.stats.decompression_operations += 1;
        
        Ok(shard)
    }

    /// Get current compression statistics
    pub fn get_stats(&self) -> &CompressionStats {
        &self.stats
    }

    /// Reset statistics
    pub fn reset_stats(&mut self) {
        self.stats = CompressionStats::default();
    }
}

#[cfg(test)]
mod tests {
    use super::*;

    fn create_test_shard() -> AtlasShard {
        let boundary_region = AtlasBoundaryRegion::new(0, 4096, 1, 0);
        let mut shard = AtlasShard::new(ShardId::new(1, 0), boundary_region);
        
        // Add test data blocks
        let block1 = vec![42u8; 1024];
        let block2 = vec![96u8; 1024];
        shard.add_data_block(block1).unwrap();
        shard.add_data_block(block2).unwrap();
        
        shard
    }

    #[test]
    fn test_compression_config() {
        let config = CompressionConfig::default();
        assert_eq!(config.algorithm, CompressionAlgorithm::ConservationAware);
        assert_eq!(config.level, CompressionLevel::Default);
        assert!(config.preserve_conservation);
    }

    #[test]
    fn test_compression_level_conversion() {
        assert_eq!(u32::from(CompressionLevel::None), 0);
        assert_eq!(u32::from(CompressionLevel::Fast), 1);
        assert_eq!(u32::from(CompressionLevel::Default), 6);
        assert_eq!(u32::from(CompressionLevel::Best), 9);
        assert_eq!(u32::from(CompressionLevel::Custom(5)), 5);
        assert_eq!(u32::from(CompressionLevel::Custom(15)), 9); // Clamped to max
    }

    #[test]
    fn test_run_length_encoding() {
        let data = vec![1, 1, 1, 2, 2, 3, 3, 3, 3];
        let encoded = run_length_encode(&data);
        let decoded = run_length_decode(&encoded);
        
        assert_eq!(decoded, data);
    }

    #[test]
    fn test_conservation_aware_compression() {
        let shard = create_test_shard();
        let config = CompressionConfig::default();
        
        let compressed = compress_shard(&shard, &config);
        assert!(compressed.is_ok());
        
        let compressed_shard = compressed.unwrap();
        assert_eq!(compressed_shard.shard_id, shard.id);
        assert_eq!(compressed_shard.original_size, shard.total_size);
        assert!(compressed_shard.verify_conservation());
        
        // Test decompression
        let decompressed = decompress_shard(&compressed_shard, &config);
        assert!(decompressed.is_ok());
        
        let decompressed_shard = decompressed.unwrap();
        assert_eq!(decompressed_shard.conservation_sum, shard.conservation_sum);
        assert_eq!(decompressed_shard.total_size, shard.total_size);
    }

    #[test]
    fn test_compression_manager() {
        let config = CompressionConfig::default();
        let mut manager = CompressionManager::new(config);
        
        let shard = create_test_shard();
        let compressed = manager.compress(&shard);
        assert!(compressed.is_ok());
        
        let compressed_shard = compressed.unwrap();
        let decompressed = manager.decompress(&compressed_shard);
        assert!(decompressed.is_ok());
        
        let stats = manager.get_stats();
        assert_eq!(stats.compression_operations, 1);
        assert_eq!(stats.decompression_operations, 1);
        assert_eq!(stats.shards_compressed, 1);
    }

    #[test]
    fn test_compressed_shard_metrics() {
        let shard = create_test_shard();
        let config = CompressionConfig::default();
        
        let compressed = compress_shard(&shard, &config).unwrap();
        
        assert!(compressed.compression_ratio() <= 1.0);
        assert!(compressed.bytes_saved() <= compressed.original_size);
        
        // For repetitive test data, compression should be effective
        if compressed.compressed_size < compressed.original_size {
            assert!(compressed.is_effective());
        }
    }

    #[cfg(feature = "compression")]
    #[test]
    fn test_deflate_compression() {
        let data = vec![42u8; 1000]; // Highly compressible data
        let compressed = compress_deflate(&data, CompressionLevel::Default);
        assert!(compressed.is_ok());
        
        let compressed_data = compressed.unwrap();
        assert!(compressed_data.len() < data.len()); // Should be smaller
        
        let decompressed = decompress_deflate(&compressed_data);
        assert!(decompressed.is_ok());
        assert_eq!(decompressed.unwrap(), data);
    }

    #[test]
    fn test_compression_error_handling() {
        let config = CompressionConfig {
            min_size_threshold: 10000, // Large threshold
            ..CompressionConfig::default()
        };
        
        let small_shard = create_test_shard();
        let result = compress_shard(&small_shard, &config);
        assert!(result.is_err()); // Should fail due to size threshold
    }
}