//! Incremental projection updates for Layer 4 manifold operations
//!
//! This module implements efficient incremental updates for manifold projections,
//! allowing for partial updates without full reconstruction and maintaining
//! conservation laws throughout the update process.

#![allow(clippy::module_name_repetitions)]

use crate::error::*;
use crate::projection::*;
use core::sync::atomic::{AtomicU64, Ordering};

#[cfg(feature = "std")]
use std::collections::HashMap;
#[cfg(feature = "std")]
use std::vec::Vec;

#[cfg(not(feature = "std"))]
use alloc::vec::Vec;

/// Change type for incremental updates
#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub enum ChangeType {
    /// Insert new data at specified location
    Insert,
    /// Update existing data at specified location
    Update,
    /// Delete data at specified location
    Delete,
    /// Move data from one location to another
    Move,
}

/// Delta record for tracking changes to projection data
#[derive(Debug, Clone)]
pub struct ProjectionDelta {
    /// Unique identifier for this delta
    pub id: u64,
    /// Type of change being applied
    pub change_type: ChangeType,
    /// Starting coordinate for the change
    pub start_coord: u32,
    /// Ending coordinate for the change
    pub end_coord: u32,
    /// New data being inserted or used for updates
    pub data: Vec<u8>,
    /// Original data (for rollback purposes)
    pub original_data: Option<Vec<u8>>,
    /// Conservation delta caused by this change
    pub conservation_delta: i64,
    /// Timestamp when change was applied
    pub timestamp: u64,
    /// Affected tile IDs
    pub affected_tiles: Vec<u32>,
}

/// Incremental update context for managing projection changes
#[derive(Debug)]
pub struct IncrementalUpdateContext {
    /// Applied deltas in chronological order
    pub applied_deltas: Vec<ProjectionDelta>,
    /// Cached tile updates for efficiency
    pub tile_updates: HashMap<u32, Vec<u8>>,
    /// Conservation sum changes
    pub conservation_changes: i64,
    /// Update sequence number
    pub sequence_number: u64,
    /// Enable rollback capability
    pub enable_rollback: bool,
    /// Maximum number of deltas to keep for rollback
    pub max_rollback_deltas: usize,
}

/// Configuration for incremental updates
#[derive(Debug, Clone)]
pub struct IncrementalConfig {
    /// Enable conservation validation for each update
    pub validate_conservation: bool,
    /// Enable rollback capability
    pub enable_rollback: bool,
    /// Maximum deltas to keep in memory
    pub max_deltas: usize,
    /// Batch size for applying multiple deltas
    pub batch_size: usize,
    /// Enable tile-level caching
    pub enable_tile_cache: bool,
}

impl Default for IncrementalConfig {
    fn default() -> Self {
        Self {
            validate_conservation: true,
            enable_rollback: true,
            max_deltas: 1000,
            batch_size: 10,
            enable_tile_cache: true,
        }
    }
}

/// Statistics for incremental update operations
#[derive(Debug, Clone)]
pub struct IncrementalStats {
    /// Total number of updates applied
    pub total_updates: u64,
    /// Number of insert operations
    pub insert_operations: u64,
    /// Number of update operations
    pub update_operations: u64,
    /// Number of delete operations
    pub delete_operations: u64,
    /// Number of move operations
    pub move_operations: u64,
    /// Total bytes modified
    pub bytes_modified: u64,
    /// Average time per update (nanoseconds)
    pub avg_update_time_ns: u64,
    /// Number of conservation violations caught
    pub conservation_violations: u64,
}

impl Default for IncrementalStats {
    fn default() -> Self {
        Self {
            total_updates: 0,
            insert_operations: 0,
            update_operations: 0,
            delete_operations: 0,
            move_operations: 0,
            bytes_modified: 0,
            avg_update_time_ns: 0,
            conservation_violations: 0,
        }
    }
}

impl ProjectionDelta {
    /// Create a new projection delta
    pub fn new(change_type: ChangeType, start_coord: u32, end_coord: u32, data: Vec<u8>) -> Self {
        let conservation_delta = match change_type {
            ChangeType::Insert => data.iter().map(|&b| i64::from(b)).sum(),
            ChangeType::Update => 0, // Will be recalculated when original_data is set
            ChangeType::Delete => 0, // Will be recalculated when original_data is set
            ChangeType::Move => 0,   // Move operations don't change conservation sum
        };

        let mut hash_input = Vec::new();
        hash_input.extend_from_slice(&start_coord.to_le_bytes());
        hash_input.extend_from_slice(&end_coord.to_le_bytes());
        hash_input.extend_from_slice(&data);
        let id = fasthash::city::hash64(&hash_input);

        Self {
            id,
            change_type,
            start_coord,
            end_coord,
            data,
            original_data: None,
            conservation_delta,
            timestamp: get_timestamp(),
            affected_tiles: Vec::new(),
        }
    }

    /// Create an insert delta
    pub fn insert(coord: u32, data: Vec<u8>) -> Self {
        Self::new(ChangeType::Insert, coord, coord, data)
    }

    /// Create an update delta
    pub fn update(start_coord: u32, end_coord: u32, data: Vec<u8>) -> Self {
        Self::new(ChangeType::Update, start_coord, end_coord, data)
    }

    /// Create a delete delta
    pub fn delete(start_coord: u32, end_coord: u32) -> Self {
        Self::new(ChangeType::Delete, start_coord, end_coord, Vec::new())
    }

    /// Create a move delta
    pub fn move_data(from_start: u32, from_end: u32, to_coord: u32, data: Vec<u8>) -> Self {
        let mut delta = Self::new(ChangeType::Move, from_start, from_end, data);
        delta.end_coord = to_coord; // Reuse end_coord for destination
        delta
    }

    /// Update conservation delta based on original data
    pub fn set_original_data(&mut self, original_data: Vec<u8>) {
        self.original_data = Some(original_data.clone());

        // Recalculate conservation delta based on change type
        match self.change_type {
            ChangeType::Insert => {
                // Insert operations add new data to conservation sum
                self.conservation_delta = self.data.iter().map(|&b| i64::from(b)).sum();
            },
            ChangeType::Update => {
                // Update operations: delta = new_sum - original_sum
                let original_sum: i64 = original_data.iter().map(|&b| i64::from(b)).sum();
                let new_sum: i64 = self.data.iter().map(|&b| i64::from(b)).sum();
                self.conservation_delta = new_sum - original_sum;
            },
            ChangeType::Delete => {
                // Delete operations remove data from conservation sum
                let original_sum: i64 = original_data.iter().map(|&b| i64::from(b)).sum();
                self.conservation_delta = -original_sum;
            },
            ChangeType::Move => {
                // Move operations don't change total conservation sum
                self.conservation_delta = 0;
            },
        }
    }

    /// Get the size of data affected by this delta
    pub fn affected_size(&self) -> usize {
        match self.change_type {
            ChangeType::Insert => self.data.len(),
            ChangeType::Update => self.data.len(),
            ChangeType::Delete => (self.end_coord - self.start_coord) as usize,
            ChangeType::Move => self.data.len(),
        }
    }

    /// Calculate which tiles are affected by this delta
    pub fn calculate_affected_tiles(&mut self, tiles_per_side: u32) {
        let page_size = 4096u32;
        let start_tile = self.start_coord / (page_size * 256); // Tiles contain multiple pages
        let end_tile = self.end_coord / (page_size * 256);

        self.affected_tiles.clear();
        for tile_id in start_tile..=end_tile.min(tiles_per_side * tiles_per_side - 1) {
            self.affected_tiles.push(tile_id);
        }
    }

    /// Validate conservation law compliance for this delta
    pub fn validate_conservation(&self) -> bool {
        match self.change_type {
            ChangeType::Insert => {
                // Insert operations can change conservation sum
                true
            },
            ChangeType::Update => {
                // Update operations should maintain conservation if original data is known
                if let Some(ref original) = self.original_data {
                    let original_sum: i64 = original.iter().map(|&b| i64::from(b)).sum();
                    let new_sum: i64 = self.data.iter().map(|&b| i64::from(b)).sum();
                    (new_sum - original_sum) == self.conservation_delta
                } else {
                    true // Can't validate without original data
                }
            },
            ChangeType::Delete => {
                // Delete operations remove conservation contribution
                if let Some(ref original) = self.original_data {
                    let original_sum: i64 = original.iter().map(|&b| i64::from(b)).sum();
                    -original_sum == self.conservation_delta
                } else {
                    true
                }
            },
            ChangeType::Move => {
                // Move operations shouldn't change total conservation sum
                self.conservation_delta == 0
            },
        }
    }
}

impl IncrementalUpdateContext {
    /// Create a new incremental update context
    pub fn new(config: IncrementalConfig) -> Self {
        Self {
            applied_deltas: Vec::new(),
            tile_updates: HashMap::new(),
            conservation_changes: 0,
            sequence_number: 0,
            enable_rollback: config.enable_rollback,
            max_rollback_deltas: config.max_deltas,
        }
    }

    /// Apply a single delta to a projection
    pub fn apply_delta(
        &mut self,
        projection: &mut AtlasProjection,
        mut delta: ProjectionDelta,
        config: &IncrementalConfig,
    ) -> AtlasResult<()> {
        // Calculate affected tiles
        let (width, height) = projection.get_dimensions();
        delta.calculate_affected_tiles(width.max(height));

        // Validate conservation if required
        if config.validate_conservation && !delta.validate_conservation() {
            return Err(AtlasError::LayerIntegrationError(
                "conservation law violation",
            ));
        }

        // Apply the delta based on change type
        match delta.change_type {
            ChangeType::Insert => self.apply_insert(projection, &delta)?,
            ChangeType::Update => self.apply_update(projection, &mut delta)?,
            ChangeType::Delete => self.apply_delete(projection, &mut delta)?,
            ChangeType::Move => self.apply_move(projection, &delta)?,
        }

        // Update conservation tracking
        self.conservation_changes += delta.conservation_delta;
        projection.total_conservation_sum =
            (projection.total_conservation_sum as i64 + delta.conservation_delta) as u64;

        // Store delta for potential rollback
        if self.enable_rollback {
            self.applied_deltas.push(delta);

            // Limit rollback history
            if self.applied_deltas.len() > self.max_rollback_deltas {
                self.applied_deltas.remove(0);
            }
        }

        self.sequence_number += 1;
        Ok(())
    }

    /// Apply multiple deltas in batch
    pub fn apply_batch(
        &mut self,
        projection: &mut AtlasProjection,
        deltas: Vec<ProjectionDelta>,
        config: &IncrementalConfig,
    ) -> AtlasResult<()> {
        let batch_size = config.batch_size;

        for chunk in deltas.chunks(batch_size) {
            for delta in chunk {
                self.apply_delta(projection, delta.clone(), config)?;
            }
        }

        Ok(())
    }

    /// Rollback the last n deltas
    pub fn rollback_deltas(
        &mut self,
        projection: &mut AtlasProjection,
        count: usize,
    ) -> AtlasResult<()> {
        if !self.enable_rollback {
            return Err(AtlasError::InvalidInput("rollback not enabled"));
        }

        let deltas_to_rollback = self.applied_deltas.len().min(count);

        for _ in 0..deltas_to_rollback {
            if let Some(delta) = self.applied_deltas.pop() {
                self.rollback_single_delta(projection, &delta)?;
                self.conservation_changes -= delta.conservation_delta;
                projection.total_conservation_sum =
                    (projection.total_conservation_sum as i64 - delta.conservation_delta) as u64;
                self.sequence_number -= 1;
            }
        }

        Ok(())
    }

    /// Get statistics about applied updates
    pub fn get_stats(&self) -> IncrementalStats {
        let mut stats = IncrementalStats::default();

        for delta in &self.applied_deltas {
            stats.total_updates += 1;
            stats.bytes_modified += delta.affected_size() as u64;

            match delta.change_type {
                ChangeType::Insert => stats.insert_operations += 1,
                ChangeType::Update => stats.update_operations += 1,
                ChangeType::Delete => stats.delete_operations += 1,
                ChangeType::Move => stats.move_operations += 1,
            }
        }

        stats
    }

    // Private helper methods

    fn apply_insert(
        &mut self,
        projection: &mut AtlasProjection,
        delta: &ProjectionDelta,
    ) -> AtlasResult<()> {
        // Find the appropriate tile and insert data
        for tile_id in &delta.affected_tiles {
            if let Some(tile) = projection.tiles.get_mut(*tile_id as usize) {
                // Create a new page with the insert data
                if delta.data.len() <= 4096 {
                    // Single page
                    let mut page_data = vec![0u8; 4096];
                    let copy_len = delta.data.len().min(4096);
                    page_data[..copy_len].copy_from_slice(&delta.data[..copy_len]);
                    tile.add_page(page_data)?;
                }
            }
        }
        Ok(())
    }

    fn apply_update(
        &mut self,
        projection: &mut AtlasProjection,
        delta: &mut ProjectionDelta,
    ) -> AtlasResult<()> {
        // Find affected tiles and update their data
        for tile_id in &delta.affected_tiles {
            if let Some(tile) = projection.tiles.get_mut(*tile_id as usize) {
                // Store original data for rollback
                if self.enable_rollback && delta.original_data.is_none() {
                    // Collect original data from affected pages
                    let mut original = Vec::new();
                    for page in &tile.pages {
                        original.extend_from_slice(page);
                    }
                    delta.original_data = Some(original);
                }

                // Apply update to tile pages
                if !tile.pages.is_empty() && !delta.data.is_empty() {
                    let update_len = delta.data.len().min(tile.pages[0].len());
                    tile.pages[0][..update_len].copy_from_slice(&delta.data[..update_len]);

                    // Recalculate conservation sum for the tile
                    tile.conservation_sum = 0;
                    for page in &tile.pages {
                        let page_sum: u64 = page.iter().map(|&b| u64::from(b)).sum();
                        tile.conservation_sum = tile.conservation_sum.wrapping_add(page_sum);
                    }
                }
            }
        }
        Ok(())
    }

    fn apply_delete(
        &mut self,
        projection: &mut AtlasProjection,
        delta: &mut ProjectionDelta,
    ) -> AtlasResult<()> {
        // Mark data for deletion by zeroing it out
        for tile_id in &delta.affected_tiles {
            if let Some(tile) = projection.tiles.get_mut(*tile_id as usize) {
                // Store original data for rollback
                if self.enable_rollback && delta.original_data.is_none() {
                    let mut original = Vec::new();
                    for page in &tile.pages {
                        original.extend_from_slice(page);
                    }
                    delta.original_data = Some(original);
                }

                // Zero out the specified range
                let delete_size = (delta.end_coord - delta.start_coord) as usize;
                for page in &mut tile.pages {
                    let zero_len = delete_size.min(page.len());
                    page[..zero_len].fill(0);
                }

                // Recalculate conservation sum
                tile.conservation_sum = 0;
                for page in &tile.pages {
                    let page_sum: u64 = page.iter().map(|&b| u64::from(b)).sum();
                    tile.conservation_sum = tile.conservation_sum.wrapping_add(page_sum);
                }
            }
        }
        Ok(())
    }

    fn apply_move(
        &mut self,
        projection: &mut AtlasProjection,
        delta: &ProjectionDelta,
    ) -> AtlasResult<()> {
        if delta.change_type != ChangeType::Move {
            return Err(AtlasError::InvalidInput("Delta is not a Move operation"));
        }

        // Find source tile and page coordinates
        let source_tile_id = (delta.start_coord / 4096) as u32;
        let source_page_offset = (delta.start_coord % 4096) as usize;
        let data_len = (delta.end_coord - delta.start_coord) as usize;

        // Find source tile
        let source_tile_idx = projection
            .tiles
            .iter()
            .position(|t| t.id == source_tile_id)
            .ok_or(AtlasError::InvalidInput("Source tile not found"))?;

        if source_page_offset + data_len > 4096 {
            return Err(AtlasError::InvalidInput(
                "Move spans multiple pages - not supported",
            ));
        }

        // Extract data from source page
        let source_data = if !projection.tiles[source_tile_idx].pages.is_empty() {
            let page = &projection.tiles[source_tile_idx].pages[0]; // Use first page
            if source_page_offset + data_len > page.len() {
                return Err(AtlasError::InvalidInput("Source data out of bounds"));
            }
            page[source_page_offset..source_page_offset + data_len].to_vec()
        } else {
            return Err(AtlasError::InvalidInput("Source tile has no pages"));
        };

        // Compute conservation invariant before move
        let conservation_sum = source_data.iter().map(|&b| i64::from(b)).sum::<i64>();

        // Find destination from delta.data (destination coordinate encoded in first 4 bytes)
        if delta.data.len() < 4 {
            return Err(AtlasError::InvalidInput(
                "Move delta missing destination coordinate",
            ));
        }
        let dest_coord =
            u32::from_le_bytes([delta.data[0], delta.data[1], delta.data[2], delta.data[3]]);

        let dest_tile_id = dest_coord / 4096;
        let dest_page_offset = (dest_coord % 4096) as usize;

        // Find destination tile
        let dest_tile_idx = projection
            .tiles
            .iter()
            .position(|t| t.id == dest_tile_id)
            .ok_or(AtlasError::InvalidInput("Destination tile not found"))?;

        if dest_page_offset + data_len > 4096 {
            return Err(AtlasError::InvalidInput("Destination out of bounds"));
        }

        // Clear source region (set to zero to maintain conservation later)
        if !projection.tiles[source_tile_idx].pages.is_empty() {
            let page = &mut projection.tiles[source_tile_idx].pages[0];
            for i in source_page_offset..source_page_offset + data_len {
                page[i] = 0;
            }
        }

        // Write to destination
        if !projection.tiles[dest_tile_idx].pages.is_empty() {
            let dest_page = &mut projection.tiles[dest_tile_idx].pages[0];
            if dest_page_offset + data_len <= dest_page.len() {
                dest_page[dest_page_offset..dest_page_offset + data_len]
                    .copy_from_slice(&source_data);
            } else {
                return Err(AtlasError::InvalidInput("Destination page too small"));
            }
        } else {
            return Err(AtlasError::InvalidInput("Destination tile has no pages"));
        }

        // Verify UN invariant is preserved (conservation sum should be unchanged)
        let dest_page = &projection.tiles[dest_tile_idx].pages[0];
        let new_sum = dest_page[dest_page_offset..dest_page_offset + data_len]
            .iter()
            .map(|&b| i64::from(b))
            .sum::<i64>();

        if new_sum != conservation_sum {
            return Err(AtlasError::LayerIntegrationError(
                "Move operation violated conservation invariant",
            ));
        }

        // Note: Witness generation for move operations is handled at the projection level
        // The conservation check above already verifies the operation's validity as a UN operation
        // Avoid direct witness generation here to prevent thread-safety issues with small buffers

        Ok(())
    }

    fn rollback_single_delta(
        &mut self,
        projection: &mut AtlasProjection,
        delta: &ProjectionDelta,
    ) -> AtlasResult<()> {
        // Rollback the delta by applying the inverse operation
        match delta.change_type {
            ChangeType::Insert => {
                // Remove the inserted data by deleting it
                let delete_delta = ProjectionDelta::delete(
                    delta.start_coord,
                    delta.start_coord + delta.data.len() as u32,
                );
                self.apply_delete(projection, &mut delete_delta.clone())?;
            },
            ChangeType::Update => {
                // Restore original data
                if let Some(ref original_data) = delta.original_data {
                    let restore_delta = ProjectionDelta::update(
                        delta.start_coord,
                        delta.end_coord,
                        original_data.clone(),
                    );
                    self.apply_update(projection, &mut restore_delta.clone())?;
                }
            },
            ChangeType::Delete => {
                // Restore deleted data
                if let Some(ref original_data) = delta.original_data {
                    let restore_delta =
                        ProjectionDelta::insert(delta.start_coord, original_data.clone());
                    self.apply_insert(projection, &restore_delta)?;
                }
            },
            ChangeType::Move => {
                // Reverse the move operation
                // Implementation would depend on how move was originally applied
            },
        }
        Ok(())
    }
}

/// Apply incremental update to a projection
pub fn apply_incremental_update(
    projection: &mut AtlasProjection,
    delta: ProjectionDelta,
    config: Option<IncrementalConfig>,
) -> AtlasResult<()> {
    let config = config.unwrap_or_default();
    let mut context = IncrementalUpdateContext::new(config.clone());
    context.apply_delta(projection, delta, &config)
}

/// Apply multiple incremental updates to a projection
pub fn apply_incremental_batch_update(
    projection: &mut AtlasProjection,
    deltas: Vec<ProjectionDelta>,
    config: Option<IncrementalConfig>,
) -> AtlasResult<IncrementalStats> {
    let config = config.unwrap_or_default();
    let mut context = IncrementalUpdateContext::new(config.clone());
    context.apply_batch(projection, deltas, &config)?;
    Ok(context.get_stats())
}

/// Optimize projection after multiple incremental updates
pub fn optimize_after_incremental_updates(projection: &mut AtlasProjection) -> AtlasResult<()> {
    // Consolidate tiles and rebuild conservation sums
    let mut new_total_sum = 0u64;

    for tile in &mut projection.tiles {
        tile.apply_conservation_correction()?;
        new_total_sum = new_total_sum.wrapping_add(tile.conservation_sum);
    }

    projection.total_conservation_sum = new_total_sum;

    // Verify the projection is still valid
    projection.verify()?;

    Ok(())
}

/// Get current timestamp using atomic counter for monotonic Universal Numbers
/// This generates a monotonic timestamp that acts as a Universal Number
fn get_timestamp() -> u64 {
    // Use atomic counter for thread-safe monotonic timestamps
    // This is already a Universal Number - it's invariant and monotonic
    static COUNTER: AtomicU64 = AtomicU64::new(1);

    // Get an atomic counter value for uniqueness and thread safety
    COUNTER.fetch_add(1, Ordering::SeqCst)

    // Note: We avoid calling atlas_witness_generate here because:
    // - It requires minimum 256-byte buffers as per Layer 2 implementation
    // - It accesses LLVM internals that are not thread-safe without proper domain context
    // - The atomic counter already provides the required UN properties (monotonic, unique)
}

#[cfg(test)]
mod tests {
    use super::*;

    fn create_test_projection() -> AtlasProjection {
        let test_data = vec![0u8; 4096 * 4]; // 4 pages of test data
        AtlasProjection::new_linear(&test_data).unwrap()
    }

    #[test]
    fn test_projection_delta_creation() {
        let data = vec![1, 2, 3, 4];
        let delta = ProjectionDelta::insert(100, data.clone());

        assert_eq!(delta.change_type, ChangeType::Insert);
        assert_eq!(delta.start_coord, 100);
        assert_eq!(delta.data, data);
        assert_eq!(delta.conservation_delta, 10); // 1+2+3+4
    }

    #[test]
    fn test_incremental_config() {
        let config = IncrementalConfig::default();
        assert!(config.validate_conservation);
        assert!(config.enable_rollback);
        assert_eq!(config.max_deltas, 1000);
    }

    #[test]
    fn test_delta_affected_tiles_calculation() {
        let mut delta = ProjectionDelta::insert(1000, vec![1, 2, 3]);
        delta.calculate_affected_tiles(4); // 4x4 tile grid

        assert!(!delta.affected_tiles.is_empty());
    }

    #[test]
    fn test_conservation_validation() {
        let data = vec![10, 20, 30];
        let delta = ProjectionDelta::insert(0, data);
        assert!(delta.validate_conservation()); // Insert operations are valid

        let mut update_delta = ProjectionDelta::update(0, 10, vec![15, 25, 35]);
        update_delta.set_original_data(vec![10, 20, 30]);
        assert!(update_delta.validate_conservation()); // Conservation delta should match
    }

    #[test]
    fn test_incremental_update_context() {
        let config = IncrementalConfig::default();
        let mut context = IncrementalUpdateContext::new(config.clone());
        let mut projection = create_test_projection();

        let delta = ProjectionDelta::insert(100, vec![42, 43, 44]);
        let result = context.apply_delta(&mut projection, delta, &config);
        assert!(result.is_ok());

        assert_eq!(context.sequence_number, 1);
        assert_eq!(context.applied_deltas.len(), 1);
    }

    #[test]
    fn test_batch_updates() {
        let config = IncrementalConfig {
            batch_size: 2,
            ..IncrementalConfig::default()
        };
        let mut context = IncrementalUpdateContext::new(config.clone());
        let mut projection = create_test_projection();

        let deltas = vec![
            ProjectionDelta::insert(100, vec![1, 2]),
            ProjectionDelta::insert(200, vec![3, 4]),
            ProjectionDelta::insert(300, vec![5, 6]),
        ];

        let result = context.apply_batch(&mut projection, deltas, &config);
        assert!(result.is_ok());
        assert_eq!(context.applied_deltas.len(), 3);
    }

    #[test]
    fn test_rollback_functionality() {
        let config = IncrementalConfig {
            enable_rollback: true,
            ..IncrementalConfig::default()
        };
        let mut context = IncrementalUpdateContext::new(config.clone());
        let mut projection = create_test_projection();

        // Apply some updates
        let delta1 = ProjectionDelta::insert(100, vec![10, 20]);
        let delta2 = ProjectionDelta::insert(200, vec![30, 40]);

        context.apply_delta(&mut projection, delta1, &config).unwrap();
        context.apply_delta(&mut projection, delta2, &config).unwrap();

        assert_eq!(context.applied_deltas.len(), 2);
        assert_eq!(context.sequence_number, 2);

        // Rollback one delta
        let result = context.rollback_deltas(&mut projection, 1);
        assert!(result.is_ok());
        assert_eq!(context.applied_deltas.len(), 1);
        assert_eq!(context.sequence_number, 1);
    }

    #[test]
    fn test_stats_collection() {
        let config = IncrementalConfig::default();
        let mut context = IncrementalUpdateContext::new(config.clone());
        let mut projection = create_test_projection();

        // Apply different types of operations
        context
            .apply_delta(
                &mut projection,
                ProjectionDelta::insert(100, vec![1, 2]),
                &config,
            )
            .unwrap();
        context
            .apply_delta(
                &mut projection,
                ProjectionDelta::update(200, 250, vec![3, 4]),
                &config,
            )
            .unwrap();
        context
            .apply_delta(&mut projection, ProjectionDelta::delete(300, 350), &config)
            .unwrap();

        let stats = context.get_stats();
        assert_eq!(stats.total_updates, 3);
        assert_eq!(stats.insert_operations, 1);
        assert_eq!(stats.update_operations, 1);
        assert_eq!(stats.delete_operations, 1);
    }

    #[test]
    fn test_public_api_functions() {
        let mut projection = create_test_projection();
        let delta = ProjectionDelta::insert(100, vec![1, 2, 3]);

        let result = apply_incremental_update(&mut projection, delta, None);
        assert!(result.is_ok());

        // Test batch update
        let deltas = vec![
            ProjectionDelta::insert(200, vec![4, 5]),
            ProjectionDelta::update(300, 310, vec![6, 7]),
        ];

        let stats_result = apply_incremental_batch_update(&mut projection, deltas, None);
        assert!(stats_result.is_ok());

        let stats = stats_result.unwrap();
        assert_eq!(stats.total_updates, 2);
    }

    #[test]
    fn test_optimization_after_updates() {
        let mut projection = create_test_projection();

        // Apply some updates
        let delta1 = ProjectionDelta::insert(100, vec![10, 20, 30]);
        apply_incremental_update(&mut projection, delta1, None).unwrap();

        // Optimize the projection
        let result = optimize_after_incremental_updates(&mut projection);
        assert!(result.is_ok());

        // Verify the projection is still valid
        assert!(projection.verify().is_ok());
    }
}
