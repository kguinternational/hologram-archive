# MCP Server Improvements for LLM Clarity

## Problems Identified

### 1. Error Messages
- Current: "Cannot create property 'namespace' on string '{'"
- Better: "Update failed: Content for 'spec' appears to be a string. Expected an object. Ensure JSON is properly formatted."

### 2. Tool Descriptions Need Examples
Each tool should include:
```typescript
// GOOD - Update single file
mcp.update({
  namespace: "hologram.test",
  files: {
    test: { /* full content */ }
  }
})

// BAD - Won't work
mcp.update({
  namespace: "hologram.test",
  files: {
    spec: { /* requires view */ },
    view: { /* but view doesn't exist yet */ }
  }
})
```

### 3. Schema Discovery
- Add `listSchemas()` - show all available schemas
- Add `getSchemaRequirements(schemaName)` - show what fields are required
- Fix `getSchema()` to actually return schema content

### 4. Validation Clarity
Instead of:
```
❌ Failed: must be equal to one of the allowed values (at /log/output/0/type)
```

Better:
```
❌ Failed: log.output[0].type = "invalid_value"
   Expected one of: ["stdout", "stderr", "file", "syslog", "network"]
   Got: "invalid_value"
   Schema: hologram.log.spec
```

### 5. Operation Sequencing
Add to tool descriptions:
- "NOTE: When adding a new conformance requirement, first add the conformance files to all components, THEN update the spec"
- "WARNING: Update operations validate the entire component. Cannot partially update."

### 6. Debugging Tools
Add new operations:
- `validateArtifact(content, type)` - validate without saving
- `explainValidation(namespace)` - detailed breakdown of what's being checked
- `diffUpdate(namespace, files)` - preview what would change

### 7. Self-Documenting Components
Every component should have examples:
```javascript
mcp.getComponentExample("hologram.view")
// Returns actual working example of view conformance
```

## Implementation Priority

1. **High**: Fix error messages to be descriptive
2. **High**: Add examples to tool descriptions
3. **Medium**: Add validation preview tools
4. **Medium**: Fix getSchema operations
5. **Low**: Add debugging operations

## For LLM-Friendly MCP Design

1. **Explicit State Management**: Tools should explain what state they expect
2. **Atomic Operations**: Make it clear what can/cannot be done together
3. **Progressive Disclosure**: Basic usage in description, advanced in examples
4. **Fail Gracefully**: Errors should suggest fixes
5. **Discoverable**: All schemas, requirements, and patterns should be queryable