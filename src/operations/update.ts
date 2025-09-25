import * as fs from 'fs';
import * as path from 'path';
import * as crypto from 'crypto';
import { SchemaValidator } from '../core/schema-validator.js';
import { ComponentIndex, ValidationError } from '../types.js';

export async function updateOperation(
  namespace: string,
  files: Partial<any>,
  specDir: string = path.join(process.cwd(), 'spec')
): Promise<{ content: Array<{ type: string; text: string }> }> {
  const validator = new SchemaValidator(specDir);
  const errors: ValidationError[] = [];

  try {
    // Check that component exists via index
    const indexPath = path.join(specDir, `${namespace}.index.json`);
    if (!fs.existsSync(indexPath)) {
      return {
        content: [
          {
            type: 'text',
            text: `Component ${namespace} does not exist. Use create operation to add new components.`,
          },
        ],
      };
    }

    // Load current index
    const index: ComponentIndex = JSON.parse(fs.readFileSync(indexPath, 'utf-8'));

    // Backup current artifacts
    const backupArtifacts = new Map<string, string>();
    const backupFilenames = new Map<string, string>();

    for (const [type, artifactRef] of Object.entries(index.artifacts)) {
      if (artifactRef) {
        const filename = `${artifactRef}.json`;
        const filePath = path.join(specDir, filename);
        if (fs.existsSync(filePath)) {
          backupArtifacts.set(type, fs.readFileSync(filePath, 'utf-8'));
          backupFilenames.set(type, filename);
        }
      }
    }

    // Phase 1: Validate and prepare updates
    const updates: Map<string, any> = new Map();
    const newArtifacts: Map<string, string> = new Map();

    for (const [key, content] of Object.entries(files)) {
      if (!content) continue;

      // Determine expected namespace for this file type
      const expectedNamespace = key === 'implementation'
        ? namespace
        : `${namespace}.${key}`;

      // Add/update required fields
      content.namespace = expectedNamespace;

      // Set conformance flag
      const isConformance = ['interface', 'docs', 'test', 'manager'].includes(key);
      content.conformance = isConformance;

      // Set parent for conformance files
      if (isConformance) {
        content.parent = namespace;
      }

      // Validate against base schema
      const baseValidation = await validator.validateBaseSchema(content);
      if (!baseValidation.valid) {
        errors.push(...baseValidation.errors.map(e => ({
          ...e,
          file: `${namespace}.${key === 'implementation' ? '' : key + '.'}json`,
        })));
      }

      // Validate conformance files against their specs
      if (isConformance) {
        const conformanceSpecFile = `hologram.${key}.spec`;
        const conformanceValidation = await validator.validateAgainstSchema(content, conformanceSpecFile);
        if (!conformanceValidation.valid) {
          errors.push(...conformanceValidation.errors.map(e => ({
            ...e,
            file: `${namespace}.${key}.json`,
          })));
        }
      }

      updates.set(key, content);
    }

    // If validation errors, don't proceed
    if (errors.length > 0) {
      return formatErrors(namespace, errors);
    }

    // Phase 2: Write new content-addressed files
    const writtenFiles: string[] = [];
    const newIndex: ComponentIndex = {
      namespace,
      artifacts: { ...index.artifacts }, // Start with existing artifacts
    };

    try {
      for (const [type, content] of updates) {
        // Calculate hash for new content
        const jsonContent = JSON.stringify(content, null, 2);
        const hash = crypto.createHash('sha256').update(jsonContent).digest('hex');
        const newFilename = `${namespace}.${hash}.json`;
        const newFilePath = path.join(specDir, newFilename);

        // Write new content-addressed file
        fs.writeFileSync(newFilePath, jsonContent);
        writtenFiles.push(newFilename);

        // Update index with new artifact reference (without .json)
        newIndex.artifacts[type] = `${namespace}.${hash}`;
      }

      // Phase 3: Validate complete component after updates
      // Temporarily write the new index for validation
      const tempIndexBackup = fs.readFileSync(indexPath, 'utf-8');
      fs.writeFileSync(indexPath, JSON.stringify(newIndex, null, 2));

      const componentValidation = await validator.validateComponent(namespace);
      if (!componentValidation.valid) {
        // Restore index and cleanup new files
        fs.writeFileSync(indexPath, tempIndexBackup);
        for (const filename of writtenFiles) {
          try {
            fs.unlinkSync(path.join(specDir, filename));
          } catch {
            // Ignore cleanup errors
          }
        }
        return formatErrors(namespace, componentValidation.errors);
      }

      // Phase 4: Cleanup old artifact files that were replaced
      for (const [type, oldFilename] of backupFilenames) {
        if (updates.has(type) && newIndex.artifacts[type] !== index.artifacts[type]) {
          try {
            fs.unlinkSync(path.join(specDir, oldFilename));
          } catch {
            // Ignore cleanup errors for old files
          }
        }
      }

      return {
        content: [
          {
            type: 'text',
            text: `✅ Successfully updated component ${namespace}:\n${[...updates.keys()].map(type => `  - Updated ${type}`).join('\n')}`,
          },
        ],
      };

    } catch (writeError) {
      // Rollback: restore old files and index
      fs.writeFileSync(indexPath, JSON.stringify(index, null, 2));

      // Delete any new files we created
      for (const filename of writtenFiles) {
        try {
          fs.unlinkSync(path.join(specDir, filename));
        } catch {
          // Ignore cleanup errors
        }
      }

      throw writeError;
    }
  } catch (error) {
    return {
      content: [
        {
          type: 'text',
          text: `Update error: ${error instanceof Error ? error.message : 'Unknown error'}`,
        },
      ],
    };
  }
}

function formatErrors(namespace: string, errors: ValidationError[]) {
  let errorText = `❌ Failed to update component ${namespace}:\n\n`;
  for (const error of errors) {
    errorText += `- ${error.file}: ${error.message}`;
    if (error.path) {
      errorText += ` (at ${error.path})`;
    }
    errorText += '\n';
  }

  return {
    content: [
      {
        type: 'text',
        text: errorText,
      },
    ],
  };
}