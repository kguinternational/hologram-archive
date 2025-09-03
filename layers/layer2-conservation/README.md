# Layer 2: Conservation Layer

## Overview

Layer 2 implements conservation laws and proof systems for Atlas-12288, including domain management, SHA-256 witness generation, and conserved memory operations. This layer ensures mathematical integrity and provides cryptographic validation.

## Components

### LLVM IR Modules
- `llvm/domains.ll` - Domain lifecycle and state management
- `llvm/witness.ll` - SHA-256 witness generation and verification
- `llvm/memory.ll` - Conserved memory operations and tracking

### Runtime Implementation  
- `runtime/conservation.c` - C runtime for domain and conservation operations

### Headers
- `include/atlas-conservation.h` - Conservation layer public API

### Tests
- `tests/test-domains.ll` - Domain lifecycle testing
- `tests/test-witness.ll` - Witness operations testing
- `tests/test-conservation.ll` - Conservation law validation
- `tests/test-conservation-l2.ll` - Layer 2 specific tests
- `tests/test-l2-focused.ll` - Focused Layer 2 operations
- `tests/test-concurrency.c` - Concurrent operations testing
- `tests/test-properties.c` - Property-based testing
- `tests/integration/` - Layer 2 integration tests

## Responsibilities

1. **Domain Management**: Atlas computation domain lifecycle:
   - Domain creation, attachment, and destruction
   - State transition validation (CREATED → ATTACHED → ACTIVE → DESTROYED)
   - Thread-safe domain operations
   - Budget tracking and validation

2. **Conservation Laws**: Mathematical integrity enforcement:
   - Mod-96 budget conservation across operations
   - Conservation violation detection and reporting
   - Delta computation and validation
   - Conserved memory operation tracking

3. **Witness System**: Cryptographic proof generation:
   - SHA-256 witness generation for data integrity
   - Witness verification and validation
   - Witness lifecycle management
   - Proof composition and chaining

4. **Memory Operations**: Conservation-aware memory management:
   - Witnessed memory allocation and deallocation
   - Conserved memcpy operations with integrity checking
   - Memory operation audit trails
   - Budget-aware memory tracking

## Mathematical Foundation

- **Conservation Law**: ∑ᵢ budgetᵢ ≡ 0 (mod 96)
- **Budget Arithmetic**: RL-96 ring operations
- **Witness Function**: SHA-256(data ⊕ domain_context)
- **Delta Invariant**: Δ(operation) = budget_final - budget_initial

## Dependencies

- **Layer 0**: Core types and operations
- **Layer 1**: Boundary operations and coordinate management
- **External**: SHA-256 implementation, threading primitives

## Interface Contract

Layer 2 enforces strict conservation with:
- **Budget Conservation**: All operations preserve mod-96 budget sum
- **State Consistency**: Domain state transitions follow strict FSM
- **Witness Integrity**: All data modifications generate cryptographic proofs
- **Thread Safety**: Concurrent domain operations are fully synchronized
- **Error Handling**: Conservation violations are detected and reported

## Error Codes

- `ATLAS_SUCCESS` - Operation completed successfully
- `ATLAS_ERROR_INVALID_ARGUMENT` - Invalid function argument
- `ATLAS_ERROR_OUT_OF_MEMORY` - Memory allocation failed
- `ATLAS_ERROR_INVALID_STATE` - Invalid domain state transition
- `ATLAS_ERROR_BUDGET_INSUFFICIENT` - Insufficient budget for operation
- `ATLAS_ERROR_CONSERVATION_VIOLATION` - Conservation law violated
- `ATLAS_ERROR_WITNESS_INVALID` - Witness verification failed
- `ATLAS_ERROR_DOMAIN_DESTROYED` - Domain has been destroyed

## Build Output

- Static library: `libatlas-conservation.a`
- Depends on: `libatlas-boundary.a`, `libatlas-core.a`

## Version

- Interface Version: 1.0.0
- Stability: Stable