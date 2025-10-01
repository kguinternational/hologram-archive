# The Container Store

## Universal Resource Storage

The container store is the foundation of the Hologram platform. It is a universal storage system where all resources exist as immutable, content-addressed data without inherent type or structure.

This document establishes the principles, capabilities, and properties of the container store from first principles.

## First Principles

### Resources as Undifferentiated Data

A **resource** is any sequence of bytes stored in the container store. Resources have no inherent type, schema, or semantic meaning. A JSON document, a binary executable, an image file, a text log—all exist as resources without distinction.

The store does not interpret, validate, or classify resources at write time. It stores bytes and provides mechanisms for retrieval and reference. Meaning emerges later through projection, not at storage time.

This principle enables universal applicability—any data can be stored, and any interpretation can be applied through projections.

### Content-Addressed Identity

Every resource is identified by a **Content Identifier (CID)**, a cryptographic hash computed from the resource's complete content.

The CID function has these properties:

- **Deterministic**: The same content always produces the same CID
- **Unique**: Different content produces different CIDs (with cryptographic collision resistance)
- **Opaque**: The CID reveals nothing about content structure or meaning
- **Immutable**: Content cannot change without changing the CID

CIDs provide **intrinsic identity**—identity derived from content itself rather than assigned externally. This makes resources self-verifying: retrieving a resource by CID guarantees you receive exactly the content that produced that CID.

### Immutability

Resources are **write-once**. Once stored, a resource's content never changes. The CID permanently identifies that specific content.

If content needs to change, a new resource with new content is written, producing a new CID. The original resource remains unchanged and available.

Immutability provides:

- **Reliable references**: A CID always refers to the same content
- **Historical record**: All versions persist as distinct resources
- **Concurrent access**: No coordination needed for reads (content never changes)
- **Distribution**: Resources can move between stores with identity intact

### References as CIDs

Resources can reference other resources by including CIDs in their content. A JSON resource might contain:

```
{
  "spec": "cid:abc123...",
  "interface": "cid:def456...",
  "documentation": "cid:789ghi..."
}
```

These references form a **directed graph** where nodes are resources and edges are CID references. The graph is immutable—references never change—but new resources can reference existing resources, extending the graph.

The reference graph enables:

- **Aggregation**: Collecting related resources by following references
- **Composition**: Building complex structures from simple resources
- **Deduplication**: Multiple resources can reference the same resource
- **Versioning**: New versions reference previous versions

## Store Capabilities

The container store provides four fundamental capabilities.

### 1. Store

**Operation**: Given content (bytes), compute the CID and store the content indexed by CID.

**Properties**:
- Idempotent: Storing identical content multiple times produces the same CID and stores content once
- Atomic: Content is either fully stored or not stored (no partial writes)
- Durable: Once stored, content persists until explicitly deleted

**Result**: The CID identifying the stored resource.

This operation makes resources available for retrieval and projection.

### 2. Retrieve

**Operation**: Given a CID, retrieve the corresponding resource content.

**Properties**:
- Deterministic: The same CID always retrieves the same content
- Verified: Retrieved content can be hashed to verify it matches the CID
- Complete: Resources are retrieved entirely (no partial retrieval in base operation)

**Result**: The complete content of the resource, or an indication that the CID is not present in the store.

This operation makes stored resources accessible.

### 3. Reference

**Operation**: Parse a resource's content to extract CIDs that reference other resources.

**Properties**:
- Format-dependent: Different content types encode references differently (JSON uses strings, binary formats use specific fields)
- Transitive: Following references from referenced resources builds the reference graph
- Graph-forming: References create a directed graph structure over resources

**Result**: A set of CIDs referenced by the resource.

This operation enables graph traversal and aggregation.

### 4. Query

**Operation**: Identify resources matching specified criteria (content patterns, reference relationships, metadata constraints).

**Properties**:
- Set-valued: Queries return zero or more matching resources
- Composable: Query results can be inputs to other queries
- Projection-enabling: Queries identify resources to aggregate into containers

**Result**: A set of CIDs for resources matching the query criteria.

This operation enables projections to identify relevant resources.

## Store Properties

These capabilities provide foundational properties that the projection engine and platform rely upon.

### Deduplication

Identical content produces the same CID. If the same content is stored multiple times, it occupies storage space once but can be referenced by multiple CID references.

This eliminates redundancy—shared resources (common dependencies, standard interfaces, repeated data) exist once in the store regardless of how many containers reference them.

### Integrity

Retrieved content can be verified by computing its hash and comparing to the CID. Any corruption, tampering, or transmission error is detectable.

This makes the store **trustless**—you can retrieve resources from untrusted sources and verify integrity cryptographically.

### Immutability

Resources never change after being written. This provides:

- **Stable references**: CIDs are reliable permanent identifiers
- **Historical completeness**: All versions of evolving data persist as separate resources
- **Conflict-free replication**: Immutable resources can be copied between stores without coordination

### Content Equivalence

If two stores contain resources with the same CID, the content is identical. This enables:

- **Store synchronization**: Comparing CID sets identifies missing resources
- **Distributed storage**: Resources can live in different stores while maintaining identity
- **Caching**: Resources can be cached anywhere without invalidation concerns

### Addressability

Every resource has a globally unique identifier (its CID) that remains valid across stores, networks, and time. Resources can be:

- Moved between storage backends without changing identity
- Cached locally while referencing remote stores
- Shared through CIDs without transferring content
- Archived and restored while maintaining references

## Graph Structure

The reference capability creates an immutable directed graph:

- **Nodes** are resources identified by CID
- **Edges** are references (CID values in resource content)
- **Properties**:
  - Acyclic or cyclic (nothing prevents cycles)
  - Immutable (edges never change, but new nodes can be added)
  - Traversable (following edges from any node explores connected resources)

The graph structure enables:

**Aggregation**: Starting from a root resource, follow references to collect all related resources (a component and its documentation, tests, dependencies).

**Reachability**: Determine what resources are transitively referenced from a starting point, enabling garbage collection of unreferenced resources.

**Versioning**: New versions of resources can reference previous versions, creating version chains in the graph.

**Composition**: Complex containers are projections over subgraphs—collecting specific resources based on reference patterns.

## Implementation Independence

The container store is defined by its capabilities and properties, not by a specific implementation.

Valid store implementations include:

**Filesystem**: Resources as files in content-addressed directories, references parsed from file contents.

**Relational Database**: Resources as BLOBs in tables, CIDs as primary keys, references extracted and indexed for query performance.

**Object Storage**: Resources as objects in cloud storage (S3, Azure Blob), CIDs as object keys.

**Distributed Hash Table**: Resources distributed across nodes in a DHT, CID-based retrieval and replication.

**Hybrid**: Filesystem for local working set, database for indexing and queries, object storage for archival.

The platform operates identically regardless of backend. This allows:

- Choosing storage appropriate to scale and access patterns
- Migrating between backends without changing platform semantics
- Using multiple backends simultaneously (local cache + remote archive)

## Store Metadata

The store may maintain metadata about resources beyond their content:

- **Storage timestamp**: When the resource was written to this store
- **Access patterns**: How frequently the resource is retrieved
- **Reference count**: How many other resources reference this CID
- **Size**: Byte count of resource content

This metadata is **not part of the resource** and does not affect the CID. It assists with store management (garbage collection, caching, performance optimization) but is not visible to projections.

Metadata is store-specific and not preserved when resources move between stores.

## Garbage Collection

Since resources are immutable and referenced by CID, unused resources can accumulate. **Garbage collection** identifies and removes resources that are no longer reachable from any root.

**Roots** are resources designated as starting points (active component definitions, running instances, materialized views). Resources reachable by following references from roots are retained. Unreachable resources can be deleted.

Garbage collection is a store management operation, not a platform operation. It operates on the store's reference graph without affecting the projection engine or platform semantics.

## The Store as Foundation

The container store provides the invariant foundation for the platform:

- All resources exist in the store
- All resources are content-addressed and immutable
- All resources are referenceable by CID
- All resources are queryable for projection

From this foundation, we build the projection-emission cycle: projections identify and aggregate resources from the store, execution produces results, and emissions write new resources back to the store.

The next document, **The Projection-Emission Cycle**, describes how the platform operates on the container store through continuous cycles of projection and emission.