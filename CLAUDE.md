# CLAUDE.md

Guidance for Claude Code when working with the Hologram repository.

## Current State

- **spec/**: 72 files (12 components × 6 files each)
- **src/**: MCP server implementation with validation tools
- All components are valid and conform to schemas

## Directory Structure

```
/workspaces/Hologram/
├── spec/                  # Component definitions
├── src/                   # MCP server and tools
│   ├── core/             # Schema validator, artifact store
│   ├── operations/       # CRUD+V operations
│   ├── utils/            # Garbage collection
│   └── __tests__/        # Integration tests
├── Makefile              # Build and maintenance
└── .artifacts/           # Temporary staging (can be cleared)
```

## Commands

From project root:
```bash
make build              # Build TypeScript
make test               # Run tests
make gc                 # Analyze spec/ for orphans
make gc-clean           # Remove orphaned files
make clean-artifacts    # Clear .artifacts staging
make mcp                # Start MCP server
```

From src/ directory:
```bash
npm run build           # Build TypeScript
npm run test            # Run tests
npm run gc              # Analyze garbage
npm run gc:clean        # Clean garbage
```

## Component Structure

Each component has exactly 6 files:
1. `namespace.index.json` - Maps artifacts to content addresses
2. `namespace.<hash>.json` (spec) - Component specification
3. `namespace.<hash>.json` (interface) - Interface conformance
4. `namespace.<hash>.json` (docs) - Documentation conformance
5. `namespace.<hash>.json` (test) - Test conformance
6. `namespace.<hash>.json` (manager) - Manager conformance

## Setting Up MCP Server

```bash
# Build the server
cd src && npm run build && cd ..

# Add to Claude Code
claude mcp add hologram "node" "src/dist/mcp-server.js"

# Verify it's added
claude mcp list

# Remove if needed
claude mcp remove hologram
```

## MCP Server Tools

Once added, the MCP server provides these tools:
- `listComponents` - Show all components and status
- `validate` - Validate a component or all
- `read` - Read component files
- `create` - Create new component (via artifacts)
- `update` - Update existing component
- `delete` - Delete component with dependency check
- `submitArtifact` - Submit artifact for staging
- `submitManifest` - Create component from artifacts
- `getComponentModel` - Get component requirements
- `getSchema` - Get specific schema

## Working with Components

**Use MCP tools only** for spec/ directory operations:
- Never directly edit files in spec/
- All operations must go through MCP server
- This ensures validation and consistency

## Validation Rules

1. All components must have 6 files
2. All files must validate against schemas
3. Conformance files must have correct parent
4. Content is addressed by SHA256 hash
5. Operations are atomic (all-or-nothing)

## Maintenance

- `.artifacts/` is temporary and can be cleared
- Orphaned files in spec/ can be cleaned with `make gc-clean`
- All content uses canonical JSON (sorted keys)