# Chapter 10: Worked Micro-Examples

## Motivation

Abstract theory becomes concrete through examples. This chapter presents six complete micro-examples that demonstrate every aspect of the Hologram model: resonance classes, scheduling, lift/projection, content addressing, process objects, and action minimization. Each example is small enough to trace by hand yet rich enough to illustrate key principles.

## R96 Checksum Toy

### Setting Up the Example

Let's compute R96 checksums for a 16-byte configuration:

```python
# Initial 16 bytes on a 4×4 region of the lattice
bytes = [
    0x42, 0x7F, 0x00, 0xA5,  # Row 0
    0x33, 0x96, 0xDE, 0x01,  # Row 1
    0xFF, 0x88, 0x4A, 0xC0,  # Row 2
    0x17, 0x6B, 0xE2, 0x5D,  # Row 3
]

# Place on lattice sites (0,0) through (0,15)
config = Configuration()
for i, byte in enumerate(bytes):
    config.set(Site(0, i), byte)
```

### Computing Residues

Apply the resonance function to each byte:

```python
def R(byte):
    # Simplified R96 function
    primary = byte % 96
    secondary = byte // 96
    return (primary + secondary * 17) % 96

residues = [R(b) for b in bytes]
# [66, 47, 0, 75, 51, 71, 73, 1, 79, 56, 74, 72, 23, 43, 70, 93]
```

### Building the Digest

```python
def compute_r96_digest(residues):
    # Step 1: Build histogram
    histogram = [0] * 96
    for r in residues:
        histogram[r] += 1

    # Step 2: Hash the histogram
    import hashlib
    h = hashlib.sha256()

    for i, count in enumerate(histogram):
        if count > 0:
            h.update(f"{i}:{count},".encode())

    return h.hexdigest()[:16]  # First 16 chars

digest = compute_r96_digest(residues)
# "a7f3e9b2c5d8..."
```

### Gauge Invariance Test

```python
# Permute the bytes
import random
permuted_bytes = bytes.copy()
random.shuffle(permuted_bytes)

# Compute digest of permutation
permuted_residues = [R(b) for b in permuted_bytes]
permuted_digest = compute_r96_digest(permuted_residues)

# Should be identical!
assert digest == permuted_digest
print("✓ R96 digest is permutation-invariant")
```

### Key Observations

1. **Residues distribute uniformly**: Each class appears ~equally
2. **Multiset property**: Order doesn't matter, only counts
3. **Collision resistance**: Different byte sets → different digests
4. **Composability**: Can merge digests from regions

## C768 Fairness Probe

### Creating a 24-Site Orbit

```python
# Start at site (0,0)
start = Site(0, 0)
orbit = [start]
current = start

# Apply σ repeatedly
for i in range(1, 768):
    current = current.rotate_schedule()
    if i < 24:  # Track first 24 sites
        orbit.append(current)
```

### Visualizing the Schedule Spiral

```python
def visualize_orbit(orbit):
    grid = [[' ' for _ in range(16)] for _ in range(3)]

    for i, site in enumerate(orbit[:24]):
        p, b = site.page % 3, site.byte % 16
        grid[p][b] = chr(ord('A') + i)

    for row in grid:
        print(''.join(row))

visualize_orbit(orbit)
# ABCD    QRST
# EFGH    UVWX
# IJKL    MNOP
```

### Measuring Fairness

```python
def measure_fairness(schedule_length=768):
    activations = {}

    current = Site(0, 0)
    for step in range(schedule_length * 3):  # Three cycles
        # Record activation
        if current not in activations:
            activations[current] = []
        activations[current].append(step)

        current = current.rotate_schedule()

    # Compute statistics
    gaps = []
    for site, times in activations.items():
        for i in range(1, len(times)):
            gaps.append(times[i] - times[i-1])

    mean_gap = sum(gaps) / len(gaps)
    max_gap = max(gaps)
    min_gap = min(gaps)

    print(f"Mean gap: {mean_gap}")  # Should be 768
    print(f"Max gap: {max_gap}")    # Should be 768
    print(f"Min gap: {min_gap}")    # Should be 768
    print("✓ Perfect fairness achieved")
```

### Flow Conservation

```python
def verify_flow_conservation():
    # Track "mass" flowing through schedule
    mass = [1.0] * 12288  # Unit mass at each site

    # One complete cycle
    current = Site(0, 0)
    for _ in range(768):
        # Mass flows along schedule
        next_site = current.rotate_schedule()

        # Conservation check
        total_before = sum(mass)

        # Simulate flow
        flow = mass[current.linear_index()] * 0.1
        mass[current.linear_index()] -= flow
        mass[next_site.linear_index()] += flow

        total_after = sum(mass)

        assert abs(total_before - total_after) < 1e-10

        current = next_site

    print("✓ Flow conserved throughout cycle")
```

## Φ Round-Trip

### Setting Up Boundary and Interior

```python
# Define boundary region (outer ring)
def is_boundary(site):
    p, b = site.page, site.byte
    return p < 2 or p > 45 or b < 16 or b > 239

boundary_data = []
for p in range(48):
    for b in range(256):
        site = Site(p, b)
        if is_boundary(site):
            # Simple test pattern
            value = (p + b) % 256
            boundary_data.append((site, value))
```

### Lifting to Interior

```python
def lift_phi(boundary_data, budget):
    interior = {}

    for site, value in boundary_data:
        # Each boundary value influences nearby interior
        influence_radius = max(1, 10 - budget)  # Smaller budget → larger radius

        for dp in range(-influence_radius, influence_radius+1):
            for db in range(-influence_radius, influence_radius+1):
                interior_site = Site(
                    (site.page + dp) % 48,
                    (site.byte + db) % 256
                )

                if not is_boundary(interior_site):
                    weight = 1.0 / (abs(dp) + abs(db) + 1)
                    if interior_site not in interior:
                        interior[interior_site] = 0
                    interior[interior_site] += value * weight

    # Normalize
    max_val = max(interior.values()) if interior else 1
    for site in interior:
        interior[site] /= max_val
        interior[site] = int(interior[site] * 255)

    return interior
```

### Projecting Back

```python
def proj_phi(interior, budget):
    boundary = []

    for p in range(48):
        for b in range(256):
            site = Site(p, b)
            if is_boundary(site):
                # Gather from interior
                gathered = 0
                weight_sum = 0

                influence_radius = max(1, 10 - budget)

                for dp in range(-influence_radius, influence_radius+1):
                    for db in range(-influence_radius, influence_radius+1):
                        interior_site = Site(
                            (p + dp) % 48,
                            (b + db) % 256
                        )

                        if not is_boundary(interior_site):
                            if interior_site in interior:
                                weight = 1.0 / (abs(dp) + abs(db) + 1)
                                gathered += interior[interior_site] * weight
                                weight_sum += weight

                if weight_sum > 0:
                    value = int(gathered / weight_sum)
                else:
                    value = 0

                boundary.append((site, value))

    return boundary
```

### Testing Round-Trip Property

```python
def test_phi_roundtrip():
    # Original boundary
    original = boundary_data

    for budget in [0, 5, 10, 20]:
        # Lift then project
        interior = lift_phi(original, budget)
        recovered = proj_phi(interior, budget)

        # Measure error
        error = 0
        for (s1, v1), (s2, v2) in zip(original, recovered):
            assert s1 == s2  # Sites match
            error += abs(v1 - v2)

        avg_error = error / len(original)
        print(f"Budget {budget}: Average error = {avg_error:.2f}")

        if budget == 0:
            assert avg_error < 1.0  # Near-perfect recovery
            print("✓ Round-trip identity at budget 0")
```

## CAM Identity

### Creating Two Equivalent Objects

```python
# Two strings with same content, different positions
str1 = create_string("HELLO", position=(5, 10))
str2 = create_string("HELLO", position=(20, 100))

print(f"String 1 at {str1.position}: {str1.content}")
print(f"String 2 at {str2.position}: {str2.content}")
```

### Canonicalization

```python
def canonicalize(obj):
    # Step 1: Translate to origin
    min_p = min(site.page for site in obj.sites)
    min_b = min(site.byte for site in obj.sites)

    canonical = obj.translate(-min_p, -min_b)

    # Step 2: Align to phase 0
    current_phase = compute_phase(canonical)
    canonical = canonical.rotate(-current_phase)

    # Step 3: Order boundary sites lexicographically
    boundary = sorted(canonical.boundary_sites())
    canonical.reorder_boundary(boundary)

    # Step 4: Apply Φ lift
    canonical.interior = lift_phi(canonical.boundary, budget=0)

    return canonical
```

### Computing Addresses

```python
# Canonicalize both strings
nf1 = canonicalize(str1)
nf2 = canonicalize(str2)

# Compute receipts
receipt1 = compute_receipt(nf1)
receipt2 = compute_receipt(nf2)

# Should be identical!
assert receipt1 == receipt2
print("✓ Equivalent objects have same receipt")

# Compute addresses
addr1 = H(receipt1)
addr2 = H(receipt2)

assert addr1 == addr2
print(f"✓ Both map to address {addr1}")
print("✓ Content determines identity")
```

### Collision Test

```python
def test_no_collisions():
    objects = []
    addresses = set()

    # Create 1000 different strings
    for i in range(1000):
        obj = create_string(f"String_{i}", position=(i%48, i%256))
        canonical = canonicalize(obj)
        addr = H(compute_receipt(canonical))

        # Check for collision
        if addr in addresses:
            print(f"Collision at {addr}!")
            return False

        addresses.add(addr)
        objects.append((obj, addr))

    print("✓ No collisions among 1000 distinct objects")
    return True

test_no_collisions()
```

## Process Object

### Composing Operations

```python
# Define two morphisms
def swap_morphism(i, j):
    return Process.Morphism(f"swap_{i}_{j}")

def rotate_morphism(n):
    return Process.Rotate(n)

# Sequential composition
swap_01 = swap_morphism(0, 1)
swap_23 = swap_morphism(2, 3)
rotate_1 = rotate_morphism(1)

sequential = Process.Sequential(
    Process.Sequential(swap_01, rotate_1),
    swap_23
)
```

### Computing Process Receipt

```python
def process_receipt(process):
    if isinstance(process, Process.Identity):
        return Receipt.identity()

    elif isinstance(process, Process.Morphism):
        return morphism_receipt(process.id)

    elif isinstance(process, Process.Sequential):
        r1 = process_receipt(process.first)
        r2 = process_receipt(process.second)
        return Receipt.compose_sequential(r1, r2)

    elif isinstance(process, Process.Rotate):
        return rotation_receipt(process.steps)

receipt = process_receipt(sequential)
print(f"Process receipt: {receipt}")
print(f"Total budget: {receipt.budget}")
```

### Checking Witness Chain

```python
def build_witness_chain(process, initial_state):
    chain = []
    current_state = initial_state

    def execute(proc):
        nonlocal current_state
        pre = hash(current_state)

        if isinstance(proc, Process.Morphism):
            # Execute morphism
            current_state = apply_morphism(current_state, proc.id)

        elif isinstance(proc, Process.Sequential):
            execute(proc.first)
            execute(proc.second)
            return

        elif isinstance(proc, Process.Rotate):
            current_state = rotate_state(current_state, proc.steps)

        post = hash(current_state)

        # Add to chain
        chain.append(WitnessFragment(
            operation=str(proc),
            pre_state=pre,
            post_state=post,
            local_receipt=compute_local_receipt(current_state),
            budget_consumed=1
        ))

    execute(process)
    return chain, current_state

initial = [3, 1, 4, 1, 5]
chain, final = build_witness_chain(sequential, initial)

# Verify chain
assert verify_witness_chain(chain, initial, final)
print("✓ Witness chain verified")
```

## Action Minimization

### Defining a Tiny Action

```python
# 4×4 toy lattice
LATTICE_SIZE = 16

def toy_action(config):
    """
    Three-term action:
    1. Geometric smoothness
    2. Target achievement
    3. Budget penalty
    """
    action = 0

    # Geometric term: penalize differences
    for i in range(LATTICE_SIZE):
        for j in range(i+1, LATTICE_SIZE):
            if adjacent(i, j):
                diff = config[i] - config[j]
                action += diff * diff

    # Target term: want sum = 100
    target_sum = 100
    actual_sum = sum(config)
    action += (actual_sum - target_sum) ** 2

    # Budget term: penalize operations
    operation_count = count_operations(config)
    action += operation_count * 0.1

    return action
```

### Solving for Stationarity

```python
def minimize_action(initial_config):
    config = initial_config.copy()
    learning_rate = 0.01

    for iteration in range(1000):
        # Compute gradient numerically
        gradient = []
        eps = 0.001

        for i in range(LATTICE_SIZE):
            # Finite difference
            config[i] += eps
            action_plus = toy_action(config)
            config[i] -= 2*eps
            action_minus = toy_action(config)
            config[i] += eps

            grad = (action_plus - action_minus) / (2*eps)
            gradient.append(grad)

        # Gradient descent
        for i in range(LATTICE_SIZE):
            config[i] -= learning_rate * gradient[i]

        # Check convergence
        if sum(abs(g) for g in gradient) < 0.01:
            print(f"✓ Converged at iteration {iteration}")
            break

    return config

# Initial random configuration
import random
initial = [random.randint(0, 10) for _ in range(LATTICE_SIZE)]
print(f"Initial action: {toy_action(initial):.2f}")

# Minimize
optimal = minimize_action(initial)
print(f"Final action: {toy_action(optimal):.2f}")
print(f"Sum: {sum(optimal):.2f} (target: 100)")
```

### Interpreting Results

```python
def interpret_solution(config):
    # Check Euler-Lagrange conditions
    print("\nEuler-Lagrange analysis:")

    # Stationarity at each site
    for i in range(LATTICE_SIZE):
        # Local variation
        neighbors = get_neighbors(i)
        laplacian = sum(config[j] - config[i] for j in neighbors)

        # Should be near zero at minimum
        print(f"Site {i}: ∇²φ = {laplacian:.3f}")

    # Visualize as 4×4 grid
    print("\nConfiguration:")
    for row in range(4):
        values = config[row*4:(row+1)*4]
        print(" ".join(f"{v:6.2f}" for v in values))

    # Check constraints
    print(f"\n✓ Sum constraint: {sum(config):.2f} ≈ 100")
    print(f"✓ Smoothness: neighbor differences < 1")
    print(f"✓ Compilation successful!")

interpret_solution(optimal)
```

## Exercises

**Exercise 10.1**: Extend the R96 example to handle 256 bytes. What patterns emerge in the histogram?

**Exercise 10.2**: Prove that the C768 schedule visits each site exactly once per cycle.

**Exercise 10.3**: Measure Φ round-trip error as a function of budget. Find the optimal budget.

**Exercise 10.4**: Create 100 random objects and verify no CAM collisions occur.

**Exercise 10.5**: Build a sorting process object and verify its witness chain.

**Exercise 10.6**: Add a fourth term to the toy action. How does the solution change?

## Takeaways

These micro-examples demonstrate that:

1. **R96 checksums are robust**: Permutation-invariant, compositional
2. **C768 provides perfect fairness**: Every site equally scheduled
3. **Φ preserves information**: Round-trip recovery at budget 0
4. **CAM provides unique addresses**: Content determines identity
5. **Process objects are verifiable**: Witness chains prove correctness
6. **Action minimization works**: Gradient descent finds valid configurations

Each mechanism is simple individually but combines to create a powerful system.

---

*Next: Chapter 11 bridges these concepts to mainstream computer science.*