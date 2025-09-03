; coordinates.ll - Atlas Coordinate System Operations (48×256)
; Layer 1: Boundary Layer - Coordinate and Page Management
; (c) 2024-2025 UOR Foundation. All rights reserved.
; SPDX-License-Identifier: MIT

source_filename = "coordinates.ll"

; =============================================================================
; Type definitions
; =============================================================================

%atlas.coordinate = type { i16, i8 }  ; (page_index, byte_offset)
%atlas.boundary = type i32            ; Φ-encoded coordinate

; =============================================================================
; Coordinate Encoding/Decoding (Φ Isomorphism)
; =============================================================================

; Encode (page, byte) coordinate pair into boundary representation
define i32 @atlas.boundary.encode(i16 %page, i8 %byte) nounwind readnone {
entry:
  ; Validate page bounds [0, 48)
  %page_valid = icmp ult i16 %page, 48
  br i1 %page_valid, label %encode, label %error
  
encode:
  ; Standard encoding: boundary = page * 256 + byte
  %page_ext = zext i16 %page to i32
  %byte_ext = zext i8 %byte to i32
  %shifted = shl i32 %page_ext, 8
  %result = or i32 %shifted, %byte_ext
  ret i32 %result
  
error:
  ; Return invalid coordinate marker
  ret i32 -1
}

; Decode boundary representation back to (page, byte) coordinate
define { i16, i8 } @atlas.boundary.decode(i32 %boundary) nounwind readnone {
entry:
  ; Validate boundary bounds [0, 12288)
  %boundary_valid = icmp ult i32 %boundary, 12288
  br i1 %boundary_valid, label %decode, label %error
  
decode:
  ; Extract page and byte components  
  %page = lshr i32 %boundary, 8
  %byte = and i32 %boundary, 255
  %page_trunc = trunc i32 %page to i16
  %byte_trunc = trunc i32 %byte to i8
  
  ; Return coordinate pair
  %result = insertvalue { i16, i8 } undef, i16 %page_trunc, 0
  %final = insertvalue { i16, i8 } %result, i8 %byte_trunc, 1
  ret { i16, i8 } %final
  
error:
  ; Return invalid coordinate pair
  %error_result = insertvalue { i16, i8 } undef, i16 -1, 0
  %error_final = insertvalue { i16, i8 } %error_result, i8 -1, 1
  ret { i16, i8 } %error_final  
}

; =============================================================================
; Coordinate Validation and Bounds Checking
; =============================================================================

; Check if coordinate is within Atlas structure bounds
define i1 @atlas.coordinate.is_valid(i16 %page, i8 %byte) nounwind readnone {
entry:
  %page_valid = icmp ult i16 %page, 48
  ret i1 %page_valid
  ; byte is always valid (0-255 fits in i8)
}

; Check if boundary coordinate is within Atlas structure bounds
define i1 @atlas.boundary.is_valid(i32 %boundary) nounwind readnone {
entry:
  %valid = icmp ult i32 %boundary, 12288
  ret i1 %valid
}

; =============================================================================
; Page Management Operations
; =============================================================================

; Get page start boundary for given page index
define i32 @atlas.page.start_boundary(i16 %page) nounwind readnone {
entry:
  %valid = icmp ult i16 %page, 48
  br i1 %valid, label %compute, label %error
  
compute:
  %page_ext = zext i16 %page to i32
  %start = shl i32 %page_ext, 8  ; page * 256
  ret i32 %start
  
error:
  ret i32 -1
}

; Get page end boundary for given page index
define i32 @atlas.page.end_boundary(i16 %page) nounwind readnone {
entry:
  %valid = icmp ult i16 %page, 48
  br i1 %valid, label %compute, label %error
  
compute:
  %page_ext = zext i16 %page to i32
  %start = shl i32 %page_ext, 8   ; page * 256
  %end = add i32 %start, 255      ; page * 256 + 255
  ret i32 %end
  
error:
  ret i32 -1
}

; Check if boundary is at page boundary (byte offset == 0)
define i1 @atlas.boundary.is_page_aligned(i32 %boundary) nounwind readnone {
entry:
  %byte = and i32 %boundary, 255
  %aligned = icmp eq i32 %byte, 0
  ret i1 %aligned
}

; =============================================================================
; Distance and Navigation Operations  
; =============================================================================

; Compute linear distance between two boundaries
define i32 @atlas.boundary.distance(i32 %from, i32 %to) nounwind readnone {
entry:
  %from_valid = icmp ult i32 %from, 12288
  %to_valid = icmp ult i32 %to, 12288
  %both_valid = and i1 %from_valid, %to_valid
  br i1 %both_valid, label %compute, label %error
  
compute:
  %diff = sub i32 %to, %from
  %abs = call i32 @atlas.abs(i32 %diff)
  ret i32 %abs
  
error:
  ret i32 -1
}

; Advance boundary by given offset (with wraparound)
define i32 @atlas.boundary.advance(i32 %boundary, i32 %offset) nounwind readnone {
entry:
  %valid = icmp ult i32 %boundary, 12288
  br i1 %valid, label %compute, label %error
  
compute:
  %new_boundary = add i32 %boundary, %offset
  %wrapped = urem i32 %new_boundary, 12288
  ret i32 %wrapped
  
error:
  ret i32 %boundary  ; Return original on error
}

; =============================================================================
; Helper Functions
; =============================================================================

; Absolute value helper
define i32 @atlas.abs(i32 %value) nounwind readnone {
entry:
  %is_negative = icmp slt i32 %value, 0
  br i1 %is_negative, label %negate, label %positive
  
negate:
  %negated = sub i32 0, %value
  ret i32 %negated
  
positive:
  ret i32 %value
}