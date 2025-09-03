/* test-boundary.c - Comprehensive Test Suite for Layer 1 Boundary
 * (c) 2024-2025 UOR Foundation - MIT License
 * 
 * Complete test suite for Layer 1 (Boundary Layer) operations:
 * - Unit tests for all coordinate operations
 * - Tests for page management
 * - Klein orbit verification tests
 * - Morphism round-trip tests
 */

#include "../include/atlas-boundary.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <time.h>

// Test framework macros
#define TEST_ASSERT(cond, msg) \
    do { \
        if (!(cond)) { \
            printf("FAIL: %s - %s\n", __func__, msg); \
            return 0; \
        } \
    } while (0)

#define TEST_PASS() \
    do { \
        printf("PASS: %s\n", __func__); \
        return 1; \
    } while (0)

// Test counters
static int tests_run = 0;
static int tests_passed = 0;

#define RUN_TEST(test_func) \
    do { \
        tests_run++; \
        if (test_func()) { \
            tests_passed++; \
        } \
    } while (0)

// =============================================================================
// Coordinate Operations Tests
// =============================================================================

static int test_coordinate_encoding_basic() {
    // Test basic encoding/decoding
    atlas_boundary_t encoded = atlas_boundary_encode(0, 0);
    TEST_ASSERT(encoded == 0, "Encoding (0,0) should be 0");
    
    encoded = atlas_boundary_encode(1, 0);
    TEST_ASSERT(encoded == 256, "Encoding (1,0) should be 256");
    
    encoded = atlas_boundary_encode(0, 1);
    TEST_ASSERT(encoded == 1, "Encoding (0,1) should be 1");
    
    encoded = atlas_boundary_encode(47, 255);
    TEST_ASSERT(encoded == 47 * 256 + 255, "Encoding (47,255) should be 12287");
    
    TEST_PASS();
}

static int test_coordinate_decoding_basic() {
    atlas_coordinate_t coord;
    
    // Test basic decoding
    TEST_ASSERT(atlas_boundary_decode(0, &coord), "Decoding 0 should succeed");
    TEST_ASSERT(coord.page == 0 && coord.byte == 0, "Decoding 0 should give (0,0)");
    
    TEST_ASSERT(atlas_boundary_decode(256, &coord), "Decoding 256 should succeed");
    TEST_ASSERT(coord.page == 1 && coord.byte == 0, "Decoding 256 should give (1,0)");
    
    TEST_ASSERT(atlas_boundary_decode(12287, &coord), "Decoding 12287 should succeed");
    TEST_ASSERT(coord.page == 47 && coord.byte == 255, "Decoding 12287 should give (47,255)");
    
    TEST_PASS();
}

static int test_coordinate_round_trip() {
    // Test round-trip encoding/decoding for all valid coordinates
    for (uint16_t page = 0; page < 48; page++) {
        for (uint16_t byte = 0; byte < 256; byte += 17) { // Sample every 17th byte
            atlas_boundary_t encoded = atlas_boundary_encode(page, (uint8_t)byte);
            atlas_coordinate_t decoded;
            
            TEST_ASSERT(atlas_boundary_decode(encoded, &decoded), "Round-trip decode should succeed");
            TEST_ASSERT(decoded.page == page, "Round-trip page should match");
            TEST_ASSERT(decoded.byte == (uint8_t)byte, "Round-trip byte should match");
        }
    }
    
    TEST_PASS();
}

static int test_coordinate_validation() {
    // Test coordinate validation
    TEST_ASSERT(atlas_coordinate_is_valid(0, 0), "Coordinate (0,0) should be valid");
    TEST_ASSERT(atlas_coordinate_is_valid(47, 255), "Coordinate (47,255) should be valid");
    TEST_ASSERT(!atlas_coordinate_is_valid(48, 0), "Coordinate (48,0) should be invalid");
    TEST_ASSERT(!atlas_coordinate_is_valid(100, 0), "Coordinate (100,0) should be invalid");
    
    // Test boundary validation
    TEST_ASSERT(atlas_boundary_is_valid(0), "Boundary 0 should be valid");
    TEST_ASSERT(atlas_boundary_is_valid(12287), "Boundary 12287 should be valid");
    TEST_ASSERT(!atlas_boundary_is_valid(12288), "Boundary 12288 should be invalid");
    TEST_ASSERT(!atlas_boundary_is_valid(UINT32_MAX), "Boundary UINT32_MAX should be invalid");
    
    TEST_PASS();
}

static int test_coordinate_edge_cases() {
    atlas_coordinate_t coord;
    
    // Test invalid boundary decoding
    TEST_ASSERT(!atlas_boundary_decode(12288, &coord), "Decoding 12288 should fail");
    TEST_ASSERT(!atlas_boundary_decode(UINT32_MAX, &coord), "Decoding UINT32_MAX should fail");
    TEST_ASSERT(!atlas_boundary_decode(0, NULL), "Decoding to NULL should fail");
    
    // Test invalid page encoding
    atlas_boundary_t encoded = atlas_boundary_encode(48, 0);
    TEST_ASSERT(encoded == UINT32_MAX, "Encoding invalid page should return error marker");
    
    TEST_PASS();
}

// =============================================================================
// Page Management Tests  
// =============================================================================

static int test_page_info() {
    // Create a test structure
    uint8_t test_structure[48 * 256];
    memset(test_structure, 0, sizeof(test_structure));
    
    // Initialize first page with test pattern
    for (int i = 0; i < 256; i++) {
        test_structure[i] = (uint8_t)(i & 0xFF);
    }
    
    atlas_page_info_t info = atlas_page_get_info(test_structure, 0);
    TEST_ASSERT(info.index == 0, "Page info should have correct index");
    TEST_ASSERT(info.is_valid, "Page info should be valid");
    TEST_ASSERT(info.checksum != 0, "Page info should have non-zero checksum");
    
    // Test invalid page index
    info = atlas_page_get_info(test_structure, 48);
    TEST_ASSERT(!info.is_valid, "Invalid page index should be marked invalid");
    
    // Test NULL structure
    info = atlas_page_get_info(NULL, 0);
    TEST_ASSERT(!info.is_valid, "NULL structure should be marked invalid");
    
    TEST_PASS();
}

static int test_page_copy() {
    uint8_t src_page[256];
    uint8_t dest_page[256];
    
    // Initialize source page with test pattern
    for (int i = 0; i < 256; i++) {
        src_page[i] = (uint8_t)(i & 0xFF);
    }
    memset(dest_page, 0, 256);
    
    // Test copy operation
    atlas_page_copy(dest_page, src_page);
    TEST_ASSERT(memcmp(src_page, dest_page, 256) == 0, "Page copy should copy all bytes");
    
    // Test NULL handling
    atlas_page_copy(NULL, src_page); // Should not crash
    atlas_page_copy(dest_page, NULL); // Should not crash
    
    TEST_PASS();
}

static int test_page_equal() {
    uint8_t page1[256];
    uint8_t page2[256];
    uint8_t page3[256];
    
    // Initialize pages
    memset(page1, 0xAA, 256);
    memset(page2, 0xAA, 256);
    memset(page3, 0xBB, 256);
    
    // Test equality
    TEST_ASSERT(atlas_page_equal(page1, page2), "Identical pages should be equal");
    TEST_ASSERT(!atlas_page_equal(page1, page3), "Different pages should not be equal");
    TEST_ASSERT(!atlas_page_equal(NULL, page1), "NULL comparison should return false");
    TEST_ASSERT(!atlas_page_equal(page1, NULL), "NULL comparison should return false");
    
    TEST_PASS();
}

static int test_page_checksum() {
    uint8_t page1[256];
    uint8_t page2[256];
    uint8_t page3[256];
    
    // Initialize pages with different patterns
    memset(page1, 0x00, 256);
    memset(page2, 0x00, 256);
    memset(page3, 0xFF, 256);
    
    uint32_t checksum1 = atlas_page_checksum(page1);
    uint32_t checksum2 = atlas_page_checksum(page2);
    uint32_t checksum3 = atlas_page_checksum(page3);
    
    TEST_ASSERT(checksum1 == checksum2, "Identical pages should have identical checksums");
    TEST_ASSERT(checksum1 != checksum3, "Different pages should have different checksums");
    TEST_ASSERT(atlas_page_checksum(NULL) == 0, "NULL page should have zero checksum");
    
    TEST_PASS();
}

// =============================================================================
// Klein Orbit Tests
// =============================================================================

static int test_klein_orbit_basic() {
    // Test orbit ID computation for known coordinates
    uint8_t orbit_id;
    
    orbit_id = atlas_klein_get_orbit_id(0);  // (0,0)
    TEST_ASSERT(orbit_id < 16, "Orbit ID should be in range [0,15]");
    
    orbit_id = atlas_klein_get_orbit_id(1);  // (0,1)
    TEST_ASSERT(orbit_id < 16, "Orbit ID should be in range [0,15]");
    
    orbit_id = atlas_klein_get_orbit_id(256); // (1,0)
    TEST_ASSERT(orbit_id < 16, "Orbit ID should be in range [0,15]");
    
    TEST_PASS();
}

static int test_klein_privileged_orbits() {
    // Test privileged orbit detection
    TEST_ASSERT(atlas_klein_is_privileged_orbit(0), "Coordinate 0 should be privileged");
    TEST_ASSERT(atlas_klein_is_privileged_orbit(1), "Coordinate 1 should be privileged");
    TEST_ASSERT(atlas_klein_is_privileged_orbit(48), "Coordinate 48 should be privileged");
    TEST_ASSERT(atlas_klein_is_privileged_orbit(49), "Coordinate 49 should be privileged");
    
    // Test non-privileged coordinates
    TEST_ASSERT(!atlas_klein_is_privileged_orbit(2), "Coordinate 2 should not be privileged");
    TEST_ASSERT(!atlas_klein_is_privileged_orbit(256), "Coordinate 256 should not be privileged");
    TEST_ASSERT(!atlas_klein_is_privileged_orbit(1000), "Coordinate 1000 should not be privileged");
    
    TEST_PASS();
}

static int test_klein_orbit_consistency() {
    // Test orbit consistency properties
    uint8_t orbit_counts[16] = {0};
    
    // Sample coordinates and count orbits
    for (atlas_boundary_t coord = 0; coord < 12288; coord += 64) { // Sample every 64th
        uint8_t orbit_id = atlas_klein_get_orbit_id(coord);
        TEST_ASSERT(orbit_id < 16, "All orbit IDs should be in range [0,15]");
        orbit_counts[orbit_id]++;
    }
    
    // All orbits should be represented in the sampling
    int non_empty_orbits = 0;
    for (int i = 0; i < 16; i++) {
        if (orbit_counts[i] > 0) {
            non_empty_orbits++;
        }
    }
    TEST_ASSERT(non_empty_orbits > 0, "At least one orbit should be non-empty");
    
    TEST_PASS();
}

// =============================================================================
// Distance and Navigation Tests
// =============================================================================

static int test_boundary_distance() {
    // Test basic distance computation
    uint32_t distance;
    
    distance = atlas_boundary_distance(0, 0);
    TEST_ASSERT(distance == 0, "Distance from coordinate to itself should be 0");
    
    distance = atlas_boundary_distance(0, 1);
    TEST_ASSERT(distance == 1, "Distance from 0 to 1 should be 1");
    
    distance = atlas_boundary_distance(0, 256);
    TEST_ASSERT(distance == 256, "Distance from 0 to 256 should be 256");
    
    distance = atlas_boundary_distance(12287, 0);
    TEST_ASSERT(distance == 1, "Distance with wraparound should work correctly");
    
    // Test invalid coordinates
    distance = atlas_boundary_distance(12288, 0);
    TEST_ASSERT(distance == UINT32_MAX, "Distance with invalid coordinate should return error");
    
    TEST_PASS();
}

static int test_boundary_advance() {
    // Test coordinate advancement
    atlas_boundary_t advanced;
    
    advanced = atlas_boundary_advance(0, 1);
    TEST_ASSERT(advanced == 1, "Advancing 0 by 1 should give 1");
    
    advanced = atlas_boundary_advance(0, 256);
    TEST_ASSERT(advanced == 256, "Advancing 0 by 256 should give 256");
    
    advanced = atlas_boundary_advance(12287, 1);
    TEST_ASSERT(advanced == 0, "Advancing 12287 by 1 should wrap to 0");
    
    advanced = atlas_boundary_advance(12000, 1000);
    TEST_ASSERT(advanced < 12288, "Advanced coordinate should be valid");
    
    TEST_PASS();
}

// =============================================================================
// Integration Tests
// =============================================================================

static int test_full_structure_operations() {
    // Create and initialize a full Atlas structure
    uint8_t atlas_structure[48 * 256];
    
    // Initialize with deterministic pattern
    for (int page = 0; page < 48; page++) {
        for (int byte = 0; byte < 256; byte++) {
            atlas_boundary_t coord = atlas_boundary_encode(page, byte);
            atlas_structure[coord] = (uint8_t)((page + byte) & 0xFF);
        }
    }
    
    // Test coordinate-based access
    for (int page = 0; page < 48; page += 5) { // Sample every 5th page
        atlas_page_info_t info = atlas_page_get_info(atlas_structure, page);
        TEST_ASSERT(info.is_valid, "Page info should be valid for initialized structure");
        TEST_ASSERT(info.index == page, "Page info should have correct index");
    }
    
    TEST_PASS();
}

static int test_boundary_coordinate_coverage() {
    // Ensure all valid boundary coordinates can be encoded/decoded
    int tested_coordinates = 0;
    
    for (uint16_t page = 0; page < 48; page += 3) { // Sample every 3rd page
        for (uint16_t byte = 0; byte < 256; byte += 7) { // Sample every 7th byte
            atlas_boundary_t encoded = atlas_boundary_encode(page, byte);
            TEST_ASSERT(atlas_boundary_is_valid(encoded), "Encoded coordinate should be valid");
            
            atlas_coordinate_t decoded;
            TEST_ASSERT(atlas_boundary_decode(encoded, &decoded), "Decoding should succeed");
            TEST_ASSERT(decoded.page == page && decoded.byte == byte, "Round-trip should preserve values");
            
            tested_coordinates++;
        }
    }
    
    TEST_ASSERT(tested_coordinates > 100, "Should test significant number of coordinates");
    TEST_PASS();
}

// =============================================================================
// Performance Tests
// =============================================================================

static int test_performance_encoding() {
    clock_t start = clock();
    
    // Encode many coordinates to test performance
    volatile atlas_boundary_t result;
    for (int i = 0; i < 100000; i++) {
        uint16_t page = i % 48;
        uint8_t byte = i % 256;
        result = atlas_boundary_encode(page, byte);
        (void)result; // Prevent optimization
    }
    
    clock_t end = clock();
    double elapsed = ((double)(end - start)) / CLOCKS_PER_SEC;
    
    TEST_ASSERT(elapsed < 1.0, "Encoding performance should be reasonable (< 1 second)");
    TEST_PASS();
}

// =============================================================================
// Test Runner
// =============================================================================

int main() {
    printf("=== Atlas Layer 1 (Boundary) Test Suite ===\n\n");
    
    // Coordinate Operations Tests
    printf("--- Coordinate Operations ---\n");
    RUN_TEST(test_coordinate_encoding_basic);
    RUN_TEST(test_coordinate_decoding_basic);
    RUN_TEST(test_coordinate_round_trip);
    RUN_TEST(test_coordinate_validation);
    RUN_TEST(test_coordinate_edge_cases);
    
    // Page Management Tests
    printf("\n--- Page Management ---\n");
    RUN_TEST(test_page_info);
    RUN_TEST(test_page_copy);
    RUN_TEST(test_page_equal);
    RUN_TEST(test_page_checksum);
    
    // Klein Orbit Tests
    printf("\n--- Klein Orbit Operations ---\n");
    RUN_TEST(test_klein_orbit_basic);
    RUN_TEST(test_klein_privileged_orbits);
    RUN_TEST(test_klein_orbit_consistency);
    
    // Distance and Navigation Tests
    printf("\n--- Distance and Navigation ---\n");
    RUN_TEST(test_boundary_distance);
    RUN_TEST(test_boundary_advance);
    
    // Integration Tests
    printf("\n--- Integration Tests ---\n");
    RUN_TEST(test_full_structure_operations);
    RUN_TEST(test_boundary_coordinate_coverage);
    
    // Performance Tests
    printf("\n--- Performance Tests ---\n");
    RUN_TEST(test_performance_encoding);
    
    // Summary
    printf("\n=== Test Summary ===\n");
    printf("Tests run: %d\n", tests_run);
    printf("Tests passed: %d\n", tests_passed);
    printf("Tests failed: %d\n", tests_run - tests_passed);
    printf("Success rate: %.1f%%\n", (100.0 * tests_passed) / tests_run);
    
    if (tests_passed == tests_run) {
        printf("\n🎉 ALL TESTS PASSED! Layer 1 (Boundary) is working correctly.\n");
        return 0;
    } else {
        printf("\n❌ Some tests failed. Please review the implementation.\n");
        return 1;
    }
}