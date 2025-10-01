# The Projection Language

## Conformance as Projection Instructions

The **projection language** is how projections are defined as resources in the container store. What appears as "conformance requirements" in component definitions is actually a **declarative language** for specifying how to project resources into containers.

This document describes the projection language—its syntax, semantics, and how conformance requirements serve as projection instructions.

## The Conformance Model

Traditional systems use conformance for validation: "does this data match this schema?"

Hologram inverts this: **conformance defines how to project**. A conformance requirement specifies:
- What resources to identify
- How to aggregate related resources
- What structure the resulting container must have
- What transformations to apply

Conformance requirements are **projection instructions** interpreted by the projection engine to produce containers.

## Projection Definition Structure

A projection definition is a resource containing:

**Identity**: The projection type and namespace (e.g., "hologram.component", "hologram.interface").

**Query Specification**: Criteria for identifying resources to project.

**Conformance Requirements**: Rules for aggregating and structuring resources into containers.

**Schema Definitions**: Expected structure for aggregated resources (JSON schemas, type definitions).

**Transformation Rules**: How to convert aggregated resources into final container format.

**Metadata**: Versioning, documentation, dependencies on other projection definitions.

The definition itself is stored in the container store, identified by CID. Different versions of a projection definition have different CIDs.

## Query Specification

The query specification identifies which resources to project.

### Namespace Queries

Resources are often organized by **namespace**—a hierarchical naming scheme embedded in resource content.

A namespace query identifies resources belonging to a specific namespace:
- "hologram.component" → all component definition resources
- "hologram.interface" → all interface resources
- "application.user" → application-specific user resources

The engine queries the store for resources with matching namespace fields.

### Content Pattern Queries

Queries can match resource content patterns:
- Resources containing specific fields
- Resources with field values matching predicates
- Resources matching text patterns
- Resources of specific content types (JSON, binary, text)

Content pattern queries enable flexible resource identification beyond namespace conventions.

### Reference Queries

Queries can identify resources based on reference relationships:
- Resources referenced by a known resource
- Resources that reference a known resource
- Resources transitively reachable from a starting point
- Resources forming specific graph patterns

Reference queries enable graph-based resource selection.

### Temporal Queries

Queries can include temporal constraints:
- Resources stored within a time range
- Resources as they existed at a specific point in time
- Resources modified after a certain timestamp

Temporal queries enable historical projections and time-travel.

### Composite Queries

Queries can combine multiple criteria:
- Namespace AND content pattern
- Reference relationship OR namespace
- Temporal constraint AND content pattern

Boolean composition enables precise resource identification.

## Conformance Requirements

Conformance requirements define how to aggregate and structure identified resources.

### Required Resources

Specify resources that **must** be present for valid projection:

A component conformance requirement might specify:
- "spec" resource (component specification)
- "interface" resource (interface definition)

Missing required resources cause projection failure.

### Optional Resources

Specify resources that **may** be present:

A component might have:
- Optional "documentation" resource
- Optional "examples" resource

Missing optional resources do not cause failure but affect container structure.

### Cardinality Constraints

Specify how many of each resource type:
- Exactly one: "spec" (1)
- One or more: "tests" (1..n)
- Zero or more: "dependencies" (0..n)
- Zero or one: "documentation" (0..1)

Cardinality violations cause projection failure.

### Reference Requirements

Specify how resources must reference each other:

An interface conformance requirement might specify:
- Must reference a "component" (parent relationship)
- Must be referenced by component's "interface" field (consistency)

Reference requirements ensure graph structure integrity.

### Schema Requirements

Specify structure for each resource type:

Each required or optional resource has an associated schema (often JSON Schema) defining:
- Required and optional fields
- Field types and formats
- Valid value ranges
- Structural constraints

Resources that don't match schemas cause validation failure.

### Semantic Requirements

Specify application-specific constraints:

- Naming conventions (namespace follows pattern)
- Version compatibility (dependencies at compatible versions)
- Uniqueness (no duplicate names within namespace)
- Business rules (specific field relationships)

Semantic requirements are evaluated during aggregation or transformation.

## Transformation Rules

Transformation rules specify how aggregated resources become the final container structure.

### Field Extraction

Extract specific fields from resources:
- From "spec" resource, extract "namespace", "version", "description"
- From "interface" resource, extract "methods"

Extracted fields populate container properties.

### Resource Combination

Combine multiple resources into unified structure:
- Merge all "test" resources into "tests" array
- Combine "documentation" resources by section

Combination creates hierarchical container structure.

### Computed Fields

Derive values from resource content:
- Count of dependencies
- Hash of concatenated test resources
- Latest timestamp across all resources

Computed fields add metadata to containers.

### Format Conversion

Convert resource encodings:
- Binary to base64 text
- JSON to human-readable formatted text
- Markdown to HTML

Format conversion adapts resources to container requirements.

### Ordering and Filtering

Order resources by property (alphabetically, by timestamp, by dependency order).

Filter resources by criteria (exclude deprecated, include only active).

Ordering and filtering refine container contents.

## Parameterized Projections

Projection definitions can include parameters that customize behavior at execution time.

### Parameter Declaration

Define parameters the projection accepts:
- "namespace" (string): Which namespace to project
- "include_optional" (boolean): Whether to include optional resources
- "max_depth" (integer): Maximum reference traversal depth

Parameters are declared in the projection definition with types and constraints.

### Parameter Binding

At execution time, the engine binds parameter values:
- From client request (explicit parameter values)
- From context (environment variables, user identity)
- From defaults (declared in projection definition)

Bound parameters are substituted into query specification and transformation rules.

### Parameterized Queries

Parameters customize queries:

Instead of hard-coding "hologram.component", parameterize as "{namespace}".

At execution, "{namespace}" is replaced with the bound value.

This enables reusing projection definitions across different namespaces.

### Conditional Rules

Rules can be conditional on parameters:

IF "include_optional" THEN aggregate optional documentation resources.

Conditional rules enable flexible projection behavior.

## Projection Composition in the Language

The projection language supports defining composed projections.

### Sub-Projection References

A projection definition can reference other projection definitions:

A component projection might specify:
- Project "hologram.interface" for interface resources
- Project "hologram.documentation" for documentation resources

The engine executes sub-projections and incorporates results into the parent projection.

### Nested Conformance

Conformance requirements can nest:

A component requires:
- "interface" conforming to "hologram.interface" projection
- "documentation" conforming to "hologram.documentation" projection

Nested conformance creates projection hierarchies.

### Projection Inheritance

Projection definitions can extend other projection definitions:

"hologram.advanced_component" extends "hologram.component":
- Inherits all base requirements
- Adds additional requirements
- Overrides specific transformation rules

Inheritance enables specialization without duplication.

## Language Semantics

### Declarative Nature

The projection language is **declarative**—it describes **what** to project, not **how** to project.

The engine determines execution strategy (query optimization, traversal algorithm, caching).

This enables engine evolution without changing projection definitions.

### Purity

Projection definitions are **pure specifications**—they don't include imperative code with side effects.

No "execute this function", only "aggregate these resources with these rules".

Purity enables analysis, optimization, and reasoning about projections.

### Composability

Projections compose cleanly:
- Reference other projections without tight coupling
- Inherit and extend without duplicating logic
- Parameterize without creating explosion of variants

Composability enables building complex projections from simple components.

### Versioning

Projection definitions are versioned through CID:
- New version → new resource → new CID
- Old versions remain available (immutability)
- Resources can reference specific projection versions

Versioning enables evolution without breaking existing projections.

## Schema Language

The projection language includes or references a schema language for defining resource structure.

### JSON Schema Integration

The platform uses JSON Schema as the primary schema language:
- Mature, widely adopted standard
- Rich constraint language
- Tool support for validation

JSON schemas are resources in the store, referenced by projection definitions.

### Schema Composition

Schemas can reference other schemas:
- Common type definitions shared across schemas
- Schema inheritance through "allOf", "oneOf"
- Modular schema construction

Schema composition mirrors projection composition.

### Schema Versioning

Like projection definitions, schemas are versioned via CID.

Projection definitions reference specific schema versions, ensuring stable validation over time.

## Language Extensions

The projection language is extensible—new query types, conformance requirements, and transformation rules can be added.

### Custom Query Types

Define new query types as resources:
- Graph pattern matching queries
- Probabilistic similarity queries
- Machine learning-based classification queries

The engine loads custom query implementations and applies them.

### Custom Validators

Define new validation rules as resources:
- Application-specific business rules
- Cross-resource consistency checks
- External system integration (check against external API)

Custom validators plug into the conformance validation phase.

### Custom Transformations

Define new transformations as resources:
- Domain-specific format conversions
- Complex computations (rendering, compilation)
- Aggregations specific to application needs

Custom transformations extend the transformation pipeline.

Extensions are themselves resources, making the language **self-extensible**.

## Conformance as Lens Definition

Viewing conformance as projection instructions reveals its true role:

**Traditional View**: "This resource conforms to this schema" (validation).

**Hologram View**: "Project resources using these conformance requirements to create this container type" (projection).

Conformance requirements are **lens definitions**—they define how to view the store through a particular lens to see a particular type of container.

Different conformance requirements (lenses) applied to the same resources produce different containers (views).

## Language Bootstrap

The projection language is defined using itself:

- "hologram.projection" is a projection definition for projecting projection definitions
- It specifies how to identify, aggregate, and structure projection definition resources
- The engine uses "hologram.projection" to understand projection definitions

This self-description enables the language to evolve—new language features are expressed as updates to the "hologram.projection" definition.

## Next Steps

Projections produce containers through the **Project → Execute** phases. The **Emit** phase produces new resources from execution results.

The next document, **The Emission Model**, describes how containers emit resources during execution, what types of emissions occur, and how emissions integrate back into the store for future projections.