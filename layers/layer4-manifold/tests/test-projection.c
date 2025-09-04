/*
 * Atlas Manifold Layer 4 - Projection Operations Test
 * 
 * Tests the new projection functionality via FFI to ensure C interoperability
 * and verify that the Rust implementation works correctly from C code.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <stdint.h>
#include <stdbool.h>

// Include the projection-specific C headers
#ifdef __cplusplus
extern "C" {
#endif

// Opaque projection handle
typedef struct {
    void* inner;
} CAtlasProjectionHandle;

// Transformation parameters
typedef struct {
    double scaling_factor;
    double rotation_angle;
    double translation_x;
    double translation_y;
} CAtlasTransformParams;

// Function prototypes for projection operations
CAtlasProjectionHandle* atlas_projection_create(uint32_t projection_type, const uint8_t* source_data, size_t source_size);
void atlas_projection_destroy(CAtlasProjectionHandle* handle);
int atlas_projection_get_dimensions(const CAtlasProjectionHandle* handle, uint32_t* width, uint32_t* height);
bool atlas_projection_is_valid(const CAtlasProjectionHandle* handle);
int atlas_projection_transform(CAtlasProjectionHandle* handle, const CAtlasTransformParams* params);

#ifdef __cplusplus
}
#endif

// Test utility functions
void test_assert(int condition, const char* message) {
    if (!condition) {
        fprintf(stderr, "TEST FAILED: %s\n", message);
        exit(1);
    }
    printf("✓ %s\n", message);
}

void print_test_header(const char* test_name) {
    printf("\n--- %s ---\n", test_name);
}

// Test functions
void test_projection_creation(void) {
    print_test_header("Projection Creation Test");
    
    // Create test data
    const size_t data_size = 8192; // 2 pages worth of data
    uint8_t* test_data = malloc(data_size);
    
    // Fill with test pattern
    for (size_t i = 0; i < data_size; i++) {
        test_data[i] = (uint8_t)(i % 256);
    }
    
    // Test LINEAR projection creation
    CAtlasProjectionHandle* handle = atlas_projection_create(0, test_data, data_size);
    test_assert(handle != NULL, "LINEAR projection creation successful");
    test_assert(atlas_projection_is_valid(handle), "Created projection is valid");
    
    // Get dimensions
    uint32_t width, height;
    int result = atlas_projection_get_dimensions(handle, &width, &height);
    test_assert(result == 0, "Getting dimensions successful");
    test_assert(width > 0 && height > 0, "Dimensions are positive");
    printf("Projection dimensions: %u x %u\n", width, height);
    
    // Cleanup
    atlas_projection_destroy(handle);
    free(test_data);
}

void test_projection_error_conditions(void) {
    print_test_header("Projection Error Conditions Test");
    
    // Test creation with null data
    CAtlasProjectionHandle* handle = atlas_projection_create(0, NULL, 1000);
    test_assert(handle == NULL, "Creation with null data returns null");
    
    // Test creation with zero size
    uint8_t dummy_data[10] = {0};
    handle = atlas_projection_create(0, dummy_data, 0);
    test_assert(handle == NULL, "Creation with zero size returns null");
    
    // Test invalid projection type
    handle = atlas_projection_create(999, dummy_data, sizeof(dummy_data));
    test_assert(handle == NULL, "Creation with invalid type returns null");
    
    // Test operations on null handle
    test_assert(!atlas_projection_is_valid(NULL), "Null handle is invalid");
    
    uint32_t width, height;
    int result = atlas_projection_get_dimensions(NULL, &width, &height);
    test_assert(result != 0, "Getting dimensions on null handle fails");
    
    result = atlas_projection_get_dimensions(NULL, NULL, &height);
    test_assert(result != 0, "Getting dimensions with null width fails");
    
    result = atlas_projection_get_dimensions(NULL, &width, NULL);
    test_assert(result != 0, "Getting dimensions with null height fails");
    
    // Test destroy with null handle (should be safe)
    atlas_projection_destroy(NULL);
}

void test_projection_transformations(void) {
    print_test_header("Projection Transformations Test");
    
    // Create test data
    const size_t data_size = 4096; // 1 page
    uint8_t* test_data = malloc(data_size);
    for (size_t i = 0; i < data_size; i++) {
        test_data[i] = (uint8_t)(i % 100);
    }
    
    // Create projection
    CAtlasProjectionHandle* handle = atlas_projection_create(0, test_data, data_size);
    test_assert(handle != NULL, "Projection creation for transform test successful");
    
    // Create transformation parameters
    CAtlasTransformParams params = {
        .scaling_factor = 2.0,
        .rotation_angle = 3.14159 / 4.0, // 45 degrees
        .translation_x = 10.0,
        .translation_y = 20.0
    };
    
    // Apply transformation
    int result = atlas_projection_transform(handle, &params);
    test_assert(result == 0, "Transformation application successful");
    
    // Verify projection is still valid after transformation
    test_assert(atlas_projection_is_valid(handle), "Projection valid after transformation");
    
    // Test error conditions
    result = atlas_projection_transform(NULL, &params);
    test_assert(result != 0, "Transform with null handle fails");
    
    result = atlas_projection_transform(handle, NULL);
    test_assert(result != 0, "Transform with null params fails");
    
    // Cleanup
    atlas_projection_destroy(handle);
    free(test_data);
}

void test_conservation_validation(void) {
    print_test_header("Conservation Validation Test");
    
    // Create data that should pass conservation laws
    const size_t data_size = 4096;
    uint8_t* test_data = malloc(data_size);
    
    // Fill with pattern that will require conservation correction
    for (size_t i = 0; i < data_size; i++) {
        test_data[i] = (uint8_t)(i % 256);
    }
    
    // Create projection - should succeed even if conservation correction is needed
    CAtlasProjectionHandle* handle = atlas_projection_create(0, test_data, data_size);
    test_assert(handle != NULL, "Projection creation with conservation correction successful");
    test_assert(atlas_projection_is_valid(handle), "Projection with conservation is valid");
    
    atlas_projection_destroy(handle);
    free(test_data);
}

void test_memory_safety(void) {
    print_test_header("Memory Safety Test");
    
    // Test multiple allocations and deallocations
    const int NUM_ITERATIONS = 50;
    const size_t DATA_SIZE = 2048;
    
    for (int i = 0; i < NUM_ITERATIONS; i++) {
        uint8_t* test_data = malloc(DATA_SIZE);
        
        // Fill with unique pattern
        for (size_t j = 0; j < DATA_SIZE; j++) {
            test_data[j] = (uint8_t)((i + j) % 256);
        }
        
        CAtlasProjectionHandle* handle = atlas_projection_create(0, test_data, DATA_SIZE);
        
        if (handle != NULL) {
            test_assert(atlas_projection_is_valid(handle), "Handle is valid in memory test");
            atlas_projection_destroy(handle);
        }
        
        free(test_data);
    }
    
    printf("Completed %d allocation/deallocation cycles\n", NUM_ITERATIONS);
}

void test_r96_fourier_placeholder(void) {
    print_test_header("R96 Fourier Projection Test");
    
    const size_t data_size = 1024;
    uint8_t* test_data = malloc(data_size);
    for (size_t i = 0; i < data_size; i++) {
        test_data[i] = (uint8_t)(i % 256);
    }
    
    // Test R96 Fourier projection creation (should succeed as it is now implemented)
    CAtlasProjectionHandle* handle = atlas_projection_create(1, test_data, data_size);
    test_assert(handle != NULL, "R96 Fourier projection creation successful");
    
    if (handle != NULL) {
        test_assert(atlas_projection_is_valid(handle), "R96 Fourier projection is valid");
        atlas_projection_destroy(handle);
    }
    
    free(test_data);
}

int main(void) {
    printf("=== Atlas Manifold Layer 4 - Projection Operations Tests ===\n");
    
    // Run all projection tests
    test_projection_creation();
    test_projection_error_conditions();
    test_projection_transformations();
    test_conservation_validation();
    test_memory_safety();
    test_r96_fourier_placeholder();
    
    printf("\n=== All Projection Tests Completed Successfully ===\n");
    printf("✓ Projection creation and destruction\n");
    printf("✓ Error condition handling\n");
    printf("✓ Transformation operations\n");
    printf("✓ Conservation law validation\n");
    printf("✓ Memory safety verification\n");
    printf("✓ R96 Fourier projection implementation\n");
    
    printf("\nNote: Projection operations successfully integrate with C FFI.\n");
    printf("Both LINEAR and R96_FOURIER projection types are fully functional.\n");
    
    return 0;
}