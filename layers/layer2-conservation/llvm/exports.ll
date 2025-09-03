; exports.ll - C ABI exports for Layer 2 Conservation functions
; Provides C-compatible names for LLVM IR witness and conservation functions

; External declarations of internal witness functions
declare ptr @atlas.witness.generate(ptr, i64)
declare i1 @atlas.witness.verify(ptr, ptr, i64)
declare void @atlas.witness.destroy(ptr)

; External declarations of conservation functions
; NOTE: atlas.conserved.delta is from Layer 0, imported directly
; NOTE: atlas.conserved.check is from Layer 0, not redeclared here
; NOTE: atlas.conserved.window.check is from Layer 0, not redeclared here  
declare i32 @atlas.conserved.sum(ptr, i64)

; External declarations of batch operations
declare void @atlas.batch.conserved_check(ptr, i32, ptr)
declare ptr @atlas.batch.delta_compute(ptr, i32)
declare ptr @atlas.batch.witness_generate(ptr, i32)
declare void @atlas.batch.get_statistics(ptr)
declare void @atlas.batch.reset_statistics()

; External declarations of Layer 2 specific conservation functions
declare i1 @atlas.conserved.window.streaming_check(ptr, i64, i64)
declare void @atlas.conserved.batch_update(ptr, i32, ptr)
declare i7 @atlas.conserved.delta_cached(ptr, ptr, i64, ptr)

; C-compatible exports for witness functions
define ptr @atlas_witness_generate_llvm(ptr %data, i64 %len) {
    %witness = call ptr @atlas.witness.generate(ptr %data, i64 %len)
    ret ptr %witness
}

define i1 @atlas_witness_verify_llvm(ptr %witness, ptr %data, i64 %len) {
    %valid = call i1 @atlas.witness.verify(ptr %witness, ptr %data, i64 %len)
    ret i1 %valid
}

define void @atlas_witness_destroy_llvm(ptr %witness) {
    call void @atlas.witness.destroy(ptr %witness)
    ret void
}

; NOTE: atlas_conserved_delta is exported by Layer 0, not re-exported here

; NOTE: atlas_conserved_check is exported by Layer 0, not re-exported here
; NOTE: atlas_conserved_window_check is exported by Layer 0, not re-exported here

; NOTE: atlas_conserved_sum is exported by Layer 0, not re-exported here

; C-compatible exports for batch operations  
define void @atlas_batch_conserved_check_llvm(ptr %buffers, i32 %count, ptr %results) {
    call void @atlas.batch.conserved_check(ptr %buffers, i32 %count, ptr %results)
    ret void
}

define ptr @atlas_batch_delta_compute_llvm(ptr %deltas, i32 %count) {
    %result = call ptr @atlas.batch.delta_compute(ptr %deltas, i32 %count)
    ret ptr %result
}

define ptr @atlas_batch_witness_generate_llvm(ptr %witnesses, i32 %count) {
    %result = call ptr @atlas.batch.witness_generate(ptr %witnesses, i32 %count)
    ret ptr %result
}

define void @atlas_batch_get_statistics_llvm(ptr %stats) {
    call void @atlas.batch.get_statistics(ptr %stats)
    ret void
}

define void @atlas_batch_reset_statistics_llvm() {
    call void @atlas.batch.reset_statistics()
    ret void
}

; C-compatible exports for Layer 2 conservation functions
define i1 @atlas_conserved_window_streaming_check_llvm(ptr %data, i64 %len, i64 %window_size) {
    %result = call i1 @atlas.conserved.window.streaming_check(ptr %data, i64 %len, i64 %window_size)
    ret i1 %result
}

define void @atlas_conserved_batch_update_llvm(ptr %buffers, i32 %count, ptr %state) {
    call void @atlas.conserved.batch_update(ptr %buffers, i32 %count, ptr %state)
    ret void
}

define i8 @atlas_conserved_delta_cached_llvm(ptr %before, ptr %after, i64 %len, ptr %cache) {
    %delta_i7 = call i7 @atlas.conserved.delta_cached(ptr %before, ptr %after, i64 %len, ptr %cache)
    %delta_i8 = zext i7 %delta_i7 to i8
    ret i8 %delta_i8
}