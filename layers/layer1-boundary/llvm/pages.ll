; pages.ll - Atlas Page Management Operations  
; Layer 1: Boundary Layer - Page Structure and Management
; (c) 2024-2025 UOR Foundation. All rights reserved.
; SPDX-License-Identifier: MIT

source_filename = "pages.ll"

; =============================================================================
; Type definitions
; =============================================================================

%atlas.page = type [256 x i8]         ; 256-byte page
%atlas.structure = type [48 x %atlas.page] ; 48 pages = 12,288 bytes
%atlas.page_info = type { i16, i1, i32 }   ; (index, is_valid, checksum)

; =============================================================================
; Page Access Operations
; =============================================================================

; Get pointer to specific page in structure
define ptr @atlas.page.get_ptr(ptr %structure, i16 %page_index) nounwind {
entry:
  ; Validate page index
  %valid = icmp ult i16 %page_index, 48
  br i1 %valid, label %compute, label %error
  
compute:
  %page_index_ext = zext i16 %page_index to i64
  %page_ptr = getelementptr %atlas.structure, ptr %structure, i32 0, i64 %page_index_ext
  ret ptr %page_ptr
  
error:
  ret ptr null
}

; Get byte pointer at specific coordinate within structure
define ptr @atlas.page.get_byte_ptr(ptr %structure, i16 %page_index, i8 %byte_offset) nounwind {
entry:
  ; Validate page index
  %page_valid = icmp ult i16 %page_index, 48
  br i1 %page_valid, label %compute, label %error
  
compute:
  %page_index_ext = zext i16 %page_index to i64
  %byte_offset_ext = zext i8 %byte_offset to i64
  %byte_ptr = getelementptr %atlas.structure, ptr %structure, i32 0, i64 %page_index_ext, i64 %byte_offset_ext
  ret ptr %byte_ptr
  
error:
  ret ptr null
}

; =============================================================================
; Page Initialization and Clearing
; =============================================================================

; Initialize page with zeros
define void @atlas.page.clear(ptr %page_ptr) nounwind {
entry:
  call void @llvm.memset.p0.i64(ptr %page_ptr, i8 0, i64 256, i1 false)
  ret void
}

; Initialize page with pattern value
define void @atlas.page.fill(ptr %page_ptr, i8 %value) nounwind {
entry:
  call void @llvm.memset.p0.i64(ptr %page_ptr, i8 %value, i64 256, i1 false)
  ret void
}

; Initialize page with index-based pattern for testing
define void @atlas.page.init_test_pattern(ptr %page_ptr, i16 %page_index) nounwind {
entry:
  br label %loop
  
loop:
  %i = phi i64 [ 0, %entry ], [ %next_i, %body ]
  %done = icmp uge i64 %i, 256
  br i1 %done, label %exit, label %body
  
body:
  ; Generate test pattern: (page_index * 256 + byte_index) % 96
  %page_ext = zext i16 %page_index to i64
  %page_offset = mul i64 %page_ext, 256
  %total_offset = add i64 %page_offset, %i
  %pattern = urem i64 %total_offset, 96
  %pattern_byte = trunc i64 %pattern to i8
  
  %byte_ptr = getelementptr i8, ptr %page_ptr, i64 %i
  store i8 %pattern_byte, ptr %byte_ptr, align 1
  
  %next_i = add i64 %i, 1
  br label %loop
  
exit:
  ret void
}

; =============================================================================
; Page Copy and Comparison Operations
; =============================================================================

; Copy entire page from source to destination
define void @atlas.page.copy(ptr %dest_page, ptr %src_page) nounwind {
entry:
  call void @llvm.memcpy.p0.p0.i64(ptr %dest_page, ptr %src_page, i64 256, i1 false)
  ret void
}

; Compare two pages for equality
define i1 @atlas.page.equal(ptr %page1, ptr %page2) nounwind readonly {
entry:
  br label %loop
  
loop:
  %i = phi i64 [ 0, %entry ], [ %next_i, %continue ]
  %done = icmp uge i64 %i, 256
  br i1 %done, label %equal, label %body
  
body:
  %ptr1 = getelementptr i8, ptr %page1, i64 %i
  %ptr2 = getelementptr i8, ptr %page2, i64 %i
  %byte1 = load i8, ptr %ptr1, align 1
  %byte2 = load i8, ptr %ptr2, align 1
  %bytes_equal = icmp eq i8 %byte1, %byte2
  br i1 %bytes_equal, label %continue, label %not_equal
  
continue:
  %next_i = add i64 %i, 1
  br label %loop
  
equal:
  ret i1 true
  
not_equal:
  ret i1 false
}

; =============================================================================
; Page Validation and Integrity
; =============================================================================

; Compute simple checksum of page (sum of all bytes mod 256)
define i32 @atlas.page.checksum(ptr %page_ptr) nounwind readonly {
entry:
  br label %loop
  
loop:
  %i = phi i64 [ 0, %entry ], [ %next_i, %body ]
  %sum = phi i32 [ 0, %entry ], [ %new_sum, %body ]
  %done = icmp uge i64 %i, 256
  br i1 %done, label %exit, label %body
  
body:
  %byte_ptr = getelementptr i8, ptr %page_ptr, i64 %i
  %byte = load i8, ptr %byte_ptr, align 1
  %byte_ext = zext i8 %byte to i32
  %new_sum = add i32 %sum, %byte_ext
  %next_i = add i64 %i, 1
  br label %loop
  
exit:
  ret i32 %sum
}

; Get page information structure
define %atlas.page_info @atlas.page.get_info(ptr %structure, i16 %page_index) nounwind readonly {
entry:
  %valid = icmp ult i16 %page_index, 48
  br i1 %valid, label %compute, label %error
  
compute:
  %page_ptr = call ptr @atlas.page.get_ptr(ptr %structure, i16 %page_index)
  %checksum = call i32 @atlas.page.checksum(ptr %page_ptr)
  
  %info = insertvalue %atlas.page_info undef, i16 %page_index, 0
  %info2 = insertvalue %atlas.page_info %info, i1 true, 1
  %info3 = insertvalue %atlas.page_info %info2, i32 %checksum, 2
  ret %atlas.page_info %info3
  
error:
  %error_info = insertvalue %atlas.page_info undef, i16 %page_index, 0
  %error_info2 = insertvalue %atlas.page_info %error_info, i1 false, 1
  %error_info3 = insertvalue %atlas.page_info %error_info2, i32 0, 2
  ret %atlas.page_info %error_info3
}

; =============================================================================
; LLVM Intrinsic Declarations
; =============================================================================

declare void @llvm.memset.p0.i64(ptr nocapture writeonly, i8, i64, i1 immarg)
declare void @llvm.memcpy.p0.p0.i64(ptr nocapture writeonly, ptr nocapture readonly, i64, i1 immarg)