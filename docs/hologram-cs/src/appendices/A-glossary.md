# Appendix A: Glossary

## Core Terms

**12,288 Lattice (𝕋)**
The universal finite state space (ℤ/48)×(ℤ/256) that serves as the carrier for all computation. Has toroidal topology with wraparound at boundaries.

**Action (S)**
Universal cost functional that determines compilation, optimization, and learning. Minimizing action subject to constraints defines lawful computation.

**Active Window**
The subset of the lattice currently being processed or verified. Enables streaming computation with bounded memory.

**Address Map (H)**
Deterministic function from normalized objects to lattice coordinates. Provides perfect hashing on the lawful domain.

## Mathematical Components

**Budget (β)**
Semantic cost in ℤ/96. Budget 0 corresponds to fully lawful computation. Non-zero budgets quantify deviation from ideal lawfulness.

**Budget Semiring (C₉₆)**
The algebraic structure (ℤ/96; +, ×) used for budget arithmetic and composition.

**C768**
Canonical schedule rotation automorphism of order 768. Ensures fairness in distributed computation.

**Carrier**
The underlying set or space on which operations act. For Hologram, this is the 12,288 lattice.

**Configuration**
An assignment of bytes to lattice sites, representing a state of computation. Elements of Σ^𝕋.

**Content-Addressable Memory (CAM)**
Storage system where objects are addressed by their content rather than location. Enables perfect deduplication.

**Crush (⟨β⟩)**
Boolean function mapping budgets to truth values. ⟨β⟩ = true iff β = 0 in ℤ/96.

## System Architecture

**Gauge**
Group of symmetry transformations that preserve semantic meaning. Includes translations, rotations, and boundary automorphisms.

**Gauge Invariance**
Property preserved under gauge transformations. Receipts and lawfulness are gauge-invariant.

**Lawful Object**
Configuration whose receipts verify at budget 0 and passes Φ round-trip test.

**Lift Operator (lift_Φ)**
Morphism from boundary to interior that preserves information at budget 0.

**Morphism**
Structure-preserving transformation between configurations. Basic computational operations.

**Normal Form (NF)**
Canonical representative of a gauge equivalence class. Used for unique addressing.

## Verification Components

**Φ Operator**
Lift/projection pair ensuring coherence between boundary and interior. Round-trip preserving at budget 0.

**Process Object**
Static lawful representation of a computation. Geometric path on 𝕋 with receipts.

**Projection Operator (proj_Φ)**
Morphism from interior to boundary. Inverse of lift at budget 0.

**R96**
System of 96 resonance equivalence classes on bytes. Provides compositional semantic labeling.

**Receipt**
Verifiable witness tuple containing R96 digest, C768 stats, Φ round-trip bit, and budget ledger.

**Resonance**
Intrinsic semantic property of bytes determining their equivalence class under R.

## Types and Semantics

**Budgeted Typing**
Type system where judgments carry explicit semantic costs. Form: Γ ⊢ x : τ [β].

**Denotation**
Semantic meaning of a program as a geometric object. Written ⟦P⟧.

**Observational Equivalence**
Programs with identical receipts modulo gauge. Semantic equality.

**Poly-Ontological Object**
Entity simultaneously inhabiting multiple mathematical categories with coherence morphisms.

**Type Safety**
Property that ill-typed configurations cannot physically exist in the lattice.

**Witness Chain**
Sequence of receipts proving correct execution. Enables verification and audit.

## Algorithmic Concepts

**Algorithmic Reification**
Making abstract computation concrete as verifiable process objects.

**Class-Local Transform**
Morphism operating within a single resonance equivalence class.

**Fairness Invariant**
Statistical property preserved by C768 rotation ensuring balanced resource usage.

**Linear-Time Verification**
O(n) verification complexity for n-sized active window plus witnesses.

**Perfect Hash**
Collision-free hash function on lawful domain via content addressing and normalization.

**Schedule Rotation (σ)**
Fixed automorphism implementing round-robin scheduling without external clock.

## Implementation Terms

**Action Density**
Local contribution to global action functional. Used in optimization.

**Compilation as Stationarity**
Program compiles iff it satisfies δS = 0 under constraints.

**Conservation Law**
Invariant preserved by lawful computation. Examples: R96, C768, Φ-coherence, budget.

**Gauge Fixing**
Selection of canonical representative from equivalence class.

**Incremental Verification**
Verifying only changed portions of configuration.

**Window-Constrained (WC)**
Complexity class for operations verifiable in bounded window.

## Distributed Systems

**Byzantine Fault Tolerance**
System property of maintaining correctness despite malicious nodes. Achieved through receipts.

**Content Routing**
Network routing based on content addresses rather than locations.

**Receipt Consensus**
Agreement protocol using receipts as votes and proofs.

**Shard**
Partition of lattice for distributed storage or computation.

**State Machine Replication**
Maintaining consistent replicas through receipt chains.

## Database Concepts

**Index-Free Architecture**
Database design without auxiliary index structures. Uses CAM for direct access.

**Merkle DAG**
Directed acyclic graph with content-addressed nodes.

**MVCC (Multi-Version Concurrency Control)**
Concurrency through content-addressed snapshots.

**Query as Proof**
Query results include cryptographic proof of correctness.

**Schema-Free Storage**
Storage supporting dynamic types through poly-ontology.

## Compiler Terms

**Action-Based Code Generation**
Selecting instructions that minimize action functional.

**Gauge Alignment**
Linking process that aligns gauge across compilation units.

**Optimization Pass**
Transformation that reduces action while preserving semantics.

**Universal Optimizer**
Single optimizer handling all programs through action minimization.

**Variational Compilation**
Compilation as solving variational problem δS = 0.

## Machine Learning

**Action Flow**
Learning dynamics as gradient flow of action functional.

**Convergence Certificate**
Receipt-based proof of optimization convergence.

**Gradient-Free Optimization**
Optimization using receipts without computing gradients.

**Lyapunov Function**
Action serving as stability guarantor for learning.

**Single Loss Function**
All ML tasks minimize the same universal action.

## Formal Properties

**Church-Rosser Property**
Confluence of reductions modulo gauge equivalence.

**Expressivity**
Class of functions denotable in the 12,288 model.

**PAC Learning Bound**
Sample complexity bound using receipt dimension.

**Strong Normalization**
Termination guarantee under budget discipline.

**Type Inhabitation**
Existence of terms with given type at specified budget.

## Network Protocol

**Authenticated Message**
Network message carrying verifiable receipt and witness.

**Epidemic Broadcast**
Probabilistic message propagation with receipt confirmation.

**Lattice-Aware Protocol**
Network protocol exploiting lattice topology.

**Receipt-Coordinated Transaction**
Distributed transaction using receipts for coordination.

**Sybil Resistance**
Protection against fake identity attacks via receipts.

## Security Properties

**Collision Resistance**
Cryptographic hardness of finding address collisions.

**Information-Theoretic Security**
Security based on information theory rather than computational hardness.

**Memory Safety**
Absence of pointer errors through content addressing.

**Non-Interference**
Property that secret data doesn't affect public observations.

**Replay Immunity**
Protection against replay attacks through receipt binding.

## Research Frontiers

**Categorical Semantics**
Category-theoretic interpretation of Hologram model.

**Convexity Analysis**
Study of action landscape convexity properties.

**Embedding Theory**
How to embed other computational models in 12,288.

**Quantum Extensions**
Extending model to quantum computation via Φ.

**Stability Theory**
Analysis of fixed points and attractors in configuration space.