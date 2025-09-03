; exports.ll - C ABI exports for Layer 0 Atlas Core functions
; Provides C-compatible names for LLVM IR functions

; External declarations of internal functions
declare i1 @atlas.conserved.check(ptr, i64)
declare i32 @atlas.conserved.sum.structure(ptr)
declare i7 @atlas.conserved.delta(ptr, ptr, i64)

; C-compatible exports for conservation functions
define i1 @atlas_conserved_check(ptr %data, i64 %len) {
    %result = call i1 @atlas.conserved.check(ptr %data, i64 %len)
    ret i1 %result
}

define i32 @atlas_conserved_sum(ptr %data, i64 %len) {
    %result = call i32 @atlas.conserved.sum.structure(ptr %data)
    ret i32 %result
}

define i8 @atlas_conserved_delta(ptr %before, ptr %after, i64 %len) {
    %delta_i7 = call i7 @atlas.conserved.delta(ptr %before, ptr %after, i64 %len)
    %delta_i8 = zext i7 %delta_i7 to i8
    ret i8 %delta_i8
}

; Memory alignment function (simple implementation)
define ptr @atlas_alloc_aligned(i64 %size) {
    %ptr = call ptr @malloc(i64 %size)
    ret ptr %ptr
}

; External malloc declaration
declare ptr @malloc(i64)