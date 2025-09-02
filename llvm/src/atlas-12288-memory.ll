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

; Atlas intrinsics (see atlas-12288-intrinsics.ll)
declare ptr @atlas.witness.generate(ptr, i64)
declare i1  @atlas.witness.verify(ptr, ptr, i64)
declare void @atlas.witness.destroy(ptr)

declare i1  @atlas.conserved.check(ptr, i64)

; Helper (provided elsewhere, used by diagnostics)
; Using internal function from ops.ll
declare i64 @atlas._sum_bytes(ptr, i64)

; =============================================================================
; Utilities
; =============================================================================

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
  %w  = call ptr @atlas.witness.generate(ptr %raw, i64 %total_size)
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
  call void @atlas.witness.destroy(ptr %w)

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

; memcpy that requires conservation of src and dst (mod 96)

define void @atlas.memcpy.conserved(ptr %dst, ptr %src, i64 %len) {
entry:
  %ok_src = call i1 @atlas.conserved.check(ptr %src, i64 %len)
  br i1 %ok_src, label %copy, label %violation

copy:
  call void @llvm.memcpy.p0.p0.i64(ptr %dst, ptr %src, i64 %len, i1 false)
  %ok_dst = call i1 @atlas.conserved.check(ptr %dst, i64 %len)
  br i1 %ok_dst, label %exit, label %violation

violation:
  call void @atlas.conservation.violation(ptr %dst, ptr %src, i64 %len)
  call void @llvm.trap()
  unreachable

exit:
  ret void
}

; memset that adjusts the final byte so sum(dst) % 96 == 0

define void @atlas.memset.conserved(ptr %dst, i8 %val, i64 %len) {
entry:
  ; empty buffer -> nothing to do
  %empty = icmp eq i64 %len, 0
  br i1 %empty, label %exit, label %do

do:
  call void @llvm.memset.p0.i64(ptr %dst, i8 %val, i64 %len, i1 false)
  ; r = (val * len) % 96  [use (len % 96) to avoid overflow]
  %len96 = urem i64 %len, 96
  %v64   = zext i8 %val to i64
  %prod  = mul i64 %v64, %len96
  %r     = urem i64 %prod, 96
  %ok    = icmp eq i64 %r, 0
  br i1 %ok, label %exit, label %fix

fix:
  %delta = sub i64 96, %r
  %last_index = add i64 %len, -1
  %plast = getelementptr i8, ptr %dst, i64 %last_index
  %oldb  = load i8, ptr %plast, align 1
  %old16 = zext i8 %oldb to i16
  %d8    = trunc i64 %delta to i8
  %d16   = zext i8 %d8 to i16
  %new16 = add i16 %old16, %d16
  %new8  = trunc i16 %new16 to i8
  store i8 %new8, ptr %plast, align 1
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
  %w       = call ptr @atlas.witness.generate(ptr %base_mem, i64 %total)
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

define void @atlas.conservation.violation(ptr %dst, ptr %src, i64 %len) {
entry:
  %dst_sum = call i64 @atlas._sum_bytes(ptr %dst, i64 %len)
  %src_sum = call i64 @atlas._sum_bytes(ptr %src, i64 %len)
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

; Metadata removed
; Metadata removed
