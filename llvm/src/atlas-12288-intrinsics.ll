; atlas-12288-intrinsics.ll — Atlas‑12,288 intrinsic declarations (LLVM 15+, opaque pointers)
; ---------------------------------------------------------------------------------
; This module declares Atlas intrinsics as *ordinary functions* (no reserved
; "llvm." prefix) so they assemble on stock LLVM toolchains. All pointer-typed
; parameters use `ptr` for opaque-pointer compatibility.
;
; If you also use `atlas-12288-types.ll`, those named types are for values and
; documentation; do not rely on typed pointers in signatures.
; ---------------------------------------------------------------------------------

source_filename = "atlas-12288-intrinsics.ll"
target triple = "x86_64-unknown-linux-gnu"

; =============================================================================
; R96 Classification Intrinsics
; =============================================================================

; Classify single byte to resonance class
declare i7 @atlas.r96.classify(i8 %byte) #0

; Vectorized classification (SSE/AVX-friendly shapes)
declare <16 x i7> @atlas.r96.classify.v16i8(<16 x i8> %bytes) #0
declare <32 x i7> @atlas.r96.classify.v32i8(<32 x i8> %bytes) #0
declare <64 x i7> @atlas.r96.classify.v64i8(<64 x i8> %bytes) #0

; Classify entire page (page passed by value as 256-lane vector)
declare <256 x i7> @atlas.r96.classify.page(<256 x i8> %page) #0

; Get resonance spectrum for a structure
;  %structure : ptr -> %atlas.structure
;  %out       : ptr -> %atlas.spectrum (written)
declare void @atlas.r96.spectrum(ptr %structure, ptr %out) #4

; =============================================================================
; Boundary Operations (Φ Isomorphism)
; =============================================================================

; Encode page/offset to boundary coordinate
declare i32 @atlas.boundary.encode(i16 %page, i8 %offset) #0

; Decode boundary coordinate to page/offset (returned as {i16, i8})
declare { i16, i8 } @atlas.boundary.decode(i32 %boundary) #0

; Transform boundary with resonance (pure arithmetic on boundary code)
declare i32 @atlas.boundary.transform(i32 %boundary, i7 %resonance) #0

; Batch boundary encoding
declare <16 x i32> @atlas.boundary.encode.v16(<16 x i16> %pages, <16 x i8> %offsets) #0

; Klein orbit operations
declare i2  @atlas.boundary.klein(i32 %boundary) #0
declare i32 @atlas.boundary.rotate(i32 %boundary, i2 %klein) #0

; =============================================================================
; Conservation Operations
; =============================================================================

; Check if a byte range is conserved
declare i1 @atlas.conserved.check(ptr %data, i64 %len) #1

; Sum page elements (for conservation check)
declare i16 @atlas.conserved.sum.page(ptr %page) #1

; Sum entire structure
declare i32 @atlas.conserved.sum.structure(ptr %structure) #1

; Compute conservation delta between two buffers
declare i32 @atlas.conserved.delta(ptr %before, ptr %after, i64 %len) #1

; Verify conservation domain descriptor
declare i1 @atlas.conserved.domain(ptr %domain) #1

; Conservation-safe addition: dst[i] = src1[i] ⊕ src2[i]
declare void @atlas.conserved.add(ptr %dst, ptr %src1, ptr %src2, i64 %len) #2

; =============================================================================
; Witness Operations
; =============================================================================

; Generate a witness handle for data (implementation-defined allocation)
declare ptr @atlas.witness.generate(ptr %data, i64 %len) #4

; Verify witness against data
declare i1 @atlas.witness.verify(ptr %w, ptr %data, i64 %len) #1

; Destroy witness (cleanup)
declare void @atlas.witness.destroy(ptr %w) #4

; Create witness chain node from current and previous
declare ptr @atlas.witness.chain(ptr %current, ptr %previous) #4

; Merge two witnesses into a new handle
declare ptr @atlas.witness.merge(ptr %w1, ptr %w2) #4

; Extract witness metadata (readonly by contract)
declare i64 @atlas.witness.timestamp(ptr %w) #1
declare i7  @atlas.witness.resonance(ptr %w) #1

; =============================================================================
; Budget Operations (RL‑96 Algebra)
; =============================================================================

declare i7 @atlas.budget.add(i7 %a, i7 %b) #0
declare i7 @atlas.budget.mul(i7 %a, i7 %b) #0
declare i7 @atlas.budget.inv(i7 %budget) #0
declare i1 @atlas.budget.zero(i7 %budget) #0
declare i1 @atlas.budget.less(i7 %a, i7 %b) #0

; Allocate and release abstract budget tokens
declare i7  @atlas.budget.alloc(i32 %amount) #4
declare void @atlas.budget.release(i7 %budget) #4

; =============================================================================
; Memory Operations
; =============================================================================

; Allocate witnessed memory
declare ptr @atlas.alloc.witnessed(i64 %size, i7 %resonance) #3

; Free witnessed memory
declare void @atlas.free.witnessed(ptr %ptr) #4

; Conservation‑preserving memcpy/memset
declare void @atlas.memcpy.conserved(ptr %dst, ptr %src, i64 %len) #2
declare void @atlas.memset.conserved(ptr %dst, i8 %val, i64 %len) #2

; Resonance‑aware allocation (alignment is a requested minimum)
declare ptr @atlas.alloc.resonant(i64 %size, i7 %resonance, i32 %alignment) #3

; Page‑aligned allocation returning a handle to the first page
declare ptr @atlas.alloc.pages(i32 %count) #3

; =============================================================================
; Resonance Operations
; =============================================================================

declare i7  @atlas.resonance.harmonic(i7 %r1, i7 %r2) #0
declare i1  @atlas.resonance.harmonizes(i7 %r1, i7 %r2) #0

declare ptr @atlas.resonance.cluster(ptr %coords, i32 %count) #4

declare i64 @atlas.resonance.schedule(i7 %resonance) #0

; =============================================================================
; Optimization Hints (no side effects expected)
; =============================================================================

declare void @atlas.assume.conserved(ptr %data, i64 %len) #0
declare void @atlas.assume.resonance(ptr %data, i7 %resonance) #0

declare void @atlas.prefetch.resonant(ptr %ptr, i7 %resonance) #3

; =============================================================================
; Debug Intrinsics (implementation-defined I/O)
; =============================================================================

declare void @atlas.debug.conservation(ptr %data, i64 %len) #4
declare void @atlas.debug.spectrum(ptr %spectrum) #4
declare i1  @atlas.debug.validate(ptr %structure) #1

; =============================================================================
; Attributes
; =============================================================================

; #0: pure/value-only (no memory access)
attributes #0 = { nounwind readnone willreturn speculatable "atlas-pure"="true" }

; #1: readonly memory access (no writes)
attributes #1 = { nounwind readonly willreturn "atlas-readonly"="true" }

; #2: conservation‑preserving operations
attributes #2 = { nounwind "atlas-conserving"="true" "atlas-witness-required"="true" }

; #3: resonance‑aware allocation/ops
attributes #3 = { nounwind "atlas-resonance-aware"="true" }

; #4: generic nounwind, effects allowed (allocation/teardown/IO as needed)
attributes #4 = { nounwind }

; =============================================================================
; Module Metadata
; =============================================================================

; Metadata removed
; Metadata removed
