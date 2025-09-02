; test-l2-focused.ll - Focused Layer 2 Conservation tests
; Test only the specific Layer 2 operations implemented

source_filename = "test-l2-focused.ll"
target datalayout = "e-m:e-p270:32:32-p271:32:32-p272:64:64-i64:64-i128:128-f80:128-n8:16:32:64-S128"
target triple = "x86_64-unknown-linux-gnu"

; Only the specific operations we want to test
declare i7 @atlas.conserved.delta(ptr, ptr, i64)
declare i1 @atlas.conserved.window.check(ptr, i64)

declare i32 @printf(ptr, ...)
declare ptr @malloc(i64)
declare void @free(ptr)
declare void @llvm.memset.p0.i64(ptr, i8, i64, i1)

@.str.pass = private unnamed_addr constant [13 x i8] c"Test PASSED\0A\00"
@.str.fail = private unnamed_addr constant [13 x i8] c"Test FAILED\0A\00"

define i32 @main() {
entry:
  ; Allocate test buffers
  %buf1 = call ptr @malloc(i64 96)
  %buf2 = call ptr @malloc(i64 96)
  
  ; Test 1: Window check with zeros (should be conserved)
  call void @llvm.memset.p0.i64(ptr %buf1, i8 0, i64 96, i1 false)
  %check1 = call i1 @atlas.conserved.window.check(ptr %buf1, i64 96)
  br i1 %check1, label %test2, label %fail

test2:
  ; Test 2: Delta between identical buffers (should be 0)
  call void @llvm.memset.p0.i64(ptr %buf2, i8 0, i64 96, i1 false)
  %delta = call i7 @atlas.conserved.delta(ptr %buf1, ptr %buf2, i64 96)
  %delta_ok = icmp eq i7 %delta, 0
  br i1 %delta_ok, label %pass, label %fail

pass:
  %msg_pass = getelementptr [13 x i8], ptr @.str.pass, i32 0, i32 0
  call i32 @printf(ptr %msg_pass)
  call void @free(ptr %buf1)
  call void @free(ptr %buf2)
  ret i32 0

fail:
  %msg_fail = getelementptr [13 x i8], ptr @.str.fail, i32 0, i32 0
  call i32 @printf(ptr %msg_fail)
  call void @free(ptr %buf1)
  call void @free(ptr %buf2)
  ret i32 1
}