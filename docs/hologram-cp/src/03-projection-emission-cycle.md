# The Projection-Emission Cycle

## The Fundamental Operating Principle

The Hologram platform operates through a continuous cycle of **projection** and **emission**:

**Project → Execute → Emit → Store → Project...**

This cycle is the fundamental mechanism by which the platform creates containers, executes operations, and evolves over time. Every interaction with Hologram—creating components, running instances, materializing views, executing operations—follows this cycle.

This document establishes how the projection-emission cycle works and why it provides a universal operating model for the platform.

## The Four Phases

### Phase 1: Project

**Projection** identifies and aggregates resources from the container store to form a container.

A projection definition specifies:
- **Query criteria**: Which resources to identify (by content patterns, reference relationships, metadata)
- **Aggregation rules**: How to collect related resources (following references, grouping by properties)
- **Transformation**: How to structure aggregated resources into a container

The projection engine evaluates the definition against the store, producing a **container**—a structured collection of resources with defined relationships and purpose.

Projection is **read-only**. It examines the store but does not modify it. The same projection evaluated multiple times against an unchanged store produces the same container.

### Phase 2: Execute

**Execution** operates on the projected container to produce results.

Execution might:
- Validate that resources satisfy constraints
- Transform resources into different formats
- Run computation using resources as inputs
- Coordinate between multiple containers
- Generate new content from container resources

Execution consumes projected resources and produces results. Results might be:
- Status information (success, failure, validation results)
- Transformed content (compiled artifacts, rendered documentation)
- Generated data (logs, metrics, computed results)
- New resource definitions (component updates, instance state)

Execution is **isolated**. It operates on projected resources without directly accessing the store. This isolation enables:
- Reproducibility: Same inputs produce same results
- Testability: Execution can be tested with mock containers
- Distribution: Execution can occur anywhere with the projected resources

### Phase 3: Emit

**Emission** writes execution results back to the container store as new resources.

Each result is:
1. Serialized to bytes
2. Hashed to produce a CID
3. Stored in the container store indexed by CID

Emitted resources become immediately available for future projections. They exist in the store alongside resources from previous operations, forming an accumulated knowledge base.

Emission is **additive**. New resources are added; existing resources are never modified. This preserves complete history—every emission creates a new snapshot while previous snapshots remain accessible.

### Phase 4: Store

**Storage** persists emitted resources durably in the container store.

The store:
- Indexes resources by CID for retrieval
- Updates queryable metadata for projection
- Extends the reference graph with new resources
- Maintains integrity guarantees (immutability, deduplication)

Once stored, resources are available for projection. The cycle completes and can begin again—new projections can include newly emitted resources.

## The Complete Cycle

Chaining these phases creates a continuous cycle:

1. A client requests an operation (create component, run instance, materialize view)
2. **Project**: The engine projects resources needed for the operation
3. **Execute**: The operation runs using projected resources
4. **Emit**: Results are emitted as new resources
5. **Store**: Emitted resources are persisted
6. The client can request further operations, which project the newly stored resources

Each cycle adds to the store. Over time, the store accumulates:
- Component definitions
- Instance states
- Operation logs
- Materialized views
- Application data
- User content

All emitted as immutable, content-addressed resources available for projection.

## Why This Cycle Works

### Universal Applicability

The same cycle applies to every platform operation:

**Creating a component definition**:
- Project: Identify artifact resources (spec, interface, docs, tests)
- Execute: Validate artifacts against schemas, compute component structure
- Emit: Write component definition resource
- Store: Persist component definition

**Running an instance**:
- Project: Identify component definition and dependencies
- Execute: Run the component's defined computation
- Emit: Write logs, state snapshots, application outputs
- Store: Persist instance emissions continuously

**Materializing a view**:
- Project: Identify resources matching view criteria
- Execute: Transform resources into view format
- Emit: Write materialized view resource
- Store: Persist view for efficient access

Every operation is a projection-emission cycle with operation-specific execution logic.

### Composability

Cycles can chain. One operation's emissions become another operation's projection inputs:

- Create component definition → emit component resource
- Create instance from component → project component, emit instance resource
- Instance runs → emit logs and state
- Materialize log view → project logs, emit view
- Query log view → project view, return results

Each operation builds on previous emissions. Complex workflows emerge from composing simple projection-emission cycles.

### Self-Evolution

The platform itself is defined through projections. Improving the platform means:
- Defining new projection definitions (new container types, new operations)
- Emitting those definitions as resources
- The engine projects the new definitions and incorporates them

The platform evolves through the same projection-emission cycle used for applications. There is no separate "platform update" mechanism—platform and application use the same primitives.

### Auditability

Every emission is immutable and content-addressed. The store contains complete history:
- What resources were projected (recorded in operation logs)
- What execution occurred (recorded in execution logs)
- What results were emitted (the emitted resources themselves)
- When operations occurred (store metadata)

This provides full audit trails without additional logging infrastructure. The projection-emission cycle inherently records its own history.

### Reproducibility

Given the same projected resources and execution logic, the same results are emitted. This enables:
- **Deterministic operations**: Re-running projections produces consistent results
- **Testing**: Mock stores with known resources enable testing projection-emission cycles
- **Debugging**: Reproducing issues by projecting the same historical resources

Immutability of resources guarantees that historical projections remain reproducible—the resources never change.

## Examples Across the Platform

### Component CRUD Operations

**Create**:
- Project: Artifact resources submitted by client
- Execute: Validate artifacts, build component structure
- Emit: Component definition resource
- Store: Component available for projection

**Read**:
- Project: Component definition and referenced resources
- Execute: Assemble complete component container
- Emit: None (read-only operation, but query results could be cached as emissions)
- Store: No new resources (unless caching results)

**Update**:
- Project: Existing component definition, new artifact resources
- Execute: Validate changes, build updated component
- Emit: New component definition resource (original unchanged)
- Store: Updated component available, original remains for versioning

**Delete**:
- Project: Component definition, dependency references
- Execute: Verify no dependencies, mark component as deleted
- Emit: Deletion marker or updated catalog
- Store: Component no longer appears in catalogs (but resources remain for history)

### Instance Lifecycle

**Start**:
- Project: Component definition
- Execute: Initialize instance state, begin execution
- Emit: Instance resource with initial state
- Store: Instance available for monitoring

**Run**:
- Project: Instance state, component definition
- Execute: Continue instance execution
- Emit: Logs, metrics, application data, state updates
- Store: Continuous stream of emitted resources

**Stop**:
- Project: Running instance resource
- Execute: Graceful shutdown
- Emit: Final state, shutdown logs
- Store: Instance state preserved

**Restart**:
- Project: Stopped instance resource
- Execute: Reinitialize from last state
- Emit: New instance resource (new start)
- Store: New instance lifecycle begins

### View Materialization

**Materialize**:
- Project: Resources matching view criteria
- Execute: Transform to view format, aggregate
- Emit: Materialized view resource
- Store: View available for efficient queries

**Refresh**:
- Project: Current view, new resources since last materialization
- Execute: Update view with new data
- Emit: Updated view resource
- Store: Fresh view available

**Query**:
- Project: Materialized view resource
- Execute: Filter/search within view
- Emit: Query results (potentially cached)
- Store: Results available for reuse

## The Generative Property

Each emission adds to the store. Over time, the store grows richer:

- More components → more capabilities
- More instances → more application data
- More logs → better observability
- More views → more ways to understand the system

This is **generative**—the system becomes more capable through use. Every operation contributes to the knowledge base available for future projections.

The projection-emission cycle creates a **flywheel effect**: more emissions enable richer projections, which enable more sophisticated operations, which produce richer emissions.

## Invariants

The cycle maintains critical invariants:

**Store Immutability**: Resources never change after emission. History is preserved.

**Projection Purity**: Projections don't modify the store. They're reproducible queries.

**Emission Atomicity**: Resources are either fully emitted or not emitted (no partial writes).

**CID Integrity**: Emitted resources are correctly identified by content hash.

**Reference Validity**: CID references in emitted resources refer to actual stored resources.

These invariants enable reasoning about the system—projections are safe queries, emissions are durable writes, the store is a reliable foundation.

## Contrast with Traditional Systems

Traditional systems often separate:
- **Data storage** (databases, filesystems)
- **Computation** (application logic)
- **Output** (write operations, side effects)

Hologram unifies these through the projection-emission cycle:
- Storage, computation, and output are phases of a single cycle
- All operations follow the same pattern
- The boundary between platform and application blurs (both use projection-emission)

This unification provides consistency—learning one operation teaches the pattern for all operations.

## Next Steps

The projection-emission cycle operates on containers. The next document, **Container Projections**, describes how projections transform undifferentiated resources into containers with structure, meaning, and purpose.