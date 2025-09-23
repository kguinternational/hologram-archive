# Hologram Roadmap

## Introduction (Engineering-Focused)
Hologram is a portable, verifiable container ecosystem for software execution and data movement. The roadmap translates the system specification into shippable, testable software components using computer-science terminology and production engineering constraints. Our objective is to deliver a content‑addressed, policy‑driven platform where **the same content digest produces the same observable behavior** on any compliant runtime, names resolve deterministically via **signed bindings**, and all lifecycle actions generate **verifiable receipts** consumable by tools and auditors.

### Purpose
- Provide a concrete, phased plan that minimizes cross‑dependencies, enables parallel workstreams, and yields usable artifacts early.
- Define crisp interfaces, schemas, and conformance tests so independent implementations can interoperate without central coordination.

### Audience & Scope
- Software engineers building runtimes, registries, control/transfer services, and developer tools.
- Security and reliability engineers defining policies, capabilities, and verification pipelines.
- Excludes end‑user UX design, billing, and vendor‑specific integrations; focuses on core protocols, object formats, and APIs.

### Design Principles (CS/Software Terms)
- **Content-addressed identity:** Canonical bytes → cryptographic digest; digest equality implies semantic equivalence.
- **Deterministic naming:** Human handles resolve to a single digest under an explicit policy (signer sets, version/tag rules).
- **Separation of concerns:** Storage (CAM), naming (NRS), runtime admission/execution, and distribution (control/data planes) are modular with narrow, stable contracts.
- **Receipt-first observability:** Mutations emit signed receipts (storage, pin, placement, scrub, transfer, binding, run, consent/grant/revocation) verifiable offline.
- **Media-aware efficiency:** Packs enable dictionary/codec reuse and Merkle‑indexed chunking for deduplication and partial fetch.
- **Backwards-friendly federation:** Peers converge via signatures and policies; no central authority is required.

### Constraints
- No reliance on blockchains or specific TEEs.
- Host‑specific specialization must not change observable behavior.
- All binary/object encodings are canonical and reproducible; builders must be deterministic.

### Success Criteria
- Reproducible builds and runs across heterogeneous hosts (mobile/embedded/PC/server).
- Deterministic resolution and cross‑registry peering without shared control.
- End‑to‑end verification: any action’s receipt can be validated by a third party offline.

---

## Roadmap Outline (Phased Delivery)

### Phase 0 — Object Model & Canonical Encodings (Foundations)
**Goals:** Establish canonical object formats and digests; lock schemas and invariants for interop.
- **Atlas object format:** Canonical serialization for projections, packs, services; digest derivation rules.
- **Schemas:** JSON/CBOR schemas for manifests and receipts with strict validation.
- **Reference tools:**
  - `atlas-encode` / `atlas-verify` (deterministic builder + verifier)
  - Fixture sets with golden digests for CI across platforms
- **Exit:** Identical inputs → identical bytes/digest on Linux/macOS/Windows CI runners.

### Phase 1 — Content-Addressed Memory (CAM) & Receipts (Local MVP)
**Goals:** Store and serve bytes by digest with verifiable retention and integrity.
- **CAM API:** `put`, `get`, `link`, `stat`, `pin(class, scope, ttl)`, `gc` with Merkle pack indexing.
- **Receipts:** Storage, pin, placement, scrub receipts; signing and offline verification CLI.
- **Name Resolution System (NRS):** Signed **binding** statements (handle → digest), deterministic resolve, rollback via re‑binding.
- **CLI/SDK:** `holo put/pin/resolve/pull` backed by local daemon.
- **Exit:** Deterministic `resolve` and authenticated `pull` into CAM with receipts validated by `receipts verify`.

### Phase 2 — Control Plane & Transfer Plane (Federation Alpha)
**Goals:** Gossip small signed statements; move data by digest with in‑stream verification.
- **Control plane:** `Publish`, `Provide/Withdraw`, `Resolve/Subscribe`, `Grant/Revoke` (user views). All messages signed; replay protection; capability/grant attachment.
- **Transfer plane:** Range/resume streaming, provider handoff, flow control; on‑the‑fly Merkle verification; transfer receipts (served/received volumes optional).
- **Router:** Provider selection by latency, scrub freshness, recent success; prefetch declared dependencies.
- **Exit:** Cross‑site resolve/pull works between independent peers with no shared authority; all effects evidenced by receipts.

### Phase 3 — Kernel Runtime (Portable Execution Beta)
**Goals:** Admit and execute projections uniformly across hosts.
- **Admission checks:** Digest, bindings, policy, capabilities; sandbox/profile compatibility.
- **Execution model:** Declared I/O surfaces (streams, kv/table views, message ports); lifecycle signals; runtime never bypasses CAM for durable I/O.
- **Host specialization:** Optional JIT/bind “thunks” keyed by `(digest, host-profile)`; behavior invariance enforced.
- **Run receipts:** Entrypoint, input/output digests, exit status, environment profile.
- **Exit:** Same digest → same observable behavior on ARM64/x86_64 under identical inputs; receipts reproduce lineage.

### Phase 4 — Registry Peers, Developer Experience, Observability (Public 1.0)
**Goals:** Make federation practical and auditable; smooth developer workflows.
- **Registry peer:** Catalog/gossip bindings; optional holding/serving with placement/transfer receipts; multi‑homing and proximity‑aware discovery.
- **DX:** CLI/SDK verbs (`build`, `pack`, `publish`, `provide`, `resolve`, `pull`, `run`, `inspect`), local CAM daemon, test vectors and conformance suite.
- **Observability:** Name history, availability/freshness dashboards derived from receipts; audit bundle export for third‑party verification.
- **Exit:** Interop demo across two independent implementations; conformance suite green.

---

## Milestone Artifacts per Phase (Checklists)
- **Phase 0:** Canonical encoding spec; golden fixtures; cross‑platform digest parity report.
- **Phase 1:** CAM daemon; receipt signer/validator; NRS resolver; local CLI.
- **Phase 2:** Control/transfer services; router; federation smoke tests; cross‑peer receipts.
- **Phase 3:** Kernel runtime; I/O adapters; run receipts; behavior‑equivalence tests.
- **Phase 4:** Registry peer; conformance suite; dashboards; developer docs and samples.

