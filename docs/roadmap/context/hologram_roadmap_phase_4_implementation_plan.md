# Phase 4 Implementation Plan — Registry Peers, Developer Experience, Observability & Conformance (Public 1.0)

## 1) Objectives
- Operationalize a **registry‑peer** role (no central authority) that gossips bindings, optionally stores/serves content, and exposes catalogs for deterministic discovery.
- Deliver a production‑ready **Developer Experience (DX)**: CLI/SDKs, templates, samples, and CI actions for build/publish/resolve/pull/run.
- Provide first‑class **observability** driven by receipts (not logs): availability/freshness, name history, transfer health, release reproducibility.
- Publish a **conformance suite** and interop fixtures enabling independent implementations to certify **Public 1.0** compatibility.

## 2) Scope
**In scope:** registry peer service, catalogs & gossip, optional holding/serving, public API, auth/capability checks, DX tooling, dashboards, conformance program, documentation & samples.

**Out of scope:** economic settlement markets (only basic transfer volume fields), complex billing, advanced policy authoring UIs.

---

## 3) Registry Peer (rp) — Architecture & Responsibilities
```
+------------------------------+
| rp (registry peer)           |
|  - Namespace AuthZ           |
|  - Binding Accept/Publish    |
|  - Catalog Builder (Merkle)  |
|  - Gossip (Phase-2 cp)       |
|  - Optional Holder (CAM)     |
|  - Optional Server (TP)      |
|  - Metrics & Receipts        |
+------------------------------+
   ^         ^          ^
   |         |          |
  NRS     Control     Transfer
 (Phase-1)  (cpd)       (tpd)
```

### 3.1 Public API (HTTP/2 + JSON, backed by cp/tp for federation)
- **Namespaces**
  - `GET /v1/namespaces` → list namespaces hosted.
  - `GET /v1/namespaces/{ns}/bindings` → current bindings; supports `?tag=stable` and `?since=rev`.
  - `POST /v1/namespaces/{ns}/bindings` *(authz: publish cap)* → submit signed Binding; returns **BindingReceipt**.
- **Catalogs**
  - `GET /v1/catalogs/current` → Merkle root CID + summary.
  - `GET /v1/catalogs/{cid}` → content‑addressed catalog snapshot (pulls via transfer plane if not local).
- **Discovery**
  - `GET /v1/resolve?handle=...&policyRef=...` → deterministic resolve (mirrors Phase‑2 ResolveReceipt decision trace).
- **Peers**
  - `GET /v1/peers` → advertise cp/tp endpoints and capabilities.

### 3.2 Namespace Authorization
- Maintain a signer allowlist per namespace (policyRef); enforce capability `publish:namespace/ns` for `POST`.
- Key rotation: accept `{kid}` rollover windows; publish updated allowlists as policy packs.

### 3.3 Catalog Construction
- Incremental Merkle catalog (sorted by `namespace/handle/tag`); publish catalog CID at intervals or after threshold of changes.
- Catalogs are **content‑addressed** artifacts; peers fetch deltas via transfer plane.

### 3.4 Optional Holding/Serving
- If rp pins content: integrate with local CAM; emit **Placement** and **Transfer** receipts, advertise via `Provide`.
- Site/region labels for proximity; throttle according to capability ceilings (`serve.egress`, `pin.bytes`).

---

## 4) Developer Experience (DX)
### 4.1 CLI (holo)
- **Build & Pack**
  - `holo build` → compile to Atlas projection; `holo pack` for packs; `holo service new` templates.
- **Publish & Provide**
  - `holo publish <handle> --cid <cid> --policy <cid>` → emits BindingReceipt; rp gossips via cp.
  - `holo provide --cid <cid> --class hot --scope public --until ...` → rp optional; otherwise local cp.
- **Resolve, Pull, Run**
  - `holo resolve <handle>` → prints CID + decision trace; `holo pull --cid <cid>`; `holo run <cid|handle>` (Phase‑3 runtime).
- **Inspect & Audit**
  - `holo receipts verify|bundle-verify` ; `holo release show <handle>` shows `(digest, bindings)` set.
- **Keys & Caps**
  - `holo keys new/rotate/export` ; `holo cap mint --scope publish:namespace/... --ceilings ...`.

### 4.2 SDKs & Templates
- **Go SDK** (full) + **TS/Rust clients**; code‑gen from protobuf/gRPC + JSON schemas.
- Templates: *projection‑hello*, *pack‑dataset*, *service‑three‑stage*, *policy‑bundle*, *grant‑issuer*.
- GitHub Actions: `holo/setup`, `holo/build`, `holo/publish`, `holo/conform`.

### 4.3 Docs & Tutorials
- “From zero to release”: build → pack → publish → provide → resolve → run → audit.
- “User contexts & views”: issue/revoke grants; simulate a service consuming user data via views.
- “Reproducible releases”: roll forward/back via bindings; prove with receipts.

---

## 5) Observability (Receipts‑First)
### 5.1 Dashboards
- **Availability & Freshness**: placement map + scrub age heatmap by CID/class/scope.
- **Name History**: handle→digest timeline with binding signer sets; rollback points highlighted.
- **Transfer Health**: throughput, TTFB, resume/handoff counts, error taxonomies.
- **Release Reproducibility**: compare release digests/receipt sets across environments.

### 5.2 Data Sources
- Only **receipts** (Binding, Provide/Withdraw, Placement, Scrub, Transfer, Resolve, Run) and signed node reports.
- No proprietary logs required; logs are auxiliary.

### 5.3 Export & Audit
- `holo audit bundle --handles ... --cids ...` → compact, signed bundle with minimal receipts to replay decisions offline.
- Third‑party verifier CLI can re‑derive decisions and produce an **AuditReport** (signed) with pass/fail per obligation.

---

## 6) Conformance & Interop Program
### 6.1 Artifacts
- **Conformance Suite v1** with categories:
  1) Canonical encodings & CIDs (Phase‑0)
  2) CAM API & receipts (Phase‑1)
  3) Control/Transfer behaviors (Phase‑2)
  4) Runtime admission & RunReceipts (Phase‑3)
  5) Registry peer behaviors (Phase‑4)
- **Fixtures**: catalogs, bindings, policy packs, transfer traces, golden run vectors.

### 6.2 Runner & Reports
- `holo conform run --profile minimal|peer|holder|runtime|full` → executes tests, emits **SignedConformanceReport**.
- Public scoreboard (opt‑in) renders vendor/version vs. profiles; reports are linkable CIDs.

### 6.3 Profiles
- **minimal‑peer**: accept/publish bindings, expose catalogs, deterministic resolve.
- **holder‑peer**: minimal‑peer + pin/serve + receipts.
- **runtime‑node**: Phase‑3 compliance for run admission/I/O.
- **full‑node**: all of the above.

---

## 7) Security & Policy
- **AuthN/Z**: mTLS for peer links; OAuth2/OIDC or signed capabilities for public HTTP API mutations.
- **Rate‑limits**: caps on publish/provide/serve; all enforced via capabilities; violations produce denial receipts.
- **Namespace Integrity**: enforce signer allowlists; rapid revocation path; catalog diffs detect equivocation attempts.
- **Privacy**: public endpoints return only digests/bindings; no user payload exposure.

---

## 8) Performance Targets
- Resolve P50 < 30 ms from rp cache; P95 < 100 ms with remote policy fetch.
- Catalog diff publication < 2 s after 100 binding updates.
- Serve throughput: saturate 10 Gb/s per rp when acting as holder with NVMe backing; CPU overhead < 7% for receipt emission.

---

## 9) Testing Strategy
- **Unit**: API authz, signer allowlists, catalog Merkle construction, deterministic resolve trace equivalence.
- **Integration**: cross‑rp gossip; failover between peers; catalog conflict resolution by policy.
- **Load**: 1M bindings, 10k QPS resolve; catalog rotate every 5 min; ensure memory/CPU caps.
- **Security**: malicious bindings (bad signatures, replay, unauthorized namespaces); API abuse & rate‑limit tests.
- **End‑to‑end**: full release flow with receipts‑only audit replay.

---

## 10) Work Breakdown Structure (WBS)
1. **Registry Peer Service**
   - Namespace auth, binding acceptor, catalog builder, HTTP API, cp/tp plumbing.
2. **Optional Holder/Server**
   - CAM integration, provide/withdraw automation, egress accounting, receipts.
3. **DX Tooling**
   - CLI verbs, SDKs, templates, CI actions, examples.
4. **Observability**
   - Dashboards, metrics emitters, audit bundle exporter.
5. **Conformance**
   - Test harness, fixtures, profiles, signed reporting.
6. **Docs**
   - API reference, operator runbooks, developer tutorials, threat model.
7. **Security**
   - Capability enforcement, OIDC integration, rate‑limiters, signer rotation.

---

## 11) Milestones & Exit Criteria
**M1: Registry Peer MVP (Week 4)**
- Accept/publish bindings; catalog snapshotting; resolve API with decision traces.

**M2: Holder/Serving (Week 7)**
- Optional pin/serve; Placement/Transfer receipts; Provide/Withdraw automation.

**M3: DX & Docs (Week 9)**
- CLI/SDK templates; GitHub Actions; end‑to‑end tutorial.

**M4: Observability & Conformance (Week 12)**
- Dashboards live; audit bundles; conformance suite v1 public; two independent implementations pass **minimal‑peer**.

**Phase‑4 Exit (Public 1.0)**
- Interop demo across at least **two** independent stacks shows: publish → gossip → resolve → pull → run → audit via receipts only.
- Conformance suite green for **minimal‑peer** and **holder‑peer** profiles on reference rp.
- Docs complete; APIs versioned; semver tags and signed binaries released.

---

## 12) Repository Layout (Phase‑4 additions)
```
/rp/              # registry peer service
/rp/api/          # HTTP API definitions & handlers
/rp/catalog/      # Merkle catalog builder, snapshots
/dx/              # CLI templates, CI actions, examples
/dash/            # dashboards (receipts-driven)
/conformance/v1/  # test harness, fixtures, profiles
/docs/            # operator + developer docs
```

---

## 13) Definition of Done (DoD)
- Registry peer API stable and documented; mTLS + capability checks enforced.
- Catalogs are content‑addressed; diffs reproducible; peers converge under policy.
- DX path demonstrated with public samples and CI templates.
- Observability shows availability/freshness and name history from receipts; audit bundles reproduce decisions offline.
- Conformance suite v1 public with signed reference reports; two external implementations pass minimal‑peer profile.

