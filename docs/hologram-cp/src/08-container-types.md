# Container Types

## Platform Container Types and Emissions

The Hologram platform provides **base container types**—projection definitions and associated operations for common containment patterns. Container types define how resources are projected into containers and what emissions those containers produce.

This document describes the platform's base container types, their projection patterns, and emission characteristics.

## Container Type Fundamentals

A **container type** is defined by:

**Projection Definition**: The conformance requirements and transformation rules for projecting resources into this container type.

**Required Resources**: What resources must be present for valid containers of this type.

**Optional Resources**: What resources may be included but aren't mandatory.

**Emission Pattern**: What resources containers of this type emit during their lifecycle.

**Operations**: What operations can be performed with containers of this type.

Container types are defined as resources in the store (projection definitions), making them extensible—new container types are added by emitting new projection definitions.

## Component Containers

**Purpose**: Define reusable capabilities as specifications.

### Projection Pattern

**Query**: Identify resources with component namespace (e.g., "hologram.component", "application.service").

**Required Resources**:
- **Spec**: Component specification (namespace, version, description, metadata)

**Optional Resources**:
- **Interface**: Interface definition resources
- **Documentation**: Human-readable documentation resources
- **Tests**: Test suite resources
- **Dependencies**: References to other components required
- **Build**: Build process definition
- **Manager**: Lifecycle management definition

**Aggregation**: Follow references from spec to collect all related resources.

**Transformation**: Structure into hierarchical component definition with spec as root, related resources as branches.

### Emission Pattern

**Creation**: Emit component definition resource containing spec and references to all conformance resources.

**Update**: Emit new component definition resource with updated content (new CID, immutable versioning).

**Deletion Marker**: Emit resource marking component as deprecated or deleted (original definition remains for history).

### Characteristics

- **Immutable**: Once created, component definitions don't change (updates create new versions)
- **Versioned**: Each update produces new CID, creating version chain
- **Reusable**: Can be referenced by multiple instances or other components
- **Self-Describing**: Contains all information needed to understand and use the component

## Instance Containers

**Purpose**: Execute component definitions with runtime state.

### Projection Pattern

**Query**: Identify resources with instance identifier (references specific component definition and instance ID).

**Required Resources**:
- **Component Reference**: CID of component definition to instantiate
- **Instance Identity**: Unique identifier for this instance

**Optional Resources**:
- **Initial State**: Starting state for the instance
- **Configuration**: Instance-specific configuration overriding defaults
- **Context**: Runtime environment information

**Aggregation**: Project component definition, then collect instance-specific resources.

**Transformation**: Combine component definition with instance state/config into executable instance container.

### Emission Pattern

**Initialization**: Emit initial instance state resource.

**Runtime**: Continuous emission stream while running:
- **State Snapshots**: Periodic or event-triggered state captures
- **Log Entries**: Events, errors, info messages
- **Application Data**: Output produced by instance execution
- **Metrics**: Performance and resource utilization data

**Termination**: Emit final state and shutdown logs when instance stops.

### Characteristics

- **Stateful**: Maintains evolving state over time through emissions
- **Ephemeral**: Can be started, stopped, removed (though emissions persist)
- **Observable**: Continuous emission stream provides visibility
- **Recoverable**: State snapshots enable restart from last known state

## Interface Containers

**Purpose**: Define contracts between components.

### Projection Pattern

**Query**: Identify interface definition resources (often by namespace or referenced by component).

**Required Resources**:
- **Interface Specification**: Methods, parameters, return types, error conditions

**Optional Resources**:
- **Schema Definitions**: Type definitions for parameters/returns
- **Documentation**: Description of interface semantics
- **Examples**: Usage examples

**Aggregation**: Collect interface spec and related type/schema resources.

**Transformation**: Structure into interface definition with methods and types clearly defined.

### Emission Pattern

**Definition**: Emit interface definition resource.

**Versioning**: Emit new interface versions (semantic versioning for compatibility tracking).

**Conformance Validation**: Implementations emit validation resources demonstrating conformance to interface.

### Characteristics

- **Contract-Defining**: Specifies expectations between components
- **Versioned**: Interface evolution tracked through versions
- **Reusable**: Multiple components can implement same interface
- **Validation-Enabling**: Enables checking component compatibility

## Documentation Containers

**Purpose**: Provide human-readable explanations and guides.

### Projection Pattern

**Query**: Identify documentation resources (by namespace, by reference from components).

**Required Resources**:
- **Documentation Content**: Markdown, HTML, or other human-readable format

**Optional Resources**:
- **Examples**: Code examples, usage demonstrations
- **Diagrams**: Visual representations
- **Cross-References**: Links to related documentation

**Aggregation**: Collect documentation resources and related examples/diagrams.

**Transformation**: Structure into navigable documentation with sections, cross-references resolved.

### Emission Pattern

**Creation**: Emit documentation resource.

**Updates**: Emit updated documentation (versioned with component versions).

**Rendered Views**: Emit transformed formats (HTML from Markdown, PDF from LaTeX).

### Characteristics

- **Human-Oriented**: Designed for human consumption, not machine execution
- **Cross-Linked**: References other documentation, components, interfaces
- **Multi-Format**: Can be projected into different representations
- **Versioned**: Evolves with component versions

## Test Containers

**Purpose**: Validate component behavior and correctness.

### Projection Pattern

**Query**: Identify test resources (by namespace, by component reference).

**Required Resources**:
- **Test Cases**: Individual test definitions (inputs, expected outputs, assertions)

**Optional Resources**:
- **Test Fixtures**: Shared setup/teardown logic
- **Test Data**: Input datasets for tests
- **Validation Rules**: Expected behavior specifications

**Aggregation**: Collect test cases, fixtures, and data into test suite.

**Transformation**: Structure into executable test suite.

### Emission Pattern

**Suite Execution**: Emit test results:
- **Pass/Fail Status**: Per-test and overall results
- **Execution Logs**: Details of test execution
- **Coverage Data**: What code/behavior was tested
- **Performance Metrics**: Test execution time

**Continuous Testing**: Emit updated test results as tests re-run.

### Characteristics

- **Executable**: Test containers run to produce results
- **Validation-Focused**: Purpose is to verify correctness
- **Evolving**: New tests added as components evolve
- **Result-Producing**: Emissions are test outcomes

## Manager Containers

**Purpose**: Control lifecycle of instances and components.

### Projection Pattern

**Query**: Identify manager definition resources (lifecycle management logic).

**Required Resources**:
- **Lifecycle Operations**: Definitions for start, stop, restart, update operations

**Optional Resources**:
- **Health Checks**: Logic for determining instance health
- **Scaling Rules**: Criteria for starting/stopping multiple instances
- **Recovery Policies**: How to handle failures

**Aggregation**: Collect manager operations and policies.

**Transformation**: Structure into manager definition with operation handlers.

### Emission Pattern

**Operation Execution**: Emit operation results:
- **State Transitions**: Instance moved from stopped to running
- **Operation Logs**: What actions were taken
- **Status Updates**: Current manager state

**Health Monitoring**: Emit health check results periodically.

### Characteristics

- **Control-Oriented**: Manages other containers' lifecycles
- **Policy-Driven**: Behavior defined by policies in projection
- **Reactive**: Responds to state changes and external requests
- **Status-Emitting**: Continuously updates status

## View Containers

**Purpose**: Materialized projections for efficient access.

### Projection Pattern

**Query**: Identify resources matching view criteria (flexible, view-specific).

**Required Resources**: Varies by view type (any resources the view aggregates).

**Optional Resources**: Varies by view type.

**Aggregation**: Follows view-specific aggregation rules (might be complex multi-step).

**Transformation**: Transform to view format (often denormalized, indexed).

### Emission Pattern

**Materialization**: Emit view resource (pre-computed projection result).

**Refresh**: Emit updated view resource as underlying resources change.

**Index Emissions**: Emit index structures for efficient view queries.

### Characteristics

- **Denormalized**: Often aggregate and flatten for query performance
- **Refreshable**: Can be updated incrementally or fully rebuilt
- **Query-Optimized**: Structured for fast access patterns
- **Derived**: Always derivable from underlying resources (can be rebuilt)

## Dependency Containers

**Purpose**: Express relationships between components.

### Projection Pattern

**Query**: Identify dependency resources (references between components).

**Required Resources**:
- **Dependent Component**: Component requiring dependencies
- **Required Component**: Component being depended upon
- **Dependency Type**: Nature of dependency (interface, data, service)

**Optional Resources**:
- **Version Constraints**: Compatible version ranges
- **Optional Flags**: Whether dependency is required or optional

**Aggregation**: Build dependency graph by following dependency references.

**Transformation**: Structure into graph or list of dependencies.

### Emission Pattern

**Dependency Declaration**: Emit dependency resource linking components.

**Resolution Results**: Emit resolved dependency sets (satisfying version constraints).

**Conflict Warnings**: Emit warnings if dependency conflicts detected.

### Characteristics

- **Graph-Forming**: Dependencies create directed graph over components
- **Constraint-Carrying**: Express version and compatibility requirements
- **Resolvable**: Dependency resolution algorithms operate on these containers
- **Validation-Enabling**: Enable checking for circular dependencies, conflicts

## Build Containers

**Purpose**: Define transformation from source to executable artifacts.

### Projection Pattern

**Query**: Identify build definition resources.

**Required Resources**:
- **Build Steps**: Sequence of transformation operations
- **Source References**: Input resources to build process

**Optional Resources**:
- **Build Environment**: Required tools, dependencies
- **Build Configuration**: Parameters, flags, options

**Aggregation**: Collect build definition and referenced sources.

**Transformation**: Structure into executable build plan.

### Emission Pattern

**Build Execution**: Emit build results:
- **Built Artifacts**: Output resources from build
- **Build Logs**: Details of build process
- **Build Status**: Success/failure
- **Build Metadata**: Build timestamp, environment, inputs

### Characteristics

- **Transformation-Focused**: Converts inputs to outputs
- **Repeatable**: Same inputs and build definition produce same outputs
- **Artifact-Producing**: Primary emission is built artifacts
- **Traceable**: Build logs provide full traceability

## Log Containers

**Purpose**: Structured logging and event records.

### Projection Pattern

**Query**: Identify log entry resources (by source, by time range, by level).

**Required Resources**:
- **Log Entries**: Individual log records

**Optional Resources**:
- **Context Information**: Associated resource references, user identity

**Aggregation**: Collect logs by query criteria (temporal, source-based).

**Transformation**: Structure into queryable log collection (often chronological).

### Emission Pattern

**Log Creation**: Emit individual log entry resources continuously.

**Log Aggregation**: Emit aggregated log views (hourly, daily summaries).

**Alert Emissions**: Emit alert resources when logs match alert criteria.

### Characteristics

- **Time-Ordered**: Logs have timestamps, often queried chronologically
- **High-Volume**: Can produce many emissions rapidly
- **Queryable**: Often projected into views for searching
- **Retention-Sensitive**: May be garbage collected after retention period

## Extending Container Types

New container types are created by emitting new projection definitions.

**Process**:
1. Define conformance requirements (what resources, what structure)
2. Define transformation rules (how to structure container)
3. Define emission patterns (what the container emits)
4. Emit projection definition resource to store
5. Platform recognizes new container type, enables projecting it

This extensibility means the platform is **open**—applications can define domain-specific container types without modifying platform code.

## Container Type Composition

Container types compose naturally:
- Components reference interfaces and documentation
- Instances reference components
- Views aggregate any container type
- Managers control instances

Composition creates rich ecosystems where container types work together.

## Next Steps

View containers deserve detailed treatment due to their role as optimized, persistent projections. The next document, **Platform Views**, explores views in depth—how they're materialized, refreshed, and used for efficient access to projected data.