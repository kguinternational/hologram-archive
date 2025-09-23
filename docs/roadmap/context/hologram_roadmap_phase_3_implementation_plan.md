# Phase 3 Implementation Plan — Kernel Runtime (Portable Execution Beta)

## 1) Objectives
- Implement a **host‑agnostic kernel runtime** that executes Atlas **projections** deterministically with **content‑addressed I/O**.
- Enforce **admission** (digest, bindings, policies, capabilities, and grants) before any execution.
- Provide stable **I/O surfaces** (streams, kv/table views, message ports) that route through CAM and NRS.
- Support **host specialization** via cached thunks keyed by `(digest, hostProfile)` without changing observable behavior.
- Emit **RunReceipts** with lineage (inputs/outputs), status, and optional attestation references.

## 2) Scope
**In scope:** runtime daemon and library, sandboxing, admission pipeline, I/O adapters to CAM/NRS, host‑profile negotiation, thunk cache, run scheduler, run receipts, debugging and observability.

**Out of scope (Phase‑3):** distributed scheduling across nodes (Phase‑4/registry), economics/settlement, advanced attestation policy (hooks only).

---

## 3) Architecture Overview
```
+------------------------------+
| krd (kernel runtime daemon)  |
|  - Admission Pipeline        |
|  - Sandbox Manager           |
|  - Thunk Cache (AOT/JIT)     |
|  - I/O Surfaces              |
|  - Scheduler & Signals       |
|  - Run Receipt Emitter       |
+------------------------------+
       ^            ^
       | gRPC/UDS   | local APIs
       |            |
+------+-----+  +---+-----------------------------+
| holo-camd  |  | Router / NRS (Phase-1/2)       |
| (storage)  |  | (resolve, provider selection)   |
+------------+  +---------------------------------+
```

**Execution models supported:**
- **AOT bytecode VM** (ref interpreter + optional JIT).
- **Native projection** (optional) via sealed thunk compiled from Atlas payload if permitted by policy.

---

## 4) Host Profiles & Capability Discovery
- **HostProfile**: `{arch, os, abi, cpuFeatures[], gpuFeatures[], accel[], memory, storageTiers[], site, runtimeVersion}`.
- `krd advertise-profile` publishes profile locally; control plane can fetch for remote attestation later.
- **Compatibility**: projection manifest includes `compatProfile` and optional alternatives. Admission ensures a match.

---

## 5) Admission Pipeline (deterministic)
1. **Fetch**: Ensure object and declared deps exist in CAM; otherwise invoke Router to fetch by CID.
2. **Integrity**: Verify Atlas CID and section digests.
3. **Bindings & Policy**: Resolve handles to CIDs per policy; lock specific binding(s) used in decision trace.
4. **Capabilities**: Check runtime capability token (e.g., `run:namespace=X` with ceilings).
5. **Grants (user data)**: Validate presented grant tokens for any `view://` dependencies; ensure scope/purpose/duration fit declared I/O.
6. **Compatibility**: Compare `compatProfile` vs HostProfile; choose thunk (AOT/JIT) keyed by `(digest, hostProfile)`.
7. **Sandbox Plan**: Build container spec (namespaces, mounts, cgroups/rlimits/seccomp or platform equivalent).
8. **Admission Receipt** *(local)*: Record decision context; folded into final RunReceipt.

If any step fails, emit a structured denial with the exact rule that failed.

---

## 6) Sandbox & Execution Environment
- **Isolation**: OS namespaces (net, pid, mount), cgroups/rlimits, seccomp/BPF; on Windows/macOS use equivalent containers/jails.
- **Filesystem view**: read‑only mounts from CAM for declared packs; writable scratch is content‑addressed (committed via `put`).
- **Network**: disabled by default; enable only if declared and allowed by capability policy.
- **Clock & entropy**: deterministic mode provides a virtual clock/PRNG seeded via declared inputs; otherwise, mark non‑deterministic.
- **Signals**: start, health, shutdown; timeouts and watchdog.

---

## 7) I/O Surfaces (stable contracts)
- **Streams**: stdin/stdout/stderr; additional named pipes described in manifest.
- **Key/Value**: bounded keyspace with schema‑hashed values; reads/writes go through CAM with view receipts optional.
- **Table Views**: append‑only or bitemporal tables; view addresses resolve via NRS; grant‑guarded access.
- **Message Ports**: pub/sub to named topics; messages are CAM nodes; transfer receipts available.
- **Blob Store**: large payloads written via `put` into CAM; references emitted on stdout or port events.

Adapters ensure no durable read/write bypasses CAM.

---

## 8) Thunk Cache (Host Specialization)
- **Key**: `(digest, hostProfile.version, vmVersion)`.
- **Entries**: compiled code + metadata (codegen options, safety checksums).
- **Eviction**: LRU with pinning for frequently used projections.
- **Verification**: after thunk selection, hash of thunk + manifest must match recorded entry; mismatch → rebuild or fall back to interpreter.
- **Invariant**: behavior equivalence test suite executed periodically (golden I/O vectors).

---

## 9) Scheduler & Lifecycle
- **Queue model**: FIFO with priority lanes; per‑tenant ceilings enforced via capabilities.
- **Prefetch**: before admit, instruct Router to stage dependencies and likely outputs (e.g., codec packs).
- **Health**: projections may expose a health endpoint; `krd` monitors and can terminate on policy breaches.
- **Backpressure**: reject or queue when scratch or RAM budget exceeded; emit denial receipts.

---

## 10) Run Receipts (v1)
Common fields: `{ projectionCid, entrypoint, inputs[], outputs[], exit, startedAt, finishedAt, hostProfileDigest, capabilityRef?, grantRefs[], evidenceRefs[] }`
- **inputs[]**: `{ role, cid|viewAddr, bindingUsed? }`
- **outputs[]**: `{ role, cid }` (CAM put receipts linked)
- **exit**: `{ code, signal?, status:"success|failure|timeout|killed" }`
- **determinism**: `{ mode:"deterministic|nondeterministic", clockSeed?, prngSeed? }`
- **decisionTrace**: reference to locked binding/resolve decisions.
- **attestation**: optional device/runtime/storage/path evidence references.

All signed (COSE) with runtime key; verifier can replay decisions offline.

---

## 11) Observability & Debuggability
- **Trace IDs**: end‑to‑end for resolve→fetch→run.
- **Structured logs**: emitted to stdout and optionally to a local ring buffer; not authoritative (receipts are).
- **`krd inspect <runId>`**: shows admission decision, bindings used, grants checked, I/O references, and sandbox plan.
- **`krd replay <runReceipt>`**: dry‑run verifier to confirm determinism and policy adherence.

---

## 12) Performance Targets
- **Admission latency**: P50 < 15 ms (with warm Router caches).
- **Cold start**: < 300 ms for interpreter; < 800 ms with thunk JIT build.
- **I/O throughput**: match Phase‑1 CAM read/write targets; no more than 5% overhead from I/O adapters.
- **CPU**: VM/JIT achieves ≥ 70% of native for compute‑bound microbenchmarks on supported hosts.

---

## 13) Security & Policy
- **Least privilege**: net off by default; explicit capability required to opt‑in.
- **Grant enforcement**: every view access must present a valid grant; revocations take effect on subsequent resolves.
- **Code provenance**: RunReceipt links to Projection CID and BindingReceipt; optional code‑review evidence via `policyRef`.
- **Sandbox escapes**: continuous tests; seccomp profiles locked per compat profile.
- **Secret handling**: secrets provided via view handles or sealed capabilities; never embedded in projection bytes.

---

## 14) CLI/SDK
- `holo run <handle|cid> [--input <role=cid|view> ...] [--cap <cap.json>] [--grant <grant.json>] [--deterministic]`.
- `holo runs ls|inspect|replay|kill`.
- SDKs expose admission and run APIs with callbacks for I/O surface adapters.

---

## 15) Testing Strategy
- **Unit**: admission rules; grant/cap checks; thunk cache keying; sandbox plan generation.
- **Golden I/O**: replay deterministic projections with fixed seeds; byte‑exact outputs across hosts.
- **Isolation**: attempt privileged syscalls; verify seccomp denials; network blocked by default.
- **Stress**: fork bombs prevention; memory ceilings; scratch fill/GC interaction.
- **Compatibility**: run same digest on ARM64/x86_64; compare RunReceipts and outputs.
- **Failure injection**: kill during write; torn outputs; router handoff mid‑run.

---

## 16) Work Breakdown Structure (WBS)
1. **Admission Engine**: decision graph + decision trace object; policy bindings.
2. **Sandbox Manager**: container spec translators per OS; seccomp/jail profiles.
3. **I/O Surfaces**: stream/kv/table/message adapters; CAM/NRS integration.
4. **Thunk Cache**: JIT/AOT pipeline; storage; verification.
5. **Scheduler**: queueing, ceilings, backpressure, health.
6. **Run Receipts**: schema + signer; replay tool.
7. **CLI/SDK**: run/inspect/replay; examples.
8. **Observability**: metrics, tracing, run dashboards.
9. **Security**: capability/grant enforcement; secret handling; fuzzing.

---

## 17) Milestones & Exit Criteria
**M1: Admission + Interpreter (Week 4)**
- Can execute a projection via interpreter; admission enforces digest/binding/policy/cap/grant.

**M2: I/O Surfaces + Receipts (Week 7)**
- Streams and CAM‑backed blob writes; RunReceipts emitted with inputs/outputs and exit status.

**M3: Thunk Cache + Sandbox Hardening (Week 9)**
- JIT/AOT thunks cached by `(digest, hostProfile)`; seccomp/jail profiles locked; determinism mode toggle.

**M4: Cross‑Host Equivalence (Week 12)**
- Same digest → same outputs and receipts on ARM64 & x86_64 using golden inputs.

**Phase‑3 Exit**
- Deterministic projections verified across hosts; receipts replayable offline; no durable I/O bypasses CAM; grants enforced.

---

## 18) Repository Layout (Phase‑3 additions)
```
/krd/           # runtime daemon
/krd/vm/        # interpreter + JIT
/krd/sandbox/   # container/jail adapters, seccomp profiles
/krd/io/        # surface adapters (streams, kv, tables, ports)
/krd/thunks/    # thunk cache store
/runs/          # receipts, replay tools
/examples/      # sample projections and golden I/O vectors
```

---

## 19) Definition of Done (DoD)
- APIs and schemas versioned and published; CLI stable.
- Cross‑host deterministic run tests pass; behavior equivalence proven for interpreter vs thunk.
- Security checks complete (seccomp/jail, network off by default, secret handling).
- Observability dashboards show admission traces and I/O lineage; `replay` verifies any run offline.

