# L2 and L3 Completion Plan

**Goal:** Finish Layer 2 (Conservation) and Layer 3 (Resonance) so Layer 4 (Manifold) can consume a minimal, fast, and stable contract. This plan focuses on the *remaining tasks*, with concrete data layouts, ABIs, acceptance criteria, and benchmark targets.

---

## Principles

* Keep **hot kernels** in **LLVM IR** (portable, SIMD‑friendly, lit/FileCheck testable). Prefer straight‑line loops that autovectorize; add tuned variants only when needed.
* Keep **orchestration & state** in a **small host runtime** (Rust or C++) with a **stable C ABI**. No STL types or exceptions across the ABI.
* **Zero‑copy:** pass raw pointers + lengths; do not take ownership unless stated. All stateful handles are **opaque** and must have matching `destroy` functions.
* **Portability:** x86‑64 (SSE2/AVX2/AVX‑512), AArch64 (NEON), and WASM SIMD. No target‑specific intrinsics in portable IR; rely on the backend or use dispatch at the C layer.
* **Determinism:** algorithms are pure given the same inputs; any randomness is seeded and explicit.

---

## Layer 2 — Conservation (domains, witnesses, budgets)

### A) LLVM IR kernels (hot paths)

* `i7 @atlas.conserved.delta(ptr before, ptr after, i64 len)`

* `i1 @atlas.conserved.window.check(ptr data, i64 len)`

* `void @atlas.conserved.update(ptr state, ptr chunk, i64 n)` (streaming)

* Conserved `memcpy`/`memset` fixups

* `ptr @atlas.witness.generate(ptr data, i64 len)` / `i1 @atlas.witness.verify(ptr w, ptr data, i64 len)`

### B) Host runtime (Rust/C++ with C ABI)

**Opaque state & errors:**

```c
// Error codes
typedef enum {
  ATLAS_OK=0,
  ATLAS_E_CONSERVATION=1,
  ATLAS_E_WITNESS=2,
  ATLAS_E_BUDGET=3,
  ATLAS_E_MEMORY=4,
  ATLAS_E_STATE=5
} atlas_error_t;

// Domain state
typedef struct atlas_domain atlas_domain_t;   // opaque

// Witness state
typedef struct atlas_witness atlas_witness_t; // opaque
```

**Functions (C ABI):**

```c
// Domain lifecycle
atlas_domain_t* atlas_domain_create(size_t bytes /*expected 12288*/, uint8_t budget_class);
int             atlas_domain_attach(atlas_domain_t*, void* base, size_t len); // 0=OK else atlas_error_t
bool            atlas_domain_verify(const atlas_domain_t*);
int             atlas_domain_commit(atlas_domain_t*); // OPEN→COMMITTED, idempotent; returns atlas_error_t
void            atlas_domain_destroy(atlas_domain_t*);

// Budget book (mod‑96)
bool atlas_budget_alloc(atlas_domain_t*, uint8_t amt);
bool atlas_budget_release(atlas_domain_t*, uint8_t amt);

// Witness lifetime
atlas_witness_t* atlas_witness_generate(const void* base, size_t len);
bool              atlas_witness_verify(const atlas_witness_t*, const void* base, size_t len);
void              atlas_witness_destroy(atlas_witness_t*);

// Convenience
uint8_t atlas_conserved_delta(const void* before, const void* after, size_t len);
```

**Internal structures (reference impl):**

```c
// Not exposed in headers
typedef enum { DOM_OPEN=0, DOM_COMMITTED=1 } dom_state_t;
struct atlas_domain {
  _Atomic int       state;         // DOM_OPEN/DOM_COMMITTED
  uint8_t*          base;          // non-owning pointer to region
  size_t            len;           // expect 12288, but generic
  _Atomic uint32_t  budget_i7;     // lower 7 bits used, range 0..95
  atlas_witness_t*  witness;       // set on commit
};
```

**Concurrency & WASM:**

* `atlas_domain_commit` uses CAS to transition `OPEN→COMMITTED`; second commit returns `ATLAS_E_STATE` (idempotent safety).
* Budgets stored in `_Atomic uint32_t`; updates: `new=(old+amt)%96` via branch‑free arithmetic; fail if insufficient to release.
* WASM build replaces atomics with single‑threaded code paths behind a `ATLAS_SINGLE_THREAD` macro/feature.

**DoD:** Race‑free commit; budgets always in `[0,95]`; witness handle immutable post‑generation; all API calls return status codes, no exceptions; leak‑free `destroy` paths.

### C) Tests & CI

* **lit/FileCheck (IR):**

  * Delta correctness (randomized seeds, edges: len=0,1,96,256,512,12288).
  * Window check truth table; memset fixup; memcpy pre/post verify.
  * Example pattern:

    ```
    ; CHECK: call i1 @atlas.conserved.window.check
    ; CHECK: icmp eq i32 %mod96, 0
    ```
* **Property tests (host):**

  * `(Σ(before)+delta) % 96 == Σ(after) % 96` for random buffers.
  * Memset fixup always yields `%96==0`.
* **Concurrency (host):**

  * 1000 threads contend on `atlas_budget_alloc/release`; commit races over same domain → exactly one success, others return `ATLAS_E_STATE`.
* **Fuzz:**

  * Random bytes for `attach/verify/commit`; ensure failure‑closed (no UB or leaks).
* **CI matrix:**

  * Ubuntu x86\_64 (AVX2), macOS ARM64 (NEON), wasm32 (SIMD) using `wasmtime` or `node --experimental-wasm-simd`.

**DoD:** All tests pass locally/CI; coverage includes happy/sad paths; no sanitizer findings under ASan/UBSan builds.

### D) Benchmarks & docs

* **Throughput:** conserved memcpy/memset (GB/s) on 12,288‑byte windows.
* **Witness:** generate/verify (MB/s) & cycles/byte on structure‑sized buffers.
* **Delta:** ns per 12,288‑byte comparison (scalar vs SIMD).
* **Targets:** AVX2 ≥ 25 GB/s memcpy (aligned), NEON ≥ 15 GB/s, WASM ≥ 5 GB/s (indicative; adjust after first run).
* **Docs:** “L2 contract” in `docs/` — signatures, invariants, error codes, performance notes, and examples.

**DoD:** Bench numbers captured per target in `docs/benchmarks/l2.md`; reproducible make targets exist (`make bench-l2-*`).

---

## Layer 3 — Resonance (classification, clustering, scheduling)

### A) LLVM IR kernels (hot paths)

* `void @atlas.r96.classify.page(ptr in256, ptr out256)` (256 bytes → 256 classes)
* `void @atlas.r96.histogram.page(ptr in256, ptr out96_u16)` (96-bin histogram)
* `i1 @atlas.r96.harmonizes(i7 r1, i7 r2)` ((r1+r2) % 96 == 0)

### B) Host runtime (Rust/C++ with C ABI)

**Data formats:**

* **Spectrum:** `uint8_t out256[256]` (classes 0..95).
* **Histogram:** `uint16_t bins[96]`.
* **Clusters (CSR):**

  * `offsets[97]` where `offsets[r]`..`offsets[r+1]-1` index class‑`r` elements.
  * `indices[n]` store **Φ‑linearized coordinates** `coord = page*256 + offset` (range 0..12287).
  * Optional SoA variant with separate `pages[]` and `offsets[]` if needed later.

**Functions:**

```c
// Spectra & histograms
void atlas_r96_classify_page(const uint8_t* in256, uint8_t out256[256]);
void atlas_r96_histogram_page(const uint8_t* in256, uint16_t out96[96]);

// Cluster builder (CSR)
typedef struct {
  const uint32_t* offsets;  // len=97
  const uint32_t* indices;  // len=n
  uint32_t        n;        // total coords
} atlas_cluster_view;

// Build clusters over N pages (base points to N*256 bytes)
atlas_cluster_view atlas_cluster_by_resonance(const uint8_t* base, size_t pages);
void               atlas_cluster_destroy(atlas_cluster_view*);

// Scheduling (phase-locked to 96)
uint64_t atlas_next_harmonic_window_from(uint64_t now, uint8_t r);
// Convenience: next from t=0
static inline uint64_t atlas_next_harmonic_window(uint8_t r){ return atlas_next_harmonic_window_from(0, r); }
```

**Tasks:**

* Decide & document spectrum representation (we choose widened `i8`).
* Build CSR by scanning pages, pushing `coord = p*256 + b` into bucket `R96(byte)`; compute `offsets` via prefix sums. Stable by coordinate.
* Region intersection helpers using Boundary Φ: input a rectangular region → filter indices.
* Minimal scheduler: `next = now + ((96 - ((now + r) % 96)) % 96)`.
* WASM: use a simple arena or `malloc` with `atlas_cluster_destroy`; ensure no leaks.

**DoD:** CSR is homogeneous (each bucket contains only its resonance), covers all inputs (n == pages\*256), strictly increasing indices within each bucket; scheduler deterministic for all `now,r`.

### C) Tests & CI

* **lit/FileCheck (IR):** classify\_page equivalence vs scalar; histogram totals; harmonize truth table.
* **Golden tests:**

  * Construct small pages with known distributions (e.g., all zeros → bin0=256).
  * Mixed patterns → hand‑checked CSR buckets.
* **Homogeneity & coverage:**

  * For each `r`, all `coord` in `offsets[r]..` `offsets[r+1]` satisfy `spectrum[coord]==r`.
* **Performance assertions:** classify\_page throughput ≥ target per arch; histogram within 20% of classify throughput.
* **WASM round‑trip:** compile to `wasm32-unknown-unknown` and execute in a headless runner; spectra/histograms match native.

**DoD:** Tests green on all targets; perf thresholds met or explained; wasm variant passes with identical outputs.

### D) Benchmarks & docs

* **Classify/histogram GB/s:** SSE2/AVX2/AVX‑512/NEON/WASM (report medians of 30 runs; pin CPU frequency where possible).
* **Cluster build:** time for 48 pages; CSR memory footprint (`(97*4 + n*4)` bytes).
* **Scheduler:** decisions/s; typical overhead < 10 ns on AVX2 hosts.
* **Docs:** “L3 contract” — spectrum format, CSR layout, `next_harmonic_window_from()` semantics with examples.

**DoD:** Bench numbers captured in `docs/benchmarks/l3.md`; reproducible make targets exist (`make bench-l3-*`).

---

## Cross‑cutting (L2 + L3)

* **C ABI header (********`include/atlas.h`****\*\*\*\*)** — contains L2/L3 groups, ownership notes, alignment (assume byte‑aligned unless stated), and `#define ATLAS_API_VERSION 1`.
* **Binary compatibility** — freeze struct sizes & field order; add padding only at the end; expose version via `atlas_get_api_version()`.
* **Metrics hooks** — optional `atlas_enable_metrics(int)` toggles internal counters; compiled out in release via `#ifndef NDEBUG`.
* **Acceptance tests** —

  * R96 partition is total; Φ round‑trip is identity; `%96` conservation for all mutation paths.
  * Cluster homogeneity; witness immutability; atomic commit safety.
* **CI artifacts** — publish `libatlas.{a,so}`, `include/atlas.h`, wasm modules; attach benches as build artifacts.
* **L4 unblocker example** — `examples/projection_seed`: domain → histogram → cluster → emit shard metadata (JSON) with witness id.

---

## Minimal Deliverables to Unblock L4

1. **IR:** `@atlas.conserved.delta`, `@atlas.r96.classify.page`, `@atlas.r96.histogram.page`.
2. **Runtime:** `atlas_domain_*`, `atlas_witness_*`, `atlas_cluster_by_resonance` (+ `destroy`), `atlas_next_harmonic_window_from`.
3. **Benches:** conserved memcpy/memset, witness gen/verify, classify/histogram, cluster build.
4. **Headers:** `include/atlas.h` with stable signatures and `atlas_error_t`; `ATLAS_API_VERSION` accessor.
