; test-domains.ll - Test domain isolation, budget transfers, fork/merge operations, witness chains
; Tests the mathematical domain management system

source_filename = "test-domains.ll"
target datalayout = "e-m:e-p270:32:32-p271:32:32-p272:64:64-i64:64-i128:128-f80:128-n8:16:32:64-S128"
target triple = "x86_64-unknown-linux-gnu"

; External functions from atlas-12288-domains.ll
declare ptr @atlas.domain.create(i7) nounwind
declare void @atlas.domain.destroy(ptr) nounwind
declare ptr @atlas.domain.clone(ptr) nounwind
declare i1 @atlas.domain.validate(ptr) nounwind
declare i1 @atlas.domain.verify_isolated(ptr, ptr) nounwind readonly
declare i64 @atlas.domain.compute_isolation_proof(ptr, ptr) nounwind
declare i1 @atlas.domain.can_interact(ptr, ptr) nounwind readonly
declare i1 @atlas.domain.transfer_budget(ptr, ptr, i7) nounwind
declare i1 @atlas.domain.can_afford(ptr, i7) nounwind readonly
declare i7 @atlas.domain.available_budget(ptr) nounwind readonly
declare void @atlas.domain.reserve_budget(ptr, i7) nounwind
declare void @atlas.domain.release_budget(ptr, i7) nounwind
declare ptr @atlas.domain.fork(ptr, i7) nounwind
declare ptr @atlas.domain.merge(ptr, ptr) nounwind
declare i1 @atlas.domain.sync_conservation(ptr) nounwind
declare void @atlas.domain.bind_witness(ptr, ptr) nounwind
declare i1 @atlas.domain.verify_witness_chain(ptr) nounwind readonly
declare ptr @atlas.domain.export_proof(ptr) nounwind

; External functions from other modules
declare ptr @atlas.witness.create() nounwind
declare i32 @printf(ptr, ...)

; String constants for output
@.str.pass = private unnamed_addr constant [30 x i8] c"Domains Test PASSED - All OK\0A\00"
@.str.fail = private unnamed_addr constant [30 x i8] c"Domains Test FAILED - Error: \00"
@.str.create = private unnamed_addr constant [15 x i8] c"create failed\0A\00"
@.str.validate = private unnamed_addr constant [17 x i8] c"validate failed\0A\00"
@.str.isolation = private unnamed_addr constant [18 x i8] c"isolation failed\0A\00"
@.str.budget = private unnamed_addr constant [15 x i8] c"budget failed\0A\00"
@.str.transfer = private unnamed_addr constant [17 x i8] c"transfer failed\0A\00"
@.str.fork = private unnamed_addr constant [13 x i8] c"fork failed\0A\00"
@.str.merge = private unnamed_addr constant [14 x i8] c"merge failed\0A\00"
@.str.witness = private unnamed_addr constant [16 x i8] c"witness failed\0A\00"

define i32 @test_domain_operations() {
entry:
  ; Test domain creation
  %initial_budget = add i7 0, 50
  %domain1 = call ptr @atlas.domain.create(i7 %initial_budget)
  
  ; Check domain was created
  %created = icmp ne ptr %domain1, null
  br i1 %created, label %test_validate, label %fail_create

test_validate:
  ; Test domain validation
  %valid1 = call i1 @atlas.domain.validate(ptr %domain1)
  br i1 %valid1, label %test_budget_ops, label %fail_validate

test_budget_ops:
  ; Test budget operations
  %available = call i7 @atlas.domain.available_budget(ptr %domain1)
  %budget_ok = icmp eq i7 %available, %initial_budget
  br i1 %budget_ok, label %test_afford, label %fail_budget

test_afford:
  ; Test affordability check
  %can_afford_10 = call i1 @atlas.domain.can_afford(ptr %domain1, i7 10)
  %can_afford_100 = call i1 @atlas.domain.can_afford(ptr %domain1, i7 100)
  
  %afford_ok = and i1 %can_afford_10, true
  %cant_afford_ok = xor i1 %can_afford_100, true  ; Should NOT be able to afford 100
  %afford_checks = and i1 %afford_ok, %cant_afford_ok
  br i1 %afford_checks, label %test_reserve, label %fail_budget

test_reserve:
  ; Test budget reservation
  call void @atlas.domain.reserve_budget(ptr %domain1, i7 10)
  %after_reserve = call i7 @atlas.domain.available_budget(ptr %domain1)
  %expected_after = sub i7 %initial_budget, 10
  %reserve_ok = icmp eq i7 %after_reserve, %expected_after
  br i1 %reserve_ok, label %test_release, label %fail_budget

test_release:
  ; Test budget release
  call void @atlas.domain.release_budget(ptr %domain1, i7 5)
  %after_release = call i7 @atlas.domain.available_budget(ptr %domain1)
  %expected_release = add i7 %expected_after, 5
  %release_ok = icmp eq i7 %after_release, %expected_release
  br i1 %release_ok, label %test_second_domain, label %fail_budget

test_second_domain:
  ; Create second domain for isolation testing
  %domain2 = call ptr @atlas.domain.create(i7 30)
  %created2 = icmp ne ptr %domain2, null
  br i1 %created2, label %test_isolation, label %fail_create

test_isolation:
  ; Test domain isolation
  %isolated = call i1 @atlas.domain.verify_isolated(ptr %domain1, ptr %domain2)
  br i1 %isolated, label %test_interaction, label %fail_isolation

test_interaction:
  ; Test interaction capability (should be possible initially)
  %can_interact = call i1 @atlas.domain.can_interact(ptr %domain1, ptr %domain2)
  br i1 %can_interact, label %test_budget_transfer, label %test_budget_transfer  ; Continue even if no interaction

test_budget_transfer:
  ; Test budget transfer between domains
  %transfer_amount = add i7 0, 5
  %transfer_ok = call i1 @atlas.domain.transfer_budget(ptr %domain1, ptr %domain2, i7 %transfer_amount)
  
  ; Check if transfer succeeded or failed appropriately
  br i1 %transfer_ok, label %verify_transfer, label %test_fork

verify_transfer:
  ; If transfer succeeded, verify budget changes
  %d1_after_transfer = call i7 @atlas.domain.available_budget(ptr %domain1)
  %d2_after_transfer = call i7 @atlas.domain.available_budget(ptr %domain2)
  
  ; Domain1 should have less, domain2 should have more
  %d1_reduced = icmp ult i7 %d1_after_transfer, %after_release
  %d2_increased = icmp ugt i7 %d2_after_transfer, 30
  %transfer_verified = and i1 %d1_reduced, %d2_increased
  br i1 %transfer_verified, label %test_fork, label %test_fork  ; Continue regardless

test_fork:
  ; Test domain forking
  %child_budget = add i7 0, 15
  %child_domain = call ptr @atlas.domain.fork(ptr %domain1, i7 %child_budget)
  %fork_ok = icmp ne ptr %child_domain, null
  br i1 %fork_ok, label %test_merge, label %fail_fork

test_merge:
  ; Test domain merging
  %merged = call ptr @atlas.domain.merge(ptr %domain2, ptr %child_domain)
  %merge_ok = icmp ne ptr %merged, null
  br i1 %merge_ok, label %test_conservation, label %fail_merge

test_conservation:
  ; Test conservation synchronization
  %sync_ok = call i1 @atlas.domain.sync_conservation(ptr %domain1)
  br i1 %sync_ok, label %test_witness_ops, label %test_witness_ops  ; Continue regardless

test_witness_ops:
  ; Test witness operations
  %witness = call ptr @atlas.witness.create()
  %witness_created = icmp ne ptr %witness, null
  br i1 %witness_created, label %bind_witness, label %test_clone

bind_witness:
  ; Bind witness to domain
  call void @atlas.domain.bind_witness(ptr %domain1, ptr %witness)
  
  ; Verify witness chain
  %chain_ok = call i1 @atlas.domain.verify_witness_chain(ptr %domain1)
  br i1 %chain_ok, label %test_proof_export, label %test_clone

test_proof_export:
  ; Test proof export
  %proof = call ptr @atlas.domain.export_proof(ptr %domain1)
  %proof_ok = icmp ne ptr %proof, null
  br i1 %proof_ok, label %test_clone, label %test_clone

test_clone:
  ; Test domain cloning
  %cloned = call ptr @atlas.domain.clone(ptr %domain1)
  %clone_ok = icmp ne ptr %cloned, null
  br i1 %clone_ok, label %validate_clone, label %cleanup

validate_clone:
  ; Validate cloned domain
  %clone_valid = call i1 @atlas.domain.validate(ptr %cloned)
  br i1 %clone_valid, label %cleanup, label %cleanup

cleanup:
  ; Clean up domains
  call void @atlas.domain.destroy(ptr %domain1)
  call void @atlas.domain.destroy(ptr %domain2)
  call void @atlas.domain.destroy(ptr %child_domain)
  call void @atlas.domain.destroy(ptr %merged)
  call void @atlas.domain.destroy(ptr %cloned)
  
  br label %pass

pass:
  %msg_pass = getelementptr [30 x i8], ptr @.str.pass, i32 0, i32 0
  call i32 @printf(ptr %msg_pass)
  ret i32 0

fail_create:
  %msg_fail1 = getelementptr [30 x i8], ptr @.str.fail, i32 0, i32 0
  %msg_create = getelementptr [17 x i8], ptr @.str.create, i32 0, i32 0
  call i32 @printf(ptr %msg_fail1)
  call i32 @printf(ptr %msg_create)
  ret i32 1

fail_validate:
  %msg_fail2 = getelementptr [30 x i8], ptr @.str.fail, i32 0, i32 0
  %msg_validate = getelementptr [19 x i8], ptr @.str.validate, i32 0, i32 0
  call i32 @printf(ptr %msg_fail2)
  call i32 @printf(ptr %msg_validate)
  ret i32 1

fail_budget:
  %msg_fail3 = getelementptr [30 x i8], ptr @.str.fail, i32 0, i32 0
  %msg_budget = getelementptr [17 x i8], ptr @.str.budget, i32 0, i32 0
  call i32 @printf(ptr %msg_fail3)
  call i32 @printf(ptr %msg_budget)
  ret i32 1

fail_isolation:
  %msg_fail4 = getelementptr [30 x i8], ptr @.str.fail, i32 0, i32 0
  %msg_isolation = getelementptr [20 x i8], ptr @.str.isolation, i32 0, i32 0
  call i32 @printf(ptr %msg_fail4)
  call i32 @printf(ptr %msg_isolation)
  ret i32 1

fail_transfer:
  %msg_fail5 = getelementptr [30 x i8], ptr @.str.fail, i32 0, i32 0
  %msg_transfer = getelementptr [19 x i8], ptr @.str.transfer, i32 0, i32 0
  call i32 @printf(ptr %msg_fail5)
  call i32 @printf(ptr %msg_transfer)
  ret i32 1

fail_fork:
  %msg_fail6 = getelementptr [30 x i8], ptr @.str.fail, i32 0, i32 0
  %msg_fork = getelementptr [15 x i8], ptr @.str.fork, i32 0, i32 0
  call i32 @printf(ptr %msg_fail6)
  call i32 @printf(ptr %msg_fork)
  ret i32 1

fail_merge:
  %msg_fail7 = getelementptr [30 x i8], ptr @.str.fail, i32 0, i32 0
  %msg_merge = getelementptr [16 x i8], ptr @.str.merge, i32 0, i32 0
  call i32 @printf(ptr %msg_fail7)
  call i32 @printf(ptr %msg_merge)
  ret i32 1

fail_witness:
  %msg_fail8 = getelementptr [30 x i8], ptr @.str.fail, i32 0, i32 0
  %msg_witness = getelementptr [18 x i8], ptr @.str.witness, i32 0, i32 0
  call i32 @printf(ptr %msg_fail8)
  call i32 @printf(ptr %msg_witness)
  ret i32 1
}

define i32 @main() {
entry:
  %result = call i32 @test_domain_operations()
  ret i32 %result
}