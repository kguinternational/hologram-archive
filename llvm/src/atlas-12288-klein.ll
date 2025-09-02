; atlas-12288-klein.ll - Klein Orbit Operations
; Klein orbits provide canonical forms and fast acceptance tests
; (c) 2024-2025 UOR Foundation. All rights reserved.
; SPDX-License-Identifier: MIT

source_filename = "atlas-12288-klein.ll"

; Klein orbit constants - privileged orbits
@atlas.klein.orbit_0 = constant i32 0
@atlas.klein.orbit_1 = constant i32 1
@atlas.klein.orbit_48 = constant i32 48
@atlas.klein.orbit_49 = constant i32 49

; === Orbit Classification ===

define i8 @atlas.klein.get_orbit_id(i32 %coord) nounwind readnone {
entry:
  ; Classify coordinate into Klein orbit
  ; Orbits form equivalence classes under V₄ group action
  
  %page = lshr i32 %coord, 8
  %byte = and i32 %coord, 255
  
  ; Compute orbit ID using Klein group structure
  %page_mod = urem i32 %page, 4
  %byte_mod = urem i32 %byte, 4
  
  ; Combine to get orbit ID (0-15 for 4x4 Klein structure)
  %orbit_base = shl i32 %page_mod, 2
  %orbit_full = or i32 %orbit_base, %byte_mod
  %orbit = trunc i32 %orbit_full to i8
  
  ret i8 %orbit
}

define i1 @atlas.klein.is_privileged_orbit(i32 %coord) nounwind readnone {
entry:
  ; Check if coordinate is in one of the four privileged orbits
  %is_0 = icmp eq i32 %coord, 0
  %is_1 = icmp eq i32 %coord, 1
  %is_48 = icmp eq i32 %coord, 48
  %is_49 = icmp eq i32 %coord, 49
  
  %or1 = or i1 %is_0, %is_1
  %or2 = or i1 %is_48, %is_49
  %privileged = or i1 %or1, %or2
  
  ret i1 %privileged
}

define void @atlas.klein.classify_structure(ptr %structure, ptr %orbit_map) nounwind {
entry:
  ; Classify entire 12,288 structure into Klein orbits
  br label %loop

loop:
  %i = phi i64 [ 0, %entry ], [ %next_i, %body ]
  %cmp = icmp ult i64 %i, 12288
  br i1 %cmp, label %body, label %done

body:
  ; Compute coordinate
  %page = udiv i64 %i, 256
  %byte = urem i64 %i, 256
  %page_trunc = trunc i64 %page to i32
  %byte_trunc = trunc i64 %byte to i32
  %coord_shifted = shl i32 %page_trunc, 8
  %coord = or i32 %coord_shifted, %byte_trunc
  
  ; Get orbit ID
  %orbit = call i8 @atlas.klein.get_orbit_id(i32 %coord)
  
  ; Store in orbit map
  %map_ptr = getelementptr i8, ptr %orbit_map, i64 %i
  store i8 %orbit, ptr %map_ptr
  
  %next_i = add i64 %i, 1
  br label %loop

done:
  ret void
}

; === V₄ Cosets ===

define i8 @atlas.klein.get_coset_id(i32 %coord) nounwind readnone {
entry:
  ; Compute V₄ coset (Klein four-group)
  ; V₄ = {e, a, b, ab} where a²=b²=(ab)²=e
  
  %page = lshr i32 %coord, 8
  %byte = and i32 %coord, 255
  
  ; Coset determined by remainders mod 2
  %page_parity = and i32 %page, 1
  %byte_parity = and i32 %byte, 1
  
  ; Four cosets: 00, 01, 10, 11
  %coset_high = shl i32 %page_parity, 1
  %coset = or i32 %coset_high, %byte_parity
  %coset_id = trunc i32 %coset to i8
  
  ret i8 %coset_id
}

define void @atlas.klein.generate_coset(i8 %coset_id, ptr %output) nounwind {
entry:
  ; Generate all elements in the given V₄ coset
  %coset_ext = zext i8 %coset_id to i32
  %page_parity = lshr i32 %coset_ext, 1
  %byte_parity = and i32 %coset_ext, 1
  
  %idx = alloca i64, align 8
  store i64 0, ptr %idx
  
  br label %page_loop

page_loop:
  %p = phi i32 [ 0, %entry ], [ %next_p, %page_end ]
  %p_cmp = icmp ult i32 %p, 48
  br i1 %p_cmp, label %page_check, label %done

page_check:
  %p_mod = and i32 %p, 1
  %p_match = icmp eq i32 %p_mod, %page_parity
  br i1 %p_match, label %byte_loop, label %page_end

byte_loop:
  %b = phi i32 [ 0, %page_check ], [ %next_b, %byte_next ]
  %b_cmp = icmp ult i32 %b, 256
  br i1 %b_cmp, label %byte_check, label %page_end

byte_check:
  %b_mod = and i32 %b, 1
  %b_match = icmp eq i32 %b_mod, %byte_parity
  br i1 %b_match, label %byte_body, label %byte_next

byte_body:
  ; Add to output
  %coord_high = shl i32 %p, 8
  %coord = or i32 %coord_high, %b
  
  %current_idx = load i64, ptr %idx
  %out_ptr = getelementptr i32, ptr %output, i64 %current_idx
  store i32 %coord, ptr %out_ptr
  
  %new_idx = add i64 %current_idx, 1
  store i64 %new_idx, ptr %idx
  br label %byte_next

byte_next:
  %next_b = add i32 %b, 1
  br label %byte_loop

page_end:
  %next_p = add i32 %p, 1
  br label %page_loop

done:
  ret void
}

define i1 @atlas.klein.verify_coset_partition(ptr %structure) nounwind readonly {
entry:
  ; Verify that V₄ cosets properly partition the structure
  %counts = alloca [4 x i32], align 4
  
  ; Initialize counts
  %c0 = getelementptr i32, ptr %counts, i64 0
  %c1 = getelementptr i32, ptr %counts, i64 1
  %c2 = getelementptr i32, ptr %counts, i64 2
  %c3 = getelementptr i32, ptr %counts, i64 3
  store i32 0, ptr %c0
  store i32 0, ptr %c1
  store i32 0, ptr %c2
  store i32 0, ptr %c3
  
  br label %loop

loop:
  %i = phi i64 [ 0, %entry ], [ %next_i, %body ]
  %cmp = icmp ult i64 %i, 12288
  br i1 %cmp, label %body, label %verify

body:
  ; Get coordinate
  %page = udiv i64 %i, 256
  %byte = urem i64 %i, 256
  %page_trunc = trunc i64 %page to i32
  %byte_trunc = trunc i64 %byte to i32
  %coord_high = shl i32 %page_trunc, 8
  %coord = or i32 %coord_high, %byte_trunc
  
  ; Get coset
  %coset_id = call i8 @atlas.klein.get_coset_id(i32 %coord)
  %coset_ext = zext i8 %coset_id to i64
  
  ; Increment count
  %count_ptr = getelementptr i32, ptr %counts, i64 %coset_ext
  %old_count = load i32, ptr %count_ptr
  %new_count = add i32 %old_count, 1
  store i32 %new_count, ptr %count_ptr
  
  %next_i = add i64 %i, 1
  br label %loop

verify:
  ; Each coset should have 3072 elements (12288/4)
  %count0 = load i32, ptr %c0
  %count1 = load i32, ptr %c1
  %count2 = load i32, ptr %c2
  %count3 = load i32, ptr %c3
  
  %eq0 = icmp eq i32 %count0, 3072
  %eq1 = icmp eq i32 %count1, 3072
  %eq2 = icmp eq i32 %count2, 3072
  %eq3 = icmp eq i32 %count3, 3072
  
  %and1 = and i1 %eq0, %eq1
  %and2 = and i1 %eq2, %eq3
  %valid = and i1 %and1, %and2
  
  ret i1 %valid
}

; === Canonicalization ===

define i32 @atlas.klein.canonicalize_coord(i32 %coord) nounwind readnone {
entry:
  ; Map coordinate to canonical representative of its orbit
  %orbit = call i8 @atlas.klein.get_orbit_id(i32 %coord)
  
  ; Canonical representative is smallest element in orbit
  %orbit_ext = zext i8 %orbit to i32
  %canon_page = and i32 %orbit_ext, 3
  %orbit_shifted = lshr i32 %orbit_ext, 2
  %canon_byte = and i32 %orbit_shifted, 3
  
  %canon_high = shl i32 %canon_page, 8
  %canonical = or i32 %canon_high, %canon_byte
  
  ret i32 %canonical
}

define void @atlas.klein.canonicalize_structure(ptr %structure) nounwind {
entry:
  %temp = alloca [12288 x i8], align 1
  call void @llvm.memcpy.p0.p0.i64(ptr %temp, ptr %structure, i64 12288, i1 false)
  
  br label %loop

loop:
  %i = phi i64 [ 0, %entry ], [ %next_i, %body ]
  %cmp = icmp ult i64 %i, 12288
  br i1 %cmp, label %body, label %done

body:
  ; Get current coordinate
  %page = udiv i64 %i, 256
  %byte = urem i64 %i, 256
  %page_trunc = trunc i64 %page to i32
  %byte_trunc = trunc i64 %byte to i32
  %coord_high = shl i32 %page_trunc, 8
  %coord = or i32 %coord_high, %byte_trunc
  
  ; Canonicalize
  %canonical = call i32 @atlas.klein.canonicalize_coord(i32 %coord)
  
  ; Extract canonical indices
  %canon_page = lshr i32 %canonical, 8
  %canon_byte = and i32 %canonical, 255
  %canon_page_ext = zext i32 %canon_page to i64
  %canon_byte_ext = zext i32 %canon_byte to i64
  %canon_idx = mul i64 %canon_page_ext, 256
  %canon_final = add i64 %canon_idx, %canon_byte_ext
  
  ; Copy data to canonical position
  %src_ptr = getelementptr i8, ptr %temp, i64 %i
  %dst_ptr = getelementptr i8, ptr %structure, i64 %canon_final
  %val = load i8, ptr %src_ptr
  store i8 %val, ptr %dst_ptr
  
  %next_i = add i64 %i, 1
  br label %loop

done:
  ret void
}

define i1 @atlas.klein.is_canonical(ptr %structure) nounwind readonly {
entry:
  ; Check if structure is in canonical form
  ; For each orbit, verify data is at canonical positions
  
  br label %loop

loop:
  %i = phi i64 [ 0, %entry ], [ %next_i, %update ]
  %is_canon = phi i1 [ true, %entry ], [ %still_canon, %update ]
  %cmp = icmp ult i64 %i, 12288
  br i1 %cmp, label %body, label %done

body:
  %page = udiv i64 %i, 256
  %byte = urem i64 %i, 256
  %page_trunc = trunc i64 %page to i32
  %byte_trunc = trunc i64 %byte to i32
  %coord_high = shl i32 %page_trunc, 8
  %coord = or i32 %coord_high, %byte_trunc
  
  %canonical = call i32 @atlas.klein.canonicalize_coord(i32 %coord)
  %matches = icmp eq i32 %coord, %canonical
  
  ; If not at canonical position, must be zero
  br i1 %matches, label %continue, label %check_zero

check_zero:
  %ptr = getelementptr i8, ptr %structure, i64 %i
  %val = load i8, ptr %ptr
  %is_zero = icmp eq i8 %val, 0
  br label %update

continue:
  br label %update

update:
  %current_ok = phi i1 [ true, %continue ], [ %is_zero, %check_zero ]
  %still_canon = and i1 %is_canon, %current_ok
  %next_i = add i64 %i, 1
  br label %loop

done:
  ret i1 %is_canon
}

; === Fast Acceptance Tests ===

define i1 @atlas.klein.quick_accept(ptr %data, i64 %len) nounwind readonly {
entry:
  ; Fast acceptance using Klein orbit properties
  
  ; Check length is valid
  %len_ok = icmp eq i64 %len, 12288
  br i1 %len_ok, label %check_privileged, label %reject

check_privileged:
  ; Check privileged orbits have expected properties
  %p0 = getelementptr i8, ptr %data, i64 0
  %p1 = getelementptr i8, ptr %data, i64 1
  %p48 = getelementptr i8, ptr %data, i64 48
  %p49 = getelementptr i8, ptr %data, i64 49
  
  %v0 = load i8, ptr %p0
  %v1 = load i8, ptr %p1
  %v48 = load i8, ptr %p48
  %v49 = load i8, ptr %p49
  
  ; Classify to resonance
  %c0 = call i7 @atlas.r96.classify(i8 %v0)
  %c1 = call i7 @atlas.r96.classify(i8 %v1)
  %c48 = call i7 @atlas.r96.classify(i8 %v48)
  %c49 = call i7 @atlas.r96.classify(i8 %v49)
  
  ; Check orbit constraints
  %c0_ext = zext i7 %c0 to i8
  %c1_ext = zext i7 %c1 to i8
  %c48_ext = zext i7 %c48 to i8
  %c49_ext = zext i7 %c49 to i8
  
  ; Privileged orbits should have specific resonance relationships
  %sum01 = add i8 %c0_ext, %c1_ext
  %sum4849 = add i8 %c48_ext, %c49_ext
  
  %mod01 = urem i8 %sum01, 96
  %mod4849 = urem i8 %sum4849, 96
  
  ; Both pairs should harmonize (sum to 0 mod 96)
  %pair1_ok = icmp eq i8 %mod01, 0
  %pair2_ok = icmp eq i8 %mod4849, 0
  
  %accepted = and i1 %pair1_ok, %pair2_ok
  ret i1 %accepted

reject:
  ret i1 false
}

define i1 @atlas.klein.verify_orbit_tiling(ptr %structure) nounwind readonly {
entry:
  ; Verify orbits properly tile the 12,288 lattice
  %orbit_counts = alloca [16 x i32], align 4
  
  ; Initialize counts
  call void @llvm.memset.p0.i64(ptr %orbit_counts, i8 0, i64 64, i1 false)
  
  br label %loop

loop:
  %i = phi i64 [ 0, %entry ], [ %next_i, %body ]
  %cmp = icmp ult i64 %i, 12288
  br i1 %cmp, label %body, label %verify

body:
  %page = udiv i64 %i, 256
  %byte = urem i64 %i, 256
  %page_trunc = trunc i64 %page to i32
  %byte_trunc = trunc i64 %byte to i32
  %coord_high = shl i32 %page_trunc, 8
  %coord = or i32 %coord_high, %byte_trunc
  
  %orbit = call i8 @atlas.klein.get_orbit_id(i32 %coord)
  %orbit_ext = zext i8 %orbit to i64
  
  %count_ptr = getelementptr i32, ptr %orbit_counts, i64 %orbit_ext
  %old_count = load i32, ptr %count_ptr
  %new_count = add i32 %old_count, 1
  store i32 %new_count, ptr %count_ptr
  
  %next_i = add i64 %i, 1
  br label %loop

verify:
  ; Each orbit should have 768 elements (12288/16)
  br label %verify_loop

verify_loop:
  %j = phi i64 [ 0, %verify ], [ %next_j, %verify_body ]
  %all_ok = phi i1 [ true, %verify ], [ %still_ok, %verify_body ]
  %j_cmp = icmp ult i64 %j, 16
  br i1 %j_cmp, label %verify_body, label %done

verify_body:
  %verify_count_ptr = getelementptr i32, ptr %orbit_counts, i64 %j
  %count = load i32, ptr %verify_count_ptr
  %count_ok = icmp eq i32 %count, 768
  %still_ok = and i1 %all_ok, %count_ok
  %next_j = add i64 %j, 1
  br label %verify_loop

done:
  ret i1 %all_ok
}

define i64 @atlas.klein.compute_orbit_signature(ptr %structure) nounwind readonly {
entry:
  ; Compute signature based on Klein orbit distribution
  %sig = alloca i64, align 8
  store i64 0, ptr %sig
  
  br label %loop

loop:
  %i = phi i64 [ 0, %entry ], [ %next_i, %body ]
  %cmp = icmp ult i64 %i, 12288
  br i1 %cmp, label %body, label %done

body:
  %page = udiv i64 %i, 256
  %byte = urem i64 %i, 256
  %page_trunc = trunc i64 %page to i32
  %byte_trunc = trunc i64 %byte to i32
  %coord_high = shl i32 %page_trunc, 8
  %coord = or i32 %coord_high, %byte_trunc
  
  %orbit = call i8 @atlas.klein.get_orbit_id(i32 %coord)
  %orbit_ext = zext i8 %orbit to i64
  
  ; Get data value
  %ptr = getelementptr i8, ptr %structure, i64 %i
  %val = load i8, ptr %ptr
  %val_ext = zext i8 %val to i64
  
  ; Update signature
  %old_sig = load i64, ptr %sig
  %contrib = mul i64 %orbit_ext, %val_ext
  %new_sig = add i64 %old_sig, %contrib
  store i64 %new_sig, ptr %sig
  
  %next_i = add i64 %i, 1
  br label %loop

done:
  %final_sig = load i64, ptr %sig
  ret i64 %final_sig
}

; External dependencies
declare i7 @atlas.r96.classify(i8) nounwind readnone
declare void @llvm.memcpy.p0.p0.i64(ptr, ptr, i64, i1) nounwind
declare void @llvm.memset.p0.i64(ptr, i8, i64, i1) nounwind

; =============================================================================
; Module metadata and flags
; =============================================================================

!llvm.module.flags = !{!0, !1}
!0 = !{i32 1, !"wchar_size", i32 4}
!1 = !{i32 7, !"PIC Level", i32 2}

; Atlas Klein orbit metadata
!llvm.ident = !{!2}
!2 = !{!"Atlas-12288 Klein Module v1.0"}