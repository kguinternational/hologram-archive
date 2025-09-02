; test-simd.ll - SIMD vector operation tests
; Test the Atlas SIMD intrinsics and vector operations

source_filename = "test-simd.ll"
target triple = "x86_64-unknown-linux-gnu"

declare <16 x i7> @atlas.r96.classify.v16i8(<16 x i8>)
declare <32 x i7> @atlas.r96.classify.v32i8(<32 x i8>)
declare <16 x i16> @atlas.sum.v16i16(<16 x i16>, <16 x i16>)
declare i32 @printf(ptr, ...)

@.str.pass = private unnamed_addr constant [13 x i8] c"Test PASSED\0A\00"
@.str.fail = private unnamed_addr constant [13 x i8] c"Test FAILED\0A\00"

define i32 @main() {
entry:
  ; Test 1: Vector classification
  ; Create a test vector with values 0..15
  %vec = insertelement <16 x i8> undef, i8 0, i32 0
  %vec1 = insertelement <16 x i8> %vec, i8 1, i32 1
  %vec2 = insertelement <16 x i8> %vec1, i8 2, i32 2
  %vec3 = insertelement <16 x i8> %vec2, i8 3, i32 3
  %vec4 = insertelement <16 x i8> %vec3, i8 4, i32 4
  %vec5 = insertelement <16 x i8> %vec4, i8 5, i32 5
  %vec6 = insertelement <16 x i8> %vec5, i8 6, i32 6
  %vec7 = insertelement <16 x i8> %vec6, i8 7, i32 7
  %vec8 = insertelement <16 x i8> %vec7, i8 8, i32 8
  %vec9 = insertelement <16 x i8> %vec8, i8 9, i32 9
  %vec10 = insertelement <16 x i8> %vec9, i8 10, i32 10
  %vec11 = insertelement <16 x i8> %vec10, i8 11, i32 11
  %vec12 = insertelement <16 x i8> %vec11, i8 12, i32 12
  %vec13 = insertelement <16 x i8> %vec12, i8 13, i32 13
  %vec14 = insertelement <16 x i8> %vec13, i8 14, i32 14
  %vec15 = insertelement <16 x i8> %vec14, i8 15, i32 15
  
  ; Classify the vector
  %classified = call <16 x i7> @atlas.r96.classify.v16i8(<16 x i8> %vec15)
  
  ; Check first element is 0 (classify(0) = 0)
  %first = extractelement <16 x i7> %classified, i32 0
  %is_zero = icmp eq i7 %first, 0
  br i1 %is_zero, label %test2, label %fail

test2:
  ; Test 2: Vector sum operation
  %a = insertelement <16 x i16> undef, i16 1, i32 0
  %a1 = insertelement <16 x i16> %a, i16 2, i32 1
  %a2 = insertelement <16 x i16> %a1, i16 3, i32 2
  %a3 = insertelement <16 x i16> %a2, i16 4, i32 3
  %a4 = insertelement <16 x i16> %a3, i16 5, i32 4
  %a5 = insertelement <16 x i16> %a4, i16 6, i32 5
  %a6 = insertelement <16 x i16> %a5, i16 7, i32 6
  %a7 = insertelement <16 x i16> %a6, i16 8, i32 7
  %a8 = insertelement <16 x i16> %a7, i16 9, i32 8
  %a9 = insertelement <16 x i16> %a8, i16 10, i32 9
  %a10 = insertelement <16 x i16> %a9, i16 11, i32 10
  %a11 = insertelement <16 x i16> %a10, i16 12, i32 11
  %a12 = insertelement <16 x i16> %a11, i16 13, i32 12
  %a13 = insertelement <16 x i16> %a12, i16 14, i32 13
  %a14 = insertelement <16 x i16> %a13, i16 15, i32 14
  %a15 = insertelement <16 x i16> %a14, i16 16, i32 15
  
  %b = insertelement <16 x i16> undef, i16 10, i32 0
  %b1 = insertelement <16 x i16> %b, i16 10, i32 1
  %b2 = insertelement <16 x i16> %b1, i16 10, i32 2
  %b3 = insertelement <16 x i16> %b2, i16 10, i32 3
  %b4 = insertelement <16 x i16> %b3, i16 10, i32 4
  %b5 = insertelement <16 x i16> %b4, i16 10, i32 5
  %b6 = insertelement <16 x i16> %b5, i16 10, i32 6
  %b7 = insertelement <16 x i16> %b6, i16 10, i32 7
  %b8 = insertelement <16 x i16> %b7, i16 10, i32 8
  %b9 = insertelement <16 x i16> %b8, i16 10, i32 9
  %b10 = insertelement <16 x i16> %b9, i16 10, i32 10
  %b11 = insertelement <16 x i16> %b10, i16 10, i32 11
  %b12 = insertelement <16 x i16> %b11, i16 10, i32 12
  %b13 = insertelement <16 x i16> %b12, i16 10, i32 13
  %b14 = insertelement <16 x i16> %b13, i16 10, i32 14
  %b15 = insertelement <16 x i16> %b14, i16 10, i32 15
  
  %sum = call <16 x i16> @atlas.sum.v16i16(<16 x i16> %a15, <16 x i16> %b15)
  
  ; Check first element is 11 (1 + 10)
  %sum_first = extractelement <16 x i16> %sum, i32 0
  %is_11 = icmp eq i16 %sum_first, 11
  br i1 %is_11, label %pass, label %fail

pass:
  %msg_pass = getelementptr [13 x i8], ptr @.str.pass, i32 0, i32 0
  call i32 @printf(ptr %msg_pass)
  ret i32 0

fail:
  %msg_fail = getelementptr [13 x i8], ptr @.str.fail, i32 0, i32 0
  call i32 @printf(ptr %msg_fail)
  ret i32 1
}