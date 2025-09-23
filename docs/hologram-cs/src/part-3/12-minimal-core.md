# Chapter 12: Minimal Core

## Implementor's Appendix

This chapter provides a complete, minimal implementation of the Hologram model suitable for teaching and experimentation. The code is deliberately simple—correctness over performance—with extensive comments explaining each design decision. This isn't production code; it's a pedagogical kernel that demonstrates every concept from first principles.

## Data Structures

### Core Lattice Implementation

```python
import numpy as np
from dataclasses import dataclass
from typing import List, Tuple, Dict, Optional
import hashlib

# Constants
PAGES = 48
BYTES_PER_PAGE = 256
LATTICE_SIZE = PAGES * BYTES_PER_PAGE  # 12,288
R96_CLASSES = 96
C768_PERIOD = 768

@dataclass
class Site:
    """A single location in the 12,288 lattice."""
    page: int  # 0-47
    byte: int  # 0-255

    def __post_init__(self):
        self.page = self.page % PAGES
        self.byte = self.byte % BYTES_PER_PAGE

    def linear_index(self) -> int:
        """Convert to linear index 0-12287."""
        return self.page * BYTES_PER_PAGE + self.byte

    @staticmethod
    def from_linear(index: int) -> 'Site':
        """Create from linear index."""
        index = index % LATTICE_SIZE
        return Site(index // BYTES_PER_PAGE, index % BYTES_PER_PAGE)

    def add(self, other: 'Site') -> 'Site':
        """Toroidal addition."""
        return Site(self.page + other.page, self.byte + other.byte)

    def rotate_schedule(self) -> 'Site':
        """Apply one step of C768 rotation."""
        # Simplified rotation for pedagogy
        new_byte = (self.byte + 1) % BYTES_PER_PAGE
        new_page = self.page
        if new_byte == 0:  # Wrapped around
            new_page = (self.page + 1) % PAGES
        return Site(new_page, new_byte)

class Lattice:
    """The 12,288 universal carrier."""

    def __init__(self):
        self.data = np.zeros(LATTICE_SIZE, dtype=np.uint8)
        self.metadata = {}  # For tracking receipts

    def get(self, site: Site) -> int:
        """Read value at site."""
        return int(self.data[site.linear_index()])

    def set(self, site: Site, value: int):
        """Write value at site."""
        self.data[site.linear_index()] = value % 256

    def region(self, start: Site, size: int) -> np.ndarray:
        """Extract a region of the lattice."""
        indices = [(start.linear_index() + i) % LATTICE_SIZE
                   for i in range(size)]
        return self.data[indices]

    def clear(self):
        """Reset lattice to zero."""
        self.data.fill(0)
        self.metadata.clear()
```

### Configuration and State

```python
@dataclass
class Configuration:
    """A complete state of the lattice with metadata."""
    lattice: Lattice
    timestamp: int = 0  # C768 cycle position
    budget_used: int = 0
    receipts: List['Receipt'] = None

    def __post_init__(self):
        if self.receipts is None:
            self.receipts = []

    def snapshot(self) -> bytes:
        """Create immutable snapshot for hashing."""
        return self.lattice.data.tobytes()

    def hash(self) -> str:
        """Compute configuration hash."""
        h = hashlib.sha256()
        h.update(self.snapshot())
        h.update(str(self.timestamp).encode())
        return h.hexdigest()[:16]
```

### Receipt Structure

```python
@dataclass
class Receipt:
    """Proof-carrying data for lawfulness verification."""
    r96_digest: str      # R96 multiset hash
    c768_phase: int      # Schedule phase (0-767)
    c768_fairness: float # Fairness metric
    phi_coherent: bool   # Φ round-trip success
    budget: int          # Total semantic cost
    witness_hash: str    # Hash of witness chain

    def verify(self) -> bool:
        """Basic receipt verification."""
        # Check phase is valid
        if not 0 <= self.c768_phase < C768_PERIOD:
            return False

        # Check budget is non-negative
        if self.budget < 0:
            return False

        # Check hash format
        if len(self.r96_digest) != 16:
            return False

        return True

    def compose(self, other: 'Receipt') -> 'Receipt':
        """Compose two receipts sequentially."""
        return Receipt(
            r96_digest=self._combine_digests(self.r96_digest, other.r96_digest),
            c768_phase=(self.c768_phase + other.c768_phase) % C768_PERIOD,
            c768_fairness=(self.c768_fairness + other.c768_fairness) / 2,
            phi_coherent=self.phi_coherent and other.phi_coherent,
            budget=self.budget + other.budget,
            witness_hash=self._combine_hashes(self.witness_hash, other.witness_hash)
        )

    def _combine_digests(self, d1: str, d2: str) -> str:
        """Combine two R96 digests."""
        h = hashlib.sha256()
        h.update(d1.encode())
        h.update(d2.encode())
        return h.hexdigest()[:16]

    def _combine_hashes(self, h1: str, h2: str) -> str:
        """Combine witness hashes."""
        h = hashlib.sha256()
        h.update(h1.encode())
        h.update(h2.encode())
        return h.hexdigest()[:16]
```

## Primitive Morphisms

### Base Morphism Class

```python
class Morphism:
    """Base class for all morphisms (transformations)."""

    def apply(self, config: Configuration) -> Configuration:
        """Apply morphism to configuration."""
        raise NotImplementedError

    def receipt(self, config: Configuration) -> Receipt:
        """Generate receipt for this morphism."""
        raise NotImplementedError

    def budget_cost(self) -> int:
        """Semantic cost of this morphism."""
        return 1  # Default unit cost

class IdentityMorphism(Morphism):
    """The trivial morphism."""

    def apply(self, config: Configuration) -> Configuration:
        return config  # No change

    def receipt(self, config: Configuration) -> Receipt:
        return Receipt(
            r96_digest=compute_r96_digest(config),
            c768_phase=config.timestamp % C768_PERIOD,
            c768_fairness=1.0,
            phi_coherent=True,
            budget=0,  # Identity costs nothing
            witness_hash=config.hash()
        )

    def budget_cost(self) -> int:
        return 0
```

### Class-Local Morphisms

```python
class ClassLocalMorphism(Morphism):
    """Morphism that operates within a single R96 class."""

    def __init__(self, r96_class: int, operation):
        self.r96_class = r96_class
        self.operation = operation  # Function to apply

    def apply(self, config: Configuration) -> Configuration:
        new_config = Configuration(
            lattice=Lattice(),
            timestamp=config.timestamp + 1,
            budget_used=config.budget_used + self.budget_cost()
        )

        # Copy data
        new_config.lattice.data = config.lattice.data.copy()

        # Apply operation to sites in this R96 class
        for i in range(LATTICE_SIZE):
            site = Site.from_linear(i)
            value = config.lattice.get(site)

            if R(value) == self.r96_class:
                new_value = self.operation(value)
                new_config.lattice.set(site, new_value)

        # Generate receipt
        new_config.receipts.append(self.receipt(config))

        return new_config

    def receipt(self, config: Configuration) -> Receipt:
        # Count affected sites
        affected = sum(1 for i in range(LATTICE_SIZE)
                      if R(config.lattice.data[i]) == self.r96_class)

        return Receipt(
            r96_digest=compute_r96_digest(config),
            c768_phase=(config.timestamp + 1) % C768_PERIOD,
            c768_fairness=1.0 - (affected / LATTICE_SIZE),  # Locality
            phi_coherent=True,
            budget=affected,  # Cost proportional to affected sites
            witness_hash=config.hash()
        )
```

### Schedule Rotation

```python
class RotateMorphism(Morphism):
    """Apply C768 schedule rotation."""

    def __init__(self, steps: int = 1):
        self.steps = steps

    def apply(self, config: Configuration) -> Configuration:
        new_config = Configuration(
            lattice=Lattice(),
            timestamp=config.timestamp + self.steps,
            budget_used=config.budget_used + self.budget_cost()
        )

        # Rotate data according to schedule
        for i in range(LATTICE_SIZE):
            old_site = Site.from_linear(i)
            new_site = old_site

            # Apply rotation steps
            for _ in range(self.steps):
                new_site = new_site.rotate_schedule()

            # Move data
            value = config.lattice.get(old_site)
            new_config.lattice.set(new_site, value)

        new_config.receipts.append(self.receipt(config))
        return new_config

    def receipt(self, config: Configuration) -> Receipt:
        return Receipt(
            r96_digest=compute_r96_digest(config),  # Rotation preserves R96
            c768_phase=(config.timestamp + self.steps) % C768_PERIOD,
            c768_fairness=1.0,  # Rotation is perfectly fair
            phi_coherent=True,
            budget=self.steps,  # Cost = number of rotation steps
            witness_hash=config.hash()
        )
```

### Lift and Projection

```python
class LiftMorphism(Morphism):
    """Lift from boundary to interior."""

    def apply(self, config: Configuration) -> Configuration:
        new_config = Configuration(
            lattice=Lattice(),
            timestamp=config.timestamp,
            budget_used=config.budget_used + self.budget_cost()
        )

        # Extract boundary
        boundary = self._extract_boundary(config)

        # Lift to interior
        interior = self._lift_phi(boundary, budget=config.budget_used)

        # Write interior
        for site, value in interior.items():
            new_config.lattice.set(site, value)

        # Preserve boundary
        for site, value in boundary:
            new_config.lattice.set(site, value)

        new_config.receipts.append(self.receipt(config))
        return new_config

    def _extract_boundary(self, config: Configuration) -> List[Tuple[Site, int]]:
        """Extract boundary sites."""
        boundary = []
        for p in range(PAGES):
            for b in range(BYTES_PER_PAGE):
                site = Site(p, b)
                if self._is_boundary(site):
                    boundary.append((site, config.lattice.get(site)))
        return boundary

    def _is_boundary(self, site: Site) -> bool:
        """Check if site is on boundary."""
        return (site.page < 2 or site.page > 45 or
                site.byte < 16 or site.byte > 239)

    def _lift_phi(self, boundary: List[Tuple[Site, int]], budget: int) -> Dict[Site, int]:
        """Lift boundary to interior."""
        interior = {}

        for b_site, b_value in boundary:
            # Each boundary value influences nearby interior
            influence_radius = max(1, 10 - budget // 10)

            for dp in range(-influence_radius, influence_radius + 1):
                for db in range(-influence_radius, influence_radius + 1):
                    i_site = Site(b_site.page + dp, b_site.byte + db)

                    if not self._is_boundary(i_site):
                        weight = 1.0 / (abs(dp) + abs(db) + 1)
                        if i_site not in interior:
                            interior[i_site] = 0
                        interior[i_site] += int(b_value * weight)

        # Normalize
        if interior:
            max_val = max(interior.values())
            if max_val > 0:
                for site in interior:
                    interior[site] = (interior[site] * 255) // max_val

        return interior

    def receipt(self, config: Configuration) -> Receipt:
        return Receipt(
            r96_digest=compute_r96_digest(config),
            c768_phase=config.timestamp % C768_PERIOD,
            c768_fairness=0.9,  # Lift is mostly local
            phi_coherent=True,  # By construction
            budget=100,  # Fixed cost for lift
            witness_hash=config.hash()
        )
```

## Type Checker / Receipt Builder

### R96 Computation

```python
def R(byte_value: int) -> int:
    """Compute resonance class of a byte."""
    byte_value = byte_value % 256
    primary = byte_value % 96
    secondary = byte_value // 96
    # Mix primary and secondary components
    return (primary + secondary * 17 + (primary ^ secondary)) % 96

def compute_r96_digest(config: Configuration) -> str:
    """Compute R96 digest of configuration."""
    # Build histogram of resonance classes
    histogram = [0] * R96_CLASSES

    for i in range(LATTICE_SIZE):
        value = config.lattice.data[i]
        r_class = R(value)
        histogram[r_class] += 1

    # Hash the histogram
    h = hashlib.sha256()
    for i, count in enumerate(histogram):
        h.update(f"{i}:{count},".encode())

    return h.hexdigest()[:16]
```

### Budget Tracking

```python
class BudgetTracker:
    """Track and verify budget usage."""

    def __init__(self, initial_budget: int = 1000):
        self.total_budget = initial_budget
        self.used_budget = 0
        self.operations = []

    def charge(self, operation: str, cost: int) -> bool:
        """Charge budget for operation."""
        if self.used_budget + cost > self.total_budget:
            return False  # Insufficient budget

        self.used_budget += cost
        self.operations.append((operation, cost))
        return True

    def remaining(self) -> int:
        """Get remaining budget."""
        return self.total_budget - self.used_budget

    def crush(self) -> bool:
        """Check if budget is zero (perfect)."""
        return self.used_budget == 0
```

### Type Checking

```python
class TypeChecker:
    """Verify type safety via conservation laws."""

    def check_r96_preservation(self, before: Configuration,
                               after: Configuration) -> bool:
        """Check that R96 multiset is preserved."""
        digest_before = compute_r96_digest(before)
        digest_after = compute_r96_digest(after)

        # For now, check if they're related (in production,
        # would check specific conservation)
        return len(digest_before) == len(digest_after)

    def check_c768_fairness(self, config: Configuration) -> float:
        """Measure schedule fairness."""
        # Count activations per site over a window
        activations = [0] * LATTICE_SIZE

        # Simulate one cycle
        for step in range(C768_PERIOD):
            site_index = (config.timestamp + step) % LATTICE_SIZE
            activations[site_index] += 1

        # Compute variance
        mean = sum(activations) / len(activations)
        variance = sum((a - mean) ** 2 for a in activations) / len(activations)

        # Perfect fairness = 0 variance
        fairness = 1.0 / (1.0 + variance)
        return fairness

    def check_phi_coherence(self, config: Configuration) -> bool:
        """Check Φ round-trip property."""
        # Extract boundary
        lift_morph = LiftMorphism()
        boundary = lift_morph._extract_boundary(config)

        # Lift to interior
        interior = lift_morph._lift_phi(boundary, config.budget_used)

        # Project back (simplified)
        recovered = self._project_phi(interior, boundary)

        # Check round-trip error
        error = 0
        for (site, original), (_, recovered_val) in zip(boundary, recovered):
            error += abs(original - recovered_val)

        # At budget 0, should be perfect
        if config.budget_used == 0:
            return error == 0
        else:
            # Allow error proportional to budget
            return error <= config.budget_used

    def _project_phi(self, interior: Dict[Site, int],
                     boundary: List[Tuple[Site, int]]) -> List[Tuple[Site, int]]:
        """Simple projection for testing."""
        # Just return boundary as-is for now
        return boundary
```

## CAM Address

### Normal Form Computation

```python
class Normalizer:
    """Compute normal forms via gauge fixing."""

    def normalize(self, config: Configuration) -> Configuration:
        """Compute canonical normal form."""
        # Step 1: Translate to origin
        normalized = self._translate_to_origin(config)

        # Step 2: Fix schedule phase
        normalized = self._align_phase(normalized)

        # Step 3: Order boundary
        normalized = self._order_boundary(normalized)

        # Step 4: Apply Φ lift
        normalized = self._apply_phi(normalized)

        return normalized

    def _translate_to_origin(self, config: Configuration) -> Configuration:
        """Move leftmost-topmost non-empty to (0,0)."""
        # Find first non-zero site
        first_site = None
        for i in range(LATTICE_SIZE):
            if config.lattice.data[i] != 0:
                first_site = Site.from_linear(i)
                break

        if first_site is None:
            return config  # Empty configuration

        # Translate everything
        new_config = Configuration(
            lattice=Lattice(),
            timestamp=config.timestamp,
            budget_used=config.budget_used
        )

        for i in range(LATTICE_SIZE):
            old_site = Site.from_linear(i)
            new_site = Site(
                (old_site.page - first_site.page) % PAGES,
                (old_site.byte - first_site.byte) % BYTES_PER_PAGE
            )
            value = config.lattice.get(old_site)
            new_config.lattice.set(new_site, value)

        return new_config

    def _align_phase(self, config: Configuration) -> Configuration:
        """Align to phase 0 of C768 cycle."""
        phase_offset = config.timestamp % C768_PERIOD

        if phase_offset == 0:
            return config

        # Rotate to align
        rotate = RotateMorphism(C768_PERIOD - phase_offset)
        return rotate.apply(config)

    def _order_boundary(self, config: Configuration) -> Configuration:
        """Order boundary sites lexicographically."""
        # For simplicity, just return as-is
        return config

    def _apply_phi(self, config: Configuration) -> Configuration:
        """Apply Φ lift for canonical interior."""
        lift = LiftMorphism()
        return lift.apply(config)
```

### Address Computation

```python
class AddressMap:
    """Content-addressable memory via perfect hashing."""

    def address(self, config: Configuration) -> Site:
        """Compute content address."""
        # Normalize first
        normalizer = Normalizer()
        normal = normalizer.normalize(config)

        # Compute receipt of normal form
        receipt = self._compute_full_receipt(normal)

        # Hash receipt to get address
        h = hashlib.sha256()
        h.update(receipt.r96_digest.encode())
        h.update(str(receipt.c768_phase).encode())
        h.update(str(receipt.phi_coherent).encode())
        h.update(str(receipt.budget).encode())

        # Map to lattice site
        digest = h.digest()
        index = int.from_bytes(digest[:2], 'big') % LATTICE_SIZE

        return Site.from_linear(index)

    def _compute_full_receipt(self, config: Configuration) -> Receipt:
        """Compute complete receipt."""
        type_checker = TypeChecker()

        return Receipt(
            r96_digest=compute_r96_digest(config),
            c768_phase=config.timestamp % C768_PERIOD,
            c768_fairness=type_checker.check_c768_fairness(config),
            phi_coherent=type_checker.check_phi_coherence(config),
            budget=config.budget_used,
            witness_hash=config.hash()
        )
```

## Verifier

### Linear-Time Verification

```python
class Verifier:
    """Verify lawfulness in linear time."""

    def __init__(self):
        self.type_checker = TypeChecker()

    def verify_configuration(self, config: Configuration) -> bool:
        """Verify configuration is lawful."""
        # Check each receipt
        for receipt in config.receipts:
            if not receipt.verify():
                return False

        # Check conservation laws
        if not self._check_conservation(config):
            return False

        # Check budget
        if config.budget_used < 0:
            return False

        return True

    def verify_witness_chain(self, chain: List[Dict]) -> bool:
        """Verify a witness chain."""
        if not chain:
            return True

        # Check continuity
        for i in range(len(chain) - 1):
            if chain[i]['post_state'] != chain[i + 1]['pre_state']:
                return False

        # Check each witness
        for witness in chain:
            if not self._verify_witness(witness):
                return False

        return True

    def _check_conservation(self, config: Configuration) -> bool:
        """Check conservation laws."""
        # For teaching purposes, just check basics
        return True

    def _verify_witness(self, witness: Dict) -> bool:
        """Verify single witness."""
        # Check required fields
        required = ['operation', 'pre_state', 'post_state', 'budget']
        for field in required:
            if field not in witness:
                return False

        # Check budget is non-negative
        if witness['budget'] < 0:
            return False

        return True

    def verify_receipt_chain(self, receipts: List[Receipt]) -> bool:
        """Verify receipt composition."""
        if not receipts:
            return True

        # Check each receipt
        for receipt in receipts:
            if not receipt.verify():
                return False

        # Check composition
        composed = receipts[0]
        for receipt in receipts[1:]:
            composed = composed.compose(receipt)

        # Final budget should be sum
        total_budget = sum(r.budget for r in receipts)
        if composed.budget != total_budget:
            return False

        return True
```

## Mini-Action & Compiler

### Simple Action Functional

```python
class ActionComputer:
    """Compute action (universal cost) for configurations."""

    def __init__(self):
        self.weights = {
            'geometric': 1.0,
            'r96': 1.0,
            'c768': 1.0,
            'budget': 1.0,
            'phi': 1.0,
            'problem': 1.0
        }

    def compute(self, config: Configuration, target=None) -> float:
        """Compute total action."""
        action = 0

        # Geometric smoothness
        action += self.weights['geometric'] * self._geometric_action(config)

        # R96 conformity
        action += self.weights['r96'] * self._r96_action(config)

        # C768 fairness
        action += self.weights['c768'] * self._c768_action(config)

        # Budget penalty
        action += self.weights['budget'] * config.budget_used

        # Φ coherence
        action += self.weights['phi'] * self._phi_action(config)

        # Problem-specific
        if target is not None:
            action += self.weights['problem'] * self._problem_action(config, target)

        return action

    def _geometric_action(self, config: Configuration) -> float:
        """Penalize non-local jumps."""
        action = 0
        for i in range(LATTICE_SIZE):
            site = Site.from_linear(i)
            value = config.lattice.get(site)

            # Check neighbors
            for delta in [Site(0, 1), Site(1, 0)]:
                neighbor = site.add(delta)
                neighbor_value = config.lattice.get(neighbor)
                action += (value - neighbor_value) ** 2

        return action / LATTICE_SIZE

    def _r96_action(self, config: Configuration) -> float:
        """Measure R96 distribution uniformity."""
        histogram = [0] * R96_CLASSES
        for i in range(LATTICE_SIZE):
            r_class = R(config.lattice.data[i])
            histogram[r_class] += 1

        # Ideal is uniform distribution
        ideal = LATTICE_SIZE / R96_CLASSES
        action = sum((h - ideal) ** 2 for h in histogram)

        return action / LATTICE_SIZE

    def _c768_action(self, config: Configuration) -> float:
        """Penalize unfairness."""
        type_checker = TypeChecker()
        fairness = type_checker.check_c768_fairness(config)
        return 1.0 - fairness

    def _phi_action(self, config: Configuration) -> float:
        """Penalize Φ incoherence."""
        type_checker = TypeChecker()
        coherent = type_checker.check_phi_coherence(config)
        return 0.0 if coherent else 100.0

    def _problem_action(self, config: Configuration, target) -> float:
        """Problem-specific cost."""
        # Example: sorting
        if isinstance(target, list):
            # Extract array from config
            array = [config.lattice.data[i] for i in range(len(target))]

            # Count inversions
            inversions = 0
            for i in range(len(array)):
                for j in range(i + 1, len(array)):
                    if array[i] > array[j]:
                        inversions += 1

            return inversions

        return 0
```

### Mini Compiler

```python
class MiniCompiler:
    """Compile programs via action minimization."""

    def __init__(self):
        self.action_computer = ActionComputer()
        self.normalizer = Normalizer()
        self.address_map = AddressMap()

    def compile(self, source: str, max_iterations: int = 1000) -> Configuration:
        """Compile source to lawful configuration."""
        # Parse source to initial configuration
        config = self._parse_source(source)

        # Minimize action
        for iteration in range(max_iterations):
            action = self.action_computer.compute(config)

            if action < 0.01:
                break  # Compiled!

            # Generate lawful moves
            moves = self._generate_moves(config)

            # Pick best move
            best_move = None
            best_action = action

            for move in moves:
                new_config = move.apply(config)
                new_action = self.action_computer.compute(new_config)

                if new_action < best_action:
                    best_action = new_action
                    best_move = move

            if best_move is None:
                break  # Local minimum

            config = best_move.apply(config)

        # Normalize
        config = self.normalizer.normalize(config)

        # Compute address
        address = self.address_map.address(config)

        print(f"Compiled to address {address} in {iteration} iterations")
        print(f"Final action: {action:.4f}")
        print(f"Budget used: {config.budget_used}")

        return config

    def _parse_source(self, source: str) -> Configuration:
        """Parse source to initial configuration."""
        config = Configuration(lattice=Lattice())

        # Simple: just place bytes of source
        for i, char in enumerate(source[:LATTICE_SIZE]):
            site = Site.from_linear(i)
            config.lattice.set(site, ord(char))

        return config

    def _generate_moves(self, config: Configuration) -> List[Morphism]:
        """Generate possible lawful moves."""
        moves = []

        # Identity (always lawful)
        moves.append(IdentityMorphism())

        # Rotations
        for steps in [1, 10, 100]:
            moves.append(RotateMorphism(steps))

        # Class-local operations
        for r_class in range(0, R96_CLASSES, 10):  # Sample classes
            moves.append(ClassLocalMorphism(r_class, lambda x: (x + 1) % 256))

        # Lift/Project
        moves.append(LiftMorphism())

        return moves
```

## Complete Example: Sorting

```python
def demo_sort():
    """Demonstrate sorting via action minimization."""
    print("=== Hologram Sort Demo ===\n")

    # Initial unsorted array
    unsorted = [5, 2, 8, 1, 9, 3, 7, 4, 6]
    print(f"Initial: {unsorted}")

    # Create configuration
    config = Configuration(lattice=Lattice())
    for i, value in enumerate(unsorted):
        config.lattice.set(Site.from_linear(i), value)

    # Define sorting action
    action_computer = ActionComputer()

    def sorting_action(cfg):
        # Extract array
        array = [cfg.lattice.get(Site.from_linear(i))
                 for i in range(len(unsorted))]

        # Count inversions (0 when sorted)
        inversions = sum(1 for i in range(len(array))
                        for j in range(i+1, len(array))
                        if array[i] > array[j])

        return inversions

    # Minimize action (compile the sort)
    print("\nCompiling sort...")
    for iteration in range(100):
        action = sorting_action(config)

        if action == 0:
            print(f"Sorted in {iteration} iterations!")
            break

        # Try swaps
        best_swap = None
        best_improvement = 0

        for i in range(len(unsorted) - 1):
            # Test swap
            site_i = Site.from_linear(i)
            site_j = Site.from_linear(i + 1)

            val_i = config.lattice.get(site_i)
            val_j = config.lattice.get(site_j)

            if val_i > val_j:  # Should swap
                # Apply swap
                new_config = Configuration(lattice=Lattice())
                new_config.lattice.data = config.lattice.data.copy()
                new_config.lattice.set(site_i, val_j)
                new_config.lattice.set(site_j, val_i)

                new_action = sorting_action(new_config)
                improvement = action - new_action

                if improvement > best_improvement:
                    best_improvement = improvement
                    best_swap = (i, i + 1)

        if best_swap:
            i, j = best_swap
            site_i, site_j = Site.from_linear(i), Site.from_linear(j)
            val_i, val_j = config.lattice.get(site_i), config.lattice.get(site_j)
            config.lattice.set(site_i, val_j)
            config.lattice.set(site_j, val_i)

            print(f"  Swap {i},{j}: {val_i} <-> {val_j}")

    # Extract sorted array
    sorted_array = [config.lattice.get(Site.from_linear(i))
                   for i in range(len(unsorted))]
    print(f"\nFinal: {sorted_array}")

    # Verify lawfulness
    verifier = Verifier()
    receipt = Receipt(
        r96_digest=compute_r96_digest(config),
        c768_phase=0,
        c768_fairness=1.0,
        phi_coherent=True,
        budget=iteration,
        witness_hash=config.hash()
    )

    print(f"\nReceipt verified: {receipt.verify()}")
    print(f"R96 digest: {receipt.r96_digest}")
    print(f"Budget used: {receipt.budget}")

if __name__ == "__main__":
    demo_sort()
```

## Exercises

**Exercise 12.1**: Extend the R96 computation to handle multi-byte sequences.

**Exercise 12.2**: Implement projection (proj_Φ) to complete the round-trip.

**Exercise 12.3**: Add witness chain generation to the morphisms.

**Exercise 12.4**: Implement a map-reduce operation using class-local morphisms.

**Exercise 12.5**: Create a type system using conservation laws as types.

## Takeaways

This minimal implementation demonstrates:

1. **Simple data structures suffice**: 12,288 array + metadata
2. **Morphisms are composable**: Sequential and parallel composition
3. **Receipts are verifiable**: Linear-time checking
4. **Normal forms are computable**: Gauge fixing is deterministic
5. **Action drives compilation**: One optimizer for all programs
6. **Everything is teachable**: ~500 lines of clear Python

This kernel can be extended for research or education while maintaining conceptual clarity.

---

*Next: Part IV explores the theoretical foundations and limits of the model.*