; RUN: opt -S < %s | FileCheck %s
; RUN: llc -O3 < %s | FileCheck %s --check-prefix=ASM

; conserved-memops.ll - Test conserved memcpy/memset with fixups
; Tests the Atlas-12288 Layer 2 conserved memory operations

source_filename = "conserved-memops.ll"
target datalayout = "e-m:e-p270:32:32-p271:32:32-p272:64:64-i64:64-i128:128-f80:128-n8:16:32:64-S128"
target triple = "x86_64-unknown-linux-gnu"

; Layer 2 conserved memory operation intrinsics
declare void @atlas.l2.memcpy.conserved(ptr, ptr, i64)
declare void @atlas.l2.memset.conserved(ptr, i8, i64)
declare void @atlas.l2.memcpy.fixup(ptr, ptr, i64, i64)
declare i64 @atlas.l2.memops.delta(ptr, i64)
declare i1 @atlas.l2.memops.verify(ptr, i64)

; Test basic conserved memcpy
define void @test_basic_conserved_memcpy() {
; CHECK-LABEL: @test_basic_conserved_memcpy
; CHECK: call void @atlas.l2.memcpy.conserved
entry:
  %src = alloca [96 x i8], align 1
  %dst = alloca [96 x i8], align 1
  
  %src_ptr = getelementptr [96 x i8], ptr %src, i32 0, i32 0
  %dst_ptr = getelementptr [96 x i8], ptr %dst, i32 0, i32 0
  
  ; Initialize source with conserved pattern
  call void @llvm.memset.p0.i64(ptr %src_ptr, i8 0, i64 96, i1 false)
  
  ; Fill pattern that sums to 0 mod 96
  br label %fill_loop

fill_loop:
  %i = phi i64 [ 0, %entry ], [ %i_next, %fill_body ]
  %done = icmp eq i64 %i, 95
  br i1 %done, label %set_last, label %fill_body

fill_body:
  %ptr_i = getelementptr i8, ptr %src_ptr, i64 %i
  %val = trunc i64 %i to i8
  store i8 %val, ptr %ptr_i
  %i_next = add i64 %i, 1
  br label %fill_loop

set_last:
  ; Last byte makes sum conserved: sum(0..94) = 4465, need 4465 + x ≡ 0 (mod 96)
  ; 4465 mod 96 = 49, so x = 96 - 49 = 47
  %last_ptr = getelementptr i8, ptr %src_ptr, i64 95
  store i8 47, ptr %last_ptr
  
  ; Initialize destination with non-conserved data
  call void @llvm.memset.p0.i64(ptr %dst_ptr, i8 255, i64 96, i1 false)
  
  ; Perform conserved memcpy
  call void @atlas.l2.memcpy.conserved(ptr %dst_ptr, ptr %src_ptr, i64 96)
  
  ; Verify destination is now conserved
  %verified = call i1 @atlas.l2.memops.verify(ptr %dst_ptr, i64 96)
  ; CHECK: call i1 @atlas.l2.memops.verify
  ret void
}

; Test conserved memset
define void @test_conserved_memset() {
; CHECK-LABEL: @test_conserved_memset
; CHECK: call void @atlas.l2.memset.conserved
entry:
  %buf = alloca [96 x i8], align 1
  %ptr = getelementptr [96 x i8], ptr %buf, i32 0, i32 0
  
  ; Initialize with random data
  call void @llvm.memset.p0.i64(ptr %ptr, i8 123, i64 96, i1 false)
  
  ; Conserved memset to value that makes sum ≡ 0 (mod 96)
  ; We want 0 * 96 ≡ 0 (mod 96), so use value 0
  call void @atlas.l2.memset.conserved(ptr %ptr, i8 0, i64 96)
  
  ; Verify result is conserved
  %verified = call i1 @atlas.l2.memops.verify(ptr %ptr, i64 96)
  ; CHECK: call i1 @atlas.l2.memops.verify
  ret void
}

; Test memcpy with fixup
define void @test_memcpy_with_fixup() {
; CHECK-LABEL: @test_memcpy_with_fixup
; CHECK: call void @atlas.l2.memcpy.fixup
entry:
  %src = alloca [96 x i8], align 1
  %dst = alloca [96 x i8], align 1
  
  %src_ptr = getelementptr [96 x i8], ptr %src, i32 0, i32 0
  %dst_ptr = getelementptr [96 x i8], ptr %dst, i32 0, i32 0
  
  ; Source has non-conserved pattern (all 1s, sum = 96)
  call void @llvm.memset.p0.i64(ptr %src_ptr, i8 1, i64 96, i1 false)
  
  ; Destination starts conserved (all 0s)
  call void @llvm.memset.p0.i64(ptr %dst_ptr, i8 0, i64 96, i1 false)
  
  ; Compute required fixup delta
  %delta = call i64 @atlas.l2.memops.delta(ptr %src_ptr, i64 96)
  ; Delta = 96 (sum of source)
  
  ; Copy with fixup to maintain conservation
  call void @atlas.l2.memcpy.fixup(ptr %dst_ptr, ptr %src_ptr, i64 96, i64 %delta)
  
  ; Verify destination remains conserved after fixup
  %verified = call i1 @atlas.l2.memops.verify(ptr %dst_ptr, i64 96)
  ; CHECK: call i1 @atlas.l2.memops.verify
  ret void
}

; Test overlapping memory regions
define void @test_overlapping_memcpy() {
; CHECK-LABEL: @test_overlapping_memcpy
; CHECK: call void @atlas.l2.memcpy.conserved
entry:
  %buf = alloca [128 x i8], align 1
  %ptr = getelementptr [128 x i8], ptr %buf, i32 0, i32 0
  
  ; Initialize first 64 bytes with conserved pattern
  call void @llvm.memset.p0.i64(ptr %ptr, i8 0, i64 128, i1 false)
  
  ; Source region (bytes 0-63)
  %src_ptr = ptr %ptr
  
  ; Destination region (bytes 32-95, overlapping)
  %dst_ptr = getelementptr i8, ptr %ptr, i64 32
  
  ; This tests conservative overlapping copy behavior
  call void @atlas.l2.memcpy.conserved(ptr %dst_ptr, ptr %src_ptr, i64 64)
  ret void
}

; Test memset with non-zero conserved value
define void @test_memset_nonzero_conserved() {
; CHECK-LABEL: @test_memset_nonzero_conserved
; CHECK: call void @atlas.l2.memset.conserved
entry:
  %buf = alloca [96 x i8], align 1
  %ptr = getelementptr [96 x i8], ptr %buf, i32 0, i32 0
  
  ; Use value that creates conserved sum
  ; For 96 bytes, we need val * 96 ≡ 0 (mod 96)
  ; This is true for any val, since 96 ≡ 0 (mod 96)
  ; But conserved memset should adjust to ensure exact conservation
  call void @atlas.l2.memset.conserved(ptr %ptr, i8 42, i64 96)
  
  %verified = call i1 @atlas.l2.memops.verify(ptr %ptr, i64 96)
  ; CHECK: call i1 @atlas.l2.memops.verify
  ret void
}

; Test large buffer operations (Atlas-12288)
define void @test_large_buffer_ops() {
; CHECK-LABEL: @test_large_buffer_ops
entry:
  %size = i64 12288
  %src = alloca i8, i64 12288, align 1
  %dst = alloca i8, i64 12288, align 1
  
  ; Initialize large source buffer
  call void @llvm.memset.p0.i64(ptr %src, i8 0, i64 %size, i1 false)
  
  ; Large conserved memcpy
  call void @atlas.l2.memcpy.conserved(ptr %dst, ptr %src, i64 %size)
  
  ; Verify large buffer conservation
  %verified = call i1 @atlas.l2.memops.verify(ptr %dst, i64 %size)
  ; CHECK: call i1 @atlas.l2.memops.verify
  ret void
}

; Test partial buffer operations
define void @test_partial_buffer_ops() {
; CHECK-LABEL: @test_partial_buffer_ops
entry:
  %buf = alloca [96 x i8], align 1
  %ptr = getelementptr [96 x i8], ptr %buf, i32 0, i32 0
  
  ; Initialize full buffer
  call void @llvm.memset.p0.i64(ptr %ptr, i8 0, i64 96, i1 false)
  
  ; Memset only middle portion (bytes 32-63)
  %middle_ptr = getelementptr i8, ptr %ptr, i64 32
  call void @atlas.l2.memset.conserved(ptr %middle_ptr, i8 10, i64 32)
  
  ; This should maintain overall buffer conservation
  %delta = call i64 @atlas.l2.memops.delta(ptr %ptr, i64 96)
  ; CHECK: call i64 @atlas.l2.memops.delta
  ret void
}

; Test memops delta calculation
define i64 @test_memops_delta() {
; CHECK-LABEL: @test_memops_delta
; CHECK: call i64 @atlas.l2.memops.delta
entry:
  %buf = alloca [96 x i8], align 1
  %ptr = getelementptr [96 x i8], ptr %buf, i32 0, i32 0
  
  ; Fill with pattern: value = index mod 256
  br label %fill_loop

fill_loop:
  %i = phi i64 [ 0, %entry ], [ %i_next, %fill_body ]
  %done = icmp eq i64 %i, 96
  br i1 %done, label %compute, label %fill_body

fill_body:
  %ptr_i = getelementptr i8, ptr %ptr, i64 %i
  %val = trunc i64 %i to i8
  store i8 %val, ptr %ptr_i
  %i_next = add i64 %i, 1
  br label %fill_loop

compute:
  ; Compute deviation from conservation
  %delta = call i64 @atlas.l2.memops.delta(ptr %ptr, i64 96)
  ; Expected: sum(0..95) mod 96 = 4560 mod 96 = 48
  ; CHECK: ; Expected delta represents deviation from conservation
  ret i64 %delta
}

; Test edge case: single byte operations
define void @test_single_byte_ops() {
; CHECK-LABEL: @test_single_byte_ops
entry:
  %src = alloca i8, align 1
  %dst = alloca i8, align 1
  
  store i8 100, ptr %src
  store i8 50, ptr %dst
  
  ; Single byte conserved copy
  call void @atlas.l2.memcpy.conserved(ptr %dst, ptr %src, i64 1)
  
  ; Single byte is always "conserved" (sum mod 1 = 0)
  %verified = call i1 @atlas.l2.memops.verify(ptr %dst, i64 1)
  ; CHECK: call i1 @atlas.l2.memops.verify
  ret void
}

; Test boundary alignment
define void @test_boundary_alignment() {
; CHECK-LABEL: @test_boundary_alignment
entry:
  %buf = alloca [96 x i8], align 32  ; 32-byte aligned
  %ptr = getelementptr [96 x i8], ptr %buf, i32 0, i32 0
  
  ; Test aligned conserved memset
  call void @atlas.l2.memset.conserved(ptr %ptr, i8 0, i64 96)
  
  ; Test unaligned offset
  %unaligned_ptr = getelementptr i8, ptr %ptr, i64 7
  call void @atlas.l2.memset.conserved(ptr %unaligned_ptr, i8 1, i64 64)
  
  %verified = call i1 @atlas.l2.memops.verify(ptr %ptr, i64 96)
  ; CHECK: call i1 @atlas.l2.memops.verify
  ret void
}

declare void @llvm.memset.p0.i64(ptr, i8, i64, i1)

; ASM: .text
; ASM: test_basic_conserved_memcpy:
; ASM: callq atlas.l2.memcpy.conserved
; ASM: test_conserved_memset:
; ASM: callq atlas.l2.memset.conserved
; ASM: test_memcpy_with_fixup:
; ASM: callq atlas.l2.memcpy.fixup