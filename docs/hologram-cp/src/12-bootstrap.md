# Bootstrap Architecture

## From Minimal Store to Self-Describing Platform

The Hologram platform is **self-describing**—it contains its own definition as projections in the store. But this creates a bootstrapping challenge: how does the platform initialize when it needs projections to understand projections?

This document describes the bootstrap sequence—how the platform cold-starts from minimal primitives and builds up to full self-describing operation.

## The Bootstrap Problem

The platform requires:
- Projection definitions to understand how to project
- Container store to hold projection definitions
- Projection engine to execute projection definitions

But:
- Projection definitions are resources in the store
- The engine needs projection definitions to project resources
- Projecting projection definitions requires the engine

This is the **bootstrap paradox**: the system needs itself to start itself.

## Bootstrap Solution: Staged Initialization

The solution is **staged initialization**—starting with minimal hard-coded capabilities and progressively replacing them with projected capabilities.

Each stage builds on the previous, until the platform is fully self-describing and no hard-coded logic remains (or only fundamental primitives remain).

## Bootstrap Layers

### Layer 0: Minimal Content-Addressed Store

**What Exists**: A content-addressed storage implementation providing the four core capabilities:
- Store (write resource, get CID)
- Retrieve (read resource by CID)
- Reference (extract CIDs from resource)
- Query (identify resources by basic criteria)

**What's Hard-Coded**: The store implementation itself (filesystem, database, memory).

**What's Not Present**: Projection engine, projection definitions, container types, operations.

This is the **axiom layer**—the minimal foundation assumed to exist.

### Layer 1: Store Definition (hologram.store)

**What Happens**: Emit the store's own definition as a resource.

**Resource Content**: JSON document describing:
- Store capabilities (store, retrieve, reference, query)
- Store properties (immutability, content addressing, deduplication)
- Store interface (how to interact with store)

**Process**:
1. Hard-coded bootstrap code creates store definition resource
2. Computes CID
3. Stores resource in the store
4. The store now contains its own definition

**Result**: The store is **introspectable**—its definition exists as a resource.

**What's Still Hard-Coded**: Projection engine (doesn't exist yet).

### Layer 2: Projection Language (hologram.projection)

**What Happens**: Emit the projection language definition as resources.

**Resource Content**:
- Projection definition schema (what fields projection definitions have)
- Conformance requirement schema (how to specify conformance)
- Query language schema (how to express queries)
- Transformation rule schema (how to define transformations)

**Process**:
1. Hard-coded bootstrap code creates projection language definition
2. Stores as resources in the store
3. Hard-coded minimal projection engine loads these definitions
4. Engine can now interpret projection definitions

**Result**: The projection language is **self-defined**—its schema exists as resources.

**What's Still Hard-Coded**: Minimal projection engine (can interpret but is hard-coded).

### Layer 3: Base Container Schemas

**What Happens**: Emit schemas for base container types.

**Resource Content**:
- hologram.component schema
- hologram.instance schema
- hologram.interface schema
- hologram.documentation schema
- hologram.test schema
- hologram.manager schema
- hologram.view schema
- hologram.dependency schema
- hologram.build schema
- hologram.log schema

**Process**:
1. Bootstrap code creates JSON Schema definitions for each container type
2. Stores schemas as resources
3. Engine can now validate resources against these schemas

**Result**: Container types are **schema-defined** rather than hard-coded.

**What's Still Hard-Coded**: Operation implementations (projection engine can validate but not execute operations).

### Layer 4: Projection Definitions

**What Happens**: Emit projection definitions for each container type.

**Resource Content**: For each container type, a projection definition specifying:
- Query to identify resources
- Conformance requirements (what resources must be present)
- Aggregation rules (how to follow references)
- Transformation rules (how to structure container)
- Schema references (what schemas apply)

**Process**:
1. Bootstrap code creates projection definitions using projection language from Layer 2
2. Stores projection definitions as resources
3. Engine loads projection definitions
4. Engine can now project resources into containers per definitions

**Result**: Container types are **projection-defined** rather than hard-coded.

**What's Still Hard-Coded**: Basic projection engine logic (query evaluation, reference traversal, transformation application).

### Layer 5: Engine Definition (hologram.engine)

**What Happens**: Emit the projection engine's own definition as resources.

**Resource Content**:
- Engine capabilities (query execution, traversal, transformation)
- Engine algorithm descriptions (how it processes projections)
- Engine extension points (how to add custom query types, validators, transformations)

**Process**:
1. Bootstrap code creates engine definition
2. Stores as resources in the store
3. Advanced implementations could use this to build engines (compilation, optimization)

**Result**: The engine is **self-described**—its behavior is documented in resources.

**What's Still Hard-Coded**: Actual engine implementation (but its behavior is defined in resources).

### Layer 6: Operation Definitions

**What Happens**: Emit operation definitions as projection definitions.

**Resource Content**: For each operation (component.create, instance.start, etc.):
- Input schema
- Projection phase specification
- Execution logic description
- Emission pattern
- Output schema

**Process**:
1. Bootstrap code creates operation definitions
2. Stores as resources
3. Platform can now execute operations per definitions

**Result**: Operations are **definition-driven** rather than hard-coded.

**What's Still Hard-Coded**: Execution logic for operations (projections define what to do, but implementation executes it).

### Layer 7: Complete Self-Description

**What Happens**: The platform contains complete definition of itself.

**What's in Store**:
- Store definition
- Projection language definition
- All container type schemas and projection definitions
- Engine definition
- Operation definitions
- Platform API definition
- Bootstrap process definition (this document as a resource!)

**Result**: The platform is **fully self-describing**—everything about its behavior exists as resources.

**What's Still Hard-Coded**: Only Layer 0 (minimal CAS store) and basic projection engine execution. Everything else is defined in resources.

## Bootstrap Sequence

The actual initialization sequence when starting a Hologram platform:

### Step 1: Initialize Store

Start with empty or existing content-addressed store.

If empty: create minimal store implementation.

If existing: load store and check for bootstrap resources.

### Step 2: Check Bootstrap State

Query store for bootstrap marker resource.

If not present: platform is uninitialized, proceed with bootstrap.

If present: platform is already bootstrapped, load existing definitions.

### Step 3: Emit Foundation Resources (if bootstrapping)

Execute Layer 0-2 bootstrap:
1. Emit hologram.store definition
2. Emit hologram.projection definition
3. Emit projection language schemas

### Step 4: Initialize Minimal Engine

Create minimal projection engine with hard-coded logic:
- Basic query evaluation
- Reference traversal
- Schema validation
- Simple transformations

Engine loads projection language definitions from store.

### Step 5: Emit Container Definitions

Execute Layer 3-4 bootstrap:
1. Emit base container type schemas
2. Emit projection definitions for each container type

Engine can now project these container types.

### Step 6: Emit Operation Definitions

Execute Layer 6 bootstrap:
1. Emit operation definitions for all platform operations

Platform can now execute operations per definitions.

### Step 7: Emit Bootstrap Marker

Create bootstrap marker resource indicating:
- Bootstrap completion timestamp
- Platform version
- Definitions emitted

Emit marker to store.

### Step 8: Verify Bootstrap

Execute validation projection:
1. Project all bootstrap resources
2. Verify schemas valid
3. Verify projection definitions valid
4. Verify operations defined

If validation passes: bootstrap complete.

If validation fails: abort, report errors.

### Step 9: Enter Normal Operation

Platform is now fully bootstrapped and operational.

All operations use projection-emission cycle.

Platform is self-describing and can introspect its own definitions.

## Bootstrap Resources

The bootstrap resources are the **platform kernel**—minimal set of resources required for self-describing operation.

These resources form the **base projection set** that all other projections build upon.

Bootstrap resources should be:
- Minimal (only essential definitions)
- Stable (rarely change, only for platform evolution)
- Well-documented (critical to platform understanding)
- Versioned (enable platform upgrades)

## Incremental Bootstrap

For large platforms, bootstrap can be incremental:

**Phase 1**: Core projection system (Layers 0-4).

**Phase 2**: Base operations (component CRUD, instance lifecycle).

**Phase 3**: Extended container types (dependency, build, log).

**Phase 4**: Views and optimization (catalog, materialized views).

**Phase 5**: Advanced features (distributed execution, federation).

Each phase emits resources, expands platform capabilities, enables next phase.

## Bootstrap from Import

Instead of hard-coded bootstrap, platform can bootstrap from resource import:

**Step 1**: Start with minimal store + engine.

**Step 2**: Import bootstrap resources from external source:
- File bundle
- Remote repository
- Another Hologram instance

**Step 3**: Engine loads imported definitions.

**Step 4**: Platform operational.

This enables platform distribution as resource bundles.

## Bootstrap Verification

After bootstrap, verify platform consistency:

**Definition Consistency**: All projection definitions reference valid schemas.

**Schema Validity**: All schemas are well-formed JSON Schema.

**Reference Integrity**: All CID references in bootstrap resources exist.

**Operation Completeness**: All required operations are defined.

**Engine Capability**: Engine can execute all projection types defined.

Verification ensures bootstrap produced valid platform state.

## Evolution After Bootstrap

Once bootstrapped, the platform evolves through normal projection-emission:

**New Container Types**: Emit new projection definitions → new container types available.

**New Operations**: Emit new operation definitions → new operations executable.

**Improved Schemas**: Emit updated schemas → refined validation.

**Platform Updates**: Emit updated bootstrap resources → platform evolves.

Evolution uses the same mechanisms as application development.

## The Minimal Kernel

What must remain hard-coded (cannot be bootstrapped away)?

**Content-Addressed Storage**: The fundamental primitive (store, retrieve, reference, query).

**Projection Execution**: The ability to evaluate projections (though projection definitions can describe how).

**Resource Serialization**: Converting between in-memory and byte representations.

These form the **irreducible kernel**—the minimal hard-coded logic required.

Everything else can be defined as resources through projections.

## Bootstrap Reproducibility

Bootstrap should be reproducible:
- Same bootstrap code produces same resources
- Same resources produce same CIDs
- Same CIDs produce identical platform state

Reproducibility enables:
- Deterministic platform creation
- Verification of platform integrity
- Consistency across distributed instances

## Next Steps

With bootstrap complete, the platform exhibits key properties that make it robust and evolvable. The next document, **Platform Properties**, describes the guarantees, characteristics, and emergent properties that result from the projection-emission model.