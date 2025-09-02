; test-witness.ll - Witness generation and verification tests

source_filename = "test-witness.ll"
target datalayout = "e-m:e-p270:32:32-p271:32:32-p272:64:64-i64:64-i128:128-f80:128-n8:16:32:64-S128"
target triple = "x86_64-unknown-linux-gnu"

declare ptr @atlas.witness.generate(ptr, i64)
declare i1 @atlas.witness.verify(ptr, ptr, i64)
declare void @atlas.witness.destroy(ptr)
declare i32 @printf(ptr, ...)
declare ptr @malloc(i64)
declare void @free(ptr)

@.str.pass = private unnamed_addr constant [13 x i8] c"Test PASSED\0A\00"
@.str.fail = private unnamed_addr constant [13 x i8] c"Test FAILED\0A\00"

define i32 @main() {
entry:
  ; Create test data
  %data = call ptr @malloc(i64 256)
  
  ; Fill with test pattern
  br label %fill_loop

fill_loop:
  %i = phi i64 [ 0, %entry ], [ %i.next, %fill_body ]
  %done = icmp eq i64 %i, 256
  br i1 %done, label %generate, label %fill_body

fill_body:
  %ptr = getelementptr i8, ptr %data, i64 %i
  %val = trunc i64 %i to i8
  store i8 %val, ptr %ptr
  %i.next = add i64 %i, 1
  br label %fill_loop

generate:
  ; Generate witness
  %witness = call ptr @atlas.witness.generate(ptr %data, i64 256)
  %is_null = icmp eq ptr %witness, null
  br i1 %is_null, label %fail, label %verify

verify:
  ; Verify witness against original data
  %valid = call i1 @atlas.witness.verify(ptr %witness, ptr %data, i64 256)
  br i1 %valid, label %modify, label %fail

modify:
  ; Modify data and verify witness should fail
  %byte_ptr = getelementptr i8, ptr %data, i64 0
  store i8 42, ptr %byte_ptr
  
  %invalid = call i1 @atlas.witness.verify(ptr %witness, ptr %data, i64 256)
  %should_fail = xor i1 %invalid, 1  ; Should be false
  br i1 %should_fail, label %pass, label %fail

pass:
  %msg_pass = getelementptr [13 x i8], ptr @.str.pass, i32 0, i32 0
  call i32 @printf(ptr %msg_pass)
  call void @atlas.witness.destroy(ptr %witness)
  call void @free(ptr %data)
  ret i32 0

fail:
  %msg_fail = getelementptr [13 x i8], ptr @.str.fail, i32 0, i32 0
  call i32 @printf(ptr %msg_fail)
  call void @free(ptr %data)
  ret i32 1
}