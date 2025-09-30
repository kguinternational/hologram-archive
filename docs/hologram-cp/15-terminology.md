# Terminology

## Glossary and Cross-References

This document defines key terms used throughout the Hologram documentation and maps them to established computer science concepts.

## Core Concepts

### Container Store

The universal storage system where all resources exist as immutable, content-addressed data.

**Related CS Concepts**: Content-addressable storage (CAS), content-addressable memory, immutable data stores, append-only logs.

**See**: Document 02 - The Container Store

### Resource

Any sequence of bytes stored in the container store. Resources have no inherent type or structure—meaning emerges through projection.

**Related CS Concepts**: Blob (Binary Large Object), untyped data, raw data, resource (REST).

**See**: Document 02 - The Container Store

### Content Identifier (CID)

A cryptographic hash of a resource's content, used as the resource's identifier. Identical content produces identical CIDs.

**Related CS Concepts**: Content hash, cryptographic hash, checksum, digest, content addressing, self-certifying identifier.

**See**: Document 02 - The Container Store

### Immutability

Resources never change after emission. Once stored, a resource's content is permanent. Updates create new resources with new CIDs.

**Related CS Concepts**: Immutable data structures, persistent data structures, write-once storage, append-only.

**See**: Document 02 - The Container Store, Document 13 - Platform Properties

## Projection Concepts

### Projection

A query-based definition that identifies, aggregates, and transforms resources into a container. Projections are pure queries without side effects.

**Related CS Concepts**: View (database), query, transformation, lens (functional programming), map/reduce, query language.

**See**: Document 04 - Container Projections

### Container

A structured collection of resources produced by projecting resources from the store. Containers have defined purpose and relationships.

**Related CS Concepts**: View (database), aggregate (DDD), composite object, container (general programming).

**See**: Document 04 - Container Projections

### Projection Definition

A resource that specifies how to project resources into containers, including query criteria, aggregation rules, and transformations.

**Related CS Concepts**: View definition, query definition, schema, specification, declarative program.

**See**: Document 06 - The Projection Language

### Conformance Requirements

Instructions within projection definitions specifying what resources must be present and how they should be structured. Conformance defines projection, not just validation.

**Related CS Concepts**: Constraints, schema, type specification, contract, invariant.

**See**: Document 06 - The Projection Language

### Projection Engine

The component that interprets projection definitions and executes them against the container store to produce containers.

**Related CS Concepts**: Query engine, interpreter, virtual machine, execution engine, runtime.

**See**: Document 05 - The Projection Engine

## Emission Concepts

### Emission

The process of writing execution results back to the container store as new resources. Emissions make ephemeral results durable.

**Related CS Concepts**: Write operation, persistence, materialization, output, side effect (though emissions are structured).

**See**: Document 07 - The Emission Model

### Projection-Emission Cycle

The fundamental operating principle: Project resources → Execute operations → Emit results → Store resources → Repeat.

**Related CS Concepts**: Read-compute-write cycle, ETL (Extract-Transform-Load), data pipeline, event loop.

**See**: Document 03 - The Projection-Emission Cycle

### Emission Stream

A sequence of emissions produced continuously over time, such as logs or state snapshots from a running instance.

**Related CS Concepts**: Event stream, log stream, observable, reactive stream, data stream.

**See**: Document 07 - The Emission Model

## Container Types

### Component Container

A container defining a reusable capability through specifications, interfaces, documentation, tests, and dependencies.

**Related CS Concepts**: Module, package, library, service definition, component (software engineering).

**See**: Document 08 - Container Types

### Instance Container

A container representing a running execution of a component definition with runtime state.

**Related CS Concepts**: Process, object instance, runtime instance, container instance (Docker), actor (actor model).

**See**: Document 08 - Container Types

### Interface Container

A container specifying contracts between components through method signatures and type definitions.

**Related CS Concepts**: Interface (OOP), API specification, contract, protocol, abstract base class.

**See**: Document 08 - Container Types

### View Container

A materialized projection stored as a resource for efficient repeated access.

**Related CS Concepts**: Materialized view (database), cached result, denormalized data, index, summary table.

**See**: Document 08 - Container Types, Document 09 - Platform Views

### Documentation Container

A container providing human-readable explanations, guides, and examples.

**Related CS Concepts**: Documentation, readme, manual, help text, javadoc/docstring.

**See**: Document 08 - Container Types

### Test Container

A container defining validation logic for verifying component behavior.

**Related CS Concepts**: Test suite, test case, test spec, unit test, integration test.

**See**: Document 08 - Container Types

## Platform Architecture

### Bootstrap

The process of initializing the platform from minimal primitives to a fully self-describing system.

**Related CS Concepts**: Bootstrapping, cold start, system initialization, self-hosting compiler.

**See**: Document 12 - Bootstrap Architecture

### Operation

A named procedure that projects resources, executes logic, and emits results. Operations are the platform's verbs.

**Related CS Concepts**: Command (CQRS), action, method, procedure, RPC call, API endpoint.

**See**: Document 10 - Platform Operations

### Platform API

The client interface for interacting with the platform—submitting operations, retrieving resources, subscribing to emissions.

**Related CS Concepts**: API, SDK, client library, REST API, RPC interface.

**See**: Document 11 - Platform API

## Advanced Concepts

### Self-Description

The property that the platform contains its own definition as resources in the store. The platform can introspect and evolve itself.

**Related CS Concepts**: Reflection, introspection, self-hosting, metacircular evaluation, reflective architecture.

**See**: Document 13 - Platform Properties

### Meta-Projection

A projection that projects other projection definitions. Projections analyzing or transforming projections.

**Related CS Concepts**: Meta-programming, reflection, higher-order function, metaobject protocol.

**See**: Document 05 - The Projection Engine

### Temporal Projection

A projection targeting resources as they existed at a specific point in time. Time-travel queries.

**Related CS Concepts**: Temporal database, time-travel query, historical query, version control, snapshot.

**See**: Document 04 - Container Projections

### Reference Graph

The directed graph formed by CID references between resources. Nodes are resources, edges are references.

**Related CS Concepts**: Directed graph, object graph, reference graph, dependency graph, DAG (if acyclic).

**See**: Document 02 - The Container Store

### Garbage Collection

The process of identifying and removing unreferenced resources to reclaim storage space.

**Related CS Concepts**: Garbage collection, reference counting, mark-and-sweep, reachability analysis.

**See**: Document 02 - The Container Store

## Established CS Mappings

### Hologram → Database Systems

- **Container Store** ↔ Database
- **Resource** ↔ Row/Document
- **CID** ↔ Primary Key (content-derived)
- **Projection** ↔ Query/View
- **Emission** ↔ INSERT/UPDATE
- **View** ↔ Materialized View
- **Reference Graph** ↔ Foreign Keys/Relationships

### Hologram → Functional Programming

- **Projection** ↔ Pure Function
- **Container** ↔ Product Type/Record
- **Projection Definition** ↔ Function Definition
- **Projection Composition** ↔ Function Composition
- **Immutability** ↔ Immutable Data Structures
- **Conformance** ↔ Type Constraint

### Hologram → Content-Addressed Storage

- **Container Store** ↔ CAS (Git, IPFS, Merkle DAG)
- **CID** ↔ SHA hash, Content Address
- **Resource** ↔ Object, Blob
- **Reference** ↔ Pointer, Link
- **Immutability** ↔ Content Addressing Property

### Hologram → Version Control

- **Emission** ↔ Commit
- **CID** ↔ Commit Hash
- **Reference Graph** ↔ Commit Graph
- **View** ↔ Working Directory
- **Temporal Projection** ↔ Checkout Historical Commit

### Hologram → Container Orchestration

- **Component** ↔ Container Image
- **Instance** ↔ Running Container
- **Manager** ↔ Orchestrator (Kubernetes)
- **Operation** ↔ Controller Action
- **Platform** ↔ Container Runtime

### Hologram → Object-Oriented Programming

- **Component** ↔ Class
- **Instance** ↔ Object Instance
- **Interface** ↔ Interface/Abstract Class
- **Operation** ↔ Method
- **Conformance** ↔ Type Constraint/Contract

## Document Cross-References

### Foundation Concepts
- Container Store: Document 02
- Projection-Emission Cycle: Document 03
- Container Projections: Document 04

### Engine and Language
- Projection Engine: Document 05
- Projection Language: Document 06
- Emission Model: Document 07

### Platform Components
- Container Types: Document 08
- Platform Views: Document 09
- Platform Operations: Document 10
- Platform API: Document 11

### System Architecture
- Bootstrap: Document 12
- Properties: Document 13
- Implementation: Document 14

## Acronyms and Abbreviations

**API**: Application Programming Interface

**CAS**: Content-Addressed Storage

**CID**: Content Identifier

**CRUD**: Create, Read, Update, Delete

**DAG**: Directed Acyclic Graph

**JSON**: JavaScript Object Notation

**MCP**: Model Context Protocol

**REST**: Representational State Transfer

**SHA**: Secure Hash Algorithm

**TTL**: Time To Live

## Conventions

### Naming Patterns

**hologram.{type}**: Platform-provided base container types (hologram.component, hologram.instance).

**{namespace}.{name}**: Application-defined resources following namespace pattern.

**{operation}.{action}**: Operation naming (component.create, instance.start).

### CID Format

CIDs are typically represented as:
- Prefix: "cid:" (optional, implementation-dependent)
- Hash: Hexadecimal digest of content
- Example: "cid:abc123def456..." or "abc123def456..."

### Resource References

Resources reference other resources by embedding CIDs in their content, typically as string values in JSON fields.

## Conclusion

This terminology document provides definitions for key concepts and maps them to established computer science concepts. Understanding these mappings helps situate Hologram within the broader context of computing systems while appreciating its unique architectural approach.

---

## The Complete Documentation

This completes the 15-document series on the Hologram Container Engine and Platform:

**Part I: Foundation** - Documents 1-4 establish the core model from first principles.

**Part II: The Engine** - Documents 5-7 detail the projection engine and emission mechanisms.

**Part III: The Platform** - Documents 8-11 describe container types, views, operations, and API.

**Part IV: System Architecture** - Documents 12-15 cover bootstrap, properties, implementation, and terminology.

Together, these documents define the conceptual model that hologram.spec implementations will realize.