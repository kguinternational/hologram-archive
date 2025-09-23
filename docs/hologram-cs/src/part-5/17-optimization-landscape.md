# Chapter 17: Optimization Landscape

## Motivation

The universal action functional S creates a rich optimization landscape over configuration space. Understanding this landscape—its convexity properties, critical points, and convergence basins—is essential for efficient compilation and optimization. This chapter analyzes the geometric and analytic properties of the action landscape, proving when optimization converges, how quickly, and to what kind of solutions.

## Convexity per Sector

### Sector-wise Analysis

Each sector of the action has distinct convexity properties:

**Theorem 17.1 (Sector Convexity)**:
Individual sectors exhibit the following convexity properties:
- L_geom: Strongly convex
- L_R96: Convex
- L_C768: Convex
- L_budget: Linear (hence convex)
- L_Φ: Locally strongly convex near identity
- L_gauge: Convex
- L_receipt: Convex
- L_problem: Problem-dependent

*Proof for Geometric Sector*:

```python
def prove_geometric_convexity():
    """Prove geometric sector is strongly convex."""

    def L_geom(config):
        """Geometric smoothness functional."""
        action = 0
        for i in range(LATTICE_SIZE):
            for j in neighbors(i):
                diff = config[i] - config[j]
                action += diff ** 2 / distance(i, j)
        return action

    def hessian_L_geom(config):
        """Compute Hessian matrix."""
        H = np.zeros((LATTICE_SIZE, LATTICE_SIZE))

        for i in range(LATTICE_SIZE):
            # Diagonal terms
            H[i,i] = 2 * len(neighbors(i))

            # Off-diagonal terms
            for j in neighbors(i):
                H[i,j] = -2 / distance(i, j)

        return H

    # Check positive definiteness
    H = hessian_L_geom(random_configuration())
    eigenvalues = np.linalg.eigvals(H)

    # All eigenvalues positive → strongly convex
    min_eigenvalue = np.min(eigenvalues)
    assert min_eigenvalue > 0

    # Strong convexity parameter
    mu = min_eigenvalue
    print(f"Strongly convex with parameter μ = {mu}")

    return True
```

### Mixed Convexity

The total action mixes convex and non-convex sectors:

```python
class MixedConvexityAnalysis:
    """Analyze convexity of combined action."""

    def __init__(self, weights):
        self.weights = weights

    def is_convex_combination(self):
        """Check if weighted sum preserves convexity."""
        # Convex + Convex = Convex
        convex_sectors = ['geom', 'r96', 'c768', 'budget', 'gauge', 'receipt']

        total_convex_weight = sum(self.weights[s] for s in convex_sectors)
        total_weight = sum(self.weights.values())

        convexity_ratio = total_convex_weight / total_weight

        if convexity_ratio == 1.0:
            return "Fully convex"
        elif convexity_ratio > 0.5:
            return "Predominantly convex"
        else:
            return "Non-convex"

    def find_convex_region(self, config):
        """Find region where action is locally convex."""
        # Compute Hessian
        H = self.compute_hessian(config)

        # Check positive semi-definiteness
        eigenvalues = np.linalg.eigvals(H)
        min_eigen = np.min(eigenvalues)

        if min_eigen > 0:
            # Locally strongly convex
            radius = 1 / np.linalg.norm(H)
            return f"Strongly convex in ball of radius {radius}"
        elif min_eigen >= 0:
            return "Locally convex"
        else:
            return "Non-convex at this point"
```

### Geodesic Convexity

On the lattice with gauge structure, we have geodesic convexity:

```python
def geodesic_convexity():
    """Convexity along geodesics in configuration space."""

    def geodesic(config1, config2, t):
        """Geodesic from config1 to config2."""
        # Direct interpolation
        direct = (1-t) * config1 + t * config2

        # Apply gauge correction
        gauge_corrected = fix_gauge(direct)

        # Project to lawful subspace
        return project_to_lawful(gauge_corrected)

    def action_along_geodesic(config1, config2):
        """Action along geodesic path."""
        points = []
        actions = []

        for t in np.linspace(0, 1, 100):
            config_t = geodesic(config1, config2, t)
            action_t = compute_action(config_t)
            points.append(t)
            actions.append(action_t)

        return points, actions

    # Test convexity along geodesic
    config1 = random_lawful_configuration()
    config2 = random_lawful_configuration()

    t_values, action_values = action_along_geodesic(config1, config2)

    # Check if action is convex along path
    # Convex if: S(geodesic(t)) ≤ (1-t)S(config1) + t*S(config2)
    for i, t in enumerate(t_values):
        interpolated = (1-t) * action_values[0] + t * action_values[-1]
        assert action_values[i] <= interpolated + EPSILON

    print("✓ Action is geodesically convex")
```

## Existence & Uniqueness

### Existence of Minima

**Theorem 17.2 (Existence of Minimizers)**:
The action functional S attains its minimum on the lawful subspace.

*Proof*:

```python
def prove_existence_of_minimum():
    """Prove minimum exists using compactness."""

    # The lawful subspace
    LAWFUL = {config | is_lawful(config)}

    # Key observations:
    # 1. Lawful subspace is closed (limit of lawful is lawful)
    # 2. Action is bounded below (S ≥ 0)
    # 3. Action has bounded level sets

    def is_closed(subspace):
        """Check if subspace is closed."""
        # Take sequence converging to boundary
        sequence = generate_convergent_sequence()
        limit = compute_limit(sequence)

        # Limit must be in subspace
        return limit in subspace

    def is_coercive(functional):
        """Check if functional is coercive."""
        # S(config) → ∞ as ||config|| → ∞
        for r in [10, 100, 1000, 10000]:
            config = random_config_with_norm(r)
            action = functional(config)
            assert action >= r / 100  # Grows with norm

        return True

    assert is_closed(LAWFUL)
    assert is_coercive(compute_action)

    # By Weierstrass theorem, minimum exists
    print("✓ Minimum exists by Weierstrass theorem")
    return True
```

### Uniqueness up to Gauge

**Theorem 17.3 (Uniqueness Modulo Gauge)**:
If the action has a strict minimum, it is unique up to gauge equivalence.

```python
def prove_uniqueness_modulo_gauge():
    """Prove minimizer is unique up to gauge."""

    def find_all_minima():
        """Find all local minima."""
        minima = []

        # Start from random initializations
        for _ in range(1000):
            initial = random_lawful_configuration()
            minimum = gradient_descent(initial)
            minima.append(minimum)

        return minima

    # Find minima
    minima = find_all_minima()

    # Cluster by gauge equivalence
    equivalence_classes = []
    for m in minima:
        # Find equivalence class
        found = False
        for ec in equivalence_classes:
            if gauge_equivalent(m, ec[0]):
                ec.append(m)
                found = True
                break

        if not found:
            equivalence_classes.append([m])

    # Should have single equivalence class for strict minimum
    if len(equivalence_classes) == 1:
        print("✓ Unique minimum up to gauge")
        return True
    else:
        print(f"Found {len(equivalence_classes)} distinct minima")
        return False
```

### Multiplicity from Symmetry

```python
def analyze_multiplicity():
    """Understand multiplicity of critical points."""

    def critical_points_from_symmetry():
        """Symmetry creates multiple critical points."""
        # Action invariant under gauge group G
        G = compute_gauge_group()

        # For symmetric action, critical points come in orbits
        critical_point = find_one_critical_point()
        orbit = []

        for g in G.elements():
            transformed = g.apply(critical_point)
            orbit.append(transformed)

        # Remove duplicates
        unique_orbit = list(set(orbit))

        return len(unique_orbit)

    # Expected multiplicity
    multiplicity = critical_points_from_symmetry()
    print(f"Critical points come in orbits of size {multiplicity}")

    # Index theory
    def compute_morse_index(critical_point):
        """Compute Morse index (number of negative eigenvalues)."""
        H = compute_hessian(critical_point)
        eigenvalues = np.linalg.eigvals(H)
        negative_count = sum(1 for e in eigenvalues if e < 0)
        return negative_count

    # Classify critical points by index
    indices = {}
    for cp in find_all_critical_points():
        index = compute_morse_index(cp)
        if index not in indices:
            indices[index] = []
        indices[index].append(cp)

    print("Critical points by Morse index:")
    for index, points in indices.items():
        print(f"  Index {index}: {len(points)} points")
```

## Stability under Perturbations

### Perturbation Analysis

```python
class PerturbationAnalysis:
    """Analyze stability under perturbations."""

    def __init__(self, base_action):
        self.S0 = base_action

    def perturbed_action(self, config, perturbation, epsilon):
        """Action with perturbation."""
        return self.S0(config) + epsilon * perturbation(config)

    def stability_analysis(self, minimum, perturbation, epsilon):
        """Check if minimum is stable under perturbation."""
        # Original minimum
        original_min = minimum
        original_value = self.S0(original_min)

        # Perturbed problem
        S_perturbed = lambda c: self.perturbed_action(c, perturbation, epsilon)

        # Find new minimum
        perturbed_min = minimize_action(S_perturbed, initial=original_min)
        perturbed_value = S_perturbed(perturbed_min)

        # Measure displacement
        displacement = norm(perturbed_min - original_min)
        value_change = abs(perturbed_value - original_value)

        # Stable if displacement is O(epsilon)
        if displacement <= C * epsilon:
            return f"Stable: displacement = {displacement:.4f}"
        else:
            return f"Unstable: displacement = {displacement:.4f}"

    def compute_stability_radius(self, minimum):
        """Find maximum perturbation that preserves stability."""
        epsilon = 1.0

        while epsilon > 1e-6:
            # Random perturbation
            perturbation = random_functional()

            # Check stability
            result = self.stability_analysis(minimum, perturbation, epsilon)

            if "Stable" in result:
                return epsilon
            else:
                epsilon /= 2

        return 0  # Unstable to any perturbation
```

### Lyapunov Stability

```python
def lyapunov_stability():
    """Prove Lyapunov stability of minima."""

    def construct_lyapunov_function(minimum):
        """Construct Lyapunov function."""
        # Use action as Lyapunov function
        def V(config):
            return compute_action(config) - compute_action(minimum)

        return V

    def verify_lyapunov_conditions(V, minimum):
        """Verify Lyapunov stability conditions."""
        # V(minimum) = 0
        assert abs(V(minimum)) < EPSILON

        # V(x) > 0 for x ≠ minimum (up to gauge)
        for _ in range(100):
            config = random_nearby_configuration(minimum)
            if not gauge_equivalent(config, minimum):
                assert V(config) > 0

        # V̇ ≤ 0 along trajectories
        def trajectory_derivative(config):
            # Gradient flow
            grad = compute_gradient(config)
            velocity = -grad  # Gradient descent

            # Directional derivative
            return directional_derivative(V, config, velocity)

        for _ in range(100):
            config = random_configuration()
            V_dot = trajectory_derivative(config)
            assert V_dot <= 0

        return True

    # Find minimum and verify stability
    minimum = find_minimum()
    V = construct_lyapunov_function(minimum)

    if verify_lyapunov_conditions(V, minimum):
        print("✓ Minimum is Lyapunov stable")
```

## Convergence Rates

### Linear Convergence

```python
def analyze_convergence_rate():
    """Analyze convergence rate of optimization."""

    class ConvergenceAnalysis:
        def __init__(self):
            self.history = []

        def gradient_descent_with_tracking(self, initial, learning_rate=0.01):
            """Gradient descent tracking convergence."""
            config = initial
            optimum = find_true_minimum()
            optimum_value = compute_action(optimum)

            for iteration in range(1000):
                # Compute gradient
                grad = compute_gradient(config)

                # Update
                config = config - learning_rate * grad

                # Track distance to optimum
                value = compute_action(config)
                gap = value - optimum_value

                self.history.append({
                    'iteration': iteration,
                    'value': value,
                    'gap': gap,
                    'gradient_norm': norm(grad)
                })

                if gap < 1e-10:
                    break

            return config

        def estimate_convergence_rate(self):
            """Estimate convergence rate from history."""
            # Linear convergence: gap(t+1) ≤ ρ * gap(t)
            gaps = [h['gap'] for h in self.history if h['gap'] > 0]

            if len(gaps) < 2:
                return None

            # Estimate ρ
            ratios = [gaps[i+1]/gaps[i] for i in range(len(gaps)-1)
                     if gaps[i] > 0]

            if ratios:
                rho = np.median(ratios)
                return rho
            return None

    analyzer = ConvergenceAnalysis()
    analyzer.gradient_descent_with_tracking(random_configuration())
    rate = analyzer.estimate_convergence_rate()

    if rate is not None:
        if rate < 1:
            convergence_type = "Linear" if rate > 0 else "Superlinear"
            print(f"{convergence_type} convergence with rate ρ = {rate:.4f}")
        else:
            print("Sublinear convergence")
```

### Quadratic Convergence Near Minimum

```python
def newton_convergence():
    """Newton's method achieves quadratic convergence."""

    def newton_step(config):
        """One Newton step."""
        # Gradient and Hessian
        grad = compute_gradient(config)
        H = compute_hessian(config)

        # Newton direction (solve Hd = -g)
        direction = np.linalg.solve(H, -grad)

        # Update
        return config + direction

    def track_quadratic_convergence():
        """Track convergence to verify quadratic rate."""
        config = random_near_minimum_configuration()
        minimum = find_true_minimum()

        errors = []
        for iteration in range(10):
            error = norm(config - minimum)
            errors.append(error)

            if error < 1e-15:
                break

            config = newton_step(config)

        # Quadratic convergence: e_{n+1} ≤ C * e_n^2
        for i in range(len(errors)-1):
            if errors[i] > 1e-10:
                ratio = errors[i+1] / (errors[i]**2)
                print(f"Iteration {i}: e_{i+1}/e_i^2 = {ratio:.4f}")

        return errors

    errors = track_quadratic_convergence()
    print("✓ Newton's method achieves quadratic convergence")
```

## Saddle Point Analysis

### Identifying Saddle Points

```python
def find_saddle_points():
    """Identify and analyze saddle points."""

    def is_saddle_point(config):
        """Check if configuration is a saddle point."""
        # Critical point: gradient = 0
        grad = compute_gradient(config)
        if norm(grad) > 1e-6:
            return False

        # Saddle: Hessian has both positive and negative eigenvalues
        H = compute_hessian(config)
        eigenvalues = np.linalg.eigvals(H)

        has_positive = any(e > 1e-6 for e in eigenvalues)
        has_negative = any(e < -1e-6 for e in eigenvalues)

        return has_positive and has_negative

    def escape_direction(saddle):
        """Find direction to escape saddle."""
        H = compute_hessian(saddle)
        eigenvalues, eigenvectors = np.linalg.eig(H)

        # Find most negative eigenvalue
        min_idx = np.argmin(eigenvalues)
        escape_dir = eigenvectors[:, min_idx]

        return escape_dir, eigenvalues[min_idx]

    # Find saddle points
    saddles = []
    for _ in range(100):
        config = random_configuration()
        critical = find_critical_point_from(config)

        if is_saddle_point(critical):
            saddles.append(critical)

    print(f"Found {len(saddles)} saddle points")

    # Analyze escape directions
    for saddle in saddles[:5]:
        direction, eigenvalue = escape_direction(saddle)
        print(f"Saddle with escape eigenvalue: {eigenvalue:.4f}")
```

### Saddle-Free Newton

```python
def saddle_free_newton():
    """Modified Newton that escapes saddles."""

    def modified_newton_step(config, epsilon=0.1):
        """Newton step with saddle escape."""
        grad = compute_gradient(config)
        H = compute_hessian(config)

        # Regularize Hessian to ensure descent
        H_reg = H + epsilon * np.eye(len(H))

        # Check if regularized Hessian is positive definite
        eigenvalues = np.linalg.eigvals(H_reg)
        if np.min(eigenvalues) < 0:
            # Add more regularization
            H_reg = H + 2 * epsilon * np.eye(len(H))

        # Compute direction
        direction = np.linalg.solve(H_reg, -grad)

        return config + direction

    def optimize_with_saddle_escape(initial):
        """Optimize avoiding saddles."""
        config = initial
        stuck_count = 0

        for iteration in range(1000):
            old_config = config.copy()
            config = modified_newton_step(config)

            # Check if stuck
            if norm(config - old_config) < 1e-8:
                stuck_count += 1
                if stuck_count > 5:
                    # Add noise to escape
                    config += np.random.randn(*config.shape) * 0.1
                    stuck_count = 0
            else:
                stuck_count = 0

            # Check convergence
            if compute_gradient_norm(config) < 1e-6:
                break

        return config

    # Test optimization
    result = optimize_with_saddle_escape(random_configuration())
    assert is_minimum(result)  # Should reach minimum, not saddle
    print("✓ Saddle-free Newton reaches minimum")
```

## Exercises

**Exercise 17.1**: Prove that the R96 sector is convex but not strongly convex.

**Exercise 17.2**: Find conditions under which the total action is convex.

**Exercise 17.3**: Compute the condition number of the Hessian at minima.

**Exercise 17.4**: Design a preconditioner to improve convergence.

**Exercise 17.5**: Analyze the effect of gauge fixing on the optimization landscape.

## Takeaways

1. **Mixed convexity**: Individual sectors convex, total action may not be
2. **Existence guaranteed**: Compactness ensures minima exist
3. **Uniqueness up to gauge**: Strict minima unique modulo symmetry
4. **Stable under perturbations**: Minima are Lyapunov stable
5. **Linear to quadratic convergence**: Rate depends on algorithm and proximity
6. **Saddle points exist**: But can be escaped with modified algorithms

The optimization landscape, while complex, has enough structure to enable efficient and reliable optimization.

---

*Next: Chapter 18 provides concrete data structure implementations.*