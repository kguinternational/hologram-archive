//! Critical Mathematical Invariants for Layer 4 Manifold Operations
//!
//! This module implements the core mathematical invariants that ensure the correctness
//! and consistency of Layer 4 operations within the Atlas Hologram system:
//!
//! 1. C768 triple-cycle validation (768-step conservation closure)
//! 2. Φ bijection verification for boundary encoding  
//! 3. Normal Form rules for `R96_FOURIER` deterministic reconstruction
//! 4. Klein orbit alignment validation {0,1,48,49}
//! 5. Conservation budget tracking (mod-96)
//! 6. Failure-closed semantics

use crate::error::*;

#[cfg(not(feature = "std"))]
use alloc::vec::Vec;
#[cfg(feature = "std")]
use std::vec::Vec;

// Type aliases to reduce complexity
type CycleState = (usize, bool, Vec<u64>);
type BudgetState = (u64, Vec<u64>, bool);
type BijectionResult = (usize, usize);
type StatsTuple = (usize, usize, f64);
type ValidationStatus = (bool, bool, bool, bool);

/// C768 cycle length constant: 16 × 48 = 3 × 256
pub const C768_CYCLE_LENGTH: usize = 768;

/// Klein orbit privileged positions for alignment
pub const KLEIN_ORBIT_POSITIONS: [usize; 4] = [0, 1, 48, 49];

/// Conservation modulus for budget tracking
pub const CONSERVATION_MODULUS: u64 = 96;

/// Maximum page size for boundary encoding
pub const BOUNDARY_PAGE_SIZE: usize = 256;

/// C768 Triple-Cycle State Tracker
///
/// Tracks a 768-step cycle that must close to validate conservation.
/// The cycle represents: 16 × 48 = 3 × 256, ensuring alignment with:
/// - R96 resonance classes (96 = 2 × 48)
/// - Page boundaries (256 bytes)
/// - Klein orbit structure (4 × 192 = 768)
#[derive(Debug, Clone)]
pub struct C768CycleTracker {
    /// Current position in the 768-step cycle
    position: usize,
    /// Accumulated state at each step
    state_accumulator: Vec<u64>,
    /// Start state for cycle closure validation
    initial_state: u64,
    /// Whether the current cycle is complete and valid
    cycle_closed: bool,
    /// Number of complete cycles processed
    completed_cycles: u64,
}

impl C768CycleTracker {
    /// Create a new C768 cycle tracker
    pub fn new() -> Self {
        Self {
            position: 0,
            state_accumulator: vec![0; C768_CYCLE_LENGTH],
            initial_state: 0,
            cycle_closed: false,
            completed_cycles: 0,
        }
    }

    /// Initialize cycle with starting state
    pub fn initialize(&mut self, initial_state: u64) -> AtlasResult<()> {
        self.initial_state = initial_state;
        self.position = 0;
        self.cycle_closed = false;
        self.state_accumulator.fill(0);
        self.state_accumulator[0] = initial_state;
        Ok(())
    }

    /// Advance one step in the cycle
    pub fn step(&mut self, state_delta: u64) -> AtlasResult<()> {
        if self.position >= C768_CYCLE_LENGTH {
            return Err(AtlasError::InvalidInput("cycle already completed"));
        }

        self.position += 1;

        if self.position < C768_CYCLE_LENGTH {
            // Accumulate state with conservation properties
            let prev_state = self.state_accumulator[self.position - 1];
            let new_state = prev_state.wrapping_add(state_delta);
            self.state_accumulator[self.position] = new_state;
        } else {
            // Final step - check closure
            self.validate_cycle_closure(state_delta)?;
        }

        Ok(())
    }

    /// Validate that the 768-step cycle closes properly
    fn validate_cycle_closure(&mut self, final_delta: u64) -> AtlasResult<()> {
        if self.position != C768_CYCLE_LENGTH {
            return Err(AtlasError::NumericalError(
                "invalid cycle position for closure",
            ));
        }

        // Compute final state
        let prev_state = self.state_accumulator[C768_CYCLE_LENGTH - 1];
        let final_state = prev_state.wrapping_add(final_delta);

        // Check if cycle closes: final state should equal initial state (mod conservation)
        let state_difference = final_state.wrapping_sub(self.initial_state);

        // Triple-cycle closure condition: difference must be divisible by 3 × conservation modulus
        if state_difference % (3 * CONSERVATION_MODULUS) != 0 {
            return Err(AtlasError::LayerIntegrationError(
                "C768 triple-cycle failed to close",
            ));
        }

        // Validate Klein orbit alignment at privileged positions
        self.validate_klein_orbit_alignment()?;

        self.cycle_closed = true;
        self.completed_cycles += 1;
        Ok(())
    }

    /// Validate Klein orbit alignment at privileged positions {0, 1, 48, 49}
    fn validate_klein_orbit_alignment(&self) -> AtlasResult<()> {
        for &pos in &KLEIN_ORBIT_POSITIONS {
            if pos >= self.state_accumulator.len() {
                return Err(AtlasError::TopologyError(
                    "klein orbit position out of bounds",
                ));
            }

            let state_value = self.state_accumulator[pos];

            // Klein orbit alignment: privileged positions must satisfy specific symmetry
            // Position 0: must be initial state
            // Position 1: must be close to initial (within orbit distance)
            // Positions 48, 49: must form conjugate pair (48 = 96/2, 49 = 48+1)
            match pos {
                0 => {
                    if state_value != self.initial_state {
                        return Err(AtlasError::TopologyError(
                            "klein orbit position 0 misaligned",
                        ));
                    }
                },
                1 => {
                    let orbit_distance = state_value.wrapping_sub(self.initial_state);
                    if orbit_distance % KLEIN_ORBIT_POSITIONS.len() as u64 != 1 {
                        return Err(AtlasError::TopologyError(
                            "klein orbit position 1 misaligned",
                        ));
                    }
                },
                48 => {
                    // Position 48 should align with R96 midpoint (48 = 96/2)
                    if state_value % CONSERVATION_MODULUS != 48 {
                        return Err(AtlasError::TopologyError(
                            "klein orbit position 48 misaligned",
                        ));
                    }
                },
                49 => {
                    // Position 49 should be conjugate to 48
                    let pos_48_state = self.state_accumulator[48];
                    let conjugate_distance = state_value.wrapping_sub(pos_48_state);
                    if conjugate_distance % CONSERVATION_MODULUS != 1 {
                        return Err(AtlasError::TopologyError(
                            "klein orbit position 49 misaligned",
                        ));
                    }
                },
                _ => return Err(AtlasError::TopologyError("invalid klein orbit position")),
            }
        }

        Ok(())
    }

    /// Check if current cycle is closed and valid
    pub fn is_cycle_closed(&self) -> bool {
        self.cycle_closed && self.position == C768_CYCLE_LENGTH
    }

    /// Get number of completed cycles
    pub fn completed_cycles(&self) -> u64 {
        self.completed_cycles
    }

    /// Reset for next cycle
    pub fn reset(&mut self) {
        self.position = 0;
        self.cycle_closed = false;
        self.state_accumulator.fill(0);
    }

    /// Process a step with data chunk
    pub fn process_step(&mut self, data_chunk: &[u8]) -> AtlasResult<()> {
        if data_chunk.is_empty() {
            return Err(AtlasError::InvalidInput("empty data chunk"));
        }

        // Calculate state delta from data chunk
        let state_delta = data_chunk.iter().map(|&b| u64::from(b)).sum::<u64>();
        self.step(state_delta)
    }

    /// Reset cycle to initial state
    pub fn reset_cycle(&mut self) {
        self.position = 0;
        self.cycle_closed = false;
        self.state_accumulator.fill(0);
    }

    /// Get current position in cycle
    pub fn get_position(&self) -> usize {
        self.position
    }

    /// Get current cycle state for diagnostics
    pub fn get_cycle_state(&self) -> CycleState {
        (
            self.position,
            self.cycle_closed,
            self.state_accumulator.clone(),
        )
    }
}

impl Default for C768CycleTracker {
    fn default() -> Self {
        Self::new()
    }
}

/// Klein Orbit Aligner for privileged position validation
///
/// Validates alignment at Klein orbit positions {0, 1, 48, 49} within the R96 classification system.
#[derive(Debug, Clone)]
pub struct KleinOrbitAligner {
    /// Alignment state at privileged positions
    position_states: Vec<Option<u64>>,
    /// Whether complete alignment has been verified
    alignment_verified: bool,
}

impl KleinOrbitAligner {
    /// Create new Klein orbit aligner
    #[allow(clippy::cast_possible_truncation)] // CONSERVATION_MODULUS is 96, well within usize range
    pub fn new() -> Self {
        Self {
            position_states: vec![None; CONSERVATION_MODULUS as usize],
            alignment_verified: false,
        }
    }

    /// Check alignment at specific position
    pub fn check_alignment(&mut self, position: usize, data_chunk: &[u8]) -> AtlasResult<()> {
        if !KLEIN_ORBIT_POSITIONS.contains(&position) {
            return Ok(()); // Only check privileged positions
        }

        if data_chunk.is_empty() {
            return Err(AtlasError::InvalidInput("empty data chunk for alignment"));
        }

        // Calculate alignment state from data
        let alignment_state =
            data_chunk.iter().map(|&b| u64::from(b)).sum::<u64>() % CONSERVATION_MODULUS;
        self.position_states[position] = Some(alignment_state);

        Ok(())
    }

    /// Verify complete alignment across all Klein orbit positions
    pub fn verify_complete_alignment(&mut self) -> AtlasResult<()> {
        // Check that all privileged positions have been recorded
        for &pos in &KLEIN_ORBIT_POSITIONS {
            if self.position_states[pos].is_none() {
                return Err(AtlasError::TopologyError(
                    "Klein orbit position not recorded",
                ));
            }
        }

        // Validate Klein orbit symmetry properties
        let pos_0 = self.position_states[0].unwrap();
        let pos_1 = self.position_states[1].unwrap();
        let pos_48 = self.position_states[48].unwrap();
        let pos_49 = self.position_states[49].unwrap();

        // Position 1 should be close to position 0 (orbit distance = 1)
        if (pos_1.wrapping_sub(pos_0)) % CONSERVATION_MODULUS != 1 {
            return Err(AtlasError::TopologyError(
                "Klein orbit positions 0,1 not properly aligned",
            ));
        }

        // Position 49 should be conjugate to position 48 (distance = 1)
        if (pos_49.wrapping_sub(pos_48)) % CONSERVATION_MODULUS != 1 {
            return Err(AtlasError::TopologyError(
                "Klein orbit positions 48,49 not properly aligned",
            ));
        }

        self.alignment_verified = true;
        Ok(())
    }

    /// Check if alignment verification is complete
    pub fn is_aligned(&self) -> bool {
        self.alignment_verified
    }
}

impl Default for KleinOrbitAligner {
    fn default() -> Self {
        Self::new()
    }
}

/// Φ Bijection Verifier for Boundary Encoding
///
/// Verifies that the boundary encoding function Φ(page, offset) = page*256 + offset
/// maintains bijective properties for proper manifold boundary conditions.
#[derive(Debug, Clone)]
pub struct PhiBijectionVerifier {
    /// Mapping table to track used encodings
    encoding_table: Vec<bool>,
    /// Maximum page number supported
    max_pages: usize,
    /// Current verification state
    verification_state: BijectionState,
}

/// State of bijection verification process
#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub enum BijectionState {
    /// Not initialized
    Uninitialized,
    /// Currently recording mappings
    Recording,
    /// Verification complete and valid
    Valid,
    /// Verification failed - not bijective
    Invalid,
}

impl PhiBijectionVerifier {
    /// Create new bijection verifier with capacity for given number of pages
    pub fn new(max_pages: usize) -> AtlasResult<Self> {
        if max_pages == 0 || max_pages > u32::MAX as usize / BOUNDARY_PAGE_SIZE {
            return Err(AtlasError::InvalidInput(
                "invalid max_pages for bijection verifier",
            ));
        }

        let total_encodings = max_pages * BOUNDARY_PAGE_SIZE;

        Ok(Self {
            encoding_table: vec![false; total_encodings],
            max_pages,
            verification_state: BijectionState::Uninitialized,
        })
    }

    /// Initialize verification process
    pub fn initialize(&mut self) -> AtlasResult<()> {
        self.encoding_table.fill(false);
        self.verification_state = BijectionState::Recording;
        Ok(())
    }

    /// Record a Φ encoding: Φ(page, offset) = page*256 + offset
    pub fn record_encoding(&mut self, page: usize, offset: usize) -> AtlasResult<()> {
        if self.verification_state != BijectionState::Recording {
            return Err(AtlasError::InvalidInput("verifier not in recording state"));
        }

        if page >= self.max_pages {
            return Err(AtlasError::InvalidInput("page exceeds maximum"));
        }

        if offset >= BOUNDARY_PAGE_SIZE {
            return Err(AtlasError::InvalidInput("offset exceeds page boundary"));
        }

        // Compute Φ encoding
        let phi_encoding = self.phi_function(page, offset);

        // Check for collision (non-bijective mapping)
        if phi_encoding >= self.encoding_table.len() {
            return Err(AtlasError::InvalidInput("phi encoding out of bounds"));
        }

        if self.encoding_table[phi_encoding] {
            self.verification_state = BijectionState::Invalid;
            return Err(AtlasError::LayerIntegrationError(
                "phi bijection violation detected",
            ));
        }

        // Record this encoding as used
        self.encoding_table[phi_encoding] = true;
        Ok(())
    }

    /// The Φ bijection function: Φ(page, offset) = page*256 + offset
    pub fn phi_function(&self, page: usize, offset: usize) -> usize {
        page * BOUNDARY_PAGE_SIZE + offset
    }

    /// Verify inverse function: given encoding, recover (page, offset)
    pub fn phi_inverse(&self, encoding: usize) -> AtlasResult<BijectionResult> {
        if encoding >= self.max_pages * BOUNDARY_PAGE_SIZE {
            return Err(AtlasError::InvalidInput("encoding exceeds maximum range"));
        }

        let page = encoding / BOUNDARY_PAGE_SIZE;
        let offset = encoding % BOUNDARY_PAGE_SIZE;

        // Verify bijection: Φ(page, offset) should equal original encoding
        if self.phi_function(page, offset) != encoding {
            return Err(AtlasError::NumericalError("phi inverse function failed"));
        }

        Ok((page, offset))
    }

    /// Complete verification and validate bijection properties
    pub fn finalize_verification(&mut self) -> AtlasResult<()> {
        if self.verification_state != BijectionState::Recording {
            return Err(AtlasError::InvalidInput("verifier not in recording state"));
        }

        // Check for any violations already detected
        if self.verification_state == BijectionState::Invalid {
            return Err(AtlasError::LayerIntegrationError(
                "bijection verification failed",
            ));
        }

        // Additional integrity checks
        let used_encodings = self.encoding_table.iter().filter(|&&used| used).count();

        // Bijection is valid even if not all encodings are used (injection property)
        // but we should have at least some valid mappings
        if used_encodings == 0 {
            return Err(AtlasError::LayerIntegrationError(
                "no valid encodings recorded",
            ));
        }

        self.verification_state = BijectionState::Valid;
        Ok(())
    }

    /// Encode using Phi function: Φ(page, offset) = page*256 + offset
    pub fn phi_encode(&self, page: usize, offset: usize) -> usize {
        self.phi_function(page, offset)
    }

    /// Mark an encoding as used (for bijection tracking)
    pub fn mark_encoding_used(&mut self, encoding: usize) -> AtlasResult<()> {
        if encoding >= self.encoding_table.len() {
            return Err(AtlasError::InvalidInput("encoding exceeds maximum range"));
        }

        if self.encoding_table[encoding] {
            return Err(AtlasError::LayerIntegrationError(
                "encoding already used - bijection violation",
            ));
        }

        self.encoding_table[encoding] = true;
        Ok(())
    }

    /// Verify bijection properties
    pub fn verify_bijection(&self) -> AtlasResult<()> {
        if self.verification_state != BijectionState::Valid
            && self.verification_state != BijectionState::Recording
        {
            return Err(AtlasError::LayerIntegrationError(
                "bijection verifier not in valid state",
            ));
        }
        Ok(())
    }

    /// Check if bijection verification is complete and valid
    pub fn is_valid(&self) -> bool {
        self.verification_state == BijectionState::Valid
    }

    /// Get verification statistics
    pub fn get_statistics(&self) -> StatsTuple {
        let used_count = self.encoding_table.iter().filter(|&&used| used).count();
        let total_capacity = self.encoding_table.len();
        let utilization = used_count as f64 / total_capacity as f64;

        (used_count, total_capacity, utilization)
    }
}

/// Conservation Budget Tracker (mod-96)
///
/// Tracks conservation budget across manifold operations with mod-96 arithmetic
/// to ensure conservation laws are maintained throughout transformations.
#[derive(Debug, Clone)]
pub struct ConservationBudgetTracker {
    /// Current total budget (maintained mod-96)
    total_budget: u64,
    /// Budget allocations by category
    category_budgets: Vec<u64>,
    /// Transaction history for audit
    transaction_history: Vec<ConservationTransaction>,
    /// Whether budget is balanced (sum % 96 == 0)
    is_balanced: bool,
}

/// Conservation transaction record
#[derive(Debug, Clone, Copy)]
pub struct ConservationTransaction {
    /// Transaction type
    pub transaction_type: TransactionType,
    /// Amount transferred (always mod-96)
    pub amount: u64,
    /// Source category (if applicable)
    pub source_category: Option<usize>,
    /// Target category (if applicable)
    pub target_category: Option<usize>,
    /// Transaction sequence number
    pub sequence: u64,
}

/// Types of conservation transactions
#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub enum TransactionType {
    /// Initial allocation
    Initialize,
    /// Budget allocation to category
    Allocate,
    /// Budget transfer between categories
    Transfer,
    /// Budget consumption/use
    Consume,
    /// Budget restoration/refund
    Restore,
    /// Validation/audit operation
    Validate,
}

impl ConservationBudgetTracker {
    /// Create new conservation budget tracker
    pub fn new(num_categories: usize) -> Self {
        Self {
            total_budget: 0,
            category_budgets: vec![0; num_categories],
            transaction_history: Vec::new(),
            is_balanced: true,
        }
    }

    /// Initialize total conservation budget
    pub fn initialize_budget(&mut self, initial_budget: u64) -> AtlasResult<()> {
        // Clear previous state when reinitializing
        self.category_budgets.fill(0);
        self.transaction_history.clear();

        // Keep the budget as given, don't normalize to mod-96 here
        // The normalization happens during allocation operations
        self.total_budget = initial_budget;

        self.record_transaction(ConservationTransaction {
            transaction_type: TransactionType::Initialize,
            amount: self.total_budget,
            source_category: None,
            target_category: None,
            sequence: self.transaction_history.len() as u64,
        });

        self.validate_conservation()?;
        Ok(())
    }

    /// Allocate budget to a specific category
    pub fn allocate_to_category(&mut self, category: usize, amount: u64) -> AtlasResult<()> {
        if category >= self.category_budgets.len() {
            return Err(AtlasError::InvalidInput("category index out of bounds"));
        }

        // Normalize amount to mod-96
        let normalized_amount = amount % CONSERVATION_MODULUS;

        // Check if enough budget is available
        let allocated_total: u64 = self.category_budgets.iter().sum();
        let available = if allocated_total <= self.total_budget {
            self.total_budget - allocated_total
        } else {
            0
        };

        if normalized_amount > available {
            return Err(AtlasError::LayerIntegrationError(
                "insufficient conservation budget",
            ));
        }

        self.category_budgets[category] =
            (self.category_budgets[category] + normalized_amount) % CONSERVATION_MODULUS;

        self.record_transaction(ConservationTransaction {
            transaction_type: TransactionType::Allocate,
            amount: normalized_amount,
            source_category: None,
            target_category: Some(category),
            sequence: self.transaction_history.len() as u64,
        });

        self.validate_conservation()?;
        Ok(())
    }

    /// Transfer budget between categories
    pub fn transfer_budget(
        &mut self,
        source_category: usize,
        target_category: usize,
        amount: u64,
    ) -> AtlasResult<()> {
        if source_category >= self.category_budgets.len()
            || target_category >= self.category_budgets.len()
        {
            return Err(AtlasError::InvalidInput("category index out of bounds"));
        }

        if source_category == target_category {
            return Err(AtlasError::InvalidInput(
                "source and target categories must be different",
            ));
        }

        let normalized_amount = amount % CONSERVATION_MODULUS;

        // Check source has sufficient budget
        if self.category_budgets[source_category] < normalized_amount {
            return Err(AtlasError::LayerIntegrationError(
                "insufficient budget in source category",
            ));
        }

        // Perform transfer
        self.category_budgets[source_category] =
            (self.category_budgets[source_category] + CONSERVATION_MODULUS - normalized_amount)
                % CONSERVATION_MODULUS;
        self.category_budgets[target_category] =
            (self.category_budgets[target_category] + normalized_amount) % CONSERVATION_MODULUS;

        self.record_transaction(ConservationTransaction {
            transaction_type: TransactionType::Transfer,
            amount: normalized_amount,
            source_category: Some(source_category),
            target_category: Some(target_category),
            sequence: self.transaction_history.len() as u64,
        });

        self.validate_conservation()?;
        Ok(())
    }

    /// Consume budget from a category
    pub fn consume_budget(&mut self, category: usize, amount: u64) -> AtlasResult<()> {
        if category >= self.category_budgets.len() {
            return Err(AtlasError::InvalidInput("category index out of bounds"));
        }

        let normalized_amount = amount % CONSERVATION_MODULUS;

        if self.category_budgets[category] < normalized_amount {
            return Err(AtlasError::LayerIntegrationError(
                "insufficient budget for consumption",
            ));
        }

        self.category_budgets[category] = (self.category_budgets[category] + CONSERVATION_MODULUS
            - normalized_amount)
            % CONSERVATION_MODULUS;

        self.record_transaction(ConservationTransaction {
            transaction_type: TransactionType::Consume,
            amount: normalized_amount,
            source_category: Some(category),
            target_category: None,
            sequence: self.transaction_history.len() as u64,
        });

        self.validate_conservation()?;
        Ok(())
    }

    /// Validate conservation law: budget allocation is consistent
    fn validate_conservation(&mut self) -> AtlasResult<()> {
        let allocated_total: u64 = self.category_budgets.iter().sum::<u64>();
        let expected_total = self.total_budget;

        // For consumption operations, we need to track the consumed amount separately
        let mut consumed_total = 0u64;
        for transaction in &self.transaction_history {
            if transaction.transaction_type == TransactionType::Consume {
                consumed_total = (consumed_total + transaction.amount) % CONSERVATION_MODULUS;
            }
        }

        // Conservation law: allocated budget should not exceed total budget
        // and the system should maintain modular arithmetic consistency
        self.is_balanced = allocated_total <= expected_total;

        if !self.is_balanced {
            return Err(AtlasError::LayerIntegrationError(
                "conservation law violation: allocated budget exceeds total",
            ));
        }

        Ok(())
    }

    /// Record transaction in history
    fn record_transaction(&mut self, transaction: ConservationTransaction) {
        self.transaction_history.push(transaction);
    }

    /// Check if budget is balanced (conservation law satisfied)
    pub fn is_balanced(&self) -> bool {
        self.is_balanced
    }

    /// Get current budget state
    pub fn get_budget_state(&self) -> BudgetState {
        (
            self.total_budget % CONSERVATION_MODULUS,
            self.category_budgets.clone(),
            self.is_balanced,
        )
    }

    /// Get available budget for a category
    pub fn get_available_budget(&self, category: usize) -> u64 {
        if category >= self.category_budgets.len() {
            return 0;
        }
        self.category_budgets[category]
    }

    /// Get transaction history
    pub fn get_transaction_history(&self) -> &[ConservationTransaction] {
        &self.transaction_history
    }

    /// Force revalidation of conservation
    pub fn force_revalidation(&mut self) -> AtlasResult<()> {
        self.validate_conservation()
    }
}

/// Failure-Closed Semantics Enforcer
///
/// Ensures that operations fail safely and maintain system integrity
/// when invariants are violated or errors occur.
#[derive(Debug, Clone)]
pub struct FailureClosedSemanticsEnforcer {
    /// Current system state
    system_state: SystemState,
    /// Error accumulator
    error_history: Vec<AtlasError>,
    /// Recovery checkpoint
    checkpoint_state: Option<SystemCheckpoint>,
    /// Failure count threshold before system lockdown
    failure_threshold: usize,
}

/// System operational state
#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub enum SystemState {
    /// Normal operation
    Normal,
    /// Warning state - some invariants stressed
    Warning,
    /// Error state - invariant violations detected
    Error,
    /// Recovery state - attempting to restore integrity
    Recovery,
    /// Locked state - too many failures, system protection engaged
    Locked,
}

/// System checkpoint for recovery
#[derive(Debug, Clone)]
pub struct SystemCheckpoint {
    /// State timestamp
    pub timestamp: u64,
    /// C768 cycle state
    pub c768_state: CycleState,
    /// Bijection verification state
    pub bijection_valid: bool,
    /// Conservation budget state
    pub budget_state: BudgetState,
    /// System state at checkpoint
    pub system_state: SystemState,
}

impl FailureClosedSemanticsEnforcer {
    /// Create new failure-closed semantics enforcer
    pub fn new(failure_threshold: usize) -> Self {
        Self {
            system_state: SystemState::Normal,
            error_history: Vec::new(),
            checkpoint_state: None,
            failure_threshold,
        }
    }

    /// Record a system error
    pub fn record_error(&mut self, error: &AtlasError) -> AtlasResult<()> {
        self.error_history.push(error.clone());

        // Update system state based on error severity and count
        self.update_system_state(error)?;

        // If too many failures, enter locked state
        if self.error_history.len() >= self.failure_threshold {
            self.system_state = SystemState::Locked;
            return Err(AtlasError::LayerIntegrationError(
                "system locked due to excessive failures",
            ));
        }

        Ok(())
    }

    /// Update system state based on error
    fn update_system_state(&mut self, error: &AtlasError) -> AtlasResult<()> {
        match error {
            AtlasError::LayerIntegrationError(_) | AtlasError::TopologyError(_) => {
                // Critical errors move to error state
                if self.system_state == SystemState::Normal {
                    self.system_state = SystemState::Warning;
                } else if self.system_state == SystemState::Warning {
                    self.system_state = SystemState::Error;
                }
            },
            AtlasError::NumericalError(_) | AtlasError::MatrixError(_) => {
                // Numerical errors are warnings unless already in error state
                if self.system_state == SystemState::Normal {
                    self.system_state = SystemState::Warning;
                }
            },
            _ => {
                // Other errors don't change state immediately
            },
        }

        Ok(())
    }

    /// Create system checkpoint
    pub fn create_checkpoint(
        &mut self,
        c768_tracker: &C768CycleTracker,
        bijection_verifier: &PhiBijectionVerifier,
        budget_tracker: &ConservationBudgetTracker,
    ) -> AtlasResult<()> {
        self.checkpoint_state = Some(SystemCheckpoint {
            timestamp: self.error_history.len() as u64, // Use error count as simple timestamp
            c768_state: c768_tracker.get_cycle_state(),
            bijection_valid: bijection_verifier.is_valid(),
            budget_state: budget_tracker.get_budget_state(),
            system_state: self.system_state,
        });

        Ok(())
    }

    /// Attempt system recovery to checkpoint
    pub fn attempt_recovery(
        &mut self,
        c768_tracker: &mut C768CycleTracker,
        budget_tracker: &mut ConservationBudgetTracker,
    ) -> AtlasResult<()> {
        if self.system_state == SystemState::Locked {
            return Err(AtlasError::LayerIntegrationError(
                "system locked, recovery not allowed",
            ));
        }

        let checkpoint = self.checkpoint_state.as_ref().ok_or(AtlasError::InvalidInput(
            "no checkpoint available for recovery",
        ))?;

        self.system_state = SystemState::Recovery;

        // Restore C768 cycle state (simplified)
        c768_tracker.reset();
        if let Some(initial_state) = checkpoint.c768_state.2.first() {
            c768_tracker.initialize(*initial_state)?;
        }

        // Force budget revalidation
        budget_tracker.force_revalidation()?;

        // If recovery successful, return to warning state
        self.system_state = SystemState::Warning;
        Ok(())
    }

    /// Check if system can perform operations
    pub fn can_operate(&self) -> bool {
        !matches!(self.system_state, SystemState::Locked | SystemState::Error)
    }

    /// Get current system state
    pub fn get_system_state(&self) -> SystemState {
        self.system_state
    }

    /// Get error count
    pub fn get_error_count(&self) -> usize {
        self.error_history.len()
    }

    /// Reset system state (only if not locked)
    pub fn reset_state(&mut self) -> AtlasResult<()> {
        if self.system_state == SystemState::Locked {
            return Err(AtlasError::LayerIntegrationError(
                "cannot reset locked system",
            ));
        }

        self.system_state = SystemState::Normal;
        self.error_history.clear();
        Ok(())
    }
}

/// Comprehensive Invariant Validator
///
/// Orchestrates all invariant checks and maintains system integrity
pub struct InvariantValidator {
    /// C768 cycle tracker
    pub c768_tracker: C768CycleTracker,
    /// Phi bijection verifier
    pub phi_verifier: PhiBijectionVerifier,
    /// Conservation budget tracker
    pub budget_tracker: ConservationBudgetTracker,
    /// Failure semantics enforcer
    pub failure_enforcer: FailureClosedSemanticsEnforcer,
}

impl InvariantValidator {
    /// Create new invariant validator
    pub fn new(max_pages: usize, num_budget_categories: usize) -> AtlasResult<Self> {
        Ok(Self {
            c768_tracker: C768CycleTracker::new(),
            phi_verifier: PhiBijectionVerifier::new(max_pages)?,
            budget_tracker: ConservationBudgetTracker::new(num_budget_categories),
            failure_enforcer: FailureClosedSemanticsEnforcer::new(10), // Allow 10 failures before lockdown
        })
    }

    /// Validate all invariants comprehensively
    pub fn validate_all_invariants(&mut self) -> AtlasResult<()> {
        if !self.failure_enforcer.can_operate() {
            return Err(AtlasError::LayerIntegrationError(
                "system cannot operate due to failures",
            ));
        }

        // Check C768 cycle closure
        if !self.c768_tracker.is_cycle_closed() {
            let error = AtlasError::LayerIntegrationError("C768 cycle not properly closed");
            self.failure_enforcer.record_error(&error)?;
            return Err(error);
        }

        // Check Phi bijection validity
        if !self.phi_verifier.is_valid() {
            let error = AtlasError::LayerIntegrationError("Phi bijection verification failed");
            self.failure_enforcer.record_error(&error)?;
            return Err(error);
        }

        // Check conservation budget balance
        if !self.budget_tracker.is_balanced() {
            let error = AtlasError::LayerIntegrationError("conservation budget not balanced");
            self.failure_enforcer.record_error(&error)?;
            return Err(error);
        }

        Ok(())
    }

    /// Initialize all invariant validators
    pub fn initialize_all(&mut self, initial_state: u64, initial_budget: u64) -> AtlasResult<()> {
        self.c768_tracker.initialize(initial_state)?;
        self.phi_verifier.initialize()?;
        self.budget_tracker.initialize_budget(initial_budget)?;

        // Create initial checkpoint
        self.failure_enforcer.create_checkpoint(
            &self.c768_tracker,
            &self.phi_verifier,
            &self.budget_tracker,
        )?;

        Ok(())
    }

    /// Create comprehensive system checkpoint
    pub fn create_comprehensive_checkpoint(&mut self) -> AtlasResult<()> {
        self.failure_enforcer.create_checkpoint(
            &self.c768_tracker,
            &self.phi_verifier,
            &self.budget_tracker,
        )
    }

    /// Attempt comprehensive system recovery
    pub fn attempt_comprehensive_recovery(&mut self) -> AtlasResult<()> {
        self.failure_enforcer
            .attempt_recovery(&mut self.c768_tracker, &mut self.budget_tracker)
    }

    /// Get comprehensive validation status
    pub fn get_validation_status(&self) -> ValidationStatus {
        (
            self.c768_tracker.is_cycle_closed(),
            self.phi_verifier.is_valid(),
            self.budget_tracker.is_balanced(),
            self.failure_enforcer.can_operate(),
        )
    }
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn test_c768_cycle_tracker_creation() {
        let tracker = C768CycleTracker::new();
        assert_eq!(tracker.position, 0);
        assert!(!tracker.cycle_closed);
        assert_eq!(tracker.completed_cycles, 0);
    }

    #[test]
    fn test_c768_cycle_initialization() {
        let mut tracker = C768CycleTracker::new();
        assert!(tracker.initialize(12345).is_ok());
        assert_eq!(tracker.initial_state, 12345);
        assert_eq!(tracker.state_accumulator[0], 12345);
    }

    #[test]
    fn test_klein_orbit_positions_constant() {
        assert_eq!(KLEIN_ORBIT_POSITIONS, [0, 1, 48, 49]);
        // Verify positions are within valid range for C768 cycle
        for &pos in &KLEIN_ORBIT_POSITIONS {
            assert!(pos < C768_CYCLE_LENGTH);
        }
    }

    #[test]
    fn test_phi_bijection_verifier_creation() {
        let verifier = PhiBijectionVerifier::new(10);
        assert!(verifier.is_ok());
        let verifier = verifier.unwrap();
        assert_eq!(verifier.max_pages, 10);
        assert_eq!(verifier.verification_state, BijectionState::Uninitialized);
    }

    #[test]
    fn test_phi_function_correctness() {
        let verifier = PhiBijectionVerifier::new(10).unwrap();

        // Test basic Phi function: Φ(page, offset) = page*256 + offset
        assert_eq!(verifier.phi_function(0, 0), 0);
        assert_eq!(verifier.phi_function(1, 0), 256);
        assert_eq!(verifier.phi_function(0, 255), 255);
        assert_eq!(verifier.phi_function(2, 128), 2 * 256 + 128);
    }

    #[test]
    fn test_phi_inverse_function() {
        let verifier = PhiBijectionVerifier::new(10).unwrap();

        // Test inverse function
        let (page, offset) = verifier.phi_inverse(0).unwrap();
        assert_eq!((page, offset), (0, 0));

        let (page, offset) = verifier.phi_inverse(256).unwrap();
        assert_eq!((page, offset), (1, 0));

        let (page, offset) = verifier.phi_inverse(2 * 256 + 128).unwrap();
        assert_eq!((page, offset), (2, 128));
    }

    #[test]
    fn test_conservation_budget_tracker() {
        let mut tracker = ConservationBudgetTracker::new(3);

        // Initialize budget
        assert!(tracker.initialize_budget(288).is_ok()); // 288 = 3 * 96
        assert_eq!(tracker.total_budget, 288); // Budget is kept as-is now

        // Try with non-zero budget
        assert!(tracker.initialize_budget(144).is_ok()); // 144 = 1.5 * 96
        assert_eq!(tracker.total_budget, 144); // Budget is kept as-is now
    }

    #[test]
    fn test_conservation_budget_allocation() {
        let mut tracker = ConservationBudgetTracker::new(2);
        tracker.initialize_budget(96).unwrap(); // Full modulus

        // Allocate to categories
        assert!(tracker.allocate_to_category(0, 48).is_ok());
        assert!(tracker.allocate_to_category(1, 48).is_ok());

        // Should be balanced now
        assert!(tracker.is_balanced());
    }

    #[test]
    fn test_failure_closed_semantics() {
        let mut enforcer = FailureClosedSemanticsEnforcer::new(3);
        assert_eq!(enforcer.get_system_state(), SystemState::Normal);
        assert!(enforcer.can_operate());

        // Record some errors
        let error1 = AtlasError::NumericalError("test error 1");
        assert!(enforcer.record_error(&error1).is_ok());
        assert_eq!(enforcer.get_system_state(), SystemState::Warning);

        let error2 = AtlasError::LayerIntegrationError("test error 2");
        assert!(enforcer.record_error(&error2).is_ok());
        assert_eq!(enforcer.get_system_state(), SystemState::Error);
    }

    #[test]
    fn test_invariant_validator_creation() {
        let validator = InvariantValidator::new(10, 3);
        assert!(validator.is_ok());
        let validator = validator.unwrap();

        let (c768_ok, phi_ok, budget_ok, system_ok) = validator.get_validation_status();
        assert!(!c768_ok); // Cycle not closed yet
        assert!(!phi_ok); // Bijection not verified yet
        assert!(budget_ok); // Budget should be balanced initially
        assert!(system_ok); // System should be operational
    }

    #[test]
    fn test_comprehensive_invariant_initialization() {
        let mut validator = InvariantValidator::new(5, 2).unwrap();
        assert!(validator.initialize_all(12345, 96).is_ok());

        // Budget should be initialized
        let (total, _categories, balanced) = validator.budget_tracker.get_budget_state();
        assert_eq!(total, 0); // 96 % 96 = 0
        assert!(balanced);
    }

    #[test]
    fn test_cycle_step_progression() {
        let mut tracker = C768CycleTracker::new();
        tracker.initialize(0).unwrap();

        // Step through a few positions
        assert!(tracker.step(1).is_ok());
        assert_eq!(tracker.position, 1);

        assert!(tracker.step(2).is_ok());
        assert_eq!(tracker.position, 2);

        // Should not be closed yet
        assert!(!tracker.is_cycle_closed());
    }

    #[test]
    fn test_bijection_recording() {
        let mut verifier = PhiBijectionVerifier::new(2).unwrap();
        verifier.initialize().unwrap();

        // Record some encodings
        assert!(verifier.record_encoding(0, 0).is_ok());
        assert!(verifier.record_encoding(0, 1).is_ok());
        assert!(verifier.record_encoding(1, 0).is_ok());

        // Try to record same encoding again - should fail
        assert!(verifier.record_encoding(0, 0).is_err());
    }

    #[test]
    fn test_budget_transfer_operations() {
        let mut tracker = ConservationBudgetTracker::new(3);
        tracker.initialize_budget(96).unwrap();

        // Allocate budget to first category
        tracker.allocate_to_category(0, 60).unwrap();
        tracker.allocate_to_category(1, 36).unwrap();

        // Transfer between categories
        assert!(tracker.transfer_budget(0, 1, 24).is_ok());

        let (_total, categories, _balanced) = tracker.get_budget_state();
        assert_eq!(categories[0], 36); // 60 - 24 = 36
        assert_eq!(categories[1], 60); // 36 + 24 = 60
    }
}
