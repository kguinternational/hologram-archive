# Atlas Manifold Security Best Practices

## Overview

This guide provides security best practices for developers working with the Atlas Manifold Layer 4 codebase, with special attention to unsafe code, FFI boundaries, and mathematical integrity guarantees.

## Core Security Principles

### 1. Defense in Depth

Every security-critical operation should have multiple layers of validation:

```rust
// ✅ GOOD: Multiple validation layers
#[no_mangle]
pub extern "C" fn atlas_projection_get_dimensions(
    handle: *const CAtlasProjectionHandle,
    width: *mut u32,
    height: *mut u32,
) -> c_int {
    // Layer 1: Null pointer validation
    if handle.is_null() || width.is_null() || height.is_null() {
        return error_to_code(&AtlasError::InvalidInput("null pointer"));
    }

    unsafe {
        // Layer 2: Handle structure validation
        let c_handle = &*handle;
        if c_handle.inner.is_null() {
            return error_to_code(&AtlasError::InvalidInput("invalid handle"));
        }
        
        // Layer 3: Internal state validation
        let atlas_handle = &*(c_handle.inner as *const AtlasProjectionHandle);
        if let Some(projection) = atlas_handle.as_ref() {
            // Layer 4: Business logic validation
            let (w, h) = projection.get_dimensions();
            *width = w;
            *height = h;
            0
        } else {
            error_to_code(&AtlasError::InvalidInput("corrupted handle"))
        }
    }
}
```

### 2. Fail-Safe Error Handling

Always design error conditions to fail safely rather than unsafely:

```rust
// ✅ GOOD: Safe failure modes
fn safe_array_access(data: &[u8], index: usize) -> AtlasResult<u8> {
    data.get(index)
        .copied()
        .ok_or_else(|| AtlasError::InvalidInput("index out of bounds"))
}

// ❌ BAD: Unsafe failure mode
fn unsafe_array_access(data: *const u8, len: usize, index: usize) -> u8 {
    unsafe {
        if index < len {
            *data.add(index)  // Could still be unsafe if data is invalid
        } else {
            0  // Silent failure might mask bugs
        }
    }
}
```

## FFI Security Guidelines

### 1. Pointer Validation Pattern

Use this standard pattern for all FFI functions receiving pointers:

```rust
#[no_mangle]
pub extern "C" fn your_ffi_function(
    ptr: *const SomeType,
    size: size_t,
    output: *mut OutputType,
) -> c_int {
    // Step 1: Validate all pointers
    if ptr.is_null() || output.is_null() || size == 0 {
        return error_to_code(&AtlasError::InvalidInput("null pointer or zero size"));
    }

    // Step 2: Validate size constraints
    if size > MAX_SAFE_SIZE {
        return error_to_code(&AtlasError::InvalidInput("size too large"));
    }

    unsafe {
        // Step 3: Create safe slice/reference
        let input_slice = core::slice::from_raw_parts(ptr, size);
        
        // Step 4: Perform safe operations
        let result = safe_processing(input_slice)?;
        
        // Step 5: Safe output writing
        *output = result;
        0
    }
}
```

### 2. Handle Management Pattern

Use this pattern for managing opaque handles:

```rust
/// Opaque handle for FFI safety
#[repr(transparent)]
pub struct CHandle {
    inner: *mut c_void,
}

impl CHandle {
    fn new(data: InternalType) -> *mut Self {
        let boxed_data = Box::new(data);
        let boxed_handle = Box::new(CHandle {
            inner: Box::into_raw(boxed_data) as *mut c_void,
        });
        Box::into_raw(boxed_handle)
    }

    unsafe fn as_ref(&self) -> Option<&InternalType> {
        if self.inner.is_null() {
            None
        } else {
            let internal = &*(self.inner as *const InternalType);
            Some(internal)
        }
    }

    unsafe fn destroy(handle: *mut Self) {
        if !handle.is_null() {
            let c_handle = Box::from_raw(handle);
            if !c_handle.inner.is_null() {
                let _internal = Box::from_raw(c_handle.inner as *mut InternalType);
                // RAII cleanup automatic
            }
        }
    }
}
```

### 3. Memory Safety Checklist

For every unsafe block, verify:

- [ ] All pointer parameters are validated for null
- [ ] Array/slice bounds are checked before access
- [ ] Memory allocation/deallocation is properly paired
- [ ] No use-after-free or double-free conditions possible
- [ ] Integer overflow cannot occur in size calculations
- [ ] External data is validated before processing

## Unsafe Code Best Practices

### 1. Safety Documentation

Every unsafe block must have a safety comment explaining why it's safe:

```rust
// ✅ GOOD: Detailed safety documentation
unsafe {
    // SAFETY: ptr is validated to be non-null above, and size is checked to be > 0
    // The caller guarantees that ptr points to at least `size` valid bytes
    // slice::from_raw_parts requires these exact preconditions
    let input_slice = core::slice::from_raw_parts(ptr, size);
    process_slice(input_slice)
}

// ❌ BAD: No safety documentation
unsafe {
    let input_slice = core::slice::from_raw_parts(ptr, size);
    process_slice(input_slice)
}
```

### 2. Minimize Unsafe Scope

Keep unsafe blocks as small as possible:

```rust
// ✅ GOOD: Minimal unsafe scope
fn process_data(ptr: *const u8, size: usize) -> AtlasResult<Vec<u8>> {
    if ptr.is_null() || size == 0 {
        return Err(AtlasError::InvalidInput("invalid parameters"));
    }

    let input_slice = unsafe {
        // SAFETY: ptr validated above, size > 0
        core::slice::from_raw_parts(ptr, size)
    };

    // Safe operations
    let mut result = Vec::with_capacity(size);
    for &byte in input_slice {
        result.push(transform_byte(byte));
    }
    Ok(result)
}

// ❌ BAD: Large unsafe scope
unsafe fn process_data(ptr: *const u8, size: usize) -> AtlasResult<Vec<u8>> {
    // Entire function is unsafe, making errors more likely
    if ptr.is_null() || size == 0 {
        return Err(AtlasError::InvalidInput("invalid parameters"));
    }

    let input_slice = core::slice::from_raw_parts(ptr, size);
    let mut result = Vec::with_capacity(size);
    for i in 0..size {
        result.push(transform_byte(*input_slice.get_unchecked(i)));
    }
    Ok(result)
}
```

### 3. Use Safe Abstractions

Prefer safe abstractions over unsafe operations when possible:

```rust
// ✅ GOOD: Safe slice operations
fn find_pattern(data: &[u8], pattern: &[u8]) -> Option<usize> {
    data.windows(pattern.len())
        .position(|window| window == pattern)
}

// ❌ BAD: Unnecessary unsafe operations  
unsafe fn find_pattern_unsafe(data: *const u8, data_len: usize, 
                               pattern: *const u8, pattern_len: usize) -> isize {
    if data.is_null() || pattern.is_null() {
        return -1;
    }
    
    for i in 0..data_len.saturating_sub(pattern_len) {
        let mut found = true;
        for j in 0..pattern_len {
            if *data.add(i + j) != *pattern.add(j) {
                found = false;
                break;
            }
        }
        if found {
            return i as isize;
        }
    }
    -1
}
```

## Mathematical Integrity

### 1. Conservation Law Validation

Always validate conservation laws to detect corruption:

```rust
/// Validate that data satisfies Layer 2 conservation laws
pub fn validate_conservation(data: &[u8]) -> AtlasResult<()> {
    let sum: u64 = data.iter().map(|&b| b as u64).sum();
    if sum % 96 != 0 {
        return Err(AtlasError::TopologyError(
            "Data violates conservation laws (sum % 96 != 0)"
        ));
    }
    Ok(())
}

/// Example usage in critical operations
fn process_conserved_data(data: &[u8]) -> AtlasResult<Vec<u8>> {
    // Validate input conservation
    validate_conservation(data)?;
    
    let result = transform_data(data)?;
    
    // Validate output conservation
    validate_conservation(&result)?;
    
    Ok(result)
}
```

### 2. Dimensional Consistency

Enforce dimensional consistency in all operations:

```rust
/// Type-safe dimensional operations
impl<const N: usize> AtlasPoint<N> {
    pub fn add(&self, other: &AtlasPoint<N>) -> AtlasPoint<N> {
        let mut coords = [0.0; N];
        for i in 0..N {
            coords[i] = self.coords[i] + other.coords[i];
        }
        AtlasPoint { coords }
    }
    
    // Compile-time prevention of dimensional mismatches
    // pub fn add_different_dim(&self, other: &AtlasPoint<M>) -> AtlasPoint<N> {
    //     // This won't compile - enforces dimensional safety
    // }
}
```

### 3. Numerical Stability

Use stable algorithms and check for numerical issues:

```rust
fn safe_matrix_inverse(matrix: &TransformMatrix<3, 3>) -> AtlasResult<TransformMatrix<3, 3>> {
    let determinant = compute_determinant(matrix);
    
    // Check for numerical stability
    if determinant.abs() < 1e-12 {
        return Err(AtlasError::NumericalError(
            "Matrix is singular or nearly singular"
        ));
    }
    
    // Proceed with numerically stable inversion
    let inverse = compute_stable_inverse(matrix, determinant)?;
    Ok(inverse)
}
```

## Testing Security

### 1. Property-Based Testing

Use property-based testing to verify security properties:

```rust
use proptest::prelude::*;

proptest! {
    /// Test that all FFI operations handle null pointers safely
    #[test]
    fn ffi_null_pointer_safety(size in 0usize..1000) {
        // All these should return errors, not crash
        let result1 = atlas_projection_create(0, std::ptr::null(), size);
        let result2 = atlas_projection_get_dimensions(
            std::ptr::null(), 
            std::ptr::null_mut(), 
            std::ptr::null_mut()
        );
        
        prop_assert!(result1.is_null());
        prop_assert_ne!(result2, 0); // Should return error code
    }
    
    /// Test conservation law preservation
    #[test] 
    fn conservation_preservation(data in conserved_data()) {
        let sum_before: u64 = data.iter().map(|&b| b as u64).sum();
        prop_assert_eq!(sum_before % 96, 0);
        
        // Process data through system
        if let Ok(processed) = process_conserved_data(&data) {
            let sum_after: u64 = processed.iter().map(|&b| b as u64).sum();
            prop_assert_eq!(sum_after % 96, 0);
        }
    }
}
```

### 2. Fuzzing Integration

Set up fuzzing for FFI boundaries:

```rust
// fuzz/fuzz_targets/ffi_projection.rs
#![no_main]
use libfuzzer_sys::fuzz_target;
use atlas_manifold::ffi::*;

fuzz_target!(|data: &[u8]| {
    if data.len() >= 4096 && data.len() % 4096 == 0 {
        // Test projection creation with arbitrary data
        let handle = atlas_projection_create_ffi(0, data.as_ptr(), data.len());
        
        if !handle.is_null() {
            // Test operations on created projection
            let mut width = 0u32;
            let mut height = 0u32;
            let _ = atlas_projection_get_dimensions(handle, &mut width, &mut height);
            
            // Cleanup
            atlas_projection_destroy(handle);
        }
    }
});
```

## Error Handling Security

### 1. Information Disclosure Prevention

Avoid leaking sensitive information in error messages:

```rust
// ✅ GOOD: Generic error messages for external callers
pub fn validate_user_input(input: &str) -> AtlasResult<ProcessedInput> {
    if input.contains("internal_debug_flag") {
        // Don't reveal internal implementation details
        return Err(AtlasError::InvalidInput("invalid input format"));
    }
    
    // Process input...
    Ok(ProcessedInput::new(input)?)
}

// ❌ BAD: Leaking internal details
pub fn validate_user_input_bad(input: &str) -> AtlasResult<ProcessedInput> {
    if input.contains("internal_debug_flag") {
        return Err(AtlasError::InvalidInput(
            "input contains forbidden internal_debug_flag which activates debug mode in production"
        ));
    }
    
    Ok(ProcessedInput::new(input)?)
}
```

### 2. Resource Cleanup on Error

Ensure resources are cleaned up even on error paths:

```rust
pub fn process_with_resources() -> AtlasResult<()> {
    let resource1 = acquire_resource1()?;
    
    let resource2 = match acquire_resource2() {
        Ok(r) => r,
        Err(e) => {
            // Clean up resource1 before propagating error
            release_resource1(resource1);
            return Err(e);
        }
    };
    
    // Or better, use RAII types that automatically clean up:
    let _resource1 = ResourceGuard::new(acquire_resource1()?);
    let _resource2 = ResourceGuard::new(acquire_resource2()?);
    
    // Resources automatically cleaned up when guards drop
    Ok(())
}
```

## Deployment Security

### 1. Build Configuration

Use security-focused build settings:

```toml
# Cargo.toml
[profile.release]
panic = "abort"           # Prevent unwinding attacks
lto = true               # Enable link-time optimization
codegen-units = 1        # Single codegen unit for optimization
opt-level = 3            # Maximum optimization

[profile.dev]
panic = "abort"          # Consistent behavior in dev/prod

# Security-focused lints
[lints.rust]
unsafe_code = "allow"    # We need unsafe for FFI
missing_docs = "warn"    # Document all public APIs
unreachable_pub = "warn" # Prevent API creep

[lints.clippy]
# Deny unsafe-related issues
undocumented_unsafe_blocks = "deny"
fn_to_numeric_cast_any = "deny"
transmute_ptr_to_ptr = "warn"
cast_possible_truncation = "warn"
```

### 2. Runtime Configuration

Configure runtime security features:

```rust
/// Initialize security-critical runtime settings
pub fn init_secure() -> AtlasResult<()> {
    // Set up memory protection if available
    #[cfg(target_os = "linux")]
    {
        // Enable stack protection features
        unsafe {
            libc::prctl(libc::PR_SET_DUMPABLE, 0);  // Prevent core dumps
        }
    }
    
    // Initialize cryptographic random number generator
    init_secure_random()?;
    
    // Set up conservation law validation
    enable_conservation_checking(true);
    
    Ok(())
}
```

## Security Monitoring

### 1. Logging Security Events

Log security-relevant events for monitoring:

```rust
use log::{warn, error, info};

fn handle_invalid_input(input: &str, error: &AtlasError) {
    warn!(
        "Invalid input rejected: {} characters, error: {}",
        input.len(),  // Log length but not content
        error
    );
}

fn handle_conservation_violation(data_size: usize, sum: u64) {
    error!(
        "Conservation law violation detected: {} bytes, sum={}, remainder={}",
        data_size,
        sum,
        sum % 96
    );
}
```

### 2. Metrics Collection

Track security-relevant metrics:

```rust
use std::sync::atomic::{AtomicU64, Ordering};

static INVALID_INPUTS_REJECTED: AtomicU64 = AtomicU64::new(0);
static CONSERVATION_VIOLATIONS: AtomicU64 = AtomicU64::new(0);
static FFI_CALLS_PROCESSED: AtomicU64 = AtomicU64::new(0);

pub fn get_security_metrics() -> SecurityMetrics {
    SecurityMetrics {
        invalid_inputs_rejected: INVALID_INPUTS_REJECTED.load(Ordering::Relaxed),
        conservation_violations: CONSERVATION_VIOLATIONS.load(Ordering::Relaxed),
        ffi_calls_processed: FFI_CALLS_PROCESSED.load(Ordering::Relaxed),
    }
}
```

## Continuous Security

### 1. Automated Security Checks

Set up automated security validation:

```bash
#!/bin/bash
# security-check.sh

set -e

echo "Running security checks..."

# Static analysis
cargo clippy -- -D warnings -D unsafe-code -D missing-safety-doc

# Check for known vulnerabilities
cargo audit

# Run property-based tests
cargo test property_based_tests --release

# Memory sanitizer (requires nightly)
RUSTFLAGS="-Z sanitizer=address" cargo +nightly test --target x86_64-unknown-linux-gnu

# Check for unsafe patterns
cargo geiger --charset utf8

echo "All security checks passed!"
```

### 2. Regular Security Reviews

Establish a regular security review process:

1. **Code Reviews**: All unsafe code changes require security review
2. **Dependency Updates**: Regular updates with security impact assessment  
3. **Threat Modeling**: Periodic review of attack surfaces
4. **Penetration Testing**: Regular testing of FFI boundaries
5. **Security Audits**: Annual third-party security audits

---

Following these practices will maintain the high security standards of the Atlas Manifold Layer 4 implementation and prevent introduction of security vulnerabilities during development and maintenance.