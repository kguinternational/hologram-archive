; test-r96.ll - R96 classification tests
; Test the R96 resonance classification operations

source_filename = "test-r96.ll"
target triple = "x86_64-unknown-linux-gnu"

declare i7 @atlas.r96.classify(i8)
declare <16 x i7> @atlas.r96.classify.v16i8(<16 x i8>)
declare i32 @printf(ptr, ...)

@.str.pass = private unnamed_addr constant [13 x i8] c"Test PASSED\0A\00"
@.str.fail = private unnamed_addr constant [13 x i8] c"Test FAILED\0A\00"
@.str.result = private unnamed_addr constant [27 x i8] c"R96(%d) = %d, expected %d\0A\00"

define i32 @main() {
entry:
  ; Test 1: classify(0) should be 0
  %r0 = call i7 @atlas.r96.classify(i8 0)
  %is0 = icmp eq i7 %r0, 0
  br i1 %is0, label %test2, label %fail

test2:
  ; Test 2: classify(96) should be 0 (wraps)
  %r96 = call i7 @atlas.r96.classify(i8 96)
  %is0_2 = icmp eq i7 %r96, 0
  br i1 %is0_2, label %test3, label %fail

test3:
  ; Test 3: classify(95) should be 95
  %r95 = call i7 @atlas.r96.classify(i8 95)
  %is95 = icmp eq i7 %r95, 95
  br i1 %is95, label %test4, label %fail

test4:
  ; Test 4: classify(192) should be 0 (192 = 2*96)
  %r192 = call i7 @atlas.r96.classify(i8 192)
  %is0_3 = icmp eq i7 %r192, 0
  br i1 %is0_3, label %test5, label %fail

test5:
  ; Test 5: classify(255) should be 63 (255 % 96 = 63)
  %r255 = call i7 @atlas.r96.classify(i8 255)
  %is63 = icmp eq i7 %r255, 63
  br i1 %is63, label %pass, label %fail

pass:
  %msg_pass = getelementptr [13 x i8], ptr @.str.pass, i32 0, i32 0
  call i32 @printf(ptr %msg_pass)
  ret i32 0

fail:
  %msg_fail = getelementptr [13 x i8], ptr @.str.fail, i32 0, i32 0
  call i32 @printf(ptr %msg_fail)
  ret i32 1
}