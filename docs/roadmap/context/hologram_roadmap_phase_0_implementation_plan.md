# Phase 0 Implementation Plan — Object Model & Canonical Encodings

## 0. Objectives
- Define the canonical object model and encodings for Hologram artifacts: **Projection**, **Pack**, **Service**, **Binding**, and **Receipt**.
- Establish deterministic build and verification pipelines where **identical inputs produce identical bytes and digests** across platforms.
- Ship reference tools, fixtures, and a conformance suite that independent teams can run to achieve cross‑implementation parity without central coordination.

## 1. Scope (what Phase 0 includes)
- Canonical binary format (**Atlas**) and manifest schemas.
- Receipt schemas and signature envelope.
- Deterministic builder, verifier, and digest derivation rules.
- Golden test vectors, fuzzing seeds, and a CI matrix for cross‑platform digest parity.
- Minimal developer tooling (CLI) and language bindings (Go/TS/Rust) for encode/verify.

**Out of scope for Phase 0:** control/transfer plane networking; runtime execution; registry/peering; user‑context grants; policy engines (only schema stubs).

## 2. Deliverables
1) **Atlas Encoding Specification v0** (CBOR/COSE or Protobuf + deterministic rules), with:
   - Canonical serialization rules (field ordering, integer widths, map key normalization, prohibited floats unless fixed‑precision, UTF‑8 NFC).
   - Byte layout for headers, sections, payloads; versioning and extension points.
   - Content‑ID (CID) derivation (hash suite; domain‑separated prefixes; multi‑hash encoding).

2) **Schemas (v0) in JSON Schema + prose spec**
   - `ProjectionManifest`, `PackManifest`, `ServiceManifest`.
   - `Binding` (handle→digest under policy ref).
   - `Receipt` families: `Storage`, `Pin`, `Placement`, `Scrub`, `Transfer`, `BindingReceipt`, `Run` (stub only), `Consent/Grant/Revocation` (stubs), common `EvidenceRef`.

3) **Reference Implementations**
   - `atlas-encode` (builder) and `atlas-verify` (verifier) in Go (ref impl) with CLI + library API.
   - Minimal ports (TypeScript, Rust) for verify‑only to prove cross‑lang determinism.

4) **Golden Fixtures & Conformance Suite**
   - Canonical object corpora with expected digests.
   - Negative fixtures (malformed/ambiguous encodings) with expected errors.
   - Cross‑platform digest parity tests; differential fuzzing harness.

5) **Documentation & Examples**
   - Spec prose, API docs, encoding diagrams, worked examples.
   - "Hello Atlas" sample (one projection, two packs, one service).

## 3. Architecture Decisions (ADRs)
- **ADR‑0001: Binary container format.** Choose CBOR + deterministic canonical form or Protobuf with elided field defaults and sorted maps. Record tradeoffs and canonicalization rules.
- **ADR‑0002: Hash suite.** Pluggable hash algorithm with default (e.g., BLAKE3‑256 or SHA‑256), domain separation string `"holo.atlas.v0"`, and multi‑hash framing.
- **ADR‑0003: Signature envelope.** COSE_Sign1 for single‑signer receipts and bindings; detached payload digests; key algorithms allowed (Ed25519 mandatory, P‑256 optional).
- **ADR‑0004: Determinism policy.** Builders must not embed non‑deterministic fields (timestamps, random seeds); if needed, expose them as explicit declared inputs.
- **ADR‑0005: Versioning & extensibility.** Forward‑compatible section TLVs with strict unknown‑field handling (ignore in verify, forbid in digest if non‑canonical).

## 4. Data Model & Schemas
### 4.1 Core object headers (common)
```text
magic: u32 = 0x484F4C4F   # "HOLO"
version: u16              # atlas format version
object_type: u8           # 0=Projection,1=Pack,2=Service,3=Binding,4=Receipt
flags: u8                 # canonicalization feature bits
sections_count: u16
```

### 4.2 Sections (typed, order‑insensitive in model; order‑sensitive in bytes via canonical sort)
- `META` (key/value, small, UTF‑8 NFC, stable sort by key)
- `MANIFEST` (typed manifest per object_type)
- `DEPS` (array of content digests)
- `PAYLOAD` (opaque bytes for packs; code/data for projections)
- `IO_DECL` (declared I/O surfaces; schemas by digest)
- `POLICY_REF` (digest of policy pack)

### 4.3 Manifest schemas (abridged)
```json
// ProjectionManifest v0 (JSON Schema outline)
{
  "$id": "holo/projection.v0",
  "type": "object",
  "required": ["name", "entrypoint", "compatProfile", "inputs", "outputs"],
  "properties": {
    "name": {"type": "string"},
    "entrypoint": {"type": "string"},
    "compatProfile": {"type": "string"},
    "inputs": {"type": "array", "items": {"$ref": "#/definitions/io"}},
    "outputs": {"type": "array", "items": {"$ref": "#/definitions/io"}},
    "deps": {"type": "array", "items": {"type": "string", "pattern": "^cid:[a-z0-9]+$"}},
    "resourceHints": {"type": "object"}
  },
  "definitions": {
    "io": {
      "type": "object",
      "required": ["name", "schemaDigest"],
      "properties": {
        "name": {"type": "string"},
        "schemaDigest": {"type": "string"}
      }
    }
  }
}
```
(Equivalent schemas for `PackManifest` and `ServiceManifest` are included in the repo.)

### 4.4 Binding & Receipt envelopes
- **Binding**: `{ handle, digest, policyRef, signer, signature, created, expires }` (timestamps allowed **outside** the digest; the digest covers canonicalized binding body without signature fields.)
- **Receipt**: common fields `{ kind, subjectDigest, ctx, capabilityRef?, evidenceRef?, issuedAt, expiresAt? }` + kind‑specific payload; signed via COSE.

## 5. Canonicalization Rules
- **Maps:** sorted by UTF‑8 codepoint of keys; no duplicate keys; keys must be NFC.
- **Numbers:** integers only unless schema defines fixed‑precision decimals encoded as scaled integers.
- **Strings:** UTF‑8 NFC; no leading/trailing whitespace in identifiers; length bounds enforced.
- **Booleans/Nulls:** explicitly prohibited in canonical surfaces unless schema requires.
- **Arrays:** element‑order significant; no heterogenous arrays unless schema lists unions.
- **Timestamps:** RFC 3339 **only** in receipts/bindings and excluded from content‑ID preimage via precise rules.
- **Compression:** if used in PAYLOAD, the raw uncompressed bytes are hashed (to avoid compressor variance).

## 6. Content ID (CID) Derivation
```
preimage := domain_sep || atlas_version || canonical_bytes
cid := multihash(hash(preimage))
```
- `domain_sep = "holo.atlas.v0"`
- `multihash`: varint algo id + digest length + digest bytes.
- Hash suite pluggable via codec id; default algo mandatory for interop.

## 7. Reference Tools (Go)
### 7.1 CLI
- `atlas-encode <manifest|binding|receipt> -i spec.json -p payload.bin -o object.atlas`
- `atlas-verify <object.atlas>` → prints CID, section table, canonicalization checks.
- `atlas-diff <a.atlas> <b.atlas>` → shows semantic vs. byte deltas.
- `holo-digest <file>` → prints CID for arbitrary canonical bytes.

### 7.2 Library API (Go)
```go
type Object struct { Header Header; Sections []Section }
func Encode(o *Object, opts EncodeOpts) ([]byte, CID, error)
func Verify(raw []byte, opts VerifyOpts) (CID, Report, error)
func CanonicalizeManifest(in []byte) ([]byte, error)
```

### 7.3 Determinism Harness
- Re‑encode same manifest/payload via different code paths and ensure byte‑identical outputs; property‑based tests (gofuzz) with shrinkers on failure.

## 8. Fixtures & Conformance
- **Golden corpus:** ~100 objects across types; `fixtures/*` with `*.atlas`, `*.json`, `CID.txt`.
- **Parity tests:** matrix across Linux/macOS/Windows, x86_64/arm64; ensure byte‑identical outputs and CIDs.
- **Negative tests:** duplicated keys, float usage, unstable map order, timestamp in digest path, payloads with different compressor settings yielding identical canonical preimage.
- **Fuzzing:** mutation operators for key order, unicode normalization, numeric widening.
- **Conformance tool:** `conform run` produces a signed report (machine‑readable) stating pass/fail per test vector.

## 9. Keys & Signatures (Phase‑0 subset)
- **Key formats:** JWK (OKP/EC) with kid; PEM import/export convenience.
- **Signing:** COSE_Sign1 (Ed25519 mandatory).
- **Verification:** deterministic JSON→canonical‑bytes transform before signature verification.
- **Key rotation:** carried in receipts as metadata only (no policy engine yet).

## 10. Error Model & Diagnostics
- Structured error codes (e.g., `E_CANON_KEY_ORDER`, `E_FLOAT_DISALLOWED`, `E_TIMESTAMP_IN_PREIMAGE`, `E_SIG_BAD`, `E_SCHEMA_MISMATCH`).
- `atlas-verify -v` prints a tree of canonicalization decisions; `--why` flag emits the precise rule triggered on failure.

## 11. Repository Layout
```
/atlas/spec/           # prose spec, diagrams, ADRs
/atlas/schemas/        # JSON Schemas (v0)
/atlas/go/             # ref impl (encode/verify)
/atlas/ts/verify/      # TS verify-only lib
/atlas/rs/verify/      # Rust verify-only lib
/fixtures/             # golden vectors
/conformance/          # runner + reports
/tools/                # CLI wrappers
```

## 12. CI/CD Matrix
- Cross‑platform runners (Linux, macOS, Windows) on x86_64 + arm64.
- Steps: build → unit tests → schema tests → determinism tests → conformance → generate digest parity report → sign report artifact.
- Artifact retention for fixtures and signed parity reports.

## 13. Security Considerations (Phase‑0)
- Canonicalization is the attack surface; enforce strict parsing and reject ambiguous inputs.
- Hash agility with registry of allowed algorithms; refuse unknown codecs.
- Limit map/array sizes; stream verification to avoid memory blow‑ups.
- All spec docs include consensus test cases for Unicode spoofing and numeric edge cases.

## 14. Documentation Plan
- Spec pages: *Encoding*, *Digests*, *Schemas*, *Receipts*, *CLI*, *Conformance*.
- Tutorial: build a projection + pack, encode, compute CID, verify, and compare across OSes.
- Troubleshooting: common determinism failures and how to fix them.

## 15. Acceptance Criteria (Phase Exit)
- Byte‑identical canonical encodings on at least two independent implementations.
- 100% pass on golden and negative fixtures across the CI matrix.
- Signed conformance report reproducible by a third party from public fixtures.
- Round‑trip encode/verify/digest stable under repeated runs (no flakiness in 10k loop).

## 16. Work Breakdown Structure (WBS)
1. **Specs & ADRs**
   - Draft ADR‑0001..0005 → review → ratify.
   - Encoding diagrams and canonicalization tables.
2. **Schemas**
   - Author JSON Schemas; generate TypeScript/go structs; schema tests.
3. **Go Reference Implementation**
   - Encoder/decoder; canonicalizer; CID; CLI tools; unit/property tests.
4. **Verify‑Only Ports**
   - TS + Rust verify libraries; cross‑lang parity harness.
5. **Fixtures & Conformance**
   - Golden vectors; negative corpus; runner + signed report.
6. **Docs & Examples**
   - Prose spec; tutorial; sample project.

## 17. Risks & Mitigations
- **Canonicalization bugs:** adopt differential testing across languages; heavy negative corpus; minimize optional fields in v0.
- **Hash algorithm debates:** define pluggable interface now; pick one mandatory default for interop.
- **Hidden nondeterminism:** forbid environment‑dependent defaults; require explicit inputs for anything time/locale/random.
- **Spec drift vs code:** keep spec as source of truth; generate code stubs from schemas where possible.

## 18. Exit Checklist (Go/No‑Go)
- [ ] ADRs merged and locked.
- [ ] Schemas published with semantic version tags.
- [ ] `atlas-encode/verify/diff` released (v0.1.0) with signed binaries.
- [ ] Verify‑only ports (TS/Rust) published.
- [ ] Conformance suite public with signed parity report.
- [ ] Docs live with end‑to‑end tutorial and troubleshooting.

---

**Next up:** Phase 1 will use these artifacts to back the CAM API and start emitting signed receipts for `put`, `pin`, `placement`, and `scrub`, reusing the same canonicalization and CID rules defined here.

