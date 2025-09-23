# Chapter 6: Programs as Geometry

## Motivation

Traditional models treat programs as sequences of instructions to be executed step-by-step. The Hologram model takes a radically different view: programs are static geometric objects—paths through the configuration space—that exist as complete entities before any "execution" occurs. This isn't just a mathematical curiosity; it fundamentally changes how we reason about composition, optimization, and correctness.

## Denotational Semantics

### Programs as Static Objects

**Definition 6.1 (Process Object)**:
A process object P is a lawful configuration on T representing a complete computation path.

Instead of:
```
execute: Program → State → State
```

We have:
```
denote: Program → ProcessObject
```

The denotation [[P]] IS the program—not instructions to be executed, but a geometric path to be verified.

### The Process Grammar

**Definition 6.2 (Process Language)**:
```
P ::= id                   (identity morphism)
    | morph_i              (primitive morphism)
    | P₁ ∘ P₂              (sequential composition)
    | P₁ ⊗ P₂              (parallel composition)
    | rotate_σ             (schedule rotation)
    | lift_Φ               (boundary→interior lift)
    | proj_Φ               (interior→boundary projection)
```

Each construct denotes a geometric transformation on T.

## Process Objects

### Geometric Interpretation

Each program construct has a geometric meaning:

- **id**: The trivial path (stay in place)
- **morph_i**: A local deformation within resonance class i
- **P₁ ∘ P₂**: Path concatenation
- **P₁ ⊗ P₂**: Parallel paths in disjoint regions
- **rotate_σ**: Following the schedule spiral
- **lift_Φ/proj_Φ**: Movement between boundary and interior

### Path Properties

**Definition 6.3 (Path Receipt)**:
Every path P has a receipt:
```
receipt(P) = (r96_path, c768_path, phi_path, budget_path)
```

The receipt captures the path's geometric invariants.

### Observational Equivalence

**Definition 6.4 (Path Equivalence)**:
```
P₁ ≡ P₂ ⟺ receipt(P₁) = receipt(P₂) modulo gauge
```

Paths are equivalent if they have the same geometric effect.

## Budget Calculus

### Cost Accounting

**Typing Rules with Budgets**:

```
Sequential Composition:
  Γ ⊢ P₁ : τ₁ → τ₂ [β₁]    Γ ⊢ P₂ : τ₂ → τ₃ [β₂]
  ------------------------------------------------
  Γ ⊢ P₁ ∘ P₂ : τ₁ → τ₃ [β₁ + β₂]

Parallel Composition:
  Γ ⊢ P₁ : τ₁ → τ₁' [β₁]    Γ ⊢ P₂ : τ₂ → τ₂' [β₂]
  ------------------------------------------------
  Γ ⊢ P₁ ⊗ P₂ : τ₁ × τ₂ → τ₁' × τ₂' [max(β₁, β₂)]
```

Note: Parallel composition takes the maximum budget, not the sum—parallelism doesn't add cost!

### Budget Optimization

**Theorem 6.1 (Optimal Parallelization)**:
For independent processes P₁, P₂:
```
budget(P₁ ∘ P₂) = β₁ + β₂
budget(P₁ ⊗ P₂) = max(β₁, β₂)
```

Always parallelize when possible to minimize budget.

## Equational Theory

### Algebraic Laws

Process objects obey algebraic laws:

**Associativity**:
```
(P₁ ∘ P₂) ∘ P₃ ≡ P₁ ∘ (P₂ ∘ P₃)
```

**Identity**:
```
id ∘ P ≡ P ≡ P ∘ id
```

**Parallel Commutativity** (when disjoint):
```
P₁ ⊗ P₂ ≡ P₂ ⊗ P₁  (if footprint(P₁) ∩ footprint(P₂) = ∅)
```

**Interchange Law**:
```
(P₁ ⊗ P₂) ∘ (P₃ ⊗ P₄) ≡ (P₁ ∘ P₃) ⊗ (P₂ ∘ P₄)
```
(when footprints are compatible)

### Commuting Diagrams

Process equivalences form commuting diagrams:

```
     P₁
A -------> B
|          |
|P₂        |P₃
v          v
C -------> D
     P₄

P₁ ∘ P₃ ≡ P₂ ∘ P₄ (if diagram commutes)
```

### Normal Forms

**Theorem 6.2 (Process Normal Form)**:
Every process has a normal form:
```
NF(P) = parallel₁ ∘ parallel₂ ∘ ... ∘ parallelₙ
```
where each parallel_i is a maximal parallel composition.

This normal form minimizes total budget.

## Running Example: Parallel Sort

Let's see how sorting becomes a geometric object:

```python
# Traditional imperative sort
def bubble_sort(arr):
    n = len(arr)
    for i in range(n):
        for j in range(0, n-i-1):
            if arr[j] > arr[j+1]:
                arr[j], arr[j+1] = arr[j+1], arr[j]

# Hologram geometric sort
def geometric_sort(config):
    # Define comparison morphisms
    compare_morphisms = []
    for i in range(n):
        for j in range(0, n-i-1):
            m = create_compare_swap(j, j+1)
            compare_morphisms.append(m)

    # Identify parallel opportunities
    parallel_groups = []
    for i in range(n):
        # Even-odd parallelization
        if i % 2 == 0:
            group = parallel_compose([
                compare_swap(2*j, 2*j+1)
                for j in range(n//2)
            ])
        else:
            group = parallel_compose([
                compare_swap(2*j+1, 2*j+2)
                for j in range(n//2-1)
            ])
        parallel_groups.append(group)

    # Compose into single geometric object
    sort_path = sequential_compose(parallel_groups)

    # The path IS the sort - no execution needed
    return sort_path

# Verify sort correctness
sort_path = geometric_sort(initial_config)
receipt = compute_receipt(sort_path)
assert receipt.preserves_multiset()  # Same elements
assert receipt.ensures_sorted()       # Correct order
assert receipt.budget <= O(n²)        # Complexity bound
```

## Geometric Optimization

### Path Straightening

Optimization becomes geometric path straightening:

```python
def optimize_path(path):
    # Find redundant loops
    loops = find_loops(path)
    for loop in loops:
        if is_null_effect(loop):
            path = remove_loop(path, loop)

    # Identify parallel opportunities
    sequential_segments = decompose_sequential(path)
    parallel_segments = []
    for seg in sequential_segments:
        parallel_segments.append(maximize_parallelism(seg))

    return compose(parallel_segments)
```

### Geodesics

**Definition 6.5 (Computational Geodesic)**:
The shortest path between configurations with respect to budget metric.

**Theorem 6.3 (Geodesic Optimality)**:
For lawful configs A,B, the geodesic from A to B minimizes budget.

## Category-Theoretic View

### The Process Category

Process objects form a category:
- **Objects**: Lawful configurations
- **Morphisms**: Process paths
- **Composition**: Path concatenation (∘)
- **Identity**: Trivial path (id)

### Functoriality

The receipt map is functorial:
```
receipt: Process → Receipt
receipt(P ∘ Q) = receipt(P) ⊕ receipt(Q)
receipt(id) = neutral_receipt
```

### Natural Transformations

Gauge transformations are natural:
```
   P
A ---> B
|      |
g      g
v      v
A' --> B'
  g(P)

g(P) ≡ᵍ P (naturality)
```

## Exercises

**Exercise 6.1**: Prove the interchange law for process composition.

**Exercise 6.2**: Find the geodesic path for transposing a matrix on T.

**Exercise 6.3**: Show that every iterative algorithm has an equivalent geometric path.

**Exercise 6.4**: Design a path that computes factorial without iteration.

**Exercise 6.5**: Prove that path straightening preserves receipts.

## Implementation Notes

```rust
#[derive(Clone)]
pub enum Process {
    Identity,
    Morphism(MorphismId),
    Sequential(Box<Process>, Box<Process>),
    Parallel(Box<Process>, Box<Process>),
    Rotate(u16),  // Number of σ rotations
    LiftPhi,
    ProjPhi,
}

impl Process {
    pub fn receipt(&self) -> Receipt {
        match self {
            Process::Identity => Receipt::identity(),
            Process::Morphism(id) => morphism_receipt(*id),
            Process::Sequential(p1, p2) => {
                p1.receipt().compose_sequential(&p2.receipt())
            }
            Process::Parallel(p1, p2) => {
                p1.receipt().compose_parallel(&p2.receipt())
            }
            Process::Rotate(n) => rotation_receipt(*n),
            Process::LiftPhi => phi_lift_receipt(),
            Process::ProjPhi => phi_proj_receipt(),
        }
    }

    pub fn optimize(self) -> Process {
        match self {
            Process::Sequential(p1, p2) => {
                // Check for parallelization opportunity
                if p1.footprint().disjoint(&p2.footprint()) {
                    Process::Parallel(p1, p2)
                } else {
                    Process::Sequential(
                        Box::new(p1.optimize()),
                        Box::new(p2.optimize())
                    )
                }
            }
            other => other,
        }
    }
}
```

## Takeaways

1. **Programs are geometric paths**: Static objects, not dynamic executions
2. **Denotation IS the program**: No gap between meaning and implementation
3. **Composition is path concatenation**: Sequential and parallel have geometric meaning
4. **Budgets compose algebraically**: Addition for sequential, max for parallel
5. **Optimization is geometric**: Path straightening and geodesic finding
6. **Equivalence is geometric**: Same receipts = same computational effect

Programs aren't instructions—they're geometric objects with algebraic structure.

---

*Next: Chapter 7 explores algorithmic reification—how these geometric programs become verifiable proofs.*