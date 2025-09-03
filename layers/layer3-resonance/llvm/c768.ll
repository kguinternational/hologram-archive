; atlas-12288-c768.ll - Triple-Cycle Conservation System
; C768 = 16×48 = 3×256 - The heart of conservation verification
; (c) 2024-2025 UOR Foundation. All rights reserved.
; SPDX-License-Identifier: MIT

source_filename = "atlas-12288-c768.ll"

; C768 Constants
@atlas.c768.cycle_length = constant i64 768
@atlas.c768.page_rhythm = constant i16 16   ; 16 × 48 = 768
@atlas.c768.byte_rhythm = constant i16 3    ; 3 × 256 = 768

; Verify conservation closes over 768-step cycle
define i1 @atlas.c768.verify_closure(ptr %structure, i64 %window_start) nounwind {
entry:
  %end = add i64 %window_start, 768
  %sum = alloca i64, align 8
  store i64 0, ptr %sum
  br label %loop

loop:
  %i = phi i64 [ %window_start, %entry ], [ %next, %body ]
  %cmp = icmp ult i64 %i, %end
  br i1 %cmp, label %body, label %check

body:
  %ptr = getelementptr i8, ptr %structure, i64 %i
  %val = load i8, ptr %ptr
  %ext = zext i8 %val to i64
  %old_sum = load i64, ptr %sum
  %new_sum = add i64 %old_sum, %ext
  store i64 %new_sum, ptr %sum
  %next = add i64 %i, 1
  br label %loop

check:
  %final_sum = load i64, ptr %sum
  %mod = urem i64 %final_sum, 96
  %closed = icmp eq i64 %mod, 0
  ret i1 %closed
}

; Compute window sums for verification
define i64 @atlas.c768.compute_window_sum(ptr %data, i64 %offset, i64 %size) nounwind readonly {
entry:
  br label %loop

loop:
  %i = phi i64 [ 0, %entry ], [ %next_i, %body ]
  %sum = phi i64 [ 0, %entry ], [ %new_sum, %body ]
  %cmp = icmp ult i64 %i, %size
  br i1 %cmp, label %body, label %done

body:
  %idx = add i64 %offset, %i
  %ptr = getelementptr i8, ptr %data, i64 %idx
  %val = load i8, ptr %ptr
  %ext = zext i8 %val to i64
  %new_sum = add i64 %sum, %ext
  %next_i = add i64 %i, 1
  br label %loop

done:
  ret i64 %sum
}

; Check page rhythm (16×48 alignment)
define i1 @atlas.c768.check_page_rhythm(ptr %structure, i64 %step) nounwind {
entry:
  ; Check if we're at a 48-page boundary (48 * 256 = 12288)
  %page_size = mul i64 48, 256
  %mod = urem i64 %step, %page_size
  %at_boundary = icmp eq i64 %mod, 0
  br i1 %at_boundary, label %verify, label %not_boundary

verify:
  ; Verify 16-period rhythm
  %cycle_num = udiv i64 %step, %page_size
  %rhythm_mod = urem i64 %cycle_num, 16
  %sum = call i64 @atlas.c768.compute_window_sum(ptr %structure, i64 %step, i64 256)
  %sum_mod = urem i64 %sum, 96
  ; At rhythm points, sum should be 0 mod 96
  %rhythm_ok = icmp eq i64 %rhythm_mod, 0
  %sum_ok = icmp eq i64 %sum_mod, 0
  %result = and i1 %rhythm_ok, %sum_ok
  ret i1 %result

not_boundary:
  ret i1 true
}

; Check byte rhythm (3×256 alignment)
define i1 @atlas.c768.check_byte_rhythm(ptr %structure, i64 %step) nounwind {
entry:
  ; Check if we're at a 256-byte boundary
  %mod = urem i64 %step, 256
  %at_boundary = icmp eq i64 %mod, 0
  br i1 %at_boundary, label %verify, label %not_boundary

verify:
  ; Verify 3-period rhythm
  %cycle_num = udiv i64 %step, 256
  %rhythm_mod = urem i64 %cycle_num, 3
  %sum = call i64 @atlas.c768.compute_window_sum(ptr %structure, i64 %step, i64 256)
  %sum_mod = urem i64 %sum, 96
  ; Every 3rd window should complete a cycle
  %is_third = icmp eq i64 %rhythm_mod, 2
  br i1 %is_third, label %check_closure, label %regular

check_closure:
  %closed = icmp eq i64 %sum_mod, 0
  ret i1 %closed

regular:
  ret i1 true

not_boundary:
  ret i1 true
}

; Find next cycle point
define i64 @atlas.c768.next_cycle_point(i64 %current) nounwind readnone {
entry:
  %remainder = urem i64 %current, 768
  %distance = sub i64 768, %remainder
  %next = add i64 %current, %distance
  ret i64 %next
}

; Verify all windows in cycle sum to 0 mod 96
define i1 @atlas.c768.verify_all_windows(ptr %structure) nounwind {
entry:
  br label %window_loop

window_loop:
  %w = phi i64 [ 0, %entry ], [ %next_w, %window_body ]
  %all_ok = phi i1 [ true, %entry ], [ %still_ok, %window_body ]
  %w_cmp = icmp ult i64 %w, 16
  br i1 %w_cmp, label %window_body, label %done

window_body:
  %window_start = mul i64 %w, 48
  %window_bytes = mul i64 %window_start, 256
  %sum = call i64 @atlas.c768.compute_window_sum(ptr %structure, i64 %window_bytes, i64 12288)
  %mod = urem i64 %sum, 96
  %ok = icmp eq i64 %mod, 0
  %still_ok = and i1 %all_ok, %ok
  %next_w = add i64 %w, 1
  br label %window_loop

done:
  ret i1 %all_ok
}

; Get residue classes for cycle analysis
define void @atlas.c768.compute_residue_classes(ptr %structure, ptr %output) nounwind {
entry:
  ; Initialize residue class counters (96 classes)
  br label %init_loop

init_loop:
  %i = phi i64 [ 0, %entry ], [ %next_i, %init_body ]
  %cmp = icmp ult i64 %i, 96
  br i1 %cmp, label %init_body, label %scan

init_body:
  %ptr = getelementptr i32, ptr %output, i64 %i
  store i32 0, ptr %ptr
  %next_i = add i64 %i, 1
  br label %init_loop

scan:
  br label %scan_loop

scan_loop:
  %j = phi i64 [ 0, %scan ], [ %next_j, %scan_body ]
  %j_cmp = icmp ult i64 %j, 12288
  br i1 %j_cmp, label %scan_body, label %complete

scan_body:
  %byte_ptr = getelementptr i8, ptr %structure, i64 %j
  %byte = load i8, ptr %byte_ptr
  %class = call i7 @atlas.r96.classify(i8 %byte)
  %class_ext = zext i7 %class to i64
  %count_ptr = getelementptr i32, ptr %output, i64 %class_ext
  %old_count = load i32, ptr %count_ptr
  %new_count = add i32 %old_count, 1
  store i32 %new_count, ptr %count_ptr
  %next_j = add i64 %j, 1
  br label %scan_loop

complete:
  ret void
}

; Phase-lock verification between page and byte cycles
define i1 @atlas.c768.verify_phase_lock(ptr %structure) nounwind {
entry:
  ; Check that page rhythm (16-period) and byte rhythm (3-period)
  ; are properly phase-locked at LCM = 768
  
  ; Sample at key phase points
  %point1 = call i1 @atlas.c768.check_page_rhythm(ptr %structure, i64 0)
  %point2 = call i1 @atlas.c768.check_byte_rhythm(ptr %structure, i64 0)
  %phase1_ok = and i1 %point1, %point2
  
  ; Check at 256-byte mark (1/3 of cycle)
  %point3 = call i1 @atlas.c768.check_byte_rhythm(ptr %structure, i64 256)
  %phase2_ok = and i1 %phase1_ok, %point3
  
  ; Check at 12288-byte mark (full page set)
  %point4 = call i1 @atlas.c768.check_page_rhythm(ptr %structure, i64 12288)
  %phase3_ok = and i1 %phase2_ok, %point4
  
  ; Verify complete closure at 768
  %closure = call i1 @atlas.c768.verify_closure(ptr %structure, i64 0)
  %all_locked = and i1 %phase3_ok, %closure
  
  ret i1 %all_locked
}

; External dependency
declare i7 @atlas.r96.classify(i8) nounwind readnone

; =============================================================================
; Module metadata and flags
; =============================================================================

!llvm.module.flags = !{!0, !1}
!0 = !{i32 1, !"wchar_size", i32 4}
!1 = !{i32 7, !"PIC Level", i32 2}

; Atlas C768 metadata
!llvm.ident = !{!2}
!2 = !{!"Atlas-12288 C768 Module v1.0"}