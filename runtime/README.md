# Atlas-12288 Layer 2 Host Runtime

A high-performance C runtime library implementing the Layer 2 host interface for the Atlas-12288 computational model. This runtime provides domain lifecycle management, budget arithmetic, witness operations, and conservation verification with thread-safe atomic operations.

## Features

### Core Functionality
- **Domain Lifecycle Management**: Create, attach, verify, commit, and destroy computational domains
- **Thread-Safe Budget Arithmetic**: Atomic mod-96 budget allocation and release operations
- **Cryptographic Witnesses**: Generate and verify computational proofs for memory regions
- **Conservation Verification**: Check conservation law compliance with delta calculations
- **Error Handling**: Comprehensive error reporting with thread-local error state

### Architecture
- **Opaque API Design**: Clean separation between public interface and internal implementation
- **Atomic State Transitions**: Thread-safe domain state management using atomic operations
- **Memory Safety**: No memory leaks with proper resource lifecycle management
- **WASM Support**: Single-threaded mode for WebAssembly targets
- **LLVM Integration**: Hot path functions delegated to optimized LLVM IR implementations

## Quick Start

### Building

```bash
# Standard build
make

# Debug build with sanitizers  
make DEBUG=1

# Single-threaded build for WASM
make WASM=1

# Standalone build (uses stubs instead of LLVM)
make RUNTIME_STANDALONE=1
```

### Basic Usage

```c
#include "atlas-runtime.h"

int main() {
    // Create a domain with 12KB memory and budget class 42
    atlas_domain_t* domain = atlas_domain_create(12288, 42);
    if (!domain) {
        printf("Failed to create domain: %s\n", 
               atlas_error_string(atlas_get_last_error()));
        return 1;
    }
    
    // Attach memory for computation
    uint8_t buffer[12288] = {0};
    if (atlas_domain_attach(domain, buffer, sizeof(buffer)) != 0) {
        printf("Failed to attach memory\n");
        atlas_domain_destroy(domain);
        return 1;
    }
    
    // Allocate budget for operations
    if (!atlas_budget_alloc(domain, 10)) {
        printf("Insufficient budget\n");
    }
    
    // Verify domain integrity
    if (atlas_domain_verify(domain)) {
        printf("Domain verification passed\n");
    }
    
    // Commit changes and generate witness
    if (atlas_domain_commit(domain) == 0) {
        printf("Domain committed successfully\n");
    }
    
    // Clean up
    atlas_domain_destroy(domain);
    return 0;
}
```

### Compilation and Linking

```bash
# Compile
gcc -std=c11 -I/path/to/atlas/runtime/include -c your_program.c

# Link with static library
gcc your_program.o -L/path/to/atlas/runtime/lib -latlas-runtime -lm -o your_program

# Link with shared library  
gcc your_program.o -L/path/to/atlas/runtime/lib -latlas-runtime -lm -o your_program
```

## API Reference

### Domain Lifecycle

- `atlas_domain_create(size_t bytes, uint8_t budget_class)` - Create new domain
- `atlas_domain_attach(atlas_domain_t* domain, void* base, size_t len)` - Attach memory
- `atlas_domain_verify(const atlas_domain_t* domain)` - Verify domain integrity  
- `atlas_domain_commit(atlas_domain_t* domain)` - Commit and generate witness
- `atlas_domain_destroy(atlas_domain_t* domain)` - Destroy domain

### Budget Management

- `atlas_budget_alloc(atlas_domain_t* domain, uint8_t amt)` - Allocate budget units
- `atlas_budget_release(atlas_domain_t* domain, uint8_t amt)` - Release budget units

### Witness Operations

- `atlas_witness_generate(const void* base, size_t len)` - Generate cryptographic witness
- `atlas_witness_verify(const atlas_witness_t* witness, const void* base, size_t len)` - Verify witness
- `atlas_witness_destroy(atlas_witness_t* witness)` - Destroy witness

### Conservation Functions

- `atlas_conserved_delta(const void* before, const void* after, size_t len)` - Calculate conservation delta

### Error Handling

- `atlas_get_last_error(void)` - Get last error code
- `atlas_error_string(atlas_error_t error)` - Get human-readable error message

## Thread Safety

- **Domain Operations**: Safe for different domains, not for same domain across threads
- **Budget Operations**: Fully atomic and thread-safe using compare-and-swap
- **Witness Operations**: Thread-safe for read-only operations
- **Error State**: Thread-local error handling

## Memory Management

- All `atlas_*_create()` functions require corresponding `atlas_*_destroy()` calls
- Attached memory is not owned by domains - caller manages lifetime  
- Error conditions indicated by return values and `atlas_get_last_error()`
- No memory leaks when used correctly

## Performance Characteristics

- **Verification Operations**: O(n) in memory size
- **Budget Operations**: O(1) with atomic synchronization
- **Witness Generation/Verification**: Involves cryptographic computation
- **Conservation Checks**: O(n) byte summation with mod-96 arithmetic

## Configuration Options

### Compile-Time Options

- `ATLAS_SINGLE_THREAD` - Disable thread safety for single-threaded environments
- `DEBUG` - Enable debug builds with address/undefined behavior sanitizers
- `WASM` - Target WebAssembly with single-threaded mode
- `RUNTIME_STANDALONE` - Build without LLVM dependencies (uses stubs)

### Build Variables

- `PREFIX` - Installation prefix (default: `/usr/local`)
- `DEBUG=1` - Debug build with sanitizers
- `WASM=1` - WebAssembly target build
- `RUNTIME_STANDALONE=1` - Standalone build mode

## Testing

```bash
# Run basic tests
make test

# Memory leak testing with Valgrind
make test-memory

# Performance benchmarks  
make benchmark
```

## Installation

```bash
# Install to system directories
sudo make install

# Install to custom location
make install PREFIX=$HOME/.local
```

## Integration with LLVM

The runtime is designed to work with the Atlas-12288 LLVM infrastructure:

- Hot path operations delegate to optimized LLVM IR implementations
- Conservation verification uses LLVM intrinsics for performance
- Witness generation leverages cryptographic LLVM functions  
- Domain management integrates with LLVM memory allocation

When built in standalone mode, stub implementations provide basic functionality for testing and development without requiring the full LLVM Atlas library.

## Error Codes

- `ATLAS_SUCCESS` (0) - Operation completed successfully
- `ATLAS_ERROR_INVALID_ARGUMENT` (1) - Invalid function argument
- `ATLAS_ERROR_OUT_OF_MEMORY` (2) - Memory allocation failed
- `ATLAS_ERROR_INVALID_STATE` (3) - Invalid domain state transition
- `ATLAS_ERROR_BUDGET_INSUFFICIENT` (4) - Insufficient budget for operation
- `ATLAS_ERROR_CONSERVATION_VIOLATION` (5) - Conservation law violated
- `ATLAS_ERROR_WITNESS_INVALID` (6) - Witness verification failed
- `ATLAS_ERROR_DOMAIN_DESTROYED` (7) - Domain has been destroyed

## License

MIT License - Copyright (c) 2024-2025 UOR Foundation

## Architecture Notes

This Layer 2 runtime implements the host-side interface for Atlas-12288 computational domains. It provides:

1. **State Machine**: Domains transition through CREATED → ATTACHED → VERIFIED → COMMITTED → DESTROYED states
2. **Budget Arithmetic**: All operations use mod-96 arithmetic (0..95 range) with atomic updates
3. **Conservation Laws**: Memory modifications must preserve computational "energy" 
4. **Witness System**: Cryptographic proofs ensure computational integrity
5. **Isolation**: Domains cannot interfere with each other without explicit interaction

The implementation prioritizes correctness, thread safety, and performance, making it suitable for production use in high-performance computational environments.