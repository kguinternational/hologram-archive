; atlas-12288-validation.ll — Extended Validation for Atlas-12288 (LLVM 15+, opaque pointers)
; ---------------------------------------------------------------------------------
; This module implements comprehensive validation routines including:
;  • Page validation (structure, alignment, entropy analysis)
;  • Structure validation (complete integrity checks, anomaly detection, repair)
;  • Cross-layer validation (boundary conservation, resonance distribution, witness integrity)
;
; Dependencies:
;  • atlas-12288-r96.ll (resonance classification)
;  • atlas-12288-c768.ll (C768 cyclic group operations) 
;  • atlas-12288-intrinsics.ll (core intrinsics)
;  • atlas-12288-harmonic.ll (harmonic operations)
; ---------------------------------------------------------------------------------

source_filename = "atlas-12288-validation.ll"

; =============================================================================
; Type definitions and validation structures
; =============================================================================

%atlas.validation.result = type { i1, i32, ptr }    ; success, error_code, message_ptr
%atlas.validation.page_info = type { i1, i1, double, i32 }  ; aligned, structured, entropy, anomalies
%atlas.validation.structure_info = type { i1, i1, i32, i32, i32 }  ; complete, conserved, pages, errors, repairs
%atlas.validation.cross_layer_info = type { i1, i1, i1, i32 }  ; boundaries_ok, resonance_ok, witness_ok, violations

; Import type definitions from other modules
%atlas.harmonic.pair = type { i7, i7 }  ; resonance pair (a, b) where a + b ≡ 0 (mod 96)

; Validation constants and thresholds
@atlas.validation.min_entropy = internal constant double 0.5, align 8
@atlas.validation.max_entropy = internal constant double 0.95, align 8
@atlas.validation.page_size = internal constant i32 256, align 4
@atlas.validation.structure_pages = internal constant i32 48, align 4
@atlas.validation.total_size = internal constant i32 12288, align 4

; Error codes
@atlas.validation.error.none = internal constant i32 0, align 4
@atlas.validation.error.alignment = internal constant i32 1001, align 4
@atlas.validation.error.entropy = internal constant i32 1002, align 4
@atlas.validation.error.structure = internal constant i32 1003, align 4
@atlas.validation.error.conservation = internal constant i32 1004, align 4
@atlas.validation.error.boundary = internal constant i32 1005, align 4
@atlas.validation.error.resonance = internal constant i32 1006, align 4
@atlas.validation.error.witness = internal constant i32 1007, align 4
@atlas.validation.error.anomaly = internal constant i32 1008, align 4

; Error message strings
@atlas.validation.msg.success = private unnamed_addr constant [18 x i8] c"Validation passed\00", align 1
@atlas.validation.msg.alignment = private unnamed_addr constant [25 x i8] c"Page alignment violation\00", align 1
@atlas.validation.msg.entropy = private unnamed_addr constant [21 x i8] c"Entropy out of range\00", align 1
@atlas.validation.msg.structure = private unnamed_addr constant [25 x i8] c"Structure integrity fail\00", align 1
@atlas.validation.msg.conservation = private unnamed_addr constant [22 x i8] c"Conservation violated\00", align 1
@atlas.validation.msg.boundary = private unnamed_addr constant [22 x i8] c"Boundary check failed\00", align 1
@atlas.validation.msg.resonance = private unnamed_addr constant [29 x i8] c"Resonance distribution error\00", align 1
@atlas.validation.msg.witness = private unnamed_addr constant [23 x i8] c"Witness integrity fail\00", align 1
@atlas.validation.msg.anomaly = private unnamed_addr constant [19 x i8] c"Anomalies detected\00", align 1

; =============================================================================
; External function declarations
; =============================================================================

; From atlas-12288-r96.ll
declare i7 @atlas.r96.classify(i8) nounwind readnone willreturn
declare void @atlas.r96.histogram(ptr, ptr) nounwind
declare i7 @atlas.r96.dominant(ptr) nounwind

; From atlas-12288-c768.ll
declare i16 @atlas.c768.normalize(i16) nounwind readnone willreturn
declare i1 @atlas.c768.aligned(i16) nounwind readnone willreturn

; From atlas-12288-intrinsics.ll
declare i1 @atlas.conserved.check(ptr, i64) nounwind readonly willreturn
declare { i16, i8 } @atlas.boundary.decode(i32) nounwind readnone willreturn
declare i32 @atlas.boundary.encode(i16, i8) nounwind readnone willreturn
declare ptr @atlas.witness.generate(ptr, i64) nounwind
declare i1 @atlas.witness.verify(ptr, ptr, i64) nounwind readonly

; From atlas-12288-harmonic.ll
declare %atlas.harmonic.pair @atlas.harmonic.find_pair(i7) nounwind readnone
declare i1 @atlas.harmonic.validate_pair(%atlas.harmonic.pair) nounwind readnone

; Standard library and LLVM intrinsics
declare ptr @malloc(i64)
declare void @free(ptr)
declare void @llvm.memset.p0.i64(ptr nocapture, i8, i64, i1)
declare double @llvm.log2.f64(double) nounwind readnone

; =============================================================================
; Page validation functions
; =============================================================================

; Calculate Shannon entropy for a page
define double @atlas.validation.calculate_entropy(ptr %page) nounwind {
entry:
  ; Allocate histogram for byte frequency analysis
  %histogram = alloca [256 x i32], align 16
  call void @llvm.memset.p0.i64(ptr %histogram, i8 0, i64 1024, i1 false)
  
  ; Count byte frequencies
  br label %count_loop
  
count_loop:
  %i = phi i32 [ 0, %entry ], [ %next, %count_body ]
  %done = icmp uge i32 %i, 256
  br i1 %done, label %entropy_calc, label %count_body
  
count_body:
  %byte_ptr = getelementptr i8, ptr %page, i32 %i
  %byte = load i8, ptr %byte_ptr, align 1
  %byte_idx = zext i8 %byte to i32
  %hist_ptr = getelementptr [256 x i32], ptr %histogram, i32 0, i32 %byte_idx
  %old_count = load i32, ptr %hist_ptr, align 4
  %new_count = add i32 %old_count, 1
  store i32 %new_count, ptr %hist_ptr, align 4
  %next = add i32 %i, 1
  br label %count_loop
  
entropy_calc:
  ; Calculate Shannon entropy: H = -Σ(p_i * log2(p_i))
  %entropy = alloca double, align 8
  store double 0.0, ptr %entropy, align 8
  br label %entropy_loop
  
entropy_loop:
  %j = phi i32 [ 0, %entropy_calc ], [ %next_j, %entropy_continue ]
  %entropy_done = icmp uge i32 %j, 256
  br i1 %entropy_done, label %entropy_exit, label %entropy_body
  
entropy_body:
  %hist_j_ptr = getelementptr [256 x i32], ptr %histogram, i32 0, i32 %j
  %count_j = load i32, ptr %hist_j_ptr, align 4
  %has_count = icmp ugt i32 %count_j, 0
  br i1 %has_count, label %entropy_calc_j, label %entropy_continue
  
entropy_calc_j:
  ; Calculate probability p_i = count_i / 256
  %count_double = uitofp i32 %count_j to double
  %prob = fdiv double %count_double, 256.0
  
  ; Calculate log2(p_i)
  %log_prob = call double @llvm.log2.f64(double %prob)
  
  ; Add -p_i * log2(p_i) to entropy
  %neg_prob = fsub double 0.0, %prob
  %term = fmul double %neg_prob, %log_prob
  %current_entropy = load double, ptr %entropy, align 8
  %new_entropy = fadd double %current_entropy, %term
  store double %new_entropy, ptr %entropy, align 8
  br label %entropy_continue
  
entropy_continue:
  %next_j = add i32 %j, 1
  br label %entropy_loop
  
entropy_exit:
  %final_entropy = load double, ptr %entropy, align 8
  ret double %final_entropy
}

; Check if page has proper structure and alignment
define i1 @atlas.validation.check_page_structure(ptr %page) nounwind {
entry:
  ; Check memory alignment (should be aligned to at least 16 bytes)
  %page_int = ptrtoint ptr %page to i64
  %alignment_mask = and i64 %page_int, 15
  %aligned = icmp eq i64 %alignment_mask, 0
  
  ; Check if page contains valid resonance distribution
  %histogram = alloca [96 x i32], align 16
  call void @atlas.r96.histogram(ptr %page, ptr %histogram)
  
  ; Verify that all resonance classes are represented reasonably
  %min_expected = udiv i32 256, 96  ; At least 2-3 per class on average
  br label %check_loop
  
check_loop:
  %i = phi i32 [ 0, %entry ], [ %next, %check_body ]
  %valid = phi i1 [ %aligned, %entry ], [ %valid_new, %check_body ]
  %done = icmp uge i32 %i, 96
  br i1 %done, label %exit, label %check_body
  
check_body:
  %hist_ptr = getelementptr [96 x i32], ptr %histogram, i32 0, i32 %i
  %count = load i32, ptr %hist_ptr, align 4
  %reasonable = icmp uge i32 %count, 1  ; At least one occurrence
  %valid_new = and i1 %valid, %reasonable
  %next = add i32 %i, 1
  br label %check_loop
  
exit:
  %final_valid = phi i1 [ %valid, %check_loop ]
  ret i1 %final_valid
}

; Comprehensive page validation
define %atlas.validation.page_info @atlas.validation.validate_page(ptr %page) nounwind {
entry:
  ; Check alignment and structure
  %aligned = call i1 @atlas.validation.check_page_structure(ptr %page)
  
  ; Calculate entropy
  %entropy = call double @atlas.validation.calculate_entropy(ptr %page)
  %min_entropy = load double, ptr @atlas.validation.min_entropy, align 8
  %max_entropy = load double, ptr @atlas.validation.max_entropy, align 8
  %entropy_ok_min = fcmp oge double %entropy, %min_entropy
  %entropy_ok_max = fcmp ole double %entropy, %max_entropy
  %entropy_ok = and i1 %entropy_ok_min, %entropy_ok_max
  %structured = and i1 %aligned, %entropy_ok
  
  ; Count anomalies (bytes that are statistical outliers)
  %anomalies = call i32 @atlas.validation.count_anomalies(ptr %page)
  
  ; Construct page info
  %info = insertvalue %atlas.validation.page_info undef, i1 %aligned, 0
  %info2 = insertvalue %atlas.validation.page_info %info, i1 %structured, 1
  %info3 = insertvalue %atlas.validation.page_info %info2, double %entropy, 2
  %info4 = insertvalue %atlas.validation.page_info %info3, i32 %anomalies, 3
  
  ret %atlas.validation.page_info %info4
}

; Count statistical anomalies in a page
define i32 @atlas.validation.count_anomalies(ptr %page) nounwind {
entry:
  ; Build resonance histogram
  %histogram = alloca [96 x i32], align 16
  call void @atlas.r96.histogram(ptr %page, ptr %histogram)
  
  ; Calculate mean and standard deviation
  %mean = alloca double, align 8
  %variance = alloca double, align 8
  store double 0.0, ptr %mean, align 8
  store double 0.0, ptr %variance, align 8
  
  ; Calculate mean
  br label %mean_loop
  
mean_loop:
  %i = phi i32 [ 0, %entry ], [ %next, %mean_body ]
  %sum = phi double [ 0.0, %entry ], [ %new_sum, %mean_body ]
  %mean_done = icmp uge i32 %i, 96
  br i1 %mean_done, label %calc_mean, label %mean_body
  
mean_body:
  %hist_ptr = getelementptr [96 x i32], ptr %histogram, i32 0, i32 %i
  %count = load i32, ptr %hist_ptr, align 4
  %count_double = uitofp i32 %count to double
  %new_sum = fadd double %sum, %count_double
  %next = add i32 %i, 1
  br label %mean_loop
  
calc_mean:
  %total_sum = phi double [ %sum, %mean_loop ]
  %mean_val = fdiv double %total_sum, 96.0
  store double %mean_val, ptr %mean, align 8
  
  ; Calculate variance
  br label %var_loop
  
var_loop:
  %j = phi i32 [ 0, %calc_mean ], [ %next_j, %var_body ]
  %var_sum = phi double [ 0.0, %calc_mean ], [ %new_var_sum, %var_body ]
  %var_done = icmp uge i32 %j, 96
  br i1 %var_done, label %count_outliers, label %var_body
  
var_body:
  %hist_j_ptr = getelementptr [96 x i32], ptr %histogram, i32 0, i32 %j
  %count_j = load i32, ptr %hist_j_ptr, align 4
  %count_j_double = uitofp i32 %count_j to double
  %diff = fsub double %count_j_double, %mean_val
  %diff_sq = fmul double %diff, %diff
  %new_var_sum = fadd double %var_sum, %diff_sq
  %next_j = add i32 %j, 1
  br label %var_loop
  
count_outliers:
  %var_total = phi double [ %var_sum, %var_loop ]
  %variance_val = fdiv double %var_total, 96.0
  store double %variance_val, ptr %variance, align 8
  %std_dev = call double @llvm.sqrt.f64(double %variance_val)
  
  ; Count values more than 2 standard deviations from mean
  %threshold = fmul double %std_dev, 2.0
  br label %outlier_loop
  
outlier_loop:
  %k = phi i32 [ 0, %count_outliers ], [ %next_k, %outlier_body ]
  %anomaly_count = phi i32 [ 0, %count_outliers ], [ %new_anomaly_count, %outlier_body ]
  %outlier_done = icmp uge i32 %k, 96
  br i1 %outlier_done, label %exit, label %outlier_body
  
outlier_body:
  %hist_k_ptr = getelementptr [96 x i32], ptr %histogram, i32 0, i32 %k
  %count_k = load i32, ptr %hist_k_ptr, align 4
  %count_k_double = uitofp i32 %count_k to double
  %deviation = fsub double %count_k_double, %mean_val
  %abs_deviation = call double @llvm.fabs.f64(double %deviation)
  %is_outlier = fcmp ogt double %abs_deviation, %threshold
  %anomaly_increment = select i1 %is_outlier, i32 1, i32 0
  %new_anomaly_count = add i32 %anomaly_count, %anomaly_increment
  %next_k = add i32 %k, 1
  br label %outlier_loop
  
exit:
  %final_count = phi i32 [ %anomaly_count, %outlier_loop ]
  ret i32 %final_count
}

; =============================================================================
; Structure validation functions  
; =============================================================================

; Validate complete Atlas-12288 structure
define %atlas.validation.structure_info @atlas.validation.validate_structure(ptr %structure) nounwind {
entry:
  %page_errors = alloca i32, align 4
  %repairs_made = alloca i32, align 4
  store i32 0, ptr %page_errors, align 4
  store i32 0, ptr %repairs_made, align 4
  
  ; Check conservation for entire structure
  %conserved = call i1 @atlas.conserved.check(ptr %structure, i64 12288)
  
  ; Validate each page
  br label %page_loop
  
page_loop:
  %page_idx = phi i32 [ 0, %entry ], [ %next_page, %next_iteration ]
  %pages_valid = phi i1 [ true, %entry ], [ %pages_valid_new, %next_iteration ]
  %page_done = icmp uge i32 %page_idx, 48
  br i1 %page_done, label %finalize, label %page_body
  
page_body:
  ; Get page pointer
  %page_offset = mul i32 %page_idx, 256
  %page_ptr = getelementptr i8, ptr %structure, i32 %page_offset
  
  ; Validate page
  %page_info = call %atlas.validation.page_info @atlas.validation.validate_page(ptr %page_ptr)
  %page_valid = extractvalue %atlas.validation.page_info %page_info, 1
  %pages_valid_new = and i1 %pages_valid, %page_valid
  
  ; Count errors and attempt repairs if needed
  %page_aligned = extractvalue %atlas.validation.page_info %page_info, 0
  br i1 %page_aligned, label %next_iteration, label %repair_page
  
repair_page:
  %current_errors = load i32, ptr %page_errors, align 4
  %new_errors = add i32 %current_errors, 1
  store i32 %new_errors, ptr %page_errors, align 4
  
  ; Attempt simple repair by normalizing page data
  %repaired = call i1 @atlas.validation.repair_page(ptr %page_ptr)
  br i1 %repaired, label %count_repair, label %next_iteration
  
count_repair:
  %current_repairs = load i32, ptr %repairs_made, align 4
  %new_repairs = add i32 %current_repairs, 1
  store i32 %new_repairs, ptr %repairs_made, align 4
  br label %next_iteration
  
next_iteration:
  %next_page = add i32 %page_idx, 1
  br label %page_loop
  
finalize:
  %final_pages_valid = phi i1 [ %pages_valid, %page_loop ]
  %complete = and i1 %final_pages_valid, %conserved
  %final_errors = load i32, ptr %page_errors, align 4
  %final_repairs = load i32, ptr %repairs_made, align 4
  
  ; Construct structure info
  %info = insertvalue %atlas.validation.structure_info undef, i1 %complete, 0
  %info2 = insertvalue %atlas.validation.structure_info %info, i1 %conserved, 1
  %info3 = insertvalue %atlas.validation.structure_info %info2, i32 48, 2
  %info4 = insertvalue %atlas.validation.structure_info %info3, i32 %final_errors, 3
  %info5 = insertvalue %atlas.validation.structure_info %info4, i32 %final_repairs, 4
  
  ret %atlas.validation.structure_info %info5
}

; Attempt to repair a damaged page
define i1 @atlas.validation.repair_page(ptr %page) nounwind {
entry:
  ; Simple repair: normalize resonance distribution
  %histogram = alloca [96 x i32], align 16
  call void @atlas.r96.histogram(ptr %page, ptr %histogram)
  
  ; Find dominant resonance
  %dominant = call i7 @atlas.r96.dominant(ptr %page)
  %dom_32 = zext i7 %dominant to i32
  
  ; Check if distribution is too skewed (dominant > 50% of page)
  %dom_ptr = getelementptr [96 x i32], ptr %histogram, i32 0, i32 %dom_32
  %dom_count = load i32, ptr %dom_ptr, align 4
  %is_skewed = icmp ugt i32 %dom_count, 128  ; More than half the page
  
  br i1 %is_skewed, label %redistribute, label %no_repair_needed
  
redistribute:
  ; Redistribute some dominant bytes to maintain conservation
  %excess = sub i32 %dom_count, 64  ; Target around 64 for dominant
  %redistribute_per_class = udiv i32 %excess, 95  ; Distribute to other classes
  
  br label %redistribute_loop
  
redistribute_loop:
  %i = phi i32 [ 0, %redistribute ], [ %next, %continue_loop ]
  %bytes_moved = phi i32 [ 0, %redistribute ], [ %final_bytes_moved, %continue_loop ]
  %redist_done = icmp uge i32 %i, 256
  %enough_moved = icmp uge i32 %bytes_moved, %excess
  %should_continue = and i1 %redist_done, %enough_moved
  %continue_redist = xor i1 %should_continue, true
  br i1 %continue_redist, label %redistribute_body, label %repair_success
  
redistribute_body:
  %byte_ptr = getelementptr i8, ptr %page, i32 %i
  %byte = load i8, ptr %byte_ptr, align 1
  %byte_class = call i7 @atlas.r96.classify(i8 %byte)
  %is_dominant = icmp eq i7 %byte_class, %dominant
  
  br i1 %is_dominant, label %maybe_change, label %skip_byte
  
maybe_change:
  ; Change some dominant bytes to less frequent classes
  %change_decision = urem i32 %i, 4  ; Change every 4th dominant byte
  %should_change = icmp eq i32 %change_decision, 0
  br i1 %should_change, label %change_byte, label %skip_byte
  
change_byte:
  ; Find a less frequent target class
  %target_class = urem i32 %i, 96
  %target_class_i7 = trunc i32 %target_class to i7
  %target_byte = trunc i32 %target_class to i8
  store i8 %target_byte, ptr %byte_ptr, align 1
  %new_bytes_moved = add i32 %bytes_moved, 1
  br label %continue_loop
  
skip_byte:
  %new_bytes_moved_skip = phi i32 [ %bytes_moved, %maybe_change ], [ %bytes_moved, %redistribute_body ]
  br label %continue_loop
  
continue_loop:
  %final_bytes_moved = phi i32 [ %new_bytes_moved, %change_byte ], [ %new_bytes_moved_skip, %skip_byte ]
  %next = add i32 %i, 1
  br label %redistribute_loop
  
repair_success:
  ret i1 true
  
no_repair_needed:
  ret i1 false
}

; =============================================================================
; Cross-layer validation functions
; =============================================================================

; Validate boundary conservation across structure
define i1 @atlas.validation.validate_boundaries(ptr %structure) nounwind {
entry:
  ; Check boundaries between pages
  br label %boundary_loop
  
boundary_loop:
  %page_idx = phi i32 [ 0, %entry ], [ %next_page, %boundary_body ]
  %boundaries_ok = phi i1 [ true, %entry ], [ %boundaries_ok_new, %boundary_body ]
  %last_page = sub i32 48, 1
  %boundary_done = icmp uge i32 %page_idx, %last_page
  br i1 %boundary_done, label %exit, label %boundary_body
  
boundary_body:
  ; Get current and next page pointers
  %page_offset = mul i32 %page_idx, 256
  %next_page_idx = add i32 %page_idx, 1
  %next_page_offset = mul i32 %next_page_idx, 256
  
  %curr_page_ptr = getelementptr i8, ptr %structure, i32 %page_offset
  %next_page_ptr = getelementptr i8, ptr %structure, i32 %next_page_offset
  
  ; Check boundary condition: last byte of current page + first byte of next page
  %last_byte_offset = add i32 %page_offset, 255
  %last_byte_ptr = getelementptr i8, ptr %structure, i32 %last_byte_offset
  %first_byte_ptr = getelementptr i8, ptr %structure, i32 %next_page_offset
  
  %last_byte = load i8, ptr %last_byte_ptr, align 1
  %first_byte = load i8, ptr %first_byte_ptr, align 1
  
  %last_class = call i7 @atlas.r96.classify(i8 %last_byte)
  %first_class = call i7 @atlas.r96.classify(i8 %first_byte)
  
  ; Check if they form a harmonic pair
  %pair = call %atlas.harmonic.pair @atlas.harmonic.find_pair(i7 %last_class)
  %pair_valid = call i1 @atlas.harmonic.validate_pair(%atlas.harmonic.pair %pair)
  %boundaries_ok_new = and i1 %boundaries_ok, %pair_valid
  
  %next_page = add i32 %page_idx, 1
  br label %boundary_loop
  
exit:
  %final_boundaries_ok = phi i1 [ %boundaries_ok, %boundary_loop ]
  ret i1 %final_boundaries_ok
}

; Validate resonance distribution across entire structure
define i1 @atlas.validation.validate_resonance_distribution(ptr %structure) nounwind {
entry:
  ; Build overall histogram
  %global_histogram = alloca [96 x i32], align 16
  call void @llvm.memset.p0.i64(ptr %global_histogram, i8 0, i64 384, i1 false)
  
  ; Accumulate histograms from all pages
  br label %accumulate_loop
  
accumulate_loop:
  %page_idx = phi i32 [ 0, %entry ], [ %next_page, %next_page_acc ]
  %acc_done = icmp uge i32 %page_idx, 48
  br i1 %acc_done, label %analyze_distribution, label %accumulate_body
  
accumulate_body:
  %page_offset = mul i32 %page_idx, 256
  %page_ptr = getelementptr i8, ptr %structure, i32 %page_offset
  
  ; Get page histogram
  %page_histogram = alloca [96 x i32], align 16
  call void @atlas.r96.histogram(ptr %page_ptr, ptr %page_histogram)
  
  ; Add to global histogram
  br label %add_loop
  
add_loop:
  %class_idx = phi i32 [ 0, %accumulate_body ], [ %next_class, %add_body ]
  %add_done = icmp uge i32 %class_idx, 96
  br i1 %add_done, label %next_page_acc, label %add_body
  
add_body:
  %page_hist_ptr = getelementptr [96 x i32], ptr %page_histogram, i32 0, i32 %class_idx
  %global_hist_ptr = getelementptr [96 x i32], ptr %global_histogram, i32 0, i32 %class_idx
  %page_count = load i32, ptr %page_hist_ptr, align 4
  %global_count = load i32, ptr %global_hist_ptr, align 4
  %new_global_count = add i32 %global_count, %page_count
  store i32 %new_global_count, ptr %global_hist_ptr, align 4
  %next_class = add i32 %class_idx, 1
  br label %add_loop
  
next_page_acc:
  %next_page = add i32 %page_idx, 1
  br label %accumulate_loop
  
analyze_distribution:
  ; Check if distribution is reasonably uniform
  %expected_per_class = udiv i32 12288, 96  ; ~128 per class
  %tolerance = udiv i32 %expected_per_class, 4  ; 25% tolerance
  %min_acceptable = sub i32 %expected_per_class, %tolerance
  %max_acceptable = add i32 %expected_per_class, %tolerance
  
  br label %check_loop
  
check_loop:
  %check_idx = phi i32 [ 0, %analyze_distribution ], [ %next_check, %check_body ]
  %distribution_ok = phi i1 [ true, %analyze_distribution ], [ %distribution_ok_new, %check_body ]
  %check_done = icmp uge i32 %check_idx, 96
  br i1 %check_done, label %exit, label %check_body
  
check_body:
  %check_hist_ptr = getelementptr [96 x i32], ptr %global_histogram, i32 0, i32 %check_idx
  %count = load i32, ptr %check_hist_ptr, align 4
  %above_min = icmp uge i32 %count, %min_acceptable
  %below_max = icmp ule i32 %count, %max_acceptable
  %class_ok = and i1 %above_min, %below_max
  %distribution_ok_new = and i1 %distribution_ok, %class_ok
  %next_check = add i32 %check_idx, 1
  br label %check_loop
  
exit:
  %final_distribution_ok = phi i1 [ %distribution_ok, %check_loop ]
  ret i1 %final_distribution_ok
}

; Validate witness integrity
define i1 @atlas.validation.validate_witness_integrity(ptr %structure, ptr %witness) nounwind {
entry:
  ; Verify witness against current structure
  %witness_valid = call i1 @atlas.witness.verify(ptr %witness, ptr %structure, i64 12288)
  ret i1 %witness_valid
}

; Comprehensive cross-layer validation
define %atlas.validation.cross_layer_info @atlas.validation.validate_cross_layer(ptr %structure, ptr %witness) nounwind {
entry:
  ; Validate boundaries
  %boundaries_ok = call i1 @atlas.validation.validate_boundaries(ptr %structure)
  
  ; Validate resonance distribution
  %resonance_ok = call i1 @atlas.validation.validate_resonance_distribution(ptr %structure)
  
  ; Validate witness integrity  
  %witness_ok = call i1 @atlas.validation.validate_witness_integrity(ptr %structure, ptr %witness)
  
  ; Count total violations
  %boundary_violations = select i1 %boundaries_ok, i32 0, i32 1
  %resonance_violations = select i1 %resonance_ok, i32 0, i32 1
  %witness_violations = select i1 %witness_ok, i32 0, i32 1
  %temp_violations = add i32 %boundary_violations, %resonance_violations
  %total_violations = add i32 %temp_violations, %witness_violations
  
  ; Construct cross-layer info
  %info = insertvalue %atlas.validation.cross_layer_info undef, i1 %boundaries_ok, 0
  %info2 = insertvalue %atlas.validation.cross_layer_info %info, i1 %resonance_ok, 1
  %info3 = insertvalue %atlas.validation.cross_layer_info %info2, i1 %witness_ok, 2
  %info4 = insertvalue %atlas.validation.cross_layer_info %info3, i32 %total_violations, 3
  
  ret %atlas.validation.cross_layer_info %info4
}

; =============================================================================
; Main validation entry points
; =============================================================================

; Comprehensive validation of entire Atlas-12288 structure
define %atlas.validation.result @atlas.validation.validate_complete(ptr %structure, ptr %witness) nounwind {
entry:
  ; Page-level validation
  %pages_valid = alloca i1, align 1
  store i1 true, ptr %pages_valid, align 1
  
  br label %page_validation_loop
  
page_validation_loop:
  %page_idx = phi i32 [ 0, %entry ], [ %next_page, %page_validation_body ]
  %page_val_done = icmp uge i32 %page_idx, 48
  br i1 %page_val_done, label %structure_validation, label %page_validation_body
  
page_validation_body:
  %page_offset = mul i32 %page_idx, 256
  %page_ptr = getelementptr i8, ptr %structure, i32 %page_offset
  %page_info = call %atlas.validation.page_info @atlas.validation.validate_page(ptr %page_ptr)
  %page_structured = extractvalue %atlas.validation.page_info %page_info, 1
  %current_pages_valid = load i1, ptr %pages_valid, align 1
  %new_pages_valid = and i1 %current_pages_valid, %page_structured
  store i1 %new_pages_valid, ptr %pages_valid, align 1
  %next_page = add i32 %page_idx, 1
  br label %page_validation_loop
  
structure_validation:
  ; Structure-level validation
  %structure_info = call %atlas.validation.structure_info @atlas.validation.validate_structure(ptr %structure)
  %structure_complete = extractvalue %atlas.validation.structure_info %structure_info, 0
  
  ; Cross-layer validation
  %cross_layer_info = call %atlas.validation.cross_layer_info @atlas.validation.validate_cross_layer(ptr %structure, ptr %witness)
  %boundaries_ok = extractvalue %atlas.validation.cross_layer_info %cross_layer_info, 0
  %resonance_ok = extractvalue %atlas.validation.cross_layer_info %cross_layer_info, 1
  %witness_ok = extractvalue %atlas.validation.cross_layer_info %cross_layer_info, 2
  
  ; Determine overall success
  %final_pages_valid = load i1, ptr %pages_valid, align 1
  %temp_success = and i1 %final_pages_valid, %structure_complete
  %temp_success2 = and i1 %temp_success, %boundaries_ok
  %temp_success3 = and i1 %temp_success2, %resonance_ok
  %overall_success = and i1 %temp_success3, %witness_ok
  
  ; Determine error code and message
  br i1 %overall_success, label %success_result, label %determine_error
  
success_result:
  %success_code = load i32, ptr @atlas.validation.error.none, align 4
  %success_msg = getelementptr [19 x i8], ptr @atlas.validation.msg.success, i32 0, i32 0
  br label %create_result
  
determine_error:
  ; Determine specific error
  br i1 %final_pages_valid, label %check_structure, label %page_error
  
page_error:
  %page_error_code = load i32, ptr @atlas.validation.error.structure, align 4
  %page_error_msg = getelementptr [25 x i8], ptr @atlas.validation.msg.structure, i32 0, i32 0
  br label %create_result
  
check_structure:
  br i1 %structure_complete, label %check_boundaries, label %structure_error
  
structure_error:
  %struct_error_code = load i32, ptr @atlas.validation.error.conservation, align 4
  %struct_error_msg = getelementptr [23 x i8], ptr @atlas.validation.msg.conservation, i32 0, i32 0
  br label %create_result
  
check_boundaries:
  br i1 %boundaries_ok, label %check_resonance, label %boundary_error
  
boundary_error:
  %boundary_error_code = load i32, ptr @atlas.validation.error.boundary, align 4
  %boundary_error_msg = getelementptr [22 x i8], ptr @atlas.validation.msg.boundary, i32 0, i32 0
  br label %create_result
  
check_resonance:
  br i1 %resonance_ok, label %check_witness, label %resonance_error
  
resonance_error:
  %resonance_error_code = load i32, ptr @atlas.validation.error.resonance, align 4
  %resonance_error_msg = getelementptr [30 x i8], ptr @atlas.validation.msg.resonance, i32 0, i32 0
  br label %create_result
  
check_witness:
  br i1 %witness_ok, label %anomaly_error, label %witness_error
  
witness_error:
  %witness_error_code = load i32, ptr @atlas.validation.error.witness, align 4
  %witness_error_msg = getelementptr [23 x i8], ptr @atlas.validation.msg.witness, i32 0, i32 0
  br label %create_result
  
anomaly_error:
  %anomaly_error_code = load i32, ptr @atlas.validation.error.anomaly, align 4
  %anomaly_error_msg = getelementptr [21 x i8], ptr @atlas.validation.msg.anomaly, i32 0, i32 0
  br label %create_result
  
create_result:
  %result_success = phi i1 [ %overall_success, %success_result ], 
                           [ false, %page_error ], [ false, %structure_error ],
                           [ false, %boundary_error ], [ false, %resonance_error ],
                           [ false, %witness_error ], [ false, %anomaly_error ]
  %result_code = phi i32 [ %success_code, %success_result ],
                        [ %page_error_code, %page_error ], [ %struct_error_code, %structure_error ],
                        [ %boundary_error_code, %boundary_error ], [ %resonance_error_code, %resonance_error ],
                        [ %witness_error_code, %witness_error ], [ %anomaly_error_code, %anomaly_error ]
  %result_msg = phi ptr [ %success_msg, %success_result ],
                       [ %page_error_msg, %page_error ], [ %struct_error_msg, %structure_error ],
                       [ %boundary_error_msg, %boundary_error ], [ %resonance_error_msg, %resonance_error ],
                       [ %witness_error_msg, %witness_error ], [ %anomaly_error_msg, %anomaly_error ]
  
  %result = insertvalue %atlas.validation.result undef, i1 %result_success, 0
  %result2 = insertvalue %atlas.validation.result %result, i32 %result_code, 1
  %result3 = insertvalue %atlas.validation.result %result2, ptr %result_msg, 2
  
  ret %atlas.validation.result %result3
}

; =============================================================================
; LLVM intrinsic declarations
; =============================================================================

declare double @llvm.sqrt.f64(double) nounwind readnone
declare double @llvm.fabs.f64(double) nounwind readnone

; =============================================================================
; Exported symbols for linking
; =============================================================================

@llvm.used = appending global [11 x ptr] [
  ptr @atlas.validation.calculate_entropy,
  ptr @atlas.validation.check_page_structure,
  ptr @atlas.validation.validate_page,
  ptr @atlas.validation.count_anomalies,
  ptr @atlas.validation.validate_structure,
  ptr @atlas.validation.repair_page,
  ptr @atlas.validation.validate_boundaries,
  ptr @atlas.validation.validate_resonance_distribution,
  ptr @atlas.validation.validate_witness_integrity,
  ptr @atlas.validation.validate_cross_layer,
  ptr @atlas.validation.validate_complete
], section "llvm.metadata"

; ---------------------------------------------------------------------------------
; End of atlas-12288-validation.ll
; ---------------------------------------------------------------------------------