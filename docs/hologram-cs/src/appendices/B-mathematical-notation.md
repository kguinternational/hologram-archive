# Appendix B: Mathematical Notation

## Basic Sets and Structures

| Notation | Meaning | Example Usage |
|----------|---------|---------------|
| ℤ/n | Integers modulo n | ℤ/96 for budget arithmetic |
| 𝕋 | The 12,288 lattice | 𝕋 = (ℤ/48) × (ℤ/256) |
| Σ | Alphabet (bytes) | Σ = ℤ₂₅₆ = {0, 1, ..., 255} |
| Σ^𝕋 | Configuration space | All functions 𝕋 → Σ |
| ℤ₉₆ | Residue classes | Codomain of resonance map |
| C₉₆ | Budget semiring | (ℤ₉₆; +, ×) |

## Functions and Maps

| Notation | Type | Description |
|----------|------|-------------|
| R | Σ → ℤ₉₆ | Resonance residue function |
| H | Object → 𝕋 | Address map (perfect hash) |
| σ | 𝕋 → 𝕋 | Schedule rotation (order 768) |
| lift_Φ | Boundary → Interior | Lift operator |
| proj_Φ | Interior → Boundary | Projection operator |
| ⟨·⟩ | ℤ₉₆ → {true, false} | Crush function |

## Lattice Coordinates

| Notation | Meaning | Range |
|----------|---------|-------|
| (p, b) | Lattice coordinate | p ∈ [0,47], b ∈ [0,255] |
| i | Linear index | i = 256p + b |
| s(p,b) | Configuration at site | s : 𝕋 → Σ |
| |𝕋| | Lattice cardinality | 12,288 |

## Type System

| Notation | Meaning |
|----------|---------|
| Γ ⊢ x : τ [β] | Budgeted typing judgment |
| τ₁ → τ₂ | Function type |
| τ₁ × τ₂ | Product type |
| τ₁ + τ₂ | Sum type |
| ∀α. τ | Polymorphic type |
| Πx:τ₁. τ₂ | Dependent type |

## Process Calculus

| Notation | Meaning |
|----------|---------|
| P ::= ... | Process grammar |
| id | Identity morphism |
| P ∘ Q | Sequential composition |
| P ⊗ Q | Parallel composition |
| ⟦P⟧ | Denotation of process P |
| P ≡ Q | Observational equivalence |

## Receipts and Verification

| Notation | Component | Type |
|----------|-----------|------|
| r₉₆ | R96 digest | Multiset histogram |
| c₇₆₈ | C768 statistics | Fairness metrics |
| φ_rt | Φ round-trip bit | Boolean |
| β_L | Budget ledger | ℤ₉₆ |
| ℛ | Receipt tuple | (r₉₆, c₇₆₈, φ_rt, β_L) |

## Action Functional

| Notation | Meaning |
|----------|---------|
| S[ψ] | Action functional on field ψ |
| δS | Variation of action |
| ℒ_sector | Sector Lagrangian |
| ∇S | Action gradient |
| H_S | Action Hessian |
| S* | Stationary action value |

## Budget Arithmetic

| Notation | Operation | Modulus |
|----------|-----------|---------|
| β₁ + β₂ | Budget addition | mod 96 |
| β₁ × β₂ | Budget multiplication | mod 96 |
| -β | Budget negation | mod 96 |
| β = 0 | Lawful (crushes to true) | - |
| β ∈ [0,47] | Non-negative budget | - |

## Gauge Transformations

| Notation | Transformation |
|----------|----------------|
| g · s | Gauge action on configuration |
| G^∘ | Boundary automorphism group |
| [s]_G | Gauge equivalence class |
| s_NF | Normal form of s |
| τ_v | Translation by vector v |

## Complexity Classes

| Class | Description |
|-------|-------------|
| CC | Conservation-Checkable |
| RC | Resonance-Commutative |
| HC | High-Commutative |
| WC | Window-Constrained |
| O(n) | Linear time in window size |

## Category Theory

| Notation | Meaning |
|----------|---------|
| Ob(C) | Objects of category C |
| Hom(A,B) | Morphisms from A to B |
| F : C → D | Functor from C to D |
| η : F ⇒ G | Natural transformation |
| A ≅ B | Isomorphism |

## Probabilistic Notation

| Notation | Meaning |
|----------|---------|
| ℙ[E] | Probability of event E |
| 𝔼[X] | Expectation of X |
| Var(X) | Variance of X |
| X ~ D | X drawn from distribution D |
| H(X) | Entropy of X |

## Linear Algebra

| Notation | Object |
|----------|--------|
| v ∈ ℝⁿ | Vector in n-dimensional space |
| A ∈ ℝᵐˣⁿ | m × n matrix |
| A^T | Matrix transpose |
| λ(A) | Eigenvalues of A |
| ‖v‖ | Norm of vector v |
| ⟨u,v⟩ | Inner product |

## Order Relations

| Notation | Meaning |
|----------|---------|
| a ≤ b | Less than or equal |
| a < b | Strictly less than |
| a ≼ b | Partial order |
| a ≺ b | Strict partial order |
| ⊥ | Bottom element |
| ⊤ | Top element |

## Logic and Proofs

| Notation | Meaning |
|----------|---------|
| ∧ | Logical and |
| ∨ | Logical or |
| ¬ | Logical not |
| → | Implication |
| ↔ | If and only if |
| ∀ | Universal quantification |
| ∃ | Existential quantification |
| ⊢ | Proves/derives |
| ⊨ | Satisfies/models |

## Set Operations

| Notation | Operation |
|----------|-----------|
| A ∪ B | Union |
| A ∩ B | Intersection |
| A \ B | Set difference |
| A × B | Cartesian product |
| 2^A | Power set |
| |A| | Cardinality |
| ∅ | Empty set |

## Special Symbols

| Symbol | Usage |
|--------|-------|
| ≡ | Equivalence, congruence |
| ≈ | Approximately equal |
| ∼ | Similar to, distributed as |
| ⊕ | Direct sum, XOR |
| ⊗ | Tensor product |
| ∘ | Function composition |
| ↦ | Maps to |
| ∈ | Element of |
| ⊆ | Subset |

## Subscripts and Superscripts

| Notation | Meaning |
|----------|---------|
| x_i | i-th component |
| x^i | i-th power or contravariant |
| x_{i,j} | Component at position (i,j) |
| x^{(k)} | k-th iteration |
| x' | Prime, derivative, or modified |
| x* | Optimal, dual, or conjugate |

## Common Abbreviations

| Abbr. | Full Form |
|-------|-----------|
| s.t. | subject to |
| w.r.t. | with respect to |
| iff | if and only if |
| i.e. | that is |
| e.g. | for example |
| cf. | compare with |
| viz. | namely |
| WLOG | without loss of generality |

## Asymptotic Notation

| Notation | Meaning |
|----------|---------|
| O(f) | Big-O (upper bound) |
| Ω(f) | Big-Omega (lower bound) |
| Θ(f) | Big-Theta (tight bound) |
| o(f) | Little-o (strict upper) |
| ω(f) | Little-omega (strict lower) |

## Units and Constants

| Symbol | Value/Meaning |
|--------|--------------|
| 12,288 | |𝕋| = 48 × 256 |
| 96 | Resonance classes |
| 768 | Order of σ |
| 48 | Number of pages |
| 256 | Bytes per page |
| 0 | Lawful budget |
| ε | Small positive value |

## Index Conventions

- Latin indices (i, j, k): Usually range over spatial dimensions or discrete sets
- Greek indices (α, β, γ): Often denote type variables or budget values
- Capital letters: Typically denote sets, types, or operators
- Lowercase letters: Usually denote elements, variables, or functions
- Bold: Often indicates vectors or matrices
- Calligraphic: Typically categories, functionals, or special sets

## Reading Guide

When encountering composite notation:
1. Identify the base symbol
2. Check for subscripts/superscripts
3. Consider the context (type theory, algebra, etc.)
4. Refer to the specific chapter for domain-specific usage

## Common Patterns

| Pattern | Meaning | Example |
|---------|---------|---------|
| X/∼ | Quotient by equivalence | 𝕋/G (gauge quotient) |
| Hom(−,−) | Morphism sets | Hom(A,B) |
| [−] | Equivalence class | [s]_G |
| ⟦−⟧ | Semantic brackets | ⟦P⟧ |
| ⟨−⟩ | Generated by, crush | ⟨β⟩ |
| {−|−} | Set builder | {x | P(x)} |