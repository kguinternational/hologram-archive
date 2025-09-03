; atlas-12288-clustering.ll - CSR clustering LLVM kernels for Layer 3 Resonance
; ---------------------------------------------------------------------------------
; This module implements high-performance CSR (Compressed Sparse Row) construction
; and clustering operations organized by resonance class for efficient data access.
; 
; Key Features:
;  • Fast CSR building functions optimized for 96-class resonance data
;  • Clustering by resonance class with optimized sparse matrix representation  
;  • SIMD-optimized histogram generation for clustering analysis
;  • Batch operations for processing large datasets efficiently
;
; Dependencies:
;  • atlas-12288-r96.ll (resonance classification)
;  • atlas-12288-simd.ll (vector operations)
; ---------------------------------------------------------------------------------

source_filename = "atlas-12288-clustering.ll"

; =============================================================================
; Type definitions for CSR data structures
; =============================================================================

%atlas.csr.matrix = type { i32, i32, ptr, ptr, ptr }    ; rows, cols, values, col_indices, row_pointers
%atlas.resonance.cluster = type { i7, i32, ptr, ptr }  ; resonance_class, size, indices_ptr, values_ptr
%atlas.cluster.stats = type { i32, i32, double }       ; count, density, affinity_score

; =============================================================================
; Constants for clustering operations
; =============================================================================

@atlas.clustering.resonance_classes = internal constant i32 96, align 4
@atlas.clustering.max_cluster_size = internal constant i32 4096, align 4
@atlas.clustering.density_threshold = internal constant double 0.75, align 8

; =============================================================================
; External function declarations
; =============================================================================

; From atlas-12288-r96.ll
declare i7 @atlas.r96.classify(i8) nounwind readnone willreturn
declare void @atlas.r96.histogram.page(ptr, ptr) nounwind readonly willreturn
declare i7 @atlas.r96.dominant(ptr) nounwind

; From atlas-12288-simd.ll  
declare <16 x i7> @atlas.r96.classify.sse2(<16 x i8>) nounwind readnone willreturn
declare <32 x i7> @atlas.r96.classify.avx2(<32 x i8>) nounwind readnone willreturn
declare i16 @atlas.conserved.sum.sse2(<16 x i8>) nounwind readnone willreturn

; Standard library functions
declare ptr @malloc(i64)
declare ptr @calloc(i64, i64) 
declare ptr @realloc(ptr, i64)
declare void @free(ptr)
declare void @llvm.memset.p0.i64(ptr nocapture, i8, i64, i1)
declare void @llvm.memcpy.p0.p0.i64(ptr nocapture, ptr nocapture readonly, i64, i1)

; =============================================================================
; Fast CSR matrix construction
; =============================================================================

; Initialize empty CSR matrix for given dimensions
define ptr @atlas.csr.create(i32 %rows, i32 %cols) nounwind {
entry:
  ; Allocate CSR matrix structure
  %matrix_ptr = call ptr @malloc(i64 40)  ; sizeof(atlas.csr.matrix) = 40
  
  ; Allocate row pointers (rows + 1 elements)
  %rows_plus_one = add i32 %rows, 1
  %row_ptr_size = mul i32 %rows_plus_one, 4
  %row_ptr_size_64 = zext i32 %row_ptr_size to i64
  %row_pointers = call ptr @calloc(i64 %row_ptr_size_64, i64 1)
  
  ; Initialize matrix structure
  %matrix = insertvalue %atlas.csr.matrix undef, i32 %rows, 0
  %matrix2 = insertvalue %atlas.csr.matrix %matrix, i32 %cols, 1
  %matrix3 = insertvalue %atlas.csr.matrix %matrix2, ptr null, 2  ; values (allocated later)
  %matrix4 = insertvalue %atlas.csr.matrix %matrix3, ptr null, 3  ; col_indices (allocated later)
  %matrix5 = insertvalue %atlas.csr.matrix %matrix4, ptr %row_pointers, 4
  
  store %atlas.csr.matrix %matrix5, ptr %matrix_ptr, align 8
  ret ptr %matrix_ptr
}

; Fast CSR building from resonance-classified data
define void @atlas.csr.build_from_resonance(ptr %matrix_ptr, ptr %data, i32 %data_size, i7 %target_resonance) nounwind {
entry:
  %matrix = load %atlas.csr.matrix, ptr %matrix_ptr, align 8
  %rows = extractvalue %atlas.csr.matrix %matrix, 0
  %row_pointers = extractvalue %atlas.csr.matrix %matrix, 4
  
  ; First pass: count non-zero elements per row for target resonance
  br label %count_loop
  
count_loop:
  %i = phi i32 [ 0, %entry ], [ %next_i, %continue_count ]
  %nnz_count = phi i32 [ 0, %entry ], [ %final_nnz_count, %continue_count ]
  %done_count = icmp uge i32 %i, %data_size
  br i1 %done_count, label %allocate_arrays, label %count_body
  
count_body:
  ; Load and classify byte
  %data_ptr = getelementptr i8, ptr %data, i32 %i
  %byte = load i8, ptr %data_ptr, align 1
  %resonance = call i7 @atlas.r96.classify(i8 %byte)
  
  ; Check if matches target resonance
  %matches = icmp eq i7 %resonance, %target_resonance
  br i1 %matches, label %increment_count, label %continue_count
  
increment_count:
  %new_nnz_count = add i32 %nnz_count, 1
  
  ; Update row pointer (assuming row = i / cols)
  %cols = extractvalue %atlas.csr.matrix %matrix, 1
  %row_idx = udiv i32 %i, %cols
  %row_cmp = icmp ult i32 %row_idx, %rows
  br i1 %row_cmp, label %update_row_ptr, label %increment_continue
  
update_row_ptr:
  %row_ptr_addr = getelementptr i32, ptr %row_pointers, i32 %row_idx
  %old_row_count = load i32, ptr %row_ptr_addr, align 4
  %new_row_count = add i32 %old_row_count, 1
  store i32 %new_row_count, ptr %row_ptr_addr, align 4
  br label %increment_continue
  
increment_continue:
  br label %continue_count
  
continue_count:
  %final_nnz_count = phi i32 [ %new_nnz_count, %increment_continue ], [ %nnz_count, %count_body ]
  %next_i = add i32 %i, 1
  br label %count_loop
  
allocate_arrays:
  ; Allocate values and column indices arrays
  %nnz_size = mul i32 %nnz_count, 4  ; i32 size
  %nnz_size_64 = zext i32 %nnz_size to i64
  %values = call ptr @malloc(i64 %nnz_size_64)
  %col_indices = call ptr @malloc(i64 %nnz_size_64)
  
  ; Convert row counts to cumulative offsets
  br label %prefix_sum_loop
  
prefix_sum_loop:
  %j = phi i32 [ 0, %allocate_arrays ], [ %next_j, %prefix_sum_body ]
  %cumulative = phi i32 [ 0, %allocate_arrays ], [ %new_cumulative, %prefix_sum_body ]
  %prefix_done = icmp ugt i32 %j, %rows  ; rows + 1
  br i1 %prefix_done, label %fill_data, label %prefix_sum_body
  
prefix_sum_body:
  %row_ptr_addr2 = getelementptr i32, ptr %row_pointers, i32 %j
  %old_count = load i32, ptr %row_ptr_addr2, align 4
  store i32 %cumulative, ptr %row_ptr_addr2, align 4
  %new_cumulative = add i32 %cumulative, %old_count
  %next_j = add i32 %j, 1
  br label %prefix_sum_loop
  
fill_data:
  ; Second pass: fill values and column indices
  %current_positions = alloca [96 x i32], align 16  ; Track current position per row
  call void @llvm.memcpy.p0.p0.i64(ptr %current_positions, ptr %row_pointers, i64 384, i1 false)
  
  br label %fill_loop
  
fill_loop:
  %k = phi i32 [ 0, %fill_data ], [ %next_k, %fill_continue ]
  %fill_done = icmp uge i32 %k, %data_size
  br i1 %fill_done, label %update_matrix, label %fill_body
  
fill_body:
  ; Load and classify byte
  %data_ptr2 = getelementptr i8, ptr %data, i32 %k
  %byte2 = load i8, ptr %data_ptr2, align 1
  %resonance2 = call i7 @atlas.r96.classify(i8 %byte2)
  
  ; Check if matches target resonance
  %matches2 = icmp eq i7 %resonance2, %target_resonance
  br i1 %matches2, label %store_element, label %fill_continue
  
store_element:
  %cols2 = extractvalue %atlas.csr.matrix %matrix, 1
  %row_idx2 = udiv i32 %k, %cols2
  %col_idx2 = urem i32 %k, %cols2
  %row_cmp2 = icmp ult i32 %row_idx2, %rows
  br i1 %row_cmp2, label %do_store, label %fill_continue
  
do_store:
  ; Get current position for this row
  %pos_ptr = getelementptr [96 x i32], ptr %current_positions, i32 0, i32 %row_idx2
  %pos = load i32, ptr %pos_ptr, align 4
  
  ; Store value and column index
  %val_ptr = getelementptr i32, ptr %values, i32 %pos
  %col_ptr = getelementptr i32, ptr %col_indices, i32 %pos
  %byte2_i32 = zext i8 %byte2 to i32
  store i32 %byte2_i32, ptr %val_ptr, align 4
  store i32 %col_idx2, ptr %col_ptr, align 4
  
  ; Update position
  %new_pos = add i32 %pos, 1
  store i32 %new_pos, ptr %pos_ptr, align 4
  br label %fill_continue
  
fill_continue:
  %next_k = add i32 %k, 1
  br label %fill_loop
  
update_matrix:
  ; Update matrix structure with allocated arrays
  %updated_matrix = insertvalue %atlas.csr.matrix %matrix, ptr %values, 2
  %updated_matrix2 = insertvalue %atlas.csr.matrix %updated_matrix, ptr %col_indices, 3
  store %atlas.csr.matrix %updated_matrix2, ptr %matrix_ptr, align 8
  ret void
}

; =============================================================================
; Resonance clustering operations
; =============================================================================

; Create resonance cluster for specific class
define ptr @atlas.clustering.create_cluster(i7 %resonance_class, i32 %initial_capacity) nounwind {
entry:
  ; Allocate cluster structure
  %cluster_ptr = call ptr @malloc(i64 32)  ; sizeof(atlas.resonance.cluster) = 32
  
  ; Allocate indices and values arrays
  %capacity_64 = zext i32 %initial_capacity to i64
  %indices_size = mul i64 %capacity_64, 4  ; i32 indices
  %values_size = mul i64 %capacity_64, 1   ; i8 values
  %indices = call ptr @malloc(i64 %indices_size)
  %values = call ptr @malloc(i64 %values_size)
  
  ; Initialize cluster structure
  %cluster = insertvalue %atlas.resonance.cluster undef, i7 %resonance_class, 0
  %cluster2 = insertvalue %atlas.resonance.cluster %cluster, i32 0, 1  ; size = 0
  %cluster3 = insertvalue %atlas.resonance.cluster %cluster2, ptr %indices, 2
  %cluster4 = insertvalue %atlas.resonance.cluster %cluster3, ptr %values, 3
  
  store %atlas.resonance.cluster %cluster4, ptr %cluster_ptr, align 8
  ret ptr %cluster_ptr
}

; Add element to resonance cluster
define void @atlas.clustering.add_to_cluster(ptr %cluster_ptr, i32 %index, i8 %value, i32 %capacity) nounwind {
entry:
  %cluster = load %atlas.resonance.cluster, ptr %cluster_ptr, align 8
  %current_size = extractvalue %atlas.resonance.cluster %cluster, 1
  %indices_ptr = extractvalue %atlas.resonance.cluster %cluster, 2
  %values_ptr = extractvalue %atlas.resonance.cluster %cluster, 3
  
  ; Check if we need to resize
  %needs_resize = icmp uge i32 %current_size, %capacity
  br i1 %needs_resize, label %resize_arrays, label %add_element
  
resize_arrays:
  ; Double the capacity
  %new_capacity = shl i32 %capacity, 1
  %new_cap_64 = zext i32 %new_capacity to i64
  %new_indices_size = mul i64 %new_cap_64, 4
  %new_values_size = mul i64 %new_cap_64, 1
  
  ; Reallocate arrays
  %new_indices = call ptr @realloc(ptr %indices_ptr, i64 %new_indices_size)
  %new_values = call ptr @realloc(ptr %values_ptr, i64 %new_values_size)
  
  ; Update cluster structure
  %resized_cluster = insertvalue %atlas.resonance.cluster %cluster, ptr %new_indices, 2
  %resized_cluster2 = insertvalue %atlas.resonance.cluster %resized_cluster, ptr %new_values, 3
  store %atlas.resonance.cluster %resized_cluster2, ptr %cluster_ptr, align 8
  br label %add_element
  
add_element:
  %final_cluster = load %atlas.resonance.cluster, ptr %cluster_ptr, align 8
  %final_indices = extractvalue %atlas.resonance.cluster %final_cluster, 2
  %final_values = extractvalue %atlas.resonance.cluster %final_cluster, 3
  
  ; Add new element at current size position
  %idx_ptr = getelementptr i32, ptr %final_indices, i32 %current_size
  %val_ptr = getelementptr i8, ptr %final_values, i32 %current_size
  store i32 %index, ptr %idx_ptr, align 4
  store i8 %value, ptr %val_ptr, align 1
  
  ; Update size
  %new_size = add i32 %current_size, 1
  %updated_cluster = insertvalue %atlas.resonance.cluster %final_cluster, i32 %new_size, 1
  store %atlas.resonance.cluster %updated_cluster, ptr %cluster_ptr, align 8
  ret void
}

; Build clusters for all resonance classes from data
define ptr @atlas.clustering.build_all_clusters(ptr %data, i32 %data_size) nounwind {
entry:
  ; Allocate array of 96 cluster pointers
  %clusters_array = call ptr @calloc(i64 96, i64 8)  ; 96 * sizeof(ptr) = 96 * 8
  
  ; Create initial clusters for each resonance class
  br label %init_clusters_loop
  
init_clusters_loop:
  %r = phi i32 [ 0, %entry ], [ %next_r, %init_clusters_body ]
  %init_done = icmp uge i32 %r, 96
  br i1 %init_done, label %scan_data, label %init_clusters_body
  
init_clusters_body:
  %r_i7 = trunc i32 %r to i7
  %cluster = call ptr @atlas.clustering.create_cluster(i7 %r_i7, i32 256)  ; Initial capacity 256
  %cluster_slot = getelementptr ptr, ptr %clusters_array, i32 %r
  store ptr %cluster, ptr %cluster_slot, align 8
  %next_r = add i32 %r, 1
  br label %init_clusters_loop
  
scan_data:
  ; Scan through data and assign each element to appropriate cluster
  br label %data_loop
  
data_loop:
  %i = phi i32 [ 0, %scan_data ], [ %next_i, %data_body ]
  %data_done = icmp uge i32 %i, %data_size
  br i1 %data_done, label %finish, label %data_body
  
data_body:
  ; Load and classify byte
  %data_ptr = getelementptr i8, ptr %data, i32 %i
  %byte = load i8, ptr %data_ptr, align 1
  %resonance = call i7 @atlas.r96.classify(i8 %byte)
  
  ; Get appropriate cluster
  %res_i32 = zext i7 %resonance to i32
  %cluster_ptr_addr = getelementptr ptr, ptr %clusters_array, i32 %res_i32
  %cluster_ptr = load ptr, ptr %cluster_ptr_addr, align 8
  
  ; Add element to cluster (using fixed capacity for simplicity)
  call void @atlas.clustering.add_to_cluster(ptr %cluster_ptr, i32 %i, i8 %byte, i32 4096)
  
  %next_i = add i32 %i, 1
  br label %data_loop
  
finish:
  ret ptr %clusters_array
}

; =============================================================================
; Optimized histogram generation for clustering
; =============================================================================

; Generate clustering histogram using SIMD optimization
define void @atlas.clustering.histogram_simd(ptr %data, i32 %data_size, ptr %histogram) nounwind {
entry:
  ; Clear histogram (96 x i32)
  call void @llvm.memset.p0.i64(ptr %histogram, i8 0, i64 384, i1 false)
  
  ; Process data in 16-byte chunks using SIMD
  %chunks = udiv i32 %data_size, 16
  %remainder = urem i32 %data_size, 16
  
  br label %simd_loop
  
simd_loop:
  %chunk_idx = phi i32 [ 0, %entry ], [ %next_chunk, %continue_simd ]
  %simd_done = icmp uge i32 %chunk_idx, %chunks
  br i1 %simd_done, label %remainder_loop, label %simd_body
  
simd_body:
  ; Load 16 bytes and classify with SIMD
  %chunk_offset = mul i32 %chunk_idx, 16
  %chunk_ptr = getelementptr i8, ptr %data, i32 %chunk_offset
  %chunk_ptr_v16 = bitcast ptr %chunk_ptr to ptr
  %bytes_vec = load <16 x i8>, ptr %chunk_ptr_v16, align 1
  
  ; Classify using SIMD
  %classes_vec = call <16 x i7> @atlas.r96.classify.sse2(<16 x i8> %bytes_vec)
  
  ; Extract and accumulate into histogram
  br label %extract_loop
  
extract_loop:
  %extract_idx = phi i32 [ 0, %simd_body ], [ %next_extract, %extract_body ]
  %extract_done = icmp uge i32 %extract_idx, 16
  br i1 %extract_done, label %continue_simd, label %extract_body
  
extract_body:
  %class_i7 = extractelement <16 x i7> %classes_vec, i32 %extract_idx
  %class_i32 = zext i7 %class_i7 to i32
  
  ; Increment histogram bin
  %hist_ptr = getelementptr [96 x i32], ptr %histogram, i32 0, i32 %class_i32
  %old_count = load i32, ptr %hist_ptr, align 4
  %new_count = add i32 %old_count, 1
  store i32 %new_count, ptr %hist_ptr, align 4
  
  %next_extract = add i32 %extract_idx, 1
  br label %extract_loop
  
continue_simd:
  %next_chunk = add i32 %chunk_idx, 1
  br label %simd_loop
  
remainder_loop:
  ; Process remaining bytes individually
  %rem_idx = phi i32 [ 0, %simd_loop ], [ %next_rem, %remainder_body ]
  %rem_done = icmp uge i32 %rem_idx, %remainder
  br i1 %rem_done, label %exit, label %remainder_body
  
remainder_body:
  %chunks_processed = mul i32 %chunks, 16
  %rem_offset = add i32 %chunks_processed, %rem_idx
  %rem_ptr = getelementptr i8, ptr %data, i32 %rem_offset
  %rem_byte = load i8, ptr %rem_ptr, align 1
  %rem_class = call i7 @atlas.r96.classify(i8 %rem_byte)
  %rem_class_i32 = zext i7 %rem_class to i32
  
  ; Increment histogram bin
  %rem_hist_ptr = getelementptr [96 x i32], ptr %histogram, i32 0, i32 %rem_class_i32
  %rem_old_count = load i32, ptr %rem_hist_ptr, align 4
  %rem_new_count = add i32 %rem_old_count, 1
  store i32 %rem_new_count, ptr %rem_hist_ptr, align 4
  
  %next_rem = add i32 %rem_idx, 1
  br label %remainder_loop
  
exit:
  ret void
}

; Generate multi-dimensional clustering statistics
define void @atlas.clustering.compute_stats(ptr %clusters_array, ptr %stats_array) nounwind readonly {
entry:
  br label %stats_loop
  
stats_loop:
  %class_idx = phi i32 [ 0, %entry ], [ %next_class, %stats_body ]
  %stats_done = icmp uge i32 %class_idx, 96
  br i1 %stats_done, label %finish_stats, label %stats_body
  
stats_body:
  ; Load cluster
  %cluster_ptr_addr = getelementptr ptr, ptr %clusters_array, i32 %class_idx
  %cluster_ptr = load ptr, ptr %cluster_ptr_addr, align 8
  %cluster = load %atlas.resonance.cluster, ptr %cluster_ptr, align 8
  %size = extractvalue %atlas.resonance.cluster %cluster, 1
  
  ; Calculate density (size / max_size)
  %max_size = load i32, ptr @atlas.clustering.max_cluster_size, align 4
  %size_f64 = uitofp i32 %size to double
  %max_size_f64 = uitofp i32 %max_size to double
  %density = fdiv double %size_f64, %max_size_f64
  
  ; Simple affinity score based on cluster size and density
  %density_threshold = load double, ptr @atlas.clustering.density_threshold, align 8
  %density_ratio = fdiv double %density, %density_threshold
  %affinity_score = call double @llvm.fmin.f64(double %density_ratio, double 1.0)
  
  ; Create and store stats
  %stats = insertvalue %atlas.cluster.stats undef, i32 %size, 0
  %stats2 = insertvalue %atlas.cluster.stats %stats, i32 %size, 1  ; Use size as density measure
  %stats3 = insertvalue %atlas.cluster.stats %stats2, double %affinity_score, 2
  
  %stats_ptr = getelementptr %atlas.cluster.stats, ptr %stats_array, i32 %class_idx
  store %atlas.cluster.stats %stats3, ptr %stats_ptr, align 8
  
  %next_class = add i32 %class_idx, 1
  br label %stats_loop
  
finish_stats:
  ret void
}

; =============================================================================
; Batch operations for clustering
; =============================================================================

; Batch process multiple pages for clustering
define void @atlas.clustering.batch_process_pages(ptr %pages_array, i32 %num_pages, ptr %results_array) nounwind {
entry:
  br label %page_loop, !llvm.loop !1
  
page_loop:
  %page_idx = phi i32 [ 0, %entry ], [ %next_page, %page_body ]
  %page_done = icmp uge i32 %page_idx, %num_pages
  br i1 %page_done, label %batch_complete, label %page_body
  
page_body:
  ; Get page pointer
  %page_ptr_addr = getelementptr ptr, ptr %pages_array, i32 %page_idx
  %page_ptr = load ptr, ptr %page_ptr_addr, align 8
  
  ; Generate histogram for this page
  %histogram = alloca [96 x i32], align 16
  call void @atlas.clustering.histogram_simd(ptr %page_ptr, i32 256, ptr %histogram)
  
  ; Store result
  %result_ptr = getelementptr [96 x i32], ptr %results_array, i32 %page_idx
  call void @llvm.memcpy.p0.p0.i64(ptr %result_ptr, ptr %histogram, i64 384, i1 false)
  
  %next_page = add i32 %page_idx, 1
  br label %page_loop, !llvm.loop !1
  
batch_complete:
  ret void
}

; Batch merge clusters for improved locality
define void @atlas.clustering.batch_merge_clusters(ptr %clusters_array, ptr %merge_indices, i32 %merge_count) nounwind {
entry:
  br label %merge_loop
  
merge_loop:
  %merge_idx = phi i32 [ 0, %entry ], [ %next_merge, %merge_body ]
  %merge_done = icmp uge i32 %merge_idx, %merge_count
  br i1 %merge_done, label %merge_complete, label %merge_body
  
merge_body:
  ; Get source and target cluster indices (packed as i64: high32=target, low32=source)
  %merge_pair_ptr = getelementptr i64, ptr %merge_indices, i32 %merge_idx
  %merge_pair = load i64, ptr %merge_pair_ptr, align 8
  %source_idx = trunc i64 %merge_pair to i32
  %target_idx_64 = lshr i64 %merge_pair, 32
  %target_idx = trunc i64 %target_idx_64 to i32
  
  ; Get source and target cluster pointers
  %source_cluster_addr = getelementptr ptr, ptr %clusters_array, i32 %source_idx
  %target_cluster_addr = getelementptr ptr, ptr %clusters_array, i32 %target_idx
  %source_cluster_ptr = load ptr, ptr %source_cluster_addr, align 8
  %target_cluster_ptr = load ptr, ptr %target_cluster_addr, align 8
  
  ; Load cluster data
  %source_cluster = load %atlas.resonance.cluster, ptr %source_cluster_ptr, align 8
  %target_cluster = load %atlas.resonance.cluster, ptr %target_cluster_ptr, align 8
  
  %source_size = extractvalue %atlas.resonance.cluster %source_cluster, 1
  %target_size = extractvalue %atlas.resonance.cluster %target_cluster, 1
  %source_indices = extractvalue %atlas.resonance.cluster %source_cluster, 2
  %source_values = extractvalue %atlas.resonance.cluster %source_cluster, 3
  %target_indices = extractvalue %atlas.resonance.cluster %target_cluster, 2
  %target_values = extractvalue %atlas.resonance.cluster %target_cluster, 3
  
  ; Reallocate target arrays to accommodate merged data
  %new_size = add i32 %source_size, %target_size
  %new_size_64 = zext i32 %new_size to i64
  %new_indices_size = mul i64 %new_size_64, 4
  %new_values_size = mul i64 %new_size_64, 1
  
  %merged_indices = call ptr @realloc(ptr %target_indices, i64 %new_indices_size)
  %merged_values = call ptr @realloc(ptr %target_values, i64 %new_values_size)
  
  ; Copy source data to end of target arrays
  %target_size_64 = zext i32 %target_size to i64
  %indices_copy_offset = mul i64 %target_size_64, 4
  %values_copy_offset = mul i64 %target_size_64, 1
  %source_size_64 = zext i32 %source_size to i64
  %indices_copy_size = mul i64 %source_size_64, 4
  %values_copy_size = mul i64 %source_size_64, 1
  
  %dest_indices_ptr = getelementptr i8, ptr %merged_indices, i64 %indices_copy_offset
  %dest_values_ptr = getelementptr i8, ptr %merged_values, i64 %values_copy_offset
  
  call void @llvm.memcpy.p0.p0.i64(ptr %dest_indices_ptr, ptr %source_indices, i64 %indices_copy_size, i1 false)
  call void @llvm.memcpy.p0.p0.i64(ptr %dest_values_ptr, ptr %source_values, i64 %values_copy_size, i1 false)
  
  ; Update target cluster
  %merged_cluster = insertvalue %atlas.resonance.cluster %target_cluster, i32 %new_size, 1
  %merged_cluster2 = insertvalue %atlas.resonance.cluster %merged_cluster, ptr %merged_indices, 2
  %merged_cluster3 = insertvalue %atlas.resonance.cluster %merged_cluster2, ptr %merged_values, 3
  store %atlas.resonance.cluster %merged_cluster3, ptr %target_cluster_ptr, align 8
  
  ; Free source cluster
  call void @free(ptr %source_indices)
  call void @free(ptr %source_values)
  call void @free(ptr %source_cluster_ptr)
  store ptr null, ptr %source_cluster_addr, align 8
  
  %next_merge = add i32 %merge_idx, 1
  br label %merge_loop
  
merge_complete:
  ret void
}

; =============================================================================
; Cleanup functions
; =============================================================================

; Destroy CSR matrix and free memory
define void @atlas.csr.destroy(ptr %matrix_ptr) nounwind {
entry:
  %matrix = load %atlas.csr.matrix, ptr %matrix_ptr, align 8
  %values = extractvalue %atlas.csr.matrix %matrix, 2
  %col_indices = extractvalue %atlas.csr.matrix %matrix, 3
  %row_pointers = extractvalue %atlas.csr.matrix %matrix, 4
  
  ; Free arrays (check for null first)
  %values_null = icmp eq ptr %values, null
  br i1 %values_null, label %check_col_indices, label %free_values
  
free_values:
  call void @free(ptr %values)
  br label %check_col_indices
  
check_col_indices:
  %col_indices_null = icmp eq ptr %col_indices, null
  br i1 %col_indices_null, label %free_row_pointers, label %free_col_indices
  
free_col_indices:
  call void @free(ptr %col_indices)
  br label %free_row_pointers
  
free_row_pointers:
  call void @free(ptr %row_pointers)
  call void @free(ptr %matrix_ptr)
  ret void
}

; Destroy resonance cluster
define void @atlas.clustering.destroy_cluster(ptr %cluster_ptr) nounwind {
entry:
  %cluster = load %atlas.resonance.cluster, ptr %cluster_ptr, align 8
  %indices = extractvalue %atlas.resonance.cluster %cluster, 2
  %values = extractvalue %atlas.resonance.cluster %cluster, 3
  
  call void @free(ptr %indices)
  call void @free(ptr %values)
  call void @free(ptr %cluster_ptr)
  ret void
}

; Destroy all clusters in array
define void @atlas.clustering.destroy_all_clusters(ptr %clusters_array) nounwind {
entry:
  br label %destroy_loop
  
destroy_loop:
  %i = phi i32 [ 0, %entry ], [ %next_i, %continue_destroy ]
  %done = icmp uge i32 %i, 96
  br i1 %done, label %free_array, label %destroy_body
  
destroy_body:
  %cluster_ptr_addr = getelementptr ptr, ptr %clusters_array, i32 %i
  %cluster_ptr = load ptr, ptr %cluster_ptr_addr, align 8
  %is_null = icmp eq ptr %cluster_ptr, null
  br i1 %is_null, label %continue_destroy, label %destroy_cluster
  
destroy_cluster:
  call void @atlas.clustering.destroy_cluster(ptr %cluster_ptr)
  br label %continue_destroy
  
continue_destroy:
  %next_i = add i32 %i, 1
  br label %destroy_loop
  
free_array:
  call void @free(ptr %clusters_array)
  ret void
}

; =============================================================================
; LLVM intrinsic declarations
; =============================================================================

declare double @llvm.fmin.f64(double, double) nounwind readnone

; =============================================================================
; Loop optimization metadata
; =============================================================================

!0 = !{}  ; for invariant loads
!1 = !{!1, !2, !3}
!2 = !{!"llvm.loop.vectorize.enable", i1 true}
!3 = !{!"llvm.loop.vectorize.width", i32 16}

; =============================================================================
; Exported symbols for linking
; =============================================================================

@llvm.used = appending global [15 x ptr] [
  ptr @atlas.csr.create,
  ptr @atlas.csr.build_from_resonance,
  ptr @atlas.clustering.create_cluster,
  ptr @atlas.clustering.add_to_cluster,
  ptr @atlas.clustering.build_all_clusters,
  ptr @atlas.clustering.histogram_simd,
  ptr @atlas.clustering.compute_stats,
  ptr @atlas.clustering.batch_process_pages,
  ptr @atlas.clustering.batch_merge_clusters,
  ptr @atlas.csr.destroy,
  ptr @atlas.clustering.destroy_cluster,
  ptr @atlas.clustering.destroy_all_clusters,
  ptr @atlas.clustering.histogram_simd,
  ptr @atlas.clustering.compute_stats,
  ptr @atlas.clustering.batch_process_pages
], section "llvm.metadata"

; ---------------------------------------------------------------------------------
; End of atlas-12288-clustering.ll
; ---------------------------------------------------------------------------------