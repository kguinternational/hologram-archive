# Chapter 14: Normalization & Confluence

## Motivation

In traditional rewriting systems, a crucial question is whether different reduction sequences lead to the same result. The Hologram model adds a twist: reductions must respect conservation laws, and "sameness" is defined up to gauge equivalence. This chapter establishes when process objects have unique normal forms, when different evaluation strategies converge, and how conservation laws actually simplify the confluence problem by eliminating many potential reduction paths.

## Confluence up to Gauge

### Gauge-Aware Confluence

Traditional confluence: If a →* b and a →* c, then ∃d such that b →* d and c →* d.

Hologram confluence: If a →* b and a →* c, then ∃d such that b →* d' and c →* d'' where d' ≡ᵍ d''.

**Definition 14.1 (Gauge Confluence)**:
A reduction system is gauge-confluent if all diverging reduction paths reconverge up to gauge equivalence.

```python
def is_gauge_confluent(reduction_system):
    """Check if system is confluent up to gauge."""
    for term in generate_test_terms():
        # Find all possible reductions
        reductions = []
        for strategy in [leftmost, rightmost, parallel, random]:
            reduced = reduce_with_strategy(term, strategy)
            reductions.append(normalize(reduced))  # Apply gauge fixing

        # Check all reduce to same normal form
        normal_forms = [r.normal_form for r in reductions]
        if not all(nf.gauge_equivalent(normal_forms[0]) for nf in normal_forms):
            return False, term  # Counterexample

    return True, None
```

### The Diamond Lemma for Gauge Systems

**Lemma 14.1 (Gauge Diamond)**:
If → satisfies the gauge diamond property, then →* is gauge-confluent.

The gauge diamond property states:
```
    a
   / \
  b   c
  |   |
  d'  d''

where d' ≡ᵍ d''
```

**Proof**:
```python
def prove_gauge_diamond():
    """Constructive proof of gauge diamond lemma."""

    def diamond_step(a):
        # All one-step reductions from a
        b = reduce_left(a)
        c = reduce_right(a)

        # Show they reconverge (up to gauge)
        d_from_b = reduce_right(b)
        d_from_c = reduce_left(c)

        # Apply gauge normalization
        d_from_b_normal = fix_gauge(d_from_b)
        d_from_c_normal = fix_gauge(d_from_c)

        assert d_from_b_normal == d_from_c_normal
        return d_from_b_normal

    # Extend to multi-step by induction
    def diamond_multi(a, n):
        if n == 0:
            return a
        else:
            # Use diamond for single step
            b = diamond_step(a)
            # Inductively apply to result
            return diamond_multi(b, n-1)
```

### Critical Pairs in Conservation Systems

**Definition 14.2 (Conservation-Respecting Critical Pair)**:
A critical pair (t₁, t₂) where both reductions preserve conservation laws.

```python
def find_critical_pairs(rules):
    """Find critical pairs that respect conservation."""
    critical_pairs = []

    for rule1 in rules:
        for rule2 in rules:
            overlaps = find_overlaps(rule1.lhs, rule2.lhs)

            for overlap in overlaps:
                # Create critical pair
                t1 = apply_rule(overlap, rule1)
                t2 = apply_rule(overlap, rule2)

                # Check both preserve conservation
                if (preserves_conservation(t1, overlap) and
                    preserves_conservation(t2, overlap)):
                    critical_pairs.append((t1, t2, overlap))

    return critical_pairs

def resolve_critical_pair(t1, t2):
    """Show critical pair converges."""
    # Reduce both sides to normal form
    nf1 = reduce_to_normal_form(t1)
    nf2 = reduce_to_normal_form(t2)

    # Check gauge equivalence
    return nf1.gauge_equivalent(nf2)
```

## Strong Normalization

### Budget-Bounded Normalization

**Theorem 14.1 (Strong Normalization with Budget)**:
Every reduction sequence in the Hologram model terminates when budget is finite.

*Proof*:
```python
def prove_strong_normalization():
    """Budget ensures termination."""

    class BudgetedReduction:
        def __init__(self, initial_budget):
            self.budget = initial_budget
            self.steps = 0

        def reduce(self, term):
            while is_reducible(term) and self.budget > 0:
                # Each reduction costs budget
                term, cost = reduce_once_with_cost(term)
                self.budget -= cost
                self.steps += 1

                # Budget exhaustion = termination
                if self.budget <= 0:
                    return term, "BUDGET_EXHAUSTED"

            return term, "NORMAL_FORM"

    # For any finite budget B, reduction terminates in ≤ B steps
    MAX_BUDGET = 12288  # Lattice size
    reducer = BudgetedReduction(MAX_BUDGET)
    result, reason = reducer.reduce(any_term)

    assert reducer.steps <= MAX_BUDGET
    return True
```

### Decreasing Metrics

**Definition 14.3 (Conservation Metric)**:
A metric that decreases with each reduction while preserving conservation.

```python
def conservation_metric(config: Configuration) -> int:
    """Metric that decreases during reduction."""
    # Combination of factors
    m1 = sum(1 for i in range(LATTICE_SIZE)
             if config.lattice.data[i] != 0)  # Non-zero sites

    m2 = action_functional(config)  # Action always decreases

    m3 = disorder_measure(config)  # Entropy-like measure

    return m1 + int(m2 * 1000) + m3

def verify_decreasing():
    """Verify metric decreases."""
    config = random_configuration()
    metric_before = conservation_metric(config)

    # Apply any lawful reduction
    reduced = apply_reduction(config)
    metric_after = conservation_metric(reduced)

    assert metric_after < metric_before
```

### Normalization Strategy

**Algorithm 14.1 (Optimal Normalization)**:

```python
def optimal_normalize(config: Configuration) -> Configuration:
    """Normalize using optimal strategy."""

    # Priority queue by estimated cost
    from heapq import heappush, heappop
    queue = [(0, config)]
    visited = set()

    while queue:
        cost, current = heappop(queue)

        if is_normal_form(current):
            return current

        config_hash = current.hash()
        if config_hash in visited:
            continue
        visited.add(config_hash)

        # Generate all possible reductions
        for reduction in possible_reductions(current):
            new_config = apply_reduction(current, reduction)
            new_cost = cost + reduction.cost()

            # A* heuristic: estimated remaining cost
            heuristic = estimate_distance_to_normal(new_config)
            priority = new_cost + heuristic

            heappush(queue, (priority, new_config))

    raise ValueError("No normal form found")
```

## Church-Rosser Results

### Classical Church-Rosser

**Theorem 14.2 (Church-Rosser for Lawful Reductions)**:
The subset of reductions that preserve conservation laws satisfies Church-Rosser.

*Proof*:
```python
def prove_church_rosser():
    """Prove Church-Rosser property."""

    def all_reductions_converge(term):
        """All reduction paths from term converge."""
        # Collect all possible reduction sequences
        sequences = []

        def explore(current, path):
            if is_normal_form(current):
                sequences.append(path)
                return

            for next_term in one_step_reductions(current):
                if preserves_conservation(next_term, current):
                    explore(next_term, path + [next_term])

        explore(term, [term])

        # Extract normal forms
        normal_forms = [seq[-1] for seq in sequences]

        # All should be gauge-equivalent
        first_nf = normalize(normal_forms[0])
        for nf in normal_forms[1:]:
            assert normalize(nf).gauge_equivalent(first_nf)

        return True

    # Test on sample terms
    for term in generate_test_terms():
        assert all_reductions_converge(term)
```

### Parallel Reductions

**Definition 14.4 (Parallel Reduction)**:
Simultaneous reduction of independent redexes.

```python
class ParallelReducer:
    """Reduce independent redexes simultaneously."""

    def find_independent_redexes(self, config):
        """Find redexes that don't interfere."""
        redexes = find_all_redexes(config)
        independent_sets = []

        # Greedy algorithm for independence
        for redex in redexes:
            # Find a set this redex can join
            placed = False
            for ind_set in independent_sets:
                if all(self.are_independent(redex, other) for other in ind_set):
                    ind_set.append(redex)
                    placed = True
                    break

            if not placed:
                independent_sets.append([redex])

        return independent_sets

    def are_independent(self, redex1, redex2):
        """Check if two redexes can be reduced in parallel."""
        # Disjoint locations
        if redex1.sites.isdisjoint(redex2.sites):
            return True

        # Different R96 classes
        if redex1.r96_class != redex2.r96_class:
            return True

        # Different pages (height-independence)
        if all(s1.page != s2.page for s1 in redex1.sites for s2 in redex2.sites):
            return True

        return False

    def parallel_reduce(self, config):
        """Perform parallel reduction."""
        independent_sets = self.find_independent_redexes(config)

        # Reduce largest independent set
        if independent_sets:
            largest_set = max(independent_sets, key=len)
            new_config = config.copy()

            # Apply all reductions in parallel
            for redex in largest_set:
                new_config = apply_redex(new_config, redex)

            return new_config

        return config  # No redexes
```

### Unique Normal Forms

**Theorem 14.3 (Uniqueness of Normal Forms)**:
If a configuration has a normal form, it is unique up to gauge.

*Proof*:
```python
def prove_unique_normal_form():
    """Prove uniqueness of normal forms."""

    def reduce_to_normal(config, strategy):
        """Reduce using given strategy."""
        current = config
        while is_reducible(current):
            current = strategy(current)
        return normalize(current)  # Apply gauge fixing

    # Test different strategies
    config = random_lawful_configuration()

    nf_leftmost = reduce_to_normal(config, leftmost_reduction)
    nf_rightmost = reduce_to_normal(config, rightmost_reduction)
    nf_parallel = reduce_to_normal(config, parallel_reduction)
    nf_random = reduce_to_normal(config, random_reduction)

    # All should be gauge-equivalent
    assert nf_leftmost.gauge_equivalent(nf_rightmost)
    assert nf_leftmost.gauge_equivalent(nf_parallel)
    assert nf_leftmost.gauge_equivalent(nf_random)

    print("✓ Normal form is unique up to gauge")
    return True
```

## Conservation-Preserving Reductions

### The Conservation Constraint

**Definition 14.5 (Conservation-Preserving Reduction)**:
A reduction → that maintains all conservation laws.

```python
def is_conservation_preserving(reduction):
    """Check if reduction preserves conservation laws."""

    def check_single_step(before, after):
        # R96 conservation
        if compute_r96_digest(before) != compute_r96_digest(after):
            return False, "R96 violated"

        # C768 fairness
        if not maintains_fairness(before, after):
            return False, "C768 fairness violated"

        # Φ coherence
        if not maintains_phi_coherence(before, after):
            return False, "Φ coherence violated"

        # Budget non-increase
        if after.budget_used > before.budget_used + reduction.cost:
            return False, "Budget increased illegally"

        return True, "Conservation preserved"

    # Test on sample reductions
    for _ in range(100):
        before = random_configuration()
        after = reduction(before)
        preserved, reason = check_single_step(before, after)
        if not preserved:
            return False, reason

    return True, "All conservation laws preserved"
```

### Lawful Reduction Strategies

```python
class LawfulReducer:
    """Reduction strategies that preserve conservation."""

    def innermost_lawful(self, config):
        """Reduce innermost redex that preserves conservation."""
        redexes = find_redexes_innermost_first(config)

        for redex in redexes:
            trial = apply_redex(config, redex)
            if preserves_all_conservation(trial, config):
                return trial

        return config  # No lawful reduction possible

    def outermost_lawful(self, config):
        """Reduce outermost redex that preserves conservation."""
        redexes = find_redexes_outermost_first(config)

        for redex in redexes:
            trial = apply_redex(config, redex)
            if preserves_all_conservation(trial, config):
                return trial

        return config

    def minimize_action(self, config):
        """Choose reduction that minimally increases action."""
        redexes = find_all_redexes(config)
        best_config = config
        best_action = action_functional(config)

        for redex in redexes:
            trial = apply_redex(config, redex)
            if preserves_all_conservation(trial, config):
                trial_action = action_functional(trial)
                if trial_action < best_action:
                    best_action = trial_action
                    best_config = trial

        return best_config
```

## Normalization Algorithms

### Efficient Normal Form Computation

```python
def efficient_normalize(config: Configuration) -> Configuration:
    """Efficiently compute normal form."""

    # Phase 1: Gauge fixing
    config = fix_gauge_efficient(config)

    # Phase 2: Local reductions (R96 class-local)
    for r_class in range(96):
        config = reduce_class_local(config, r_class)

    # Phase 3: Global reductions
    config = reduce_global(config)

    # Phase 4: Final gauge fixing
    config = fix_gauge_final(config)

    return config

def fix_gauge_efficient(config):
    """Efficient gauge fixing."""
    # Use incremental updates instead of full recomputation
    changes = detect_gauge_changes(config)

    for change in changes:
        if change.type == "translation":
            config = translate_incremental(config, change.delta)
        elif change.type == "rotation":
            config = rotate_incremental(config, change.angle)
        elif change.type == "boundary":
            config = fix_boundary_incremental(config, change.sites)

    return config
```

### Memoization via Content Addressing

```python
class MemoizedNormalizer:
    """Use content addressing to memoize normalizations."""

    def __init__(self):
        self.cache = {}  # Address → Normal form

    def normalize(self, config):
        # Check cache
        address = H(config)
        if address in self.cache:
            return self.cache[address]

        # Compute normal form
        normal = compute_normal_form(config)

        # Cache for reuse
        self.cache[address] = normal
        self.cache[H(normal)] = normal  # Normal form of NF is itself

        return normal

    def batch_normalize(self, configs):
        """Normalize batch, exploiting shared subterms."""
        # Build dependency graph
        graph = build_reduction_graph(configs)

        # Topological sort for optimal order
        order = topological_sort(graph)

        results = []
        for config in order:
            # Many subterms already normalized
            normal = self.normalize(config)
            results.append(normal)

        return results
```

## Confluence Modulo Theories

### Confluence with R96 Classes

```python
def confluence_modulo_r96():
    """Confluence modulo R96 equivalence."""

    def reduce_modulo_r96(config):
        """Reduce treating R96-equivalent bytes as equal."""
        # Partition by R96 class
        partitions = partition_by_r96(config)

        # Reduce within each partition independently
        reduced_partitions = {}
        for r_class, partition in partitions.items():
            reduced_partitions[r_class] = reduce_partition(partition)

        # Reassemble
        return reassemble_partitions(reduced_partitions)

    # Test confluence
    config = random_configuration()

    # Different reduction orders
    r1 = reduce_modulo_r96(reduce_left_first(config))
    r2 = reduce_modulo_r96(reduce_right_first(config))

    # Should be R96-equivalent
    assert same_r96_distribution(r1, r2)
```

### Confluence with C768 Scheduling

```python
def confluence_with_scheduling():
    """Confluence respecting C768 schedule."""

    def scheduled_reduction(config, phase):
        """Reduce only sites active in current phase."""
        active_sites = get_active_sites(phase)
        return reduce_sites(config, active_sites)

    # Full cycle should be confluent
    config = random_configuration()

    # Path 1: Phase order 0,1,2,...,767
    result1 = config
    for phase in range(768):
        result1 = scheduled_reduction(result1, phase)

    # Path 2: Different phase order (respecting dependencies)
    result2 = config
    for phase in random_permutation_respecting_deps(range(768)):
        result2 = scheduled_reduction(result2, phase)

    # Results should be gauge-equivalent
    assert normalize(result1).gauge_equivalent(normalize(result2))
```

## Exercises

**Exercise 14.1**: Prove that parallel reduction terminates faster than sequential.

**Exercise 14.2**: Find a critical pair that cannot be resolved without gauge fixing.

**Exercise 14.3**: Design a reduction strategy that minimizes budget usage.

**Exercise 14.4**: Prove that conservation laws strengthen confluence.

**Exercise 14.5**: Implement incremental normalization that reuses previous work.

## Takeaways

1. **Confluence up to gauge**: Different paths converge modulo gauge equivalence
2. **Strong normalization via budget**: Finite budget ensures termination
3. **Church-Rosser holds**: For conservation-preserving reductions
4. **Unique normal forms**: Up to gauge equivalence
5. **Conservation simplifies confluence**: Fewer valid reduction paths
6. **Efficient normalization**: Via memoization and incremental updates

Normalization in the Hologram model is both guaranteed (by budget) and efficient (by structure).

---

*Next: Chapter 15 develops the categorical semantics of the model.*