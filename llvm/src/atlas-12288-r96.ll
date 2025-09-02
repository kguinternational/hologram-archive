; atlas-12288-r96.ll — R96 classification table + implementations (LLVM 15+, opaque pointers)
; ---------------------------------------------------------------------------------
; Notes
;  • Uses opaque pointers (`ptr`) throughout.
;  • Matches the intrinsics in atlas-12288-intrinsics.ll:
;      - atlas.r96.classify(i8) -> i7                (pure, modulo form)
;      - atlas.r96.classify.v16i8(<16 x i8>) -> <16 x i7>
;      - atlas.r96.classify.v32i8(<32 x i8>) -> <32 x i7>
;      - atlas.r96.classify.page(<256 x i8>) -> <256 x i7>
;  • A fast table-based scalar helper (@atlas.r96.classify.impl) is provided
;    for internal use when attributes don’t require readnone.
; ---------------------------------------------------------------------------------

source_filename = "atlas-12288-r96.ll"

; =============================================================================
; R96 Classification Table (period-96 mapping over 0..255)
; =============================================================================

@atlas.r96.table = internal constant [256 x i7] [
  ; 0..95
  i7 0,  i7 1,  i7 2,  i7 3,  i7 4,  i7 5,  i7 6,  i7 7,
  i7 8,  i7 9,  i7 10, i7 11, i7 12, i7 13, i7 14, i7 15,
  i7 16, i7 17, i7 18, i7 19, i7 20, i7 21, i7 22, i7 23,
  i7 24, i7 25, i7 26, i7 27, i7 28, i7 29, i7 30, i7 31,
  i7 32, i7 33, i7 34, i7 35, i7 36, i7 37, i7 38, i7 39,
  i7 40, i7 41, i7 42, i7 43, i7 44, i7 45, i7 46, i7 47,
  i7 48, i7 49, i7 50, i7 51, i7 52, i7 53, i7 54, i7 55,
  i7 56, i7 57, i7 58, i7 59, i7 60, i7 61, i7 62, i7 63,
  i7 64, i7 65, i7 66, i7 67, i7 68, i7 69, i7 70, i7 71,
  i7 72, i7 73, i7 74, i7 75, i7 76, i7 77, i7 78, i7 79,
  i7 80, i7 81, i7 82, i7 83, i7 84, i7 85, i7 86, i7 87,
  i7 88, i7 89, i7 90, i7 91, i7 92, i7 93, i7 94, i7 95,

  ; 96..191 (repeat)
  i7 0,  i7 1,  i7 2,  i7 3,  i7 4,  i7 5,  i7 6,  i7 7,
  i7 8,  i7 9,  i7 10, i7 11, i7 12, i7 13, i7 14, i7 15,
  i7 16, i7 17, i7 18, i7 19, i7 20, i7 21, i7 22, i7 23,
  i7 24, i7 25, i7 26, i7 27, i7 28, i7 29, i7 30, i7 31,
  i7 32, i7 33, i7 34, i7 35, i7 36, i7 37, i7 38, i7 39,
  i7 40, i7 41, i7 42, i7 43, i7 44, i7 45, i7 46, i7 47,
  i7 48, i7 49, i7 50, i7 51, i7 52, i7 53, i7 54, i7 55,
  i7 56, i7 57, i7 58, i7 59, i7 60, i7 61, i7 62, i7 63,
  i7 64, i7 65, i7 66, i7 67, i7 68, i7 69, i7 70, i7 71,
  i7 72, i7 73, i7 74, i7 75, i7 76, i7 77, i7 78, i7 79,
  i7 80, i7 81, i7 82, i7 83, i7 84, i7 85, i7 86, i7 87,
  i7 88, i7 89, i7 90, i7 91, i7 92, i7 93, i7 94, i7 95,

  ; 192..255 (partial repeat 64 bytes)
  i7 0,  i7 1,  i7 2,  i7 3,  i7 4,  i7 5,  i7 6,  i7 7,
  i7 8,  i7 9,  i7 10, i7 11, i7 12, i7 13, i7 14, i7 15,
  i7 16, i7 17, i7 18, i7 19, i7 20, i7 21, i7 22, i7 23,
  i7 24, i7 25, i7 26, i7 27, i7 28, i7 29, i7 30, i7 31,
  i7 32, i7 33, i7 34, i7 35, i7 36, i7 37, i7 38, i7 39,
  i7 40, i7 41, i7 42, i7 43, i7 44, i7 45, i7 46, i7 47,
  i7 48, i7 49, i7 50, i7 51, i7 52, i7 53, i7 54, i7 55,
  i7 56, i7 57, i7 58, i7 59, i7 60, i7 61, i7 62, i7 63
], align 64

; =============================================================================
; Scalar classification
; =============================================================================

; Pure modulo form (no memory reads) — matches #0 attributes in intrinsics.
define i7 @atlas.r96.classify(i8 %byte) nounwind readnone willreturn {
entry:
  %ext = zext i8 %byte to i16
  %mod = urem i16 %ext, 96
  %cls = trunc i16 %mod to i7
  ret i7 %cls
}

; Table-based helper (reads constant memory); not exposed in intrinsics.
define i7 @atlas.r96.classify.impl(i8 %byte) nounwind readonly willreturn {
entry:
  %idx32 = zext i8 %byte to i32
  %ptr   = getelementptr [256 x i7], ptr @atlas.r96.table, i32 0, i32 %idx32
  %class = load i7, ptr %ptr, align 1, !invariant.load !0
  ret i7 %class
}

; Alternate modulo form — kept for A/B and inlining experiments.
define i7 @atlas.r96.classify.mod(i8 %byte) nounwind readnone willreturn {
entry:
  %ext = zext i8 %byte to i16
  %mod = urem i16 %ext, 96
  %cls = trunc i16 %mod to i7
  ret i7 %cls
}

; =============================================================================
; Vector classification
; =============================================================================

; 16-byte block
define <16 x i7> @atlas.r96.classify.v16i8(<16 x i8> %bytes) nounwind readnone willreturn {
entry:
  %e0  = extractelement <16 x i8> %bytes, i32 0
  %e1  = extractelement <16 x i8> %bytes, i32 1
  %e2  = extractelement <16 x i8> %bytes, i32 2
  %e3  = extractelement <16 x i8> %bytes, i32 3
  %e4  = extractelement <16 x i8> %bytes, i32 4
  %e5  = extractelement <16 x i8> %bytes, i32 5
  %e6  = extractelement <16 x i8> %bytes, i32 6
  %e7  = extractelement <16 x i8> %bytes, i32 7
  %e8  = extractelement <16 x i8> %bytes, i32 8
  %e9  = extractelement <16 x i8> %bytes, i32 9
  %e10 = extractelement <16 x i8> %bytes, i32 10
  %e11 = extractelement <16 x i8> %bytes, i32 11
  %e12 = extractelement <16 x i8> %bytes, i32 12
  %e13 = extractelement <16 x i8> %bytes, i32 13
  %e14 = extractelement <16 x i8> %bytes, i32 14
  %e15 = extractelement <16 x i8> %bytes, i32 15
  %c0  = call i7 @atlas.r96.classify(i8 %e0)
  %c1  = call i7 @atlas.r96.classify(i8 %e1)
  %c2  = call i7 @atlas.r96.classify(i8 %e2)
  %c3  = call i7 @atlas.r96.classify(i8 %e3)
  %c4  = call i7 @atlas.r96.classify(i8 %e4)
  %c5  = call i7 @atlas.r96.classify(i8 %e5)
  %c6  = call i7 @atlas.r96.classify(i8 %e6)
  %c7  = call i7 @atlas.r96.classify(i8 %e7)
  %c8  = call i7 @atlas.r96.classify(i8 %e8)
  %c9  = call i7 @atlas.r96.classify(i8 %e9)
  %c10 = call i7 @atlas.r96.classify(i8 %e10)
  %c11 = call i7 @atlas.r96.classify(i8 %e11)
  %c12 = call i7 @atlas.r96.classify(i8 %e12)
  %c13 = call i7 @atlas.r96.classify(i8 %e13)
  %c14 = call i7 @atlas.r96.classify(i8 %e14)
  %c15 = call i7 @atlas.r96.classify(i8 %e15)

  %v0  = insertelement <16 x i7> undef, i7 %c0,  i32 0
  %v1  = insertelement <16 x i7> %v0,   i7 %c1,  i32 1
  %v2  = insertelement <16 x i7> %v1,   i7 %c2,  i32 2
  %v3  = insertelement <16 x i7> %v2,   i7 %c3,  i32 3
  %v4  = insertelement <16 x i7> %v3,   i7 %c4,  i32 4
  %v5  = insertelement <16 x i7> %v4,   i7 %c5,  i32 5
  %v6  = insertelement <16 x i7> %v5,   i7 %c6,  i32 6
  %v7  = insertelement <16 x i7> %v6,   i7 %c7,  i32 7
  %v8  = insertelement <16 x i7> %v7,   i7 %c8,  i32 8
  %v9  = insertelement <16 x i7> %v8,   i7 %c9,  i32 9
  %v10 = insertelement <16 x i7> %v9,   i7 %c10, i32 10
  %v11 = insertelement <16 x i7> %v10,  i7 %c11, i32 11
  %v12 = insertelement <16 x i7> %v11,  i7 %c12, i32 12
  %v13 = insertelement <16 x i7> %v12,  i7 %c13, i32 13
  %v14 = insertelement <16 x i7> %v13,  i7 %c14, i32 14
  %v15 = insertelement <16 x i7> %v14,  i7 %c15, i32 15
  ret <16 x i7> %v15
}

; 32-byte block (two 16s)
define <32 x i7> @atlas.r96.classify.v32i8(<32 x i8> %bytes) nounwind readnone willreturn {
entry:
  %low  = shufflevector <32 x i8> %bytes, <32 x i8> undef,
           <16 x i32> <i32 0,  i32 1,  i32 2,  i32 3,  i32 4,  i32 5,  i32 6,  i32 7,
                        i32 8,  i32 9,  i32 10, i32 11, i32 12, i32 13, i32 14, i32 15>
  %high = shufflevector <32 x i8> %bytes, <32 x i8> undef,
           <16 x i32> <i32 16, i32 17, i32 18, i32 19, i32 20, i32 21, i32 22, i32 23,
                        i32 24, i32 25, i32 26, i32 27, i32 28, i32 29, i32 30, i32 31>
  %lcls = call <16 x i7> @atlas.r96.classify.v16i8(<16 x i8> %low)
  %hcls = call <16 x i7> @atlas.r96.classify.v16i8(<16 x i8> %high)
  %res  = shufflevector <16 x i7> %lcls, <16 x i7> %hcls,
           <32 x i32> <i32 0,  i32 1,  i32 2,  i32 3,  i32 4,  i32 5,  i32 6,  i32 7,
                        i32 8,  i32 9,  i32 10, i32 11, i32 12, i32 13, i32 14, i32 15,
                        i32 16, i32 17, i32 18, i32 19, i32 20, i32 21, i32 22, i32 23,
                        i32 24, i32 25, i32 26, i32 27, i32 28, i32 29, i32 30, i32 31>
  ret <32 x i7> %res
}

; 256-byte page — loop-based pure implementation
define <256 x i7> @atlas.r96.classify.page(<256 x i8> %page) nounwind readnone willreturn {
entry:
  br label %loop

loop:
  %i     = phi i32 [ 0, %entry ], [ %next, %body ]
  %acc   = phi <256 x i7> [ undef, %entry ], [ %acc2, %body ]
  %done  = icmp eq i32 %i, 256
  br i1 %done, label %exit, label %body

body:
  %b     = extractelement <256 x i8> %page, i32 %i
  %c     = call i7 @atlas.r96.classify(i8 %b)
  %acc2  = insertelement <256 x i7> %acc, i7 %c, i32 %i
  %next  = add i32 %i, 1
  br label %loop

exit:
  %final = phi <256 x i7> [ %acc, %loop ]
  ret <256 x i7> %final
}

; =============================================================================
; Page + histogram utilities (pointer-based helpers)
; =============================================================================

; Zero and fill a 96-bin histogram for a raw page pointer.
;  page: ptr to 256 bytes
;  histogram: ptr to [96 x i32]

declare void @llvm.memset.p0.i64(ptr nocapture, i8, i64, i1)

define void @atlas.r96.histogram(ptr %page, ptr %histogram) nounwind {
entry:
  ; memset(histogram, 0, 96 * 4)
  call void @llvm.memset.p0.i64(ptr %histogram, i8 0, i64 384, i1 false)
  br label %loop

loop:
  %i = phi i32 [ 0, %entry ], [ %next, %body ]
  %done = icmp eq i32 %i, 256
  br i1 %done, label %exit, label %body

body:
  ; byte = page[i]
  %bptr = getelementptr i8, ptr %page, i32 %i
  %byte = load i8, ptr %bptr, align 1
  ; class = classify(byte)
  %cls  = call i7 @atlas.r96.classify(i8 %byte)
  %idx  = zext i7 %cls to i32
  ; ++histogram[class]
  %hptr = getelementptr [96 x i32], ptr %histogram, i32 0, i32 %idx
  %old  = load i32, ptr %hptr, align 4
  %new  = add i32 %old, 1
  store i32 %new, ptr %hptr, align 4
  ; next
  %next = add i32 %i, 1
  br label %loop

exit:
  ret void
}

; Return dominant (arg‑max) resonance class in a page.
define i7 @atlas.r96.dominant(ptr %page) nounwind {
entry:
  %hist = alloca [96 x i32], align 16
  call void @atlas.r96.histogram(ptr %page, ptr %hist)
  br label %scan

scan:
  %i          = phi i32 [ 0, %entry ], [ %next, %scan ]
  %max_count  = phi i32 [ 0, %entry ], [ %max_count.new, %scan ]
  %max_class  = phi i7  [ 0, %entry ], [ %max_class.new, %scan ]

  %hptr       = getelementptr [96 x i32], ptr %hist, i32 0, i32 %i
  %count      = load i32, ptr %hptr, align 4
  %gt         = icmp ugt i32 %count, %max_count
  %i_trunc    = trunc i32 %i to i7
  %max_count.new = select i1 %gt, i32 %count, i32 %max_count
  %max_class.new = select i1 %gt, i7  %i_trunc, i7  %max_class

  %next       = add i32 %i, 1
  %done       = icmp eq i32 %next, 96
  br i1 %done, label %exit, label %scan

exit:
  %res = phi i7 [ %max_class.new, %scan ]
  ret i7 %res
}

; =============================================================================
; Metadata
; =============================================================================

!0 = !{}                          ; for !invariant.load on table accesses
; Metadata removed - not valid syntax
