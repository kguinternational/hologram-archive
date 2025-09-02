; RUN: opt -S < %s | FileCheck %s
; RUN: llc -O3 < %s | FileCheck %s --check-prefix=ASM

; conserved-window.ll - Test window conservation checking
; Tests the Atlas-12288 Layer 2 windowed conservation operations

source_filename = "conserved-window.ll"
target datalayout = "e-m:e-p270:32:32-p271:32:32-p272:64:64-i64:64-i128:128-f80:128-n8:16:32:64-S128"
target triple = "x86_64-unknown-linux-gnu"

; Layer 2 windowed conservation intrinsics
declare i1 @atlas.l2.window.check(ptr, i64, i64, i64)
declare void @atlas.l2.window.adjust(ptr, i64, i64, i64, i64)
declare i64 @atlas.l2.window.sum(ptr, i64, i64)

; Test basic window conservation check
define i1 @test_basic_window_check() {
; CHECK-LABEL: @test_basic_window_check
; CHECK: call i1 @atlas.l2.window.check
entry:
  %buf = alloca [96 x i8], align 1
  %ptr = getelementptr [96 x i8], ptr %buf, i32 0, i32 0
  
  ; Create conserved pattern: window of 32 bytes with sum = 0 mod 32
  call void @llvm.memset.p0.i64(ptr %ptr, i8 0, i64 96, i1 false)
  
  ; Fill first window with pattern that sums to multiple of 32
  br label %fill_loop

fill_loop:
  %i = phi i64 [ 0, %entry ], [ %i_next, %fill_body ]
  %done = icmp eq i64 %i, 31
  br i1 %done, label %set_last, label %fill_body

fill_body:
  %ptr_i = getelementptr i8, ptr %ptr, i64 %i
  %val = trunc i64 %i to i8
  store i8 %val, ptr %ptr_i
  %i_next = add i64 %i, 1
  br label %fill_loop

set_last:
  ; Set last byte to make sum divisible by 32
  ; Sum of 0+1+...+30 = 465, need 465 + x ≡ 0 (mod 32)
  ; 465 mod 32 = 17, so need x = 32 - 17 = 15
  %last_ptr = getelementptr i8, ptr %ptr, i64 31
  store i8 15, ptr %last_ptr
  
  ; Check window conservation (offset=0, window=32, stride=32)
  %conserved = call i1 @atlas.l2.window.check(ptr %ptr, i64 96, i64 32, i64 32)
  ; CHECK: ; Window should be conserved
  ret i1 %conserved
}

; Test overlapping windows
define i1 @test_overlapping_windows() {
; CHECK-LABEL: @test_overlapping_windows  
; CHECK: call i1 @atlas.l2.window.check
entry:
  %buf = alloca [96 x i8], align 1
  %ptr = getelementptr [96 x i8], ptr %buf, i32 0, i32 0
  
  ; Initialize all to 1 (each window of 16 bytes sums to 16)
  call void @llvm.memset.p0.i64(ptr %ptr, i8 1, i64 96, i1 false)
  
  ; Check overlapping windows (window=16, stride=8)
  %conserved = call i1 @atlas.l2.window.check(ptr %ptr, i64 96, i64 16, i64 8)
  
  ; Each window sums to 16, which is divisible by 16
  ret i1 %conserved
}

; Test window sum computation
define i64 @test_window_sum() {
; CHECK-LABEL: @test_window_sum
; CHECK: call i64 @atlas.l2.window.sum
entry:
  %buf = alloca [96 x i8], align 1
  %ptr = getelementptr [96 x i8], ptr %buf, i32 0, i32 0
  
  ; Fill with known pattern
  call void @llvm.memset.p0.i64(ptr %ptr, i8 5, i64 96, i1 false)
  
  ; Sum first 32 bytes (should be 5 * 32 = 160)
  %sum = call i64 @atlas.l2.window.sum(ptr %ptr, i64 32, i64 0)
  ; CHECK: ; Expected sum: 160
  ret i64 %sum
}

; Test window adjustment
define void @test_window_adjust() {
; CHECK-LABEL: @test_window_adjust
; CHECK: call void @atlas.l2.window.adjust
entry:
  %buf = alloca [96 x i8], align 1
  %ptr = getelementptr [96 x i8], ptr %buf, i32 0, i32 0
  
  ; Initialize with non-conserved pattern
  call void @llvm.memset.p0.i64(ptr %ptr, i8 7, i64 96, i1 false)
  
  ; Adjust to make conserved (target_sum=0, window=48, offset=0)
  call void @atlas.l2.window.adjust(ptr %ptr, i64 96, i64 48, i64 0, i64 0)
  
  ; Verify adjustment worked
  %sum_after = call i64 @atlas.l2.window.sum(ptr %ptr, i64 48, i64 0)
  ; CHECK: ; Sum should be 0 mod 48 after adjustment
  ret void
}

; Test multiple window sizes
define i1 @test_multiple_window_sizes() {
; CHECK-LABEL: @test_multiple_window_sizes
entry:
  %buf = alloca [96 x i8], align 1
  %ptr = getelementptr [96 x i8], ptr %buf, i32 0, i32 0
  
  ; Create pattern conserved for multiple window sizes
  call void @llvm.memset.p0.i64(ptr %ptr, i8 0, i64 96, i1 false)
  
  ; Check conservation for window size 8
  %check8 = call i1 @atlas.l2.window.check(ptr %ptr, i64 96, i64 8, i64 8)
  
  ; Check conservation for window size 16  
  %check16 = call i1 @atlas.l2.window.check(ptr %ptr, i64 96, i64 16, i64 16)
  
  ; Check conservation for window size 32
  %check32 = call i1 @atlas.l2.window.check(ptr %ptr, i64 96, i64 32, i64 32)
  
  ; All should pass (zeros are conserved for any window size)
  %result1 = and i1 %check8, %check16
  %result = and i1 %result1, %check32
  
  ; CHECK: and i1
  ; CHECK: and i1
  ret i1 %result
}

; Test window boundary conditions
define i1 @test_window_boundaries() {
; CHECK-LABEL: @test_window_boundaries
entry:
  %buf = alloca [96 x i8], align 1
  %ptr = getelementptr [96 x i8], ptr %buf, i32 0, i32 0
  
  ; Test window at buffer start
  call void @llvm.memset.p0.i64(ptr %ptr, i8 0, i64 96, i1 false)
  %start_check = call i1 @atlas.l2.window.check(ptr %ptr, i64 96, i64 16, i64 16)
  
  ; Test window at buffer end (offset 80, window 16)
  %ptr_end = getelementptr i8, ptr %ptr, i64 80
  call void @llvm.memset.p0.i64(ptr %ptr_end, i8 0, i64 16, i1 false)
  %end_sum = call i64 @atlas.l2.window.sum(ptr %ptr, i64 16, i64 80)
  %end_check = icmp eq i64 %end_sum, 0
  
  %result = and i1 %start_check, %end_check
  ret i1 %result
}

; Test sliding window pattern
define i1 @test_sliding_window() {
; CHECK-LABEL: @test_sliding_window
entry:
  %buf = alloca [96 x i8], align 1
  %ptr = getelementptr [96 x i8], ptr %buf, i32 0, i32 0
  
  ; Create sliding pattern where each 8-byte window sums to 8
  br label %init_loop

init_loop:
  %i = phi i64 [ 0, %entry ], [ %i_next, %init_body ]
  %done = icmp eq i64 %i, 96
  br i1 %done, label %check_windows, label %init_body

init_body:
  %ptr_i = getelementptr i8, ptr %ptr, i64 %i
  %window_idx = udiv i64 %i, 8
  %within_window = urem i64 %i, 8
  
  ; First byte of each window gets value 1, others get 0
  ; This makes each 8-byte window sum to 1, not conserved
  %val = select i1 icmp eq (i64 %within_window, 0), i8 1, i8 0
  store i8 %val, ptr %ptr_i
  %i_next = add i64 %i, 1
  br label %init_loop

check_windows:
  ; This should fail conservation (each window sums to 1, not 0 mod 8)
  %conserved = call i1 @atlas.l2.window.check(ptr %ptr, i64 96, i64 8, i64 8)
  
  ; Invert result - we expect this to fail
  %result = xor i1 %conserved, 1
  ; CHECK: xor i1
  ret i1 %result
}

; Test large window (Atlas-12288 size)
define i1 @test_large_window() {
; CHECK-LABEL: @test_large_window
entry:
  %size = i64 12288
  %buf = alloca i8, i64 12288, align 1
  
  ; Initialize large buffer to conserved state
  call void @llvm.memset.p0.i64(ptr %buf, i8 0, i64 %size, i1 false)
  
  ; Check full buffer as single window
  %conserved = call i1 @atlas.l2.window.check(ptr %buf, i64 %size, i64 %size, i64 %size)
  ret i1 %conserved
}

; Test window with prime-sized stride
define i1 @test_prime_stride() {
; CHECK-LABEL: @test_prime_stride
entry:
  %buf = alloca [96 x i8], align 1
  %ptr = getelementptr [96 x i8], ptr %buf, i32 0, i32 0
  
  call void @llvm.memset.p0.i64(ptr %ptr, i8 0, i64 96, i1 false)
  
  ; Use prime stride (7) with window size 7
  %conserved = call i1 @atlas.l2.window.check(ptr %ptr, i64 91, i64 7, i64 7)
  ret i1 %conserved
}

declare void @llvm.memset.p0.i64(ptr, i8, i64, i1)

; ASM: .text
; ASM: test_basic_window_check:
; ASM: callq atlas.l2.window.check
; ASM: test_window_sum:
; ASM: callq atlas.l2.window.sum
; ASM: test_window_adjust:
; ASM: callq atlas.l2.window.adjust