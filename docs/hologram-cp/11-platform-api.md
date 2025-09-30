# Platform API

## Client Interaction Model

The **Platform API** is how clients interact with the Hologram platform—submitting operations, retrieving results, subscribing to emissions, and querying resources. The API provides a uniform interface across different client implementations.

This document describes the API model, request/response patterns, and client interaction principles.

## API Principles

### Protocol Independence

The API model is abstract—not tied to specific protocols (HTTP, gRPC, MCP, CLI).

Different client implementations provide different protocol bindings:
- **MCP Server**: Model Context Protocol for AI tool integration
- **HTTP API**: RESTful or GraphQL over HTTP
- **CLI Client**: Command-line interface
- **SDK Libraries**: Language-specific client libraries

All bindings map to the same underlying API model.

### Operation-Centric

The API is organized around **operations** (component.create, instance.start, view.materialize).

Each operation is a distinct API endpoint or command with specific:
- Input parameters
- Execution semantics
- Output format
- Error conditions

Clients invoke operations and receive results.

### Resource-Oriented

Operations manipulate **resources** identified by CID or namespace.

The API provides:
- Resource submission (emit new resources)
- Resource retrieval (project existing resources)
- Resource querying (identify resources matching criteria)

Resources are the nouns, operations are the verbs.

### Asynchronous by Default

Many operations (instance start, view materialization) have non-trivial duration.

The API supports **asynchronous execution**:
- Submit operation request, receive operation ID
- Poll or subscribe to operation status
- Retrieve results when complete

Short operations can complete synchronously, but async is always available.

## Request Model

### Operation Requests

All operation requests have common structure:

**Operation Identifier**: Which operation to execute (e.g., "component.create").

**Parameters**: Operation-specific inputs:
- Resource CIDs (references to existing resources)
- Values (strings, numbers, structured data)
- Options (flags, preferences)

**Context**: Metadata about the request:
- Client identity (user, service account)
- Request ID (for tracing)
- Timeout preferences
- Priority hints

**Idempotency Token**: Optional token ensuring repeated requests don't duplicate operations.

### Resource Submission

Submitting resources for emission:

**Content**: The resource data (bytes or structured data).

**Metadata**: Optional metadata:
- Content type (JSON, binary, text)
- Description
- Tags for categorization

**Validation Options**: Whether to validate before emission, what schema to use.

The API computes CID, validates (if requested), and emits to store.

### Projection Requests

Requesting projection without performing full operation:

**Projection Definition**: Which projection to execute (by CID or type).

**Parameters**: Projection parameters (namespace filter, depth limit, etc.).

**Format**: Desired output format (structured data, human-readable, binary).

The API executes projection, returns container without emitting.

## Response Model

### Synchronous Responses

For operations that complete quickly:

**Status**: Success or failure indicator.

**Result**: Operation output:
- Emitted resource CIDs
- Projected resource content
- Computed values

**Metadata**: Execution information:
- Duration
- Resources accessed
- Warnings

**Errors**: If failure, detailed error information.

### Asynchronous Responses

For long-running operations:

**Operation ID**: Unique identifier for tracking operation.

**Status Endpoint**: Where to check operation status.

**Estimated Duration**: If known, expected completion time.

Clients poll status endpoint or subscribe to status updates:

**Status Updates**: Periodic messages indicating progress:
- State (pending, running, completed, failed)
- Progress percentage (if computable)
- Intermediate results

**Completion**: Final message with full results or error.

### Streaming Responses

For operations producing continuous output (instance logs, event streams):

**Stream Handle**: Identifier for the stream.

**Stream Endpoint**: Where to receive stream data.

**Stream Control**: Operations to pause, resume, or close stream.

Stream delivers items as they're emitted:
- Log entries
- State updates
- Events

Stream terminates on completion or explicit close.

## Error Model

### Error Structure

Errors provide detailed information:

**Error Code**: Machine-readable identifier (RESOURCE_NOT_FOUND, VALIDATION_FAILED).

**Error Message**: Human-readable description.

**Context**: Where error occurred:
- Operation phase (projection, execution, emission)
- Resource CID that caused error
- Stack trace (if applicable)

**Recovery Suggestions**: Possible remediation (fix schema, provide missing resource).

**Related Resources**: CIDs of resources involved in error.

### Error Categories

**Client Errors**: Invalid input, malformed request, unauthorized access.
- HTTP 4xx equivalent
- Client should fix request and retry

**Server Errors**: Platform failure, store unavailable, internal error.
- HTTP 5xx equivalent
- Client should retry (possibly with backoff)

**Resource Errors**: Resource not found, validation failed, reference broken.
- Specific to Hologram model
- May require emitting new/different resources

**Operation Errors**: Operation-specific failures (can't start instance, dependency conflict).
- Varies by operation
- Check operation documentation for possible errors

## Authentication and Authorization

### Identity

Clients authenticate, establishing identity:
- User identity (human user)
- Service identity (automated client)
- Anonymous (if platform allows)

Identity is provided in request context.

### Permissions

The platform enforces access control:
- Read permissions (project resources)
- Write permissions (emit resources)
- Operation permissions (execute specific operations)

Permission model is flexible:
- Resource-based (permissions per resource namespace)
- Role-based (roles grant permission sets)
- Attribute-based (contextual permissions)

Unauthorized operations return permission denied error.

## Client Patterns

Common interaction patterns.

### Submit-Retrieve Pattern

For two-phase operations (artifact/manifest):

1. Client submits multiple resources (artifacts)
2. API returns CIDs
3. Client submits manifest referencing CIDs
4. API executes operation (component create)
5. Client retrieves result

This pattern enables staging and validation before final operation.

### Subscribe-Process Pattern

For monitoring running instances:

1. Client starts instance
2. Client subscribes to instance emission stream
3. API delivers logs, state, data as emitted
4. Client processes items as received
5. Client closes stream when done

This pattern enables real-time monitoring.

### Query-Refine Pattern

For discovery:

1. Client queries catalog (broad search)
2. API returns initial results
3. Client refines query (add filters)
4. API returns refined results
5. Repeat until desired resources found

This pattern supports interactive exploration.

### Batch-Process Pattern

For high-volume operations:

1. Client submits batch of operations
2. API processes concurrently
3. API returns batch results
4. Client checks each result

This pattern optimizes throughput.

## API Versioning

### Projection Versioning

Since operations are defined by projections in the store, API evolution is natural:

**New Operations**: Emit new projection definitions to store.

**Operation Changes**: Emit new version of projection definition (new CID).

**Deprecation**: Mark old projection versions as deprecated (still available).

The API surface evolves through the same projection-emission mechanism.

### Protocol Versioning

Protocol bindings (HTTP, MCP) have independent versions:
- Protocol version indicates what features are available
- Older clients use older protocol versions
- Platform supports multiple protocol versions

## Implementation Examples

How different client types interact:

### MCP Server Client

MCP (Model Context Protocol) client for AI tools:

**Request**: AI tool invokes MCP tool (listComponents, validate, create).

**Mapping**: MCP tool maps to platform operation.

**Execution**: Platform executes operation.

**Response**: Platform returns result formatted per MCP.

**Streaming**: MCP server events for long operations.

### HTTP API Client

HTTP client for web applications:

**Request**: HTTP POST to operation endpoint with JSON body.

**Mapping**: URL path + method indicate operation, body contains parameters.

**Execution**: Platform executes operation.

**Response**: HTTP response with JSON result.

**Streaming**: Server-Sent Events or WebSocket for streams.

### CLI Client

Command-line client for operators:

**Request**: CLI command with arguments and flags.

**Mapping**: Command name maps to operation, arguments to parameters.

**Execution**: Platform executes operation.

**Response**: Formatted text output, exit code indicates success/failure.

**Streaming**: Line-by-line output for streams.

### SDK Library Client

Programming language library for applications:

**Request**: Function call with typed parameters.

**Mapping**: Function name maps to operation.

**Execution**: Library serializes request, sends to platform, deserializes response.

**Response**: Typed result objects.

**Streaming**: Iterator or observable pattern for streams.

## API Discovery

Clients can discover available operations:

**List Operations**: API endpoint returns available operations with descriptions.

**Operation Schema**: Each operation provides input/output schema.

**Examples**: Operations include usage examples.

**Documentation**: API provides links to detailed documentation.

Discovery enables dynamic clients that adapt to platform capabilities.

## Rate Limiting and Quotas

Platforms may enforce limits:

**Rate Limits**: Maximum requests per time period (per client, per operation type).

**Quotas**: Maximum resource consumption (storage, compute, emissions).

**Throttling**: Slowing requests when approaching limits.

Clients receive feedback about limits:
- Current usage
- Remaining quota
- Reset time
- Throttle status

## Caching and ETags

For read operations, caching improves performance:

**Resource ETags**: CID serves as natural ETag (content-based).

**Conditional Requests**: "If-None-Match: CID" returns 304 Not Modified if unchanged.

**Cache Headers**: Platform provides cache duration hints.

Content addressing makes caching reliable—CID guarantees content identity.

## Bulk Operations

For efficiency, API supports bulk operations:

**Batch Submission**: Submit multiple resources in one request.

**Batch Retrieval**: Retrieve multiple resources by CID list.

**Batch Operations**: Execute multiple operations atomically.

Bulk operations reduce round-trips and enable atomic multi-resource transactions.

## WebSocket/Streaming Support

For real-time interaction:

**Persistent Connection**: WebSocket or similar for bidirectional communication.

**Operation Streaming**: Submit operations, receive results over same connection.

**Emission Subscription**: Subscribe to emission streams, receive items as emitted.

**Multiplexing**: Multiple operations/streams over one connection.

Streaming enables responsive interactive applications.

## API Observability

The API provides visibility into platform behavior:

**Operation Logs**: Record of API operations performed.

**Performance Metrics**: Latency, throughput, error rates per operation.

**Resource Metrics**: Store size, emission rate, projection frequency.

**Client Metrics**: Per-client usage patterns.

Observability helps clients understand platform health and optimize usage.

## Next Steps

The platform API enables clients to interact with the system, but the platform itself must bootstrap from minimal primitives. The next section, Part IV: System Architecture, begins with **Bootstrap Architecture**—how the platform initializes itself from a minimal content-addressed store to a fully functional projection-emission system.