; atlas-12288-simd.ll — SIMD/Vector optimized operations for Atlas‑12,288
; ---------------------------------------------------------------------------------
; Corrected for LLVM 15+ opaque pointers, valid intrinsics, and portable SIMD IR.
; Notes
;  • Removes non-existent x86 gather intrinsics under SSE2/AVX512 and avoids
;    table-gather from @atlas.r96.table (which may be internal in other modules).
;  • Implements vector classification via arithmetic: r = x % 96 using only
;    shifts, compares, and adds (valid on SSE2/AVX2/AVX-512/NEON).
;  • Uses generic vector-reduce intrinsics on widened (i16) lanes to avoid
;    overflow when summing i8 elements.
;  • Opaque pointers (ptr) used everywhere.
; ---------------------------------------------------------------------------------

source_filename = "atlas-12288-simd.ll"

; =============================================================================
; Utility constants (vectors of splats)
; =============================================================================

@.v16_i16_5  = private unnamed_addr constant <16 x i16> <i16 5,  i16 5,  i16 5,  i16 5,  i16 5,  i16 5,  i16 5,  i16 5,
                                                i16 5,  i16 5,  i16 5,  i16 5,  i16 5,  i16 5,  i16 5,  i16 5>
@.v16_i16_3  = private unnamed_addr constant <16 x i16> <i16 3,  i16 3,  i16 3,  i16 3,  i16 3,  i16 3,  i16 3,  i16 3,
                                                i16 3,  i16 3,  i16 3,  i16 3,  i16 3,  i16 3,  i16 3,  i16 3>
@.v16_i16_6  = private unnamed_addr constant <16 x i16> <i16 6,  i16 6,  i16 6,  i16 6,  i16 6,  i16 6,  i16 6,  i16 6,
                                                i16 6,  i16 6,  i16 6,  i16 6,  i16 6,  i16 6,  i16 6,  i16 6>
@.v16_i16_32 = private unnamed_addr constant <16 x i16> <i16 32, i16 32, i16 32, i16 32, i16 32, i16 32, i16 32, i16 32,
                                                i16 32, i16 32, i16 32, i16 32, i16 32, i16 32, i16 32, i16 32>
@.v16_i16_64 = private unnamed_addr constant <16 x i16> <i16 64, i16 64, i16 64, i16 64, i16 64, i16 64, i16 64, i16 64,
                                                i16 64, i16 64, i16 64, i16 64, i16 64, i16 64, i16 64, i16 64>

@.v32_i16_5  = private unnamed_addr constant <32 x i16> zeroinitializer
@.v32_i16_3  = private unnamed_addr constant <32 x i16> zeroinitializer
@.v32_i16_6  = private unnamed_addr constant <32 x i16> zeroinitializer
@.v32_i16_32 = private unnamed_addr constant <32 x i16> zeroinitializer
@.v32_i16_64 = private unnamed_addr constant <32 x i16> zeroinitializer

@.v64_i16_5  = private unnamed_addr constant <64 x i16> zeroinitializer
@.v64_i16_3  = private unnamed_addr constant <64 x i16> zeroinitializer
@.v64_i16_6  = private unnamed_addr constant <64 x i16> zeroinitializer
@.v64_i16_32 = private unnamed_addr constant <64 x i16> zeroinitializer
@.v64_i16_64 = private unnamed_addr constant <64 x i16> zeroinitializer

; NOTE: The v32/v64 splats are materialized via insertelement/shuffle below to
; avoid giant constant initializers; the zero placeholders exist to allow GEPs.

; =============================================================================
; Helpers to build splat vectors at runtime (to keep file size small)
; =============================================================================

; build_splat_32(x:i16)
define internal <32 x i16> @atlas._splat32_i16(i16 %x) nounwind readnone {
entry:
  %b   = insertelement <32 x i16> undef, i16 %x, i32 0
  %b2  = shufflevector <32 x i16> %b, <32 x i16> undef,
         <32 x i32> <i32 0, i32 0, i32 0, i32 0, i32 0, i32 0, i32 0, i32 0,
                      i32 0, i32 0, i32 0, i32 0, i32 0, i32 0, i32 0, i32 0,
                      i32 0, i32 0, i32 0, i32 0, i32 0, i32 0, i32 0, i32 0,
                      i32 0, i32 0, i32 0, i32 0, i32 0, i32 0, i32 0, i32 0>
  ret <32 x i16> %b2
}

; build_splat_64(x:i16)
define internal <64 x i16> @atlas._splat64_i16(i16 %x) nounwind readnone {
entry:
  %b   = insertelement <64 x i16> undef, i16 %x, i32 0
  %b2  = shufflevector <64 x i16> %b, <64 x i16> undef,
         <64 x i32> <i32 0, i32 0, i32 0, i32 0, i32 0, i32 0, i32 0, i32 0,
                      i32 0, i32 0, i32 0, i32 0, i32 0, i32 0, i32 0, i32 0,
                      i32 0, i32 0, i32 0, i32 0, i32 0, i32 0, i32 0, i32 0,
                      i32 0, i32 0, i32 0, i32 0, i32 0, i32 0, i32 0, i32 0,
                      i32 0, i32 0, i32 0, i32 0, i32 0, i32 0, i32 0, i32 0,
                      i32 0, i32 0, i32 0, i32 0, i32 0, i32 0, i32 0, i32 0,
                      i32 0, i32 0, i32 0, i32 0, i32 0, i32 0, i32 0, i32 0,
                      i32 0, i32 0, i32 0, i32 0, i32 0, i32 0, i32 0, i32 0>
  ret <64 x i16> %b2
}

; =============================================================================
; SSE2 (128-bit) — classify 16 bytes and sum 16 bytes
; =============================================================================

; r = x % 96 via q = floor((x>>5)/3) in {0,1,2} and r = x - q*96
; Returns <16 x i7> to match Atlas intrinsics vector element width.

define <16 x i7> @atlas.r96.classify.sse2(<16 x i8> %bytes) nounwind readnone willreturn {
entry:
  %x16   = zext <16 x i8> %bytes to <16 x i16>
  %v5    = load <16 x i16>, ptr @.v16_i16_5, align 32
  %v3    = load <16 x i16>, ptr @.v16_i16_3, align 32
  %v6    = load <16 x i16>, ptr @.v16_i16_6, align 32
  %sh5   = lshr <16 x i16> %x16, %v5
  %ge3   = icmp uge <16 x i16> %sh5, %v3
  %ge6   = icmp uge <16 x i16> %sh5, %v6
  %q1    = zext <16 x i1> %ge3 to <16 x i16>
  %q2    = zext <16 x i1> %ge6 to <16 x i16>
  %q     = add <16 x i16> %q1, %q2                       ; 0..2
  %q64   = shl <16 x i16> %q,  %v6 ; q<<6
  %q32   = shl <16 x i16> %q,  %v5 ; q<<5
  %q96   = add <16 x i16> %q64, %q32
  %r16   = sub <16 x i16> %x16, %q96
  %r8    = trunc <16 x i16> %r16 to <16 x i8>
  %r7    = trunc <16 x i8>  %r8  to <16 x i7>
  ret <16 x i7> %r7
}

; Sum 16 bytes into i16 (widen then reduce)

declare i16 @llvm.experimental.vector.reduce.add.v16i16(<16 x i16>)

define i16 @atlas.conserved.sum.sse2(<16 x i8> %data) nounwind readnone willreturn {
entry:
  %w = zext <16 x i8> %data to <16 x i16>
  %s = call i16 @llvm.experimental.vector.reduce.add.v16i16(<16 x i16> %w)
  ret i16 %s
}

; Encode 16 coordinates in parallel: (page<<16) | offset

define <16 x i32> @atlas.boundary.encode.sse2(<16 x i16> %pages, <16 x i8> %offsets) nounwind readnone willreturn {
entry:
  %p32   = zext <16 x i16> %pages   to <16 x i32>
  %o32   = zext <16 x i8>  %offsets to <16 x i32>
  %sh    = shl  <16 x i32> %p32, <i32 16, i32 16, i32 16, i32 16, i32 16, i32 16, i32 16, i32 16, i32 16, i32 16, i32 16, i32 16, i32 16, i32 16, i32 16, i32 16>
  %res   = or   <16 x i32> %sh, %o32
  ret <16 x i32> %res
}

; =============================================================================
; AVX2 (256-bit) — classify 32 bytes and sum 32 bytes
; =============================================================================

declare i16 @llvm.experimental.vector.reduce.add.v32i16(<32 x i16>)

define <32 x i7> @atlas.r96.classify.avx2(<32 x i8> %bytes) nounwind readnone willreturn {
entry:
  %x16  = zext <32 x i8> %bytes to <32 x i16>
  %five = call <32 x i16> @atlas._splat32_i16(i16 5)
  %thr  = call <32 x i16> @atlas._splat32_i16(i16 3)
  %six  = call <32 x i16> @atlas._splat32_i16(i16 6)
  %sh5  = lshr <32 x i16> %x16, %five
  %ge3  = icmp uge <32 x i16> %sh5, %thr
  %ge6  = icmp uge <32 x i16> %sh5, %six
  %q1   = zext <32 x i1> %ge3 to <32 x i16>
  %q2   = zext <32 x i1> %ge6 to <32 x i16>
  %q    = add <32 x i16> %q1, %q2
  %sh6s = call <32 x i16> @atlas._splat32_i16(i16 6)
  %sh5s = call <32 x i16> @atlas._splat32_i16(i16 5)
  %q64  = shl <32 x i16> %q, %sh6s
  %q32  = shl <32 x i16> %q, %sh5s
  %q96  = add <32 x i16> %q64, %q32
  %r16  = sub <32 x i16> %x16, %q96
  %r8   = trunc <32 x i16> %r16 to <32 x i8>
  %r7   = trunc <32 x i8>  %r8  to <32 x i7>
  ret <32 x i7> %r7
}

define i16 @atlas.conserved.sum.avx2(<32 x i8> %data) nounwind readnone willreturn {
entry:
  %w = zext <32 x i8> %data to <32 x i16>
  %s = call i16 @llvm.experimental.vector.reduce.add.v32i16(<32 x i16> %w)
  ret i16 %s
}

; Classify a 256-byte page as eight 32-byte chunks and return <256 x i7>

define <256 x i7> @atlas.r96.classify.page.avx2(<256 x i8> %page) nounwind readnone willreturn {
entry:
  ; slice into 8 chunks of 32
  %s0 = shufflevector <256 x i8> %page, <256 x i8> undef,
        <32 x i32> <i32 0, i32 1, i32 2, i32 3, i32 4, i32 5, i32 6, i32 7,
                     i32 8, i32 9, i32 10, i32 11, i32 12, i32 13, i32 14, i32 15,
                     i32 16, i32 17, i32 18, i32 19, i32 20, i32 21, i32 22, i32 23,
                     i32 24, i32 25, i32 26, i32 27, i32 28, i32 29, i32 30, i32 31>
  %s1 = shufflevector <256 x i8> %page, <256 x i8> undef,
        <32 x i32> <i32 32, i32 33, i32 34, i32 35, i32 36, i32 37, i32 38, i32 39,
                     i32 40, i32 41, i32 42, i32 43, i32 44, i32 45, i32 46, i32 47,
                     i32 48, i32 49, i32 50, i32 51, i32 52, i32 53, i32 54, i32 55,
                     i32 56, i32 57, i32 58, i32 59, i32 60, i32 61, i32 62, i32 63>
  %s2 = shufflevector <256 x i8> %page, <256 x i8> undef,
        <32 x i32> <i32 64, i32 65, i32 66, i32 67, i32 68, i32 69, i32 70, i32 71,
                     i32 72, i32 73, i32 74, i32 75, i32 76, i32 77, i32 78, i32 79,
                     i32 80, i32 81, i32 82, i32 83, i32 84, i32 85, i32 86, i32 87,
                     i32 88, i32 89, i32 90, i32 91, i32 92, i32 93, i32 94, i32 95>
  %s3 = shufflevector <256 x i8> %page, <256 x i8> undef,
        <32 x i32> <i32 96, i32 97, i32 98, i32 99, i32 100, i32 101, i32 102, i32 103,
                     i32 104, i32 105, i32 106, i32 107, i32 108, i32 109, i32 110, i32 111,
                     i32 112, i32 113, i32 114, i32 115, i32 116, i32 117, i32 118, i32 119,
                     i32 120, i32 121, i32 122, i32 123, i32 124, i32 125, i32 126, i32 127>
  %s4 = shufflevector <256 x i8> %page, <256 x i8> undef,
        <32 x i32> <i32 128, i32 129, i32 130, i32 131, i32 132, i32 133, i32 134, i32 135,
                     i32 136, i32 137, i32 138, i32 139, i32 140, i32 141, i32 142, i32 143,
                     i32 144, i32 145, i32 146, i32 147, i32 148, i32 149, i32 150, i32 151,
                     i32 152, i32 153, i32 154, i32 155, i32 156, i32 157, i32 158, i32 159>
  %s5 = shufflevector <256 x i8> %page, <256 x i8> undef,
        <32 x i32> <i32 160, i32 161, i32 162, i32 163, i32 164, i32 165, i32 166, i32 167,
                     i32 168, i32 169, i32 170, i32 171, i32 172, i32 173, i32 174, i32 175,
                     i32 176, i32 177, i32 178, i32 179, i32 180, i32 181, i32 182, i32 183,
                     i32 184, i32 185, i32 186, i32 187, i32 188, i32 189, i32 190, i32 191>
  %s6 = shufflevector <256 x i8> %page, <256 x i8> undef,
        <32 x i32> <i32 192, i32 193, i32 194, i32 195, i32 196, i32 197, i32 198, i32 199,
                     i32 200, i32 201, i32 202, i32 203, i32 204, i32 205, i32 206, i32 207,
                     i32 208, i32 209, i32 210, i32 211, i32 212, i32 213, i32 214, i32 215,
                     i32 216, i32 217, i32 218, i32 219, i32 220, i32 221, i32 222, i32 223>
  %s7 = shufflevector <256 x i8> %page, <256 x i8> undef,
        <32 x i32> <i32 224, i32 225, i32 226, i32 227, i32 228, i32 229, i32 230, i32 231,
                     i32 232, i32 233, i32 234, i32 235, i32 236, i32 237, i32 238, i32 239,
                     i32 240, i32 241, i32 242, i32 243, i32 244, i32 245, i32 246, i32 247,
                     i32 248, i32 249, i32 250, i32 251, i32 252, i32 253, i32 254, i32 255>

  %c0 = call <32 x i7> @atlas.r96.classify.avx2(<32 x i8> %s0)
  %c1 = call <32 x i7> @atlas.r96.classify.avx2(<32 x i8> %s1)
  %c2 = call <32 x i7> @atlas.r96.classify.avx2(<32 x i8> %s2)
  %c3 = call <32 x i7> @atlas.r96.classify.avx2(<32 x i8> %s3)
  %c4 = call <32 x i7> @atlas.r96.classify.avx2(<32 x i8> %s4)
  %c5 = call <32 x i7> @atlas.r96.classify.avx2(<32 x i8> %s5)
  %c6 = call <32 x i7> @atlas.r96.classify.avx2(<32 x i8> %s6)
  %c7 = call <32 x i7> @atlas.r96.classify.avx2(<32 x i8> %s7)

  ; stitch back into <256 x i7>
  %lo0 = shufflevector <32 x i7> %c0, <32 x i7> %c1,
         <64 x i32> <i32 0, i32 1, i32 2, i32 3, i32 4, i32 5, i32 6, i32 7,
                      i32 8, i32 9, i32 10, i32 11, i32 12, i32 13, i32 14, i32 15,
                      i32 16, i32 17, i32 18, i32 19, i32 20, i32 21, i32 22, i32 23,
                      i32 24, i32 25, i32 26, i32 27, i32 28, i32 29, i32 30, i32 31,
                      i32 32, i32 33, i32 34, i32 35, i32 36, i32 37, i32 38, i32 39,
                      i32 40, i32 41, i32 42, i32 43, i32 44, i32 45, i32 46, i32 47,
                      i32 48, i32 49, i32 50, i32 51, i32 52, i32 53, i32 54, i32 55,
                      i32 56, i32 57, i32 58, i32 59, i32 60, i32 61, i32 62, i32 63>
  ; Combine 32-byte chunks into 64-byte chunks (just concatenate them)
  %lo1 = shufflevector <32 x i7> %c2, <32 x i7> %c3,
         <64 x i32> <i32 0, i32 1, i32 2, i32 3, i32 4, i32 5, i32 6, i32 7,
                      i32 8, i32 9, i32 10, i32 11, i32 12, i32 13, i32 14, i32 15,
                      i32 16, i32 17, i32 18, i32 19, i32 20, i32 21, i32 22, i32 23,
                      i32 24, i32 25, i32 26, i32 27, i32 28, i32 29, i32 30, i32 31,
                      i32 32, i32 33, i32 34, i32 35, i32 36, i32 37, i32 38, i32 39,
                      i32 40, i32 41, i32 42, i32 43, i32 44, i32 45, i32 46, i32 47,
                      i32 48, i32 49, i32 50, i32 51, i32 52, i32 53, i32 54, i32 55,
                      i32 56, i32 57, i32 58, i32 59, i32 60, i32 61, i32 62, i32 63>
  %lo2 = shufflevector <32 x i7> %c4, <32 x i7> %c5,
         <64 x i32> <i32 0, i32 1, i32 2, i32 3, i32 4, i32 5, i32 6, i32 7,
                      i32 8, i32 9, i32 10, i32 11, i32 12, i32 13, i32 14, i32 15,
                      i32 16, i32 17, i32 18, i32 19, i32 20, i32 21, i32 22, i32 23,
                      i32 24, i32 25, i32 26, i32 27, i32 28, i32 29, i32 30, i32 31,
                      i32 32, i32 33, i32 34, i32 35, i32 36, i32 37, i32 38, i32 39,
                      i32 40, i32 41, i32 42, i32 43, i32 44, i32 45, i32 46, i32 47,
                      i32 48, i32 49, i32 50, i32 51, i32 52, i32 53, i32 54, i32 55,
                      i32 56, i32 57, i32 58, i32 59, i32 60, i32 61, i32 62, i32 63>
  %lo3 = shufflevector <32 x i7> %c6, <32 x i7> %c7,
         <64 x i32> <i32 0, i32 1, i32 2, i32 3, i32 4, i32 5, i32 6, i32 7,
                      i32 8, i32 9, i32 10, i32 11, i32 12, i32 13, i32 14, i32 15,
                      i32 16, i32 17, i32 18, i32 19, i32 20, i32 21, i32 22, i32 23,
                      i32 24, i32 25, i32 26, i32 27, i32 28, i32 29, i32 30, i32 31,
                      i32 32, i32 33, i32 34, i32 35, i32 36, i32 37, i32 38, i32 39,
                      i32 40, i32 41, i32 42, i32 43, i32 44, i32 45, i32 46, i32 47,
                      i32 48, i32 49, i32 50, i32 51, i32 52, i32 53, i32 54, i32 55,
                      i32 56, i32 57, i32 58, i32 59, i32 60, i32 61, i32 62, i32 63>

  %q0 = shufflevector <64 x i7> %lo0, <64 x i7> %lo1,
        <128 x i32> <i32 0, i32 1, i32 2, i32 3, i32 4, i32 5, i32 6, i32 7,
                      i32 8, i32 9, i32 10, i32 11, i32 12, i32 13, i32 14, i32 15,
                      i32 16, i32 17, i32 18, i32 19, i32 20, i32 21, i32 22, i32 23,
                      i32 24, i32 25, i32 26, i32 27, i32 28, i32 29, i32 30, i32 31,
                      i32 32, i32 33, i32 34, i32 35, i32 36, i32 37, i32 38, i32 39,
                      i32 40, i32 41, i32 42, i32 43, i32 44, i32 45, i32 46, i32 47,
                      i32 48, i32 49, i32 50, i32 51, i32 52, i32 53, i32 54, i32 55,
                      i32 56, i32 57, i32 58, i32 59, i32 60, i32 61, i32 62, i32 63,
                      i32 64, i32 65, i32 66, i32 67, i32 68, i32 69, i32 70, i32 71,
                      i32 72, i32 73, i32 74, i32 75, i32 76, i32 77, i32 78, i32 79,
                      i32 80, i32 81, i32 82, i32 83, i32 84, i32 85, i32 86, i32 87,
                      i32 88, i32 89, i32 90, i32 91, i32 92, i32 93, i32 94, i32 95,
                      i32 96, i32 97, i32 98, i32 99, i32 100, i32 101, i32 102, i32 103,
                      i32 104, i32 105, i32 106, i32 107, i32 108, i32 109, i32 110, i32 111,
                      i32 112, i32 113, i32 114, i32 115, i32 116, i32 117, i32 118, i32 119,
                      i32 120, i32 121, i32 122, i32 123, i32 124, i32 125, i32 126, i32 127>
  %q1 = shufflevector <64 x i7> %lo2, <64 x i7> %lo3,
        <128 x i32> <i32 0, i32 1, i32 2, i32 3, i32 4, i32 5, i32 6, i32 7,
                      i32 8, i32 9, i32 10, i32 11, i32 12, i32 13, i32 14, i32 15,
                      i32 16, i32 17, i32 18, i32 19, i32 20, i32 21, i32 22, i32 23,
                      i32 24, i32 25, i32 26, i32 27, i32 28, i32 29, i32 30, i32 31,
                      i32 32, i32 33, i32 34, i32 35, i32 36, i32 37, i32 38, i32 39,
                      i32 40, i32 41, i32 42, i32 43, i32 44, i32 45, i32 46, i32 47,
                      i32 48, i32 49, i32 50, i32 51, i32 52, i32 53, i32 54, i32 55,
                      i32 56, i32 57, i32 58, i32 59, i32 60, i32 61, i32 62, i32 63,
                      i32 64, i32 65, i32 66, i32 67, i32 68, i32 69, i32 70, i32 71,
                      i32 72, i32 73, i32 74, i32 75, i32 76, i32 77, i32 78, i32 79,
                      i32 80, i32 81, i32 82, i32 83, i32 84, i32 85, i32 86, i32 87,
                      i32 88, i32 89, i32 90, i32 91, i32 92, i32 93, i32 94, i32 95,
                      i32 96, i32 97, i32 98, i32 99, i32 100, i32 101, i32 102, i32 103,
                      i32 104, i32 105, i32 106, i32 107, i32 108, i32 109, i32 110, i32 111,
                      i32 112, i32 113, i32 114, i32 115, i32 116, i32 117, i32 118, i32 119,
                      i32 120, i32 121, i32 122, i32 123, i32 124, i32 125, i32 126, i32 127>

  ; Combine both halves to get full 256 vector
  %out = shufflevector <128 x i7> %q0, <128 x i7> %q1,
         <256 x i32> <i32 0, i32 1, i32 2, i32 3, i32 4, i32 5, i32 6, i32 7,
                       i32 8, i32 9, i32 10, i32 11, i32 12, i32 13, i32 14, i32 15,
                       i32 16, i32 17, i32 18, i32 19, i32 20, i32 21, i32 22, i32 23,
                       i32 24, i32 25, i32 26, i32 27, i32 28, i32 29, i32 30, i32 31,
                       i32 32, i32 33, i32 34, i32 35, i32 36, i32 37, i32 38, i32 39,
                       i32 40, i32 41, i32 42, i32 43, i32 44, i32 45, i32 46, i32 47,
                       i32 48, i32 49, i32 50, i32 51, i32 52, i32 53, i32 54, i32 55,
                       i32 56, i32 57, i32 58, i32 59, i32 60, i32 61, i32 62, i32 63,
                       i32 64, i32 65, i32 66, i32 67, i32 68, i32 69, i32 70, i32 71,
                       i32 72, i32 73, i32 74, i32 75, i32 76, i32 77, i32 78, i32 79,
                       i32 80, i32 81, i32 82, i32 83, i32 84, i32 85, i32 86, i32 87,
                       i32 88, i32 89, i32 90, i32 91, i32 92, i32 93, i32 94, i32 95,
                       i32 96, i32 97, i32 98, i32 99, i32 100, i32 101, i32 102, i32 103,
                       i32 104, i32 105, i32 106, i32 107, i32 108, i32 109, i32 110, i32 111,
                       i32 112, i32 113, i32 114, i32 115, i32 116, i32 117, i32 118, i32 119,
                       i32 120, i32 121, i32 122, i32 123, i32 124, i32 125, i32 126, i32 127,
                       i32 128, i32 129, i32 130, i32 131, i32 132, i32 133, i32 134, i32 135,
                       i32 136, i32 137, i32 138, i32 139, i32 140, i32 141, i32 142, i32 143,
                       i32 144, i32 145, i32 146, i32 147, i32 148, i32 149, i32 150, i32 151,
                       i32 152, i32 153, i32 154, i32 155, i32 156, i32 157, i32 158, i32 159,
                       i32 160, i32 161, i32 162, i32 163, i32 164, i32 165, i32 166, i32 167,
                       i32 168, i32 169, i32 170, i32 171, i32 172, i32 173, i32 174, i32 175,
                       i32 176, i32 177, i32 178, i32 179, i32 180, i32 181, i32 182, i32 183,
                       i32 184, i32 185, i32 186, i32 187, i32 188, i32 189, i32 190, i32 191,
                       i32 192, i32 193, i32 194, i32 195, i32 196, i32 197, i32 198, i32 199,
                       i32 200, i32 201, i32 202, i32 203, i32 204, i32 205, i32 206, i32 207,
                       i32 208, i32 209, i32 210, i32 211, i32 212, i32 213, i32 214, i32 215,
                       i32 216, i32 217, i32 218, i32 219, i32 220, i32 221, i32 222, i32 223,
                       i32 224, i32 225, i32 226, i32 227, i32 228, i32 229, i32 230, i32 231,
                       i32 232, i32 233, i32 234, i32 235, i32 236, i32 237, i32 238, i32 239,
                       i32 240, i32 241, i32 242, i32 243, i32 244, i32 245, i32 246, i32 247,
                       i32 248, i32 249, i32 250, i32 251, i32 252, i32 253, i32 254, i32 255>
  ret <256 x i7> %out
}

; =============================================================================
; AVX‑512 (512-bit) — classify 64 bytes and sum 64 bytes
; =============================================================================

declare i16 @llvm.experimental.vector.reduce.add.v64i16(<64 x i16>)

define <64 x i7> @atlas.r96.classify.avx512(<64 x i8> %bytes) nounwind readnone willreturn {
entry:
  %x16  = zext <64 x i8> %bytes to <64 x i16>
  %five = call <64 x i16> @atlas._splat64_i16(i16 5)
  %thr  = call <64 x i16> @atlas._splat64_i16(i16 3)
  %six  = call <64 x i16> @atlas._splat64_i16(i16 6)
  %sh5  = lshr <64 x i16> %x16, %five
  %ge3  = icmp uge <64 x i16> %sh5, %thr
  %ge6  = icmp uge <64 x i16> %sh5, %six
  %q1   = zext <64 x i1> %ge3 to <64 x i16>
  %q2   = zext <64 x i1> %ge6 to <64 x i16>
  %q    = add <64 x i16> %q1, %q2
  %sh6s = call <64 x i16> @atlas._splat64_i16(i16 6)
  %sh5s = call <64 x i16> @atlas._splat64_i16(i16 5)
  %q64  = shl <64 x i16> %q, %sh6s
  %q32  = shl <64 x i16> %q, %sh5s
  %q96  = add <64 x i16> %q64, %q32
  %r16  = sub <64 x i16> %x16, %q96
  %r8   = trunc <64 x i16> %r16 to <64 x i8>
  %r7   = trunc <64 x i8>  %r8  to <64 x i7>
  ret <64 x i7> %r7
}

define i16 @atlas.conserved.sum.avx512(<64 x i8> %data) nounwind readnone willreturn {
entry:
  %w = zext <64 x i8> %data to <64 x i16>
  %s = call i16 @llvm.experimental.vector.reduce.add.v64i16(<64 x i16> %w)
  ret i16 %s
}

; Process an entire 12,288‑byte structure in 64‑byte strides (AVX‑512 width)

define void @atlas.structure.process.avx512(ptr %s) nounwind {
entry:
  %base = bitcast ptr %s to ptr
  br label %loop

loop:
  %i    = phi i32 [ 0, %entry ], [ %next, %body ]
  %done = icmp eq i32 %i, 192                ; 12288 / 64
  br i1 %done, label %exit, label %body

body:
  %ptr = getelementptr <64 x i8>, ptr %base, i32 %i
  %blk = load <64 x i8>, ptr %ptr, align 64
  %cls = call <64 x i7> @atlas.r96.classify.avx512(<64 x i8> %blk)
  ; Example: compute checksum of block (for debugging/verification)
  %w   = zext <64 x i8> %blk to <64 x i16>
  %sum = call i16 @llvm.experimental.vector.reduce.add.v64i16(<64 x i16> %w)
  ; (In real pipelines you would forward %cls to subsequent passes.)
  %next = add i32 %i, 1
  br label %loop

exit:
  ret void
}

; =============================================================================
; ARM NEON — classify 16 bytes and sum 16 bytes (generic IR)
; =============================================================================

define <16 x i7> @atlas.r96.classify.neon(<16 x i8> %bytes) nounwind readnone willreturn {
entry:
  ; Same arithmetic as SSE2 variant; NEON backend will generate TBL-free code.
  %x16   = zext <16 x i8> %bytes to <16 x i16>
  %v5    = load <16 x i16>, ptr @.v16_i16_5, align 32
  %v3    = load <16 x i16>, ptr @.v16_i16_3, align 32
  %v6    = load <16 x i16>, ptr @.v16_i16_6, align 32
  %sh5   = lshr <16 x i16> %x16, %v5
  %ge3   = icmp uge <16 x i16> %sh5, %v3
  %ge6   = icmp uge <16 x i16> %sh5, %v6
  %q1    = zext <16 x i1> %ge3 to <16 x i16>
  %q2    = zext <16 x i1> %ge6 to <16 x i16>
  %q     = add <16 x i16> %q1, %q2
  %q64   = shl <16 x i16> %q,  %v6
  %q32   = shl <16 x i16> %q,  %v5
  %q96   = add <16 x i16> %q64, %q32
  %r16   = sub <16 x i16> %x16, %q96
  %r8    = trunc <16 x i16> %r16 to <16 x i8>
  %r7    = trunc <16 x i8>  %r8  to <16 x i7>
  ret <16 x i7> %r7
}

; Sum 16 bytes into i16 (widen then reduce)
; Note: already declared above for SSE2

define i16 @atlas.conserved.sum.neon(<16 x i8> %data) nounwind readnone willreturn {
entry:
  %w = zext <16 x i8> %data to <16 x i16>
  %s = call i16 @llvm.experimental.vector.reduce.add.v16i16(<16 x i16> %w)
  ret i16 %s
}

; =============================================================================
; Generic vector+pointer helpers
; =============================================================================

; Generic conservation check over arbitrary buffer length using 64-byte strides.
; Returns true iff sum(data) % 96 == 0
; Note: v64i16 reduce already declared above

define i1 @atlas.conserved.vector.generic(ptr %data, i64 %len) nounwind readonly {
entry:
  %chunks   = udiv i64 %len, 64
  %rem      = urem i64 %len, 64
  %base     = bitcast ptr %data to ptr
  br label %loop

loop:
  %i    = phi i64 [ 0, %entry ], [ %i.next, %body ]
  %acc  = phi i64 [ 0, %entry ], [ %acc.next, %body ]
  %done = icmp uge i64 %i, %chunks
  br i1 %done, label %remainder, label %body

body:
  %ptr  = getelementptr <64 x i8>, ptr %base, i64 %i
  %blk  = load <64 x i8>, ptr %ptr, align 1
  %w    = zext <64 x i8> %blk to <64 x i16>
  %sum  = call i16 @llvm.experimental.vector.reduce.add.v64i16(<64 x i16> %w)
  %sum64 = zext i16 %sum to i64
  %acc.next = add i64 %acc, %sum64
  %i.next   = add i64 %i, 1
  br label %loop

remainder:
  br label %rloop

rloop:
  %j    = phi i64 [ 0, %remainder ], [ %j.next, %rbody ]
  %acc2 = phi i64 [ %acc, %remainder ], [ %acc2.next, %rbody ]
  %rdone = icmp uge i64 %j, %rem
  br i1 %rdone, label %check, label %rbody

rbody:
  %chunks64 = mul i64 %chunks, 64
  %off   = add i64 %chunks64, %j               ; offset = chunks*64 + j
  %pbyte = getelementptr i8, ptr %data, i64 %off
  %b     = load i8, ptr %pbyte, align 1
  %b64   = zext i8 %b to i64
  %acc2.next = add i64 %acc2, %b64
  %j.next = add i64 %j, 1
  br label %rloop

check:
  %mod = urem i64 %acc2, 96
  %ok  = icmp eq i64 %mod, 0
  ret i1 %ok
}

; =============================================================================
; Function attributes (target feature toggles)
; =============================================================================

; #0: Basic SSE2 support for 128-bit vectors (x86/x86-64 baseline)
attributes #0 = { "target-features"="+sse2" }
; #1: Advanced Vector Extensions (AVX/AVX2) for 256-bit vectors
attributes #1 = { "target-features"="+avx,+avx2" }
; #2: AVX-512 support for 512-bit vectors (high-end x86-64)
attributes #2 = { "target-features"="+avx512f,+avx512bw" }
; #3: ARM NEON support for 128-bit vectors (AArch64/ARM64)
attributes #3 = { "target-features"="+neon" }

; =============================================================================
; Module metadata
; =============================================================================

; Metadata removed
; Metadata removed
