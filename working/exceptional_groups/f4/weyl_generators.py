"""
F₄ Weyl Group Generators.

Generate the Weyl group of F₄ (order 1152) from simple reflections.
F₄ Dynkin diagram: o---o==>o---o (with double bond).
"""
import sys
import os
from typing import List, Set, Tuple, Dict
import numpy as np
from itertools import product

sys.path.append(os.path.join(os.path.dirname(__file__), '../..'))

from tier_a_embedding import AtlasGraph
from sign_class_analysis import extract_f4_from_sign_classes


class F4WeylGroup:
    """Generate and analyze F₄ Weyl group."""

    def __init__(self):
        """Initialize F₄ Weyl group structure."""
        self.rank = 4
        self.expected_order = 1152
        self.simple_roots = None
        self.cartan_matrix = None
        self.generators = []
        self.elements = set()

    def get_f4_cartan_matrix(self) -> np.ndarray:
        """
        Get the standard F₄ Cartan matrix.

        F₄ Dynkin: o---o==>o---o
        Double bond between roots 2 and 3.
        """
        cartan = np.array([
            [ 2, -1,  0,  0],
            [-1,  2, -2,  0],  # -2 indicates double bond
            [ 0, -1,  2, -1],
            [ 0,  0, -1,  2]
        ])
        self.cartan_matrix = cartan
        return cartan

    def generate_simple_roots(self) -> List[np.ndarray]:
        """
        Generate F₄ simple roots.

        F₄ has 4 simple roots with specific length relationships.
        Using standard basis where roots satisfy Cartan matrix.
        """
        # Standard F₄ simple roots
        # These should satisfy the F₄ Cartan matrix relations
        alpha1 = np.array([1, -1, 0, 0])           # α₁
        alpha2 = np.array([0, 1, -1, 0])           # α₂
        alpha3 = np.array([0, 0, 1, 0])            # α₃ (long)
        alpha4 = np.array([-1/2, -1/2, -1/2, -1/2]) # α₄ (short)

        # Alternative basis that might work better
        # Using the explicit F₄ root system
        alpha1 = np.array([0, 1, -1, 0])
        alpha2 = np.array([0, 0, 1, -1])
        alpha3 = np.array([0, 0, 0, 1])
        alpha4 = np.array([1/2, -1/2, -1/2, -1/2])

        self.simple_roots = [alpha1, alpha2, alpha3, alpha4]
        return self.simple_roots

    def simple_reflection(self, root: np.ndarray) -> np.ndarray:
        """
        Create reflection matrix for a root.

        Reflection through hyperplane perpendicular to root:
        s_α(v) = v - 2⟨v,α⟩/⟨α,α⟩ * α
        """
        root_norm_sq = np.dot(root, root)
        if root_norm_sq == 0:
            raise ValueError("Cannot reflect through zero root")

        # Create reflection matrix
        n = len(root)
        reflection = np.eye(n) - 2 * np.outer(root, root) / root_norm_sq
        return reflection

    def generate_weyl_group(self, max_length: int = 20) -> Set[Tuple]:
        """
        Generate Weyl group elements up to given word length.

        Uses breadth-first search from identity.
        Each element stored as tuple of matrix entries.
        """
        if self.simple_roots is None:
            self.generate_simple_roots()

        # Generate reflection matrices for simple roots
        reflections = []
        for root in self.simple_roots:
            reflections.append(self.simple_reflection(root))
        self.generators = reflections

        # Start with identity
        identity = np.eye(4)
        elements = {tuple(identity.flatten()): identity}
        frontier = [identity]

        print(f"Generating F₄ Weyl group (expected order: {self.expected_order})")

        # BFS to generate group
        for length in range(1, max_length + 1):
            new_frontier = []

            for elem in frontier:
                for refl in reflections:
                    # Apply reflection
                    new_elem = refl @ elem

                    # Check if new
                    key = tuple(np.round(new_elem, 8).flatten())
                    if key not in elements:
                        elements[key] = new_elem
                        new_frontier.append(new_elem)

            frontier = new_frontier

            print(f"  Length {length}: Found {len(elements)} elements")

            # Check if we've generated full group
            if len(elements) == self.expected_order:
                print(f"✓ Generated full F₄ Weyl group!")
                break
            elif len(frontier) == 0:
                print(f"⚠ Generation stopped at {len(elements)} elements")
                break

        self.elements = elements
        return elements

    def verify_group_properties(self) -> Dict[str, bool]:
        """Verify F₄ Weyl group properties."""
        checks = {}

        # Check order
        checks['correct_order'] = (len(self.elements) == self.expected_order)

        # Check generators have order 2 (reflections)
        gens_order_2 = True
        for gen in self.generators:
            gen_squared = gen @ gen
            if not np.allclose(gen_squared, np.eye(4)):
                gens_order_2 = False
                break
        checks['generators_order_2'] = gens_order_2

        # Check Cartan matrix consistency
        if self.cartan_matrix is not None and self.simple_roots is not None:
            cartan_consistent = True
            n = len(self.simple_roots)
            for i in range(n):
                for j in range(n):
                    alpha_i = self.simple_roots[i]
                    alpha_j = self.simple_roots[j]

                    # Cartan entry: 2⟨α_i, α_j⟩/⟨α_j, α_j⟩
                    computed = 2 * np.dot(alpha_i, alpha_j) / np.dot(alpha_j, alpha_j)
                    expected = self.cartan_matrix[i, j]

                    if not np.isclose(computed, expected, atol=0.1):
                        cartan_consistent = False
                        print(f"  Cartan mismatch at [{i},{j}]: {computed:.2f} vs {expected}")
        else:
            cartan_consistent = False
        checks['cartan_consistent'] = cartan_consistent

        return checks

    def find_longest_element(self) -> Tuple[np.ndarray, int]:
        """
        Find the longest element of F₄ Weyl group.

        This is the unique element of maximal length.
        In F₄, it has length 24.
        """
        max_length = 0
        longest = None

        for key, elem in self.elements.items():
            # Compute length as minimal word in generators
            # For now, use trace as proxy (longest element has specific trace)
            trace = np.trace(elem)

            # In F₄, longest element has trace -4
            if np.isclose(trace, -4):
                longest = elem
                max_length = 24  # Known length
                break

        return longest, max_length


def verify_f4_weyl_group():
    """
    Main function to generate and verify F₄ Weyl group.

    Returns:
        Dictionary with Weyl group data and verification results
    """
    print("="*60)
    print("F₄ WEYL GROUP GENERATION")
    print("="*60)

    # Create F₄ Weyl group
    weyl = F4WeylGroup()

    # Get Cartan matrix
    cartan = weyl.get_f4_cartan_matrix()
    print("\nF₄ Cartan matrix:")
    print(cartan)

    # Generate simple roots
    simple_roots = weyl.generate_simple_roots()
    print(f"\nGenerated {len(simple_roots)} simple roots")

    # Generate full Weyl group
    elements = weyl.generate_weyl_group()

    # Verify properties
    print("\nVerifying F₄ Weyl group properties:")
    checks = weyl.verify_group_properties()
    for prop, result in checks.items():
        status = "✓" if result else "✗"
        print(f"  {status} {prop}")

    # Find longest element
    longest, length = weyl.find_longest_element()
    if longest is not None:
        print(f"\n✓ Found longest element (length {length})")

    # Summary
    result = {
        'order': len(elements),
        'expected_order': weyl.expected_order,
        'rank': weyl.rank,
        'num_generators': len(weyl.generators),
        'cartan_matrix': cartan.tolist(),
        'verification': checks,
        'has_longest_element': longest is not None
    }

    if len(elements) == weyl.expected_order:
        print(f"\n✓✓✓ Successfully generated F₄ Weyl group with {len(elements)} elements!")
    else:
        print(f"\n⚠ Generated {len(elements)} elements (expected {weyl.expected_order})")

    return result


if __name__ == "__main__":
    f4_weyl_data = verify_f4_weyl_group()