# Hologram

A component-based system defined through JSON Schema specifications with content-addressed storage.

## Structure

```
spec/           # Component definitions (72 files)
src/            # MCP server and validation tools
Makefile        # Build and maintenance commands
```

## Components

The system currently contains 12 validated components:
- `hologram` - Root component
- `hologram.component` - Component model definition
- `hologram.spec` - Specification schema
- `hologram.interface` - Interface conformance
- `hologram.docs` - Documentation conformance
- `hologram.test` - Test conformance
- `hologram.manager` - Lifecycle management conformance
- `hologram.create` - Create operation
- `hologram.read` - Read operation
- `hologram.update` - Update operation
- `hologram.validate` - Validation operation
- `hologram.index` - Index structure

Each component has 6 files:
- spec.json - Component specification
- interface.json - Interface definition
- docs.json - Documentation
- test.json - Test specifications
- manager.json - Lifecycle management
- index.json - File mappings

## Usage

### Build and Test
```bash
make build          # Build TypeScript
make test           # Run tests
make validate       # Validate all components
```

### Maintenance
```bash
make gc             # Analyze spec/ for orphaned files
make gc-clean       # Remove orphaned files
make clean-artifacts # Clear .artifacts staging area
```

### MCP Server

Start the server manually:
```bash
make mcp            # Start MCP server
make mcp-validate   # Validate via MCP
```

Add to Claude Code:
```bash
# From project root
claude mcp add hologram "node" "src/dist/mcp-server.js"

# Or with full path
claude mcp add hologram "node" "/workspaces/Hologram/src/dist/mcp-server.js"
```

## MCP Tools

The Model Context Protocol server provides these tools:

**Core Operations:**
- `validate` - Validate components
- `read` - Read component files
- `create` - Create new components
- `update` - Update existing components
- `delete` - Delete components

**Artifact Management:**
- `submitArtifact` - Submit artifact for validation
- `submitManifest` - Create component from artifacts

**Discovery:**
- `listComponents` - Show all components and status
- `getComponentModel` - Get component requirements
- `getSchema` - Get specific schema
- `getComponentExample` - Get example structures
- `discover` - Discover available schemas

## File Format

All files use content-addressed storage with SHA256 hashes:
- `namespace.index.json` - Component index
- `namespace.hash.json` - Content files

## Requirements

- Node.js 22+
- TypeScript 5+