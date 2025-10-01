# The Projection Engine

## Executing Projections

The **projection engine** is the component that interprets projection definitions and executes them against the container store to produce containers.

This document describes how the engine works, its architecture, execution model, and how it orchestrates the projection-emission cycle.

## Engine Responsibilities

The projection engine:

**Interprets Projection Definitions**: Reads projection definition resources from the store and parses them into executable instructions.

**Executes Queries**: Evaluates query criteria against the store to identify matching resources.

**Traverses References**: Follows CID references to aggregate related resources according to aggregation rules.

**Applies Transformations**: Structures aggregated resources into containers according to transformation logic.

**Manages Execution Context**: Maintains state during projection execution (visited resources, depth tracking, intermediate results).

**Coordinates Composition**: Orchestrates nested and sequential projections, managing dependencies between them.

**Handles Errors**: Validates constraints, reports failures, ensures projections complete successfully or fail cleanly.

The engine is the **interpreter** for the projection language—it gives operational meaning to declarative projection definitions.

## Execution Model

### Projection Request

A projection execution begins with a **request** specifying:
- **Projection Definition CID**: Which projection to execute
- **Parameters**: Values for parameterized queries or transformations
- **Context**: Additional information (user identity, environment, temporal constraints)

The engine retrieves the projection definition resource, validates it, and begins execution.

### Three-Phase Execution

The engine executes projections in three phases, corresponding to the projection mechanism:

#### Phase 1: Query Evaluation (Identification)

The engine evaluates query criteria from the projection definition against the container store.

Steps:
1. Parse query specification from projection definition
2. Apply parameters to customize query
3. Execute query against store (using store's query capability)
4. Receive resource set (CIDs of matching resources)
5. Validate resource set meets minimum requirements (if specified)

Result: A set of CIDs identifying resources to aggregate.

#### Phase 2: Reference Traversal (Aggregation)

The engine follows references from the resource set to collect related resources.

Steps:
1. Parse aggregation rules from projection definition
2. Initialize traversal state (visited set, depth counter)
3. For each resource in the set:
   - Retrieve resource content from store
   - Extract CID references from content
   - Determine which references to follow (based on rules)
   - Recursively retrieve referenced resources
   - Track relationships in aggregated graph
4. Apply depth limits and cycle detection
5. Validate aggregated resources meet conformance requirements

Result: An aggregated resource graph with content and relationships.

#### Phase 3: Transformation (Structuring)

The engine transforms the aggregated graph into container format.

Steps:
1. Parse transformation logic from projection definition
2. Extract required fields from resources
3. Combine resources according to structure rules
4. Apply formatting and encoding conversions
5. Compute derived values
6. Order and filter based on container requirements
7. Validate final container structure

Result: A structured container ready for use.

### Execution Context

The engine maintains execution context throughout projection:

**Visited Resources**: Set of CIDs already retrieved during traversal, preventing redundant retrieval and cycle detection.

**Depth Tracking**: Current traversal depth for enforcing depth limits.

**Error State**: Collection of validation failures or constraint violations encountered during execution.

**Intermediate Results**: Partial containers or computed values used across phases.

**Parameter Bindings**: Resolved parameter values used throughout execution.

Context is isolated per projection—concurrent projections do not share context or interfere with each other.

## Query Evaluation

The engine delegates query evaluation to the container store's query capability, but orchestrates the process:

**Query Translation**: Convert projection definition's query specification into store-specific query format.

**Query Execution**: Invoke store query, handling pagination or streaming for large result sets.

**Result Filtering**: Apply additional filters that the store cannot evaluate (complex content patterns, computed predicates).

**Result Validation**: Ensure query results meet cardinality constraints (minimum/maximum resource counts).

The engine abstracts store-specific query details—the same projection definition works with different store implementations that have different query capabilities.

## Reference Traversal Algorithm

Reference traversal follows a **controlled graph traversal**:

```
function traverse(rootCIDs, rules, maxDepth):
    visited = empty set
    toVisit = queue of (CID, depth=0) from rootCIDs
    graph = empty aggregated graph

    while toVisit not empty:
        (cid, depth) = toVisit.dequeue()

        if cid in visited:
            continue
        if depth > maxDepth:
            continue

        visited.add(cid)
        content = store.retrieve(cid)
        graph.addResource(cid, content)

        references = extractReferences(content)
        for refCID in references:
            if shouldFollow(refCID, rules):
                toVisit.enqueue((refCID, depth+1))
                graph.addEdge(cid, refCID)

    return graph
```

Key aspects:

**Visited Tracking**: Prevents retrieving the same resource multiple times and handles reference cycles.

**Depth Limits**: Controls traversal extent, preventing unbounded graph exploration.

**Selective Following**: Rules determine which references to follow (by reference type, target resource properties).

**Breadth-First**: Queue-based traversal explores resources level by level (though depth-first is also valid).

## Transformation Pipeline

Transformation applies a sequence of operations to the aggregated graph:

**Extraction**: Select specific resources or fields from the graph.

**Combination**: Merge multiple resources into unified structures.

**Computation**: Derive values from resource content (counts, aggregations, computed fields).

**Formatting**: Convert encodings or representations (JSON to human-readable, binary to text).

**Filtering**: Remove resources or fields that don't meet criteria.

**Ordering**: Sort resources by specified properties.

Each operation takes the current intermediate result and produces a new intermediate result, building toward the final container structure.

## Conformance Validation

During aggregation, the engine validates that resources conform to requirements specified in the projection definition.

**Schema Validation**: Resources match expected structure (JSON schema, type definitions).

**Cardinality Constraints**: Required resources are present, optional resources handled correctly.

**Reference Integrity**: Referenced CIDs exist in the store and are reachable.

**Semantic Constraints**: Application-specific rules (version compatibility, naming conventions).

Validation failures:
- **Hard Failures**: Abort projection, return error (missing required resources)
- **Soft Failures**: Record warning, continue (optional resource not found)
- **Accumulation**: Collect all failures, report at completion

The engine provides detailed error information identifying which resources or constraints failed.

## Projection Composition

The engine handles composed projections where one projection uses another's results.

**Nested Projection**: During Phase 2 (aggregation), encounter a reference that requires sub-projection:
1. Suspend current projection
2. Initiate sub-projection with referenced projection definition
3. Execute sub-projection to completion
4. Incorporate sub-projection result into current aggregated graph
5. Resume current projection

**Sequential Projection**: One projection completes, results emitted to store, next projection projects the emitted resources:
1. Execute first projection
2. Emit results to store
3. Store returns CIDs of emitted resources
4. Execute second projection with emitted CIDs as query parameters
5. Continue chain

**Parallel Projection**: Multiple projections execute concurrently:
1. Engine spawns multiple execution contexts
2. Each context executes its projection independently
3. Contexts do not share state (pure queries, no interference)
4. Results collected as projections complete

Composition enables building complex containers from simple, reusable projection definitions.

## Caching and Optimization

The engine can optimize repeated projections:

**Result Caching**: Store completed container results, keyed by (projection definition CID, parameters). Repeated identical projections return cached results.

**Partial Caching**: Cache intermediate results (query results, aggregated graphs) for reuse in similar projections.

**Incremental Evaluation**: For dynamic projections over evolving stores, update previous results with only new/changed resources rather than complete re-evaluation.

**Query Planning**: Analyze projection definitions to optimize execution order (evaluate selective queries first, parallelize independent operations).

Caching is transparent—cached projections return identical results to fresh evaluation, just faster.

## Error Handling

The engine provides structured error information:

**Query Failures**: Store query errors (malformed query, store unavailable).

**Retrieval Failures**: Missing CIDs, corrupted resources, access denied.

**Validation Failures**: Resources don't conform to schema or constraints.

**Transformation Failures**: Cannot convert format, computation errors, invalid structure.

**Resource Errors**: Out of memory, timeout, execution limits exceeded.

Each error includes:
- Error type and description
- Affected resource CIDs
- Projection definition context (which phase, which rule)
- Suggestions for resolution (if applicable)

Errors propagate: sub-projection failures cause parent projection failure, providing complete failure trace.

## Execution Guarantees

The engine provides guarantees:

**Purity**: Projections do not modify the store or have side effects. Same input always produces same output.

**Isolation**: Concurrent projections do not interfere. Each has independent execution context.

**Completeness**: Successful projection returns complete container meeting all conformance requirements. Partial results are not returned.

**Determinism**: Given identical store state and parameters, projection produces identical results.

**Atomicity**: Projection either completes successfully or fails cleanly. No partial state persists.

These guarantees enable reliable reasoning about projections and composing them confidently.

## Engine Architecture

The engine consists of modular components:

**Definition Parser**: Reads projection definition resources, validates syntax, produces executable representation.

**Query Executor**: Translates and executes queries against store, handles results.

**Traversal Manager**: Implements reference traversal algorithm, manages visited tracking and depth limits.

**Transformation Engine**: Applies transformation pipeline, validates intermediate results.

**Conformance Validator**: Checks resources against schemas and constraints.

**Context Manager**: Maintains execution context, handles error state, provides telemetry.

**Composition Coordinator**: Manages nested and sequential projections, tracks dependencies.

Components interact through well-defined interfaces, enabling implementations in different languages or with different optimization strategies.

## Engine Implementation Independence

Like the container store, the projection engine is defined by its behavior, not implementation.

Valid engine implementations might:
- Interpret projection definitions directly (interpreter)
- Compile projection definitions to native code (compiler)
- Distribute execution across multiple nodes (distributed engine)
- Specialize for specific projection types (optimizing engine)

The platform works identically with any conforming engine implementation.

## Meta-Projections

The engine can project its own projection definitions—examining, analyzing, or transforming them.

**Projection Analysis**: Project all projection definitions, analyze query patterns, identify optimization opportunities.

**Projection Validation**: Project projection definitions, validate they're well-formed and reference valid resources.

**Projection Composition**: Project multiple projection definitions, synthesize new composed projection definition.

This reflexivity enables the platform to reason about and improve its own projection capabilities.

## Next Steps

The projection engine interprets projection definitions written in the **projection language**. The next document describes this language—how conformance requirements specify projection instructions, and how projections are defined as resources in the store.