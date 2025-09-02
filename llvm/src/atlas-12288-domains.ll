; atlas-12288-domains.ll - Mathematical Domain Management
; Domains as conservation contexts, not business logic
; (c) 2024-2025 UOR Foundation. All rights reserved.
; SPDX-License-Identifier: MIT

; Domain type definition
%atlas.domain = type {
  i64,        ; domain_id (unique identifier)
  ptr,        ; structure (-> 12,288 bytes)
  i7,         ; budget (0..95)
  i32,        ; conservation_sum
  ptr,        ; witness
  i64,        ; isolation_proof
  i64         ; c768_phase
}

; Global domain counter for unique IDs
@atlas.domain.next_id = global i64 0

; === Domain Lifecycle ===

define ptr @atlas.domain.create(i7 %initial_budget) nounwind {
entry:
  ; Allocate domain structure
  %domain = call ptr @malloc(i64 56)  ; sizeof(%atlas.domain)
  
  ; Generate unique ID
  %id_ptr = load i64, ptr @atlas.domain.next_id
  %new_id = add i64 %id_ptr, 1
  store i64 %new_id, ptr @atlas.domain.next_id
  
  ; Initialize domain fields
  %id_field = getelementptr %atlas.domain, ptr %domain, i32 0, i32 0
  store i64 %id_ptr, ptr %id_field
  
  ; Allocate structure memory
  %structure = call ptr @atlas.alloc.aligned(i64 12288)
  %struct_field = getelementptr %atlas.domain, ptr %domain, i32 0, i32 1
  store ptr %structure, ptr %struct_field
  
  ; Set initial budget
  %budget_field = getelementptr %atlas.domain, ptr %domain, i32 0, i32 2
  store i7 %initial_budget, ptr %budget_field
  
  ; Initialize conservation sum
  %sum_field = getelementptr %atlas.domain, ptr %domain, i32 0, i32 3
  store i32 0, ptr %sum_field
  
  ; No witness yet
  %witness_field = getelementptr %atlas.domain, ptr %domain, i32 0, i32 4
  store ptr null, ptr %witness_field
  
  ; Generate isolation proof
  %isolation = call i64 @generate_isolation_proof()
  %isolation_field = getelementptr %atlas.domain, ptr %domain, i32 0, i32 5
  store i64 %isolation, ptr %isolation_field
  
  ; Initialize C768 phase
  %phase_field = getelementptr %atlas.domain, ptr %domain, i32 0, i32 6
  store i64 0, ptr %phase_field
  
  ret ptr %domain
}

define void @atlas.domain.destroy(ptr %domain) nounwind {
entry:
  %null_check = icmp eq ptr %domain, null
  br i1 %null_check, label %done, label %destroy

destroy:
  ; Free structure memory
  %struct_field = getelementptr %atlas.domain, ptr %domain, i32 0, i32 1
  %structure = load ptr, ptr %struct_field
  call void @free(ptr %structure)
  
  ; Free witness if present
  %witness_field = getelementptr %atlas.domain, ptr %domain, i32 0, i32 4
  %witness = load ptr, ptr %witness_field
  %has_witness = icmp ne ptr %witness, null
  br i1 %has_witness, label %free_witness, label %free_domain

free_witness:
  call void @atlas.witness.destroy(ptr %witness)
  br label %free_domain

free_domain:
  ; Free domain structure
  call void @free(ptr %domain)
  br label %done

done:
  ret void
}

define ptr @atlas.domain.clone(ptr %domain) nounwind {
entry:
  ; Get original budget
  %budget_field = getelementptr %atlas.domain, ptr %domain, i32 0, i32 2
  %budget = load i7, ptr %budget_field
  
  ; Create new domain
  %new_domain = call ptr @atlas.domain.create(i7 %budget)
  
  ; Copy structure data
  %old_struct_field = getelementptr %atlas.domain, ptr %domain, i32 0, i32 1
  %old_structure = load ptr, ptr %old_struct_field
  %new_struct_field = getelementptr %atlas.domain, ptr %new_domain, i32 0, i32 1
  %new_structure = load ptr, ptr %new_struct_field
  call void @llvm.memcpy.p0.p0.i64(ptr %new_structure, ptr %old_structure, i64 12288, i1 false)
  
  ; Copy conservation sum
  %old_sum_field = getelementptr %atlas.domain, ptr %domain, i32 0, i32 3
  %old_sum = load i32, ptr %old_sum_field
  %new_sum_field = getelementptr %atlas.domain, ptr %new_domain, i32 0, i32 3
  store i32 %old_sum, ptr %new_sum_field
  
  ; Copy C768 phase
  %old_phase_field = getelementptr %atlas.domain, ptr %domain, i32 0, i32 6
  %old_phase = load i64, ptr %old_phase_field
  %new_phase_field = getelementptr %atlas.domain, ptr %new_domain, i32 0, i32 6
  store i64 %old_phase, ptr %new_phase_field
  
  ret ptr %new_domain
}

define i1 @atlas.domain.validate(ptr %domain) nounwind {
entry:
  ; Check domain structure validity
  %null_check = icmp eq ptr %domain, null
  br i1 %null_check, label %invalid, label %check_structure

check_structure:
  %struct_field = getelementptr %atlas.domain, ptr %domain, i32 0, i32 1
  %structure = load ptr, ptr %struct_field
  %struct_valid = icmp ne ptr %structure, null
  br i1 %struct_valid, label %check_budget, label %invalid

check_budget:
  %budget_field = getelementptr %atlas.domain, ptr %domain, i32 0, i32 2
  %budget = load i7, ptr %budget_field
  %budget_valid = icmp ult i7 %budget, 96
  br i1 %budget_valid, label %check_conservation, label %invalid

check_conservation:
  ; Verify conservation
  %conserved = call i1 @atlas.conserved.check(ptr %structure, i64 12288)
  br i1 %conserved, label %valid, label %invalid

valid:
  ret i1 true

invalid:
  ret i1 false
}

; === Isolation Verification ===

define i1 @atlas.domain.verify_isolated(ptr %d1, ptr %d2) nounwind readonly {
entry:
  ; Check if two domains are properly isolated
  %iso1_field = getelementptr %atlas.domain, ptr %d1, i32 0, i32 5
  %iso2_field = getelementptr %atlas.domain, ptr %d2, i32 0, i32 5
  
  %iso1 = load i64, ptr %iso1_field
  %iso2 = load i64, ptr %iso2_field
  
  ; Domains are isolated if their proofs don't share factors
  %gcd = call i64 @compute_gcd(i64 %iso1, i64 %iso2)
  %isolated = icmp eq i64 %gcd, 1
  
  ret i1 %isolated
}

define i64 @atlas.domain.compute_isolation_proof(ptr %d1, ptr %d2) nounwind {
entry:
  ; Compute proof that domains can/cannot interact
  %iso1_field = getelementptr %atlas.domain, ptr %d1, i32 0, i32 5
  %iso2_field = getelementptr %atlas.domain, ptr %d2, i32 0, i32 5
  
  %iso1 = load i64, ptr %iso1_field
  %iso2 = load i64, ptr %iso2_field
  
  ; Proof is product of isolation values
  %proof = mul i64 %iso1, %iso2
  ret i64 %proof
}

define i1 @atlas.domain.can_interact(ptr %d1, ptr %d2) nounwind readonly {
entry:
  ; Check if domains can interact (not isolated)
  %isolated = call i1 @atlas.domain.verify_isolated(ptr %d1, ptr %d2)
  %can_interact = xor i1 %isolated, true
  ret i1 %can_interact
}

; === Budget Management ===

define i1 @atlas.domain.transfer_budget(ptr %from, ptr %to, i7 %amount) nounwind {
entry:
  ; Get current budgets
  %from_budget_field = getelementptr %atlas.domain, ptr %from, i32 0, i32 2
  %to_budget_field = getelementptr %atlas.domain, ptr %to, i32 0, i32 2
  
  %from_budget = load i7, ptr %from_budget_field
  %to_budget = load i7, ptr %to_budget_field
  
  ; Check if transfer is valid
  %has_funds = icmp uge i7 %from_budget, %amount
  br i1 %has_funds, label %check_overflow, label %fail

check_overflow:
  ; Check if recipient can receive
  %to_ext = zext i7 %to_budget to i8
  %amount_ext = zext i7 %amount to i8
  %new_to = add i8 %to_ext, %amount_ext
  %overflow = icmp ugt i8 %new_to, 95
  br i1 %overflow, label %fail, label %transfer

transfer:
  ; Perform transfer
  %new_from = sub i7 %from_budget, %amount
  store i7 %new_from, ptr %from_budget_field
  
  %new_to_trunc = trunc i8 %new_to to i7
  store i7 %new_to_trunc, ptr %to_budget_field
  
  ret i1 true

fail:
  ret i1 false
}

define i1 @atlas.domain.can_afford(ptr %domain, i7 %cost) nounwind readonly {
entry:
  %budget_field = getelementptr %atlas.domain, ptr %domain, i32 0, i32 2
  %budget = load i7, ptr %budget_field
  %can_afford = icmp uge i7 %budget, %cost
  ret i1 %can_afford
}

define i7 @atlas.domain.available_budget(ptr %domain) nounwind readonly {
entry:
  %budget_field = getelementptr %atlas.domain, ptr %domain, i32 0, i32 2
  %budget = load i7, ptr %budget_field
  ret i7 %budget
}

define void @atlas.domain.reserve_budget(ptr %domain, i7 %amount) nounwind {
entry:
  %budget_field = getelementptr %atlas.domain, ptr %domain, i32 0, i32 2
  %budget = load i7, ptr %budget_field
  %new_budget = sub i7 %budget, %amount
  store i7 %new_budget, ptr %budget_field
  ret void
}

define void @atlas.domain.release_budget(ptr %domain, i7 %amount) nounwind {
entry:
  %budget_field = getelementptr %atlas.domain, ptr %domain, i32 0, i32 2
  %budget = load i7, ptr %budget_field
  %budget_ext = zext i7 %budget to i8
  %amount_ext = zext i7 %amount to i8
  %new_budget_ext = add i8 %budget_ext, %amount_ext
  %new_budget = trunc i8 %new_budget_ext to i7
  store i7 %new_budget, ptr %budget_field
  ret void
}

; === Domain Operations ===

define ptr @atlas.domain.fork(ptr %parent, i7 %child_budget) nounwind {
entry:
  ; Check if parent can afford fork
  %can_afford = call i1 @atlas.domain.can_afford(ptr %parent, i7 %child_budget)
  br i1 %can_afford, label %fork, label %fail

fork:
  ; Create child domain
  %child = call ptr @atlas.domain.create(i7 %child_budget)
  
  ; Copy parent structure to child
  %parent_struct_field = getelementptr %atlas.domain, ptr %parent, i32 0, i32 1
  %parent_struct = load ptr, ptr %parent_struct_field
  %child_struct_field = getelementptr %atlas.domain, ptr %child, i32 0, i32 1
  %child_struct = load ptr, ptr %child_struct_field
  call void @llvm.memcpy.p0.p0.i64(ptr %child_struct, ptr %parent_struct, i64 12288, i1 false)
  
  ; Deduct budget from parent
  call void @atlas.domain.reserve_budget(ptr %parent, i7 %child_budget)
  
  ; Inherit C768 phase
  %parent_phase_field = getelementptr %atlas.domain, ptr %parent, i32 0, i32 6
  %parent_phase = load i64, ptr %parent_phase_field
  %child_phase_field = getelementptr %atlas.domain, ptr %child, i32 0, i32 6
  store i64 %parent_phase, ptr %child_phase_field
  
  ret ptr %child

fail:
  ret ptr null
}

define ptr @atlas.domain.merge(ptr %d1, ptr %d2) nounwind {
entry:
  ; Get budgets
  %b1_field = getelementptr %atlas.domain, ptr %d1, i32 0, i32 2
  %b2_field = getelementptr %atlas.domain, ptr %d2, i32 0, i32 2
  %b1 = load i7, ptr %b1_field
  %b2 = load i7, ptr %b2_field
  
  ; Compute merged budget
  %b1_ext = zext i7 %b1 to i8
  %b2_ext = zext i7 %b2 to i8
  %merged_budget_ext = add i8 %b1_ext, %b2_ext
  %merged_budget_mod = urem i8 %merged_budget_ext, 96
  %merged_budget = trunc i8 %merged_budget_mod to i7
  
  ; Create merged domain
  %merged = call ptr @atlas.domain.create(i7 %merged_budget)
  
  ; Merge structures (XOR for simplicity)
  %s1_field = getelementptr %atlas.domain, ptr %d1, i32 0, i32 1
  %s2_field = getelementptr %atlas.domain, ptr %d2, i32 0, i32 1
  %merged_field = getelementptr %atlas.domain, ptr %merged, i32 0, i32 1
  
  %s1 = load ptr, ptr %s1_field
  %s2 = load ptr, ptr %s2_field
  %merged_struct = load ptr, ptr %merged_field
  
  call void @merge_structures(ptr %s1, ptr %s2, ptr %merged_struct)
  
  ret ptr %merged
}

define i1 @atlas.domain.sync_conservation(ptr %domain) nounwind {
entry:
  ; Update conservation sum
  %struct_field = getelementptr %atlas.domain, ptr %domain, i32 0, i32 1
  %structure = load ptr, ptr %struct_field
  
  %sum = call i64 @atlas._sum_bytes(ptr %structure, i64 12288)
  %sum_mod = urem i64 %sum, 96
  %sum_trunc = trunc i64 %sum_mod to i32
  
  %sum_field = getelementptr %atlas.domain, ptr %domain, i32 0, i32 3
  store i32 %sum_trunc, ptr %sum_field
  
  ; Check if conserved
  %conserved = icmp eq i32 %sum_trunc, 0
  ret i1 %conserved
}

; === Witness Binding ===

define void @atlas.domain.bind_witness(ptr %domain, ptr %witness) nounwind {
entry:
  %witness_field = getelementptr %atlas.domain, ptr %domain, i32 0, i32 4
  
  ; Free old witness if present
  %old_witness = load ptr, ptr %witness_field
  %has_old = icmp ne ptr %old_witness, null
  br i1 %has_old, label %free_old, label %bind

free_old:
  call void @atlas.witness.destroy(ptr %old_witness)
  br label %bind

bind:
  store ptr %witness, ptr %witness_field
  ret void
}

define i1 @atlas.domain.verify_witness_chain(ptr %domain) nounwind readonly {
entry:
  %witness_field = getelementptr %atlas.domain, ptr %domain, i32 0, i32 4
  %witness = load ptr, ptr %witness_field
  
  %has_witness = icmp ne ptr %witness, null
  br i1 %has_witness, label %verify, label %no_witness

verify:
  %struct_field = getelementptr %atlas.domain, ptr %domain, i32 0, i32 1
  %structure = load ptr, ptr %struct_field
  %valid = call i1 @atlas.witness.verify(ptr %witness, ptr %structure, i64 12288)
  ret i1 %valid

no_witness:
  ret i1 false
}

define ptr @atlas.domain.export_proof(ptr %domain) nounwind {
entry:
  ; Generate exportable proof
  %struct_field = getelementptr %atlas.domain, ptr %domain, i32 0, i32 1
  %structure = load ptr, ptr %struct_field
  
  %witness = call ptr @atlas.witness.generate(ptr %structure, i64 12288)
  
  ; Bind to domain
  call void @atlas.domain.bind_witness(ptr %domain, ptr %witness)
  
  ret ptr %witness
}

; Helper functions
define internal i64 @generate_isolation_proof() nounwind {
entry:
  ; Generate a unique isolation proof (simplified: use timestamp)
  %time = call i64 @time(ptr null)
  %proof = mul i64 %time, 997  ; Large prime
  ret i64 %proof
}

define internal i64 @compute_gcd(i64 %a, i64 %b) nounwind readnone {
entry:
  br label %loop

loop:
  %x = phi i64 [ %a, %entry ], [ %y, %body ]
  %y = phi i64 [ %b, %entry ], [ %rem, %body ]
  %cmp = icmp eq i64 %y, 0
  br i1 %cmp, label %done, label %body

body:
  %rem = urem i64 %x, %y
  br label %loop

done:
  ret i64 %x
}

define internal void @merge_structures(ptr %s1, ptr %s2, ptr %out) nounwind {
entry:
  br label %loop

loop:
  %i = phi i64 [ 0, %entry ], [ %next_i, %body ]
  %cmp = icmp ult i64 %i, 12288
  br i1 %cmp, label %body, label %done

body:
  %p1 = getelementptr i8, ptr %s1, i64 %i
  %p2 = getelementptr i8, ptr %s2, i64 %i
  %po = getelementptr i8, ptr %out, i64 %i
  
  %v1 = load i8, ptr %p1
  %v2 = load i8, ptr %p2
  %merged = xor i8 %v1, %v2
  store i8 %merged, ptr %po
  
  %next_i = add i64 %i, 1
  br label %loop

done:
  ret void
}

; External dependencies
declare ptr @malloc(i64) nounwind
declare void @free(ptr) nounwind
declare i64 @time(ptr) nounwind
declare ptr @atlas.alloc.aligned(i64) nounwind
declare void @atlas.witness.destroy(ptr) nounwind
declare ptr @atlas.witness.generate(ptr, i64) nounwind
declare i1 @atlas.witness.verify(ptr, ptr, i64) nounwind readonly
declare i1 @atlas.conserved.check(ptr, i64) nounwind readonly
declare i64 @atlas._sum_bytes(ptr, i64) nounwind readonly
declare void @llvm.memcpy.p0.p0.i64(ptr, ptr, i64, i1) nounwind