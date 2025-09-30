# The Emission Model

## How Containers Emit Resources

**Emission** is the process by which execution results are written back to the container store as new resources. Emissions complete the projection-emission cycle, making execution results available for future projections.

This document describes the emission model—what emissions are, how they work, emission types, and how they integrate into the platform.

## What is an Emission?

An **emission** is the creation of a new resource in the container store as a result of executing a projection.

Emissions have these properties:

**Content**: The data being emitted (bytes, structured data, references to other resources).

**Identity**: The CID computed from the content, uniquely identifying the emission.

**Immutability**: Once emitted, the resource never changes (consistent with store immutability).

**Addressability**: The emission is immediately available for retrieval and projection via its CID.

**Atomicity**: The emission is either fully stored or not stored (no partial emissions).

Emissions transform ephemeral execution results into durable resources.

## The Emission Process

### Phase 1: Result Generation

During the **Execute** phase of the projection-emission cycle, operations produce results:
- Validated component definitions
- Instance state snapshots
- Log entries
- Computed views
- Application data
- Metadata

Results exist initially as ephemeral execution artifacts (in-memory structures, temporary files).

### Phase 2: Serialization

Results are **serialized** into a canonical byte representation:

**Structured Data**: Converted to canonical JSON (sorted keys, consistent formatting).

**Binary Data**: Used as-is (already bytes).

**References**: Embedded as CID strings within serialized content.

**Metadata**: Included in serialized representation or as separate associated resource.

Serialization produces deterministic byte sequences—the same result always serializes identically.

### Phase 3: Content Addressing

The serialized content is **hashed** to compute the CID:
- Apply cryptographic hash function (SHA-256 or similar)
- Produce fixed-length hash digest
- Format as CID (typically "cid:" prefix + hex digest)

Content addressing ensures identical content produces identical CIDs (deduplication) and enables integrity verification.

### Phase 4: Storage

The serialized content and CID are **stored** in the container store:
- Store maps CID to content
- Content becomes retrievable via the CID
- Store updates indexes and metadata
- Reference graph is extended if content contains CID references

Once stored, the emission is durable and available for projection.

### Phase 5: Reference Return

The emission process returns the CID to the caller (the operation that produced the result).

The caller can:
- Return the CID to the client (for user-initiated operations)
- Use the CID in further processing (as input to subsequent projections)
- Emit additional resources that reference the new CID

Returned CIDs enable chaining—one emission becomes input to the next operation.

## Emission Types

Different operations emit different types of resources.

### Definition Emissions

**Component Definitions**: Results of creating or updating components.

Content:
- Component specification
- References to interface, documentation, tests, dependencies
- Version information
- Namespace and metadata

These emissions define reusable capabilities available for instantiation.

### State Emissions

**Instance State**: Snapshots of running instance state.

Content:
- Current variable values
- Execution position
- Pending tasks
- Resource allocations
- State timestamp

State emissions enable persistence, recovery, and debugging.

### Log Emissions

**Log Entries**: Records of events during execution.

Content:
- Timestamp
- Log level (info, warning, error)
- Message
- Context (operation, resource, user)
- References to related resources

Log emissions provide observability and audit trails.

### Data Emissions

**Application Data**: User content, computed results, generated artifacts.

Content:
- Application-specific structured data
- Files or binary artifacts
- User input or user-generated content
- Computed aggregations or transformations

Data emissions are the primary output of running instances.

### View Emissions

**Materialized Views**: Pre-computed projections stored for efficient access.

Content:
- Query results
- Aggregated resources
- Transformed representations
- View metadata (freshness timestamp, query parameters)

View emissions optimize repeated queries.

### Metadata Emissions

**Operational Metadata**: Information about operations themselves.

Content:
- Operation start/end timestamps
- Resources projected during operation
- Execution duration
- Success/failure status
- Error details

Metadata emissions enable monitoring and analysis.

## Emission Streams

Some operations produce **continuous emissions** rather than single emissions.

**Running Instances**: Emit logs, state snapshots, and application data continuously while running.

**Live Views**: Emit updated view resources as underlying resources change.

**Event Streams**: Emit event resources as events occur (user actions, external triggers).

Emission streams are sequences of individual emissions, each producing a distinct resource with distinct CID.

### Buffering and Batching

High-frequency emission streams may use buffering:
- Collect multiple emission candidates in memory
- Batch serialize and store together
- Reduce store overhead from individual tiny emissions

Batching is transparent—logically each emission is distinct, even if physically stored in a batch.

### Stream Termination

Emission streams terminate when:
- Instance stops (graceful or forced)
- View subscription is cancelled
- Event source is closed

Termination may produce a final emission marking stream completion.

## Emission References

Emitted resources can reference other resources via CID references.

**Parent References**: A log emission references the instance that produced it.

**Dependency References**: A component definition references interface and documentation resources.

**Sequential References**: A new component version references the previous version.

**Aggregation References**: A view emission references all resources it aggregated.

References form the resource graph, enabling:
- Tracing relationships (find all logs for an instance)
- Versioning (follow version chains)
- Garbage collection (determine reachability)

## Emission Deduplication

If emitted content is identical to an existing resource, the existing CID is returned without storing duplicate content.

This occurs naturally from content addressing:
1. Serialize result
2. Compute CID
3. Check if CID exists in store
4. If yes, return existing CID (no storage needed)
5. If no, store content and return new CID

Deduplication is automatic and transparent—callers always receive correct CID regardless of whether content was newly stored or already existed.

## Emission Validation

Before emitting, the platform may validate that the result conforms to expected structure:

**Schema Validation**: Ensure emitted resource matches schema for its type.

**Reference Integrity**: Ensure referenced CIDs exist in the store.

**Constraint Satisfaction**: Ensure business rules or semantic constraints are met.

Validation failures prevent emission—invalid resources are not stored. The operation reports error and can retry or abort.

## Emission Atomicity

Emissions are atomic operations:

**All-or-Nothing**: Content is either fully stored and indexed, or not stored at all (no partial writes).

**Consistent State**: Store remains consistent whether emission succeeds or fails.

**Isolation**: Concurrent emissions don't interfere—each produces independent resource.

Atomicity enables reliable reasoning: if emission succeeds, the resource is available and correctly formed.

## Emission Transactions

Operations that emit multiple related resources can use **emission transactions**:

1. Prepare all resources to emit
2. Compute all CIDs
3. Validate all resources
4. Store all resources atomically
5. Update store indexes

If any resource fails validation, the entire transaction aborts—no resources are emitted.

Transactions ensure related resources appear together (component definition with all conformance resources) or not at all.

## Emission Idempotence

Repeating an emission operation with identical input produces identical output:
- Same result content
- Same CID
- Store state unchanged (deduplication)

Idempotence enables safe retries after failures—re-emitting is harmless.

## Emission Observability

The platform provides visibility into emissions:

**Emission Logs**: Record of what was emitted, when, by what operation.

**Emission Metrics**: Count of emissions, size distribution, emission rate.

**Emission Traces**: Which projections led to which emissions (operation chains).

Observability helps understand platform behavior and debug issues.

## Emissions as First-Class Resources

Emitted resources are indistinguishable from any other resources in the store:
- Same content addressing
- Same immutability
- Same projectionability
- Same lifecycle

There is no special "emission" type—emissions simply add resources to the store.

This uniformity means:
- Emissions can be projected like any resources
- Emissions can reference and be referenced by any resources
- Platform and application emissions are treated identically

## The Feedback Loop

Emissions create a feedback loop:

1. **Project** resources from store
2. **Execute** operation on projected resources
3. **Emit** results as new resources to store
4. New resources become available for projection
5. Future projections include newly emitted resources
6. **Repeat**

Each cycle enriches the store. Over time:
- More components → more operations possible
- More logs → better observability
- More state → more sophisticated recovery
- More views → more efficient queries

The feedback loop makes the platform **generative**—it becomes more capable through use.

## Emission Policies

The platform may define policies governing emissions:

**Retention**: How long emissions persist before garbage collection.

**Rate Limits**: Maximum emission rate to prevent store overload.

**Size Limits**: Maximum size of individual emissions.

**Access Control**: Who can emit what types of resources.

**Schema Requirements**: What schemas emissions must conform to.

Policies ensure store health and enforce organizational requirements.

## Emission Patterns

Common emission patterns across operations:

**Create-Emit**: Create new resource, emit immediately (component creation).

**Accumulate-Emit**: Collect results over time, emit periodically (batched logs).

**Transform-Emit**: Project resources, transform, emit result (view materialization).

**Trigger-Emit**: External event triggers emission (user action creates data).

**Replicate-Emit**: Emit copy of resource from another store (synchronization).

Patterns provide templates for implementing new operations.

## Emission and Versioning

Every emission creates a new version:
- New component definition with changes → new CID, new version
- Updated instance state → new CID, new state snapshot
- Refreshed view → new CID, new view version

Old versions remain in store (immutability). Version history is automatically preserved through distinct CIDs.

Clients can reference specific versions (by CID) or request "latest" (projection query for most recent by timestamp).

## Emission Performance

Emission performance depends on:

**Serialization Cost**: Time to convert results to bytes.

**Hashing Cost**: Time to compute CID.

**Store Write Latency**: Time for store to persist and index resource.

**Validation Cost**: Time to validate before emission.

Optimization strategies:
- Efficient serialization formats
- Fast hash algorithms
- Asynchronous store writes (return CID before write completes)
- Batch emissions

High emission rate is critical for interactive applications and streaming data.

## Contrast with Traditional Systems

Traditional systems separate computation output from storage:
- Computation produces results
- Separate storage operation writes results to database/filesystem
- Multiple result formats (database rows, files, caches)

Hologram unifies:
- Emission is the canonical output mechanism
- Emitted resources are immediately storable and projectable
- Single format (content-addressed resources)

Unification simplifies the platform—everything flows through the same mechanism.

## Next Steps

With the projection engine executing projections and the emission model creating new resources, the platform provides a complete cycle for operations.

The next section, Part III: The Platform, describes the **Container Types** provided by the platform—the specific projection definitions and emission patterns for components, instances, interfaces, documentation, tests, views, and more.