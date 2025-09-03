; exports.ll - Layer 1 Boundary C ABI Exports
; (c) 2024-2025 UOR Foundation - MIT License
; 
; C ABI exports for Layer 1 (Boundary Layer) functions
; Provides C-compatible interface to internal LLVM implementations

source_filename = "exports.ll"

; =============================================================================
; External LLVM Function Declarations
; =============================================================================

; From coordinates.ll
declare i32 @atlas.boundary.encode(i16, i8)
declare { i16, i8 } @atlas.boundary.decode(i32)
declare i1 @atlas.coordinate.is_valid(i16, i8)
declare i1 @atlas.boundary.is_valid(i32)
declare i32 @atlas.boundary.distance(i32, i32)
declare i32 @atlas.boundary.advance(i32, i32)

; From klein.ll
declare i8 @atlas.klein.get_orbit_id(i32)
declare i1 @atlas.klein.is_privileged_orbit(i32)

; From pages.ll
declare ptr @atlas.page.get_ptr(ptr, i16)
declare ptr @atlas.page.get_byte_ptr(ptr, i16, i8)
declare void @atlas.page.copy(ptr, ptr)
declare i1 @atlas.page.equal(ptr, ptr)
declare i32 @atlas.page.checksum(ptr)

; From phi.ll (morphisms) - actual functions available
declare i32 @atlas.morphism.boundary_auto(i32, i8, i8)
declare i1 @atlas.morphism.is_valid_unit(i8, i8)

; =============================================================================
; C ABI Export Functions
; =============================================================================

; Coordinate Operations
define i32 @atlas_boundary_encode_llvm(i16 %page, i8 %byte) {
entry:
  %result = call i32 @atlas.boundary.encode(i16 %page, i8 %byte)
  ret i32 %result
}

define void @atlas_boundary_decode_llvm(i32 %boundary, ptr %page_out, ptr %byte_out) {
entry:
  %result = call { i16, i8 } @atlas.boundary.decode(i32 %boundary)
  %page = extractvalue { i16, i8 } %result, 0
  %byte = extractvalue { i16, i8 } %result, 1
  
  ; Store results through output pointers
  store i16 %page, ptr %page_out
  store i8 %byte, ptr %byte_out
  ret void
}

define i1 @atlas_coordinate_is_valid_llvm(i16 %page, i8 %byte) {
entry:
  %result = call i1 @atlas.coordinate.is_valid(i16 %page, i8 %byte)
  ret i1 %result
}

define i1 @atlas_boundary_is_valid_llvm(i32 %boundary) {
entry:
  %result = call i1 @atlas.boundary.is_valid(i32 %boundary)
  ret i1 %result
}

; Klein Orbit Operations
define i8 @atlas_klein_get_orbit_id_llvm(i32 %coord) {
entry:
  %result = call i8 @atlas.klein.get_orbit_id(i32 %coord)
  ret i8 %result
}

define i1 @atlas_klein_is_privileged_orbit_llvm(i32 %coord) {
entry:
  %result = call i1 @atlas.klein.is_privileged_orbit(i32 %coord)
  ret i1 %result
}

; Page Operations
define ptr @atlas_page_get_ptr_llvm(ptr %structure, i16 %page_index) {
entry:
  %result = call ptr @atlas.page.get_ptr(ptr %structure, i16 %page_index)
  ret ptr %result
}

define ptr @atlas_page_get_byte_ptr_llvm(ptr %structure, i16 %page_index, i8 %byte_offset) {
entry:
  %result = call ptr @atlas.page.get_byte_ptr(ptr %structure, i16 %page_index, i8 %byte_offset)
  ret ptr %result
}

define void @atlas_page_copy_llvm(ptr %dest_page, ptr %src_page) {
entry:
  call void @atlas.page.copy(ptr %dest_page, ptr %src_page)
  ret void
}

define i1 @atlas_page_equal_llvm(ptr %page1, ptr %page2) {
entry:
  %result = call i1 @atlas.page.equal(ptr %page1, ptr %page2)
  ret i1 %result
}

define i32 @atlas_page_checksum_llvm(ptr %page_ptr) {
entry:
  %result = call i32 @atlas.page.checksum(ptr %page_ptr)
  ret i32 %result
}

; Distance and Navigation Operations
define i32 @atlas_boundary_distance_llvm(i32 %from, i32 %to) {
entry:
  %result = call i32 @atlas.boundary.distance(i32 %from, i32 %to)
  ret i32 %result
}

define i32 @atlas_boundary_advance_llvm(i32 %boundary, i32 %offset) {
entry:
  %result = call i32 @atlas.boundary.advance(i32 %boundary, i32 %offset)
  ret i32 %result
}

; Morphism Operations
define i32 @atlas_morphism_boundary_auto_llvm(i32 %coord, i8 %u48, i8 %u256) {
entry:
  %result = call i32 @atlas.morphism.boundary_auto(i32 %coord, i8 %u48, i8 %u256)
  ret i32 %result
}

define i1 @atlas_morphism_is_valid_unit_llvm(i8 %u48, i8 %u256) {
entry:
  %result = call i1 @atlas.morphism.is_valid_unit(i8 %u48, i8 %u256)
  ret i1 %result
}

; =============================================================================
; Advanced Export Functions
; =============================================================================

; Batch coordinate validation
define i1 @atlas_validate_coordinates_llvm(ptr %coords, i64 %count) {
entry:
  br label %loop

loop:
  %i = phi i64 [ 0, %entry ], [ %next_i, %check_next ]
  %all_valid = phi i1 [ true, %entry ], [ %still_valid, %check_next ]
  
  %done = icmp uge i64 %i, %count
  br i1 %done, label %exit, label %check_coord

check_coord:
  %coord_ptr = getelementptr i32, ptr %coords, i64 %i
  %coord = load i32, ptr %coord_ptr
  %valid = call i1 @atlas.boundary.is_valid(i32 %coord)
  %still_valid = and i1 %all_valid, %valid
  
  ; Early exit if invalid found
  br i1 %valid, label %check_next, label %exit

check_next:
  %next_i = add i64 %i, 1
  br label %loop

exit:
  %result = phi i1 [ %all_valid, %loop ], [ false, %check_coord ]
  ret i1 %result
}

; Page range operations
define void @atlas_page_range_copy_llvm(ptr %dest, ptr %src, i16 %start_page, i16 %num_pages) {
entry:
  br label %loop

loop:
  %i = phi i16 [ 0, %entry ], [ %next_i, %continue ]
  %done = icmp uge i16 %i, %num_pages
  br i1 %done, label %exit, label %body

body:
  %current_page = add i16 %start_page, %i
  %dest_page_ptr = call ptr @atlas.page.get_ptr(ptr %dest, i16 %current_page)
  %src_page_ptr = call ptr @atlas.page.get_ptr(ptr %src, i16 %current_page)
  
  ; Skip if either pointer is null
  %dest_valid = icmp ne ptr %dest_page_ptr, null
  %src_valid = icmp ne ptr %src_page_ptr, null
  %both_valid = and i1 %dest_valid, %src_valid
  br i1 %both_valid, label %do_copy, label %continue

do_copy:
  call void @atlas.page.copy(ptr %dest_page_ptr, ptr %src_page_ptr)
  br label %continue

continue:
  %next_i = add i16 %i, 1
  br label %loop

exit:
  ret void
}

; Klein orbit batch classification  
define void @atlas_klein_classify_batch_llvm(ptr %coords, ptr %orbit_ids, i64 %count) {
entry:
  br label %loop

loop:
  %i = phi i64 [ 0, %entry ], [ %next_i, %body ]
  %done = icmp uge i64 %i, %count
  br i1 %done, label %exit, label %body

body:
  %coord_ptr = getelementptr i32, ptr %coords, i64 %i
  %coord = load i32, ptr %coord_ptr
  %orbit_id = call i8 @atlas.klein.get_orbit_id(i32 %coord)
  
  %orbit_ptr = getelementptr i8, ptr %orbit_ids, i64 %i
  store i8 %orbit_id, ptr %orbit_ptr
  
  %next_i = add i64 %i, 1
  br label %loop

exit:
  ret void
}