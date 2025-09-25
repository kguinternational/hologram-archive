#!/usr/bin/env node
import { Server } from "@modelcontextprotocol/sdk/server/index.js";
import { StdioServerTransport } from "@modelcontextprotocol/sdk/server/stdio.js";
import {
  CallToolRequestSchema,
  ListToolsRequestSchema,
} from "@modelcontextprotocol/sdk/types.js";
import { validateOperation } from "./operations/validate.js";
import { readOperation } from "./operations/read.js";
import { createOperation } from "./operations/create.js";
import { updateOperation } from "./operations/update.js";
import { deleteOperation } from "./operations/delete.js";
import { submitArtifactOperation } from "./operations/artifact.js";
import { submitManifestOperation } from "./operations/manifest.js";
import {
  getComponentModelOperation,
  getSchemaOperation,
  listComponentsOperation,
  getComponentExampleOperation
} from "./operations/discover.js";

const server = new Server(
  {
    name: "hologram-component-manager",
    version: "0.1.0",
  },
  {
    capabilities: {
      tools: {},
    },
  }
);

// Handle tool listing
server.setRequestHandler(ListToolsRequestSchema, async () => {
  return {
    tools: [
      {
        name: "submitArtifact",
        description: "Submit and validate an individual artifact (spec, implementation, or conformance)",
        inputSchema: {
          type: "object",
          properties: {
            content: {
              type: "object",
              description: "The JSON content of the artifact",
            },
            type: {
              type: "string",
              enum: ["spec", "implementation", "conformance"],
              description: "Type of artifact",
            },
          },
          required: ["content", "type"],
        },
      },
      {
        name: "submitManifest",
        description: "Submit a manifest to create a complete component from validated artifacts",
        inputSchema: {
          type: "object",
          properties: {
            namespace: {
              type: "string",
              description: "Component namespace",
            },
            artifacts: {
              type: "object",
              description: "Map of artifact type to CID",
              properties: {
                spec: { type: "string" },
                implementation: { type: "string" },
                interface: { type: "string" },
                docs: { type: "string" },
                test: { type: "string" },
                manager: { type: "string" },
              },
              required: ["spec", "implementation"],
            },
          },
          required: ["namespace", "artifacts"],
        },
      },
      {
        name: "validate",
        description: "Validate a component or all components in spec/",
        inputSchema: {
          type: "object",
          properties: {
            namespace: {
              type: "string",
              description: "Component namespace to validate (omit to validate all)",
            },
          },
        },
      },
      {
        name: "read",
        description: "Read component files",
        inputSchema: {
          type: "object",
          properties: {
            namespace: {
              type: "string",
              description: "Component namespace",
            },
            file: {
              type: "string",
              description: "Specific file to read (omit to read all)",
            },
          },
          required: ["namespace"],
        },
      },
      {
        name: "create",
        description: "Create a new component with all 6 required files",
        inputSchema: {
          type: "object",
          properties: {
            namespace: {
              type: "string",
              description: "Component namespace",
            },
            files: {
              type: "object",
              description: "Component files content",
              properties: {
                spec: { type: "object" },
                implementation: { type: "object" },
                interface: { type: "object" },
                docs: { type: "object" },
                test: { type: "object" },
                manager: { type: "object" },
              },
              required: ["spec", "implementation", "interface", "docs", "test", "manager"],
            },
          },
          required: ["namespace", "files"],
        },
      },
      {
        name: "update",
        description: "Update component files",
        inputSchema: {
          type: "object",
          properties: {
            namespace: {
              type: "string",
              description: "Component namespace",
            },
            files: {
              type: "object",
              description: "Files to update",
              properties: {
                spec: { type: "object" },
                implementation: { type: "object" },
                interface: { type: "object" },
                docs: { type: "object" },
                test: { type: "object" },
                manager: { type: "object" },
              },
            },
          },
          required: ["namespace", "files"],
        },
      },
      {
        name: "delete",
        description: "Delete a component",
        inputSchema: {
          type: "object",
          properties: {
            namespace: {
              type: "string",
              description: "Component namespace to delete",
            },
          },
          required: ["namespace"],
        },
      },
      {
        name: "getComponentModel",
        description: "Get the Hologram component model - explains what files are needed",
        inputSchema: {
          type: "object",
          properties: {},
        },
      },
      {
        name: "getSchema",
        description: "Get a specific schema to understand structure requirements",
        inputSchema: {
          type: "object",
          properties: {
            schemaName: {
              type: "string",
              description: "Name of schema (e.g., 'hologram.interface')",
            },
          },
          required: ["schemaName"],
        },
      },
      {
        name: "listComponents",
        description: "List all components and their validation status",
        inputSchema: {
          type: "object",
          properties: {},
        },
      },
      {
        name: "getComponentExample",
        description: "Get example component structures as templates",
        inputSchema: {
          type: "object",
          properties: {
            componentType: {
              type: "string",
              enum: ["simple", "conformance", "operation"],
              description: "Type of example to show",
            },
          },
        },
      },
    ],
  };
});

// Handle tool execution
server.setRequestHandler(CallToolRequestSchema, async (request) => {
  const { name, arguments: args } = request.params;

  try {
    switch (name) {
      case "submitArtifact":
        return await submitArtifactOperation(
          args?.content as any,
          args?.type as 'spec' | 'implementation' | 'conformance'
        );

      case "submitManifest":
        return await submitManifestOperation(
          args?.namespace as string,
          args?.artifacts as Record<string, string>
        );

      case "validate":
        return await validateOperation(args?.namespace as string | undefined);

      case "read":
        return await readOperation(
          args?.namespace as string,
          args?.file as string | undefined
        );

      case "create":
        return await createOperation(
          args?.namespace as string,
          args?.files as any
        );

      case "update":
        return await updateOperation(
          args?.namespace as string,
          args?.files as any
        );

      case "delete":
        return await deleteOperation(args?.namespace as string);

      case "getComponentModel":
        return await getComponentModelOperation();

      case "getSchema":
        return await getSchemaOperation(args?.schemaName as string);

      case "listComponents":
        return await listComponentsOperation();

      case "getComponentExample":
        return await getComponentExampleOperation(
          args?.componentType as 'simple' | 'conformance' | 'operation' | undefined
        );

      default:
        throw new Error(`Unknown tool: ${name}`);
    }
  } catch (error) {
    return {
      content: [
        {
          type: "text",
          text: `Error: ${error instanceof Error ? error.message : "Unknown error"}`,
        },
      ],
    };
  }
});

async function run() {
  const transport = new StdioServerTransport();
  await server.connect(transport);
  console.error("Hologram Component Manager MCP Server running");
}

run().catch((error) => {
  console.error("Server error:", error);
  process.exit(1);
});