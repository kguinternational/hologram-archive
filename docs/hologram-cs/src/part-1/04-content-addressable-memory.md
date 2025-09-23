# Chapter 4: Content-Addressable Memory

## Motivation

Traditional memory systems use arbitrary addresses—pointers that have no relationship to the data they reference. This creates fundamental problems: dangling pointers, buffer overflows, cache misses, and the entire machinery of memory management.

The Hologram model takes a radical approach: addresses ARE the content. More precisely, the address of an object is a mathematical function of its receipts and normal form. This gives us perfect hashing on the lawful domain—no collisions, no collision resolution, no load factors. Memory safety isn't added through checks and bounds; it's intrinsic to the addressing scheme.

## Lawful Domain of Addressability

### What Can Be Addressed?

Not everything deserves an address. In the Hologram model, only lawful objects can be addressed.

**Definition 4.1 (Lawful Object)**:
An object ω is lawful if:
1. Its R96 digest verifies
2. Its C768 metrics are fair
3. It passes Φ round-trip at budget 0
4. Its total budget is 0

**Definition 4.2 (Domain of Addressability)**:
```
DOM = {ω ∈ Configurations | is_lawful(ω)}
```

This immediately eliminates malformed data, corrupted structures, and adversarial inputs—they literally cannot have addresses.

### The Unlawful Wilderness

What about unlawful objects? They exist mathematically but cannot be stored:
- No address → no storage location
- No receipt → no verification
- No normal form → no canonical representation

They're computational "dark matter"—theoretically present but practically inaccessible.

## Canonicalization via Gauge Fixing

### The Problem of Equivalence

Many distinct configurations represent the same semantic object:

```python
# These are semantically identical:
list1 = [1,2,3] at sites (0,0), (0,1), (0,2)
list2 = [1,2,3] at sites (5,10), (5,11), (5,12)  # Translated
list3 = [1,2,3] at sites (0,0), (1,0), (2,0)      # Different layout
```

We need a canonical choice—a normal form.

### Gauge Fixing Protocol

**Algorithm 4.1 (Normalization)**:

```python
def normalize(object):
    # Step 1: Fix translation
    object = translate_to_origin(object)

    # Step 2: Fix schedule phase
    object = align_to_phase_zero(object)

    # Step 3: Fix boundary orientation
    object = canonical_boundary(object)

    # Step 4: Apply Φ lift for interior
    object.interior = lift_phi(object.boundary)

    return object
```

**Definition 4.3 (Normal Form)**:
The normal form NF(ω) of object ω is the unique representative in its gauge equivalence class selected by the normalization protocol.

**Theorem 4.1 (Normal Form Uniqueness)**:
For lawful object ω, NF(ω) is unique and computable in O(|ω|) time.

*Proof*: Each gauge fixing step has a unique outcome:
- Translation: Leftmost-topmost non-empty site goes to (0,0)
- Schedule: Align to phase 0 of C768 cycle
- Boundary: Lexicographic ordering of boundary sites
- Φ: Deterministic lift operation

The composition of deterministic operations is deterministic. □

### Canonical Coordinates

Once normalized, objects have canonical coordinates:

```rust
struct NormalForm {
    anchor: Site,           // Always (0,0) after normalization
    extent: (u8, u8),      // Bounding box dimensions
    phase: u16,            // Always 0 after normalization
    boundary: Vec<u8>,     // Canonical boundary ordering
    interior: Vec<u8>,     // Determined by lift_Φ(boundary)
}
```

## Address Map H

### The Perfect Hash Function

**Definition 4.4 (Address Map)**:
```
H: DOM → T
H(ω) = reduce(hash(NF(ω).receipt), T)
```

Breaking this down:
1. Normalize ω to get NF(ω)
2. Extract the receipt of NF(ω)
3. Hash the receipt to get uniform distribution
4. Reduce modulo 12,288 to get a lattice site

**Theorem 4.2 (Perfect Hashing on Lawful Domain)**:
For lawful objects ω₁, ω₂ ∈ DOM:
```
H(ω₁) = H(ω₂) ⟺ ω₁ ≡ᵍ ω₂
```

That is, addresses collide if and only if objects are gauge-equivalent (semantically identical).

*Proof*:
(⟸) If ω₁ ≡ᵍ ω₂, then NF(ω₁) = NF(ω₂), so H(ω₁) = H(ω₂).

(⟹) If H(ω₁) = H(ω₂), then receipts match after normalization. By lawfulness and receipt completeness, ω₁ ≡ᵍ ω₂. □

### No Collision Resolution Needed

Traditional hash tables need collision resolution:
- Chaining (linked lists at each bucket)
- Open addressing (probing for empty slots)
- Cuckoo hashing (multiple hash functions)

The Hologram model needs none of this. Collisions only occur for semantically identical objects, which should map to the same address anyway.

### Load Factor Is Meaningless

Traditional hash tables track load factor (items/buckets) and resize when it gets too high. In the Hologram model:
- No resize needed (T is fixed at 12,288)
- No performance degradation with occupancy
- Deduplication is automatic (identical objects share addresses)

## Content-Addressed Storage in Practice

### Writing Objects

```python
def store(object):
    # Verify lawfulness
    if not is_lawful(object):
        raise ValueError("Cannot store unlawful object")

    # Normalize
    normal_form = normalize(object)

    # Compute address
    address = H(normal_form)

    # Store at address
    lattice[address] = normal_form

    return address
```

### Reading Objects

```python
def retrieve(address):
    # Direct lookup - O(1)
    normal_form = lattice[address]

    if normal_form is None:
        return None

    # Verify receipts (paranoid mode)
    if not verify_receipt(normal_form):
        raise IntegrityError("Corrupted object")

    return normal_form
```

### Deduplication Example

```python
# Create two "different" strings
s1 = create_string("Hello", position=(0,0))
s2 = create_string("Hello", position=(10,20))

# Store both
addr1 = store(s1)  # Normalizes and stores
addr2 = store(s2)  # Normalizes to same form

assert addr1 == addr2  # Same address!
assert lattice[addr1] == NF("Hello")  # Single copy stored
```

## Running Example: Building a Dictionary

Let's implement a key-value dictionary using content addressing:

```python
class ContentDict:
    def __init__(self):
        self.lattice = Lattice()

    def put(self, key, value):
        # Create lawful pair object
        pair = create_pair(key, value)
        receipt = compute_receipt(pair)

        # Normalize and address
        normal = normalize(pair)
        address = H(normal)

        # Store
        self.lattice[address] = normal

        return address

    def get(self, key):
        # Create probe with key
        probe = create_probe(key)

        # Normalize probe
        normal_probe = normalize(probe)

        # Compute expected address
        address = H_partial(normal_probe)  # Hash of partial key

        # Retrieve and extract value
        stored = self.lattice[address]
        if stored and matches_key(stored, key):
            return extract_value(stored)
        return None

# Usage
d = ContentDict()
d.put("name", "Alice")
d.put("age", 30)

print(d.get("name"))  # "Alice"
print(d.get("age"))   # 30

# Duplicate puts are free
d.put("name", "Alice")  # No new storage used
```

## Identity and Equality

### Content Determines Identity

In traditional systems:
```c
int* p1 = malloc(sizeof(int));
int* p2 = malloc(sizeof(int));
*p1 = 42;
*p2 = 42;
// p1 != p2 (different addresses despite same content)
```

In the Hologram model:
```python
obj1 = create_int(42)
obj2 = create_int(42)
addr1 = H(obj1)
addr2 = H(obj2)
# addr1 == addr2 (same content → same address)
```

### Equality Is Decidable

**Algorithm 4.2 (Object Equality)**:
```python
def equal(obj1, obj2):
    # Lawfulness check
    if not (is_lawful(obj1) and is_lawful(obj2)):
        return False

    # Address comparison
    return H(obj1) == H(obj2)
```

This is O(n) in object size, not O(n²) deep comparison.

## Distributed CAM

Content addressing naturally extends to distributed systems:

### Global Address Space

Every node in a distributed system sees the same address for the same content:

```python
# Node A
obj = create_object(data)
addr = H(obj)  # 0x7A3F

# Node B (independent)
obj2 = create_object(same_data)
addr2 = H(obj2)  # 0x7A3F (same!)
```

### Automatic Deduplication

When nodes exchange objects:

```python
def receive_object(obj, sender):
    addr = H(obj)

    if lattice[addr] is not None:
        # Already have it, ignore duplicate
        return addr

    # New object, store it
    lattice[addr] = normalize(obj)
    return addr
```

### Content-Based Routing

Route requests based on content, not location:

```python
def route_request(content_hash):
    # Determine which node owns this content
    responsible_node = content_hash % num_nodes

    if responsible_node == self.node_id:
        return handle_locally(content_hash)
    else:
        return forward_to(responsible_node, content_hash)
```

## Exercises

**Exercise 4.1**: Prove that normalization is idempotent: NF(NF(ω)) = NF(ω).

**Exercise 4.2**: Calculate the probability of address collision for unlawful (random) data. Why is it much higher than for lawful data?

**Exercise 4.3**: Design a version control system using content addressing. How do you handle commits and branches?

**Exercise 4.4**: Implement a B-tree where node addresses are content-determined. What happens during rebalancing?

**Exercise 4.5**: Show that content addressing makes certain attacks impossible. Which attacks remain possible?

## Implementation Notes

Here's production code for the address map:

```rust
use sha3::{Sha3_256, Digest};

pub struct AddressMap {
    hasher: Sha3_256,
}

impl AddressMap {
    pub fn address_of(&mut self, object: &LawfulObject) -> Site {
        // Normalize
        let normal_form = object.normalize();

        // Extract receipt
        let receipt = normal_form.receipt();

        // Hash receipt
        self.hasher.reset();
        self.hasher.update(receipt.as_bytes());
        let hash = self.hasher.finalize();

        // Reduce to lattice site
        let index = u16::from_le_bytes([hash[0], hash[1]]) % 12288;
        Site::from_linear(index)
    }
}

pub struct ContentStore {
    lattice: Lattice,
    address_map: AddressMap,
}

impl ContentStore {
    pub fn put(&mut self, object: LawfulObject) -> Result<Site, StoreError> {
        // Compute address
        let addr = self.address_map.address_of(&object);

        // Check for existing object
        if let Some(existing) = self.lattice.get(addr) {
            if !existing.equivalent_to(&object) {
                // This should be impossible for lawful objects
                return Err(StoreError::ImpossibleCollision);
            }
            // Deduplicated
            return Ok(addr);
        }

        // Store new object
        self.lattice.set(addr, object.normalize());
        Ok(addr)
    }

    pub fn get(&self, addr: Site) -> Option<&LawfulObject> {
        self.lattice.get(addr)
    }
}
```

## Takeaways

1. **Addresses are content**: H(object) determined by receipts and normal form
2. **Perfect hashing on lawful domain**: No collisions between distinct lawful objects
3. **Normalization ensures uniqueness**: Each equivalence class has one representative
4. **Deduplication is automatic**: Identical content → same address
5. **Memory safety is intrinsic**: No pointers, no dangling references
6. **Distributed systems benefit**: Global content addressing across nodes

Content-addressable memory isn't just an optimization—it's a fundamental restructuring of how we think about storage and identity.

---

*This completes Part I. Next, Part II explores how these foundations support a complete type system and programming model.*