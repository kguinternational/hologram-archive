# CLAUDE.md - Working Directory

## Atlas Implementation Principles

**CRITICAL: Mathematical Rigor Required**

### PROHIBITED in ALL Atlas work:

- ❌ **NumPy** - NO floating point arrays, NO approximate arithmetic
- ❌ **Floats** - NO `float`, NO `np.float64`, NO decimal approximations
- ❌ **Numerical tolerance comparisons** - NO `abs(a - b) < eps`
- ❌ **Approximate methods** - NO gradient descent, NO numerical optimization
- ❌ **Probabilistic/statistical methods** - NO random sampling for structure discovery

### REQUIRED:

- ✅ **Exact rational arithmetic** - `fractions.Fraction` for all non-integer values
- ✅ **Exact integer arithmetic** - `int` for all integer operations
- ✅ **Half-integer quantization** - For coordinates in ℤ ∪ ½ℤ, use `Fraction(n, 2)`
- ✅ **Symbolic computation** - If needed, use `sympy` with exact types
- ✅ **Algebraic methods** - First-principles construction from group theory
- ✅ **Exact equality** - Use `==` for exact comparison, NO tolerances

## Why This Matters

**Atlas is a mathematical structure with EXACT properties:**
- 96 resonance classes (exact count)
- Cartan matrices with exact integer/half-integer entries
- Root systems with exact norm ratios (e.g., √2/√1 for F₄)
- Weyl group orders (exact: 12 for G₂, 1152 for F₄)

**Floating point arithmetic DESTROYS structure:**
- 0.99999... ≠ 1 breaks unity class constraint
- Accumulated rounding errors invalidate algebraic identities
- Cannot verify exact root relationships
- Cannot distinguish true symmetries from numerical artifacts

## Implementation Approach

**Build from first principles:**
1. Start with exact algebraic definitions
2. Construct structures combinatorially
3. Verify properties by exact computation
4. Let structures EMERGE from categorical framework
5. Do NOT impose external Lie theory assumptions

**Do NOT:**
1. Approximate and then "clean up"
2. Optimize numerically and hope for integer results
3. Use tolerance-based convergence
4. Treat Atlas as a numerical optimization problem

## This Workspace

All code in `/workspaces/Hologram/working/` must adhere to these principles.

**Existing violations:**
- `action_framework/` - Currently uses numpy/float, needs redesign for exact arithmetic
- Any file importing `numpy` is INVALID for Atlas work

**Valid approaches:**
- `exceptional_groups/` - First-principles algebraic construction
- Symbolic Lagrangian formulation (no numerical minimization)
- Categorical/functorial definitions
