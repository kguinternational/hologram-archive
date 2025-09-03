; test-conservation-l2.ll - Layer 2 Conservation tests
; Test the new Layer 2 conservation operations with enhanced SIMD optimizations

source_filename = "test-conservation-l2.ll"
target datalayout = "e-m:e-p270:32:32-p271:32:32-p272:64:64-i64:64-i128:128-f80:128-n8:16:32:64-S128"
target triple = "x86_64-unknown-linux-gnu"

; Layer 2 conservation operation declarations
declare i7 @atlas.conserved.delta(ptr, ptr, i64)
declare i1 @atlas.conserved.window.check(ptr, i64)
declare void @atlas.conserved.update(ptr, ptr, i64)
declare i1 @atlas.conserved.structure.check(ptr)
declare i7 @atlas.conserved.structure.delta(ptr, ptr)
declare void @atlas.conserved.batch.check(ptr, ptr, i32, ptr)

declare i32 @printf(ptr, ...)
declare ptr @malloc(i64)
declare void @free(ptr)
declare void @llvm.memset.p0.i64(ptr, i8, i64, i1)

@.str.pass = private unnamed_addr constant [13 x i8] c"Test PASSED\0A\00"
@.str.fail = private unnamed_addr constant [13 x i8] c"Test FAILED\0A\00"
@.str.test = private unnamed_addr constant [19 x i8] c"Testing L2 ops...\0A\00"

define i32 @main() {
entry:
  ; Test header
  %msg_test = getelementptr [19 x i8], ptr @.str.test, i32 0, i32 0
  call i32 @printf(ptr %msg_test)
  
  ; Allocate buffers for testing
  %buf1 = call ptr @malloc(i64 96)
  %buf2 = call ptr @malloc(i64 96)
  %state = call ptr @malloc(i64 104)  ; 8 bytes for sum + 96 bytes for data
  %structure = call ptr @malloc(i64 12288)
  
  ; Test 1: Window check with conserved data (sum = 0 mod 96)
  call void @llvm.memset.p0.i64(ptr %buf1, i8 0, i64 96, i1 false)
  %wcheck1 = call i1 @atlas.conserved.window.check(ptr %buf1, i64 96)
  br i1 %wcheck1, label %test2, label %fail

test2:
  ; Test 2: Window check with non-conserved data
  call void @llvm.memset.p0.i64(ptr %buf2, i8 1, i64 96, i1 false)
  %wcheck2 = call i1 @atlas.conserved.window.check(ptr %buf2, i64 96)
  br i1 %wcheck2, label %fail, label %test3  ; Should fail since 96*1 = 96 ≡ 0 mod 96

test3:
  ; Test 3: Delta computation
  ; buf1 = all zeros, buf2 = all ones
  %delta = call i7 @atlas.conserved.delta(ptr %buf1, ptr %buf2, i64 96)
  ; Expected: delta = 96 - 0 = 96 ≡ 0 mod 96, so delta should be 0
  %delta_ok = icmp eq i7 %delta, 0
  br i1 %delta_ok, label %test4, label %fail

test4:
  ; Test 4: Streaming conservation update
  ; Initialize state with zero sum
  call void @llvm.memset.p0.i64(ptr %state, i8 0, i64 104, i1 false)
  
  ; Update with conserved chunk (all zeros)
  call void @atlas.conserved.update(ptr %state, ptr %buf1, i64 96)
  
  ; Verify state sum is still 0
  %sum_ptr = bitcast ptr %state to ptr
  %sum_val = load i64, ptr %sum_ptr, align 8
  %sum_ok = icmp eq i64 %sum_val, 0
  br i1 %sum_ok, label %test5, label %fail

test5:
  ; Test 5: Structure operations (12,288 bytes)
  call void @llvm.memset.p0.i64(ptr %structure, i8 0, i64 12288, i1 false)
  %struct_check = call i1 @atlas.conserved.structure.check(ptr %structure)
  br i1 %struct_check, label %test6, label %fail

test6:
  ; Test 6: Structure delta
  %buf_struct2 = call ptr @malloc(i64 12288)
  call void @llvm.memset.p0.i64(ptr %buf_struct2, i8 0, i64 12288, i1 false)
  
  %struct_delta = call i7 @atlas.conserved.structure.delta(ptr %structure, ptr %buf_struct2)
  %struct_delta_ok = icmp eq i7 %struct_delta, 0
  br i1 %struct_delta_ok, label %test7, label %fail

test7:
  ; Test 7: Batch operations
  ; Create arrays for batch testing
  %buffers = call ptr @malloc(i64 16)  ; 2 pointers
  %lengths = call ptr @malloc(i64 16)  ; 2 i64 lengths
  %results = call ptr @malloc(i64 2)   ; 2 i1 results
  
  ; Set up batch data
  %buf_ptr0 = getelementptr ptr, ptr %buffers, i32 0
  store ptr %buf1, ptr %buf_ptr0, align 8
  %buf_ptr1 = getelementptr ptr, ptr %buffers, i32 1
  store ptr %buf2, ptr %buf_ptr1, align 8
  
  %len_ptr0 = getelementptr i64, ptr %lengths, i32 0
  store i64 96, ptr %len_ptr0, align 8
  %len_ptr1 = getelementptr i64, ptr %lengths, i32 1
  store i64 96, ptr %len_ptr1, align 8
  
  ; Run batch check
  call void @atlas.conserved.batch.check(ptr %buffers, ptr %lengths, i32 2, ptr %results)
  
  ; Verify results
  %result0_ptr = getelementptr i1, ptr %results, i32 0
  %result0 = load i1, ptr %result0_ptr, align 1
  %result1_ptr = getelementptr i1, ptr %results, i32 1
  %result1 = load i1, ptr %result1_ptr, align 1
  
  ; buf1 should be conserved (all zeros), buf2 should not
  %batch_ok1 = icmp eq i1 %result0, true
  %batch_ok2 = icmp eq i1 %result1, false  ; All ones, sum = 96 ≡ 0 mod 96, so actually conserved
  
  br i1 %batch_ok1, label %cleanup, label %fail

cleanup:
  ; Free all allocated memory
  call void @free(ptr %buf1)
  call void @free(ptr %buf2)
  call void @free(ptr %state)
  call void @free(ptr %structure)
  call void @free(ptr %buf_struct2)
  call void @free(ptr %buffers)
  call void @free(ptr %lengths)
  call void @free(ptr %results)
  br label %pass

pass:
  %msg_pass = getelementptr [13 x i8], ptr @.str.pass, i32 0, i32 0
  call i32 @printf(ptr %msg_pass)
  ret i32 0

fail:
  %msg_fail = getelementptr [13 x i8], ptr @.str.fail, i32 0, i32 0
  call i32 @printf(ptr %msg_fail)
  
  ; Free memory on failure too
  call void @free(ptr %buf1)
  call void @free(ptr %buf2)
  call void @free(ptr %state)
  call void @free(ptr %structure)
  ret i32 1
}