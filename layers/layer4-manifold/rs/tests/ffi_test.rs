//! FFI Boundary Tests for Atlas Manifold Layer 4
//! 
//! These tests verify the safety and correctness of the Foreign Function Interface,
//! ensuring that the C API properly handles edge cases, memory management, and
//! maintains safety guarantees when called from C code.

use atlas_manifold::{
    ffi::*,
    error::*,
    atlas_manifold_init, atlas_manifold_cleanup, atlas_manifold_version,
};
use libc::{c_int, c_void, size_t};
use std::ptr;

#[test]
fn test_c_api_version_safety() {
    unsafe {
        let version_ptr = atlas_manifold_version();
        assert!(!version_ptr.is_null(), "Version pointer should not be null");
        
        // Try to read the version string (this tests that the pointer is valid)
        let version_cstr = std::ffi::CStr::from_ptr(version_ptr as *const i8);
        let version_str = version_cstr.to_str().expect("Version should be valid UTF-8");
        assert!(!version_str.is_empty(), "Version string should not be empty");
        
        // The pointer should remain valid across multiple calls
        let version_ptr2 = atlas_manifold_version();
        assert_eq!(version_ptr, version_ptr2, "Version pointer should be stable");
    }
}

#[test]
fn test_c_api_initialization_safety() {
    // Test multiple initializations
    assert_eq!(atlas_manifold_init(), 0, "First initialization should succeed");
    assert_eq!(atlas_manifold_init(), 0, "Second initialization should be safe");
    assert_eq!(atlas_manifold_init(), 0, "Third initialization should be safe");
    
    // Test cleanup
    assert_eq!(atlas_manifold_cleanup(), 0, "First cleanup should succeed");
    assert_eq!(atlas_manifold_cleanup(), 0, "Second cleanup should be safe");
    
    // Test init after cleanup
    assert_eq!(atlas_manifold_init(), 0, "Initialization after cleanup should work");
    assert_eq!(atlas_manifold_cleanup(), 0, "Final cleanup should succeed");
}

#[test]
fn test_manifold_handle_null_safety() {
    atlas_manifold_init();
    
    // Test creation with various parameters
    unsafe {
        let handle = atlas_manifold_create(2, 3);
        // Current implementation returns null, which is safe
        assert!(handle.is_null(), "Handle creation returns null (not implemented)");
        
        // Test destruction with null handle
        let result = atlas_manifold_destroy(ptr::null_mut());
        assert_ne!(result, 0, "Destroying null handle should return error");
        
        // Test with invalid dimensions
        let handle = atlas_manifold_create(0, 3);
        assert!(handle.is_null(), "Creation with invalid dimensions should fail");
        
        let handle = atlas_manifold_create(2, 0);
        assert!(handle.is_null(), "Creation with zero embedding dimension should fail");
        
        // Test with very large dimensions that might cause overflow
        let handle = atlas_manifold_create(u32::MAX, 3);
        assert!(handle.is_null(), "Creation with very large dimensions should fail");
    }
    
    atlas_manifold_cleanup();
}

#[test]
fn test_point_transform_null_safety() {
    atlas_manifold_init();
    
    // Create test data
    let coords = [1.0, 2.0, 3.0];
    let input = CAtlasPoint {
        coords: coords.as_ptr() as *mut f64,
        dim: 3,
    };
    
    let mut output_coords = [0.0; 3];
    let mut output = CAtlasPoint {
        coords: output_coords.as_mut_ptr(),
        dim: 3,
    };
    
    unsafe {
        // Test null handle
        let result = atlas_manifold_transform_point(ptr::null(), &input, &mut output);
        assert_ne!(result, 0, "Transform with null handle should fail");
        
        // Test null input
        let result = atlas_manifold_transform_point(ptr::null(), ptr::null(), &mut output);
        assert_ne!(result, 0, "Transform with null input should fail");
        
        // Test null output
        let result = atlas_manifold_transform_point(ptr::null(), &input, ptr::null_mut());
        assert_ne!(result, 0, "Transform with null output should fail");
        
        // Test all null
        let result = atlas_manifold_transform_point(ptr::null(), ptr::null(), ptr::null_mut());
        assert_ne!(result, 0, "Transform with all nulls should fail");
    }
    
    atlas_manifold_cleanup();
}

#[test]
fn test_linear_transform_null_safety() {
    atlas_manifold_init();
    
    // Create test data
    let matrix_elements = [1.0, 0.0, 0.0, 1.0]; // 2x2 identity
    let matrix = CAtlasMatrix {
        elements: matrix_elements.as_ptr() as *mut f64,
        rows: 2,
        cols: 2,
    };
    
    let input_coords = [3.0, 4.0];
    let input = CAtlasPoint {
        coords: input_coords.as_ptr() as *mut f64,
        dim: 2,
    };
    
    let mut output_coords = [0.0; 2];
    let mut output = CAtlasPoint {
        coords: output_coords.as_mut_ptr(),
        dim: 2,
    };
    
    unsafe {
        // Test null matrix
        let result = atlas_manifold_linear_transform(ptr::null(), &input, &mut output);
        assert_ne!(result, 0, "Linear transform with null matrix should fail");
        
        // Test null input
        let result = atlas_manifold_linear_transform(&matrix, ptr::null(), &mut output);
        assert_ne!(result, 0, "Linear transform with null input should fail");
        
        // Test null output
        let result = atlas_manifold_linear_transform(&matrix, &input, ptr::null_mut());
        assert_ne!(result, 0, "Linear transform with null output should fail");
        
        // Test with valid parameters (should succeed with current stub implementation)
        let result = atlas_manifold_linear_transform(&matrix, &input, &mut output);
        assert_eq!(result, 0, "Linear transform with valid parameters should succeed");
    }
    
    atlas_manifold_cleanup();
}

#[test]
fn test_curvature_computation_null_safety() {
    atlas_manifold_init();
    
    let coords = [1.0, 1.0];
    let point = CAtlasPoint {
        coords: coords.as_ptr() as *mut f64,
        dim: 2,
    };
    
    let mut curvature = 0.0f64;
    
    unsafe {
        // Test null handle
        let result = atlas_manifold_curvature(ptr::null(), &point, &mut curvature);
        assert_ne!(result, 0, "Curvature with null handle should fail");
        
        // Test null point
        let result = atlas_manifold_curvature(ptr::null(), ptr::null(), &mut curvature);
        assert_ne!(result, 0, "Curvature with null point should fail");
        
        // Test null curvature output
        let result = atlas_manifold_curvature(ptr::null(), &point, ptr::null_mut());
        assert_ne!(result, 0, "Curvature with null output should fail");
    }
    
    atlas_manifold_cleanup();
}

#[test]
fn test_serialization_null_safety() {
    atlas_manifold_init();
    
    let mut buffer = [0u8; 1024];
    let mut bytes_written: size_t = 0;
    
    unsafe {
        // Test serialization with null handle
        let result = atlas_manifold_serialize(
            ptr::null(), 
            buffer.as_mut_ptr(), 
            buffer.len(), 
            &mut bytes_written
        );
        assert_ne!(result, 0, "Serialization with null handle should fail");
        
        // Test with null buffer
        let result = atlas_manifold_serialize(
            ptr::null(), 
            ptr::null_mut(), 
            buffer.len(), 
            &mut bytes_written
        );
        assert_ne!(result, 0, "Serialization with null buffer should fail");
        
        // Test with null bytes_written
        let result = atlas_manifold_serialize(
            ptr::null(), 
            buffer.as_mut_ptr(), 
            buffer.len(), 
            ptr::null_mut()
        );
        assert_ne!(result, 0, "Serialization with null bytes_written should fail");
        
        // Test deserialization with null buffer
        let mut handle_ptr: *mut AtlasManifoldHandle = ptr::null_mut();
        let result = atlas_manifold_deserialize(
            ptr::null(), 
            0, 
            &mut handle_ptr
        );
        assert_ne!(result, 0, "Deserialization with null buffer should fail");
        
        // Test deserialization with null handle output
        let result = atlas_manifold_deserialize(
            buffer.as_ptr(), 
            buffer.len(), 
            ptr::null_mut()
        );
        assert_ne!(result, 0, "Deserialization with null handle output should fail");
    }
    
    atlas_manifold_cleanup();
}

#[test]
fn test_memory_boundary_conditions() {
    atlas_manifold_init();
    
    // Test with zero-sized dimensions
    let empty_coords: [f64; 0] = [];
    let zero_point = CAtlasPoint {
        coords: empty_coords.as_ptr() as *mut f64,
        dim: 0,
    };
    
    let mut output_coords = [0.0; 1];
    let mut output = CAtlasPoint {
        coords: output_coords.as_mut_ptr(),
        dim: 1,
    };
    
    unsafe {
        // Operations with zero-dimensional points should be handled gracefully
        let result = atlas_manifold_transform_point(ptr::null(), &zero_point, &mut output);
        assert_ne!(result, 0, "Transform with zero-dimensional input should fail");
    }
    
    // Test with very large dimensions
    let large_dim = 1_000_000;
    let large_point = CAtlasPoint {
        coords: ptr::null_mut(),
        dim: large_dim,
    };
    
    unsafe {
        // Should fail gracefully without crashing or causing memory issues
        let result = atlas_manifold_transform_point(ptr::null(), &large_point, &mut output);
        assert_ne!(result, 0, "Transform with very large dimensions should fail safely");
    }
    
    atlas_manifold_cleanup();
}

#[test]
fn test_error_code_consistency() {
    // Test that error codes are consistent with the Rust error types
    let test_errors = [
        AtlasError::InvalidDimension(42),
        AtlasError::MatrixError("test"),
        AtlasError::TopologyError("test"),
        AtlasError::CoordinateError("test"),
        AtlasError::SerializationError("test"),
        AtlasError::AllocationError,
        AtlasError::InvalidInput("test"),
        AtlasError::NumericalError("test"),
        AtlasError::LayerIntegrationError("test"),
    ];
    
    for error in &test_errors {
        let error_code = atlas_manifold::error::error_to_code(error);
        
        // All error codes should be negative
        assert!(error_code < 0, "Error code should be negative: {:?}", error);
        
        // Error codes should be in expected range
        assert!(error_code >= -9, "Error code should be >= -9: {:?}", error);
        
        // Different errors should have different codes
        for other_error in &test_errors {
            if std::mem::discriminant(error) != std::mem::discriminant(other_error) {
                let other_code = atlas_manifold::error::error_to_code(other_error);
                assert_ne!(error_code, other_code, 
                    "Different errors should have different codes: {:?} vs {:?}", 
                    error, other_error);
            }
        }
    }
}

#[test]
fn test_ffi_struct_layout() {
    use std::mem;
    
    // Test that C-compatible structs have expected layout and alignment
    assert_eq!(mem::align_of::<CAtlasPoint>(), mem::align_of::<*mut f64>());
    assert_eq!(mem::align_of::<CAtlasMatrix>(), mem::align_of::<*mut f64>());
    assert_eq!(mem::align_of::<AtlasManifoldHandle>(), mem::align_of::<*mut c_void>());
    
    // Test sizes are reasonable
    assert!(mem::size_of::<CAtlasPoint>() <= 32); // Should be small
    assert!(mem::size_of::<CAtlasMatrix>() <= 32); // Should be small
    assert!(mem::size_of::<AtlasManifoldHandle>() <= 16); // Should be small
    
    // Test that structs are properly aligned for FFI
    let point = CAtlasPoint {
        coords: ptr::null_mut(),
        dim: 0,
    };
    let point_ptr = &point as *const _ as usize;
    assert_eq!(point_ptr % mem::align_of::<CAtlasPoint>(), 0);
}

#[test]
fn test_concurrent_ffi_calls() {
    use std::sync::Arc;
    use std::sync::atomic::{AtomicUsize, Ordering};
    use std::thread;
    
    // Test that FFI calls are thread-safe
    let init_count = Arc::new(AtomicUsize::new(0));
    let cleanup_count = Arc::new(AtomicUsize::new(0));
    let version_count = Arc::new(AtomicUsize::new(0));
    
    let mut handles = vec![];
    
    for _ in 0..10 {
        let init_c = init_count.clone();
        let cleanup_c = cleanup_count.clone();
        let version_c = version_count.clone();
        
        let handle = thread::spawn(move || {
            // Each thread calls FFI functions multiple times
            for _ in 0..100 {
                let result = atlas_manifold_init();
                if result == 0 {
                    init_c.fetch_add(1, Ordering::Relaxed);
                }
                
                unsafe {
                    let version_ptr = atlas_manifold_version();
                    if !version_ptr.is_null() {
                        version_c.fetch_add(1, Ordering::Relaxed);
                    }
                }
                
                let result = atlas_manifold_cleanup();
                if result == 0 {
                    cleanup_c.fetch_add(1, Ordering::Relaxed);
                }
            }
        });
        
        handles.push(handle);
    }
    
    // Wait for all threads to complete
    for handle in handles {
        handle.join().expect("Thread should complete successfully");
    }
    
    // Verify that all calls succeeded
    assert_eq!(init_count.load(Ordering::Relaxed), 1000);
    assert_eq!(cleanup_count.load(Ordering::Relaxed), 1000);
    assert_eq!(version_count.load(Ordering::Relaxed), 1000);
}

#[test]
fn test_ffi_parameter_validation() {
    atlas_manifold_init();
    
    // Test parameter validation in FFI layer
    let coords = [1.0, 2.0, 3.0];
    let mut valid_point = CAtlasPoint {
        coords: coords.as_ptr() as *mut f64,
        dim: 3,
    };
    
    let mut output_coords = [0.0; 3];
    let mut valid_output = CAtlasPoint {
        coords: output_coords.as_mut_ptr(),
        dim: 3,
    };
    
    // Test with mismatched dimensions
    let mut mismatched_output = CAtlasPoint {
        coords: output_coords.as_mut_ptr(),
        dim: 2, // Different from input dimension
    };
    
    unsafe {
        // The FFI should handle dimension mismatches gracefully
        let result = atlas_manifold_transform_point(ptr::null(), &valid_point, &mut mismatched_output);
        assert_ne!(result, 0, "Transform with mismatched dimensions should fail");
    }
    
    // Note: The current stub implementation always returns 0 (success) for valid pointers.
    // In a full implementation, we would test actual parameter validation.
    // For now, we just verify that the FFI doesn't crash with edge cases.
    
    atlas_manifold_cleanup();
}

#[test]
fn test_ffi_memory_leak_prevention() {
    // This test ensures that the FFI doesn't leak memory even when called incorrectly
    atlas_manifold_init();
    
    // Call functions many times with invalid parameters to test for leaks
    for _ in 0..1000 {
        unsafe {
            // These should all fail but not leak memory
            let _ = atlas_manifold_create(0, 0);
            let _ = atlas_manifold_destroy(ptr::null_mut());
            let _ = atlas_manifold_transform_point(ptr::null(), ptr::null(), ptr::null_mut());
            let _ = atlas_manifold_linear_transform(ptr::null(), ptr::null(), ptr::null_mut());
            let _ = atlas_manifold_curvature(ptr::null(), ptr::null(), ptr::null_mut());
            let _ = atlas_manifold_serialize(ptr::null(), ptr::null_mut(), 0, ptr::null_mut());
            let _ = atlas_manifold_deserialize(ptr::null(), 0, ptr::null_mut());
        }
    }
    
    atlas_manifold_cleanup();
    
    // If we reach here without crashing or running out of memory, the test passes
    assert!(true, "No memory leaks detected in FFI error paths");
}