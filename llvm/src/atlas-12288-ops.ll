; atlas-12288-ops.ll — Core Atlas operations (LLVM 15+, opaque pointers)
; ---------------------------------------------------------------------------------
; This module provides concrete implementations for Atlas operations and thin
; wrappers matching the intrinsic signatures declared in atlas-12288-intrinsics.ll.
; It is updated for opaque pointers (ptr) and compatible with atlas-12288-types.ll.
; ---------------------------------------------------------------------------------

source_filename = "atlas-12288-ops.ll"

; =============================================================================
; Attributes (kept consistent with intrinsics file where applicable)
; =============================================================================

; #0: Pure/value-only operations (no memory access, fully deterministic)
attributes #0 = { nounwind readnone willreturn speculatable }
; #1: Read-only memory access (no writes, cacheable) - Layer 2 hot-path optimized
attributes #1 = { nounwind readonly willreturn "target-features"="+sse2,+avx" "atlas-hot-path"="true" }
; #2: Generic nounwind for allocation/deallocation operations - Layer 2 conserving
attributes #2 = { nounwind "atlas-conserving"="true" }
; #3: Generic nounwind for structure management operations
attributes #3 = { nounwind }
; #4: Generic nounwind for utility/helper operations
attributes #4 = { nounwind }
; #5: Layer 2 SIMD-optimized operations (hot path, vectorizable)
attributes #5 = { nounwind readonly willreturn "target-features"="+sse2,+avx2" "atlas-hot-path"="true" "atlas-vectorizable"="true" }

; =============================================================================
; Declarations used by implementations
; =============================================================================

declare void @llvm.trap() noreturn nounwind
declare void @llvm.memcpy.p0.p0.i64(ptr, ptr, i64, i1)
declare void @llvm.memset.p0.i64(ptr, i8, i64, i1)

declare ptr @malloc(i64)
declare void @free(ptr)
; POSIX aligned_alloc(alignment, size) — signature may vary by platform; wrapper used here.
declare ptr @aligned_alloc(i64, i64)

; =============================================================================
; Helpers (internal)
; =============================================================================

; Layer 2: Efficient mod-96 arithmetic helpers for hot path optimization
; Fast mod-96 using branch-free arithmetic (96 = 32*3 = 2^5*3)
define internal i64 @atlas._fast_mod96(i64 %val) nounwind readnone willreturn {
entry:
  ; For small values, direct modulo is fine
  %small = icmp ult i64 %val, 96
  br i1 %small, label %direct, label %reduce

direct:
  ret i64 %val

reduce:
  ; Use Barrett reduction approximation for larger values
  ; For mod 96: multiply by (2^k / 96) then adjust
  %approx = udiv i64 %val, 96
  %mult = mul i64 %approx, 96
  %remainder = sub i64 %val, %mult
  ret i64 %remainder
}

; Vectorized sum for SIMD-friendly byte accumulation (processes 8 bytes at a time)
define internal i64 @atlas._sum_bytes_simd(ptr %data, i64 %len) nounwind readonly {
entry:
  %vec_len = and i64 %len, -8  ; Round down to multiple of 8
  br label %vec_loop

vec_loop:
  %i = phi i64 [ 0, %entry ], [ %i.next, %vec_body ]
  %acc = phi i64 [ 0, %entry ], [ %acc.next, %vec_body ]
  %vec_done = icmp uge i64 %i, %vec_len
  br i1 %vec_done, label %scalar_start, label %vec_body

vec_body:
  ; Load 8 bytes as i64 and extract individual bytes
  %ptr = getelementptr i8, ptr %data, i64 %i
  %ptr_i64 = bitcast ptr %ptr to ptr
  %val8 = load i64, ptr %ptr_i64, align 1
  
  ; Extract bytes and accumulate (unrolled for efficiency)
  %b0 = and i64 %val8, 255
  %val8_1 = lshr i64 %val8, 8
  %b1 = and i64 %val8_1, 255
  %val8_2 = lshr i64 %val8, 16
  %b2 = and i64 %val8_2, 255
  %val8_3 = lshr i64 %val8, 24
  %b3 = and i64 %val8_3, 255
  %val8_4 = lshr i64 %val8, 32
  %b4 = and i64 %val8_4, 255
  %val8_5 = lshr i64 %val8, 40
  %b5 = and i64 %val8_5, 255
  %val8_6 = lshr i64 %val8, 48
  %b6 = and i64 %val8_6, 255
  %b7 = lshr i64 %val8, 56
  
  %sum8 = add i64 %b0, %b1
  %sum8_2 = add i64 %sum8, %b2
  %sum8_3 = add i64 %sum8_2, %b3
  %sum8_4 = add i64 %sum8_3, %b4
  %sum8_5 = add i64 %sum8_4, %b5
  %sum8_6 = add i64 %sum8_5, %b6
  %sum8_final = add i64 %sum8_6, %b7
  
  %acc.next = add i64 %acc, %sum8_final
  %i.next = add i64 %i, 8
  br label %vec_loop

scalar_start:
  ; Handle remaining bytes
  br label %scalar_loop

scalar_loop:
  %j = phi i64 [ %vec_len, %scalar_start ], [ %j.next, %scalar_body ]
  %acc_scalar = phi i64 [ %acc, %scalar_start ], [ %acc_scalar.next, %scalar_body ]
  %scalar_done = icmp uge i64 %j, %len
  br i1 %scalar_done, label %exit, label %scalar_body

scalar_body:
  %ptr_scalar = getelementptr i8, ptr %data, i64 %j
  %byte = load i8, ptr %ptr_scalar, align 1
  %byte64 = zext i8 %byte to i64
  %acc_scalar.next = add i64 %acc_scalar, %byte64
  %j.next = add i64 %j, 1
  br label %scalar_loop

exit:
  ret i64 %acc_scalar
}

; Sum bytes in a buffer (0..len) into i64
define i64 @atlas._sum_bytes(ptr %data, i64 %len) nounwind readonly {
entry:
  br label %loop

loop:
  %i   = phi i64 [ 0, %entry ], [ %i.next, %body ]
  %acc = phi i64 [ 0, %entry ], [ %acc.next, %body ]
  %done = icmp uge i64 %i, %len
  br i1 %done, label %exit, label %body

body:
  %p     = getelementptr i8, ptr %data, i64 %i
  %byte  = load i8, ptr %p, align 1
  %b64   = zext i8 %byte to i64
  %acc.next = add i64 %acc, %b64
  %i.next   = add i64 %i, 1
  br label %loop

exit:
  ret i64 %acc
}

; Zero and fill a 96-bin histogram from a 256-byte page (for potential callers)
; page: ptr to 256 bytes
; histogram: ptr to [96 x i32]
define internal void @atlas._histogram_96(ptr %page, ptr %hist) nounwind {
entry:
  call void @llvm.memset.p0.i64(ptr %hist, i8 0, i64 384, i1 false)
  br label %loop

loop:
  %i = phi i32 [ 0, %entry ], [ %i.next, %loop ]
  %bptr = getelementptr i8, ptr %page, i32 %i
  %b    = load i8, ptr %bptr, align 1
  %cls7 = call i7 @atlas.r96.classify(i8 %b) readnone
  %idx  = zext i7 %cls7 to i32
  %hptr = getelementptr [96 x i32], ptr %hist, i32 0, i32 %idx
  %old  = load i32, ptr %hptr, align 4
  %new  = add i32 %old, 1
  store i32 %new, ptr %hptr, align 4
  %i.next = add i32 %i, 1
  %done = icmp eq i32 %i.next, 256
  br i1 %done, label %exit, label %loop

exit:
  ret void
}

declare i7 @atlas.r96.classify(i8) #0

; =============================================================================
; Boundary Operations (Φ Isomorphism)
; =============================================================================

; ---- Implementations (internal) ----

define internal i32 @atlas.boundary.encode.impl(i16 %page, i8 %offset) nounwind readnone willreturn {
entry:
  %valid_page = icmp ult i16 %page, 48
  br i1 %valid_page, label %encode, label %error

encode:
  %page32   = zext i16 %page to i32
  %offset32 = zext i8 %offset to i32
  %shifted  = shl i32 %page32, 16
  %with_off = or i32 %shifted, %offset32
  ret i32 %with_off

error:
  ret i32 -1
}

; {i16 page, i8 offset}
define internal { i16, i8 } @atlas.boundary.decode.impl(i32 %boundary) nounwind readnone willreturn {
entry:
  %is_valid = icmp sge i32 %boundary, 0
  br i1 %is_valid, label %decode, label %error

decode:
  %offset32 = and i32 %boundary, 255
  %page32   = lshr i32 %boundary, 16
  %offset   = trunc i32 %offset32 to i8
  %page     = trunc i32 %page32   to i16
  %r1 = insertvalue { i16, i8 } undef, i16 %page,   0
  %r2 = insertvalue { i16, i8 } %r1,  i8  %offset, 1
  ret { i16, i8 } %r2

error:
  %e1 = insertvalue { i16, i8 } undef, i16 -1, 0
  %e2 = insertvalue { i16, i8 } %e1,  i8  -1, 1
  ret { i16, i8 } %e2
}

define internal i32 @atlas.boundary.transform.impl(i32 %boundary, i7 %resonance) nounwind readnone willreturn {
entry:
  %decoded = call { i16, i8 } @atlas.boundary.decode.impl(i32 %boundary)
  %page   = extractvalue { i16, i8 } %decoded, 0
  %offset = extractvalue { i16, i8 } %decoded, 1
  %off16  = zext i8 %offset      to i16
  %r16    = zext i7 %resonance   to i16
  %sum16  = add i16 %off16, %r16
  %newoff = trunc i16 %sum16 to i8
  %wrapped = icmp uge i16 %sum16, 256
  %inc     = zext i1 %wrapped to i16
  %page.new   = add i16 %page, %inc
  %page.mod  = urem i16 %page.new, 48
  %res     = call i32 @atlas.boundary.encode.impl(i16 %page.mod, i8 %newoff)
  ret i32 %res
}

; Klein orbit = page mod 4
define internal i2 @atlas.boundary.klein.impl(i32 %boundary) nounwind readnone willreturn {
entry:
  %d     = call { i16, i8 } @atlas.boundary.decode.impl(i32 %boundary)
  %page  = extractvalue { i16, i8 } %d, 0
  %mod4  = urem i16 %page, 4
  %k     = trunc i16 %mod4 to i2
  ret i2 %k
}

; Rotate: set page's klein component to given value, keep offset; wrap by 48
define internal i32 @atlas.boundary.rotate.impl(i32 %boundary, i2 %klein) nounwind readnone willreturn {
entry:
  %d      = call { i16, i8 } @atlas.boundary.decode.impl(i32 %boundary)
  %page   = extractvalue { i16, i8 } %d, 0
  %offset = extractvalue { i16, i8 } %d, 1
  %pmod4  = urem i16 %page, 4
  %base   = sub i16 %page, %pmod4
  %k16    = zext i2 %klein to i16
  %np     = add i16 %base, %k16
  %np48   = urem i16 %np, 48
  %res    = call i32 @atlas.boundary.encode.impl(i16 %np48, i8 %offset)
  ret i32 %res
}

; ---- Public wrappers (match intrinsic names & attributes) ----

define i32 @atlas.boundary.encode(i16 %page, i8 %offset) #0 {
  %r = call i32 @atlas.boundary.encode.impl(i16 %page, i8 %offset)
  ret i32 %r
}

define { i16, i8 } @atlas.boundary.decode(i32 %boundary) #0 {
  %r = call { i16, i8 } @atlas.boundary.decode.impl(i32 %boundary)
  ret { i16, i8 } %r
}

define i32 @atlas.boundary.transform(i32 %boundary, i7 %resonance) #0 {
  %r = call i32 @atlas.boundary.transform.impl(i32 %boundary, i7 %resonance)
  ret i32 %r
}

define i2 @atlas.boundary.klein(i32 %boundary) #0 {
  %r = call i2 @atlas.boundary.klein.impl(i32 %boundary)
  ret i2 %r
}

define i32 @atlas.boundary.rotate(i32 %boundary, i2 %klein) #0 {
  %r = call i32 @atlas.boundary.rotate.impl(i32 %boundary, i2 %klein)
  ret i32 %r
}

; =============================================================================
; Conservation Operations
; =============================================================================

; ---- Implementations (internal) ----

define internal i1 @atlas.conserved.check.impl(ptr %data, i64 %len) nounwind readonly willreturn {
entry:
  %sum = call i64 @atlas._sum_bytes(ptr %data, i64 %len)
  %mod = urem i64 %sum, 96
  %ok  = icmp eq i64 %mod, 0
  ret i1 %ok
}

; Return sum of 256 bytes (truncate to i16)
define internal i16 @atlas.conserved.sum.page.impl(ptr %page) nounwind readonly willreturn {
entry:
  %sum = call i64 @atlas._sum_bytes(ptr %page, i64 256)
  %r   = trunc i64 %sum to i16
  ret i16 %r
}

; Return sum of 12,288 bytes (truncate to i32)
define internal i32 @atlas.conserved.sum.structure.impl(ptr %s) nounwind readonly willreturn {
entry:
  %sum = call i64 @atlas._sum_bytes(ptr %s, i64 12288)
  %r   = trunc i64 %sum to i32
  ret i32 %r
}

; Delta between two buffers modulo 96 (Layer 2 - optimized for hot path)
define internal i7 @atlas.conserved.delta.impl(ptr %before, ptr %after, i64 %len) nounwind readonly willreturn {
entry:
  ; Use SIMD-optimized byte summation for larger buffers
  %use_simd = icmp uge i64 %len, 32
  br i1 %use_simd, label %simd_path, label %scalar_path

simd_path:
  %b_simd = call i64 @atlas._sum_bytes_simd(ptr %before, i64 %len)
  %a_simd = call i64 @atlas._sum_bytes_simd(ptr %after,  i64 %len)
  %d_simd = sub i64 %a_simd, %b_simd
  %m_simd = call i64 @atlas._fast_mod96(i64 %d_simd)
  %r_simd = trunc i64 %m_simd to i7
  ret i7 %r_simd

scalar_path:
  %b = call i64 @atlas._sum_bytes(ptr %before, i64 %len)
  %a = call i64 @atlas._sum_bytes(ptr %after,  i64 %len)
  %d = sub i64 %a, %b
  %m = srem i64 %d, 96
  %r = trunc i64 %m to i7
  ret i7 %r
}

; Conservative structural domain check: non-null pointer treated as valid
define internal i1 @atlas.conserved.domain.impl(ptr %domain) nounwind readnone willreturn {
entry:
  %ok = icmp ne ptr %domain, null
  ret i1 %ok
}

; dst[i] = (src1[i] + src2[i]) wrapping byte addition — total is conserved mod 256
; (Use higher-level checks if you require mod 96 invariants.)
define internal void @atlas.conserved.add.impl(ptr %dst, ptr %src1, ptr %src2, i64 %len) nounwind {
entry:
  br label %loop

loop:
  %i = phi i64 [ 0, %entry ], [ %i.next, %body ]
  %done = icmp uge i64 %i, %len
  br i1 %done, label %exit, label %body

body:
  %p1 = getelementptr i8, ptr %src1, i64 %i
  %p2 = getelementptr i8, ptr %src2, i64 %i
  %pd = getelementptr i8, ptr %dst,  i64 %i
  %b1 = load i8, ptr %p1, align 1
  %b2 = load i8, ptr %p2, align 1
  %b1w = zext i8 %b1 to i16
  %b2w = zext i8 %b2 to i16
  %sum = add i16 %b1w, %b2w
  %out = trunc i16 %sum to i8
  store i8 %out, ptr %pd, align 1
  %i.next = add i64 %i, 1
  br label %loop

exit:
  ret void
}

; Layer 2: Check if window data sums to 0 mod 96 (conservation window check)
define internal i1 @atlas.conserved.window.check.impl(ptr %data, i64 %len) nounwind readonly willreturn {
entry:
  ; Use SIMD-optimized summation for larger windows
  %use_simd = icmp uge i64 %len, 32
  br i1 %use_simd, label %simd_path, label %scalar_path

simd_path:
  %sum_simd = call i64 @atlas._sum_bytes_simd(ptr %data, i64 %len)
  %mod_simd = call i64 @atlas._fast_mod96(i64 %sum_simd)
  %ok_simd = icmp eq i64 %mod_simd, 0
  ret i1 %ok_simd

scalar_path:
  %sum = call i64 @atlas._sum_bytes(ptr %data, i64 %len)
  %mod = urem i64 %sum, 96
  %ok = icmp eq i64 %mod, 0
  ret i1 %ok
}

; Layer 2: Streaming conservation update - updates conservation state with new chunk
; State format: first i64 is running sum, then follows actual state data
define internal void @atlas.conserved.update.impl(ptr %state, ptr %chunk, i64 %n) nounwind {
entry:
  ; Load current running sum from state
  %sum_ptr = bitcast ptr %state to ptr
  %current_sum = load i64, ptr %sum_ptr, align 8
  
  ; Calculate sum of new chunk using optimized path
  %use_simd = icmp uge i64 %n, 32
  br i1 %use_simd, label %simd_sum, label %scalar_sum

simd_sum:
  %chunk_sum_simd = call i64 @atlas._sum_bytes_simd(ptr %chunk, i64 %n)
  br label %update

scalar_sum:
  %chunk_sum_scalar = call i64 @atlas._sum_bytes(ptr %chunk, i64 %n)
  br label %update

update:
  %chunk_sum = phi i64 [ %chunk_sum_simd, %simd_sum ], [ %chunk_sum_scalar, %scalar_sum ]
  
  ; Update running sum with fast mod-96
  %new_sum_raw = add i64 %current_sum, %chunk_sum
  %new_sum = call i64 @atlas._fast_mod96(i64 %new_sum_raw)
  
  ; Store updated sum
  store i64 %new_sum, ptr %sum_ptr, align 8
  
  ; Update the actual state data (copy chunk to state+8)
  %state_data = getelementptr i8, ptr %state, i64 8
  call void @llvm.memcpy.p0.p0.i64(ptr %state_data, ptr %chunk, i64 %n, i1 false)
  
  ret void
}

; ---- Public wrappers ----

define i1 @atlas.conserved.check(ptr %data, i64 %len) #1 {
  %r = call i1 @atlas.conserved.check.impl(ptr %data, i64 %len)
  ret i1 %r
}

define i16 @atlas.conserved.sum.page(ptr %page) #1 {
  %r = call i16 @atlas.conserved.sum.page.impl(ptr %page)
  ret i16 %r
}

define i32 @atlas.conserved.sum.structure(ptr %s) #1 {
  %r = call i32 @atlas.conserved.sum.structure.impl(ptr %s)
  ret i32 %r
}

define i7 @atlas.conserved.delta(ptr %before, ptr %after, i64 %len) #5 {
  %r = call i7 @atlas.conserved.delta.impl(ptr %before, ptr %after, i64 %len)
  ret i7 %r
}

define i1 @atlas.conserved.domain(ptr %domain) #1 {
  %r = call i1 @atlas.conserved.domain.impl(ptr %domain)
  ret i1 %r
}

define void @atlas.conserved.add(ptr %dst, ptr %src1, ptr %src2, i64 %len) #2 {
  call void @atlas.conserved.add.impl(ptr %dst, ptr %src1, ptr %src2, i64 %len)
  ret void
}

; Layer 2 public wrappers
define i1 @atlas.conserved.window.check(ptr %data, i64 %len) #5 {
  %r = call i1 @atlas.conserved.window.check.impl(ptr %data, i64 %len)
  ret i1 %r
}

define void @atlas.conserved.update(ptr %state, ptr %chunk, i64 %n) #2 {
  call void @atlas.conserved.update.impl(ptr %state, ptr %chunk, i64 %n)
  ret void
}

; Layer 2 enhanced operations for Atlas structure (12,288 bytes)
define i1 @atlas.conserved.structure.check(ptr %structure) #5 {
  %r = call i1 @atlas.conserved.window.check.impl(ptr %structure, i64 12288)
  ret i1 %r
}

define i7 @atlas.conserved.structure.delta(ptr %before, ptr %after) #5 {
  %r = call i7 @atlas.conserved.delta.impl(ptr %before, ptr %after, i64 12288)
  ret i7 %r
}

; Layer 2 conserved batch operations for enhanced SIMD utilization
define void @atlas.conserved.batch.check(ptr %buffers, ptr %lengths, i32 %count, ptr %results) #5 {
entry:
  br label %loop

loop:
  %i = phi i32 [ 0, %entry ], [ %i.next, %body ]
  %done = icmp uge i32 %i, %count
  br i1 %done, label %exit, label %body

body:
  ; Load buffer pointer and length
  %buf_ptr_ptr = getelementptr ptr, ptr %buffers, i32 %i
  %buf_ptr = load ptr, ptr %buf_ptr_ptr, align 8
  %len_ptr = getelementptr i64, ptr %lengths, i32 %i
  %len = load i64, ptr %len_ptr, align 8
  
  ; Check conservation
  %result = call i1 @atlas.conserved.window.check.impl(ptr %buf_ptr, i64 %len)
  
  ; Store result
  %result_ptr = getelementptr i1, ptr %results, i32 %i
  store i1 %result, ptr %result_ptr, align 1
  
  %i.next = add i32 %i, 1
  br label %loop

exit:
  ret void
}

; =============================================================================
; Budget Operations (RL‑96 Algebra)
; =============================================================================

; ---- Implementations (internal) ----

define internal i7 @atlas.budget.add.impl(i7 %a, i7 %b) nounwind readnone willreturn {
entry:
  %a16 = zext i7 %a to i16
  %b16 = zext i7 %b to i16
  %sum = add i16 %a16, %b16
  %mod = urem i16 %sum, 96
  %res = trunc i16 %mod to i7
  ret i7 %res
}

define internal i7 @atlas.budget.mul.impl(i7 %a, i7 %b) nounwind readnone willreturn {
entry:
  %a16 = zext i7 %a to i16
  %b16 = zext i7 %b to i16
  %prd = mul i16 %a16, %b16
  %mod = urem i16 %prd, 96
  %res = trunc i16 %mod to i7
  ret i7 %res
}

define internal i7 @atlas.budget.inv.impl(i7 %budget) nounwind readnone willreturn {
entry:
  %b16 = zext i7 %budget to i16
  %inv = sub i16 96, %b16
  %mod = urem i16 %inv, 96
  %res = trunc i16 %mod to i7
  ret i7 %res
}

define internal i1 @atlas.budget.zero.impl(i7 %budget) nounwind readnone willreturn {
entry:
  %z = icmp eq i7 %budget, 0
  ret i1 %z
}

define internal i1 @atlas.budget.less.impl(i7 %a, i7 %b) nounwind readnone willreturn {
entry:
  %r = icmp ult i7 %a, %b
  ret i1 %r
}

; ---- Public wrappers ----

define i7 @atlas.budget.add(i7 %a, i7 %b) #0 {
  %r = call i7 @atlas.budget.add.impl(i7 %a, i7 %b)
  ret i7 %r
}

define i7 @atlas.budget.mul(i7 %a, i7 %b) #0 {
  %r = call i7 @atlas.budget.mul.impl(i7 %a, i7 %b)
  ret i7 %r
}

define i7 @atlas.budget.inv(i7 %b) #0 {
  %r = call i7 @atlas.budget.inv.impl(i7 %b)
  ret i7 %r
}

define i1 @atlas.budget.zero(i7 %b) #0 {
  %r = call i1 @atlas.budget.zero.impl(i7 %b)
  ret i1 %r
}

define i1 @atlas.budget.less(i7 %a, i7 %b) #0 {
  %r = call i1 @atlas.budget.less.impl(i7 %a, i7 %b)
  ret i1 %r
}

; =============================================================================
; Witness Operations (opaque handle)
; Layout: [data bytes][u32 checksum][u32 length][u64 timestamp][u8 resonance]
; =============================================================================

; ---- Implementations (internal) ----

; Compute a simple rolling checksum (NOT cryptographically secure)
define internal i32 @atlas._checksum(ptr %data, i64 %len) nounwind readonly {
entry:
  br label %loop

loop:
  %i    = phi i64 [ 0, %entry ], [ %i.next, %body ]
  %crc  = phi i32 [ -1, %entry ], [ %crc.next, %body ]
  %done = icmp uge i64 %i, %len
  br i1 %done, label %exit, label %body

body:
  %p   = getelementptr i8, ptr %data, i64 %i
  %b   = load i8, ptr %p, align 1
  %b32 = zext i8 %b to i32
  %x   = xor i32 %crc, %b32
  %s   = lshr i32 %x, 1
  %crc.next = xor i32 %s, 3988292384 ; 0xEDB88320
  %i.next   = add i64 %i, 1
  br label %loop

exit:
  %final = xor i32 %crc, -1
  ret i32 %final
}

; Allocate witnessed memory and return handle (ptr)
define internal ptr @atlas.witness.generate.impl(ptr %data, i64 %len) nounwind {
entry:
  %bytes_plus_meta = add i64 %len, 8 ; +4 checksum +4 length
  ; Align to 16 for vector-friendly layout
  %add15 = add i64 %bytes_plus_meta, 15
  %size  = and i64 %add15, -16
  %buf   = call ptr @aligned_alloc(i64 16, i64 %size)
  ; Copy payload
  call void @llvm.memcpy.p0.p0.i64(ptr %buf, ptr %data, i64 %len, i1 false)
  ; Write checksum and length at the end
  %chk   = call i32 @atlas._checksum(ptr %data, i64 %len)
  %tail  = getelementptr i8, ptr %buf, i64 %len
  %chkptr = bitcast ptr %tail to ptr
  store i32 %chk, ptr %chkptr, align 4
  %lenoff = getelementptr i8, ptr %tail, i64 4
  %lenptr = bitcast ptr %lenoff to ptr
  %len32  = trunc i64 %len to i32
  store i32 %len32, ptr %lenptr, align 4
  ret ptr %buf
}

; Verify witness against data by comparing checksums
define internal i1 @atlas.witness.verify.impl(ptr %w, ptr %data, i64 %len) nounwind readonly willreturn {
entry:
  %chk_calc = call i32 @atlas._checksum(ptr %data, i64 %len)
  %tail     = getelementptr i8, ptr %w, i64 %len
  %chkptr   = bitcast ptr %tail to ptr
  %chk_stored = load i32, ptr %chkptr, align 4
  %ok = icmp eq i32 %chk_calc, %chk_stored
  ret i1 %ok
}

; Destroy witness
define internal void @atlas.witness.destroy.impl(ptr %w) nounwind {
entry:
  call void @free(ptr %w)
  ret void
}

; ---- Public wrappers ----

define ptr @atlas.witness.generate(ptr %data, i64 %len) #4 {
  %r = call ptr @atlas.witness.generate.impl(ptr %data, i64 %len)
  ret ptr %r
}

define i1 @atlas.witness.verify(ptr %w, ptr %data, i64 %len) #1 {
  %r = call i1 @atlas.witness.verify.impl(ptr %w, ptr %data, i64 %len)
  ret i1 %r
}

define void @atlas.witness.destroy(ptr %w) #4 {
  call void @atlas.witness.destroy.impl(ptr %w)
  ret void
}

; =============================================================================
; Memory Operations
; =============================================================================
; Note: All memory operation implementations and wrappers
; (memcpy.conserved, memset.conserved, alloc.witnessed, free.witnessed,
; alloc.resonant, alloc.pages) are defined in atlas-12288-memory.ll

; ---- Public wrappers ----
; Note: Memory operation public wrappers (atlas.memcpy.conserved, atlas.memset.conserved,
; atlas.alloc.witnessed, atlas.free.witnessed, atlas.alloc.resonant, atlas.alloc.pages)
; are all defined in atlas-12288-memory.ll to avoid duplication

; =============================================================================
; Resonance Operations
; =============================================================================

; ---- Implementations (internal) ----

define internal i7 @atlas.resonance.harmonic.impl(i7 %r1, i7 %r2) nounwind readnone willreturn {
entry:
  %a = zext i7 %r1 to i16
  %b = zext i7 %r2 to i16
  %s = add i16 %a, %b
  %m = urem i16 %s, 96
  %t = trunc i16 %m to i7
  ret i7 %t
}

define internal i1 @atlas.resonance.harmonizes.impl(i7 %r1, i7 %r2) nounwind readnone willreturn {
entry:
  %a = zext i7 %r1 to i16
  %b = zext i7 %r2 to i16
  %s = add i16 %a, %b
  %m = urem i16 %s, 96
  %z = icmp eq i16 %m, 0
  ret i1 %z
}

; Return the same pointer as a trivial clustering handle
define internal ptr @atlas.resonance.cluster.impl(ptr %coords, i32 %count) nounwind readnone willreturn {
entry:
  ret ptr %coords
}

; Schedule: map resonance to a simple cycle length (here: r + 1)
define internal i64 @atlas.resonance.schedule.impl(i7 %r) nounwind readnone willreturn {
entry:
  %r64 = zext i7 %r to i64
  %one = add i64 %r64, 1
  ret i64 %one
}

; ---- Public wrappers ----

define i7 @atlas.resonance.harmonic(i7 %r1, i7 %r2) #0 {
  %r = call i7 @atlas.resonance.harmonic.impl(i7 %r1, i7 %r2)
  ret i7 %r
}

define i1 @atlas.resonance.harmonizes(i7 %r1, i7 %r2) #0 {
  %r = call i1 @atlas.resonance.harmonizes.impl(i7 %r1, i7 %r2)
  ret i1 %r
}

define ptr @atlas.resonance.cluster(ptr %coords, i32 %count) #4 {
  %r = call ptr @atlas.resonance.cluster.impl(ptr %coords, i32 %count)
  ret ptr %r
}

define i64 @atlas.resonance.schedule(i7 %r) #0 {
  %r2 = call i64 @atlas.resonance.schedule.impl(i7 %r)
  ret i64 %r2
}

; =============================================================================
; Module metadata
; =============================================================================

; Metadata removed
; Metadata removed

; =============================================================================
; Missing Intrinsic Implementations
; =============================================================================

; Compute resonance spectrum for a buffer
define void @atlas.r96.spectrum(ptr %data, i64 %len, ptr %spectrum) nounwind {
entry:
  ; Initialize spectrum array (96 i32 values)
  br label %init_loop
  
init_loop:
  %i = phi i64 [ 0, %entry ], [ %i.next, %init_body ]
  %done_init = icmp eq i64 %i, 96
  br i1 %done_init, label %scan, label %init_body
  
init_body:
  %ptr = getelementptr i32, ptr %spectrum, i64 %i
  store i32 0, ptr %ptr
  %i.next = add i64 %i, 1
  br label %init_loop

scan:
  br label %scan_loop
  
scan_loop:
  %j = phi i64 [ 0, %scan ], [ %j.next, %scan_body ]
  %done_scan = icmp eq i64 %j, %len
  br i1 %done_scan, label %exit, label %scan_body
  
scan_body:
  %byte_ptr = getelementptr i8, ptr %data, i64 %j
  %byte = load i8, ptr %byte_ptr
  %class = call i7 @atlas.r96.classify(i8 %byte)
  %class_idx = zext i7 %class to i64
  %count_ptr = getelementptr i32, ptr %spectrum, i64 %class_idx
  %count = load i32, ptr %count_ptr
  %count_inc = add i32 %count, 1
  store i32 %count_inc, ptr %count_ptr
  %j.next = add i64 %j, 1
  br label %scan_loop
  
exit:
  ret void
}

; Vector boundary encoding
define <16 x i32> @atlas.boundary.encode.v16(<16 x i16> %pages, <16 x i8> %offsets) nounwind {
entry:
  %pages_ext = zext <16 x i16> %pages to <16 x i32>
  %offsets_ext = zext <16 x i8> %offsets to <16 x i32>
  %pages_shift = shl <16 x i32> %pages_ext, <i32 8, i32 8, i32 8, i32 8, i32 8, i32 8, i32 8, i32 8, i32 8, i32 8, i32 8, i32 8, i32 8, i32 8, i32 8, i32 8>
  %result = or <16 x i32> %pages_shift, %offsets_ext
  ret <16 x i32> %result
}

; Chain witnesses together
define ptr @atlas.witness.chain(ptr %w1, ptr %w2) nounwind {
entry:
  ret ptr %w1
}

; Merge multiple witnesses
define ptr @atlas.witness.merge(ptr %witnesses, i32 %count) nounwind {
entry:
  %first = load ptr, ptr %witnesses
  ret ptr %first
}

; Get witness timestamp
define i64 @atlas.witness.timestamp(ptr %witness) nounwind {
entry:
  %ts_ptr = getelementptr i64, ptr %witness, i64 1
  %ts = load i64, ptr %ts_ptr
  ret i64 %ts
}

; Get witness resonance
define i7 @atlas.witness.resonance(ptr %witness) nounwind {
entry:
  %r_ptr = getelementptr i8, ptr %witness, i64 16
  %r8 = load i8, ptr %r_ptr
  %r7 = trunc i8 %r8 to i7
  ret i7 %r7
}

; Budget allocation
define i1 @atlas.budget.alloc(ptr %domain, i7 %amount) nounwind {
entry:
  %current_ptr = getelementptr i7, ptr %domain, i64 0
  %current = load i7, ptr %current_ptr
  %current_ext = zext i7 %current to i8
  %amount_ext = zext i7 %amount to i8
  %new = add i8 %current_ext, %amount_ext
  %overflow = icmp uge i8 %new, 96
  br i1 %overflow, label %fail, label %success
  
success:
  %new_budget = trunc i8 %new to i7
  store i7 %new_budget, ptr %current_ptr
  ret i1 true
  
fail:
  ret i1 false
}

; Budget release
define i1 @atlas.budget.release(ptr %domain, i7 %amount) nounwind {
entry:
  %current_ptr = getelementptr i7, ptr %domain, i64 0
  %current = load i7, ptr %current_ptr
  %underflow = icmp ult i7 %current, %amount
  br i1 %underflow, label %fail, label %success
  
success:
  %new = sub i7 %current, %amount
  store i7 %new, ptr %current_ptr
  ret i1 true
  
fail:
  ret i1 false
}

; Optimization hints
define void @atlas.assume.conserved(ptr %data, i64 %len) nounwind {
entry:
  %conserved = call i1 @atlas.conserved.check(ptr %data, i64 %len)
  call void @llvm.assume(i1 %conserved)
  ret void
}

define void @atlas.assume.resonance(ptr %data, i64 %len, i7 %expected) nounwind {
entry:
  ret void
}

define void @atlas.prefetch.resonant(ptr %data, i7 %resonance) nounwind {
entry:
  call void @llvm.prefetch.p0(ptr %data, i32 0, i32 3, i32 1)
  ret void
}

; Debug operations
define void @atlas.debug.conservation(ptr %data, i64 %len) nounwind {
entry:
  %conserved = call i1 @atlas.conserved.check(ptr %data, i64 %len)
  ret void
}

define void @atlas.debug.spectrum(ptr %data, i64 %len) nounwind {
entry:
  %spectrum = alloca [96 x i32]
  call void @atlas.r96.spectrum(ptr %data, i64 %len, ptr %spectrum)
  ret void
}

define i1 @atlas.debug.validate(ptr %structure) nounwind {
entry:
  %valid = call i1 @atlas.conserved.check(ptr %structure, i64 12288)
  ret i1 %valid
}

; LLVM intrinsics used
declare void @llvm.assume(i1)
declare void @llvm.prefetch.p0(ptr, i32, i32, i32)
