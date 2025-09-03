/* atlas-boundary.h - Atlas-12288 Layer 1 Boundary Interface
 * (c) 2024-2025 UOR Foundation - MIT License
 * 
 * Public API for Atlas-12288 Layer 1 (Boundary Layer) providing:
 * - Coordinate system operations (48×256)
 * - Page management functions
 * - Klein orbit operations
 * - Φ isomorphism and morphisms
 */

#ifndef ATLAS_BOUNDARY_H
#define ATLAS_BOUNDARY_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

// =============================================================================
// Version Information
// =============================================================================

#define ATLAS_BOUNDARY_VERSION_MAJOR 1
#define ATLAS_BOUNDARY_VERSION_MINOR 0
#define ATLAS_BOUNDARY_VERSION_PATCH 0

// =============================================================================
// Type Definitions
// =============================================================================

/* Coordinate pair (page_index, byte_offset) */
typedef struct {
    uint16_t page;    /* Page index [0, 47] */
    uint8_t  byte;    /* Byte offset [0, 255] */
} atlas_coordinate_t;

/* Boundary encoded coordinate */
typedef uint32_t atlas_boundary_t;

/* Page information */
typedef struct {
    uint16_t index;      /* Page index */
    bool     is_valid;   /* Page validation status */
    uint32_t checksum;   /* Page checksum */
} atlas_page_info_t;

// =============================================================================
// Coordinate Operations
// =============================================================================

/**
 * Encode coordinate pair into boundary representation.
 * 
 * @param page Page index [0, 47]
 * @param byte Byte offset [0, 255]
 * @return Encoded boundary coordinate, or UINT32_MAX on error
 */
atlas_boundary_t atlas_boundary_encode(uint16_t page, uint8_t byte);

/**
 * Decode boundary coordinate into component parts.
 * 
 * @param boundary Encoded boundary coordinate
 * @param coord Output coordinate structure
 * @return true on success, false on error
 */
bool atlas_boundary_decode(atlas_boundary_t boundary, atlas_coordinate_t* coord);

/**
 * Check if coordinate is within valid bounds.
 * 
 * @param page Page index
 * @param byte Byte offset  
 * @return true if valid, false otherwise
 */
bool atlas_coordinate_is_valid(uint16_t page, uint8_t byte);

/**
 * Check if boundary coordinate is within valid bounds.
 * 
 * @param boundary Encoded boundary coordinate
 * @return true if valid, false otherwise
 */
bool atlas_boundary_is_valid(atlas_boundary_t boundary);

// =============================================================================
// Page Management
// =============================================================================

/**
 * Get page information for specified page index.
 * 
 * @param structure Pointer to Atlas structure
 * @param page_index Page index [0, 47]
 * @return Page information structure
 */
atlas_page_info_t atlas_page_get_info(const void* structure, uint16_t page_index);

/**
 * Copy entire page from source to destination.
 * 
 * @param dest_page Destination page pointer
 * @param src_page Source page pointer
 */
void atlas_page_copy(void* dest_page, const void* src_page);

/**
 * Compare two pages for equality.
 * 
 * @param page1 First page pointer
 * @param page2 Second page pointer
 * @return true if pages are identical, false otherwise
 */
bool atlas_page_equal(const void* page1, const void* page2);

/**
 * Compute checksum of page contents.
 * 
 * @param page_ptr Page pointer
 * @return Page checksum
 */
uint32_t atlas_page_checksum(const void* page_ptr);

// =============================================================================
// Klein Orbit Operations
// =============================================================================

/**
 * Get Klein orbit ID for coordinate.
 * 
 * @param coord Boundary coordinate
 * @return Klein orbit ID [0, 15]
 */
uint8_t atlas_klein_get_orbit_id(atlas_boundary_t coord);

/**
 * Check if coordinate is in privileged Klein orbit.
 * 
 * @param coord Boundary coordinate
 * @return true if in privileged orbit {0, 1, 48, 49}
 */
bool atlas_klein_is_privileged_orbit(atlas_boundary_t coord);

// =============================================================================
// Distance and Navigation
// =============================================================================

/**
 * Compute linear distance between two boundary coordinates.
 * 
 * @param from Source coordinate
 * @param to Destination coordinate
 * @return Distance, or UINT32_MAX on error
 */
uint32_t atlas_boundary_distance(atlas_boundary_t from, atlas_boundary_t to);

/**
 * Advance boundary coordinate by offset with wraparound.
 * 
 * @param boundary Source coordinate
 * @param offset Advance offset
 * @return Advanced coordinate
 */
atlas_boundary_t atlas_boundary_advance(atlas_boundary_t boundary, uint32_t offset);

#ifdef __cplusplus
}
#endif

#endif /* ATLAS_BOUNDARY_H */