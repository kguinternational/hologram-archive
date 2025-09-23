# Phase 2 Implementation Plan — Control Plane, Transfer Plane, and Router (Federation Alpha)

## 1) Objectives
- Stand up a **federated control plane** carrying small, signed statements for publishing, availability, and access mediation.
- Implement a **digest‑addressed transfer plane** with in‑stream verification, resume, and provider handoff.
- Ship a **Router** that selects healthy providers, prefetches dependencies, and improves end‑to‑end latency without central coordination.
- Maintain strict separation from Phase‑1 (local‑only) concerns while reusing canonical encodings, CIDs, and receipt formats.

## 2) Scope
**In scope:**
- Signed control messages: `Publish`, `Provide`, `Withdraw`, `Resolve`, `Subscribe`, `Grant`, `Revoke`.
- Peering/gossip and catalog exchange; version negotiation and replay protection.
- Transfer protocol: range/resume, Merkle‑verified streaming, mid‑stream source handoff.
- Router policies: provider scoring, locality awareness, prefetch heuristics, failure backoff.
- Receipts for control/transfer actions; wiring into Phase‑1 receipt verifier.

**Out of scope:** kernel admission/execution (Phase‑3), registry UX, economic settlement beyond basic transfer volume fields.

---

## 3) Architecture Overview
```
+-----------------------------+         +-----------------------------+
| Node A                      |         | Node B                      |
|  - holo-camd (Phase-1)      |         |  - holo-camd                |
|  - holo-nrs (Phase-1)       |         |  - holo-nrs                  |
|  - cpd (control-plane)      |<------->|  - cpd (control-plane)      |
|  - tpd (transfer-plane)     |<------->|  - tpd (transfer-plane)     |
|  - router                   |         |  - router                   |
+-----------------------------+         +-----------------------------+
            ^   ^                                      ^   ^
            |   | gRPC (local)                         |   | gRPC (local)
            |   +-- CLI/SDK                            |   +-- CLI/SDK
            +------ Phase‑1 CAM/NRS APIs               +------ Phase‑1 CAM/NRS APIs
```

---

## 4) Control Plane Specification (v1)
### 4.1 Transport and Framing
- **Transport:** QUIC or TLS‑TCP with mTLS (node identity certs). Datagram support optional for gossip bursts.
- **Envelope:** COSE_Sign1 on canonical CBOR payloads.
- **Replay protection:** per‑peer sliding window with `(nonce, issuedAt)`; reject stale or duplicate `(kid, nonce)`.
- **Batching:** `GossipBundle` carrying N messages (max size configurable), each self‑signed; bundle itself unsigned.

### 4.2 Message Types (CBOR/JSON Schema outline)
Common fields: `{ msgId, kind, issuedAt, expiresAt?, kid, capabilityRef?, policyRef?, scope }`

- **Publish** (Namespace owner → network):
  ```json
  {"kind":"Publish","handle":"holo://org/app:stable","cid":"cid:...","policyRef":"cid:policy","revision":42}
  ```
  Effects: update binding cache; emit **BindingReceipt** (Phase‑1 family) locally.

- **Provide** (Holder → network):
  ```json
  {"kind":"Provide","cid":"cid:...","class":"hot|warm|cold","scope":"org|fed|public","until":"2026-01-01T00:00:00Z","site":"us-west/den1"}
  ```
  Effects: advertise availability; drives Router selection; emits **PlacementReceipt**.

- **Withdraw** (Holder → network): same as Provide with `withdraw:true`.

- **Resolve** (Client → peers): query for bindings/availability under `policyRef`; response is **ResolveReply** with `{binding, providers[]}`.

- **Subscribe** (Client → peers): long‑lived interest in `{cid|handle}`; peers push updates (new providers, withdrawals, revocations).

- **Grant** (User authority → providers/peers):
  ```json
  {"kind":"Grant","subject":"holo://user.alice/context","views":["profile.basic"],"aud":"holo://org/service","purpose":"personalize","notBefore":"...","notAfter":"...","ceilings":{"egressMB":100}}
  ```
  Effects: updates resolve decisions; future transfers must attach `grantRef`.

- **Revoke** (User authority → network): immediately invalidates matching Grants at resolve time.

### 4.3 Policies and Scopes
- **Scope values:** `local`, `org`, `federation`, `public` (forward‑compatible).
- **Policy enforcement order:** signature → capability → policyRef evaluation → time window → action.
- **Version negotiation:** peers advertise `cp/v1`, `tp/v1`; downgrade if compatible; otherwise refuse.

### 4.4 Catalog Exchange
- **Catalog** is a content‑addressed snapshot of `{namespace → bindings}` with a Merkle index for partial sync.
- Peers exchange catalog CIDs via `GossipBundle`; fetch deltas over transfer plane.

---

## 5) Transfer Plane Specification (v1)
### 5.1 Goals
- Fetch by **CID**, verify in‑stream, support **range**, **resume**, and **provider handoff** with zero trust of intermediaries.

### 5.2 Wire Protocol (over QUIC streams)
Frames (all length‑prefixed):
- `REQ_INIT {cid, wantRanges[], resumeToken?}`
- `RESP_META {size, merkleRoot, packId?, chunkTableHash}`
- `RESP_DATA {rangeStart, chunkIdx, chunkHash, chunkBytes, proof[]}`
- `RESP_EOF {ok, bytesSent, lastChunkIdx}`
- `ERR {code, detail}`

### 5.3 Verification
- Client validates each `RESP_DATA` chunk by hashing and verifying Merkle proof to `merkleRoot`; once complete, recomputes root and compares to CID preimage root (Phase‑0 derivation rules).

### 5.4 Resume & Handoff
- **Resume:** server issues `resumeToken` binding `{cid, lastVerifiedOffset, packId, serverNonce}`; client can reconnect with token and continue.
- **Handoff:** Router may switch providers mid‑transfer; client closes stream after last verified chunk boundary and reissues `REQ_INIT` with `lastVerifiedOffset` to a new provider; de‑dup via CAM prevents double‑write.

### 5.5 Flow Control and Congestion
- QUIC native; implement application‑level window hints (`REQ_TUNE {targetKBps, maxInFlightChunks}`) for high‑latency links.

### 5.6 Transfer Receipts
- **TransferReceipt(provider, client)**: `{cid, bytes, durationMs, fromSite, toSite, resumeCount, handoffs, grantRef?, pathEvidenceRef?}` signed by both sides (optional dual‑signature mode).

---

## 6) Router (v1)
### 6.1 Responsibilities
- **Provider selection:** choose providers for a given CID using multi‑factor scoring.
- **Prefetch:** stage declared dependencies (from manifests) and hot packs into local CAM.
- **Backoff:** adaptively avoid flapping providers; degrade gracefully.

### 6.2 Inputs
- Provide/Withdraw announcements; scrub freshness; historical TransferReceipts; local latency probes; site/region topology.

### 6.3 Scoring Function
```
score(p) = w1*latency^{-1} + w2*freshness + w3*successRate + w4*placementDiversity + w5*locality
```
- `freshness`: recency of ScrubReceipts.
- `successRate`: EWMA of completed transfers / attempts.
- `placementDiversity`: favor spreading over failure domains.
- `locality`: same site/region boost.
- Tunables `w1..w5` via policy; default safe values provided.

### 6.4 Prefetch Strategy
- Read manifests for `{deps[], views[]}`; prefetch within pre‑declared budgets.
- Sliding‑window prefetch keyed to pack chunk boundaries; avoid cache pollution by tracking observed re‑use counts.

### 6.5 Failure Handling
- Threshold‑based circuit breaker per provider.
- Handoff when `score(new) - score(current) > δ` for N consecutive probes.

---

## 7) Receipts (Phase‑2 additions)
- **ProvideReceipt/WithdrawReceipt**: evidence of availability announcements with `(class, scope, until, site)`.
- **ResolveReceipt**: `{handle|cid, policyRef, selectedCid, decisionTrace}` for audit of deterministic resolution across peers.
- **TransferReceipt**: as above; optional dual‑signature mode.
- **SubscribeReceipt**: start/stop of subscriptions (for observability).

All reuse COSE_Sign1 and Phase‑0 canonicalization.

---

## 8) Security & Trust
- mTLS between peers; node identity certs signed by org CA or self‑signed + policy trust roots.
- Capabilities: short‑lived tokens with ceilings (`publish`, `provide`, `serve`, `resolve`); bound to receipts.
- Grants: user‑scoped authorizations attached to Resolve/Transfer; revocations enforced at decision time.
- Anti‑replay: `(kid, nonce)` cache; time skew tolerance ±120s by policy.
- Privacy: only digests/handles/policy refs in control plane; payload bytes flow on transfer plane.

---

## 9) CLI/SDK (Phase‑2)
- `holo cp publish|provide|withdraw|subscribe|grant|revoke|gossip`.
- `holo tp fetch --cid <cid> [--range ...] [--resume] [--handoff]`.
- `holo router plan <cid|handle>` → shows scoring and chosen provider.
- SDKs (Go full; TS/Rust clients) expose streaming APIs and hooks for custom scoring.

---

## 10) Observability
- Live views: provider scorecards, scrub freshness heatmap, transfer throughput, handoff events.
- Audit views: deterministic resolve decision traces; name→digest timeline (from ResolveReceipts and BindingReceipts).
- Export: `holo report --federation` bundle with control/transfer receipts for third‑party verification.

---

## 11) Performance Targets
- Control message P50 < 25 ms intra‑region; < 120 ms inter‑region.
- Time‑to‑first‑byte after `tp fetch` < 150 ms intra‑region with hot provider.
- Goodput within 85–95% of link capacity for large sequential transfers; < 5% overhead for Merkle proofs.
- Handoff penalty < 250 ms when providers are pre‑probed.

---

## 12) Testing Strategy
- **Unit:** envelope signing/verification; nonce windows; policy evaluation; catalog Merkle proofs.
- **Integration:** multi‑node clusters with fault injection (packet loss, latency spikes, provider crashes).
- **Deterministic resolve tests:** cross‑peer identical decision traces under same policies.
- **Resume/handoff tests:** inject disconnects; verify byte‑exact continuity in CAM.
- **Soak:** long‑running gossip with 1M bindings and 10k providers; memory caps enforced.
- **Interoperability:** black‑box exchange between two independent implementations (Go ref + alt impl).

---

## 13) Work Breakdown Structure (WBS)
1. **Control Envelope & Transport**
   - COSE + mTLS plumbing; nonce cache; message codecs; QUIC/TLS listeners.
2. **Messages & Policies**
   - Implement Publish/Provide/Withdraw/Resolve/Subscribe/Grant/Revoke; decision trace object.
3. **Catalogs & Gossip**
   - Merkle catalog builder; delta sync; gossip scheduler; backpressure.
4. **Transfer Plane**
   - Wire protocol; Merkle proof paths; range/resume; handoff; benchmarks.
5. **Router**
   - Scoring model; probe engine; prefetch; backoff/circuit breaker; policy hooks.
6. **Receipts**
   - Provide/Resolve/Transfer/Subscribe receipts; verifier updates; dual‑sign mode.
7. **CLI/SDK**
   - Commands and client libs; streaming demos; docs.
8. **Observability**
   - Metrics emitters; dashboards; audit export.
9. **Security**
   - Capability service; grant attachment; revocation enforcement.
10. **Interoperability & Conformance**
   - Cross‑impl test plan; public vectors; signed reports.

---

## 14) Milestones & Exit Criteria
**M1: Control Plane MVP (Week 4)**
- Publish/Provide/Withdraw/Resolve implemented with signed envelopes and nonce windows; simple gossip works.

**M2: Transfer Plane MVP (Week 7)**
- Digest‑addressed fetch with range/resume and Merkle verification; TransferReceipts emitted.

**M3: Router v1 (Week 9)**
- Provider scoring and prefetch; handoff policy operational; circuit breaker prevents flapping.

**M4: Interop & Observability (Week 12)**
- Two implementations exchange catalogs and serve transfers; deterministic resolve parity; dashboards live; signed interop report.

**Phase‑2 Exit**
- Cross‑site resolve/pull succeeds with no shared central authority; handoff tested; receipts verify offline end‑to‑end.

---

## 15) Risks & Mitigations
- **Gossip storms / amplification** → bounded fan‑out, randomized timers, bloom filters for seen msgIds.
- **Clock skew** → tolerant windows + alerts; NTP enforcement.
- **Handoff thrash** → hysteresis in scoring; minimum dwell time; tie‑breakers.
- **Merkle proof overhead** → batch proofs across adjacent chunks; tune chunk size.
- **Policy divergence** → pin policy refs in receipts; decision traces for audit.

---

## 16) Repository Layout (Phase‑2 additions)
```
/cp/            # control-plane daemon (cpd), messages, gossip, policies
/tp/            # transfer-plane daemon (tpd), wire protocol, proofs
/router/        # provider scoring, prefetch, probes
/interop/       # test harnesses for multi-node clusters
/dash/          # metrics and dashboards
```

---

## 17) Definition of Done (DoD)
- Protocol specs documented with state machines and sequence diagrams.
- mTLS keys provisioned; capability/grant checks enforced for all impactful messages.
- Deterministic resolve yields identical decision traces across two peers under the same policy.
- Transfer plane passes resume/handoff torture tests; CAM stores byte‑identical content with verified Merkle roots.
- Interop demonstration published with signed report and reproducible scripts.

