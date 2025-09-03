; RUN: opt -S < %s | FileCheck %s
; RUN: llc -O3 < %s | FileCheck %s --check-prefix=ASM

; witness-ops.ll - Test witness generation and verification
; Tests the Atlas-12288 Layer 2 witness operations for integrity

source_filename = "witness-ops.ll"
target datalayout = "e-m:e-p270:32:32-p271:32:32-p272:64:64-i64:64-i128:128-f80:128-n8:16:32:64-S128"
target triple = "x86_64-unknown-linux-gnu"

; Layer 2 witness operation intrinsics
declare ptr @atlas.l2.witness.generate(ptr, i64, i32)
declare i1 @atlas.l2.witness.verify(ptr, ptr, i64, i32)
declare void @atlas.l2.witness.destroy(ptr)
declare ptr @atlas.l2.witness.update(ptr, ptr, i64, i64, i32)
declare i1 @atlas.l2.witness.compare(ptr, ptr)
declare i64 @atlas.l2.witness.size(i32)

; Test basic witness generation and verification
define i1 @test_basic_witness_ops() {
; CHECK-LABEL: @test_basic_witness_ops
; CHECK: call ptr @atlas.l2.witness.generate
; CHECK: call i1 @atlas.l2.witness.verify
entry:
  %buf = alloca [96 x i8], align 1
  %ptr = getelementptr [96 x i8], ptr %buf, i32 0, i32 0
  
  ; Initialize with known pattern
  br label %fill_loop

fill_loop:
  %i = phi i64 [ 0, %entry ], [ %i_next, %fill_body ]
  %done = icmp eq i64 %i, 96
  br i1 %done, label %generate, label %fill_body

fill_body:
  %ptr_i = getelementptr i8, ptr %ptr, i64 %i
  %val = trunc i64 %i to i8
  store i8 %val, ptr %ptr_i
  %i_next = add i64 %i, 1
  br label %fill_loop

generate:
  ; Generate witness with SHA256 algorithm (type 1)
  %witness = call ptr @atlas.l2.witness.generate(ptr %ptr, i64 96, i32 1)
  
  ; Check witness was created
  %is_null = icmp eq ptr %witness, null
  br i1 %is_null, label %fail, label %verify

verify:
  ; Verify witness against original data
  %valid = call i1 @atlas.l2.witness.verify(ptr %witness, ptr %ptr, i64 96, i32 1)
  br i1 %valid, label %cleanup, label %fail

cleanup:
  call void @atlas.l2.witness.destroy(ptr %witness)
  ret i1 true

fail:
  ret i1 false
}

; Test witness with different hash algorithms
define i1 @test_witness_algorithms() {
; CHECK-LABEL: @test_witness_algorithms
entry:
  %buf = alloca [96 x i8], align 1
  %ptr = getelementptr [96 x i8], ptr %buf, i32 0, i32 0
  
  call void @llvm.memset.p0.i64(ptr %ptr, i8 42, i64 96, i1 false)
  
  ; Test SHA256 (type 1)
  %witness_sha256 = call ptr @atlas.l2.witness.generate(ptr %ptr, i64 96, i32 1)
  %verify_sha256 = call i1 @atlas.l2.witness.verify(ptr %witness_sha256, ptr %ptr, i64 96, i32 1)
  
  ; Test Blake3 (type 2)
  %witness_blake3 = call ptr @atlas.l2.witness.generate(ptr %ptr, i64 96, i32 2)
  %verify_blake3 = call i1 @atlas.l2.witness.verify(ptr %witness_blake3, ptr %ptr, i64 96, i32 2)
  
  ; Test CRC32 (type 3, faster but less secure)
  %witness_crc32 = call ptr @atlas.l2.witness.generate(ptr %ptr, i64 96, i32 3)
  %verify_crc32 = call i1 @atlas.l2.witness.verify(ptr %witness_crc32, ptr %ptr, i64 96, i32 3)
  
  ; All should verify correctly
  %result1 = and i1 %verify_sha256, %verify_blake3
  %result = and i1 %result1, %verify_crc32
  
  ; Cleanup
  call void @atlas.l2.witness.destroy(ptr %witness_sha256)
  call void @atlas.l2.witness.destroy(ptr %witness_blake3)
  call void @atlas.l2.witness.destroy(ptr %witness_crc32)
  
  ; CHECK: and i1
  ret i1 %result
}

; Test witness update mechanism
define i1 @test_witness_update() {
; CHECK-LABEL: @test_witness_update
; CHECK: call ptr @atlas.l2.witness.update
entry:
  %buf = alloca [96 x i8], align 1
  %ptr = getelementptr [96 x i8], ptr %buf, i32 0, i32 0
  
  ; Initialize buffer
  call void @llvm.memset.p0.i64(ptr %ptr, i8 100, i64 96, i1 false)
  
  ; Generate initial witness
  %witness = call ptr @atlas.l2.witness.generate(ptr %ptr, i64 96, i32 1)
  
  ; Modify a portion of the buffer (bytes 32-47)
  %modify_ptr = getelementptr i8, ptr %ptr, i64 32
  call void @llvm.memset.p0.i64(ptr %modify_ptr, i8 200, i64 16, i1 false)
  
  ; Update witness for the modified region
  %updated_witness = call ptr @atlas.l2.witness.update(ptr %witness, ptr %modify_ptr, i64 32, i64 16, i32 1)
  
  ; Verify updated witness against modified buffer
  %valid = call i1 @atlas.l2.witness.verify(ptr %updated_witness, ptr %ptr, i64 96, i32 1)
  
  ; Old witness should no longer verify
  %old_invalid = call i1 @atlas.l2.witness.verify(ptr %witness, ptr %ptr, i64 96, i32 1)
  %old_should_fail = xor i1 %old_invalid, 1
  
  %result = and i1 %valid, %old_should_fail
  
  call void @atlas.l2.witness.destroy(ptr %witness)
  call void @atlas.l2.witness.destroy(ptr %updated_witness)
  ret i1 %result
}

; Test witness comparison
define i1 @test_witness_compare() {
; CHECK-LABEL: @test_witness_compare
; CHECK: call i1 @atlas.l2.witness.compare
entry:
  %buf1 = alloca [96 x i8], align 1
  %buf2 = alloca [96 x i8], align 1
  
  %ptr1 = getelementptr [96 x i8], ptr %buf1, i32 0, i32 0
  %ptr2 = getelementptr [96 x i8], ptr %buf2, i32 0, i32 0
  
  ; Same content in both buffers
  call void @llvm.memset.p0.i64(ptr %ptr1, i8 77, i64 96, i1 false)
  call void @llvm.memset.p0.i64(ptr %ptr2, i8 77, i64 96, i1 false)
  
  ; Generate witnesses
  %witness1 = call ptr @atlas.l2.witness.generate(ptr %ptr1, i64 96, i32 1)
  %witness2 = call ptr @atlas.l2.witness.generate(ptr %ptr2, i64 96, i32 1)
  
  ; Witnesses should be equal
  %equal = call i1 @atlas.l2.witness.compare(ptr %witness1, ptr %witness2)
  
  ; Modify second buffer
  %first_byte = getelementptr i8, ptr %ptr2, i64 0
  store i8 78, ptr %first_byte
  
  ; Generate new witness for modified buffer
  %witness3 = call ptr @atlas.l2.witness.generate(ptr %ptr2, i64 96, i32 1)
  
  ; Witnesses should now be different
  %different = call i1 @atlas.l2.witness.compare(ptr %witness1, ptr %witness3)
  %not_equal = xor i1 %different, 1
  
  %result = and i1 %equal, %not_equal
  
  call void @atlas.l2.witness.destroy(ptr %witness1)
  call void @atlas.l2.witness.destroy(ptr %witness2)
  call void @atlas.l2.witness.destroy(ptr %witness3)
  ret i1 %result
}

; Test witness size calculation
define i64 @test_witness_size() {
; CHECK-LABEL: @test_witness_size
; CHECK: call i64 @atlas.l2.witness.size
entry:
  ; SHA256 witness size (should be 32 bytes + metadata)
  %size_sha256 = call i64 @atlas.l2.witness.size(i32 1)
  
  ; Blake3 witness size (should be 32 bytes + metadata)  
  %size_blake3 = call i64 @atlas.l2.witness.size(i32 2)
  
  ; CRC32 witness size (should be 4 bytes + metadata)
  %size_crc32 = call i64 @atlas.l2.witness.size(i32 3)
  
  ; Return sum for testing (SHA256 + Blake3 should be similar, CRC32 much smaller)
  %sum1 = add i64 %size_sha256, %size_blake3
  %total = add i64 %sum1, %size_crc32
  
  ; CHECK: add i64
  ret i64 %total
}

; Test witness with large buffer (Atlas-12288)
define i1 @test_large_buffer_witness() {
; CHECK-LABEL: @test_large_buffer_witness
entry:
  %size = i64 12288
  %buf = alloca i8, i64 12288, align 1
  
  ; Fill large buffer with pattern
  br label %fill_loop

fill_loop:
  %i = phi i64 [ 0, %entry ], [ %i_next, %fill_body ]
  %done = icmp eq i64 %i, %size
  br i1 %done, label %generate, label %fill_body

fill_body:
  %ptr_i = getelementptr i8, ptr %buf, i64 %i
  %pattern = urem i64 %i, 256
  %val = trunc i64 %pattern to i8
  store i8 %val, ptr %ptr_i
  %i_next = add i64 %i, 1
  br label %fill_loop

generate:
  ; Generate witness for large buffer
  %witness = call ptr @atlas.l2.witness.generate(ptr %buf, i64 %size, i32 1)
  
  ; Verify large buffer witness
  %valid = call i1 @atlas.l2.witness.verify(ptr %witness, ptr %buf, i64 %size, i32 1)
  
  call void @atlas.l2.witness.destroy(ptr %witness)
  ret i1 %valid
}

; Test tamper detection
define i1 @test_tamper_detection() {
; CHECK-LABEL: @test_tamper_detection
entry:
  %buf = alloca [96 x i8], align 1
  %ptr = getelementptr [96 x i8], ptr %buf, i32 0, i32 0
  
  ; Initialize with specific pattern
  call void @llvm.memset.p0.i64(ptr %ptr, i8 55, i64 96, i1 false)
  
  ; Generate witness
  %witness = call ptr @atlas.l2.witness.generate(ptr %ptr, i64 96, i32 1)
  
  ; Verify original data
  %valid_original = call i1 @atlas.l2.witness.verify(ptr %witness, ptr %ptr, i64 96, i32 1)
  
  ; Tamper with single bit in middle of buffer
  %tamper_ptr = getelementptr i8, ptr %ptr, i64 48
  %original_val = load i8, ptr %tamper_ptr
  %tampered_val = xor i8 %original_val, 1  ; Flip LSB
  store i8 %tampered_val, ptr %tamper_ptr
  
  ; Verification should fail
  %valid_tampered = call i1 @atlas.l2.witness.verify(ptr %witness, ptr %ptr, i64 96, i32 1)
  %detected_tamper = xor i1 %valid_tampered, 1
  
  ; Restore original value
  store i8 %original_val, ptr %tamper_ptr
  
  ; Verification should succeed again
  %valid_restored = call i1 @atlas.l2.witness.verify(ptr %witness, ptr %ptr, i64 96, i32 1)
  
  ; All conditions must be met
  %result1 = and i1 %valid_original, %detected_tamper
  %result = and i1 %result1, %valid_restored
  
  call void @atlas.l2.witness.destroy(ptr %witness)
  ; CHECK: and i1
  ret i1 %result
}

; Test witness with empty buffer
define i1 @test_empty_buffer_witness() {
; CHECK-LABEL: @test_empty_buffer_witness
entry:
  %buf = alloca [1 x i8], align 1  ; Minimal allocation
  %ptr = getelementptr [1 x i8], ptr %buf, i32 0, i32 0
  
  ; Generate witness for zero-length data
  %witness = call ptr @atlas.l2.witness.generate(ptr %ptr, i64 0, i32 1)
  
  ; Verify empty buffer witness
  %valid = call i1 @atlas.l2.witness.verify(ptr %witness, ptr %ptr, i64 0, i32 1)
  
  call void @atlas.l2.witness.destroy(ptr %witness)
  ret i1 %valid
}

; Test witness performance characteristics
define i64 @test_witness_performance() {
; CHECK-LABEL: @test_witness_performance
entry:
  %buf = alloca [1024 x i8], align 1
  %ptr = getelementptr [1024 x i8], ptr %buf, i32 0, i32 0
  
  call void @llvm.memset.p0.i64(ptr %ptr, i8 42, i64 1024, i1 false)
  
  ; Time multiple witness generations (conceptually)
  %witness1 = call ptr @atlas.l2.witness.generate(ptr %ptr, i64 1024, i32 1)  ; SHA256
  %witness2 = call ptr @atlas.l2.witness.generate(ptr %ptr, i64 1024, i32 2)  ; Blake3
  %witness3 = call ptr @atlas.l2.witness.generate(ptr %ptr, i64 1024, i32 3)  ; CRC32
  
  ; Verify all witnesses
  %verify1 = call i1 @atlas.l2.witness.verify(ptr %witness1, ptr %ptr, i64 1024, i32 1)
  %verify2 = call i1 @atlas.l2.witness.verify(ptr %witness2, ptr %ptr, i64 1024, i32 2)  
  %verify3 = call i1 @atlas.l2.witness.verify(ptr %witness3, ptr %ptr, i64 1024, i32 3)
  
  ; Count successful operations
  %count1 = select i1 %verify1, i64 1, i64 0
  %count2 = select i1 %verify2, i64 1, i64 0
  %count3 = select i1 %verify3, i64 1, i64 0
  
  %total = add i64 %count1, %count2
  %result = add i64 %total, %count3
  
  call void @atlas.l2.witness.destroy(ptr %witness1)
  call void @atlas.l2.witness.destroy(ptr %witness2)
  call void @atlas.l2.witness.destroy(ptr %witness3)
  
  ; Should return 3 if all operations succeeded
  ret i64 %result
}

declare void @llvm.memset.p0.i64(ptr, i8, i64, i1)

; ASM: .text
; ASM: test_basic_witness_ops:
; ASM: callq atlas.l2.witness.generate
; ASM: callq atlas.l2.witness.verify
; ASM: test_witness_update:
; ASM: callq atlas.l2.witness.update
; ASM: test_witness_compare:
; ASM: callq atlas.l2.witness.compare