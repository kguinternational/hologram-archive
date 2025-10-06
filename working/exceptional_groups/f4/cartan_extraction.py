"""
F₄ Cartan Matrix Extraction.

Extract and verify the Cartan matrix from the quotient adjacency structure.
"""
import sys
import os
from typing import List, Tuple, Optional
import json

sys.path.append(os.path.join(os.path.dirname(__file__), '../..'))

from tier_a_embedding.analysis import QuotientAnalyzer


class CartanExtractor:
    """Extract Cartan matrix from root system adjacency."""

    def __init__(self, adjacency_matrix: List[List[bool]], degree_sequence: List[int]):
        """
        Initialize with adjacency data.

        Args:
            adjacency_matrix: 48x48 adjacency matrix
            degree_sequence: Degree of each vertex
        """
        self.adjacency = adjacency_matrix
        self.degrees = degree_sequence
        self.n = len(adjacency_matrix)

    def find_simple_roots(self) -> List[int]:
        """
        Find 4 simple roots that generate F₄.

        F₄ has rank 4, so we need 4 simple roots.
        Strategy: Look for roots with specific connectivity patterns.
        """
        # For F₄, we expect the Dynkin diagram: o---o==>o---o
        # This means:
        # - 2 roots connected by single bond (endpoints)
        # - 2 roots connected by double bond (middle)

        # Start with roots of different degrees as candidates
        degree_5_roots = [i for i, d in enumerate(self.degrees) if d == 5]
        degree_6_roots = [i for i, d in enumerate(self.degrees) if d == 6]

        # Try to find 4 roots with minimal connections to each other
        # Simple roots should form a tree (no cycles)
        candidates = self._find_tree_subset(4)

        if len(candidates) == 4:
            print(f"Found simple root candidates: {candidates}")
            return candidates

        # Fallback: take first 4 roots
        print("Warning: Using first 4 roots as simple roots")
        return list(range(4))

    def _find_tree_subset(self, size: int) -> List[int]:
        """Find a subset of given size that forms a tree."""
        # Start with vertex of minimum degree
        min_degree_vertex = min(range(self.n), key=lambda i: self.degrees[i])
        subset = [min_degree_vertex]
        used = {min_degree_vertex}

        # Greedily extend
        while len(subset) < size and len(subset) < self.n:
            # Find vertex connected to exactly one vertex in subset
            best_vertex = None
            for v in range(self.n):
                if v not in used:
                    connections = sum(1 for u in subset if self.adjacency[v][u])
                    if connections == 1:  # Connected to exactly one
                        best_vertex = v
                        break

            if best_vertex is None:
                # No perfect candidate, take any connected vertex
                for v in range(self.n):
                    if v not in used:
                        for u in subset:
                            if self.adjacency[v][u]:
                                best_vertex = v
                                break
                        if best_vertex is not None:
                            break

            if best_vertex is None:
                break

            subset.append(best_vertex)
            used.add(best_vertex)

        return subset[:size]

    def extract_cartan_submatrix(self, simple_roots: List[int]) -> List[List[int]]:
        """
        Extract 4x4 Cartan matrix for given simple roots.

        The Cartan matrix C has entries:
        - C[i][i] = 2 (diagonal)
        - C[i][j] = -m where m is the bond multiplicity
        """
        n = len(simple_roots)
        cartan = [[0] * n for _ in range(n)]

        for i in range(n):
            for j in range(n):
                if i == j:
                    cartan[i][j] = 2
                else:
                    # Check if roots are adjacent
                    ri, rj = simple_roots[i], simple_roots[j]
                    if self.adjacency[ri][rj]:
                        # Connected - determine bond type
                        # For now, assume single bonds (-1)
                        # TODO: Determine double bonds for F₄
                        cartan[i][j] = -1
                    else:
                        cartan[i][j] = 0

        return cartan

    def refine_cartan_for_f4(self, cartan: List[List[int]]) -> List[List[int]]:
        """
        Refine Cartan matrix to match F₄ pattern.

        F₄ Cartan matrix should be:
        [[ 2, -1,  0,  0],
         [-1,  2, -2,  0],  # Note the -2 for double bond
         [ 0, -1,  2, -1],
         [ 0,  0, -1,  2]]
        """
        # Look for the characteristic -2 entry pattern
        # This requires analyzing bond multiplicities

        # For now, return as-is with a note
        # TODO: Implement proper bond multiplicity detection
        return cartan


def extract_cartan_matrix(f4_structure) -> Tuple[List[List[int]], bool]:
    """
    Extract Cartan matrix from F₄ structure.

    Args:
        f4_structure: F4Structure from sign_class_analysis

    Returns:
        Tuple of (cartan_matrix, is_valid)
    """
    extractor = CartanExtractor(
        f4_structure.adjacency_matrix,
        f4_structure.degree_sequence
    )

    # Find simple roots
    simple_roots = extractor.find_simple_roots()

    # Extract Cartan submatrix
    cartan = extractor.extract_cartan_submatrix(simple_roots)

    # Refine for F₄
    cartan = extractor.refine_cartan_for_f4(cartan)

    # Verify basic properties
    is_valid = verify_cartan_properties(cartan)

    return cartan, is_valid


def verify_cartan_properties(cartan: List[List[int]]) -> bool:
    """Verify basic Cartan matrix properties."""
    n = len(cartan)

    # Check size (F₄ has rank 4)
    if n != 4:
        print(f"Wrong size: {n} (expected 4)")
        return False

    # Check diagonal = 2
    for i in range(n):
        if cartan[i][i] != 2:
            print(f"Diagonal element [{i}][{i}] = {cartan[i][i]} (expected 2)")
            return False

    # Check off-diagonal <= 0
    for i in range(n):
        for j in range(n):
            if i != j and cartan[i][j] > 0:
                print(f"Off-diagonal [{i}][{j}] = {cartan[i][j]} (expected <= 0)")
                return False

    return True


def verify_f4_cartan(cartan: List[List[int]]) -> bool:
    """
    Verify this is specifically the F₄ Cartan matrix.

    F₄ Cartan matrix:
    [[ 2, -1,  0,  0],
     [-1,  2, -2,  0],
     [ 0, -1,  2, -1],
     [ 0,  0, -1,  2]]
    """
    expected_f4 = [
        [ 2, -1,  0,  0],
        [-1,  2, -2,  0],
        [ 0, -1,  2, -1],
        [ 0,  0, -1,  2]
    ]

    # Check various permutations since root ordering may differ
    # For now, just check if we have the right entries
    our_entries = sorted([cartan[i][j] for i in range(4) for j in range(4)])
    f4_entries = sorted([expected_f4[i][j] for i in range(4) for j in range(4)])

    entries_match = our_entries == f4_entries

    if entries_match:
        print("✓ Cartan matrix has F₄ entry pattern!")
    else:
        print(f"Entry mismatch:")
        print(f"  Expected: {f4_entries}")
        print(f"  Got: {our_entries}")

    # Check for the characteristic -2 entry
    has_double_bond = any(cartan[i][j] == -2 for i in range(4) for j in range(4))

    if not has_double_bond:
        print("⚠ Warning: No -2 entry found (F₄ has a double bond)")

    return entries_match


def display_cartan_matrix(cartan: List[List[int]], name: str = "Cartan Matrix"):
    """Display Cartan matrix nicely."""
    print(f"\n{name}:")
    print("     ", end="")
    for j in range(len(cartan)):
        print(f"{j:3}", end=" ")
    print()

    for i, row in enumerate(cartan):
        print(f"{i:3}: ", end="")
        for val in row:
            if val == 0:
                print("  .", end=" ")
            else:
                print(f"{val:3}", end=" ")
        print()


if __name__ == "__main__":
    # Test with extracted F₄ structure
    from sign_class_analysis import extract_f4_from_sign_classes

    print("="*60)
    print("F₄ CARTAN MATRIX EXTRACTION")
    print("="*60)

    f4_structure, _ = extract_f4_from_sign_classes()

    cartan, is_valid = extract_cartan_matrix(f4_structure)

    display_cartan_matrix(cartan, "Extracted Cartan Matrix")

    print(f"\nBasic validity: {is_valid}")
    print(f"F₄ verification: {verify_f4_cartan(cartan)}")