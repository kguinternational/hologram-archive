import * as fs from 'fs';
import * as path from 'path';
import { SchemaValidator } from '../core/schema-validator.js';

/**
 * Get the component model - what files are required for a complete component
 */
export async function getComponentModelOperation(): Promise<{ content: Array<{ type: string; text: string }> }> {
  const validator = new SchemaValidator(path.join(process.cwd(), 'spec'));

  try {
    const componentModel = await validator.getComponentRequirements();

    if (!componentModel) {
      return {
        content: [{
          type: 'text',
          text: '❌ Component model (hologram.component.json) not found'
        }]
      };
    }

    // Build a guide for the LLM
    let guide = 'HOLOGRAM COMPONENT MODEL\n';
    guide += '========================\n\n';

    guide += 'To create a complete Hologram component, you need exactly 6 files:\n\n';

    guide += '1. SPECIFICATION FILE ({namespace}.spec.json):\n';
    guide += '   - Purpose: JSON Schema that defines the structure\n';
    guide += '   - Schema: Must be a valid JSON Schema\n';
    guide += '   - Example: See hologram.component.spec.json\n\n';

    guide += '2. IMPLEMENTATION FILE ({namespace}.json):\n';
    guide += '   - Purpose: The actual component implementation\n';
    guide += '   - Schema: Must validate against {namespace}.spec.json\n';
    guide += '   - Required fields: namespace, parent, conformance\n\n';

    guide += '3. REQUIRED CONFORMANCE FILES:\n';

    const conformanceDescriptions: Record<string, string> = {
      interface: 'Every component must define its public interface',
      docs: 'Every component must have documentation',
      test: 'Every component must have tests',
      manager: 'Every component must define lifecycle management'
    };

    for (const [key, req] of Object.entries(componentModel.component.conformance_requirements)) {
      const requirement = req as any;
      if (requirement.required) {
        guide += `\n   ${key.toUpperCase()} ({namespace}.${key}.json):\n`;
        guide += `   - Purpose: ${conformanceDescriptions[key] || requirement.description || 'Required conformance'}\n`;
        guide += `   - Schema: ${requirement.schema}\n`;
        guide += `   - Required fields: namespace, parent, conformance=true, ${key}\n`;
      }
    }

    guide += '\nKEY RULES:\n';
    guide += '• All namespaces must follow pattern: hologram[.subcomponent]*\n';
    guide += '• Parent must reference the correct parent component\n';
    guide += '• Conformance flag: false for implementations, true for conformance files\n';
    guide += '• Every conformance component (interface, docs, test, manager) must have its own full conformance\n';
    guide += '• Self-referential conformance is required (e.g., test.test.json)\n';

    guide += '\nWORKFLOW:\n';
    guide += '1. Use submitArtifact for each file (validates individually)\n';
    guide += '2. Collect CIDs from each submission\n';
    guide += '3. Use submitManifest with namespace and CIDs to create complete component\n';

    return {
      content: [{
        type: 'text',
        text: guide
      }]
    };
  } catch (error) {
    return {
      content: [{
        type: 'text',
        text: `Error: ${error instanceof Error ? error.message : 'Unknown error'}`
      }]
    };
  }
}

/**
 * Get a specific schema to understand structure requirements
 */
export async function getSchemaOperation(
  schemaName: string
): Promise<{ content: Array<{ type: string; text: string }> }> {
  const specDir = path.join(process.cwd(), 'spec');

  try {
    // Add .spec.json if not provided
    const fileName = schemaName.endsWith('.json')
      ? schemaName
      : `${schemaName}.spec.json`;

    const schemaPath = path.join(specDir, fileName);

    if (!fs.existsSync(schemaPath)) {
      // List available schemas to help
      const files = fs.readdirSync(specDir)
        .filter(f => f.endsWith('.spec.json'))
        .map(f => f.replace('.spec.json', ''));

      return {
        content: [{
          type: 'text',
          text: `Schema '${schemaName}' not found.\n\nAvailable schemas:\n${files.map(f => `  - ${f}`).join('\n')}\n\nUse: getSchema("hologram.interface") to see interface schema`
        }]
      };
    }

    const schema = JSON.parse(fs.readFileSync(schemaPath, 'utf-8'));

    // Add helpful context
    let response = `SCHEMA: ${fileName}\n`;
    response += '=' .repeat(50) + '\n\n';
    response += JSON.stringify(schema, null, 2);
    response += '\n\n';
    response += 'KEY POINTS:\n';

    // Extract required fields
    if (schema.required) {
      response += `• Required fields: ${schema.required.join(', ')}\n`;
    }

    // Extract properties
    if (schema.properties) {
      response += `• Properties: ${Object.keys(schema.properties).join(', ')}\n`;
    }

    // Note patterns
    if (schema.properties?.namespace?.pattern) {
      response += `• Namespace pattern: ${schema.properties.namespace.pattern}\n`;
    }

    return {
      content: [{
        type: 'text',
        text: response
      }]
    };
  } catch (error) {
    return {
      content: [{
        type: 'text',
        text: `Error: ${error instanceof Error ? error.message : 'Unknown error'}`
      }]
    };
  }
}

/**
 * List all components and their status
 */
export async function listComponentsOperation(): Promise<{ content: Array<{ type: string; text: string }> }> {
  const validator = new SchemaValidator(path.join(process.cwd(), 'spec'));

  try {
    const validation = await validator.validateAllComponents();

    let response = 'HOLOGRAM COMPONENTS STATUS\n';
    response += '=' .repeat(50) + '\n\n';

    const components = Array.from(validation.componentResults.keys()).sort();

    for (const namespace of components) {
      const result = validation.componentResults.get(namespace);
      const status = result?.valid ? '✅' : '❌';

      response += `${status} ${namespace}`;

      if (!result?.valid && result?.errors) {
        const missingFiles = result.errors
          .filter(e => e.message.includes('Required file missing'))
          .map(e => e.file.replace(`${namespace}.`, '').replace('.json', ''));

        if (missingFiles.length > 0) {
          response += ` (missing: ${missingFiles.join(', ')})`;
        }
      }

      response += '\n';
    }

    response += '\n' + '=' .repeat(50) + '\n';
    response += `Total: ${components.length} components\n`;
    response += `Valid: ${Array.from(validation.componentResults.values()).filter(r => r.valid).length}\n`;
    response += `Invalid: ${Array.from(validation.componentResults.values()).filter(r => !r.valid).length}\n`;

    response += '\nTo create a new component, use getComponentModel() for guidance';

    return {
      content: [{
        type: 'text',
        text: response
      }]
    };
  } catch (error) {
    return {
      content: [{
        type: 'text',
        text: `Error: ${error instanceof Error ? error.message : 'Unknown error'}`
      }]
    };
  }
}

/**
 * Get an example component structure to use as a template
 */
export async function getComponentExampleOperation(
  componentType?: 'simple' | 'conformance' | 'operation'
): Promise<{ content: Array<{ type: string; text: string }> }> {
  const type = componentType || 'simple';

  let example = `EXAMPLE: Creating a ${type} component\n`;
  example += '=' .repeat(50) + '\n\n';

  switch (type) {
    case 'conformance':
      example += 'EXAMPLE: hologram.newconformance\n\n';
      example += '1. Spec file (hologram.newconformance.spec.json):\n';
      example += JSON.stringify({
        "$schema": "http://json-schema.org/draft-07/schema#",
        "$id": "hologram.newconformance.spec.json",
        "title": "New Conformance Type",
        "type": "object",
        "properties": {
          "namespace": { "type": "string" },
          "parent": { "type": "string" },
          "conformance": { "type": "boolean" },
          "newconformance": {
            "type": "object",
            "properties": {
              "version": { "type": "string" },
              "description": { "type": "string" }
            }
          }
        },
        "required": ["namespace", "parent", "conformance", "newconformance"]
      }, null, 2);

      example += '\n\n2. Implementation (hologram.newconformance.json):\n';
      example += JSON.stringify({
        "namespace": "hologram.newconformance",
        "parent": "hologram",
        "conformance": false,
        "version": "0.1.0",
        "description": "Defines new conformance type"
      }, null, 2);

      example += '\n\n3. SELF-REFERENTIAL conformance (hologram.newconformance.newconformance.json):\n';
      example += JSON.stringify({
        "namespace": "hologram.newconformance.newconformance",
        "parent": "hologram.newconformance",
        "conformance": true,
        "newconformance": {
          "version": "0.1.0",
          "description": "Conformance for the conformance spec itself!"
        }
      }, null, 2);
      break;

    default:
      example += 'COMPLETE WORKING EXAMPLE: hologram.example\n\n';
      example += 'This is a complete, valid component you can use as a template:\n\n';

      example += '1. SPEC FILE (hologram.example.spec.json):\n';
      example += JSON.stringify({
        "$schema": "http://json-schema.org/draft-07/schema#",
        "$id": "hologram.example.spec.json",
        "title": "Example Component",
        "description": "A simple example component to demonstrate structure",
        "type": "object",
        "properties": {
          "namespace": {
            "type": "string",
            "pattern": "^hologram\\.example(\\.\\w+)*$"
          },
          "parent": { "type": "string" },
          "conformance": { "type": "boolean" },
          "version": { "type": "string" },
          "description": { "type": "string" },
          "config": {
            "type": "object",
            "properties": {
              "enabled": { "type": "boolean" },
              "settings": { "type": "object" }
            }
          }
        },
        "required": ["namespace", "parent", "conformance"]
      }, null, 2);

      example += '\n\n2. IMPLEMENTATION (hologram.example.json):\n';
      example += JSON.stringify({
        "namespace": "hologram.example",
        "parent": "hologram",
        "conformance": false,
        "version": "0.1.0",
        "description": "Example component implementation",
        "config": {
          "enabled": true,
          "settings": {}
        }
      }, null, 2);

      example += '\n\n3. INTERFACE (hologram.example.interface.json):\n';
      example += JSON.stringify({
        "namespace": "hologram.example.interface",
        "parent": "hologram.example",
        "conformance": true,
        "interface": {
          "version": "0.1.0",
          "description": "Public interface for example component",
          "methods": {
            "initialize": {
              "description": "Initialize the example component",
              "input": { "type": "object" },
              "output": { "type": "boolean" }
            },
            "execute": {
              "description": "Execute example operation",
              "input": { "type": "object" },
              "output": { "type": "object" }
            }
          }
        }
      }, null, 2);

      example += '\n\n4. DOCS (hologram.example.docs.json):\n';
      example += JSON.stringify({
        "namespace": "hologram.example.docs",
        "parent": "hologram.example",
        "conformance": true,
        "docs": {
          "version": "0.1.0",
          "description": "Documentation for example component",
          "sections": [
            {
              "title": "Overview",
              "content": "This example component demonstrates the structure of a Hologram component."
            },
            {
              "title": "Usage",
              "content": "Use this as a template for creating new components."
            }
          ]
        }
      }, null, 2);

      example += '\n\n5. TEST (hologram.example.test.json):\n';
      example += JSON.stringify({
        "namespace": "hologram.example.test",
        "parent": "hologram.example",
        "conformance": true,
        "test": {
          "version": "0.1.0",
          "description": "Tests for example component",
          "tests": [
            {
              "name": "initialization",
              "description": "Test component initialization",
              "type": "unit"
            },
            {
              "name": "execution",
              "description": "Test component execution",
              "type": "integration"
            }
          ]
        }
      }, null, 2);

      example += '\n\n6. MANAGER (hologram.example.manager.json):\n';
      example += JSON.stringify({
        "namespace": "hologram.example.manager",
        "parent": "hologram.example",
        "conformance": true,
        "manager": {
          "version": "0.1.0",
          "description": "Lifecycle management for example component",
          "operations": {
            "create": { "supported": true },
            "read": { "supported": true },
            "update": { "supported": true },
            "delete": { "supported": true },
            "validate": { "supported": true }
          }
        }
      }, null, 2);

      example += '\n\nWORKFLOW TO CREATE THIS COMPONENT:\n';
      example += '1. Call submitArtifact() with each JSON above\n';
      example += '2. Collect the CID from each response\n';
      example += '3. Call submitManifest() with:\n';
      example += JSON.stringify({
        "namespace": "hologram.example",
        "artifacts": {
          "spec": "cid:...",
          "implementation": "cid:...",
          "interface": "cid:...",
          "docs": "cid:...",
          "test": "cid:...",
          "manager": "cid:..."
        }
      }, null, 2);
      break;
  }

  return {
    content: [{
      type: 'text',
      text: example
    }]
  };
}