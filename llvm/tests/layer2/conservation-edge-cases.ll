; RUN: opt -S < %s | FileCheck %s
; RUN: llc -O3 < %s | FileCheck %s --check-prefix=ASM
; RUN: lli %s | FileCheck %s --check-prefix=EXEC

; conservation-edge-cases.ll - Edge cases (empty, single byte, overflow)
; Tests the Atlas-12288 Layer 2 conservation edge cases and boundary conditions

source_filename = "conservation-edge-cases.ll"
target datalayout = "e-m:e-p270:32:32-p271:32:32-p272:64:64-i64:64-i128:128-f80:128-n8:16:32:64-S128"
target triple = "x86_64-unknown-linux-gnu"

; Layer 2 conservation intrinsics
declare i1 @atlas.l2.conserved.check(ptr, i64)
declare i64 @atlas.l2.conserved.sum(ptr, i64)
declare void @atlas.l2.conserved.adjust(ptr, i64, i64)
declare i1 @atlas.l2.conserved.overflow_check(ptr, i64)
declare i64 @atlas.l2.conserved.modulus(i64, i64)

; Utility functions
declare i32 @printf(ptr, ...)
declare ptr @malloc(i64)
declare void @free(ptr)

@.str.pass = private unnamed_addr constant [6 x i8] c"PASS\0A\00"
@.str.fail = private unnamed_addr constant [6 x i8] c"FAIL\0A\00"
@.str.test = private unnamed_addr constant [10 x i8] c"Test: %s\0A\00"

; Test empty buffer edge case
define i1 @test_empty_buffer() {
; CHECK-LABEL: @test_empty_buffer
; CHECK: call i1 @atlas.l2.conserved.check
entry:
  %dummy = alloca i8, align 1  ; Need some allocation for pointer
  
  ; Empty buffer should be considered conserved (vacuous truth)
  %conserved = call i1 @atlas.l2.conserved.check(ptr %dummy, i64 0)
  
  ; Sum of empty buffer should be 0
  %sum = call i64 @atlas.l2.conserved.sum(ptr %dummy, i64 0)
  %sum_zero = icmp eq i64 %sum, 0
  
  %result = and i1 %conserved, %sum_zero
  ; CHECK: and i1
  ret i1 %result
}

; Test single byte buffer
define i1 @test_single_byte() {
; CHECK-LABEL: @test_single_byte
entry:
  %buf = alloca i8, align 1
  
  ; Single byte with value 0 (conserved: 0 mod 1 = 0)
  store i8 0, ptr %buf
  %conserved_zero = call i1 @atlas.l2.conserved.check(ptr %buf, i64 1)
  
  ; Single byte with non-zero value (also conserved: any value mod 1 = 0)
  store i8 255, ptr %buf
  %conserved_max = call i1 @atlas.l2.conserved.check(ptr %buf, i64 1)
  
  ; Both should be conserved (any value mod 1 = 0)
  %result = and i1 %conserved_zero, %conserved_max
  ret i1 %result
}

; Test overflow boundary conditions
define i1 @test_overflow_boundaries() {
; CHECK-LABEL: @test_overflow_boundaries
; CHECK: call i1 @atlas.l2.conserved.overflow_check
entry:
  %buf = alloca [256 x i8], align 1
  %ptr = getelementptr [256 x i8], ptr %buf, i32 0, i32 0
  
  ; Fill buffer with maximum values (255 each)
  call void @llvm.memset.p0.i64(ptr %ptr, i8 255, i64 256, i1 false)
  
  ; Check for potential overflow (255 * 256 = 65280, within i64 range)
  %no_overflow = call i1 @atlas.l2.conserved.overflow_check(ptr %ptr, i64 256)
  
  ; Compute actual sum
  %sum = call i64 @atlas.l2.conserved.sum(ptr %ptr, i64 256)
  %expected_sum = icmp eq i64 %sum, 65280
  
  %result = and i1 %no_overflow, %expected_sum
  ; CHECK: and i1
  ret i1 %result
}

; Test maximum buffer size (Atlas-12288)
define i1 @test_maximum_buffer() {
; CHECK-LABEL: @test_maximum_buffer
entry:
  %max_size = i64 12288
  %buf = alloca i8, i64 12288, align 1
  
  ; Initialize with pattern that won't overflow
  ; Use value 1 so sum = 12288, well within i64 range
  call void @llvm.memset.p0.i64(ptr %buf, i8 1, i64 %max_size, i1 false)
  
  ; Check overflow safety
  %no_overflow = call i1 @atlas.l2.conserved.overflow_check(ptr %buf, i64 %max_size)
  
  ; Check if conserved (12288 mod 12288 = 0, so it is conserved)
  %conserved = call i1 @atlas.l2.conserved.check(ptr %buf, i64 %max_size)
  
  %result = and i1 %no_overflow, %conserved
  ret i1 %result
}

; Test modulus edge cases
define i1 @test_modulus_edge_cases() {
; CHECK-LABEL: @test_modulus_edge_cases
; CHECK: call i64 @atlas.l2.conserved.modulus
entry:
  ; Test modulus with various edge values
  
  ; 0 mod anything = 0
  %mod1 = call i64 @atlas.l2.conserved.modulus(i64 0, i64 96)
  %zero_mod = icmp eq i64 %mod1, 0
  
  ; Large number mod small number
  %mod2 = call i64 @atlas.l2.conserved.modulus(i64 1000000, i64 96)
  %large_mod = icmp ult i64 %mod2, 96  ; Should be < 96
  
  ; Number mod 1 = 0 (anything mod 1 is 0)
  %mod3 = call i64 @atlas.l2.conserved.modulus(i64 12345, i64 1)
  %one_mod = icmp eq i64 %mod3, 0
  
  ; Power of 2 modulus (common case)
  %mod4 = call i64 @atlas.l2.conserved.modulus(i64 1023, i64 256)
  %pow2_mod = icmp eq i64 %mod4, 255  ; 1023 mod 256 = 255
  
  %result1 = and i1 %zero_mod, %large_mod
  %result2 = and i1 %one_mod, %pow2_mod
  %result = and i1 %result1, %result2
  
  ; CHECK: and i1
  ret i1 %result
}

; Test non-standard buffer sizes (primes, etc.)
define i1 @test_nonstandard_sizes() {
; CHECK-LABEL: @test_nonstandard_sizes
entry:
  ; Test prime-sized buffer (97 bytes)
  %buf_prime = alloca [97 x i8], align 1
  %ptr_prime = getelementptr [97 x i8], ptr %buf_prime, i32 0, i32 0
  
  call void @llvm.memset.p0.i64(ptr %ptr_prime, i8 0, i64 97, i1 false)
  %prime_conserved = call i1 @atlas.l2.conserved.check(ptr %ptr_prime, i64 97)
  
  ; Test odd-sized buffer (33 bytes)
  %buf_odd = alloca [33 x i8], align 1
  %ptr_odd = getelementptr [33 x i8], ptr %buf_odd, i32 0, i32 0
  
  ; Fill with pattern that sums to multiple of 33
  call void @llvm.memset.p0.i64(ptr %ptr_odd, i8 33, i64 33, i1 false)  ; Sum = 33*33 = 1089 = 33*33
  %odd_conserved = call i1 @atlas.l2.conserved.check(ptr %ptr_odd, i64 33)
  
  ; Test power-of-2 size (128 bytes)
  %buf_pow2 = alloca [128 x i8], align 1
  %ptr_pow2 = getelementptr [128 x i8], ptr %buf_pow2, i32 0, i32 0
  
  call void @llvm.memset.p0.i64(ptr %ptr_pow2, i8 2, i64 128, i1 false)  ; Sum = 256 = 2*128
  %pow2_conserved = call i1 @atlas.l2.conserved.check(ptr %ptr_pow2, i64 128)
  
  %result1 = and i1 %prime_conserved, %odd_conserved
  %result = and i1 %result1, %pow2_conserved
  
  ret i1 %result
}

; Test conservation adjustment edge cases
define i1 @test_adjustment_edge_cases() {
; CHECK-LABEL: @test_adjustment_edge_cases
; CHECK: call void @atlas.l2.conserved.adjust
entry:
  %buf = alloca [96 x i8], align 1
  %ptr = getelementptr [96 x i8], ptr %buf, i32 0, i32 0
  
  ; Start with non-conserved buffer (all 1s, sum = 96)
  call void @llvm.memset.p0.i64(ptr %ptr, i8 1, i64 96, i1 false)
  
  ; Adjust to target sum of 0 (conserved)
  call void @atlas.l2.conserved.adjust(ptr %ptr, i64 96, i64 0)
  
  ; Verify adjustment worked
  %conserved_after = call i1 @atlas.l2.conserved.check(ptr %ptr, i64 96)
  
  ; Test adjustment to specific non-zero target (multiple of size)
  call void @atlas.l2.conserved.adjust(ptr %ptr, i64 96, i64 192)  ; 2*96
  %sum_after = call i64 @atlas.l2.conserved.sum(ptr %ptr, i64 96)
  %target_reached = icmp eq i64 %sum_after, 192
  
  ; Verify still conserved (192 mod 96 = 0)
  %still_conserved = call i1 @atlas.l2.conserved.check(ptr %ptr, i64 96)
  
  %result1 = and i1 %conserved_after, %target_reached
  %result = and i1 %result1, %still_conserved
  
  ret i1 %result
}

; Test integer wraparound edge cases
define i1 @test_integer_wraparound() {
; CHECK-LABEL: @test_integer_wraparound
entry:
  %buf = alloca [256 x i8], align 1
  %ptr = getelementptr [256 x i8], ptr %buf, i32 0, i32 0
  
  ; Fill with 0,1,2,...,255 pattern (uses all byte values)
  br label %fill_loop

fill_loop:
  %i = phi i64 [ 0, %entry ], [ %i_next, %fill_body ]
  %done = icmp eq i64 %i, 256
  br i1 %done, label %test, label %fill_body

fill_body:
  %ptr_i = getelementptr i8, ptr %ptr, i64 %i
  %val = trunc i64 %i to i8
  store i8 %val, ptr %ptr_i
  %i_next = add i64 %i, 1
  br label %fill_loop

test:
  ; Sum should be 0+1+...+255 = 32640
  %sum = call i64 @atlas.l2.conserved.sum(ptr %ptr, i64 256)
  %expected_sum = icmp eq i64 %sum, 32640
  
  ; Check modulus: 32640 mod 256 = 128 (not conserved)
  %mod_result = call i64 @atlas.l2.conserved.modulus(i64 32640, i64 256)
  %not_conserved = icmp ne i64 %mod_result, 0
  
  ; Test overflow safety even with this pattern
  %no_overflow = call i1 @atlas.l2.conserved.overflow_check(ptr %ptr, i64 256)
  
  %result1 = and i1 %expected_sum, %not_conserved
  %result = and i1 %result1, %no_overflow
  
  ret i1 %result
}

; Test unaligned access edge cases
define i1 @test_unaligned_access() {
; CHECK-LABEL: @test_unaligned_access
entry:
  %buf = alloca [100 x i8], align 1  ; Intentionally not power-of-2 aligned
  %ptr = getelementptr [100 x i8], ptr %buf, i32 0, i32 0
  
  ; Test unaligned starting address (offset by 3 bytes)
  %unaligned_ptr = getelementptr i8, ptr %ptr, i64 3
  
  ; Initialize the unaligned region
  call void @llvm.memset.p0.i64(ptr %unaligned_ptr, i8 0, i64 64, i1 false)
  
  ; Check conservation on unaligned access
  %conserved = call i1 @atlas.l2.conserved.check(ptr %unaligned_ptr, i64 64)
  
  ; Test sum computation on unaligned access
  %sum = call i64 @atlas.l2.conserved.sum(ptr %unaligned_ptr, i64 64)
  %sum_zero = icmp eq i64 %sum, 0
  
  %result = and i1 %conserved, %sum_zero
  ret i1 %result
}

; Test conservation with all possible byte values
define i1 @test_all_byte_values() {
; CHECK-LABEL: @test_all_byte_values
entry:
  ; Create buffer with one instance of each possible byte value (0-255)
  %buf = alloca [256 x i8], align 1
  %ptr = getelementptr [256 x i8], ptr %buf, i32 0, i32 0
  
  br label %fill_unique

fill_unique:
  %i = phi i64 [ 0, %entry ], [ %i_next, %fill_unique_body ]
  %done = icmp eq i64 %i, 256
  br i1 %done, label %test_unique, label %fill_unique_body

fill_unique_body:
  %ptr_i = getelementptr i8, ptr %ptr, i64 %i
  %unique_val = trunc i64 %i to i8
  store i8 %unique_val, ptr %ptr_i
  %i_next = add i64 %i, 1
  br label %fill_unique

test_unique:
  ; Sum of 0+1+...+255 = 32640
  %sum_unique = call i64 @atlas.l2.conserved.sum(ptr %ptr, i64 256)
  
  ; Check if this is conserved: 32640 mod 256 = 128 ≠ 0 (not conserved)
  %not_conserved = call i1 @atlas.l2.conserved.check(ptr %ptr, i64 256)
  %should_fail = xor i1 %not_conserved, 1  ; Expect false, so invert to get true
  
  ; Test duplicate values pattern (all same)
  call void @llvm.memset.p0.i64(ptr %ptr, i8 128, i64 256, i1 false)
  ; Sum = 128 * 256 = 32768, 32768 mod 256 = 0 (conserved)
  %all_same_conserved = call i1 @atlas.l2.conserved.check(ptr %ptr, i64 256)
  
  %result = and i1 %should_fail, %all_same_conserved
  ret i1 %result
}

; Test performance on edge case sizes
define i64 @test_performance_edge_cases() {
; CHECK-LABEL: @test_performance_edge_cases
entry:
  %count = i64 0
  
  ; Test very small buffers (1-10 bytes)
  %buf_small = alloca [10 x i8], align 1
  %ptr_small = getelementptr [10 x i8], ptr %buf_small, i32 0, i32 0
  call void @llvm.memset.p0.i64(ptr %ptr_small, i8 0, i64 10, i1 false)
  
  br label %small_loop

small_loop:
  %size = phi i64 [ 1, %entry ], [ %size_next, %small_test ]
  %small_done = icmp ugt i64 %size, 10
  br i1 %small_done, label %large_test, label %small_test

small_test:
  %conserved_small = call i1 @atlas.l2.conserved.check(ptr %ptr_small, i64 %size)
  %increment = select i1 %conserved_small, i64 1, i64 0
  %count_updated = add i64 %count, %increment
  %size_next = add i64 %size, 1
  br label %small_loop

large_test:
  ; Test largest possible buffer
  %buf_large = alloca i8, i64 12288, align 1
  call void @llvm.memset.p0.i64(ptr %buf_large, i8 0, i64 12288, i1 false)
  
  %conserved_large = call i1 @atlas.l2.conserved.check(ptr %buf_large, i64 12288)
  %final_increment = select i1 %conserved_large, i64 1, i64 0
  %final_count = add i64 %count_updated, %final_increment
  
  ; Should return 11 if all tests passed (10 small + 1 large)
  ; CHECK: ; Expected return: 11
  ret i64 %final_count
}

; Main test runner for execution verification
define i32 @main() {
; EXEC-LABEL: main
entry:
  ; Test empty buffer
  %test1 = call i1 @test_empty_buffer()
  %msg1 = select i1 %test1, ptr @.str.pass, ptr @.str.fail
  call i32 @printf(ptr %msg1)
  
  ; Test single byte
  %test2 = call i1 @test_single_byte()
  %msg2 = select i1 %test2, ptr @.str.pass, ptr @.str.fail
  call i32 @printf(ptr %msg2)
  
  ; Test overflow boundaries
  %test3 = call i1 @test_overflow_boundaries()
  %msg3 = select i1 %test3, ptr @.str.pass, ptr @.str.fail
  call i32 @printf(ptr %msg3)
  
  ; Count passed tests
  %passed1 = select i1 %test1, i32 1, i32 0
  %passed2 = select i1 %test2, i32 1, i32 0
  %passed3 = select i1 %test3, i32 1, i32 0
  
  %total_passed = add i32 %passed1, %passed2
  %final_passed = add i32 %total_passed, %passed3
  
  ; Return 0 if all passed, non-zero otherwise
  %success = icmp eq i32 %final_passed, 3
  %result = select i1 %success, i32 0, i32 1
  
  ; EXEC: PASS
  ; EXEC: PASS  
  ; EXEC: PASS
  ret i32 %result
}

declare void @llvm.memset.p0.i64(ptr, i8, i64, i1)

; ASM: .text
; ASM: test_empty_buffer:
; ASM: callq atlas.l2.conserved.check
; ASM: test_overflow_boundaries:
; ASM: callq atlas.l2.conserved.overflow_check
; ASM: test_modulus_edge_cases:
; ASM: callq atlas.l2.conserved.modulus