; atlas-12288-scheduling.ll - Resonance scheduling operations for Layer 3
; ---------------------------------------------------------------------------------
; This module implements the deterministic resonance scheduling algorithm with
; phase-locked scheduling aligned to mod-96 boundaries for optimal processing.
;
; Key Features:
;  • Harmonic window calculation functions based on resonance classes
;  • Phase-locked scheduling aligned to mod-96 boundaries
;  • Next window computation: next = now + ((96 - ((now + r) % 96)) % 96)
;  • Batch scheduling operations for high-throughput processing
;  • SIMD-optimized batch window calculations
;
; Dependencies:
;  • atlas-12288-r96.ll (resonance classification)
;  • atlas-12288-harmonic.ll (harmonic operations)
; ---------------------------------------------------------------------------------

source_filename = "atlas-12288-scheduling.ll"

; =============================================================================
; Type definitions for scheduling data structures
; =============================================================================

%atlas.schedule.window = type { i32, i32, i7, i32 }       ; start, length, resonance, phase
%atlas.schedule.batch = type { i32, ptr, ptr, ptr }       ; count, windows_ptr, times_ptr, results_ptr
%atlas.schedule.config = type { i32, i32, double, i1 }    ; period, offset, phase_factor, locked
%atlas.harmonic.sequence = type { i32, ptr, double }      ; length, resonances_ptr, frequency
%atlas.harmonic.window = type { i32, i32, i7 }           ; start, length, resonance (from harmonic.ll)

; =============================================================================
; Constants for scheduling operations
; =============================================================================

@atlas.scheduling.mod_base = internal constant i32 96, align 4
@atlas.scheduling.phase_lock_period = internal constant i32 768, align 4  ; LCM(96, 768/96) = 768
@atlas.scheduling.default_phase_factor = internal constant double 1.618033988749895, align 8  ; Golden ratio
@atlas.scheduling.max_window_size = internal constant i32 4096, align 4

; Phase alignment table for common resonance classes (precomputed for performance)
@atlas.scheduling.phase_table = internal constant [96 x i32] [
  i32 0,   i32 1,   i32 2,   i32 3,   i32 4,   i32 5,   i32 6,   i32 7,
  i32 8,   i32 9,   i32 10,  i32 11,  i32 12,  i32 13,  i32 14,  i32 15,
  i32 16,  i32 17,  i32 18,  i32 19,  i32 20,  i32 21,  i32 22,  i32 23,
  i32 24,  i32 25,  i32 26,  i32 27,  i32 28,  i32 29,  i32 30,  i32 31,
  i32 32,  i32 33,  i32 34,  i32 35,  i32 36,  i32 37,  i32 38,  i32 39,
  i32 40,  i32 41,  i32 42,  i32 43,  i32 44,  i32 45,  i32 46,  i32 47,
  i32 48,  i32 49,  i32 50,  i32 51,  i32 52,  i32 53,  i32 54,  i32 55,
  i32 56,  i32 57,  i32 58,  i32 59,  i32 60,  i32 61,  i32 62,  i32 63,
  i32 64,  i32 65,  i32 66,  i32 67,  i32 68,  i32 69,  i32 70,  i32 71,
  i32 72,  i32 73,  i32 74,  i32 75,  i32 76,  i32 77,  i32 78,  i32 79,
  i32 80,  i32 81,  i32 82,  i32 83,  i32 84,  i32 85,  i32 86,  i32 87,
  i32 88,  i32 89,  i32 90,  i32 91,  i32 92,  i32 93,  i32 94,  i32 95
], align 64

; =============================================================================
; External function declarations
; =============================================================================

; From atlas-12288-r96.ll
declare i7 @atlas.r96.classify(i8) nounwind readnone willreturn
declare i1 @atlas.r96.harmonizes(i7, i7) nounwind readnone willreturn

; From atlas-12288-harmonic.ll
declare %atlas.harmonic.window @atlas.harmonic.compute_window(i7, i16) nounwind
declare ptr @atlas.harmonic.compute_windows(ptr, i32) nounwind

; Standard library functions
declare ptr @malloc(i64)
declare ptr @calloc(i64, i64)
declare void @free(ptr)
declare void @llvm.memset.p0.i64(ptr nocapture, i8, i64, i1)
declare void @llvm.memcpy.p0.p0.i64(ptr nocapture, ptr nocapture readonly, i64, i1)

; =============================================================================
; Core scheduling functions
; =============================================================================

; Calculate next harmonic window using the deterministic scheduling algorithm
; Formula: next = now + ((96 - ((now + r) % 96)) % 96)
define i32 @atlas.scheduling.next_window(i32 %now, i7 %resonance) nounwind readnone willreturn {
entry:
  ; Convert resonance to i32 for arithmetic
  %r = zext i7 %resonance to i32
  
  ; Calculate (now + r)
  %now_plus_r = add i32 %now, %r
  
  ; Calculate (now + r) % 96
  %mod_base = load i32, ptr @atlas.scheduling.mod_base, align 4
  %inner_mod = urem i32 %now_plus_r, %mod_base
  
  ; Calculate 96 - ((now + r) % 96)
  %complement = sub i32 %mod_base, %inner_mod
  
  ; Calculate ((96 - ((now + r) % 96)) % 96)
  %outer_mod = urem i32 %complement, %mod_base
  
  ; Calculate next = now + ((96 - ((now + r) % 96)) % 96)
  %next = add i32 %now, %outer_mod
  
  ret i32 %next
}

; Calculate harmonic window with phase alignment
define %atlas.schedule.window @atlas.scheduling.compute_harmonic_window(i32 %now, i7 %resonance, i32 %base_length) nounwind {
entry:
  ; Get next aligned window position
  %next_pos = call i32 @atlas.scheduling.next_window(i32 %now, i7 %resonance)
  
  ; Calculate window length based on resonance and golden ratio
  %r_i32 = zext i7 %resonance to i32
  %phase_factor = load double, ptr @atlas.scheduling.default_phase_factor, align 8
  %r_double = uitofp i32 %r_i32 to double
  %scaled_length = fmul double %r_double, %phase_factor
  %length_double = fadd double %scaled_length, 64.0  ; Base length 64
  %calculated_length = fptoui double %length_double to i32
  
  ; Ensure length is within reasonable bounds
  %min_length = select i1 true, i32 %calculated_length, i32 32  ; Minimum 32
  %max_window_size = load i32, ptr @atlas.scheduling.max_window_size, align 4
  %length_cmp = icmp ugt i32 %min_length, %max_window_size
  %final_length = select i1 %length_cmp, i32 %max_window_size, i32 %min_length
  
  ; Calculate phase based on resonance class
  %res_idx = zext i7 %resonance to i32
  %phase_ptr = getelementptr [96 x i32], ptr @atlas.scheduling.phase_table, i32 0, i32 %res_idx
  %phase = load i32, ptr %phase_ptr, align 4, !invariant.load !0
  
  ; Construct window structure
  %window = insertvalue %atlas.schedule.window undef, i32 %next_pos, 0
  %window2 = insertvalue %atlas.schedule.window %window, i32 %final_length, 1
  %window3 = insertvalue %atlas.schedule.window %window2, i7 %resonance, 2
  %window4 = insertvalue %atlas.schedule.window %window3, i32 %phase, 3
  
  ret %atlas.schedule.window %window4
}

; Phase-locked scheduling with mod-96 alignment
define i1 @atlas.scheduling.is_phase_locked(i32 %time, i7 %resonance) nounwind readnone willreturn {
entry:
  ; Calculate expected phase position
  %next_window = call i32 @atlas.scheduling.next_window(i32 %time, i7 %resonance)
  
  ; Check if current time is aligned to the computed next window
  %mod_base = load i32, ptr @atlas.scheduling.mod_base, align 4
  %time_mod = urem i32 %time, %mod_base
  %next_mod = urem i32 %next_window, %mod_base
  
  %is_locked = icmp eq i32 %time_mod, %next_mod
  ret i1 %is_locked
}

; Get optimal scheduling window for current resonance state
define %atlas.schedule.window @atlas.scheduling.get_optimal_window(ptr %resonance_data, i32 %data_size, i32 %current_time) nounwind {
entry:
  ; Find dominant resonance class in data
  %dominant_resonance = call i7 @atlas.r96.dominant(ptr %resonance_data)
  
  ; Calculate base window length from data size
  %base_length = udiv i32 %data_size, 4  ; Use 1/4 of data size as base
  
  ; Compute harmonic window
  %window = call %atlas.schedule.window @atlas.scheduling.compute_harmonic_window(i32 %current_time, i7 %dominant_resonance, i32 %base_length)
  
  ret %atlas.schedule.window %window
}

; =============================================================================
; Batch scheduling operations
; =============================================================================

; Create batch scheduler for multiple windows
define ptr @atlas.scheduling.create_batch_scheduler(i32 %max_windows) nounwind {
entry:
  ; Allocate batch structure
  %batch_ptr = call ptr @malloc(i64 32)  ; sizeof(atlas.schedule.batch) = 32
  
  ; Allocate arrays for windows, times, and results
  %windows_size = mul i32 %max_windows, 20  ; sizeof(atlas.schedule.window) = 20
  %times_size = mul i32 %max_windows, 4    ; sizeof(i32) = 4
  %results_size = mul i32 %max_windows, 4  ; sizeof(i32) = 4
  
  %windows_size_64 = zext i32 %windows_size to i64
  %times_size_64 = zext i32 %times_size to i64
  %results_size_64 = zext i32 %results_size to i64
  
  %windows_ptr = call ptr @malloc(i64 %windows_size_64)
  %times_ptr = call ptr @malloc(i64 %times_size_64)
  %results_ptr = call ptr @malloc(i64 %results_size_64)
  
  ; Initialize batch structure
  %batch = insertvalue %atlas.schedule.batch undef, i32 0, 0  ; count = 0
  %batch2 = insertvalue %atlas.schedule.batch %batch, ptr %windows_ptr, 1
  %batch3 = insertvalue %atlas.schedule.batch %batch2, ptr %times_ptr, 2
  %batch4 = insertvalue %atlas.schedule.batch %batch3, ptr %results_ptr, 3
  
  store %atlas.schedule.batch %batch4, ptr %batch_ptr, align 8
  ret ptr %batch_ptr
}

; Add window to batch scheduler
define void @atlas.scheduling.batch_add_window(ptr %batch_ptr, %atlas.schedule.window %window, i32 %time) nounwind {
entry:
  %batch = load %atlas.schedule.batch, ptr %batch_ptr, align 8
  %current_count = extractvalue %atlas.schedule.batch %batch, 0
  %windows_ptr = extractvalue %atlas.schedule.batch %batch, 1
  %times_ptr = extractvalue %atlas.schedule.batch %batch, 2
  
  ; Store window and time at current count position
  %window_addr = getelementptr %atlas.schedule.window, ptr %windows_ptr, i32 %current_count
  %time_addr = getelementptr i32, ptr %times_ptr, i32 %current_count
  
  store %atlas.schedule.window %window, ptr %window_addr, align 4
  store i32 %time, ptr %time_addr, align 4
  
  ; Update count
  %new_count = add i32 %current_count, 1
  %updated_batch = insertvalue %atlas.schedule.batch %batch, i32 %new_count, 0
  store %atlas.schedule.batch %updated_batch, ptr %batch_ptr, align 8
  ret void
}

; Process all windows in batch using vectorized operations
define void @atlas.scheduling.batch_process(ptr %batch_ptr, i32 %base_time) nounwind {
entry:
  %batch = load %atlas.schedule.batch, ptr %batch_ptr, align 8
  %count = extractvalue %atlas.schedule.batch %batch, 0
  %windows_ptr = extractvalue %atlas.schedule.batch %batch, 1
  %times_ptr = extractvalue %atlas.schedule.batch %batch, 2
  %results_ptr = extractvalue %atlas.schedule.batch %batch, 3
  
  br label %process_loop, !llvm.loop !1
  
process_loop:
  %i = phi i32 [ 0, %entry ], [ %next_i, %process_body ]
  %done = icmp uge i32 %i, %count
  br i1 %done, label %complete, label %process_body
  
process_body:
  ; Load window and time
  %window_addr = getelementptr %atlas.schedule.window, ptr %windows_ptr, i32 %i
  %time_addr = getelementptr i32, ptr %times_ptr, i32 %i
  %result_addr = getelementptr i32, ptr %results_ptr, i32 %i
  
  %window = load %atlas.schedule.window, ptr %window_addr, align 4
  %time = load i32, ptr %time_addr, align 4
  
  ; Extract window properties
  %start = extractvalue %atlas.schedule.window %window, 0
  %length = extractvalue %atlas.schedule.window %window, 1
  %resonance = extractvalue %atlas.schedule.window %window, 2
  %phase = extractvalue %atlas.schedule.window %window, 3
  
  ; Calculate next window time using scheduling algorithm
  %adjusted_time = add i32 %base_time, %time
  %next_time = call i32 @atlas.scheduling.next_window(i32 %adjusted_time, i7 %resonance)
  
  ; Check if window is phase-locked
  %is_locked = call i1 @atlas.scheduling.is_phase_locked(i32 %next_time, i7 %resonance)
  %result = zext i1 %is_locked to i32
  
  ; Store result
  store i32 %result, ptr %result_addr, align 4
  
  %next_i = add i32 %i, 1
  br label %process_loop, !llvm.loop !1
  
complete:
  ret void
}

; =============================================================================
; Vectorized scheduling operations
; =============================================================================

; Vectorized next window calculation for 4 resonances at once
define <4 x i32> @atlas.scheduling.next_window_v4(<4 x i32> %now_vec, <4 x i7> %resonance_vec) nounwind readnone willreturn {
entry:
  ; Extend resonances to i32 for arithmetic
  %r_vec_i32 = zext <4 x i7> %resonance_vec to <4 x i32>
  
  ; Calculate (now + r) for all elements
  %now_plus_r_vec = add <4 x i32> %now_vec, %r_vec_i32
  
  ; Create mod_base vector (96, 96, 96, 96)
  %mod_base_scalar = load i32, ptr @atlas.scheduling.mod_base, align 4
  %mod_base_vec = insertelement <4 x i32> undef, i32 %mod_base_scalar, i32 0
  %mod_base_vec2 = insertelement <4 x i32> %mod_base_vec, i32 %mod_base_scalar, i32 1
  %mod_base_vec3 = insertelement <4 x i32> %mod_base_vec2, i32 %mod_base_scalar, i32 2
  %mod_base_vec4 = insertelement <4 x i32> %mod_base_vec3, i32 %mod_base_scalar, i32 3
  
  ; Calculate (now + r) % 96 for all elements
  %inner_mod_vec = urem <4 x i32> %now_plus_r_vec, %mod_base_vec4
  
  ; Calculate 96 - ((now + r) % 96) for all elements
  %complement_vec = sub <4 x i32> %mod_base_vec4, %inner_mod_vec
  
  ; Calculate ((96 - ((now + r) % 96)) % 96) for all elements
  %outer_mod_vec = urem <4 x i32> %complement_vec, %mod_base_vec4
  
  ; Calculate next = now + ((96 - ((now + r) % 96)) % 96) for all elements
  %next_vec = add <4 x i32> %now_vec, %outer_mod_vec
  
  ret <4 x i32> %next_vec
}

; Batch calculate multiple harmonic windows using SIMD
define void @atlas.scheduling.compute_harmonic_windows_batch(ptr %now_array, ptr %resonance_array, ptr %windows_array, i32 %count) nounwind {
entry:
  ; Process 4 elements at a time using vectorization
  %vec_count = udiv i32 %count, 4
  %remainder = urem i32 %count, 4
  
  br label %vec_loop
  
vec_loop:
  %vec_i = phi i32 [ 0, %entry ], [ %next_vec_i, %continue_vec ]
  %vec_done = icmp uge i32 %vec_i, %vec_count
  br i1 %vec_done, label %remainder_loop, label %vec_body
  
vec_body:
  ; Load 4 elements from each array
  %base_idx = mul i32 %vec_i, 4
  
  ; Load now values
  %now_addr = getelementptr i32, ptr %now_array, i32 %base_idx
  %now_ptr = bitcast ptr %now_addr to ptr
  %now_vec = load <4 x i32>, ptr %now_ptr, align 16
  
  ; Load resonance values  
  %res_addr = getelementptr i7, ptr %resonance_array, i32 %base_idx
  %res_ptr = bitcast ptr %res_addr to ptr
  %res_vec = load <4 x i7>, ptr %res_ptr, align 4
  
  ; Calculate next windows using vectorized operation
  %next_vec = call <4 x i32> @atlas.scheduling.next_window_v4(<4 x i32> %now_vec, <4 x i7> %res_vec)
  
  ; Calculate individual window properties and store
  br label %store_loop
  
store_loop:
  %store_i = phi i32 [ 0, %vec_body ], [ %next_store_i, %store_body ]
  %store_done = icmp uge i32 %store_i, 4
  br i1 %store_done, label %continue_vec, label %store_body
  
store_body:
  %elem_idx = add i32 %base_idx, %store_i
  %next_pos = extractelement <4 x i32> %next_vec, i32 %store_i
  %resonance = extractelement <4 x i7> %res_vec, i32 %store_i
  
  ; Calculate window length (simplified for batch processing)
  %r_i32 = zext i7 %resonance to i32
  %base_length = mul i32 %r_i32, 8  ; Simple scaling: r * 8
  %length_min = select i1 true, i32 %base_length, i32 64  ; Minimum 64
  %max_size = load i32, ptr @atlas.scheduling.max_window_size, align 4
  %length_cmp = icmp ugt i32 %length_min, %max_size
  %final_length = select i1 %length_cmp, i32 %max_size, i32 %length_min
  
  ; Get phase from table
  %res_idx = zext i7 %resonance to i32
  %phase_ptr = getelementptr [96 x i32], ptr @atlas.scheduling.phase_table, i32 0, i32 %res_idx
  %phase = load i32, ptr %phase_ptr, align 4, !invariant.load !0
  
  ; Create and store window
  %window = insertvalue %atlas.schedule.window undef, i32 %next_pos, 0
  %window2 = insertvalue %atlas.schedule.window %window, i32 %final_length, 1
  %window3 = insertvalue %atlas.schedule.window %window2, i7 %resonance, 2
  %window4 = insertvalue %atlas.schedule.window %window3, i32 %phase, 3
  
  %window_addr = getelementptr %atlas.schedule.window, ptr %windows_array, i32 %elem_idx
  store %atlas.schedule.window %window4, ptr %window_addr, align 4
  
  %next_store_i = add i32 %store_i, 1
  br label %store_loop
  
continue_vec:
  %next_vec_i = add i32 %vec_i, 1
  br label %vec_loop
  
remainder_loop:
  ; Process remaining elements individually
  %rem_base_idx = mul i32 %vec_count, 4
  br label %rem_loop
  
rem_loop:
  %rem_i = phi i32 [ 0, %remainder_loop ], [ %next_rem_i, %rem_body ]
  %rem_done = icmp uge i32 %rem_i, %remainder
  br i1 %rem_done, label %complete_batch, label %rem_body
  
rem_body:
  %rem_idx = add i32 %rem_base_idx, %rem_i
  
  ; Load individual elements
  %rem_now_ptr = getelementptr i32, ptr %now_array, i32 %rem_idx
  %rem_res_ptr = getelementptr i7, ptr %resonance_array, i32 %rem_idx
  %now = load i32, ptr %rem_now_ptr, align 4
  %rem_resonance = load i7, ptr %rem_res_ptr, align 1
  
  ; Calculate window using scalar function
  %rem_window = call %atlas.schedule.window @atlas.scheduling.compute_harmonic_window(i32 %now, i7 %rem_resonance, i32 256)
  
  ; Store result
  %rem_window_addr = getelementptr %atlas.schedule.window, ptr %windows_array, i32 %rem_idx
  store %atlas.schedule.window %rem_window, ptr %rem_window_addr, align 4
  
  %next_rem_i = add i32 %rem_i, 1
  br label %rem_loop
  
complete_batch:
  ret void
}

; =============================================================================
; Advanced scheduling functions
; =============================================================================

; Create harmonic sequence for optimized scheduling
define ptr @atlas.scheduling.create_harmonic_sequence(ptr %resonance_data, i32 %data_size) nounwind {
entry:
  ; Allocate sequence structure
  %sequence_ptr = call ptr @malloc(i64 24)  ; sizeof(atlas.harmonic.sequence) = 24
  
  ; Allocate array for unique resonances found in data
  %resonances_array = call ptr @malloc(i64 96)  ; Max 96 resonances, 1 byte each
  call void @llvm.memset.p0.i64(ptr %resonances_array, i8 0, i64 96, i1 false)
  
  %unique_count = alloca i32, align 4
  store i32 0, ptr %unique_count, align 4
  
  ; Track which resonances we've seen
  %seen_mask = alloca [96 x i1], align 1
  call void @llvm.memset.p0.i64(ptr %seen_mask, i8 0, i64 96, i1 false)
  
  ; Scan data to find unique resonances
  br label %scan_loop
  
scan_loop:
  %scan_i = phi i32 [ 0, %entry ], [ %next_scan_i, %continue_scan ]
  %scan_done = icmp uge i32 %scan_i, %data_size
  br i1 %scan_done, label %calculate_frequency, label %scan_body
  
scan_body:
  ; Classify current byte
  %data_ptr = getelementptr i8, ptr %resonance_data, i32 %scan_i
  %byte = load i8, ptr %data_ptr, align 1
  %resonance = call i7 @atlas.r96.classify(i8 %byte)
  %res_idx = zext i7 %resonance to i32
  
  ; Check if we've seen this resonance before
  %seen_ptr = getelementptr [96 x i1], ptr %seen_mask, i32 0, i32 %res_idx
  %already_seen = load i1, ptr %seen_ptr, align 1
  br i1 %already_seen, label %continue_scan, label %add_resonance
  
add_resonance:
  ; Mark as seen
  store i1 true, ptr %seen_ptr, align 1
  
  ; Add to unique resonances array
  %current_count = load i32, ptr %unique_count, align 4
  %res_array_ptr = getelementptr i8, ptr %resonances_array, i32 %current_count
  %resonance_i8 = zext i7 %resonance to i8
  store i8 %resonance_i8, ptr %res_array_ptr, align 1
  
  ; Update count
  %new_count = add i32 %current_count, 1
  store i32 %new_count, ptr %unique_count, align 4
  br label %continue_scan
  
continue_scan:
  %next_scan_i = add i32 %scan_i, 1
  br label %scan_loop
  
calculate_frequency:
  ; Calculate harmonic frequency based on unique resonance count and golden ratio
  %final_count = load i32, ptr %unique_count, align 4
  %count_double = uitofp i32 %final_count to double
  %golden_ratio = load double, ptr @atlas.scheduling.default_phase_factor, align 8
  %frequency = fdiv double %count_double, %golden_ratio
  
  ; Create sequence structure
  %sequence = insertvalue %atlas.harmonic.sequence undef, i32 %final_count, 0
  %sequence2 = insertvalue %atlas.harmonic.sequence %sequence, ptr %resonances_array, 1
  %sequence3 = insertvalue %atlas.harmonic.sequence %sequence2, double %frequency, 2
  
  store %atlas.harmonic.sequence %sequence3, ptr %sequence_ptr, align 8
  ret ptr %sequence_ptr
}

; Apply harmonic sequence to generate optimal scheduling windows
define ptr @atlas.scheduling.apply_harmonic_sequence(ptr %sequence_ptr, i32 %base_time, i32 %window_count) nounwind {
entry:
  %sequence = load %atlas.harmonic.sequence, ptr %sequence_ptr, align 8
  %length = extractvalue %atlas.harmonic.sequence %sequence, 0
  %resonances_ptr = extractvalue %atlas.harmonic.sequence %sequence, 1
  %frequency = extractvalue %atlas.harmonic.sequence %sequence, 2
  
  ; Allocate windows array
  %windows_size = mul i32 %window_count, 20  ; sizeof(atlas.schedule.window) = 20
  %windows_size_64 = zext i32 %windows_size to i64
  %windows_array = call ptr @malloc(i64 %windows_size_64)
  
  br label %generate_loop
  
generate_loop:
  %gen_i = phi i32 [ 0, %entry ], [ %next_gen_i, %generate_body ]
  %gen_done = icmp uge i32 %gen_i, %window_count
  br i1 %gen_done, label %complete_generation, label %generate_body
  
generate_body:
  ; Select resonance using harmonic pattern
  %resonance_idx = urem i32 %gen_i, %length
  %resonance_ptr = getelementptr i8, ptr %resonances_ptr, i32 %resonance_idx
  %resonance_i8 = load i8, ptr %resonance_ptr, align 1
  %resonance = trunc i8 %resonance_i8 to i7
  
  ; Calculate time offset using harmonic frequency
  %i_double = uitofp i32 %gen_i to double
  %time_offset_double = fmul double %i_double, %frequency
  %time_offset = fptoui double %time_offset_double to i32
  %current_time = add i32 %base_time, %time_offset
  
  ; Generate window
  %window = call %atlas.schedule.window @atlas.scheduling.compute_harmonic_window(i32 %current_time, i7 %resonance, i32 512)
  
  ; Store window
  %window_addr = getelementptr %atlas.schedule.window, ptr %windows_array, i32 %gen_i
  store %atlas.schedule.window %window, ptr %window_addr, align 4
  
  %next_gen_i = add i32 %gen_i, 1
  br label %generate_loop
  
complete_generation:
  ret ptr %windows_array
}

; =============================================================================
; Cleanup functions
; =============================================================================

; Destroy batch scheduler
define void @atlas.scheduling.destroy_batch_scheduler(ptr %batch_ptr) nounwind {
entry:
  %batch = load %atlas.schedule.batch, ptr %batch_ptr, align 8
  %windows_ptr = extractvalue %atlas.schedule.batch %batch, 1
  %times_ptr = extractvalue %atlas.schedule.batch %batch, 2
  %results_ptr = extractvalue %atlas.schedule.batch %batch, 3
  
  call void @free(ptr %windows_ptr)
  call void @free(ptr %times_ptr)
  call void @free(ptr %results_ptr)
  call void @free(ptr %batch_ptr)
  ret void
}

; Destroy harmonic sequence
define void @atlas.scheduling.destroy_harmonic_sequence(ptr %sequence_ptr) nounwind {
entry:
  %sequence = load %atlas.harmonic.sequence, ptr %sequence_ptr, align 8
  %resonances_ptr = extractvalue %atlas.harmonic.sequence %sequence, 1
  
  call void @free(ptr %resonances_ptr)
  call void @free(ptr %sequence_ptr)
  ret void
}

; =============================================================================
; External function declarations (from atlas-12288-r96.ll)
; =============================================================================

declare i7 @atlas.r96.dominant(ptr) nounwind

; =============================================================================
; Loop optimization metadata
; =============================================================================

!0 = !{}  ; for invariant loads
!1 = !{!1, !2, !3}
!2 = !{!"llvm.loop.vectorize.enable", i1 true}
!3 = !{!"llvm.loop.vectorize.width", i32 4}

; =============================================================================
; Exported symbols for linking
; =============================================================================

@llvm.used = appending global [15 x ptr] [
  ptr @atlas.scheduling.next_window,
  ptr @atlas.scheduling.compute_harmonic_window,
  ptr @atlas.scheduling.is_phase_locked,
  ptr @atlas.scheduling.get_optimal_window,
  ptr @atlas.scheduling.create_batch_scheduler,
  ptr @atlas.scheduling.batch_add_window,
  ptr @atlas.scheduling.batch_process,
  ptr @atlas.scheduling.next_window_v4,
  ptr @atlas.scheduling.compute_harmonic_windows_batch,
  ptr @atlas.scheduling.create_harmonic_sequence,
  ptr @atlas.scheduling.apply_harmonic_sequence,
  ptr @atlas.scheduling.destroy_batch_scheduler,
  ptr @atlas.scheduling.destroy_harmonic_sequence,
  ptr @atlas.scheduling.get_optimal_window,
  ptr @atlas.scheduling.is_phase_locked
], section "llvm.metadata"

; ---------------------------------------------------------------------------------
; End of atlas-12288-scheduling.ll
; ---------------------------------------------------------------------------------