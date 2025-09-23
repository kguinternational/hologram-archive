# Reader's Guide & Conventions

## Mathematical Prerequisites

This book assumes comfort with:
- **Discrete Mathematics**: Modular arithmetic, equivalence relations, group theory basics
- **Automata Theory**: Finite state machines, regular languages, decidability
- **Type Theory**: Typing judgments, inference rules, soundness and completeness
- **Denotational Semantics**: Mathematical objects as program meanings, compositionality
- **Basic Topology**: Continuity, compactness (for optimization discussions)

Category theory appears occasionally but is not required—we explain categorical concepts when used.

## Notation & Core Objects

### The Fundamental Space

**The 12,288 Lattice**: ℤ/48 × ℤ/256
- Written as **T** throughout
- Elements: (p,b) where p ∈ [0,47], b ∈ [0,255]
- Linear indexing: i = 256p + b
- Cardinality: |T| = 12,288
- Topology: Toroidal with wraparound

### Algebraic Structures

**Alphabet**: Σ = ℤ₂₅₆ (the byte space)

**Resonance Residue**: R: Σ → ℤ₉₆
- Partitions bytes into 96 equivalence classes
- Compositional: R(concat(x,y)) determined by R(x) and R(y)

**Budget Semiring**: C₉₆ = (ℤ₉₆; +, ×)
- Semantic costs compose additively
- Budget 0 represents "fully lawful"

**Crush Operator**: ⟨β⟩ ∈ {false, true}
- ⟨β⟩ = true ⟺ β = 0 in ℤ₉₆
- Decidable truth from arithmetic

### Transformations

**Schedule Rotation**: σ: T → T
- Fixed automorphism of order 768
- Generates fairness invariants
- Written C768 when discussing the cyclic group

**Lift/Projection Pair**:
- lift_Φ: boundary → interior
- proj_Φ: interior → boundary
- Round-trip: proj_Φ ∘ lift_Φ = id at budget 0

**Gauge Actions**:
- Translations on T
- Schedule rotation σ
- Boundary automorphism subgroup G°

### Information Structures

**Configuration**: s ∈ Σᵀ
- Assignment of bytes to lattice sites
- Subject to conservation laws

**Receipt**: (R₉₆_digest, C₇₆₈_stats, Φ_roundtrip, budget_ledger)
- Verifiable witness of lawfulness
- Compositional under morphism composition

**Process Object**: Static lawful program denotation
- Geometric path on T
- Characterized by receipts modulo gauge

## Reading Conventions

### Typography

- **Bold** for defined terms on first appearance
- *Italic* for emphasis and meta-level discussion
- `Monospace` for code and concrete implementations
- SMALL CAPS for system components (e.g., VERIFIER, COMPILER)

### Mathematical Style

Definitions are numbered within chapters:
> **Definition 3.2 (Resonance Class)**: An equivalence relation on Σ...

Theorems state precise claims:
> **Theorem 4.7**: The address map H is injective on the lawful domain.

Proofs are marked clearly:
> *Proof*: By induction on configuration size...□

### Examples and Exercises

**Running Examples** appear in gray boxes:
```
Example: 16-site configuration
Sites: (0,0) through (3,3)
Bytes: [0x42, 0x7F, ...]
Residues: [18, 31, ...]
R96 digest: 0xA5F9...
```

**Exercises** test understanding:
> **Exercise 2.3**: Prove that receipts are class functions on gauge orbits.

Solutions appear in Appendix D.

### Cross-References

- Forward references: "We will see in Chapter 7..."
- Backward references: "Recall from Section 3.2..."
- Margin notes:
  - ⚡ Connection to another chapter
  - 🔬 Open research question
  - ⚠️ Common misconception
  - 💡 Key insight

## Pedagogical Approach

Each chapter follows this structure:

1. **Motivation**: Why does this concept matter?
2. **Core Definitions**: Precise mathematical foundations
3. **CS Analogues**: Connections to familiar concepts
4. **Theorems & Properties**: What can we prove?
5. **Running Example**: Concrete instantiation
6. **Implementation Notes**: How to build it
7. **Exercises**: Test your understanding
8. **Takeaways**: Key insights to remember

## Quick Reference Guides

### Symbol Glossary

| Symbol | Meaning |
|--------|---------|
| T | The 12,288 lattice (ℤ/48 × ℤ/256) |
| Σ | Alphabet (ℤ₂₅₆) |
| R | Resonance map to ℤ₉₆ |
| σ | Schedule rotation (order 768) |
| Φ | Lift/projection operator pair |
| β | Budget in C₉₆ |
| ⟨·⟩ | Crush to boolean |
| H | Address map (perfect hash) |
| S | Action (universal cost) |
| ⊗ | Parallel composition |
| ∘ | Sequential composition |
| ≡ᵍ | Gauge equivalence |
| ⊢ | Typing judgment |

### Concept Map

```
Information → Intrinsic Structure → Conservation Laws
     ↓              ↓                      ↓
   Bytes     Resonance Classes       Type System
     ↓              ↓                      ↓
  Lattice T    Receipts            Programs as Proofs
     ↓              ↓                      ↓
   CAM/Hash    Verification          Compilation
```

## How Different Readers Should Proceed

### For Theoreticians
- Focus on Parts I, II, and IV
- Pay special attention to proofs and exercises
- Explore connections to category theory and type theory

### For Systems Builders
- Start with Part III for motivation
- Study Parts I and V carefully
- Focus on implementation notes and Appendix E

### For Security Researchers
- Begin with Chapter 9 (Security properties)
- Understand receipt verification (Chapter 3)
- Study collision resistance proofs (Chapter 16)

### For Compiler Designers
- Focus on Chapter 8 (Universal cost)
- Study denotational semantics (Chapter 6)
- Examine the mini-compiler (Chapter 12)

## Beyond This Book

Active research areas (marked with 🔬) include:
- Expressivity bounds for the 12,288 model
- Quantum extensions preserving conservation laws
- Hardware implementations of receipt verification
- Distributed consensus via receipt agreement

The bibliography provides entry points to the broader literature.

## Getting Started

Turn to Chapter 1 to begin with first principles, or jump to Chapter 10 for concrete examples that demonstrate the model in action. Either path will lead you to a new understanding of computation itself.