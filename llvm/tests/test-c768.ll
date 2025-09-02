; test-c768.ll - Test 768-cycle closure, page/byte rhythms, window sum conservation, phase-locking
; Tests the C768 triple-cycle conservation system

source_filename = "test-c768.ll"
target datalayout = "e-m:e-p270:32:32-p271:32:32-p272:64:64-i64:64-i128:128-f80:128-n8:16:32:64-S128"
target triple = "x86_64-unknown-linux-gnu"

; External functions from atlas-12288-c768.ll
declare i1 @atlas.c768.verify_closure(ptr, i64) nounwind
declare i64 @atlas.c768.compute_window_sum(ptr, i64, i64) nounwind readonly
declare i1 @atlas.c768.check_page_rhythm(ptr, i64) nounwind
declare i1 @atlas.c768.check_byte_rhythm(ptr, i64) nounwind
declare i64 @atlas.c768.next_cycle_point(i64) nounwind readnone
declare i1 @atlas.c768.verify_all_windows(ptr) nounwind
declare void @atlas.c768.compute_residue_classes(ptr, ptr) nounwind
declare i1 @atlas.c768.verify_phase_lock(ptr) nounwind

; External functions from other modules
declare ptr @atlas.alloc.aligned(i64) nounwind
declare i32 @printf(ptr, ...)
declare void @llvm.memset.p0.i64(ptr, i8, i64, i1) nounwind

; String constants for output
@.str.pass = private unnamed_addr constant [27 x i8] c"C768 Test PASSED - All OK\0A\00"
@.str.fail = private unnamed_addr constant [27 x i8] c"C768 Test FAILED - Error: \00"
@.str.closure = private unnamed_addr constant [16 x i8] c"closure failed\0A\00"
@.str.window_sum = private unnamed_addr constant [19 x i8] c"window sum failed\0A\00"
@.str.page_rhythm = private unnamed_addr constant [20 x i8] c"page rhythm failed\0A\00"
@.str.byte_rhythm = private unnamed_addr constant [20 x i8] c"byte rhythm failed\0A\00"
@.str.phase_lock = private unnamed_addr constant [19 x i8] c"phase lock failed\0A\00"
@.str.all_windows = private unnamed_addr constant [20 x i8] c"all windows failed\0A\00"

define i32 @test_768_cycle_closure() {
entry:
  ; Allocate test structure
  %structure = call ptr @atlas.alloc.aligned(i64 12288)
  
  ; Initialize with a pattern that should have proper closure
  ; Fill with values that will create proper 768-cycle behavior
  call void @llvm.memset.p0.i64(ptr %structure, i8 1, i64 12288, i1 false)
  
  ; Test 768-cycle closure at window start 0
  %closure1 = call i1 @atlas.c768.verify_closure(ptr %structure, i64 0)
  br i1 %closure1, label %test_window_sum, label %fail_closure

test_window_sum:
  ; Test window sum computation
  %sum = call i64 @atlas.c768.compute_window_sum(ptr %structure, i64 0, i64 768)
  %sum_valid = icmp ne i64 %sum, 0  ; Should be non-zero for filled structure
  br i1 %sum_valid, label %test_page_rhythm, label %fail_window_sum

test_page_rhythm:
  ; Test page rhythm at boundary
  %page_ok = call i1 @atlas.c768.check_page_rhythm(ptr %structure, i64 0)
  br i1 %page_ok, label %test_byte_rhythm, label %fail_page_rhythm

test_byte_rhythm:
  ; Test byte rhythm at boundary
  %byte_ok = call i1 @atlas.c768.check_byte_rhythm(ptr %structure, i64 0)
  br i1 %byte_ok, label %test_phase_lock, label %fail_byte_rhythm

test_phase_lock:
  ; Test phase-locking between page and byte rhythms
  %phase_ok = call i1 @atlas.c768.verify_phase_lock(ptr %structure)
  br i1 %phase_ok, label %test_all_windows, label %fail_phase_lock

test_all_windows:
  ; Test that all windows in cycle sum correctly
  %all_ok = call i1 @atlas.c768.verify_all_windows(ptr %structure)
  br i1 %all_ok, label %test_cycle_point, label %fail_all_windows

test_cycle_point:
  ; Test cycle point calculation
  %next_point = call i64 @atlas.c768.next_cycle_point(i64 100)
  %expected = icmp eq i64 %next_point, 768  ; 100 -> next 768 boundary
  br i1 %expected, label %pass, label %fail_all_windows

pass:
  %msg_pass = getelementptr [28 x i8], ptr @.str.pass, i32 0, i32 0
  call i32 @printf(ptr %msg_pass)
  ret i32 0

fail_closure:
  %msg_fail = getelementptr [28 x i8], ptr @.str.fail, i32 0, i32 0
  %msg_closure = getelementptr [16 x i8], ptr @.str.closure, i32 0, i32 0
  call i32 @printf(ptr %msg_fail)
  call i32 @printf(ptr %msg_closure)
  ret i32 1

fail_window_sum:
  %msg_fail2 = getelementptr [28 x i8], ptr @.str.fail, i32 0, i32 0
  %msg_window = getelementptr [21 x i8], ptr @.str.window_sum, i32 0, i32 0
  call i32 @printf(ptr %msg_fail2)
  call i32 @printf(ptr %msg_window)
  ret i32 1

fail_page_rhythm:
  %msg_fail3 = getelementptr [28 x i8], ptr @.str.fail, i32 0, i32 0
  %msg_page = getelementptr [22 x i8], ptr @.str.page_rhythm, i32 0, i32 0
  call i32 @printf(ptr %msg_fail3)
  call i32 @printf(ptr %msg_page)
  ret i32 1

fail_byte_rhythm:
  %msg_fail4 = getelementptr [28 x i8], ptr @.str.fail, i32 0, i32 0
  %msg_byte = getelementptr [22 x i8], ptr @.str.byte_rhythm, i32 0, i32 0
  call i32 @printf(ptr %msg_fail4)
  call i32 @printf(ptr %msg_byte)
  ret i32 1

fail_phase_lock:
  %msg_fail5 = getelementptr [28 x i8], ptr @.str.fail, i32 0, i32 0
  %msg_phase = getelementptr [21 x i8], ptr @.str.phase_lock, i32 0, i32 0
  call i32 @printf(ptr %msg_fail5)
  call i32 @printf(ptr %msg_phase)
  ret i32 1

fail_all_windows:
  %msg_fail6 = getelementptr [28 x i8], ptr @.str.fail, i32 0, i32 0
  %msg_all = getelementptr [24 x i8], ptr @.str.all_windows, i32 0, i32 0
  call i32 @printf(ptr %msg_fail6)
  call i32 @printf(ptr %msg_all)
  ret i32 1
}

define i32 @main() {
entry:
  %result = call i32 @test_768_cycle_closure()
  ret i32 %result
}