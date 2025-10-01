# Platform Views

## Materialized Container Projections

**Views** are persistent materializations of projections—pre-computed container projections emitted as resources for efficient repeated access. Views optimize common queries by storing projection results rather than re-projecting on every access.

This document describes platform views, their materialization process, refresh strategies, and role in the platform architecture.

## What is a View?

A **view** is a container produced by projecting resources, then emitted as a resource itself for reuse.

Views have dual nature:

**As Containers**: Views are projected containers—results of executing projection definitions against the store.

**As Resources**: Views are emitted to the store as resources, making them durable and projectable themselves.

This duality enables:
- Expensive projections computed once, accessed many times
- Projections of projections (meta-views)
- Version control of view states (immutable CIDs)
- Distribution of views between stores

## View Materialization

**Materialization** is the process of executing a projection and emitting the result as a view resource.

### Materialization Process

**Phase 1: Projection Execution**

Execute the view's projection definition:
1. Query store to identify resources
2. Aggregate related resources by following references
3. Transform aggregated resources into view structure
4. Validate view satisfies conformance requirements

Result: A view container (ephemeral, in-memory).

**Phase 2: Emission**

Emit the view container as a resource:
1. Serialize view container to canonical format
2. Compute CID from serialized content
3. Store view resource in container store
4. Index view for efficient retrieval

Result: A view resource with CID, available for projection.

**Phase 3: Catalog Update**

Update the view catalog:
1. Record view existence (CID, view type, parameters)
2. Record materialization timestamp
3. Record source resources (what was projected)
4. Index by view type and parameters for discovery

Result: View discoverable through catalog queries.

## View Types

Different view types serve different purposes.

### Catalog Views

**Purpose**: Index available resources for discovery.

**Content**:
- List of all components with metadata (namespace, version, description)
- List of all instances with status (running, stopped)
- List of available interfaces
- Component dependency graph

**Refresh**: Periodic or on-demand when components added/removed.

**Usage**: Clients query catalog views to discover what's available.

### Component Views

**Purpose**: Denormalized component information for fast access.

**Content**:
- Complete component definition
- Inlined interface definitions (rather than just references)
- Rendered documentation (HTML from Markdown)
- Test result summaries

**Refresh**: When component is updated.

**Usage**: Displaying component details without traversing references.

### Instance Views

**Purpose**: Runtime state and observability.

**Content**:
- Current instance state
- Recent logs (last N entries)
- Performance metrics
- Resource utilization

**Refresh**: Continuous or frequent (real-time view).

**Usage**: Monitoring dashboards, debugging, operations.

### Log Views

**Purpose**: Aggregated logs for searching and analysis.

**Content**:
- Log entries grouped by instance/component/time
- Indexed by timestamp, level, source
- Aggregated statistics (error counts, rates)

**Refresh**: Continuous as logs emitted, periodic aggregation.

**Usage**: Log search, troubleshooting, audit.

### Dependency Views

**Purpose**: Visualize and analyze component relationships.

**Content**:
- Complete dependency graph
- Transitive dependency closure
- Reverse dependencies (what depends on this)
- Dependency conflict detection

**Refresh**: When components or dependencies change.

**Usage**: Understanding system structure, impact analysis.

### Documentation Views

**Purpose**: Searchable, navigable documentation.

**Content**:
- All documentation resources indexed by topic/component
- Cross-references resolved
- Search index for full-text queries
- Table of contents and navigation structure

**Refresh**: When documentation updated.

**Usage**: Documentation portals, search interfaces.

### Spec Views

**Purpose**: Export component definitions for version control.

**Content**:
- Component definitions in filesystem-friendly format
- Directory structure mirroring component namespaces
- Human-readable JSON formatting

**Refresh**: On-demand when user wants to sync to filesystem.

**Usage**: Git commits, external tool integration, review.

## View Refresh Strategies

Views can become stale as underlying resources change. **Refresh strategies** determine when and how to update views.

### On-Demand Refresh

View refreshed when explicitly requested by client.

**Advantages**:
- No background computation cost
- View freshness controlled by client

**Disadvantages**:
- First access after staleness incurs refresh latency
- Clients must know when refresh is needed

**Applicable**: Views where staleness is acceptable (historical snapshots, archived data).

### Periodic Refresh

View refreshed at regular intervals (hourly, daily).

**Advantages**:
- Bounded staleness (at most one period old)
- Simple scheduling
- Predictable refresh cost

**Disadvantages**:
- May refresh unnecessarily (if no changes)
- Still can be stale between refreshes

**Applicable**: Catalog views, dashboards, reports.

### Incremental Refresh

View updated with only new/changed resources since last refresh.

**Advantages**:
- Lower cost than full refresh (only process changes)
- Keeps view fresher
- Scales better with large views

**Disadvantages**:
- More complex implementation
- Requires tracking changes

**Applicable**: Log views, event streams, time-series data.

### Invalidation-Based Refresh

View refreshed when underlying resources change.

**Advantages**:
- View always fresh (no staleness)
- No unnecessary refreshes (only when needed)

**Disadvantages**:
- Requires change tracking/notification
- May refresh too frequently if resources change often

**Applicable**: Critical views (security, availability), small views with fast refresh.

### Lazy Refresh

View refreshed on first access after invalidation.

**Advantages**:
- Combines invalidation detection with on-demand refresh
- No refresh cost if view not accessed

**Disadvantages**:
- First post-invalidation access has latency

**Applicable**: Frequently invalidated but infrequently accessed views.

## View Versioning

Because views are resources with CIDs, they're naturally versioned.

Each refresh produces a new view resource with a new CID. The old view remains (immutability).

This enables:

**Temporal Queries**: Project the view as it existed at a specific time (by CID or timestamp).

**Consistency**: Multiple queries against the same view CID see identical data (no concurrent modifications).

**Auditing**: Complete history of view states preserved.

**Rollback**: If refresh produces incorrect view, use previous version.

The view catalog tracks view versions, maintaining:
- Current view CID (latest refresh)
- Previous view CIDs (history)
- Refresh timestamps
- Underlying resource versions

## View Consistency

Views can have different consistency guarantees.

### Eventually Consistent Views

View may lag behind store state. Projecting a view might return data from before recent emissions.

**Advantages**: Lower refresh cost, better performance.

**Disadvantages**: Stale data possible.

**Applicable**: Most views (catalogs, dashboards, documentation).

### Strongly Consistent Views

View reflects all emissions up to query time.

**Advantages**: No staleness, accurate data.

**Disadvantages**: Higher cost (frequent refresh or projection without materialization).

**Applicable**: Critical operational views (instance status, security).

### Snapshot Consistent Views

View reflects store state at specific point in time (consistent snapshot).

**Advantages**: Consistent data (all resources from same timepoint).

**Disadvantages**: Intentionally stale (snapshot timestamp < current time).

**Applicable**: Reports, analytics, historical analysis.

## View Queries

Views are resources, so they can be projected. **View queries** project views to extract information.

### Direct View Projection

Retrieve entire view resource by CID, use contents directly.

Fast (single resource retrieval), but returns entire view.

### Filtered View Projection

Project view, apply filters to select subset.

More flexible than direct retrieval, enables searching within view.

### View Composition

Project multiple views, combine results.

Enables answering queries that span view types (components with instances + logs).

### Parameterized View Projection

Views can be parameterized (by namespace, time range, filters).

Projection binds parameters, retrieves specific view variant.

Enables view families (component view for each namespace) without storing every variant.

## View Caching

Views serve as caches—materialized projections avoid re-computation.

**Cache Hit**: Projecting a view retrieves materialized result (fast).

**Cache Miss**: View doesn't exist or is stale, requires refresh (slow).

**Cache Invalidation**: Determine when view is stale and needs refresh.

Standard caching challenges apply:
- How to detect staleness
- When to refresh
- How to balance freshness vs cost

View refresh strategies are cache invalidation policies.

## Views and Storage Backend

Views can be stored in the same backend as resources or in specialized storage:

**Same Backend**: Views are resources like any others (simple, consistent).

**Specialized Storage**: Views in database optimized for queries (relational DB, search index).

**Hybrid**: Frequently accessed views in fast storage, others in standard storage.

The view model is independent of storage—views are logically resources regardless of physical storage.

## View Materialization Cost

Materializing views has costs:

**Projection Cost**: Time and resources to execute projection (query, aggregate, transform).

**Storage Cost**: Space to store materialized view resources.

**Refresh Cost**: Overhead of detecting changes and updating views.

**Consistency Cost**: Ensuring view matches store state.

Optimization strategies:

**Selective Materialization**: Only materialize frequently accessed views.

**Incremental Refresh**: Update rather than rebuild views.

**Approximate Views**: Accept approximate results for lower cost (sampling, estimation).

**Lazy Materialization**: Defer materialization until first access.

## Views as Projections of Projections

Views are resources, so views can project other views—**meta-views**.

A summary view might project detail views:
- Daily log view projects hourly log views
- System overview view projects component and instance views
- Dashboard view projects multiple operational views

Meta-views enable hierarchical aggregation and composition without accessing underlying resources repeatedly.

## View Definition

View definitions are projection definitions with additional metadata:

**Projection Definition**: How to project resources into view.

**Refresh Strategy**: When and how to refresh view.

**Retention Policy**: How long to keep old view versions.

**Indexing Hints**: What fields to index for efficient queries.

**Access Patterns**: Expected query patterns (optimize for these).

View definitions are resources in the store, enabling new view types to be added by emitting new view definitions.

## Views and Garbage Collection

Views are derived—they can always be rebuilt from underlying resources.

This makes views candidates for garbage collection:
- Remove old view versions beyond retention policy
- Remove unused views to free storage
- Rebuild views from resources if needed

Views provide the **denormalization** tradeoff: storage cost vs query performance.

## The Spec View

The **spec view** deserves special attention—it's the view of component definitions formatted for filesystem storage and version control.

**Purpose**: Enable git-based workflow for component definitions.

**Content**: Component definition resources serialized as filesystem hierarchy:
- Directory per component namespace
- Files per resource type (spec, interface, docs, tests)
- Human-readable formatting (pretty-printed JSON)
- Index files for navigation

**Refresh**: On-demand when developer wants to commit changes.

**Bidirectional**: Resources can be read from spec view (import from git) or written to spec view (export for git).

The spec view bridges Hologram (content-addressed store) with traditional version control systems (filesystem + git).

## Next Steps

Views are consumed through operations—projection executions that read resources, potentially materialize views, and emit results.

The next document, **Platform Operations**, describes the operations provided by the platform—how they use projection-emission cycles, what inputs they require, and what outputs they produce.