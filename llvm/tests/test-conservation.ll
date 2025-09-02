; test-conservation.ll - Conservation law tests
; Test conservation checking and preserving operations

source_filename = "test-conservation.ll"
target datalayout = "e-m:e-p270:32:32-p271:32:32-p272:64:64-i64:64-i128:128-f80:128-n8:16:32:64-S128"
target triple = "x86_64-unknown-linux-gnu"

declare i1 @atlas.conserved.check(ptr, i64)
declare void @atlas.conserved.add(ptr, ptr, ptr, i64)
declare void @atlas.memset.conserved(ptr, i8, i64)
declare i32 @printf(ptr, ...)
declare ptr @malloc(i64)
declare void @free(ptr)

@.str.pass = private unnamed_addr constant [13 x i8] c"Test PASSED\0A\00"
@.str.fail = private unnamed_addr constant [13 x i8] c"Test FAILED\0A\00"
@.str.test = private unnamed_addr constant [15 x i8] c"Testing %s...\0A\00"

define i32 @main() {
entry:
  ; Allocate test buffer
  %buf = call ptr @malloc(i64 96)
  
  ; Test 1: Buffer of zeros should be conserved (sum = 0)
  call void @llvm.memset.p0.i64(ptr %buf, i8 0, i64 96, i1 false)
  %check1 = call i1 @atlas.conserved.check(ptr %buf, i64 96)
  br i1 %check1, label %test2, label %fail

test2:
  ; Test 2: Fill with pattern that sums to 0 mod 96
  ; Fill with 0,1,2,...,95 (sum = 4560 = 47*96 + 48, so mod 96 = 48)
  ; Need to adjust last element to make it conserved
  br label %fill_loop

fill_loop:
  %i = phi i64 [ 0, %test2 ], [ %i.next, %fill_body ]
  %done = icmp eq i64 %i, 96
  br i1 %done, label %check2, label %fill_body

fill_body:
  %ptr = getelementptr i8, ptr %buf, i64 %i
  %val = trunc i64 %i to i8
  store i8 %val, ptr %ptr
  %i.next = add i64 %i, 1
  br label %fill_loop

check2:
  ; Adjust last byte to make sum = 0 mod 96
  %last_ptr = getelementptr i8, ptr %buf, i64 95
  store i8 48, ptr %last_ptr  ; This makes sum divisible by 96
  
  %check2_result = call i1 @atlas.conserved.check(ptr %buf, i64 96)
  br i1 %check2_result, label %pass, label %fail

pass:
  %msg_pass = getelementptr [13 x i8], ptr @.str.pass, i32 0, i32 0
  call i32 @printf(ptr %msg_pass)
  call void @free(ptr %buf)
  ret i32 0

fail:
  %msg_fail = getelementptr [13 x i8], ptr @.str.fail, i32 0, i32 0
  call i32 @printf(ptr %msg_fail)
  call void @free(ptr %buf)
  ret i32 1
}

declare void @llvm.memset.p0.i64(ptr, i8, i64, i1)