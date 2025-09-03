; exports.ll - C ABI exports for Layer 3 Resonance functions
; Provides C-compatible names for LLVM IR functions

; External declarations of internal functions
declare i7 @atlas.r96.classify(i8)
declare void @atlas.r96.histogram.page(ptr, ptr)
declare i7 @atlas.r96.dominant(ptr)
declare i1 @atlas.r96.harmonizes(i7, i7)




; C-compatible exports
define i8 @atlas_r96_classify_llvm(i8 %byte) {
    %class_i7 = call i7 @atlas.r96.classify(i8 %byte)
    %class = zext i7 %class_i7 to i8
    ret i8 %class
}

define void @atlas_r96_histogram_page_llvm(ptr %page, ptr %histogram) {
    call void @atlas.r96.histogram.page(ptr %page, ptr %histogram)
    ret void
}

define i8 @atlas_page_resonance_class_llvm(ptr %page) {
    %class_i7 = call i7 @atlas.r96.dominant(ptr %page)
    %class = zext i7 %class_i7 to i8
    ret i8 %class
}

define i1 @atlas_resonance_harmonizes_llvm(i8 %r1, i8 %r2) {
    %r1_i7 = trunc i8 %r1 to i7
    %r2_i7 = trunc i8 %r2 to i7
    %result = call i1 @atlas.r96.harmonizes(i7 %r1_i7, i7 %r2_i7)
    ret i1 %result
}

; atlas_next_harmonic_window_from_llvm - computed in C runtime
; This function doesn't have an LLVM implementation

define void @atlas_r96_classify_page_llvm(ptr %in, ptr %out) {
    call void @atlas.r96.classify.page.ptr(ptr %in, ptr %out)
    ret void
}

declare void @atlas.r96.classify.page.ptr(ptr, ptr)


