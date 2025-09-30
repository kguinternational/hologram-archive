# Platform Operations

## Operations as Projections That Emit

**Operations** are the platform's mechanisms for interacting with resources through projection-emission cycles. Every operation follows the pattern: project resources, execute logic, emit results.

This document describes platform operations, their structure, and how they embody the projection-emission cycle.

## Operation Fundamentals

An **operation** is a named, executable procedure that:

**Projects**: Identifies and aggregates resources from the store based on operation inputs.

**Executes**: Performs computation, validation, transformation, or coordination using projected resources.

**Emits**: Writes results back to the store as new resources.

**Returns**: Provides operation outcome to the caller (success/failure, emitted resource CIDs).

Operations are the **verbs** of the platform—the actions that transform store state.

## Operation Structure

Every operation has:

**Identity**: Operation name and type (e.g., "component.create", "instance.start").

**Inputs**: Parameters provided by caller (resource CIDs, values, options).

**Projection Phase**: How the operation identifies and aggregates resources from the store.

**Execution Phase**: What computation the operation performs on projected resources.

**Emission Phase**: What resources the operation writes back to the store.

**Outputs**: What the operation returns to caller (status, emitted CIDs, error information).

This structure maps directly to the projection-emission cycle.

## Component Operations

Operations for managing component definitions.

### Component Create

**Purpose**: Define a new component from artifacts.

**Inputs**:
- Artifact resources (spec, interface, docs, tests, etc.)
- Component namespace
- Version information

**Projection Phase**:
- Project artifact resources by CID
- Project component model (hologram.component definition) to understand conformance requirements

**Execution Phase**:
- Validate artifacts against schemas
- Check conformance requirements satisfied
- Build component definition structure
- Assign unique component identifier

**Emission Phase**:
- Emit component definition resource
- Emit component index entry (for catalog)

**Outputs**:
- Component definition CID
- Success/failure status
- Validation messages

### Component Read

**Purpose**: Retrieve component definition and related resources.

**Inputs**:
- Component namespace or CID

**Projection Phase**:
- Project component definition resource
- Project referenced resources (interface, docs, tests per request)

**Execution Phase**:
- Assemble complete component container
- Resolve references
- Apply formatting/transformation if requested

**Emission Phase**:
- None (read-only operation)
- May cache projection result for performance

**Outputs**:
- Component definition
- Referenced resources
- Metadata

### Component Update

**Purpose**: Modify existing component definition.

**Inputs**:
- Component namespace or CID
- Updated artifact resources
- Change description

**Projection Phase**:
- Project existing component definition
- Project new artifact resources
- Project dependents (components depending on this one)

**Execution Phase**:
- Validate new artifacts
- Check backward compatibility if required
- Build updated component definition
- Verify dependents not broken

**Emission Phase**:
- Emit new component definition resource (new CID, immutable versioning)
- Emit change log entry
- Update component index

**Outputs**:
- New component definition CID
- Change summary
- Compatibility status

### Component Delete

**Purpose**: Remove component from catalog.

**Inputs**:
- Component namespace or CID
- Force flag (delete even with dependents)

**Projection Phase**:
- Project component definition
- Project reverse dependencies (what depends on this)

**Execution Phase**:
- Check for dependents
- If dependents exist and not forced, fail
- Mark component as deleted

**Emission Phase**:
- Emit deletion marker resource
- Update component index (remove from catalog)
- Emit dependency warning if forced deletion

**Outputs**:
- Deletion status
- Affected dependents list

### Component Validate

**Purpose**: Verify component definition correctness.

**Inputs**:
- Component namespace or CID

**Projection Phase**:
- Project component definition
- Project all referenced resources
- Project schemas for validation

**Execution Phase**:
- Validate spec against schema
- Validate conformance resources
- Check reference integrity
- Verify dependencies resolvable

**Emission Phase**:
- Emit validation report resource

**Outputs**:
- Validation result (pass/fail)
- Detailed validation messages
- Report CID

## Instance Operations

Operations for managing running instances.

### Instance Create

**Purpose**: Instantiate a component definition.

**Inputs**:
- Component definition CID or namespace
- Instance configuration
- Initial state (optional)

**Projection Phase**:
- Project component definition
- Project dependency components (transitive)
- Project runtime context

**Execution Phase**:
- Resolve dependencies
- Initialize instance state
- Allocate resources
- Prepare execution environment

**Emission Phase**:
- Emit instance resource (initial state)
- Emit instance catalog entry
- Emit initialization log

**Outputs**:
- Instance ID and CID
- Initialization status

### Instance Start

**Purpose**: Begin instance execution.

**Inputs**:
- Instance ID

**Projection Phase**:
- Project instance resource
- Project component definition
- Project dependencies

**Execution Phase**:
- Load instance state
- Begin execution per component definition
- Start emission streams (logs, state, data)

**Emission Phase**:
- Emit instance state update (status: running)
- Emit startup logs
- Begin continuous emission stream

**Outputs**:
- Start status
- Instance runtime information

### Instance Stop

**Purpose**: Halt instance execution.

**Inputs**:
- Instance ID
- Graceful timeout (optional)

**Projection Phase**:
- Project running instance resource

**Execution Phase**:
- Signal instance to stop
- Wait for graceful shutdown (up to timeout)
- Force stop if timeout exceeded
- Finalize state

**Emission Phase**:
- Emit final instance state (status: stopped)
- Emit shutdown logs
- Emit execution summary

**Outputs**:
- Stop status
- Final state CID

### Instance Restart

**Purpose**: Stop and start instance.

**Inputs**:
- Instance ID
- Restart options

**Projection Phase**:
- Project instance resource

**Execution Phase**:
- Execute stop operation
- Execute start operation

**Emission Phase**:
- Emissions from stop and start operations
- Emit restart log entry

**Outputs**:
- Restart status
- New instance state CID

### Instance Inspect

**Purpose**: Retrieve instance state and status.

**Inputs**:
- Instance ID

**Projection Phase**:
- Project instance resource
- Project recent logs
- Project component definition

**Execution Phase**:
- Assemble current instance state
- Gather runtime information
- Compute uptime and metrics

**Emission Phase**:
- None (read-only)
- May cache inspection result

**Outputs**:
- Instance state
- Status information
- Logs and metrics

## View Operations

Operations for managing materialized views.

### View Materialize

**Purpose**: Execute projection and emit result as view resource.

**Inputs**:
- View definition CID
- Projection parameters
- Materialization options

**Projection Phase**:
- Project view definition
- Execute view's projection (project resources per view definition)

**Execution Phase**:
- Transform projection result to view format
- Apply indexing or optimization
- Validate view structure

**Emission Phase**:
- Emit view resource
- Emit view catalog entry
- Update view index

**Outputs**:
- View CID
- Materialization timestamp
- Statistics (resources processed, view size)

### View Refresh

**Purpose**: Update existing view with current store state.

**Inputs**:
- View CID or identifier
- Refresh strategy (full, incremental)

**Projection Phase**:
- Project existing view
- Project new/changed resources since last refresh
- Project view definition

**Execution Phase**:
- Determine changes since last materialization
- Apply incremental updates or full rebuild
- Validate updated view

**Emission Phase**:
- Emit refreshed view resource (new CID)
- Update view catalog with new version
- Emit refresh log

**Outputs**:
- New view CID
- Changes applied
- Refresh statistics

### View Query

**Purpose**: Execute query against materialized view.

**Inputs**:
- View CID or identifier
- Query parameters (filters, search terms)

**Projection Phase**:
- Project view resource
- Apply query filters

**Execution Phase**:
- Search or filter within view
- Compute results
- Apply pagination or limits

**Emission Phase**:
- None (read-only)
- May emit query result cache

**Outputs**:
- Query results
- Result metadata (count, timing)

## Artifact Operations

Operations for staging resources before component creation.

### Artifact Submit

**Purpose**: Stage a resource for later use in component creation.

**Inputs**:
- Artifact content
- Artifact type (spec, interface, docs, etc.)

**Projection Phase**:
- None (no existing resources needed)

**Execution Phase**:
- Validate artifact content
- Check artifact type valid

**Emission Phase**:
- Emit artifact resource to store
- Record in artifact staging area

**Outputs**:
- Artifact CID
- Staging status

### Manifest Submit

**Purpose**: Create component from staged artifacts.

**Inputs**:
- Manifest listing artifact CIDs and types
- Component namespace

**Projection Phase**:
- Project staged artifact resources
- Project component model

**Execution Phase**:
- Execute component create operation using artifacts
- Clear artifact staging area

**Emission Phase**:
- Emissions from component create
- Clear staging artifacts

**Outputs**:
- Component definition CID
- Component creation status

## Validation Operations

Operations for verifying correctness.

### Schema Validate

**Purpose**: Validate resource against schema.

**Inputs**:
- Resource CID
- Schema CID or type

**Projection Phase**:
- Project resource to validate
- Project schema definition

**Execution Phase**:
- Apply schema validation
- Collect validation errors/warnings

**Emission Phase**:
- Emit validation report

**Outputs**:
- Validation result
- Error details

### Reference Validate

**Purpose**: Verify reference integrity.

**Inputs**:
- Resource CID
- Traversal depth

**Projection Phase**:
- Project resource
- Project all referenced resources (recursively)

**Execution Phase**:
- Check all CID references exist
- Verify no dangling references
- Detect reference cycles if problematic

**Emission Phase**:
- Emit reference validation report

**Outputs**:
- Integrity status
- Missing references
- Cycle information

## Catalog Operations

Operations for discovering resources.

### Catalog List

**Purpose**: List available resources by type.

**Inputs**:
- Resource type (components, instances, views)
- Filters (namespace pattern, status)

**Projection Phase**:
- Project catalog view for resource type
- Apply filters

**Execution Phase**:
- Extract matching entries
- Sort and paginate

**Emission Phase**:
- None (read-only)
- May emit query result cache

**Outputs**:
- Resource list
- Metadata

### Catalog Search

**Purpose**: Search resources by content or metadata.

**Inputs**:
- Search query (text, patterns)
- Resource types to search

**Projection Phase**:
- Project search indexes or catalog views
- Execute search query

**Execution Phase**:
- Rank results by relevance
- Apply pagination

**Emission Phase**:
- None (read-only)
- May emit search result cache

**Outputs**:
- Search results
- Relevance scores

## Dependency Operations

Operations for managing component relationships.

### Dependency Resolve

**Purpose**: Compute transitive dependencies for a component.

**Inputs**:
- Component CID or namespace

**Projection Phase**:
- Project component definition
- Project dependency view or build graph

**Execution Phase**:
- Traverse dependency references
- Build transitive closure
- Check for conflicts or cycles

**Emission Phase**:
- Emit dependency resolution result
- Emit dependency graph

**Outputs**:
- Resolved dependencies (all required components)
- Resolution status
- Conflict information

## Operation Composition

Operations can compose:

**Sequential**: Component Create → Instance Create → Instance Start (build workflow).

**Parallel**: Multiple Instance Start operations concurrently.

**Conditional**: Instance Stop → if graceful failed → Instance Force Stop.

**Transactional**: Artifact Submit (multiple) → Manifest Submit (atomic component creation).

Composition enables complex workflows from simple operations.

## Operation Atomicity

Operations are atomic with respect to emissions:
- All emissions succeed or all fail
- Store remains consistent regardless of operation outcome
- Partial failures result in complete rollback

This enables reliable operations even with concurrent access.

## Operation Idempotence

Many operations are idempotent:
- Repeating with same inputs produces same result
- Component Create with identical artifacts produces same CID
- View Materialize with same parameters produces same view

Idempotence enables safe retries after failures.

## Operation Observability

All operations emit logs and metadata:
- Operation start/end timestamps
- Resources projected
- Execution duration
- Success/failure status
- Emitted resource CIDs

This provides complete audit trail and debugging information.

## Next Steps

Operations are invoked through the **Platform API**—the client interface for interacting with the platform. The next document describes the API model, request/response patterns, and how clients use the API to perform operations.