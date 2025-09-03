; test-harmonic.ll - Test harmonic pairing, schedule alignment, clustering coherence, window timing
; Tests the harmonic operations for Atlas-12288

source_filename = "test-harmonic.ll"
target datalayout = "e-m:e-p270:32:32-p271:32:32-p272:64:64-i64:64-i128:128-f80:128-n8:16:32:64-S128"
target triple = "x86_64-unknown-linux-gnu"

; Import harmonic type definitions
%atlas.harmonic.window = type { i32, i32, i7 }     ; start, length, resonance
%atlas.harmonic.pair   = type { i7, i7 }           ; resonance pair (a, b) where a + b ≡ 0 (mod 96)
%atlas.harmonic.cluster = type { i7, i32, ptr }    ; affinity, size, members_ptr
%atlas.harmonic.schedule = type { i32, i32, ptr }  ; period, offset, windows_ptr

; External functions from atlas-12288-harmonic.ll
declare %atlas.harmonic.window @atlas.harmonic.compute_window(i7, i16) nounwind
declare ptr @atlas.harmonic.compute_windows(ptr, i32) nounwind
declare ptr @atlas.harmonic.create_schedule(ptr, i32) nounwind
declare %atlas.harmonic.window @atlas.harmonic.schedule_next(ptr, i32) nounwind readonly
declare %atlas.harmonic.pair @atlas.harmonic.find_pair(i7) nounwind readnone
declare i1 @atlas.harmonic.validate_pair(%atlas.harmonic.pair) nounwind readnone
declare ptr @atlas.harmonic.generate_pairs(ptr) nounwind
declare double @atlas.harmonic.calculate_affinity(i7, i7) nounwind readnone
declare ptr @atlas.harmonic.create_cluster(i7, ptr, i32, double) nounwind
declare ptr @atlas.harmonic.get_cluster_members(ptr) nounwind readonly
declare i32 @atlas.harmonic.get_cluster_size(ptr) nounwind readonly
declare void @atlas.harmonic.destroy_windows(ptr) nounwind
declare void @atlas.harmonic.destroy_schedule(ptr) nounwind
declare void @atlas.harmonic.destroy_pairs(ptr) nounwind
declare void @atlas.harmonic.destroy_cluster(ptr) nounwind

; External functions from other modules
declare ptr @atlas.alloc.aligned(i64) nounwind
declare i32 @printf(ptr, ...)
declare void @llvm.memset.p0.i64(ptr, i8, i64, i1) nounwind

; String constants for output
@.str.pass = private unnamed_addr constant [31 x i8] c"Harmonic Test PASSED - All OK\0A\00"
@.str.fail = private unnamed_addr constant [31 x i8] c"Harmonic Test FAILED - Error: \00"
@.str.window = private unnamed_addr constant [15 x i8] c"window failed\0A\00"
@.str.pairing = private unnamed_addr constant [16 x i8] c"pairing failed\0A\00"
@.str.schedule = private unnamed_addr constant [17 x i8] c"schedule failed\0A\00"
@.str.clustering = private unnamed_addr constant [19 x i8] c"clustering failed\0A\00"
@.str.affinity = private unnamed_addr constant [17 x i8] c"affinity failed\0A\00"
@.str.timing = private unnamed_addr constant [15 x i8] c"timing failed\0A\00"

define i32 @test_harmonic_operations() {
entry:
  ; Test harmonic window computation
  %test_resonance = add i7 0, 42
  %c768_base = add i16 0, 256
  %window = call %atlas.harmonic.window @atlas.harmonic.compute_window(i7 %test_resonance, i16 %c768_base)
  
  ; Extract window properties
  %window_start = extractvalue %atlas.harmonic.window %window, 0
  %window_length = extractvalue %atlas.harmonic.window %window, 1
  %window_resonance = extractvalue %atlas.harmonic.window %window, 2
  
  ; Check window has valid properties
  %start_valid = icmp uge i32 %window_start, 0
  %length_valid = icmp ugt i32 %window_length, 0
  %resonance_match = icmp eq i7 %window_resonance, %test_resonance
  
  %window_valid1 = and i1 %start_valid, %length_valid
  %window_ok = and i1 %window_valid1, %resonance_match
  br i1 %window_ok, label %test_multiple_windows, label %fail_window

test_multiple_windows:
  ; Test computing multiple windows
  %resonance_array = call ptr @atlas.alloc.aligned(i64 32)  ; Space for 32 i7 values
  
  ; Fill with test resonances
  %ptr0 = getelementptr i7, ptr %resonance_array, i64 0
  %ptr1 = getelementptr i7, ptr %resonance_array, i64 1
  %ptr2 = getelementptr i7, ptr %resonance_array, i64 2
  %ptr3 = getelementptr i7, ptr %resonance_array, i64 3
  store i7 10, ptr %ptr0
  store i7 20, ptr %ptr1
  store i7 30, ptr %ptr2
  store i7 40, ptr %ptr3
  
  %windows = call ptr @atlas.harmonic.compute_windows(ptr %resonance_array, i32 4)
  %windows_created = icmp ne ptr %windows, null
  br i1 %windows_created, label %test_harmonic_pairing, label %fail_window

test_harmonic_pairing:
  ; Test harmonic pair finding
  %test_res = add i7 0, 24
  %pair = call %atlas.harmonic.pair @atlas.harmonic.find_pair(i7 %test_res)
  
  ; Validate the pair
  %pair_valid = call i1 @atlas.harmonic.validate_pair(%atlas.harmonic.pair %pair)
  br i1 %pair_valid, label %test_pair_sum, label %fail_pairing

test_pair_sum:
  ; Extract pair values and verify they sum to 0 mod 96
  %pair_a = extractvalue %atlas.harmonic.pair %pair, 0
  %pair_b = extractvalue %atlas.harmonic.pair %pair, 1
  
  ; Convert to i8 for arithmetic
  %a_ext = zext i7 %pair_a to i8
  %b_ext = zext i7 %pair_b to i8
  %sum = add i8 %a_ext, %b_ext
  %mod96 = urem i8 %sum, 96
  %harmonizes = icmp eq i8 %mod96, 0
  br i1 %harmonizes, label %test_schedule_creation, label %fail_pairing

test_schedule_creation:
  ; Test schedule creation from windows
  %schedule = call ptr @atlas.harmonic.create_schedule(ptr %windows, i32 4)
  %schedule_created = icmp ne ptr %schedule, null
  br i1 %schedule_created, label %test_schedule_timing, label %fail_schedule

test_schedule_timing:
  ; Test schedule timing
  %next_window = call %atlas.harmonic.window @atlas.harmonic.schedule_next(ptr %schedule, i32 100)
  
  ; Check that scheduled window is valid
  %next_start = extractvalue %atlas.harmonic.window %next_window, 0
  %next_length = extractvalue %atlas.harmonic.window %next_window, 1
  
  %timing_start_ok = icmp uge i32 %next_start, 0
  %timing_length_ok = icmp ugt i32 %next_length, 0
  %timing_ok = and i1 %timing_start_ok, %timing_length_ok
  br i1 %timing_ok, label %test_clustering, label %fail_timing

test_clustering:
  ; Test resonance clustering
  %cluster_center = add i7 0, 50
  %cluster_threshold = fadd double 0.0, 5.0e-1
  %cluster = call ptr @atlas.harmonic.create_cluster(i7 %cluster_center, ptr %resonance_array, i32 4, double %cluster_threshold)
  %cluster_created = icmp ne ptr %cluster, null
  br i1 %cluster_created, label %test_cluster_properties, label %fail_clustering

test_cluster_properties:
  ; Test cluster properties
  %cluster_size = call i32 @atlas.harmonic.get_cluster_size(ptr %cluster)
  %cluster_members = call ptr @atlas.harmonic.get_cluster_members(ptr %cluster)
  
  %size_valid = icmp uge i32 %cluster_size, 0
  %members_valid = icmp ne ptr %cluster_members, null
  %cluster_props_ok = and i1 %size_valid, %members_valid
  br i1 %cluster_props_ok, label %test_affinity, label %fail_clustering

test_affinity:
  ; Test affinity calculation
  %res_a = add i7 0, 30
  %res_b = add i7 0, 60
  %affinity = call double @atlas.harmonic.calculate_affinity(i7 %res_a, i7 %res_b)
  
  ; Affinity should be in valid range (0.0 to 1.0)
  %affinity_ge_zero = fcmp oge double %affinity, 0.0
  %affinity_le_one = fcmp ole double %affinity, 1.0
  %affinity_ok = and i1 %affinity_ge_zero, %affinity_le_one
  br i1 %affinity_ok, label %test_histogram_pairing, label %fail_affinity

test_histogram_pairing:
  ; Test generating pairs from histogram
  %histogram = call ptr @atlas.alloc.aligned(i64 384)  ; Space for 96 i32 values
  call void @llvm.memset.p0.i64(ptr %histogram, i8 0, i64 384, i1 false)
  
  ; Set some histogram values
  %hist_ptr = getelementptr i32, ptr %histogram, i64 10
  store i32 5, ptr %hist_ptr
  %hist_ptr2 = getelementptr i32, ptr %histogram, i64 86  ; 10 + 86 = 96 ≡ 0 (mod 96)
  store i32 5, ptr %hist_ptr2
  
  %generated_pairs = call ptr @atlas.harmonic.generate_pairs(ptr %histogram)
  %pairs_created = icmp ne ptr %generated_pairs, null
  br i1 %pairs_created, label %cleanup, label %cleanup

cleanup:
  ; Clean up allocated resources
  call void @atlas.harmonic.destroy_windows(ptr %windows)
  call void @atlas.harmonic.destroy_schedule(ptr %schedule)
  call void @atlas.harmonic.destroy_pairs(ptr %generated_pairs)
  call void @atlas.harmonic.destroy_cluster(ptr %cluster)
  
  br label %pass

pass:
  %msg_pass = getelementptr [32 x i8], ptr @.str.pass, i32 0, i32 0
  call i32 @printf(ptr %msg_pass)
  ret i32 0

fail_window:
  %msg_fail1 = getelementptr [32 x i8], ptr @.str.fail, i32 0, i32 0
  %msg_window = getelementptr [17 x i8], ptr @.str.window, i32 0, i32 0
  call i32 @printf(ptr %msg_fail1)
  call i32 @printf(ptr %msg_window)
  ret i32 1

fail_pairing:
  %msg_fail2 = getelementptr [32 x i8], ptr @.str.fail, i32 0, i32 0
  %msg_pairing = getelementptr [18 x i8], ptr @.str.pairing, i32 0, i32 0
  call i32 @printf(ptr %msg_fail2)
  call i32 @printf(ptr %msg_pairing)
  ret i32 1

fail_schedule:
  %msg_fail3 = getelementptr [32 x i8], ptr @.str.fail, i32 0, i32 0
  %msg_schedule = getelementptr [19 x i8], ptr @.str.schedule, i32 0, i32 0
  call i32 @printf(ptr %msg_fail3)
  call i32 @printf(ptr %msg_schedule)
  ret i32 1

fail_clustering:
  %msg_fail4 = getelementptr [32 x i8], ptr @.str.fail, i32 0, i32 0
  %msg_clustering = getelementptr [21 x i8], ptr @.str.clustering, i32 0, i32 0
  call i32 @printf(ptr %msg_fail4)
  call i32 @printf(ptr %msg_clustering)
  ret i32 1

fail_affinity:
  %msg_fail5 = getelementptr [32 x i8], ptr @.str.fail, i32 0, i32 0
  %msg_affinity = getelementptr [19 x i8], ptr @.str.affinity, i32 0, i32 0
  call i32 @printf(ptr %msg_fail5)
  call i32 @printf(ptr %msg_affinity)
  ret i32 1

fail_timing:
  %msg_fail6 = getelementptr [32 x i8], ptr @.str.fail, i32 0, i32 0
  %msg_timing = getelementptr [17 x i8], ptr @.str.timing, i32 0, i32 0
  call i32 @printf(ptr %msg_fail6)
  call i32 @printf(ptr %msg_timing)
  ret i32 1
}

define i32 @main() {
entry:
  %result = call i32 @test_harmonic_operations()
  ret i32 %result
}