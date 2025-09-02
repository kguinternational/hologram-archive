; atlas-12288-types.ll — LLVM 15+ (opaque pointers) types module for Atlas‑12,288
; -----------------------------------------------------------------------------
; This file defines the canonical type layout for Atlas‑12,288 constructs.
; It is “types-only” and safe to include as a prelude in IR you generate.
;
; Key conventions
;  • Opaque pointers (`ptr`) instead of typed pointers (LLVM 15+).
;  • Byte-addressable storage; narrow integer types (e.g., i7) are for arithmetic
;    semantics, not guaranteed packed storage.
;  • Documented ranges via comments; use `llvm.assume` and !range at use sites.
;  • Fill in a target datalayout in your build system if you need ABI stability.
; -----------------------------------------------------------------------------

source_filename = "atlas-12288-types.ll"

;--- Optional module flags (harmless on most toolchains) -----------------------
!llvm.module.flags = !{!0}
!0 = !{i32 1, !"wchar_size", i32 4}

; Atlas metadata anchors (informational; not consumed by LLVM)
; These are commented out as they're not valid LLVM metadata syntax
; atlas.pages              = 48
; atlas.bytes_per_page     = 256
; atlas.total_elements     = 12288
; atlas.resonance_classes  = 96     ; values intended in [0,96)
; atlas.version_major      = 1

;==============================================================================
;                               TYPE DEFINITIONS
;==============================================================================

; Basic storage aliases --------------------------------------------------------
%atlas.byte        = type i8
%atlas.page        = type [256 x i8]                      ; 256-byte page
%atlas.structure   = type [48 x %atlas.page]              ; 48 pages = 12,288 bytes

; Narrow semantic types --------------------------------------------------------
%atlas.resonance   = type i7       ; semantic range [0,95]
%atlas.budget      = type i7       ; semantic range [0,95]
%atlas.klein       = type i2       ; Z2 x Z2 components, or similar tag

; Addressing / coordinates -----------------------------------------------------
; coordinate = (page_index, byte_offset)
%atlas.coordinate  = type { i16, i8 }                     ; fits 48 pages, 256 offsets
%atlas.boundary    = type i32                              ; Φ-encoded coordinate / hash

; Witness / provenance ---------------------------------------------------------
; NOTE: ptr fields are opaque; annotate alignment or pointee with assumes when known.
%atlas.witness           = type { ptr, i32, i32 }         ; (opaque ctx, id, flags)
%atlas.witness.extended  = type { %atlas.witness, i64, %atlas.resonance, %atlas.budget }
%atlas.witness.chain     = type { %atlas.witness, ptr }   ; ptr = next (nullable)

; Conservation / domain --------------------------------------------------------
%atlas.conservation = type { i32, i1, %atlas.witness }    ; (rule_id, active?, who)
%atlas.domain       = type { i64, ptr, %atlas.conservation, %atlas.budget }

; Spectral and clustering forms ------------------------------------------------
%atlas.spectrum    = type [256 x %atlas.resonance]
%atlas.cluster     = type { %atlas.resonance, i32, ptr }  ; ptr -> %atlas.coordinate or table
%atlas.harmonic    = type { %atlas.resonance, %atlas.resonance, i1 } ; (a,b, in_phase?)

; Memory forms -----------------------------------------------------------------
%atlas.memory          = type { ptr, i64, %atlas.resonance, %atlas.witness }
%atlas.memory.aligned  = type { ptr, i32, %atlas.budget }  ; ptr with alignment budget tag

; Transforms / batches ---------------------------------------------------------
%atlas.transform   = type { %atlas.coordinate, %atlas.coordinate, %atlas.resonance, %atlas.witness }
%atlas.batch       = type { i32, ptr, %atlas.conservation } ; (count, ptr ops, policy)

; Errors / versioning ----------------------------------------------------------
%atlas.error          = type i32
%atlas.error.context  = type { %atlas.error, ptr, %atlas.coordinate }
%atlas.version.t      = type { i16, i16, i32 }             ; (major, minor, build)
%atlas.capabilities   = type { i1, i1, i1, i1 }            ; feature bitmap

;==============================================================================
;                         HELPER DECLARATIONS (OPTIONAL)
;==============================================================================
; You can use these to encode invariants at use-sites in your own IR.
; Example: assert resonance values are in [0,96) without emitting a branch.

declare void @llvm.assume(i1)

; Example pattern (for reference, not executed here):
;   %r7  = ... : i7
;   %ok  = icmp ult i7 %r7, 96
;   call void @llvm.assume(i1 %ok)
;
; When crossing ABI boundaries where strict ranges matter, consider passing as
; i8 and masking on load/store. Keep arithmetic in i7 to preserve intent.

; End of atlas-12288-types.ll
