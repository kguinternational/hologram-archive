# Introduction

## The Hologram Container Engine and Platform

Hologram is a universal container engine and platform built on a fundamental insight: **all information can be contained through projection rather than classification**.

Traditional systems organize information through type hierarchies, schemas, and rigid classifications imposed at write time. Hologram inverts this model. Resources exist without inherent type or structure in a universal store, identified only by their content. Meaning, structure, and purpose emerge through **projections**—queries that identify, aggregate, and transform resources into containers.

This document introduces the core concepts, architecture, and operating principles of the Hologram Container Engine and Platform.

## What Hologram Is

Hologram is:

- **A universal resource store** using content-addressed storage where resources exist as immutable, undifferentiated data identified by cryptographic hash of their content

- **A projection engine** that executes projection definitions to identify, aggregate, and transform resources into containers with meaning and purpose

- **A container platform** providing base container types (components, instances, interfaces, documentation, tests, managers, views) that are themselves defined through projections

- **A self-describing system** where the platform's own definition exists as projections in the store, enabling evolution without external dependencies

## Core Innovation: Containment Through Projection

The central innovation is the **projection-emission cycle**:

1. **Projection** identifies and aggregates resources from the store based on queries
2. **Execution** operates on projected resources, producing results
3. **Emission** writes new resources back to the store as immutable, content-addressed data
4. **Storage** persists emitted resources, making them available for future projections

This cycle applies uniformly across the platform:

- Creating a component definition projects artifacts and emits component resources
- Running an instance projects a component definition and emits logs, state, and application data
- Materializing a view projects resources and emits a persistent representation
- Every operation is a projection that emits

The cycle creates a **self-evolving system** where emitted resources become inputs to future projections, enabling continuous refinement and extension without modifying the engine or platform core.

## Key Principles

### 1. Resources Have No Inherent Type

Resources in the store are undifferentiated data. A JSON document, a binary blob, a text file—all exist as content with a cryptographic identifier. Type, structure, and meaning emerge through projection, not classification.

### 2. Content Addressing Provides Identity

Resources are identified by the cryptographic hash of their content (Content Identifier, or CID). Identical content always produces the same CID. This provides:

- **Deduplication** - identical content stored once
- **Integrity** - content cannot change without changing CID
- **Immutability** - resources are write-once, enabling reliable referencing
- **Distribution** - content can move between stores while maintaining identity

### 3. Projections Create Containers

A **projection** is a query-based definition that identifies resources matching certain criteria, aggregates them according to relationship rules, and transforms them into a container. The same resources can participate in multiple projections, appearing in different containers with different purposes.

### 4. Emissions Produce New Resources

Executing a projection produces results—these results are **emitted** as new resources to the store. Emissions are themselves projectable, creating feedback loops where operations build on previous results.

### 5. The Platform Is Self-Describing

The projection engine, container types, and platform operations are all defined as projections in the store. The system contains its own definition, enabling introspection, evolution, and extension through the same mechanisms used for application containers.

## Architecture Overview

The platform consists of three fundamental layers:

### The Container Store

A content-addressed storage system providing four core capabilities:

- **Store**: Write immutable resources identified by content hash
- **Retrieve**: Read resources by CID
- **Reference**: Link resources through CID references
- **Query**: Identify resources matching projection criteria

The store is implementation-independent—filesystem, database, distributed storage, or cloud object storage can provide the backend.

### The Projection Engine

Executes projection definitions to create containers:

- Evaluates queries to identify resources in the store
- Follows references to aggregate related resources
- Applies transformations to produce container contents
- Delivers projected containers to requesters

The engine interprets projection definitions (themselves resources in the store) and orchestrates the projection-emission cycle.

### The Container Platform

Provides base container types and operations:

- **Component containers** define reusable capabilities
- **Instance containers** execute component definitions
- **Interface containers** specify contracts between components
- **Documentation containers** provide human-readable views
- **Test containers** validate behavior
- **Manager containers** control lifecycle operations
- **View containers** materialize persistent projections

All platform container types are defined through projections, making them extensible and evolvable.

## The Projection-Emission Cycle

The fundamental operating principle is the continuous cycle:

**Project → Execute → Emit → Store → Project...**

Every interaction with Hologram follows this cycle:

- A client submits a projection request
- The engine identifies and aggregates resources
- Execution produces results
- Results are emitted as new resources to the store
- Emitted resources become available for projection

This creates a **generative system** where operations compound—each emission adds to the store, enabling more sophisticated projections that produce richer emissions.

## What Hologram Enables

The projection model provides unique capabilities:

**Flexible Organization**: Resources can participate in multiple containers simultaneously. A documentation resource might appear in a component container, a searchable documentation view, and a tutorial container—without duplication or synchronization.

**Immutable History**: All resources are immutable. Changes create new resources with new CIDs. The complete history exists in the store, enabling time-travel queries and audit trails.

**Composable Projections**: Projections can project other projections. A view might project components, which themselves project interfaces, documentation, and tests. Composition creates powerful abstractions without runtime overhead.

**Self-Evolution**: The platform definition exists as projections. Improving Hologram means emitting new projection definitions to the store. The system evolves through the same mechanisms used for applications.

**Implementation Independence**: The projection model is abstract—the engine, store backend, and platform operations can be implemented in any language, on any infrastructure. Portability is inherent.

## Reading Guide

This documentation is organized into four parts, each building on the previous:

**Part I: Foundation** (Documents 1-4)
Establishes first principles: the container store, projection-emission cycle, and how resources become containers through projection.

**Part II: The Engine** (Documents 5-7)
Details how the projection engine executes projections, the projection language for defining projections, and how emissions work.

**Part III: The Platform** (Documents 8-11)
Describes platform container types, views, operations, and the client API for interacting with the platform.

**Part IV: System Architecture** (Documents 12-15)
Covers bootstrap architecture, system properties, implementation considerations, and terminology.

Each document builds on concepts from previous documents. Reading sequentially provides the clearest understanding, though individual documents can serve as reference material.

## Next Steps

The next document, **The Container Store**, establishes the foundation by describing how content-addressed storage provides the universal resource container underlying all projections.

From that foundation, we build up the projection-emission cycle, the projection engine, and the container platform—all based on the simple primitive of immutable, content-addressed resources.