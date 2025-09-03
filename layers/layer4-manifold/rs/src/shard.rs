//! Data sharding and distribution for manifold operations

use crate::{types::*, error::*};

#[cfg(feature = "std")]
use std::vec::Vec;

#[cfg(not(feature = "std"))]
use alloc::vec::Vec;

impl ShardId {
    /// Create a new shard identifier
    pub fn new(primary: u64, secondary: u32) -> Self {
        Self { primary, secondary }
    }
    
    /// Create a shard ID from a hash value
    pub fn from_hash(hash: u64) -> Self {
        Self {
            primary: hash,
            secondary: (hash >> 32) as u32,
        }
    }
    
    /// Get the combined identifier as a single u64
    pub fn combined(&self) -> u64 {
        self.primary ^ ((self.secondary as u64) << 32)
    }
}

/// Sharding strategy for distributing manifold data
#[derive(Debug, Clone)]
pub enum ShardStrategy {
    /// Hash-based sharding using coordinate hash
    CoordinateHash { num_shards: usize },
    /// Spatial partitioning based on coordinate ranges
    Spatial { bounds: Vec<(Float, Float)> },
    /// Hierarchical sharding for multi-level decomposition
    Hierarchical { levels: Vec<usize> },
    /// Custom sharding function
    Custom { shard_fn: fn(&[Float]) -> ShardId },
}

/// Shard metadata containing distribution information
#[derive(Debug, Clone)]
pub struct ShardMetadata {
    /// Shard identifier
    pub id: ShardId,
    /// Number of points in this shard
    pub point_count: usize,
    /// Bounding box of points in this shard
    pub bounds: Vec<(Float, Float)>,
    /// Load factor (0.0 to 1.0)
    pub load_factor: Float,
}

/// Shard manager for coordinating distributed operations
pub struct ShardManager {
    /// Sharding strategy
    strategy: ShardStrategy,
    /// Active shards metadata
    shards: Vec<ShardMetadata>,
    /// Total number of points across all shards
    total_points: usize,
}

impl ShardManager {
    /// Create a new shard manager with the specified strategy
    pub fn new(strategy: ShardStrategy) -> Self {
        Self {
            strategy,
            shards: Vec::new(),
            total_points: 0,
        }
    }
    
    /// Determine which shard a point belongs to
    pub fn shard_for_point<const N: usize>(&self, point: &AtlasPoint<N>) -> AtlasResult<ShardId> {
        match &self.strategy {
            ShardStrategy::CoordinateHash { num_shards } => {
                let hash = self.hash_coordinates(&point.coords);
                let shard_index = (hash as usize) % num_shards;
                Ok(ShardId::new(shard_index as u64, 0))
            },
            ShardStrategy::Spatial { bounds } => {
                self.spatial_shard(point, bounds)
            },
            ShardStrategy::Hierarchical { levels } => {
                self.hierarchical_shard(point, levels)
            },
            ShardStrategy::Custom { shard_fn } => {
                Ok(shard_fn(&point.coords))
            },
        }
    }
    
    /// Add a new shard to the manager
    pub fn add_shard(&mut self, metadata: ShardMetadata) {
        self.total_points += metadata.point_count;
        self.shards.push(metadata);
    }
    
    /// Remove a shard from the manager
    pub fn remove_shard(&mut self, shard_id: ShardId) -> AtlasResult<()> {
        let index = self.shards
            .iter()
            .position(|s| s.id == shard_id)
            .ok_or(AtlasError::InvalidInput("shard not found"))?;
            
        let removed = self.shards.remove(index);
        self.total_points -= removed.point_count;
        Ok(())
    }
    
    /// Get metadata for a specific shard
    pub fn shard_metadata(&self, shard_id: ShardId) -> Option<&ShardMetadata> {
        self.shards.iter().find(|s| s.id == shard_id)
    }
    
    /// Get all active shards
    pub fn all_shards(&self) -> &[ShardMetadata] {
        &self.shards
    }
    
    /// Rebalance shards based on load factors
    pub fn rebalance(&mut self) -> AtlasResult<Vec<ShardOperation>> {
        let mut operations = Vec::new();
        
        // Find overloaded and underloaded shards
        let avg_load = self.total_points as Float / self.shards.len() as Float;
        
        for shard in &self.shards {
            let expected_load = avg_load;
            let actual_load = shard.point_count as Float;
            
            if actual_load > expected_load * 1.2 {
                // Shard is overloaded - split it
                operations.push(ShardOperation::Split {
                    shard_id: shard.id,
                    split_strategy: SplitStrategy::Balanced,
                });
            } else if actual_load < expected_load * 0.5 && self.shards.len() > 1 {
                // Shard is underloaded - consider merging
                operations.push(ShardOperation::Merge {
                    source_id: shard.id,
                    target_id: self.find_merge_candidate(shard.id)?,
                });
            }
        }
        
        Ok(operations)
    }
    
    // Helper methods
    
    fn hash_coordinates(&self, coords: &[Float]) -> u64 {
        // Simple hash function for coordinates
        let mut hash: u64 = 0;
        for &coord in coords {
            let bits = coord.to_bits();
            hash = hash.wrapping_mul(31).wrapping_add(bits);
        }
        hash
    }
    
    fn spatial_shard<const N: usize>(
        &self, 
        point: &AtlasPoint<N>, 
        bounds: &[(Float, Float)]
    ) -> AtlasResult<ShardId> {
        if bounds.len() != N {
            return Err(AtlasError::InvalidDimension(bounds.len() as u32));
        }
        
        let mut shard_coords = Vec::with_capacity(N);
        
        for (i, &coord) in point.coords.iter().enumerate() {
            let (min_bound, max_bound) = bounds[i];
            if coord < min_bound || coord > max_bound {
                return Err(AtlasError::CoordinateError("point outside bounds"));
            }
            
            // Normalize coordinate to [0, 1] range
            let normalized = (coord - min_bound) / (max_bound - min_bound);
            shard_coords.push(normalized);
        }
        
        // Convert normalized coordinates to shard ID
        let shard_coords_slice: Vec<Float> = shard_coords;
        let primary = self.hash_coordinates(&shard_coords_slice);
        Ok(ShardId::new(primary, 0))
    }
    
    fn hierarchical_shard<const N: usize>(
        &self, 
        point: &AtlasPoint<N>, 
        levels: &[usize]
    ) -> AtlasResult<ShardId> {
        let mut primary = 0u64;
        let mut secondary = 0u32;
        
        for (level, &partitions) in levels.iter().enumerate() {
            let level_hash = self.hash_coordinates(&point.coords[..core::cmp::min(N, level + 1)]);
            let partition = (level_hash as usize) % partitions;
            
            if level < 4 {
                primary = (primary << 8) | (partition as u64);
            } else {
                secondary = (secondary << 8) | (partition as u32);
            }
        }
        
        Ok(ShardId::new(primary, secondary))
    }
    
    fn find_merge_candidate(&self, shard_id: ShardId) -> AtlasResult<ShardId> {
        // Find the shard with the lowest load factor for merging
        self.shards
            .iter()
            .filter(|s| s.id != shard_id)
            .min_by(|a, b| a.load_factor.partial_cmp(&b.load_factor).unwrap())
            .map(|s| s.id)
            .ok_or(AtlasError::InvalidInput("no merge candidate found"))
    }
}

/// Operations that can be performed on shards
#[derive(Debug, Clone)]
pub enum ShardOperation {
    /// Split an overloaded shard
    Split {
        shard_id: ShardId,
        split_strategy: SplitStrategy,
    },
    /// Merge two underloaded shards
    Merge {
        source_id: ShardId,
        target_id: ShardId,
    },
    /// Migrate data between shards
    Migrate {
        from_shard: ShardId,
        to_shard: ShardId,
        data_range: (usize, usize),
    },
}

/// Strategy for splitting shards
#[derive(Debug, Clone)]
pub enum SplitStrategy {
    /// Split to balance load evenly
    Balanced,
    /// Split along a specific dimension
    Dimensional { dimension: usize },
    /// Split using a custom function
    Custom { split_fn: fn(&ShardMetadata) -> (ShardId, ShardId) },
}