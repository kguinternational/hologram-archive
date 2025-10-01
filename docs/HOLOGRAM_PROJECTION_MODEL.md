# The Hologram Projection Model

## Executive Summary

Hologram is a universal resource store where all information exists as content-addressed resources, and everything else—components, instances, interfaces, documentation, tests—emerges through projections. Rather than defining rigid types or structures, Hologram defines a projection language that allows resources to be viewed and interpreted in different ways. This document describes the complete conceptual model of the Hologram system.

## Core Principle: Everything is a Projection

The fundamental insight of Hologram is that there is only one source of truth: the store. Everything else that we perceive—component definitions, running instances, documentation, test suites—are projections from this singular store. Just as a hologram contains all its visual information in every fragment, the Hologram store contains all system information, viewable from different angles through projections.

## The Store: Universal Resource Container

The Hologram store is the singular source of truth. It contains all resources in the system, identified by content-addressed IDs (CIDs). The store makes no assumptions about what these resources represent—they are simply data with identity. The store provides only four fundamental capabilities:

First, resources can be stored and assigned a CID based on their content. Second, resources can be retrieved by their CID. Third, resources can reference other resources, forming a graph. Fourth, resources can be queried and projected into different views.

The store itself has no concept of "components" or "instances" or "documentation." These concepts emerge through projection.

## The Projection Language: Conformance as Lens

What Hologram calls "conformance" is actually its projection language. When we say a component "conforms" to an interface, we're actually saying that the component can be projected through an interface lens. Conformance requirements are not rules to follow—they are instructions for how to project resources.

The projection language defines several key aspects: how to identify resources that match projection criteria, which related resources should be included in the projection, how projections themselves can be projected (recursive projections), and how multiple projections compose to form complete views.

## Projection Types: The Base Lenses

Hologram defines several fundamental projection types that serve as the base lenses through which resources are viewed:

### Component Projection

The component projection identifies resources that serve as definitions. A resource becomes a "component" when it has a namespace and has conformance set to false. This projection aggregates related projections including interface, documentation, tests, and lifecycle management. The component projection doesn't create a component—it reveals resources that can be interpreted as components.

### Instance Projection

The instance projection reveals resources that represent runtime states. A resource becomes an "instance" when it references a parent component and contains state information. Instances are projections of components with added runtime context, configuration, and operational data.

### Interface Projection

The interface projection extracts the contract or API surface from resources. It reveals methods, inputs, outputs, and protocols. Every resource can potentially have an interface projection—even projections themselves have interfaces describing how they work.

### Documentation Projection

The documentation projection extracts human-readable information. It transforms technical specifications into narratives, examples, and explanations. This projection makes resources understandable to humans rather than machines.

### Test Projection

The test projection reveals validation and verification aspects. It shows how resources can be validated, what constraints they must satisfy, and what behaviors they must exhibit.

### Manager Projection

The manager projection reveals operational aspects—how resources are created, updated, deleted, started, stopped, and monitored. It projects the lifecycle and operational semantics onto resources.

## Recursive Projections: The Self-Describing System

A unique aspect of Hologram is that projections themselves are stored in the store and can be projected. This creates a self-describing system with several remarkable properties:

Projection definitions are themselves resources in the store, subject to the same storage and retrieval mechanisms as any other resource. Each projection type must define how it itself is projected—for example, the test projection must have its own test projection, and the interface projection must have its own interface projection. The system becomes self-documenting by projecting its own projection definitions into human-readable forms. New projection types can be added dynamically by storing new projection definitions in the store.

This recursive nature means that Hologram contains its own documentation, its own specification, and its own implementation guide—all as projections from the store.

## The Materialization Process

When a projection is applied to resources in the store, it produces a materialized view. This process involves:

### Identification
The projection language identifies which resources match the projection criteria. For example, resources with namespace and conformance=false can be projected as components.

### Aggregation
Related resources are gathered according to the projection definition. A component projection gathers its interface, documentation, test, and manager projections.

### Transformation
The gathered resources are transformed into the requested view format. This might be JSON for APIs, markdown for documentation, or filesystem layout for version control.

### Delivery
The materialized view is delivered to the requesting client in the appropriate format. The same resources can be materialized differently for different clients.

## Views: Materialized Projections for Different Purposes

Views are named, persistent materializations of projections. Hologram supports several standard views:

### The Spec View
This view materializes component definitions into filesystem structures suitable for version control. It projects resources into the spec directory with content-addressed filenames, enabling Git-based workflows while maintaining the store as the source of truth.

### The Instance View
This view materializes running instances and their states. It projects runtime information, configurations, and operational data for active system components.

### The Catalog View
This view materializes a browsable index of available resources. It projects metadata, relationships, and discovery information to enable resource exploration.

### Custom Views
New views can be defined by creating view projection definitions. Each view specifies its query criteria, transformation rules, and output format.

## Client Interactions: Working with Projections

Clients interact with Hologram by requesting projections and submitting resources. The process follows a consistent pattern:

### Resource Submission
Clients submit resources to the store, receiving CIDs in return. Resources are immutable once stored—modifications create new resources with new CIDs.

### Projection Definition
Clients define how resources relate by submitting projection definitions. These definitions establish the relationships that allow resources to be projected together.

### Projection Execution
Clients request projections by specifying the projection type and target resources. The projection engine materializes the requested view from the store.

### View Synchronization
For persistent views like the filesystem spec view, clients can request synchronization to update the materialized view with the latest store state.

## Implementation Independence

Critically, the Hologram model is independent of implementation choices. The store can be implemented using:

Filesystem storage with directories and files, database storage with tables and queries, distributed storage across multiple nodes, or cloud storage services. The projection model remains the same regardless of storage backend.

Similarly, the projection engine can be implemented in any language and can use various query technologies. What matters is that it correctly interprets the projection language, not how it accomplishes this interpretation.

## Bootstrap and Self-Hosting

Hologram exhibits an elegant bootstrap property. The minimal system requires only a store capable of storing and retrieving by CID, plus the ability to store and interpret the hologram.store definition itself. From this minimal base, the entire system emerges through successive layers of self-definition:

The hologram.store resource defines what resources are and how they are stored. The hologram.projection resource defines the projection language itself. The base projection types (component, instance, interface, etc.) define the fundamental lenses for interpreting resources. All other functionality builds upon these foundational projections.

This bootstrap sequence creates a system that is both minimal in its requirements and complete in its capabilities.

## Comparison with Traditional Systems

Traditional systems typically define fixed types, rigid schemas, and predetermined relationships. Objects have inherent types that determine their behavior. Schemas are external to data and must be separately maintained. Different views require different data copies or complex transformation logic.

Hologram inverts this model:

Resources have no inherent type—types emerge from projections. Schemas are themselves resources in the store, making the system self-describing. Views are projections, not copies, maintaining a single source of truth. New interpretations can be added without modifying existing resources.

## Practical Implications

This projection model has several practical implications for system design and operation:

### Infinite Extensibility
New projection types can be added at any time without modifying the core system. Existing resources can be projected in new ways as requirements evolve.

### Perfect Auditability
Every resource is immutable and content-addressed, creating an indelible audit trail. The entire history of the system can be reconstructed from the store.

### Simplified Distribution
Since everything is in the store, distribution becomes a matter of syncing stores. No special protocols are needed for different resource types.

### Natural Versioning
Resources are immutable, so versions are naturally preserved. Different versions are just different resources with different CIDs.

### Unified Interface
All operations become projections—create, read, update, delete are just different projection types applied to resources.

## The Complete Model

Hologram is fundamentally a projection engine operating on a universal store. The architecture consists of three essential elements:

The store contains undifferentiated resources identified by content-addressed IDs. These resources have no inherent meaning or type—they are simply data with identity.

The projection language, expressed through what Hologram calls conformance, defines how to view and interpret these resources. This language is itself stored as resources, making the system self-modifying.

The base projections—component, instance, interface, documentation, test, and manager—provide the fundamental lenses through which resources gain meaning. These projections are not hardcoded but are themselves defined as resources in the store.

This model achieves several seemingly contradictory goals simultaneously. It remains simple at its core (just a store and projections) while being powerful enough to represent any system. It provides flexibility (new projections can be added at any time) while maintaining rigor (content addressing ensures integrity). It is self-contained (includes its own complete definition) yet open (can integrate with any external system through appropriate projections).

The Hologram projection model represents a fundamental paradigm shift in system architecture. Rather than defining what things are, we define ways of seeing. Rather than fixing types and structures, we allow them to emerge through projection. Rather than separating data from its interpretation, we unite them in a single, self-describing store.

This is not merely a technical architecture but a new way of thinking about information systems—one where meaning emerges from perspective, where structure arises from interpretation, and where the system contains not just data but the very means of its own understanding.