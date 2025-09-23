# Chapter 5: Lawfulness as a Type System

## Motivation

Type systems traditionally bolt safety onto computation through external rules and checks. The Hologram model inverts this: types ARE conservation laws, and type safety is physical law. A type error isn't a rule violation caught by a checker—it's a configuration that cannot exist, like a perpetual motion machine or negative probability.

This chapter develops a type system where:
- Types have intrinsic cost (budgets)
- Type checking is receipt verification
- Ill-typed programs are physically impossible
- Polymorphism arises from gauge invariance

## Budgeted Typing Judgments

### Types with Cost

Traditional typing judgment: `Γ ⊢ e : τ` (expression e has type τ in context Γ)

Hologram typing judgment: `Γ ⊢ x : τ [β]` (object x has type τ at budget β in context Γ)

**Definition 5.1 (Budgeted Type)**:
A budgeted type is a pair (τ, β) where:
- τ is a semantic property (membership in a lawful class)
- β ∈ ℤ₉₆ is the cost to verify membership

**Definition 5.2 (Type Checking as Verification)**:
```
Γ ⊢ x : τ [β] ⟺ verify_receipt(x, τ) with cost β
```

### The Crush Operator

**Definition 5.3 (Truth via Crush)**:
```
⟨β⟩ = true  ⟺ β = 0
⟨β⟩ = false ⟺ β ≠ 0
```

This gives us a decision procedure:
- Type checking succeeds ⟺ ⟨β⟩ = true ⟺ β = 0
- Perfect typing requires zero budget
- Non-zero budget indicates approximate typing

### Subtyping via Budget Ordering

**Definition 5.4 (Budget Subtyping)**:
```
τ[β₁] <: τ[β₂] ⟺ β₁ ≤ β₂
```

Smaller budgets are more precise types:
- τ[0]: Perfectly lawful, exact type
- τ[10]: Approximately typed, 10 units of uncertainty
- τ[95]: Nearly untyped, maximum uncertainty

## Type Constructors & Discipline

### Base Types from Conservation Laws

**R96 Types** (Resonance-based):
```
τᴿ(k) = {x | R(x) = k} for k ∈ ℤ₉₆
```

Objects with specific resonance signatures.

**C768 Types** (Schedule-compatible):
```
τᶜ(phase) = {x | compatible_with_phase(x, phase)}
```

Objects that can execute at given schedule phase.

**Φ Types** (Coherence-preserving):
```
τᶠ = {x | proj_Φ(lift_Φ(x)) = x at β=0}
```

Objects that survive round-trip encoding.

### Composite Types

**Product Types**:
```
Γ ⊢ x : τ₁ [β₁]    Γ ⊢ y : τ₂ [β₂]
----------------------------------------
Γ ⊢ (x,y) : τ₁ × τ₂ [β₁ + β₂]
```

Budgets add for products—verifying both costs the sum.

**Sum Types**:
```
Γ ⊢ x : τ₁ [β] ∨ Γ ⊢ x : τ₂ [β]
----------------------------------
Γ ⊢ x : τ₁ + τ₂ [β]
```

Same budget for either alternative.

**Function Types**:
```
Γ, x:τ₁[0] ⊢ e : τ₂ [β]
-------------------------
Γ ⊢ λx.e : τ₁ →[β] τ₂ [0]
```

Functions have budget-annotated arrows.

### Dependent Types

Types can depend on receipts:

**Definition 5.5 (Receipt-Dependent Type)**:
```
Πr:Receipt. τ(r) = Type depending on receipt r
```

Example:
```
SortedList(r) = {list | receipt(list).r96 = r ∧ is_sorted(list)}
```

A list with specific R96 digest that's also sorted.

## Poly-Ontological Objects

### Multiple Mathematical Faces

A single Hologram object can simultaneously be:
- A number (arithmetic operations)
- A group element (group operations)
- An operator (function application)
- A proof (evidence of a proposition)

**Definition 5.6 (Poly-Ontological Object)**:
An object ω with multiple type facets:
```
ω : Number[0] ∧ Group[0] ∧ Operator[0] ∧ Proof[0]
```

### Coherence Morphisms

Moving between facets requires coherence:

**Definition 5.7 (Facet Morphism)**:
```
cast : ω:τ₁[β₁] → ω:τ₂[β₂]
```

At budget 0, casts are isomorphisms.

### Running Example: The Number-Operator Duality

```python
# Object that's both number and operator
class NumOp:
    def __init__(self, value, receipt):
        self.value = value      # Numeric facet
        self.receipt = receipt

    # As number
    def add(self, other):
        return NumOp(self.value + other.value,
                    compose_receipts(self.receipt, other.receipt))

    # As operator
    def apply(self, arg):
        return NumOp(self.value * arg.value,  # Multiply operation
                    op_receipt(self.receipt, arg.receipt))

    # Coherence: applying 2 is same as adding twice
    # ω.apply(x) ≡ x.add(ω).add(ω) when ω.value = 2

x = NumOp(3, receipt_3)
two = NumOp(2, receipt_2)

# Use as number
y = x.add(two)      # 3 + 2 = 5

# Use as operator
z = two.apply(x)    # 2 × 3 = 6

# Both maintain receipts!
```

## Type Checking as Physics

### Physical Impossibility of Type Errors

Traditional type error:
```python
"hello" + 5  # TypeError: cannot add string and int
```

Hologram type error:
```python
string_config = create_string("hello")  # R96 class 17
int_config = create_int(5)              # R96 class 42

# Addition requires matching R96 classes
add(string_config, int_config)
# IMPOSSIBLE: receipts don't verify, configuration cannot exist
```

The error isn't caught—it's prevented by physics.

### Conservation-Based Type Safety

**Theorem 5.1 (Type Safety via Conservation)**:
If configuration s is lawful, then all operations preserving conservation laws preserve types.

*Proof*: Types are defined by conservation properties. Operations that preserve conservation laws preserve these properties by definition. □

### The No-Cast Theorem

**Theorem 5.2 (No Unsafe Casts)**:
At budget β=0, casting between incompatible types is impossible.

*Proof*: Casting would require changing receipts. At β=0, receipts are immutable (conservation). Therefore, objects cannot change type without budget expenditure. □

## Running Example: A Type-Safe Container

```rust
// Container parameterized by R96 class
struct Container<const R: u8> {
    data: Vec<LawfulObject>,
    receipt: Receipt,
}

impl<const R: u8> Container<R> {
    // Can only insert objects with matching resonance
    fn insert(&mut self, obj: LawfulObject) -> Result<(), TypeError> {
        if obj.receipt.r96_class() != R {
            // Physically impossible to insert
            return Err(TypeError::ResonanceMismatch);
        }

        self.data.push(obj);
        self.receipt = compose_receipts(self.receipt, obj.receipt);
        Ok(())
    }

    // Extraction preserves type
    fn get(&self, index: usize) -> Option<&LawfulObject> {
        self.data.get(index)
        // Returned object guaranteed to have R96 class R
    }
}

// Usage
let mut int_container: Container<42> = Container::new();  // R96=42 for ints
let mut str_container: Container<17> = Container::new();  // R96=17 for strings

let int_obj = create_int(100);
let str_obj = create_string("hello");

int_container.insert(int_obj);  // OK
int_container.insert(str_obj);  // ERROR: Resonance mismatch

// Type safety without runtime checks!
```

## Gradual Typing via Budgets

### From Untyped to Typed

Start with high-budget (weakly typed) code:

```python
# Budget 95: Almost no typing
def process_anything(x):  # x : Any[95]
    return transform(x)   # No guarantees

# Budget 50: Some typing
def process_structured(x):  # x : Structured[50]
    verify_basic_structure(x)
    return transform(x)     # Basic guarantees

# Budget 0: Full typing
def process_exact(x):       # x : Exact[0]
    verify_complete_receipt(x)
    return transform(x)     # Complete guarantees
```

### Type Refinement

Gradually reduce budget through verification:

```python
def refine_type(obj, target_budget):
    current_budget = obj.budget

    while current_budget > target_budget:
        # Perform verification step
        obj = verify_step(obj)
        current_budget -= verification_cost

    return obj  # Now at target_budget
```

## Exercises

**Exercise 5.1**: Prove that type checking is decidable in the Hologram model.

**Exercise 5.2**: Design a polymorphic type that works for any R96 class. What's its budget cost?

**Exercise 5.3**: Show that function composition preserves typing:
If f: τ₁ →[β₁] τ₂ and g: τ₂ →[β₂] τ₃, then g∘f: τ₁ →[β₁+β₂] τ₃.

**Exercise 5.4**: Implement a typed channel that only accepts messages of specific resonance class.

**Exercise 5.5**: Prove that every lawful object has at least one type at budget 0.

## Implementation Notes

Type checking in practice:

```rust
pub struct TypeChecker {
    receipt_verifier: ReceiptVerifier,
    budget_tracker: BudgetTracker,
}

impl TypeChecker {
    pub fn check(&mut self, obj: &Object, typ: &Type) -> Result<Budget, TypeError> {
        // Start with maximum budget
        let mut budget = Budget::MAX;

        // Check each type constraint
        for constraint in typ.constraints() {
            match constraint {
                Constraint::R96(class) => {
                    if !self.receipt_verifier.verify_r96(obj, class) {
                        return Err(TypeError::R96Mismatch);
                    }
                    budget = budget.saturating_sub(R96_COST);
                }
                Constraint::C768(phase) => {
                    if !self.receipt_verifier.verify_c768(obj, phase) {
                        return Err(TypeError::C768Incompatible);
                    }
                    budget = budget.saturating_sub(C768_COST);
                }
                Constraint::Phi => {
                    if !self.receipt_verifier.verify_phi(obj) {
                        return Err(TypeError::PhiIncoherent);
                    }
                    budget = budget.saturating_sub(PHI_COST);
                }
            }
        }

        Ok(budget)
    }
}

// Type-safe operations
pub fn typed_add<const R: u8>(
    x: TypedObject<R>,
    y: TypedObject<R>
) -> TypedObject<R> {
    // Can only add objects with same resonance
    // Compiler enforces this!
    let result = add_internal(x.inner(), y.inner());
    TypedObject::new(result)
}
```

## Takeaways

1. **Types are conservation laws**: Not external rules but intrinsic properties
2. **Budgets quantify typing precision**: β=0 means exactly typed
3. **Type errors are physically impossible**: Like perpetual motion machines
4. **Poly-ontology enables rich types**: Objects have multiple coherent facets
5. **No unsafe casts at zero budget**: Conservation laws prevent type confusion
6. **Gradual typing through budget reduction**: From dynamic to static typing

Types in the Hologram model aren't bureaucracy—they're the physics of information.

---

*Next: Chapter 6 shows how programs themselves become geometric objects with algebraic properties.*