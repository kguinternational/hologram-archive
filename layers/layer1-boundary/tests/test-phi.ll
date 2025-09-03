; test-morphisms.ll - Test boundary automorphisms, selector gauge transforms, NF-lift round-trips, morphism composition
; Tests the structure-preserving maps and morphism operations

source_filename = "test-morphisms.ll"
target datalayout = "e-m:e-p270:32:32-p271:32:32-p272:64:64-i64:64-i128:128-f80:128-n8:16:32:64-S128"
target triple = "x86_64-unknown-linux-gnu"

; External functions from atlas-12288-morphisms.ll
declare i32 @atlas.morphism.boundary_auto(i32, i8, i8) nounwind readnone
declare i1 @atlas.morphism.is_valid_unit(i8, i8) nounwind readnone
declare void @atlas.morphism.apply_auto_to_structure(ptr, i8, i8) nounwind
declare void @atlas.morphism.permute_toggles(ptr, ptr) nounwind
declare void @atlas.morphism.complement_toggles(ptr) nounwind
declare i1 @atlas.morphism.verify_unity_constraint(ptr) nounwind readonly
declare i1 @atlas.morphism.verify_pin_constraint(ptr) nounwind readonly
declare void @atlas.morphism.apply_gauge_transform(ptr, ptr) nounwind
declare ptr @atlas.morphism.nf_lift(ptr, i64) nounwind
declare ptr @atlas.morphism.nf_project(ptr) nounwind
declare i1 @atlas.morphism.verify_roundtrip(ptr, ptr) nounwind readonly
declare void @atlas.morphism.compute_normal_form(ptr, ptr) nounwind
declare i1 @atlas.morphism.preserves_histogram(ptr, ptr) nounwind readonly
declare void @atlas.morphism.apply_evaluator(ptr, ptr) nounwind
declare void @atlas.morphism.klein_window_character(ptr, ptr) nounwind
declare i1 @atlas.morphism.verify_character_orthogonality(ptr, ptr) nounwind readonly
declare i7 @atlas.morphism.rl_compose(i7, i7) nounwind readnone
declare i1 @atlas.morphism.rl_conservative_collapse(i7) nounwind readnone
declare void @atlas.morphism.apply_rl_arrow(ptr, ptr) nounwind

; External functions from other modules
declare ptr @atlas.alloc.aligned(i64) nounwind
declare i32 @printf(ptr, ...)
declare void @llvm.memset.p0.i64(ptr, i8, i64, i1) nounwind
declare void @llvm.memcpy.p0.p0.i64(ptr, ptr, i64, i1) nounwind

; String constants for output
@.str.pass = private unnamed_addr constant [32 x i8] c"Morphisms Test PASSED - All OK\0A\00"
@.str.fail = private unnamed_addr constant [32 x i8] c"Morphisms Test FAILED - Error: \00"
@.str.boundary_auto = private unnamed_addr constant [22 x i8] c"boundary auto failed\0A\00"
@.str.valid_units = private unnamed_addr constant [20 x i8] c"valid units failed\0A\00"
@.str.gauge_transform = private unnamed_addr constant [24 x i8] c"gauge transform failed\0A\00"
@.str.nf_roundtrip = private unnamed_addr constant [21 x i8] c"NF roundtrip failed\0A\00"
@.str.histogram = private unnamed_addr constant [18 x i8] c"histogram failed\0A\00"
@.str.rl_compose = private unnamed_addr constant [19 x i8] c"RL compose failed\0A\00"

define i32 @test_boundary_automorphisms() {
entry:
  ; Test boundary automorphism with valid units
  %coord = or i32 256, 128  ; page=1, byte=128
  %u48 = add i8 0, 5   ; coprime to 48
  %u256 = add i8 0, 3  ; coprime to 256 (odd)
  
  ; Check units are valid
  %units_valid = call i1 @atlas.morphism.is_valid_unit(i8 %u48, i8 %u256)
  br i1 %units_valid, label %test_auto, label %fail_valid_units

test_auto:
  ; Apply boundary automorphism
  %new_coord = call i32 @atlas.morphism.boundary_auto(i32 %coord, i8 %u48, i8 %u256)
  
  ; Should get a different coordinate
  %changed = icmp ne i32 %coord, %new_coord
  br i1 %changed, label %test_structure_auto, label %fail_boundary_auto

test_structure_auto:
  ; Test applying automorphism to full structure
  %structure = call ptr @atlas.alloc.aligned(i64 12288)
  call void @llvm.memset.p0.i64(ptr %structure, i8 42, i64 12288, i1 false)
  
  %orig = call ptr @atlas.alloc.aligned(i64 12288)
  call void @llvm.memcpy.p0.p0.i64(ptr %orig, ptr %structure, i64 12288, i1 false)
  
  call void @atlas.morphism.apply_auto_to_structure(ptr %structure, i8 %u48, i8 %u256)
  
  ; Check that structure changed (should be different after automorphism)
  %preserved = call i1 @atlas.morphism.verify_roundtrip(ptr %orig, ptr %structure)
  %changed_struct = xor i1 %preserved, true  ; Should be different
  br i1 %changed_struct, label %test_gauge, label %fail_boundary_auto

test_gauge:
  ; Test gauge transforms
  %gauge = call ptr @atlas.alloc.aligned(i64 8)
  call void @llvm.memset.p0.i64(ptr %gauge, i8 1, i64 8, i1 false)
  
  ; Create toggle basis
  %toggles = call ptr @atlas.alloc.aligned(i64 8)
  %ptr0 = getelementptr i8, ptr %toggles, i64 0
  %ptr1 = getelementptr i8, ptr %toggles, i64 1
  %ptr2 = getelementptr i8, ptr %toggles, i64 2
  store i8 1, ptr %ptr0  ; unity pair element 1
  store i8 1, ptr %ptr1  ; unity pair element 2
  store i8 1, ptr %ptr2  ; pinned oscillator
  
  ; Verify unity constraint
  %unity_ok = call i1 @atlas.morphism.verify_unity_constraint(ptr %toggles)
  br i1 %unity_ok, label %test_pin, label %fail_gauge_transform

test_pin:
  ; Verify pin constraint
  %pin_ok = call i1 @atlas.morphism.verify_pin_constraint(ptr %toggles)
  br i1 %pin_ok, label %test_complement, label %fail_gauge_transform

test_complement:
  ; Test complement operation
  call void @atlas.morphism.complement_toggles(ptr %toggles)
  %val0 = load i8, ptr %ptr0
  %is_complemented = icmp eq i8 %val0, 0  ; Should be flipped
  br i1 %is_complemented, label %test_nf_lift, label %fail_gauge_transform

test_nf_lift:
  ; Test NF-lift operations
  %boundary = call ptr @atlas.alloc.aligned(i64 256)
  call void @llvm.memset.p0.i64(ptr %boundary, i8 1, i64 256, i1 false)
  
  %bulk = call ptr @atlas.morphism.nf_lift(ptr %boundary, i64 256)
  %projected = call ptr @atlas.morphism.nf_project(ptr %bulk)
  
  ; Test roundtrip (should preserve structure)
  %roundtrip_ok = call i1 @atlas.morphism.verify_roundtrip(ptr %boundary, ptr %projected)
  br i1 %roundtrip_ok, label %test_histogram, label %fail_nf_roundtrip

test_histogram:
  ; Test histogram preservation
  %test_data = call ptr @atlas.alloc.aligned(i64 12288)
  call void @llvm.memset.p0.i64(ptr %test_data, i8 7, i64 12288, i1 false)
  
  %hist_preserved = call i1 @atlas.morphism.preserves_histogram(ptr %test_data, ptr %test_data)
  br i1 %hist_preserved, label %test_rl_operations, label %fail_histogram

test_rl_operations:
  ; Test RL semiring operations
  %a = add i7 0, 10
  %b = add i7 0, 20
  %composed = call i7 @atlas.morphism.rl_compose(i7 %a, i7 %b)
  %expected = add i7 0, 30  ; 10 + 20 = 30
  %compose_ok = icmp eq i7 %composed, %expected
  br i1 %compose_ok, label %test_collapse, label %fail_rl_compose

test_collapse:
  ; Test conservative collapse
  %budget = add i7 0, 42
  %collapsed = call i1 @atlas.morphism.rl_conservative_collapse(i7 %budget)
  %should_be_true = icmp eq i1 %collapsed, true  ; Non-zero budget -> true
  br i1 %should_be_true, label %pass, label %fail_rl_compose

pass:
  %msg_pass = getelementptr [32 x i8], ptr @.str.pass, i32 0, i32 0
  call i32 @printf(ptr %msg_pass)
  ret i32 0

fail_boundary_auto:
  %msg_fail1 = getelementptr [32 x i8], ptr @.str.fail, i32 0, i32 0
  %msg_boundary = getelementptr [25 x i8], ptr @.str.boundary_auto, i32 0, i32 0
  call i32 @printf(ptr %msg_fail1)
  call i32 @printf(ptr %msg_boundary)
  ret i32 1

fail_valid_units:
  %msg_fail2 = getelementptr [32 x i8], ptr @.str.fail, i32 0, i32 0
  %msg_units = getelementptr [23 x i8], ptr @.str.valid_units, i32 0, i32 0
  call i32 @printf(ptr %msg_fail2)
  call i32 @printf(ptr %msg_units)
  ret i32 1

fail_gauge_transform:
  %msg_fail3 = getelementptr [32 x i8], ptr @.str.fail, i32 0, i32 0
  %msg_gauge = getelementptr [27 x i8], ptr @.str.gauge_transform, i32 0, i32 0
  call i32 @printf(ptr %msg_fail3)
  call i32 @printf(ptr %msg_gauge)
  ret i32 1

fail_nf_roundtrip:
  %msg_fail4 = getelementptr [32 x i8], ptr @.str.fail, i32 0, i32 0
  %msg_nf = getelementptr [22 x i8], ptr @.str.nf_roundtrip, i32 0, i32 0
  call i32 @printf(ptr %msg_fail4)
  call i32 @printf(ptr %msg_nf)
  ret i32 1

fail_histogram:
  %msg_fail5 = getelementptr [32 x i8], ptr @.str.fail, i32 0, i32 0
  %msg_hist = getelementptr [21 x i8], ptr @.str.histogram, i32 0, i32 0
  call i32 @printf(ptr %msg_fail5)
  call i32 @printf(ptr %msg_hist)
  ret i32 1

fail_rl_compose:
  %msg_fail6 = getelementptr [32 x i8], ptr @.str.fail, i32 0, i32 0
  %msg_rl = getelementptr [22 x i8], ptr @.str.rl_compose, i32 0, i32 0
  call i32 @printf(ptr %msg_fail6)
  call i32 @printf(ptr %msg_rl)
  ret i32 1
}

define i32 @main() {
entry:
  %result = call i32 @test_boundary_automorphisms()
  ret i32 %result
}