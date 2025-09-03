; RUN: opt -S < %s | FileCheck %s
; RUN: llc -O3 < %s | FileCheck %s --check-prefix=ASM

; conserved-delta.ll - Test delta computation with various inputs
; Tests the Atlas-12288 Layer 2 delta computation operations

source_filename = "conserved-delta.ll"
target datalayout = "e-m:e-p270:32:32-p271:32:32-p272:64:64-i64:64-i128:128-f80:128-n8:16:32:64-S128"
target triple = "x86_64-unknown-linux-gnu"

; Layer 2 delta computation intrinsics
declare i64 @atlas.l2.delta.compute(ptr, ptr, i64)
declare void @atlas.l2.delta.apply(ptr, i64, i64)
declare i1 @atlas.l2.delta.validate(i64, i64)

; Test basic delta computation
define i64 @test_basic_delta() {
; CHECK-LABEL: @test_basic_delta
; CHECK: call i64 @atlas.l2.delta.compute
; CHECK: ret i64
entry:
  %buf1 = alloca [96 x i8], align 1
  %buf2 = alloca [96 x i8], align 1
  
  ; Initialize first buffer with pattern
  %ptr1 = getelementptr [96 x i8], ptr %buf1, i32 0, i32 0
  %ptr2 = getelementptr [96 x i8], ptr %buf2, i32 0, i32 0
  
  call void @llvm.memset.p0.i64(ptr %ptr1, i8 42, i64 96, i1 false)
  call void @llvm.memset.p0.i64(ptr %ptr2, i8 84, i64 96, i1 false)
  
  ; Compute delta
  %delta = call i64 @atlas.l2.delta.compute(ptr %ptr1, ptr %ptr2, i64 96)
  
  ; Expected delta: (84 - 42) * 96 = 42 * 96 = 4032
  ; CHECK: ; Expected delta: 4032
  ret i64 %delta
}

; Test zero delta computation
define i64 @test_zero_delta() {
; CHECK-LABEL: @test_zero_delta
; CHECK: call i64 @atlas.l2.delta.compute
; CHECK: ret i64 0
entry:
  %buf1 = alloca [96 x i8], align 1
  %buf2 = alloca [96 x i8], align 1
  
  %ptr1 = getelementptr [96 x i8], ptr %buf1, i32 0, i32 0
  %ptr2 = getelementptr [96 x i8], ptr %buf2, i32 0, i32 0
  
  ; Same pattern in both buffers
  call void @llvm.memset.p0.i64(ptr %ptr1, i8 100, i64 96, i1 false)
  call void @llvm.memset.p0.i64(ptr %ptr2, i8 100, i64 96, i1 false)
  
  %delta = call i64 @atlas.l2.delta.compute(ptr %ptr1, ptr %ptr2, i64 96)
  ret i64 %delta
}

; Test delta with mixed pattern
define i64 @test_mixed_pattern_delta() {
; CHECK-LABEL: @test_mixed_pattern_delta
; CHECK: call i64 @atlas.l2.delta.compute
entry:
  %buf1 = alloca [96 x i8], align 1
  %buf2 = alloca [96 x i8], align 1
  
  %ptr1 = getelementptr [96 x i8], ptr %buf1, i32 0, i32 0
  %ptr2 = getelementptr [96 x i8], ptr %buf2, i32 0, i32 0
  
  ; Fill buf1: 0,1,2,...,95
  br label %fill1_loop

fill1_loop:
  %i1 = phi i64 [ 0, %entry ], [ %i1_next, %fill1_body ]
  %done1 = icmp eq i64 %i1, 96
  br i1 %done1, label %fill2_start, label %fill1_body

fill1_body:
  %ptr1_i = getelementptr i8, ptr %ptr1, i64 %i1
  %val1 = trunc i64 %i1 to i8
  store i8 %val1, ptr %ptr1_i
  %i1_next = add i64 %i1, 1
  br label %fill1_loop

fill2_start:
  ; Fill buf2: 95,94,93,...,0 (reverse pattern)
  br label %fill2_loop

fill2_loop:
  %i2 = phi i64 [ 0, %fill2_start ], [ %i2_next, %fill2_body ]
  %done2 = icmp eq i64 %i2, 96
  br i1 %done2, label %compute, label %fill2_body

fill2_body:
  %ptr2_i = getelementptr i8, ptr %ptr2, i64 %i2
  %val2_raw = sub i64 95, %i2
  %val2 = trunc i64 %val2_raw to i8
  store i8 %val2, ptr %ptr2_i
  %i2_next = add i64 %i2, 1
  br label %fill2_loop

compute:
  %delta = call i64 @atlas.l2.delta.compute(ptr %ptr1, ptr %ptr2, i64 96)
  ; Sum of buf1: 0+1+...+95 = 4560
  ; Sum of buf2: 95+94+...+0 = 4560
  ; Delta should be 0
  ; CHECK: ; Expected delta: 0 (symmetric patterns)
  ret i64 %delta
}

; Test delta application
define void @test_delta_apply() {
; CHECK-LABEL: @test_delta_apply
; CHECK: call void @atlas.l2.delta.apply
entry:
  %buf = alloca [96 x i8], align 1
  %ptr = getelementptr [96 x i8], ptr %buf, i32 0, i32 0
  
  ; Initialize with zeros
  call void @llvm.memset.p0.i64(ptr %ptr, i8 0, i64 96, i1 false)
  
  ; Apply delta of 960 (should add 10 to each byte)
  call void @atlas.l2.delta.apply(ptr %ptr, i64 960, i64 96)
  
  ; Verify first byte
  %first_byte = load i8, ptr %ptr
  ; CHECK: load i8, ptr %ptr
  ret void
}

; Test delta validation
define i1 @test_delta_validate() {
; CHECK-LABEL: @test_delta_validate
; CHECK: call i1 @atlas.l2.delta.validate
entry:
  ; Valid delta (multiple of size for uniform distribution)
  %valid1 = call i1 @atlas.l2.delta.validate(i64 960, i64 96)
  
  ; Invalid delta (would cause overflow)
  %valid2 = call i1 @atlas.l2.delta.validate(i64 25600, i64 96)
  
  %result = and i1 %valid1, %valid2
  ret i1 %result
}

; Test edge case: single byte buffer
define i64 @test_single_byte_delta() {
; CHECK-LABEL: @test_single_byte_delta
; CHECK: call i64 @atlas.l2.delta.compute
entry:
  %buf1 = alloca i8, align 1
  %buf2 = alloca i8, align 1
  
  store i8 50, ptr %buf1
  store i8 200, ptr %buf2
  
  %delta = call i64 @atlas.l2.delta.compute(ptr %buf1, ptr %buf2, i64 1)
  ; Expected: 200 - 50 = 150
  ret i64 %delta
}

; Test edge case: maximum size delta
define i64 @test_large_buffer_delta() {
; CHECK-LABEL: @test_large_buffer_delta
; CHECK: call i64 @atlas.l2.delta.compute
entry:
  %size = i64 12288  ; Atlas-12288 maximum
  %buf1 = alloca i8, i64 12288, align 1
  %buf2 = alloca i8, i64 12288, align 1
  
  call void @llvm.memset.p0.i64(ptr %buf1, i8 1, i64 %size, i1 false)
  call void @llvm.memset.p0.i64(ptr %buf2, i8 2, i64 %size, i1 false)
  
  %delta = call i64 @atlas.l2.delta.compute(ptr %buf1, ptr %buf2, i64 %size)
  ; Expected: (2 - 1) * 12288 = 12288
  ret i64 %delta
}

declare void @llvm.memset.p0.i64(ptr, i8, i64, i1)

; ASM: .text
; ASM: test_basic_delta:
; ASM: callq atlas.l2.delta.compute
; ASM: test_zero_delta:
; ASM: callq atlas.l2.delta.compute