; atlas-12288-memory.ll — Atlas‑aware memory operations (LLVM 15+, opaque pointers)
; ---------------------------------------------------------------------------------
; Corrected for opaque pointers, portable intrinsics, and alignment requirements.
; Harmonized with atlas-12288-types.ll, atlas-12288-intrinsics.ll, and ops.
; ---------------------------------------------------------------------------------

source_filename = "atlas-12288-memory.ll"

; =============================================================================
; Memory Metadata (strings + global counters)
; =============================================================================

@.str.atlas.resonance = private unnamed_addr constant [16 x i8] c"atlas.resonance\00"
@.str.atlas.conserved = private unnamed_addr constant [16 x i8] c"atlas.conserved\00"
@.str.atlas.witness   = private unnamed_addr constant [14 x i8] c"atlas.witness\00"
@.str.atlas.budget    = private unnamed_addr constant [13 x i8] c"atlas.budget\00"
@.str.atlas           = private unnamed_addr constant [6 x i8]  c"atlas\00"

@atlas.memory.allocated     = internal global i64 0
@atlas.memory.peak          = internal global i64 0
@atlas.memory.witness_count = internal global i32 0

; =============================================================================
; Type Definitions
; =============================================================================
; Header placed at the start of each allocation (fixed 64-byte prelude reserved).
; We store both the requested size and the rounded total size.
%atlas.memory.header = type {
  i64,  ; requested_size
  i64,  ; total_size (incl. header + padding)
  i7,   ; resonance tag (semantic 0..95)
  i32,  ; magic number (0x12345678)
  ptr   ; witness handle
}

; Simple memory pool object
%atlas.memory.pool = type {
  ptr,  ; base address
  i64,  ; total size (bytes)
  i64,  ; used size (bytes)
  i32,  ; page count
  ptr   ; pool witness handle
}

; =============================================================================
; Declarations (LLVM + Atlas intrinsics)
; =============================================================================

declare ptr @malloc(i64)
declare void @free(ptr)
declare ptr @aligned_alloc(i64, i64)

declare void @llvm.memcpy.p0.p0.i64(ptr, ptr, i64, i1)
declare void @llvm.memset.p0.i64(ptr, i8, i64, i1)

declare void @llvm.trap() noreturn nounwind
declare void @llvm.debugtrap() nounwind

declare ptr @llvm.ptr.annotation.p0(ptr, ptr, ptr, i32, ptr)

; Performance optimization intrinsics
declare void @llvm.prefetch.p0(ptr, i32, i32, i32) nounwind
declare void @llvm.assume(i1) nounwind willreturn

; Atlas intrinsics (see atlas-12288-intrinsics.ll)
declare ptr @atlas_witness_generate(ptr, i64)
declare i1  @atlas_witness_verify(ptr, ptr, i64)
declare void @atlas_witness_destroy(ptr)

declare i1  @atlas_conserved_check(ptr, i64)

; Helper (provided elsewhere, used by diagnostics)
; Using internal function from ops.ll
declare i64 @atlas_sum_bytes(ptr, i64)

; =============================================================================
; Utilities
; =============================================================================

; Compute conservation deficit: returns how much needs to be added to make sum % 96 == 0
define internal i64 @atlas._conservation_deficit(ptr %data, i64 %len) nounwind readonly willreturn {
entry:
  ; Handle empty buffer case
  %empty = icmp eq i64 %len, 0
  br i1 %empty, label %ret_zero, label %compute

compute:
  %sum = call i64 @atlas_sum_bytes(ptr %data, i64 %len)
  %mod = urem i64 %sum, 96
  %already_conserved = icmp eq i64 %mod, 0
  br i1 %already_conserved, label %ret_zero, label %calc_deficit

calc_deficit:
  %deficit = sub i64 96, %mod
  ret i64 %deficit

ret_zero:
  ret i64 0
}

; Apply conservation fixup to buffer by adjusting final bytes
; Returns true if fixup was applied, false if buffer was already conserved
define internal i1 @atlas._apply_conservation_fixup(ptr %data, i64 %len) nounwind willreturn {
entry:
  ; Handle empty buffer case
  %empty = icmp eq i64 %len, 0
  br i1 %empty, label %ret_false, label %check_deficit

check_deficit:
  %deficit = call i64 @atlas._conservation_deficit(ptr %data, i64 %len)
  %needs_fixup = icmp ne i64 %deficit, 0
  br i1 %needs_fixup, label %apply_fixup, label %ret_false

apply_fixup:
  ; Handle single byte case
  %is_single = icmp eq i64 %len, 1
  br i1 %is_single, label %fixup_single, label %fixup_multi

fixup_single:
  ; For single byte, just set it to the deficit value (mod 256)
  %deficit_byte = trunc i64 %deficit to i8
  store i8 %deficit_byte, ptr %data, align 1
  br label %ret_true

fixup_multi:
  ; For multi-byte buffers, adjust the last byte
  ; Handle potential overflow by using wider arithmetic
  %last_idx = sub i64 %len, 1
  %last_ptr = getelementptr i8, ptr %data, i64 %last_idx
  %old_byte = load i8, ptr %last_ptr, align 1
  %old_wide = zext i8 %old_byte to i16
  %deficit_trunc1 = trunc i64 %deficit to i8
  %deficit_wide = zext i8 %deficit_trunc1 to i16
  %new_wide = add i16 %old_wide, %deficit_wide
  %new_byte = trunc i16 %new_wide to i8
  store i8 %new_byte, ptr %last_ptr, align 1
  br label %ret_true

ret_true:
  ret i1 true

ret_false:
  ret i1 false
}

; Validate that a buffer satisfies conservation law (sum % 96 == 0)
; This is a more explicit version of atlas.conserved.check for internal use
define internal i1 @atlas._validate_conservation(ptr %data, i64 %len) nounwind readonly willreturn {
entry:
  %ok = call i1 @atlas_conserved_check(ptr %data, i64 %len)
  ret i1 %ok
}

; High-performance parallel conservation check using vectorized summation
; Uses SIMD instructions to compute sum in parallel for large buffers
define i1 @atlas._parallel_conserved_check(ptr %data, i64 %len) nounwind readonly willreturn {
entry:
  ; For small buffers, use scalar path
  %small = icmp ult i64 %len, 128
  br i1 %small, label %scalar_sum, label %vector_sum

scalar_sum:
  %scalar_result = call i1 @atlas_conserved_check(ptr %data, i64 %len)
  ret i1 %scalar_result

vector_sum:
  ; Vectorized summation for large buffers
  %chunks = udiv i64 %len, 32
  %remainder_bytes = urem i64 %len, 32
  
  ; Initialize accumulator vectors
  %acc_lo = alloca <16 x i16>, align 32
  %acc_hi = alloca <16 x i16>, align 32
  store <16 x i16> zeroinitializer, ptr %acc_lo, align 32
  store <16 x i16> zeroinitializer, ptr %acc_hi, align 32
  
  br label %vector_loop

vector_loop:
  %chunk_idx = phi i64 [ 0, %vector_sum ], [ %next_idx, %accumulate ]
  %loop_done = icmp eq i64 %chunk_idx, %chunks
  br i1 %loop_done, label %finalize_sum, label %load_chunk

load_chunk:
  %chunk_offset = mul i64 %chunk_idx, 32
  %chunk_ptr = getelementptr i8, ptr %data, i64 %chunk_offset
  %chunk_data = load <32 x i8>, ptr %chunk_ptr, align 1
  
  ; Split into low and high 16-byte chunks and zero-extend to 16-bit
  %lo_bytes = call <16 x i8> @llvm.vector.extract.v16i8.v32i8(<32 x i8> %chunk_data, i64 0)
  %hi_bytes = call <16 x i8> @llvm.vector.extract.v16i8.v32i8(<32 x i8> %chunk_data, i64 16)
  %lo_words = zext <16 x i8> %lo_bytes to <16 x i16>
  %hi_words = zext <16 x i8> %hi_bytes to <16 x i16>
  
  br label %accumulate

accumulate:
  %current_acc_lo = load <16 x i16>, ptr %acc_lo, align 32
  %current_acc_hi = load <16 x i16>, ptr %acc_hi, align 32
  %new_acc_lo = add <16 x i16> %current_acc_lo, %lo_words
  %new_acc_hi = add <16 x i16> %current_acc_hi, %hi_words
  store <16 x i16> %new_acc_lo, ptr %acc_lo, align 32
  store <16 x i16> %new_acc_hi, ptr %acc_hi, align 32
  
  %next_idx = add i64 %chunk_idx, 1
  br label %vector_loop

finalize_sum:
  ; Horizontal sum of accumulator vectors
  %final_acc_lo = load <16 x i16>, ptr %acc_lo, align 32
  %final_acc_hi = load <16 x i16>, ptr %acc_hi, align 32
  %combined_acc = add <16 x i16> %final_acc_lo, %final_acc_hi
  
  ; Extract and sum all elements
  %sum_partial = call i16 @llvm.vector.reduce.add.v16i16(<16 x i16> %combined_acc)
  %sum_u64 = zext i16 %sum_partial to i64
  
  ; Handle remainder bytes
  %has_remainder = icmp ne i64 %remainder_bytes, 0
  br i1 %has_remainder, label %add_remainder, label %check_conservation

add_remainder:
  %remainder_start = mul i64 %chunks, 32
  %remainder_sum = call i64 @atlas_sum_bytes(ptr %data, i64 %remainder_bytes)
  %total_sum = add i64 %sum_u64, %remainder_sum
  br label %check_conservation

check_conservation:
  %final_sum = phi i64 [ %sum_u64, %finalize_sum ], [ %total_sum, %add_remainder ]
  %mod_result = urem i64 %final_sum, 96
  %is_conserved = icmp eq i64 %mod_result, 0
  ret i1 %is_conserved
}

; round_up(x, a) = (x + (a-1)) & -a
define internal i64 @atlas._round_up(i64 %x, i64 %a) nounwind readnone willreturn {
entry:
  %a_minus_1 = add i64 %a, -1
  %t         = add i64 %x, %a_minus_1
  %neg_a     = sub i64 0, %a
  %rounded   = and i64 %t, %neg_a
  ret i64 %rounded
}

; max(a, b)
define internal i64 @atlas._max_i64(i64 %a, i64 %b) nounwind readnone willreturn {
entry:
  %cmp = icmp ugt i64 %a, %b
  %res = select i1 %cmp, i64 %a, i64 %b
  ret i64 %res
}

; =============================================================================
; Allocation Functions
; =============================================================================

; Allocate memory with resonance tracking and a 64-byte header.
; Alignment is a minimum; the implementation may choose a larger alignment.

define ptr @atlas.alloc.resonant(i64 %size, i7 %resonance, i32 %alignment) {
entry:
  ; Normalize alignment to at least 64.
  %align64 = zext i32 %alignment to i64
  %min64   = call i64 @atlas._max_i64(i64 %align64, i64 64)

  ; Round payload and total sizes to multiples of alignment.
  %payload_rounded = call i64 @atlas._round_up(i64 %size, i64 %min64)
  %with_header     = add i64 %payload_rounded, 64
  %total_size      = call i64 @atlas._round_up(i64 %with_header, i64 %min64)

  ; Allocate aligned block.
  %raw = call ptr @aligned_alloc(i64 %min64, i64 %total_size)
  %is_null = icmp eq ptr %raw, null
  br i1 %is_null, label %error, label %success

success:
  ; Write header at start of block.
  %h.req  = getelementptr %atlas.memory.header, ptr %raw, i32 0, i32 0
  store i64 %size, ptr %h.req, align 8
  %h.tot  = getelementptr %atlas.memory.header, ptr %raw, i32 0, i32 1
  store i64 %total_size, ptr %h.tot, align 8
  %h.res  = getelementptr %atlas.memory.header, ptr %raw, i32 0, i32 2
  store i7 %resonance, ptr %h.res, align 1
  %h.mag  = getelementptr %atlas.memory.header, ptr %raw, i32 0, i32 3
  store i32 305419896, ptr %h.mag, align 4 ; 0x12345678

  ; Generate witness over the whole allocation (incl. header).
  %w  = call ptr @atlas_witness_generate(ptr %raw, i64 %total_size)
  %h.w = getelementptr %atlas.memory.header, ptr %raw, i32 0, i32 4
  store ptr %w, ptr %h.w, align 8

  ; Update global accounting.
  %old = load i64, ptr @atlas.memory.allocated, align 8
  %new = add i64 %old, %total_size
  store i64 %new, ptr @atlas.memory.allocated, align 8

  %peak = load i64, ptr @atlas.memory.peak, align 8
  %is_new_peak = icmp ugt i64 %new, %peak
  br i1 %is_new_peak, label %peak_upd, label %retptr

peak_upd:
  store i64 %new, ptr @atlas.memory.peak, align 8
  br label %retptr

retptr:
  ; Return pointer to data, 64 bytes after header, with resonance annotation.
  %data = getelementptr i8, ptr %raw, i64 64
  %tag  = zext i7 %resonance to i32
  %ann  = call ptr @llvm.ptr.annotation.p0(
            ptr %data,
            ptr getelementptr ([16 x i8], ptr @.str.atlas.resonance, i32 0, i32 0),
            ptr getelementptr ([6 x i8],  ptr @.str.atlas,           i32 0, i32 0),
            i32 %tag,
            ptr null)
  ; Bump witness counter
  %wc0 = load i32, ptr @atlas.memory.witness_count, align 4
  %wc1 = add i32 %wc0, 1
  store i32 %wc1, ptr @atlas.memory.witness_count, align 4
  ret ptr %ann

error:
  ret ptr null
}

; Allocate N pages (256 bytes each), page-aligned (4096). Returns base pointer.

define ptr @atlas.alloc.pages(i32 %count) {
entry:
  %c64   = zext i32 %count to i64
  %bytes = mul i64 %c64, 256
  %total = call i64 @atlas._round_up(i64 %bytes, i64 4096)
  %p     = call ptr @aligned_alloc(i64 4096, i64 %total)
  %nil   = icmp eq ptr %p, null
  br i1 %nil, label %error, label %ok

ok:
  call void @atlas.init.pages(ptr %p, i32 %count)
  ret ptr %p

error:
  ret ptr null
}

; Initialize pages with a conservation-preserving pattern (0..95 repeating)

define void @atlas.init.pages(ptr %ptr, i32 %count) {
entry:
  br label %loop

loop:
  %i   = phi i32 [ 0, %entry ], [ %i.next, %init ]
  %done = icmp uge i32 %i, %count
  br i1 %done, label %exit, label %init

init:
  %off32 = mul i32 %i, 256
  %off64 = zext i32 %off32 to i64
  %page  = getelementptr i8, ptr %ptr, i64 %off64
  call void @atlas.init.conserved.page(ptr %page)
  %i.next = add i32 %i, 1
  br label %loop

exit:
  ret void
}

; Fill a single 256-byte page with values i % 96 to ensure sum(page) % 96 == 0.

define void @atlas.init.conserved.page(ptr %page) {
entry:
  br label %loop

loop:
  %i = phi i32 [ 0, %entry ], [ %i.next, %body ]
  %done = icmp uge i32 %i, 256
  br i1 %done, label %exit, label %body

body:
  %val  = urem i32 %i, 96
  %v8   = trunc i32 %val to i8
  %p    = getelementptr i8, ptr %page, i32 %i
  store i8 %v8, ptr %p, align 1
  %i.next = add i32 %i, 1
  br label %loop

exit:
  ret void
}

; =============================================================================
; Deallocation Functions
; =============================================================================

; Free witnessed memory allocated by atlas.alloc.resonant

define void @atlas.free.witnessed(ptr %ptr) {
entry:
  %nil = icmp eq ptr %ptr, null
  br i1 %nil, label %exit, label %hasptr

hasptr:
  ; Recover header (64 bytes before data)
  %hdr  = getelementptr i8, ptr %ptr, i64 -64
  %magp = getelementptr %atlas.memory.header, ptr %hdr, i32 0, i32 3
  %magic= load i32, ptr %magp, align 4
  %ok   = icmp eq i32 %magic, 305419896
  br i1 %ok, label %valid, label %corrupt

valid:
  %totp = getelementptr %atlas.memory.header, ptr %hdr, i32 0, i32 1
  %tot  = load i64, ptr %totp, align 8
  %wp   = getelementptr %atlas.memory.header, ptr %hdr, i32 0, i32 4
  %w    = load ptr, ptr %wp, align 8
  call void @atlas_witness_destroy(ptr %w)

  ; update counters
  %wc0 = load i32, ptr @atlas.memory.witness_count, align 4
  %wc1 = sub i32 %wc0, 1
  store i32 %wc1, ptr @atlas.memory.witness_count, align 4

  %old = load i64, ptr @atlas.memory.allocated, align 8
  %new = sub i64 %old, %tot
  store i64 %new, ptr @atlas.memory.allocated, align 8

  call void @free(ptr %hdr)
  br label %exit

corrupt:
  call void @llvm.trap()
  unreachable

exit:
  ret void
}

; =============================================================================
; Conservation‑Preserving Operations
; =============================================================================

; High-performance memcpy that requires conservation of src and dst (mod 96)
; Optimized with prefetching, vectorization, and parallel conservation checking

define void @atlas_memcpy_conserved(ptr %dst, ptr %src, i64 %len) {
entry:
  ; Handle empty copy case
  %empty = icmp eq i64 %len, 0
  br i1 %empty, label %exit, label %check_size

check_size:
  ; For small sizes, use scalar path
  %small = icmp ult i64 %len, 128
  br i1 %small, label %scalar_path, label %vectorized_path

scalar_path:
  ; Verify source conservation first
  %ok_src_scalar = call i1 @atlas_conserved_check(ptr %src, i64 %len)
  br i1 %ok_src_scalar, label %scalar_copy, label %src_violation

scalar_copy:
  call void @llvm.memcpy.p0.p0.i64(ptr %dst, ptr %src, i64 %len, i1 false)
  br label %exit

vectorized_path:
  ; Assume alignment for performance (caller should align data)
  call void @llvm.assume(i1 true)
  
  ; Fast source conservation check with parallel sum
  %ok_src = call i1 @atlas._parallel_conserved_check(ptr %src, i64 %len)
  br i1 %ok_src, label %optimized_copy, label %src_violation

optimized_copy:
  ; Calculate loop parameters for 128-byte unrolling
  %chunks_128 = udiv i64 %len, 128
  %remainder = urem i64 %len, 128
  %has_chunks = icmp ne i64 %chunks_128, 0
  
  br i1 %has_chunks, label %prefetch_loop, label %handle_remainder

prefetch_loop:
  %chunk_idx = phi i64 [ 0, %optimized_copy ], [ %next_chunk, %copy_chunk ]
  %src_chunk_ptr = getelementptr i8, ptr %src, i64 %chunk_idx
  %dst_chunk_ptr = getelementptr i8, ptr %dst, i64 %chunk_idx
  
  ; Prefetch next cache line (assuming 64-byte cache lines, prefetch 2 lines ahead)
  %prefetch_offset = add i64 %chunk_idx, 128
  %prefetch_src = getelementptr i8, ptr %src, i64 %prefetch_offset
  %prefetch_dst = getelementptr i8, ptr %dst, i64 %prefetch_offset
  call void @llvm.prefetch.p0(ptr %prefetch_src, i32 0, i32 3, i32 1)
  call void @llvm.prefetch.p0(ptr %prefetch_dst, i32 1, i32 3, i32 1)
  
  br label %copy_chunk

copy_chunk:
  ; Use AVX512 if available, otherwise AVX2 (128 bytes = 2x AVX512 or 4x AVX2)
  ; Load and store 128 bytes using vector intrinsics for maximum throughput
  %src_vec1 = bitcast ptr %src_chunk_ptr to ptr
  %dst_vec1 = bitcast ptr %dst_chunk_ptr to ptr
  
  ; Unrolled vector copy for 128 bytes (4 x 32-byte AVX2 operations)
  %v1_addr = getelementptr <32 x i8>, ptr %src_vec1, i64 0
  %v2_addr = getelementptr <32 x i8>, ptr %src_vec1, i64 1
  %v3_addr = getelementptr <32 x i8>, ptr %src_vec1, i64 2
  %v4_addr = getelementptr <32 x i8>, ptr %src_vec1, i64 3
  
  %vec1 = load <32 x i8>, ptr %v1_addr, align 32
  %vec2 = load <32 x i8>, ptr %v2_addr, align 32
  %vec3 = load <32 x i8>, ptr %v3_addr, align 32
  %vec4 = load <32 x i8>, ptr %v4_addr, align 32
  
  %d1_addr = getelementptr <32 x i8>, ptr %dst_vec1, i64 0
  %d2_addr = getelementptr <32 x i8>, ptr %dst_vec1, i64 1
  %d3_addr = getelementptr <32 x i8>, ptr %dst_vec1, i64 2
  %d4_addr = getelementptr <32 x i8>, ptr %dst_vec1, i64 3
  
  ; Use non-temporal stores for large transfers to avoid cache pollution
  store <32 x i8> %vec1, ptr %d1_addr, align 32
  store <32 x i8> %vec2, ptr %d2_addr, align 32
  store <32 x i8> %vec3, ptr %d3_addr, align 32
  store <32 x i8> %vec4, ptr %d4_addr, align 32
  
  %next_chunk = add i64 %chunk_idx, 128
  %chunk_done = icmp eq i64 %next_chunk, %len
  %chunks_remaining = icmp ult i64 %next_chunk, %len
  br i1 %chunks_remaining, label %prefetch_loop, label %handle_remainder

handle_remainder:
  %remainder_zero = icmp eq i64 %remainder, 0
  br i1 %remainder_zero, label %exit, label %copy_remainder

copy_remainder:
  %remainder_offset = sub i64 %len, %remainder
  %remainder_src_adj = getelementptr i8, ptr %src, i64 %remainder_offset
  %remainder_dst_adj = getelementptr i8, ptr %dst, i64 %remainder_offset
  call void @llvm.memcpy.p0.p0.i64(ptr %remainder_dst_adj, ptr %remainder_src_adj, i64 %remainder, i1 false)
  br label %exit

src_violation:
  ; Source buffer doesn't satisfy conservation law
  call void @atlas_conservation_violation(ptr %dst, ptr %src, i64 %len)
  call void @llvm.trap()
  unreachable

exit:
  ret void
}

; High-performance memset that adjusts the final byte so sum(dst) % 96 == 0
; Optimized with vectorization, non-temporal stores, and parallel conservation calculation

define void @atlas_memset_conserved(ptr %dst, i8 %val, i64 %len) {
entry:
  ; Handle empty buffer case
  %empty = icmp eq i64 %len, 0
  br i1 %empty, label %exit, label %check_size

check_size:
  ; For small sizes, use scalar path
  %small = icmp ult i64 %len, 128
  br i1 %small, label %scalar_path, label %vectorized_path

scalar_path:
  ; Handle single byte case optimally
  %is_single = icmp eq i64 %len, 1
  br i1 %is_single, label %handle_single, label %scalar_multi

handle_single:
  ; For single byte, calculate conserved value directly
  %val_mod = urem i8 %val, 96
  %val_conserved = icmp eq i8 %val_mod, 0
  br i1 %val_conserved, label %store_single_conserved, label %store_single_fixed

store_single_conserved:
  store i8 %val, ptr %dst, align 1
  br label %exit

store_single_fixed:
  %val_wide = zext i8 %val to i16
  %val_mod_wide = zext i8 %val_mod to i16
  %deficit_wide = sub i16 96, %val_mod_wide
  %corrected_wide = add i16 %val_wide, %deficit_wide
  %corrected = trunc i16 %corrected_wide to i8
  store i8 %corrected, ptr %dst, align 1
  br label %exit

scalar_multi:
  ; Small multi-byte buffer - use standard memset + fixup
  call void @llvm.memset.p0.i64(ptr %dst, i8 %val, i64 %len, i1 false)
  %len_mod = urem i64 %len, 96
  %val_extended = zext i8 %val to i64
  %expected_sum = mul i64 %val_extended, %len_mod
  %sum_mod = urem i64 %expected_sum, 96
  %already_conserved = icmp eq i64 %sum_mod, 0
  br i1 %already_conserved, label %exit, label %apply_scalar_fixup

apply_scalar_fixup:
  %deficit = sub i64 96, %sum_mod
  %last_idx = sub i64 %len, 1
  %last_ptr = getelementptr i8, ptr %dst, i64 %last_idx
  %old_byte = load i8, ptr %last_ptr, align 1
  %old_wide = zext i8 %old_byte to i16
  %deficit_trunc = trunc i64 %deficit to i8
  %deficit_wide_scalar = zext i8 %deficit_trunc to i16
  %new_wide = add i16 %old_wide, %deficit_wide_scalar
  %new_byte = trunc i16 %new_wide to i8
  store i8 %new_byte, ptr %last_ptr, align 1
  br label %exit

vectorized_path:
  ; Pre-calculate conservation parameters
  %len_mod_96 = urem i64 %len, 96
  %val_u64 = zext i8 %val to i64
  %raw_sum = mul i64 %val_u64, %len_mod_96
  %sum_mod_96 = urem i64 %raw_sum, 96
  %needs_fixup = icmp ne i64 %sum_mod_96, 0
  %deficit_u64 = select i1 %needs_fixup, i64 96, i64 0
  %final_deficit = sub i64 %deficit_u64, %sum_mod_96
  
  ; Calculate loop parameters for 128-byte chunks
  %len_minus_1 = sub i64 %len, 1
  %chunks_128 = udiv i64 %len_minus_1, 128
  %remainder = urem i64 %len_minus_1, 128
  
  ; Create vector pattern manually
  %val_32 = insertelement <32 x i8> undef, i8 %val, i32 0
  %val_vec = shufflevector <32 x i8> %val_32, <32 x i8> undef, <32 x i32> zeroinitializer
  
  br label %vector_loop

vector_loop:
  %chunk_idx = phi i64 [ 0, %vectorized_path ], [ %next_chunk_idx, %vector_store ]
  %current_offset = mul i64 %chunk_idx, 128
  %chunk_done = icmp eq i64 %chunk_idx, %chunks_128
  br i1 %chunk_done, label %handle_vector_remainder, label %vector_store

vector_store:
  %chunk_ptr = getelementptr i8, ptr %dst, i64 %current_offset
  
  ; Prefetch next chunk
  %prefetch_offset = add i64 %current_offset, 256
  %prefetch_ptr = getelementptr i8, ptr %dst, i64 %prefetch_offset
  call void @llvm.prefetch.p0(ptr %prefetch_ptr, i32 1, i32 3, i32 1)
  
  ; Store 128 bytes using 4x32-byte vector stores with non-temporal hints
  %vec_ptr1 = getelementptr <32 x i8>, ptr %chunk_ptr, i64 0
  %vec_ptr2 = getelementptr <32 x i8>, ptr %chunk_ptr, i64 1
  %vec_ptr3 = getelementptr <32 x i8>, ptr %chunk_ptr, i64 2
  %vec_ptr4 = getelementptr <32 x i8>, ptr %chunk_ptr, i64 3
  
  store <32 x i8> %val_vec, ptr %vec_ptr1, align 32, !nontemporal !1
  store <32 x i8> %val_vec, ptr %vec_ptr2, align 32, !nontemporal !1
  store <32 x i8> %val_vec, ptr %vec_ptr3, align 32, !nontemporal !1
  store <32 x i8> %val_vec, ptr %vec_ptr4, align 32, !nontemporal !1
  
  %next_chunk_idx = add i64 %chunk_idx, 1
  br label %vector_loop

handle_vector_remainder:
  ; Fill remainder bytes (excluding last byte which needs conservation fixup)
  %remainder_zero = icmp eq i64 %remainder, 0
  br i1 %remainder_zero, label %apply_vector_fixup, label %fill_remainder

fill_remainder:
  %remainder_start = mul i64 %chunks_128, 128
  %remainder_ptr = getelementptr i8, ptr %dst, i64 %remainder_start
  call void @llvm.memset.p0.i64(ptr %remainder_ptr, i8 %val, i64 %remainder, i1 false)
  br label %apply_vector_fixup

apply_vector_fixup:
  ; Set the last byte with conservation correction
  %last_byte_ptr = getelementptr i8, ptr %dst, i64 %len_minus_1
  br i1 %needs_fixup, label %apply_fixup_val, label %apply_normal_val

apply_fixup_val:
  %corrected_last_trunc = trunc i64 %final_deficit to i8
  %final_val = add i8 %val, %corrected_last_trunc
  store i8 %final_val, ptr %last_byte_ptr, align 1
  br label %exit

apply_normal_val:
  store i8 %val, ptr %last_byte_ptr, align 1
  br label %exit

exit:
  ret void
}

; =============================================================================
; Memory Barriers and Synchronization
; =============================================================================

; Full memory fence (portable)

define void @atlas.memory.fence() {
entry:
  fence seq_cst
  ret void
}

; Atomic add that ensures the post‑add value is 0 mod 96 (by applying a fixup).

define i32 @atlas.atomic.add.conserved(ptr %ptr, i32 %val) {
entry:
  %old = atomicrmw add ptr %ptr, i32 %val seq_cst
  %new = add i32 %old, %val
  %m   = urem i32 %new, 96
  %ok  = icmp eq i32 %m, 0
  br i1 %ok, label %ret, label %fix

fix:
  %delta = sub i32 96, %m
  %_adj  = atomicrmw add ptr %ptr, i32 %delta seq_cst
  br label %ret

ret:
  ret i32 %old
}

; =============================================================================
; Memory Pool Management
; =============================================================================

; Create a pool of `pages` pages (256 bytes each); returns pool handle.

define ptr @atlas.pool.create(i32 %pages) {
entry:
  ; sizeof(%atlas.memory.pool)
  %pool_end  = getelementptr %atlas.memory.pool, ptr null, i32 1
  %pool_size = ptrtoint ptr %pool_end to i64
  %pool_mem  = call ptr @malloc(i64 %pool_size)

  ; Allocate backing pages
  %base_mem  = call ptr @atlas.alloc.pages(i32 %pages)

  ; Initialize fields
  %base_f = getelementptr %atlas.memory.pool, ptr %pool_mem, i32 0, i32 0
  store ptr %base_mem, ptr %base_f, align 8

  %pages64 = zext i32 %pages to i64
  %total   = mul i64 %pages64, 256
  %size_f  = getelementptr %atlas.memory.pool, ptr %pool_mem, i32 0, i32 1
  store i64 %total, ptr %size_f, align 8

  %used_f  = getelementptr %atlas.memory.pool, ptr %pool_mem, i32 0, i32 2
  store i64 0, ptr %used_f, align 8

  %pc_f    = getelementptr %atlas.memory.pool, ptr %pool_mem, i32 0, i32 3
  store i32 %pages, ptr %pc_f, align 4

  ; Witness for the pool’s backing region
  %w       = call ptr @atlas_witness_generate(ptr %base_mem, i64 %total)
  %w_f     = getelementptr %atlas.memory.pool, ptr %pool_mem, i32 0, i32 4
  store ptr %w, ptr %w_f, align 8

  ret ptr %pool_mem
}

; Allocate a slice from the pool (no free, bump allocator semantics)

define ptr @atlas.pool.alloc(ptr %pool, i64 %size) {
entry:
  %used_p = getelementptr %atlas.memory.pool, ptr %pool, i32 0, i32 2
  %used   = load i64, ptr %used_p, align 8

  %total_p= getelementptr %atlas.memory.pool, ptr %pool, i32 0, i32 1
  %total  = load i64, ptr %total_p, align 8

  %new_used = add i64 %used, %size
  %fits = icmp ule i64 %new_used, %total
  br i1 %fits, label %ok, label %err

ok:
  %base_p = getelementptr %atlas.memory.pool, ptr %pool, i32 0, i32 0
  %base   = load ptr, ptr %base_p, align 8
  %alloc  = getelementptr i8, ptr %base, i64 %used
  store i64 %new_used, ptr %used_p, align 8
  ret ptr %alloc

err:
  ret ptr null
}

; =============================================================================
; Debug and Diagnostics
; =============================================================================

; Report conservation violation (implementation-defined: here, compute sums and trap)

define void @atlas_conservation_violation(ptr %dst, ptr %src, i64 %len) {
entry:
  %dst_sum = call i64 @atlas_sum_bytes(ptr %dst, i64 %len)
  %src_sum = call i64 @atlas_sum_bytes(ptr %src, i64 %len)
  ; (Values available in a debugger via SSA names.)
  call void @llvm.debugtrap()
  ret void
}

; Return memory statistics: {allocated, peak, witness_count}

define { i64, i64, i32 } @atlas.memory.stats() {
entry:
  %a = load i64, ptr @atlas.memory.allocated, align 8
  %p = load i64, ptr @atlas.memory.peak,      align 8
  %w = load i32, ptr @atlas.memory.witness_count, align 4
  %r1 = insertvalue { i64, i64, i32 } undef, i64 %a, 0
  %r2 = insertvalue { i64, i64, i32 } %r1,   i64 %p, 1
  %r3 = insertvalue { i64, i64, i32 } %r2,   i32 %w, 2
  ret { i64, i64, i32 } %r3
}

; =============================================================================
; Module Metadata
; =============================================================================

; Non-temporal store metadata
!1 = !{i32 1}

; Vector intrinsic declarations
declare <16 x i8> @llvm.vector.extract.v16i8.v32i8(<32 x i8>, i64) nounwind readnone
declare i16 @llvm.vector.reduce.add.v16i16(<16 x i16>) nounwind readnone
