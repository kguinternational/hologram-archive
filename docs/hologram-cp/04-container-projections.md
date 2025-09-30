# Container Projections

## From Resources to Containers

Resources in the container store exist as undifferentiated, content-addressed data without inherent structure or meaning. **Projections** transform resources into **containers**—structured collections with defined purpose, relationships, and semantics.

This document establishes how projections work, what containers are, and how the projection mechanism provides universal containment.

## What is a Container?

A **container** is a structured view of resources produced by a projection. Containers have:

**Identity**: A container is identified by the projection that produced it and the query parameters used (which resources, what time, what filters).

**Content**: A container contains or references specific resources from the store.

**Structure**: A container organizes resources according to the projection definition (aggregation rules, transformation logic).

**Purpose**: A container serves a defined role (component definition, running instance, documentation view, test suite).

**Behavior**: A container may define operations that can be performed with its resources (validate, execute, transform).

Containers are **ephemeral projections** or **materialized resources**:
- **Ephemeral**: The container exists during projection evaluation, used immediately, then discarded
- **Materialized**: The container is emitted as a resource to the store for persistent access

## Projection Mechanism

A projection transforms resources into containers through three stages:

### Stage 1: Identification

**Query evaluation** identifies resources in the store matching specified criteria.

Query criteria can include:
- **Content patterns**: Resources containing specific data (JSON with particular fields, text matching patterns)
- **CID references**: Resources referenced by known CIDs
- **Reference relationships**: Resources that reference or are referenced by other resources
- **Metadata constraints**: Resources with specific metadata (stored during a time range, accessed frequently)

The result is a **resource set**—the CIDs of matching resources.

### Stage 2: Aggregation

**Aggregation** collects related resources by following reference relationships.

Starting from the resource set, aggregation:
- Follows CID references to retrieve related resources
- Applies traversal rules (depth limits, reference type filters)
- Collects transitive closure (all reachable resources) or specific subgraphs
- Groups resources by relationships or properties

The result is an **aggregated resource graph**—resources and their relationships.

### Stage 3: Transformation

**Transformation** structures aggregated resources into container format.

Transformation might:
- Extract specific fields from resources
- Combine multiple resources into unified structure
- Apply formatting or encoding conversions
- Compute derived values from resource content
- Order or filter based on container requirements

The result is a **container**—the final structured collection ready for use.

## Projection Definitions

A **projection definition** is itself a resource in the store that specifies how to project resources into containers.

Projection definitions contain:
- **Query specification**: How to identify resources
- **Aggregation rules**: How to collect related resources
- **Transformation logic**: How to structure the container
- **Conformance requirements**: What constraints aggregated resources must satisfy

Because projection definitions are resources, they can be:
- Versioned (new projection definitions with different CIDs)
- Composed (projection definitions that reference other projection definitions)
- Projected (meta-projections that analyze or transform projection definitions)

This creates **recursive projections**—the system projects its own projection definitions.

## Container Types Through Projection

Different projection definitions create different container types. The platform provides base projection definitions for standard container types:

**Component Container**:
- Identifies: Resources with component namespace
- Aggregates: Spec, interface, documentation, tests, dependencies
- Transforms: Structured component definition
- Purpose: Reusable capability definition

**Instance Container**:
- Identifies: Resources with instance identifier
- Aggregates: Component definition, instance state, runtime context
- Transforms: Executable instance
- Purpose: Running component with state

**Interface Container**:
- Identifies: Resources with interface definitions
- Aggregates: Method signatures, type definitions, contracts
- Transforms: Interface specification
- Purpose: Contract between components

**Documentation Container**:
- Identifies: Documentation resources
- Aggregates: Docs by namespace, cross-references
- Transforms: Human-readable documentation
- Purpose: Understanding and guidance

**Test Container**:
- Identifies: Test resources
- Aggregates: Test cases, expected results, validation logic
- Transforms: Executable test suite
- Purpose: Validation and verification

**View Container**:
- Identifies: Resources matching view criteria
- Aggregates: According to view definition
- Transforms: Materialized view format
- Purpose: Efficient access to projected data

Each container type is defined by its projection definition. New container types are created by emitting new projection definitions.

## Projection Composition

Projections can compose—one projection can use another's results as input.

**Nested Projection**: A component container projects interface containers for each interface it references. The interface projections run as sub-projections of the component projection.

**Sequential Projection**: Create component → project component → create instance → project instance. Each projection's output becomes input to the next operation.

**Parallel Projection**: Multiple projections execute concurrently over the same store. A documentation view and a test view project the same components simultaneously.

Composition enables building complex containers from simple projections without implementing complex monolithic projection logic.

## Projection Parameters

Projections can accept parameters that customize their behavior:

**Resource Filters**: Project only resources matching specific criteria (namespace, time range, content patterns).

**Depth Limits**: Control how deeply to follow references during aggregation.

**Transformation Options**: Enable optional transformations (include/exclude certain fields, format preferences).

**Temporal Constraints**: Project resources as they existed at a specific point in time (time-travel queries).

Parameterized projections enable reusing projection definitions for different contexts without creating multiple definition variants.

## The Projection Graph

Executing multiple projections creates a **projection graph**:

- **Nodes**: Containers produced by projections
- **Edges**: Dependencies where one projection uses another's results

The projection graph shows relationships between containers:
- Component containers reference interface containers
- Instance containers reference component containers
- View containers reference multiple other container types

The projection graph is distinct from the resource reference graph:
- Resource graph: Immutable CID references between resources
- Projection graph: Ephemeral relationships between projected containers

## Containers vs Resources

Key distinctions:

**Resources**:
- Immutable content in the store
- Identified by CID (content hash)
- No inherent structure or meaning
- Permanent (until garbage collected)

**Containers**:
- Structured projections of resources
- Identified by projection definition + parameters
- Defined structure and purpose
- Ephemeral (exist during use) or materialized (emitted as resources)

A resource can participate in multiple containers. The same documentation resource might appear in:
- A component container (as component docs)
- A documentation view container (as part of searchable docs)
- A tutorial container (as example content)

Projection determines how the resource appears and what role it serves.

## Projection Purity

Projections are **pure queries**—they do not modify the store or have side effects.

Properties:
- **Deterministic**: Same store state and parameters produce same container
- **Repeatable**: Projecting multiple times yields consistent results
- **Concurrent**: Multiple projections can run simultaneously without interference
- **Cacheable**: Projection results can be cached and reused

Purity enables:
- **Testing**: Projections can be tested with known resource sets
- **Debugging**: Reproducing projections for investigation
- **Optimization**: Caching projection results for performance

Side effects (creating resources, executing stateful operations) occur during the **Execute** phase of the projection-emission cycle, not during projection itself.

## Temporal Projections

Because resources are immutable, the store preserves history. **Temporal projections** project resources as they existed at a previous time.

A temporal projection:
1. Identifies resources that existed at the specified time (based on store metadata)
2. Aggregates using only references that existed at that time
3. Produces a container representing historical state

This enables:
- **Versioning**: View previous versions of components or instances
- **Audit**: Understand what resources were available when operations occurred
- **Debugging**: Reproduce historical state to investigate issues

Temporal projections work because resources never change—a CID always refers to the same content, regardless of when you project it.

## Dynamic Projections

Some projections depend on runtime state or external context:

**Instance Projections**: Project current instance state, which changes as the instance emits new resources.

**Live View Projections**: Project continuously, incorporating new resources as they're emitted.

**Contextual Projections**: Include user-specific or environment-specific resources (user permissions, deployment configuration).

Dynamic projections are still pure queries against the store's current state, but that state evolves through emissions, causing projection results to change over time.

## Projection Performance

Projection performance depends on:

**Store Query Efficiency**: How quickly the store can identify matching resources (indexing, caching).

**Graph Traversal**: How many references must be followed during aggregation (depth, branching factor).

**Transformation Complexity**: How much computation is required to structure the container.

**Resource Size**: How much data must be retrieved and processed.

Optimization strategies:
- **Materialized Views**: Pre-compute and emit common projections as resources
- **Incremental Updates**: Update views with only new/changed resources rather than full reprojection
- **Lazy Aggregation**: Follow references only as needed rather than complete traversal
- **Query Optimization**: Index store metadata for fast resource identification

## Projection as Lens

Projections act as **lenses** that reveal different aspects of the store:

- A component lens reveals reusable definitions
- An instance lens reveals running state
- A documentation lens reveals human-readable content
- A dependency lens reveals relationship graphs

The same underlying resources appear differently through different lenses. There is no "true" view—all containers are valid projections serving different purposes.

This multiplicity enables flexible organization: structure resources however projections require without committing to a single schema or hierarchy.

## Next Steps

Projections are defined and executed by the **Projection Engine**, which interprets projection definitions and orchestrates the identification-aggregation-transformation pipeline.

The next section, Part II: The Engine, details how the projection engine works, the projection language for defining projections, and how emissions create new resources for future projections.