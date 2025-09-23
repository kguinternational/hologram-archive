# Chapter 3: Intrinsic Labels, Schedules, and Receipts

## Motivation

Having established the 12,288 lattice as our computational space, we now need to give meaning to configurations on that space. In traditional computing, meaning comes from external interpretation—a bit pattern means what we say it means. In the Hologram model, meaning is intrinsic through three labeling systems:

1. **R96 Resonance Classes**: Semantic types as algebraic invariants
2. **C768 Cycle Structure**: Fair scheduling built into physics
3. **Φ Lift/Projection**: Information preservation under transformation

These aren't separate systems bolted together—they're three aspects of a unified labeling scheme that makes lawfulness decidable and cheap to verify.

## Resonance Classes (R96)

### The Residue System

**Definition 3.1 (Resonance Map)**:
```
R: Σ → ℤ₉₆
R(b) = h₁(b mod 96) ⊕ h₂(⌊b/96⌋) ⊕ h₃(b)
```

where h₁, h₂, h₃ are carefully chosen mixing functions ensuring:
- Uniform distribution across residue classes
- Algebraic compositionality
- Collision resistance on structured inputs

**Theorem 3.1 (Residue Distribution)**:
For random byte b, P(R(b) = k) = 1/96 for all k ∈ ℤ₉₆.

### Compositional Semantics

The magic of R96: residues compose algebraically.

**Definition 3.2 (Multiset Residue)**:
For bytes b₁,...,bₙ:
```
R({b₁,...,bₙ}) = ⊕ᵢ R(bᵢ) (multiset sum in ℤ₉₆)
```

**Property 3.1 (Permutation Invariance)**:
R({b₁,...,bₙ}) = R({bπ(1),...,bπ(n)}) for any permutation π.

This means semantic meaning is independent of ordering—crucial for parallelism.

### The R96 Checksum

**Definition 3.3 (R96 Digest)**:
For configuration s on region Ω ⊂ T:
```
R96(s,Ω) = Hash(histogram(R(s(t)) for t ∈ Ω))
```

The histogram captures the distribution of residue classes, and the hash produces a fixed-size digest.

### CS Analogue

Think of R96 as:
- A **semantic hash function** that preserves algebraic structure
- A **type system** where types are residue classes
- An **abstract interpretation** that's complete for certain invariants
- A **homomorphic fingerprint** enabling computation on encrypted data

### Running Example: String Processing

```python
text = "HELLO WORLD"
bytes = [ord(c) for c in text]
# [72, 69, 76, 76, 79, 32, 87, 79, 82, 76, 68]

residues = [R(b) for b in bytes]
# [24, 21, 28, 28, 31, 32, 39, 31, 34, 28, 20]

r96_digest = compute_r96_digest(residues)
# Histogram: {20:1, 21:1, 24:1, 28:3, 31:2, 32:1, 34:1, 39:1}
# Digest: 0x7A3E... (deterministic hash of histogram)
```

Any transformation that preserves the histogram preserves semantic meaning.

## Cycle Structure (C768)

### The Universal Schedule

**Definition 3.4 (Schedule Automorphism)**:
```
σ: T → T with order 768
σ = σ₄₈ × σ₁₆ where:
  σ₄₈: ℤ/48 → ℤ/48, rotation by 1
  σ₁₆: ℤ/256 → ℤ/256, rotation by 16
  lcm(48,16) = 768
```

Every site gets exactly one "time slot" per 768-step cycle.

### Fairness Invariants

**Definition 3.5 (Fairness Metrics)**:
```
FairnessMetrics = {
    mean_activation: ℝ,        // Average activations per cycle
    variance_activation: ℝ,     // Spread of activations
    max_wait: ℕ,               // Longest wait between activations
    flow_balance: ℤ₉₆,         // Net flow around cycle
}
```

**Theorem 3.2 (Perfect Fairness)**:
Under σ, every site is visited exactly once per 768 steps, giving:
- mean_activation = 1/768
- variance_activation = 0 (perfect uniformity)
- max_wait = 768

### Orbit Structure

The schedule creates orbits—paths that sites follow under repeated application of σ:

```
Orbit(t) = {t, σ(t), σ²(t), ..., σ⁷⁶⁷(t)}
```

**Property 3.2**: Every orbit has exactly 768 elements (σ is a cyclic permutation).

### CS Interpretation

C768 is simultaneously:
- A **round-robin scheduler** with perfect fairness
- A **clock generator** with guaranteed periodicity
- A **load balancer** with zero overhead
- A **consensus mechanism** with deterministic ordering

### Interaction with Computation

Programs don't fight the schedule—they surf it:

```rust
fn execute_on_schedule(lattice: &mut Lattice, start: Site) {
    let mut current = start;
    for step in 0..768 {
        // Process site at its scheduled time
        let value = lattice.get(current);
        let processed = process(value, step);
        lattice.set(current, processed);

        current = current.rotate_schedule();
    }
    // After 768 steps, we're back at start
}
```

## The Φ Operator

### Lift and Projection

**Definition 3.6 (Φ Operator Pair)**:
```
lift_Φ: Σᴮ → Σᴵ    (boundary → interior)
proj_Φ: Σᴵ → Σᴮ    (interior → boundary)
```

where B ⊂ T is the boundary region and I ⊂ T is the interior.

### Round-Trip Property

**Theorem 3.3 (Φ Coherence)**:
At budget β = 0:
```
proj_Φ ∘ lift_Φ = id_B
```

At budget β > 0:
```
||proj_Φ ∘ lift_Φ(x) - x|| ≤ f(β)
```

where f is a known error bound function.

### Information-Theoretic Interpretation

Φ is an optimal encoder/decoder pair:
- **Lift**: Embeds boundary data into interior with redundancy
- **Projection**: Extracts boundary from interior, error-correcting

The budget β controls the compression/redundancy tradeoff.

### CS Analogue

Φ resembles:
- **Erasure codes** in distributed storage
- **Holographic encoding** in quantum error correction
- **Dimensionality reduction** preserving essential features
- **Adjoint functors** in category theory (when β = 0)

### Implementation Sketch

```python
def lift_phi(boundary_data, budget):
    # Spread boundary data across interior with redundancy
    interior = np.zeros(INTERIOR_SIZE)

    for i, value in enumerate(boundary_data):
        # Each boundary byte influences multiple interior sites
        spread_pattern = generate_spread(i, budget)
        for site, weight in spread_pattern:
            interior[site] += weight * value

    return normalize(interior)

def proj_phi(interior_data, budget):
    # Extract boundary from interior via optimal estimation
    boundary = np.zeros(BOUNDARY_SIZE)

    for i in range(BOUNDARY_SIZE):
        # Combine interior evidence for each boundary site
        gather_pattern = generate_gather(i, budget)
        boundary[i] = sum(interior[s] * w for s,w in gather_pattern)

    return quantize(boundary)
```

## Budgets & Receipts

### The Budget Semiring

**Definition 3.7 (Budget Algebra)**:
```
C₉₆ = (ℤ₉₆, +, ×, 0, 1)
```

Budgets track semantic cost:
- Addition for sequential composition
- Multiplication for parallel scaling
- Zero means "perfectly lawful"

**Definition 3.8 (Crush to Truth)**:
```
⟨β⟩ = true  iff β = 0 in ℤ₉₆
⟨β⟩ = false iff β ≠ 0 in ℤ₉₆
```

This gives us a decision procedure: lawful = zero budget.

### Receipt Structure

**Definition 3.9 (Complete Receipt)**:
```rust
struct Receipt {
    // R96 sector
    r96_digest: [u8; 32],      // Multiset hash of residues

    // C768 sector
    c768_cycle_count: u32,      // Which cycle we're in
    c768_phase: u16,           // Position within cycle (0-767)
    c768_fairness: FairnessMetrics,

    // Φ sector
    phi_lift_sites: BitSet,    // Which sites were lifted
    phi_proj_sites: BitSet,    // Which sites were projected
    phi_round_trip: bool,      // Did round-trip succeed?

    // Budget sector
    budget_total: i96,         // Accumulated semantic cost
    budget_breakdown: BudgetLedger,  // Detailed accounting
}
```

### Receipt Verification

**Algorithm 3.1 (Linear Receipt Verification)**:
```python
def verify_receipt(config, receipt):
    # Check R96 (O(n) residue computation)
    computed_r96 = compute_r96_digest(config)
    if computed_r96 != receipt.r96_digest:
        return False

    # Check C768 (O(1) phase lookup)
    expected_phase = compute_phase(config.timestamp)
    if expected_phase != receipt.c768_phase:
        return False

    # Check Φ (O(boundary) round-trip test)
    if receipt.phi_round_trip:
        boundary = extract_boundary(config)
        interior = extract_interior(config)
        if proj_phi(lift_phi(boundary)) != boundary:
            return False

    # Check budget (O(k) for k operations)
    if receipt.budget_total != sum(receipt.budget_breakdown):
        return False

    return True
```

**Theorem 3.4 (Verification Complexity)**:
Receipt verification is O(n) where n is the active window size.

*Proof*: Each check requires at most one pass through the data. No searching, no exponential paths. □

## Composition of Receipts

### Sequential Composition

When composing operations A;B:

```
receipt(A;B) = {
    r96: hash(r96_A, r96_B),
    c768: advance_phase(c768_A, duration_B),
    phi: phi_A ∧ phi_B,
    budget: budget_A + budget_B
}
```

### Parallel Composition

When composing operations A||B:

```
receipt(A||B) = {
    r96: merge_histograms(r96_A, r96_B),
    c768: sync_phases(c768_A, c768_B),
    phi: phi_A ∧ phi_B,
    budget: max(budget_A, budget_B)  // Parallel doesn't add cost
}
```

## Running Example: Sorting with Receipts

Let's trace receipt generation through a sorting operation:

```python
# Initial configuration
data = [42, 17, 99, 3, 58]
sites = [(0,0), (0,1), (0,2), (0,3), (0,4)]

# Step 1: Compute initial receipt
r1 = {
    'r96': compute_r96(data),  # Hash of [R(42), R(17), R(99), R(3), R(58)]
    'c768_phase': 0,
    'phi': True,  # Boundary data, trivially coherent
    'budget': 0   # No operations yet
}

# Step 2: Bubble sort pass
swap(data, 0, 1)  # 42 <-> 17
r2 = {
    'r96': r1['r96'],  # Swapping preserves multiset!
    'c768_phase': 1,    # Advanced one step
    'phi': True,        # Still coherent
    'budget': 1         # One comparison operation
}

# ... continue sorting ...

# Final: Verify sorted
final_data = [3, 17, 42, 58, 99]
final_receipt = {
    'r96': r1['r96'],   # Same multiset of residues
    'c768_phase': 10,   # After 10 operations
    'phi': True,        # Maintained coherence
    'budget': 10        # Total comparisons
}

assert verify_receipt(final_data, final_receipt)  # Passes!
```

The receipt proves we sorted without adding or removing elements.

## Exercises

**Exercise 3.1**: Prove that R96 is a homomorphism from byte sequences under concatenation to ℤ₉₆ under addition.

**Exercise 3.2**: Calculate the complete C768 orbit for site (0,0). How many distinct sites are visited?

**Exercise 3.3**: Design a Φ operator that achieves 2x compression at budget β=10. What's the round-trip error?

**Exercise 3.4**: Show that receipt verification catches single-bit errors with probability ≥ 1 - 1/96.

**Exercise 3.5**: Implement receipt composition for a map-reduce operation. How do the budgets combine?

## Implementation Notes

Here's a production-quality receipt builder:

```rust
pub struct ReceiptBuilder {
    hasher: R96Hasher,
    scheduler: C768Scheduler,
    phi_tracker: PhiTracker,
    budget_ledger: BudgetLedger,
}

impl ReceiptBuilder {
    pub fn new(initial_config: &Configuration) -> Self {
        Self {
            hasher: R96Hasher::from_config(initial_config),
            scheduler: C768Scheduler::at_phase(0),
            phi_tracker: PhiTracker::new(),
            budget_ledger: BudgetLedger::new(),
        }
    }

    pub fn record_operation(&mut self, op: &Operation) {
        self.hasher.update(op.affected_bytes());
        self.scheduler.advance(op.duration());
        self.phi_tracker.track(op.phi_operations());
        self.budget_ledger.charge(op.cost());
    }

    pub fn finalize(self) -> Receipt {
        Receipt {
            r96_digest: self.hasher.finalize(),
            c768_state: self.scheduler.get_state(),
            phi_coherent: self.phi_tracker.is_coherent(),
            budget: self.budget_ledger.total(),
        }
    }
}
```

## Takeaways

1. **R96 gives semantic types**: 96 equivalence classes with algebraic structure
2. **C768 ensures perfect fairness**: Every site gets equal time
3. **Φ preserves information**: Round-trip identity at zero budget
4. **Budgets track lawfulness**: Zero budget = perfectly lawful
5. **Receipts are proof-carrying data**: Linear-time verification
6. **Composition is algebraic**: Receipts compose like the operations they witness

These three labeling systems—R96, C768, Φ—work together to make lawfulness intrinsic and verifiable.

---

*Next: Chapter 4 shows how these labels enable perfect hashing and content-addressable memory.*