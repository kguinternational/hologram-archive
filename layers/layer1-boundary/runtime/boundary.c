/* boundary.c - Atlas-12288 Layer 1 Boundary Runtime Implementation
 * (c) 2024-2025 UOR Foundation - MIT License
 * 
 * Complete C runtime for Layer 1 (Boundary Layer) providing:
 * - Coordinate system operations (48×256)
 * - Page management functions  
 * - Klein orbit operations
 * - Φ isomorphism and morphisms
 */

#include "../include/atlas-boundary.h"
#include <string.h>
#include <stdlib.h>

// Forward declarations for LLVM functions (optional - could use when available)
// For now, we'll implement everything in pure C for reliability

// =============================================================================
// Constants
// =============================================================================

#define ATLAS_PAGE_SIZE 256
#define ATLAS_NUM_PAGES 48
#define ATLAS_TOTAL_SIZE (ATLAS_NUM_PAGES * ATLAS_PAGE_SIZE) // 12,288

// =============================================================================
// Coordinate Operations
// =============================================================================

atlas_boundary_t atlas_boundary_encode(uint16_t page, uint8_t byte) {
    // Validate input bounds
    if (page >= ATLAS_NUM_PAGES) {
        return UINT32_MAX; // Error marker
    }
    
    // Standard encoding: boundary = page * 256 + byte
    return (uint32_t)page * ATLAS_PAGE_SIZE + (uint32_t)byte;
}

bool atlas_boundary_decode(atlas_boundary_t boundary, atlas_coordinate_t* coord) {
    if (!coord || boundary >= ATLAS_TOTAL_SIZE) {
        return false;
    }
    
    coord->page = (uint16_t)(boundary / ATLAS_PAGE_SIZE);
    coord->byte = (uint8_t)(boundary % ATLAS_PAGE_SIZE);
    
    return true;
}

bool atlas_coordinate_is_valid(uint16_t page, uint8_t byte) {
    // Page must be in valid range [0, 47], byte is always valid [0, 255]
    (void)byte; // Byte is always valid for uint8_t
    return page < ATLAS_NUM_PAGES;
}

bool atlas_boundary_is_valid(atlas_boundary_t boundary) {
    return boundary < ATLAS_TOTAL_SIZE;
}

// =============================================================================
// Page Management
// =============================================================================

atlas_page_info_t atlas_page_get_info(const void* structure, uint16_t page_index) {
    atlas_page_info_t info = {0};
    
    if (!structure || page_index >= ATLAS_NUM_PAGES) {
        info.index = page_index;
        info.is_valid = false;
        info.checksum = 0;
        return info;
    }
    
    info.index = page_index;
    info.is_valid = true;
    
    // Calculate page pointer
    const uint8_t* page_ptr = (const uint8_t*)structure + (page_index * ATLAS_PAGE_SIZE);
    info.checksum = atlas_page_checksum(page_ptr);
    
    return info;
}

void atlas_page_copy(void* dest_page, const void* src_page) {
    if (!dest_page || !src_page) {
        return;
    }
    
    memcpy(dest_page, src_page, ATLAS_PAGE_SIZE);
}

bool atlas_page_equal(const void* page1, const void* page2) {
    if (!page1 || !page2) {
        return false;
    }
    
    return memcmp(page1, page2, ATLAS_PAGE_SIZE) == 0;
}

uint32_t atlas_page_checksum(const void* page_ptr) {
    if (!page_ptr) {
        return 0;
    }
    
    const uint8_t* bytes = (const uint8_t*)page_ptr;
    uint32_t checksum = 0;
    
    // Simple additive checksum with rotation
    for (int i = 0; i < ATLAS_PAGE_SIZE; i++) {
        checksum = ((checksum << 1) | (checksum >> 31)) + bytes[i];
    }
    
    return checksum;
}

// =============================================================================
// Klein Orbit Operations
// =============================================================================

uint8_t atlas_klein_get_orbit_id(atlas_boundary_t coord) {
    if (!atlas_boundary_is_valid(coord)) {
        return 0; // Default to orbit 0 for invalid coordinates
    }
    
    // Extract page and byte components
    uint16_t page = coord / ATLAS_PAGE_SIZE;
    uint8_t byte = coord % ATLAS_PAGE_SIZE;
    
    // Compute orbit ID using Klein group structure (0-15 for 4x4 Klein structure)
    uint8_t page_mod = page % 4;
    uint8_t byte_mod = byte % 4;
    
    return (page_mod << 2) | byte_mod;
}

bool atlas_klein_is_privileged_orbit(atlas_boundary_t coord) {
    // Privileged orbits are {0, 1, 48, 49}
    return (coord == 0 || coord == 1 || coord == 48 || coord == 49);
}

// =============================================================================
// Distance and Navigation
// =============================================================================

uint32_t atlas_boundary_distance(atlas_boundary_t from, atlas_boundary_t to) {
    if (!atlas_boundary_is_valid(from) || !atlas_boundary_is_valid(to)) {
        return UINT32_MAX;
    }
    
    // Linear distance with proper handling of wraparound
    if (to >= from) {
        return to - from;
    } else {
        return (ATLAS_TOTAL_SIZE - from) + to;
    }
}

atlas_boundary_t atlas_boundary_advance(atlas_boundary_t boundary, uint32_t offset) {
    if (!atlas_boundary_is_valid(boundary)) {
        return boundary;
    }
    
    // Advance with wraparound
    return (boundary + offset) % ATLAS_TOTAL_SIZE;
}

// =============================================================================
// Internal Helper Functions
// =============================================================================

// Helper functions can be added here as needed