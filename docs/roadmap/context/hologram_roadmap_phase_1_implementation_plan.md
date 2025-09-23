# Phase 1 Implementation Plan — Content‑Addressed Memory (CAM), Receipts, and Name Resolution (NRS)

## 1) Objectives
- Ship a local-first storage substrate that serves bytes **by content digest** with verifiable persistence and integrity.
- Implement a deterministic **Name Resolution System (NRS)** based on signed **bindings** (handle → digest under policy).
- Establish a receipt framework (sign/verify, offline appraisal) for all Phase‑1 mutations: storage, pin, placement, scrub, binding.
- Provide a production‑grade CLI/SDK and daemon suitable for desktop/server/embedded targets.

## 2) Scope (Phase‑1)
**In scope**: CAM daemon + API, Merkle pack format, pin classes + GC, scrub/repair loop, NRS resolver + binding publisher, receipt schemas + signer/validator, CLI/SDKs, local observability, initial security hardening.

**Out of scope**: network control/transfer plane, cross‑site peering, runtime execution (admission/run receipts), user‑context grants (policy stubs only).

---

## 3) Architecture Overview
```
+--------------------------+
| holo-camd (daemon)       |
|  - Pack Store            |
|  - Object Index (CID→loc)|
|  - Pinset/GC Manager     |
|  - Scrub/Repair Worker   |
|  - Receipt Emitter       |
+-----------+--------------+
            | gRPC/UDS API
            v
        CLI / SDKs

+--------------------------+
| holo-nrs (resolver)      |
|  - Policy Engine (v0)    |
|  - Binding Verifier      |
|  - Catalog Cache         |
|  - Receipt Emitter       |
+--------------------------+
```

---

## 4) APIs
### 4.1 CAM gRPC/UDS API (v1)
- `Put(Blob) → {cid, StorageReceipt}`
- `Get({cid, range?}) → bytes`
- `Link({parentCid, childCid, role}) → {LinkReceipt}`
- `Stat({cid}) → {size, kind, merkleRoot, packId?, refs[]}`
- `Pin({cid, class, scope, ttl}) → {PinReceipt}`
- `Unpin({cid}) → {UnpinReceipt}`
- `GC({budget}) → {GcReportReceipt}`
- `Scrub({cid|selector, sampleRate}) → {ScrubReceipt}`
- `Place({cid, policy}) → {PlacementReceipt}`  *(local replication/EC within device tiers)*
- `Verify({cid}) → {ok, report}` *(re‑walk Merkle checks without mutating)*

**Error model**: gRPC codes + structured fields (`code`, `rule`, `detail`, `cid`).

### 4.2 NRS API (v1)
- `Resolve({handle, policyRef?}) → {cid, usedBinding}`
- `PublishBinding({handle, cid, policyRef, expiresAt}) → {BindingReceipt}` *(requires signer)*
- `ValidateBinding({bindingBytes}) → {ok, signer, policy}`
- `GetHistory({handle}) → {bindings[]}`

### 4.3 Receipt Verification API (lib)
- `VerifyReceipt(bytes) → {ok, subjectCid, kind, signer, evidenceRefs[]}`
- `BundleVerify(dir|zip) → SignedReport`

---

## 5) Storage Model
### 5.1 Pack Layout
- **Content‑defined chunking** (CDC) with rolling hash (e.g., Gear/Rabin) targeting mean chunk size 64–256 KiB.
- **Merkle Index**: binary Merkle tree over chunks; root hash equals content CID preimage root (domain‑separated).
- **Pack File**: header + chunk table + data region; supports random range reads; reference counts per chunk within pack.
- **Dedup**: identical chunks across packs are single‑instanced in the chunk store; pack index maps logical→physical.

### 5.2 Tiering
- **RAM cache** (hot), **device‑local SSD** (warm), **spinning/remote** (cold). Class semantics drive pin placement.
- **Selectors**: allow pinning a whole object or a sub‑DAG by label (e.g., `deps`, `payload` only).

### 5.3 Pin & GC
- **Pin classes**: `hot`, `warm`, `cold` with target latency/SLOs and minimum scrub cadence.
- **Scope** (Phase‑1 local only): `local` device; `site` reserved.
- **GC policy**: mark‑sweep over pinsets + live bindings; only unpinned and unreferenced chunks are eligible.

### 5.4 Scrub & Repair
- **Scrub** reads chunks by schedule and recomputes Merkle paths.
- **Repair** in Phase‑1 is local (re‑hydrate from redundant local copies if EC enabled). Remote repair deferred to Phase‑2.

---

## 6) Receipts (Phase‑1 Set)
Common envelope fields: `{ kind, subjectCid, time, signerKid, capabilityRef?, policyRef?, evidenceRef?, scope, class?, meta{} }`

- **StorageReceipt**: `{ size, merkleRoot, packId }`
- **PinReceipt**: `{ class, scope, ttl, pinId }`
- **PlacementReceipt**: `{ strategy: "replica"|"ec", tiers[], copies, ecParams? }`
- **ScrubReceipt**: `{ sampleRate, lastGood, sampleSetHash }`
- **BindingReceipt**: `{ handle, cid, policyRef, created, expires }`
- **LinkReceipt**: `{ parentCid, childCid, role }`
- **GcReportReceipt**: `{ freedBytes, objects, chunks }`

Signing: COSE_Sign1 (Ed25519). All receipts verifiable offline via `holo receipts verify`.

---

## 7) Naming & Policy (NRS)
- **Handle syntax**: `holo://<namespace>/<path>:<tag>`; tags map to either semantic versions or policy‑defined selectors.
- **Bindings**: signed statements referencing `policyRef`; deterministic resolve → single `cid` or **none**.
- **Policies (v0)**: signer allowlist, tag resolution rule (e.g., semver max, timestamp max), optional residency constraints (ignored in Phase‑1 local mode but preserved in receipts).
- **Rollback**: publish a new binding targeting a prior digest; history retained via receipts.

---

## 8) CLI/SDK
**CLI verbs**
- `holo cam put|get|stat|pin|unpin|gc|scrub|verify`
- `holo nrs resolve|publish|history|verify-binding`
- `holo receipts verify|bundle-verify`

**SDKs**
- Go (full), TypeScript (client), Rust (client). Shared protobuf/gRPC definitions and codegen.

---

## 9) Observability
- Local web UI (`holo ui`) or TUI: pin coverage by class, scrub age histogram, pack/byte heatmap, name→digest history.
- Export: `holo report` emits machine‑readable JSON summaries signed by the node.

---

## 10) Security Hardening (Phase‑1)
- **Canonicalization gate**: all mutable inputs pass Phase‑0 canonicalizer before entering stores.
- **Key handling**: in‑daemon keyring with passphrase‑encrypted private keys; external HSM interfaces optional.
- **Replay windows** for bindings; monotonic clock check; enforce `notBefore/notAfter`.
- **Resource limits**: per‑request byte ceilings; pack file size caps; streaming verification to bound memory.
- **Filesystem safety**: atomic fsync’ed writes (write‑ahead log), crash‑safe pack commit.

---

## 11) Performance Targets
- TTFB under 5ms for hot RAM‑cached chunks; under 20ms for warm SSD; sustained read ≥ 1 GB/s per NVMe where available.
- CDC throughput ≥ 300 MB/s per core; Merkle build ≥ 500 MB/s per core.
- Scrub cadence configurable; overhead < 5% of disk bandwidth at default sampleRate=0.02.

---

## 12) Testing Strategy
- **Unit tests**: chunker boundaries; Merkle proofs; CID derivation; schema validation.
- **Property tests**: re‑encode/verify idempotence; CDC boundary invariants; GC safety (no live chunk reclaimed).
- **Fault injection**: torn‑write simulation; bit‑flip corruption; clock skew; key rotation.
- **Golden vectors**: reuse Phase‑0 fixtures + new pack corpora.
- **Soak tests**: 10M object ingest with mixed sizes; periodic GC/scrub; power‑cycle stress.

---

## 13) Work Breakdown Structure (WBS)
1. **Pack Store & Merkle Index**
   - CDC library → Pack writer/reader → Merkle builder → Range/read API → Benchmarks.
2. **Index & Metadata**
   - CID→Pack/offset index; refcounts; link graph; stat surfaces.
3. **Pin/GC**
   - Pin registry; eviction policies; mark‑sweep GC; selectors.
4. **Scrub/Repair**
   - Scheduler; verifier; sample sets; report + receipt emission.
5. **Receipt Framework**
   - COSE signing; verify tool; bundle reports.
6. **NRS**
   - Policy engine v0; binding signer/verifier; resolve decision trace; history.
7. **CLI/SDKs**
   - gRPC definitions; Go client; TS/Rust clients; UX polish.
8. **Observability**
   - Metrics; TUI/web; report exporter.
9. **Security**
   - Keyring; replay windows; caps/limits; fs safety.
10. **Docs**
   - API docs; operational runbooks; troubleshooting.

---

## 14) Milestones & Exit Criteria
**M1: Local CAM MVP (Week 4)**
- Put/Get/Stat/Pin/GC working; Storage/Pin receipts signed and verified.
- CDC+Merkle pack format stable; benchmarks published.

**M2: Scrub/Placement (Week 6)**
- Scrub scheduler + receipts; local placement strategies; Verify API.

**M3: NRS Deterministic Resolve (Week 8)**
- Resolve and PublishBinding; policy allowlist; deterministic decision trace; Binding receipts.

**M4: Conformance & Observability (Week 10)**
- Conformance suite green; digest/receipt parity across OS/arch; local UI ready; signed node report.

**Phase‑1 Exit**
- Deterministic `resolve` → `cid` across two independent nodes using identical policy.
- `pull` from local daemon restores full object and verifies all Merkle paths.
- Pin coverage and scrub freshness visible; receipts verify offline; GC proven safe via invariants.

---

## 15) Risks & Mitigations
- **CDC instability across builds** → lock parameters; property tests across compilers/arches.
- **GC race conditions** → two‑phase commit for chunk reclamation; refcount snapshots.
- **Clock issues** (bindings validity) → monotonic clock + NTP sanity checks; wide default validity windows.
- **Key management complexity** → provide `holo keys` helper; support external signer plugin.

---

## 16) Repository Layout (Phase‑1 additions)
```
/cam/          # daemon + store + APIs
/nrs/          # resolver + policy engine + binding tools
/receipts/     # envelope types + signer/validator
/cli/          # holo subcommands
/sdk/go|ts|rs/ # clients
/ui/           # local observability (optional TUI/web)
```

---

## 17) Definition of Done (DoD)
- APIs frozen at v1 with semver tags; protobuf/gRPC files published.
- CLI stable; manpages generated; JSON outputs documented.
- All receipts round‑trip in `bundle-verify`; third‑party can appraise without running daemon.
- Performance SLOs met on reference hardware.
- Security checklist cleared (replay windows, fs safety, canonicalization gate).

