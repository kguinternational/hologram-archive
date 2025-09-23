# Hologram: A Portable, Verifiable Container Ecosystem

**Publisher:** UOR Foundation (https://uor.foundation)

---

## 1. Front Matter

**Title:** Hologram: A Portable, Verifiable Container Ecosystem  
**Authors:** Generator Architecture Working Group, UOR Foundation  
**Affiliations:** UOR Foundation  
**Abstract:**  
Hologram is a container ecosystem built around content-addressed memory, deterministic naming, and a receipt-first distribution model. It introduces three fundamental publishable units—projections (runnable units), packs (reusable bundles), and services (deployable compositions)—and a set of cooperating subsystems: an Atlas bytecode format, a kernel runtime, a content-addressed memory layer, a name resolution system, a minimal control plane, a verifiable transfer plane, and a receipt and capability framework. Together they provide portable execution (“same digest, same behavior”), decentralized distribution (cross-registry peering without a central authority), and verifiable persistence (pins and scrubs with auditable receipts). This document specifies Hologram’s concepts, contracts, and interfaces from a system-builder’s perspective, separating the minimal core from optional ecosystem layers.

**Keywords:** container, content-addressed memory, deterministic naming, federated registry, verifiable distribution, receipts, capabilities

---

## 2. Introduction

Modern container systems improved software packaging and isolation but left three problems unresolved at internet scale: (1) true portability across heterogeneous hosts and runtimes, (2) deterministic naming across registries without central authorities, and (3) verifiable persistence and lineage of the bytes that applications rely on. Hologram addresses these problems by treating content identity—not location or publisher—as the ground truth; by separating names from storage through signed bindings and policies; and by turning every meaningful action in the lifecycle into a small, verifiable record.

Hologram is an information-system architecture, not a monolith. It standardizes a canonical object format (Atlas bytecode), a runtime that can admit and execute it in a host-agnostic way, a content-addressed memory layer (CAM), a name resolution system (NRS), a minimal control plane for publishing, availability, and policy exchange, a simple but robust transfer plane, and a uniform receipt framework for storage, naming, movement, and execution.

**Scope.** This paper defines the concepts and contracts required to build a Hologram container ecosystem: publishable units and manifests; storage and naming rules; runtime admission and execution semantics; registry peering; control/transfer protocols; identity, capabilities, and receipts. It does not prescribe user interfaces, dashboards, billing models, or particular cryptographic parameter choices beyond required properties.

**Non-goals.** Hologram does not depend on or extend existing container/OCI semantics; it does not assume a central authority; it does not require blockchains, tokens, or specific TEEs; and it does not redefine operating systems. It complements existing infrastructure by providing a portable substrate and a minimal set of verifiable contracts.

---

## 3. Design Goals and Principles

**Portability.** Same content ID (digest) yields the same behavior under any compliant runtime. Host-specific specialization is an implementation detail that must not alter observable outcomes.

**Deterministic naming.** Human-readable handles always resolve to exactly one digest (or none) under a declared policy. The resolve decision is reproducible and explainable by the binding records consulted.

**Separation of concerns.** Storage, naming, execution, and distribution are independent subsystems with narrow contracts. Implementations may vary, but interfaces and receipts must not.

**Receipt-first.** Every state transition—publishing, storing, pinning, placing, scrubbing, transferring, viewing, running—yields a signed record that a stranger can verify offline. Logs are optional; receipts are authoritative.

**Media-aware efficiency.** Reusable decoder and dictionary bundles are first-class packs. Payloads transmit residual information relative to shared packs, reducing bandwidth and storage while preserving lossless semantics where required.

**Backwards-friendly adoption.** Peering and distribution work without central enrollment. Gateways, registries, and devices can join by pinning a small bootstrap set and speaking the control/transfer protocols.

---

## 4. Core Concepts and Terminology

**Projection.** A runnable unit compiled to Atlas bytecode. A projection declares its entrypoint, inputs/outputs, compatibility profile, and dependencies. Projections are typically published by a **publisher** (an organization/team) into the publisher’s namespace.

**Pack.** A reusable typed bundle addressed by digest. Examples include dataset packs, dictionary packs, interface packs, UI packs, policy packs, and codec packs. Packs can be published by publishers **or** by users (e.g., a user’s profile pack).

**Service.** A deployable composition of one or more projections wired together with declared state forms and lifecycle hints. Services are published by publishers; they **consume** user data via grants rather than owning user data.

**Handle.** A human-readable name within a namespace (e.g., `holo://org/product:stable` or `holo://user.alice/profile:current`).

**Digest.** The content ID derived from canonical encoding of the object. Digest equality implies byte equality and semantic equivalence.

**Binding.** A signed statement that a handle refers to a specific digest under a named policy. Bindings exist both for publisher-owned objects (projections, services, publisher packs) and for **user-owned objects** (user context packs and views).

**Capability.** A short-lived, scoped token authorizing specific publisher-side actions (publish, store, fetch, serve) with ceilings and expiry.

**Access grant.** A user-scoped authorization allowing a relying party (service/provider/peer user) to access specific **views** of user-owned packs for defined purposes and durations.

**Receipt.** A signed, verifiable record of an action (e.g., storage, pin, placement, scrub, transfer, view, run, settlement). Receipts reference digests, capabilities/grants, policies, and times.

**Pin.** A retention lease promising that a digest (or pack) will remain stored at a specified class and scope until a given time.

**User context.** A user-controlled namespace and storage scope for personal state (profiles, settings, messages, media, keys). Users publish their own data packs and bind handles within this context.

**Exposure policy.** A user-authored policy describing what parts of a user context may be disclosed, to whom, for what duration and purpose.

**Local cache vs global context.** Local cache is what a device/site can serve immediately. Global context is the federation of bindings, pins, placements, and policies known through peering.

**Scope.** The dissemination domain for control messages and storage obligations: local, organization, federation, or public.

---

## 5. System Overview

**Two-ring ownership (publisher vs user).** Hologram distinguishes between publisher-owned content (projections, services, publisher packs) and user-owned content (user context packs and views). To the platform, a published app is **publisher data**. To the app, a user’s records are **user data** that live in the user’s context and are accessed only via user grants. This division is enforced by naming (separate namespaces), by capabilities/grants, and by receipts.

Hologram organizes the container lifecycle into distinct phases linked by receipts.

Hologram organizes the container lifecycle into distinct phases linked by receipts.

**Build.** A developer compiles a projection or prepares packs/services. The Atlas bytecode system produces canonical objects with digests.

**Publish.** A publisher issues a binding for a handle to a digest under policy. The binding is signed and gossiped to peers.

**Distribute.** Holders pin content, place replicas or shards, and advertise availability. Availability and integrity are evidenced by placement and scrub receipts.

**Fetch.** A resolver deterministically maps a handle to a digest using policy and available bindings. A router discovers nearby holders, and the transfer plane streams verifiable chunks into CAM.

**Run.** The kernel admits a projection by verifying policy and integrity. The runtime executes, interacts with declared I/O surfaces, and emits run records.

**Update/Retire.** New bindings shift handles to new digests; old content is retained until pins expire and policies allow garbage collection. History is reproducible from receipts.

The control plane carries small signed statements (publish, provide, resolve, subscribe, gossip). The data plane transports bytes by digest with in-stream verification. Receipts connect the phases and preserve auditability.

---

## 5a. Multi-Device Substrate

Hologram is inherently multi-device. The same content IDs, manifests, receipts, and control/transfer protocols operate uniformly across mobile, embedded, PC, server, and network equipment.

**Uniform semantics.** Digests, bindings, pins, and receipts have identical meaning on all device classes. A projection admitted on a handset or a rack server behaves identically by specification.

**Host profiles.** Devices advertise concise host profiles (CPU/GPU/accelerator features, storage tiers, network locality). The kernel may specialize tiny thunks per profile without changing observable behavior.

**Tiered CAM.** Embedded nodes emphasize small hot caches and delegated pins; PCs and servers hold larger warm/cold packs; network devices (gateways, routers) excel at hot pins and low-latency serving for nearby peers.

**Sync by digest.** Multi-device consistency is name→digest determinism plus receipt exchange. New or offline devices converge by resolving handles, fetching missing digests from healthy providers, and replaying receipts—no device is privileged.

**Policy portability.** User and org policies (signer sets, exposure rules, pin minima) are content objects; devices of all classes enforce the same policies from the same digests.

---

## 6. Atlas Bytecode System (host-agnostic object format)

The bytecode system provides a canonical representation for all publishable objects—projections, packs, and services—independent of host architecture. It defines:

**Canonical encoding.** A strict on-wire format for headers, sections, and payloads that admits a single encoding per semantic object. Canonicalization ensures that equivalent objects yield the same digest.

**Sections and versioning.** Each object organizes content into typed sections (metadata, dependencies, declared I/O, code, data). Sections are independently versioned and extensible while preserving canonical digest rules.

**Determinism.** Builders must produce identical bytes given identical inputs (code, packs, configuration). Non-determinism (timestamps, random seeds) is prohibited or isolated into declared inputs.

**Content ID derivation.** Digests are computed over canonical bytes. Any change to semantics must alter the digest. This property powers deduplication, cross-registry referencing, and offline verification.

---

## 7. Content-Addressed Memory (CAM)

CAM is the storage substrate that serves bytes by content ID with integrity and retention guarantees.

**Node model.** Each stored unit is a node with a digest, type, size, and optional schema hash. Nodes can reference other nodes, forming a dependency DAG.

**Chunk/pack layout.** Large objects are subdivided into content-defined chunks and grouped into packs. Packs include a Merkle index for efficient verification and partial fetch/resume.

**Cache hierarchy.** Implementations may expose RAM, device-local, site-local, and geo-distributed tiers. Mobile/embedded devices prioritize small L-hot caches and delegated pins; PCs/servers provide larger warm/cold capacity; network gear (gateways, routers) emphasize L-hot serving for nearby peers. Movement between tiers is policy-driven and recorded by receipts.** Implementations may expose RAM, device-local, site-local, and geo-distributed tiers. Movement between tiers is policy-driven and recorded by receipts.

**Pins and garbage collection.** Pins keep content resident for a duration and class (hot, warm, cold). Garbage collection removes only content with no live bindings and no live pins.

**Placement and integrity.** Replication and erasure coding strategies are documented by placement receipts. Periodic scrubs verify integrity and produce scrub receipts. Repairs are recorded explicitly.

**API surface.** Minimal calls include `put`, `get`, `link`, `stat`, `pin`, and `gc`. All mutating calls return receipts suitable for gossip and audit.

---

## 8. Hologram Kernel Runtime

The kernel runtime ensures that the same digest runs the same way everywhere.

**Admission.** Before execution, the runtime verifies the object’s digest, required bindings, and required policies. Admission checks include capability validation and integrity proofs.

**Execution model.** A projection starts at its declared entrypoint. It interacts with declared I/O surfaces (streams, key/value, table views, message ports) and obeys lifecycle signals (start, health, shutdown).

**Host specialization.** Implementations may JIT or bind small host-specific thunks for performance per host profile (mobile, embedded, PC, server, network). Specialization must not change observable behavior. Caches of host-specific thunks are keyed by digest + host profile.** Implementations may JIT or bind small host-specific thunks for performance. Specialization must not change observable behavior. Caches of host-specific thunks are keyed by digest + host profile.

**Run records.** Each admitted execution emits a run receipt summarizing entrypoint, input references, output references, and exit status.

**Integration.** The runtime cooperates with the Router for prefetching and with CAM for verified I/O. It never bypasses CAM for reads or writes that affect durable state.

---

## 9. Manifests: Projections, Packs, and Services

This section defines **what is published**, **by whom**, and **where it lives**.

**Publishing contexts.** There are two:
- **Publisher context** — the organization’s namespace. Projections, services, and publisher-owned packs are bound here (e.g., `holo://acme/payments:1.4`). Pins and availability for these are principally the publisher’s responsibility.
- **User context** — the individual’s namespace. User records, settings, messages, and other personal packs are bound here (e.g., `holo://user.alice/payments/2025/09/…`). Pins and availability are principally the user’s responsibility (often co-funded by relying services via per-transfer commitments).

**Projection manifest.** Declares entrypoint, required packs, optional compatible projections for extension, declared inputs/outputs and their schemas, resource hints (CPU/memory/I/O), and compatibility profile. Projections are **published in the publisher context**. Durable I/O targets **user context views** when operating on user data.

**Pack manifest.** Declares kind (dataset, dictionary, interface, UI, policy, codec, etc.), schema or interface hashes, version, and compatibility matrix. Packs may be published in the **publisher context** (shared dictionaries/codecs) or in a **user context** (user profile, message ledger). User packs should expose **views** for selective disclosure.

**Service manifest.** Describes a deployable composition: participating projections, wiring between their interfaces, declared state forms (which are packs when materialized), health/readiness conditions, and rollout strategies. Services are published in the **publisher context** and are designed to **consume** user views under access grants rather than internalizing user data.

**Dependency graph.** All manifests reference dependencies by digest. Resolution is deterministic. Reuse is automatic: identical dependencies across publishers and users share the same digests and storage. Crucially, dependencies that are **user-owned** are referenced as **view addresses** plus grant requirements, not as publisher-owned storage.** All manifests reference dependencies by digest. Resolution is deterministic. Reuse is automatic: identical dependencies across publishers share the same digests and storage.

---

## 10. Naming and Policy (Name Resolution System)

The naming system separates human names from storage locations through signed bindings and resolver policies.

**Handle syntax.** Names live in namespaces controlled by organizations. A handle may include tags or semantic versions.

**Bindings.** A binding associates a handle with a digest under a named policy. It is signed by keys authorized for the namespace.

**Resolver policy.** Policies define signer allowlists, tag resolution rules, and optional residency constraints. Resolution returns exactly one digest (or none) and the specific binding used.

**Conflict handling.** If multiple bindings exist, the resolver selects one per policy and records the decision. History of changes is preserved by the sequence of bindings.

**Rollback.** Reverting a handle to a prior digest is accomplished by publishing a new binding that points back to the earlier digest. Reproducibility follows from digest identity.

---

## 11. Control-Plane Protocol

The control plane carries small, signed statements to coordinate publishing, availability, persistence, and user-mediated data sharing. It intentionally does not carry payloads.

**Publish.** Introduces or updates a binding for a handle→digest under policy.

**Provide/Withdraw.** Announces that a peer is storing a digest at a certain class and scope until a given time; withdraws after expiry or policy change.

**Resolve/Subscribe.** Resolves handles to digests and subscribes to availability for specific digests within a scope.

**Grant/Revoke.** A user issues or revokes an access grant over specific views of their user context to a relying party. Grants include scope, purpose, duration, and ceilings. Revocations propagate like other control statements and take effect at resolve time.

**Gossip.** Batches of receipts (bindings, pins, placements, scrubs, transfers, grants, revocations) that peers exchange to converge state.

**Persistence commitments.** A small envelope may accompany a transfer request to commit to retention of the digests being moved (class, scope, duration, sponsor ceilings). Providers convert commitments into pin and placement receipts.

**Error and retry.** Messages include replay protection, expiry, and backoff guidance. Failure reasons are explicit (unauthorized, policy conflict, stale, limit exceeded).

**Security and attestation.** Every message is signed and may carry a fresh nonce to bind replies. Signed control-plane statements are in-band attestations of current state (e.g., availability), and can be linked from receipts. Capability and grant references bound to messages constrain authorized effects and attach to resulting receipts. Profiles can require additional out-of-band evidence (e.g., device quotes) to accompany specific messages.** Every message is signed. Capability and grant references bound to messages constrain authorized effects and attach to the resulting receipts.

---

## 12. Transfer/Data Plane

The transfer plane moves content by digest with integrity verification during streaming.

**Digest-addressed transfer.** Clients request content by digest. Providers stream chunks and Merkle proofs. Clients verify on the fly and accept into CAM upon completion.

**Framing and resume.** Chunk framing supports range requests, partial verification, and resume after interruption. Integrity protects both chunk contents and placement in the overall object.

**Windowing and retries.** Implementations may tune flow control and retry strategies; correctness relies solely on digest and Merkle verification, not on transport trust.

**Provider handoff.** Clients can switch providers mid-stream if a healthier source is identified. Verification continues seamlessly.

---

## 13. Receipts and Verification

Receipts are the durable, verifiable evidence of system behavior. They enable audit without logs and cross-party trust without central authorities.

**Storage receipt.** Evidence that a node with a specific digest has been stored in CAM and is available locally.

**Binding receipt.** The signed binding for handle→digest under policy; includes signer identities and timestamps.

**Pin receipt.** A retention lease for a digest or pack, stating class, scope, and expiry.

**Placement receipt.** Details of how a digest or pack is replicated or erasure-coded across failure domains.

**Scrub receipt.** Evidence that stored content was re-read and verified recently; includes method and sample rate where applicable.

**Transfer receipt.** Proof that a provider served, and a client received, content by digest (and optionally volume for settlement).

**View receipt.** Evidence that a declared view over stored content was materialized correctly.

**Run receipt.** Summary of a projection execution: entrypoint, input/output references, exit status, and environment profile.

**Consent receipt.** Evidence that a user issued an access grant: subject context, grant scope (views/handles), relying party identity, purpose, duration, ceilings, and revocation terms.

**Grant receipt.** A machine-readable grant token bound to a consent receipt that authorizes access; includes capability linkage and expiry.

**Revocation receipt.** Evidence that a prior grant has been revoked; resolvers and providers must enforce revocation at decision time.

**Settlement-related receipts.** Conversion (barter→credits), extension (lease extension), delegation (obligation transfer), and handoff (seamless provider change) records.

**Attestation references.** Any receipt may carry references to external or in-band evidence (device/runtime/storage/transfer attestations). Evidence references bind nonces, timestamps, and attester identities to the action being recorded so appraisers can evaluate trust. Control-plane statements themselves are a form of in-band attestation when signed by trusted keys and tied to fresh nonces.

**Verification.** All receipts are signed and include necessary references (digests, capabilities, grants, policies, optional evidence). Verifiers can check signatures, replay decisions, validate consistency, and evaluate attached evidence offline.

**Retention and compaction.** Systems may compact receipts after agreed horizons (e.g., aggregate transfer receipts), preserving the ability to audit critical obligations (bindings, pins, placements, scrubs, grants, revocations, essential evidence links) indefinitely.** All receipts are signed and include necessary references (digests, capabilities, grants, policies). Verifiers can check signatures, replay decisions, and validate consistency offline.

**Retention and compaction.** Systems may compact receipts after agreed horizons (e.g., aggregate transfer receipts), preserving the ability to audit critical obligations (bindings, pins, placements, scrubs, grants, revocations) indefinitely.

---

## 14. Identity and Capabilities

Identity and authorization rely on keys and short-lived, attenuable capabilities.

**Keys.** Organizations control namespaces and publish bindings. Devices and services sign storage, transfer, and run receipts. Rotation policies are explicit and receipts indicate the keys used.

**Capabilities.** A capability authorizes actions in narrow scopes with ceilings and expiry. Examples: publish in a namespace; pin up to a limit in a region; serve content with an egress budget; fetch within a scope. Capabilities are referenced by receipts so actions are provably within limits.

**Attestation (multi-party, layered, optional).** Hologram treats attestation as evidence that can be attached to actions and assessed by policy. Multiple parties may attest to different properties:
- Device/runtime attestation: who/where a projection ran, kernel integrity, host profile.
- Storage/placement attestation: where and how content is held, media class, isolation.
- Transfer/path attestation: which holder served bytes, from which site, under what network policy.
- Policy/process attestation: organizational checks (code review, provenance) tied to bindings.

Evidence can be out-of-band (e.g., TEE quotes) or in-band via signed control-plane statements (e.g., a provide announcement with freshness and a nonce). Receipts include `evidenceRef` fields that bind a specific action to specific evidence objects (nonces, timestamps, attester ids). Appraisers—resolvers, providers, or relying services—apply local policy to accept or reject actions based on required evidence classes. No single attestation mechanism is mandated; profiles may enumerate acceptable suites and freshness bounds.

---

## 15. Registry Peers and Federation

Registries are peers, not authorities. A registry peer’s responsibilities are minimal and well-defined.

**Publish and catalog.** Accept bindings for namespaces it is authorized to host and expose catalogs of handles and current bindings. Catalogs are content-addressed and signable.

**Gossip and peering.** Exchange bindings, pin and placement announcements, and scrubs with other peers according to policy. No central enrollment is required; trust arises from signatures and policy.

**Optional storage.** A registry may also act as a holder, pinning and serving content. When it does, it emits placement and transfer receipts like any holder.

**Cross-registry discovery.** Clients can consult multiple peers when resolving. If several peers host the same signed binding, results converge. If conflicting bindings exist, policy decides which signer sets are accepted.

**Multi-homing.** A handle may be bound identically at multiple peers. Clients prefer nearby, healthy providers without changing names.

---

## 16. Router Behavior

Routers select good providers for a digest and pre-stage content to improve runtime performance.

**Provider selection.** Consider latency, freshness of scrub receipts, recent transfer success, and placement diversity. Favor providers that minimize time-to-first-byte and total completion time.

**Prefetch and pre-pin.** Use manifests (declared dependencies, views) and recent access patterns to pre-stage packs and nodes in local/site caches before runtime needs them.

**Health scoring.** Update provider scores based on observed transfer receipts, timeouts, and scrub recency. Avoid flapping between providers unless health degrades.

---

## 17. Bootstrap and Upgrade

Any device can join the ecosystem by pinning a small root set and trusting digests before trusting peers.

**Root set.** Minimal objects required to parse, verify, and participate: canonical encoding rules, verifier implementation, signature suites, baseline policies, and common decoders/dictionaries.

**Acquisition.** Obtain the root set from any number of sources (org servers, mirrors, removable media). Verify by digest before accepting. Embedded and mobile devices may ship with the root set baked in and refresh on first connect; servers and network devices can cache and mirror locally for fast fleet bring-up.

**Upgrade.** Publish new versions of root objects as ordinary content with new digests. Deprecation is signaled by policy and catalog updates. Participants converge by verification, not decree.** Obtain the root set from any number of sources (org servers, mirrors, removable media). Verify by digest before accepting.

**Upgrade.** Publish new versions of root objects as ordinary content with new digests. Deprecation is signaled by policy and catalog updates. Participants converge by verification, not decree.

---

## 18. Security and Privacy

Hologram assumes a zero-trust network and prioritizes privacy without compromising verifiability.

**Encrypt at write.** Private content is encrypted before storage. CAM stores ciphertext; receipts reference digests and policies.

**Selective disclosure.** Views provide typed, minimally sufficient interfaces to data without exposing entire objects. User contexts expose views rather than raw packs.

**Verifiable consent.** Access to user contexts is governed by access grants backed by consent receipts. Providers and resolvers enforce grants and revocations at access time.

**Public content.** Public packs may use convergent encryption or plaintext to maximize deduplication where allowed.

**Capability- and grant-bounded operations.** All impactful actions require either capabilities (infrastructure-scope) or access grants (user-scope). Rate limits and ceilings prevent abuse.

**Attestation policy.** Deployments may define attestation profiles that specify which evidence classes are required for which decisions (e.g., run admission, high-value pins, cross-region serving) and freshness bounds for evidence. Control-plane statements serve as in-band attestations; additional evidence (device/runtime/storage/path) can be attached and validated by policy.

**Threat model.** Adversaries can forge availability claims, replay messages, attempt equivocation, or seek unauthorized access to user data. Mitigations include signatures, replay windows, policy-bound resolution, grant enforcement, evidence freshness checks, and cross-checking of placement/scrub receipts.** Adversaries can forge availability claims, replay messages, attempt equivocation, or seek unauthorized access to user data. Mitigations include signatures, replay windows, policy-bound resolution, grant enforcement, and cross-checking of placement/scrub receipts.

---

## 19. Persistence and Economics

Persistence is explicit and verifiable. Economics are receipt-based and require no memberships.

**Pin classes and scope.** Classes (hot, warm, cold) carry expectations for latency, durability, and scrub cadence. Scope determines where obligations apply (local, org, federation, public).

**Per-transfer commitments.** Transfers may carry small commitments to fund pins for the digests moved (class, scope, duration, sponsor ceilings). Providers convert commitments into pin and placement receipts.

**Grace ladders.** If barter compute cannot clear (offline device, missed window), leases extend or convert to credits automatically. Pins do not lapse silently.

**Edge incentives.** Gateways and ISPs can earn credits by keeping popular digests hot near users. Households may contribute storage and bandwidth within ceilings.

**Settlement.** Periodic settlement receipts net pins provided, transfers served, and compute delivered. Disputes are resolved by replaying receipts.

---

## 20. Observability and Transparency

Operations and audits rely on receipts, not proprietary logs.

**Availability and freshness.** Index placement and scrub receipts to display coverage and last-verified times for important digests.

**Name history.** Show handle→digest changes over time, including the binding receipt used for each change.

**Reproducible releases.** A release is a digest plus a set of bindings. Reproducibility follows from digest identity and stored manifests.

**Audit export.** Produce compact bundles of receipts to satisfy reviews or compliance checks without exposing payloads.

---

## 21. Developer Experience

Hologram keeps developer workflows familiar while strengthening guarantees.

**CLI/SDK flows.** Build (compile to projection), pack (prepare typed bundles), publish (create bindings), provide (announce availability), resolve (deterministically select digests), pull (fetch by digest), run (admit and execute), and inspect (manifests, receipts, coverage).

**Local environment.** A local CAM daemon and control/transfer clients make it easy to test distribution and runtime behavior.

**Schema validation.** Manifests and receipts use stable schemas with strict validation. Evolution is additive and versioned.

**Debuggability.** Because every action yields receipts, developers can examine why a resolve picked a digest, why a runtime admission failed, or where a transfer sourced bytes from.

---

## 21a. Subscriber Experience and User Contexts

This section describes the role of end users (subscribers) who run projections and maintain personal state in their own contexts.

**Multi-device by default.** A user’s phones, laptops, embedded devices, and home gateways share the same handles and digests. Each device resolves names, fetches what it needs, and pins according to user policy; receipts synchronize when connectivity is present.

**Publisher app vs user data.** A built container (projection/service) is **publisher data** from the platform’s perspective. The records it reads/writes are **user data** that live in the user’s context. Apps operate through declared **views** and user-issued **grants**, never by relocating user data into publisher namespaces.

**What a user controls.** Each user has a namespace and storage scope (the user context). Within it, the user can publish data packs (profiles, settings, messages, media), bind human handles, and define exposure policies.

**Publishing personal data.** User devices write data into CAM as packs within the user context, producing storage receipts. A handle like `holo://user.alice/profile:current` can bind to the current profile digest via a user-signed binding.

**Sharing with providers or peers.** When a service needs access (e.g., messaging, social, personalization), the user issues an access grant over specific views (e.g., "read profile basics", "post message", "read inbox"). The grant is backed by a consent receipt. Providers present the grant when resolving or fetching; resolvers and providers enforce scope and expiry.

**Messaging/social example.** A messaging projection writes messages into the sender’s context and exposes a view addressable by recipients. Recipients subscribe to the sender’s view handle with a valid grant; routers fetch from the nearest healthy providers; each read/write is evidenced by transfer and (optionally) view receipts. No centralized inbox is required; the user context is the durable store.

**Revocation and expiry.** Users can revoke grants at any time. Revocations propagate through the control plane; subsequent resolves fail or downgrade access according to policy. Expired grants are rejected at resolve time without manual cleanup.

**Portability and backup.** Because user data is content-addressed, moving to a new device is a matter of resolving handles and fetching digests. Pins can be funded directly by the user or indirectly via per-transfer commitments from relying parties.

**Offline and recovery.** User devices operate against local CAM; changes are synchronized as receipts when connectivity returns. Conflicts are handled by binding updates and deterministic resolution, not by overwriting opaque state.

**UX implications.** Consents are tangible (you can inspect the grant and its scope), sharing is explicit (handles and views), and revoke is effective (enforced by resolvers/providers). Apps can display who has access to which views and for how long by listing consent/grant receipts.

**Minimal user-facing verbs.** Share (issue a grant for views to a party), Stop sharing (revoke), Publish (bind a handle in your context), Back up (pin packs to chosen providers), Restore (resolve and fetch to a new device).** Share (issue a grant for views to a party), Stop sharing (revoke), Publish (bind a handle in your context), Back up (pin packs to chosen providers), Restore (resolve and fetch to a new device).

---

## 22. Interoperability and Conformance

Interoperability depends on precise contracts and shared test artifacts.

**Required behaviors.** Subsystems must adhere to canonicalization, digest derivation, receipt formats, and minimal API surfaces.

**Test vectors and fixtures.** Publicly available canonical objects, manifests, and receipts allow independent implementations to confirm correctness.

**Compliance levels.** Clear profiles define what it means to be a minimal peer, a registry, a holder, or a full runtime node.

**Version negotiation.** Control-plane and transfer-plane messages include version hints and downgrade strategies. Backwards compatibility is maintained for core fields.

---

## 23. Case Studies

**Personal multi-device sync.** A user buys a new device and signs in. Handles resolve to digests; the router fetches from the nearest healthy providers; the local CAM pins frequently used packs. The experience is fast, deterministic, and independent of any single vendor.

**Cross-registry distribution for public packs.** An organization publishes a dictionary pack and binds it under a public namespace. Multiple registries pin and provide it. Developers everywhere reference the same digest; routers prefer local providers; availability is evidenced by placement and scrub receipts.

**Production service composed of multiple projections.** A service manifest wires three projections together (ingest, transform, serve) and declares state packs. Rollouts publish new bindings per environment. Prefetch and pre-pin ensure runtime locality; run receipts document lineage; rollback is a binding update.

---

## 24. Evaluation Methodology

**Portability.** Verify that identical digests produce identical observable behavior across diverse hosts.

**Distribution performance.** Measure resolve latency, time-to-first-byte, completion time, cache hit rates, and the reduction in residual bytes due to shared packs.

**Persistence health.** Track pin coverage by class/scope, scrub age distributions, and repair times after induced failures.

**Security drills.** Attempt receipt forgery, replay attacks, and equivocation; confirm that verifiers and policies detect and reject them.

---

## 25. Conclusion

Hologram reframes containers as content-first, verifiable objects that run identically anywhere, are named deterministically under policy, and persist through explicit, auditable commitments. By keeping the core small—canonical bytecode, CAM, naming, runtime admission, a minimal control plane and transfer plane, identity and capabilities, and receipts—Hologram enables a decentralized, federated distribution fabric without sacrificing developer ergonomics.

For implementers, the path is straightforward: build the Atlas bytecode encoder/decoder; stand up a CAM with packs and Merkle indexing; implement the name resolver with signed bindings; add the minimal control and transfer planes; wire in receipts; and run projections under the kernel. From there, registries, gateways, and developer tooling can grow atop a substrate that is portable, deterministic, and verifiable by design.

