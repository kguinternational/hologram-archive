import * as path from 'path';
import { submitArtifactOperation } from './artifact.js';
import { submitManifestOperation } from './manifest.js';
import { ComponentFiles } from '../types.js';

export async function createOperation(
  namespace: string,
  files: ComponentFiles,
  specDir: string = path.join(process.cwd(), 'spec')
): Promise<{ content: Array<{ type: string; text: string }> }> {
  // Use the artifact/manifest pattern for atomic creation
  const artifacts: Record<string, string> = {};
  const errors: string[] = [];

  try {
    // Phase 1: Submit spec artifact
    if (files.spec) {
      const specResult = await submitArtifactOperation(files.spec, 'spec');
      const specText = specResult.content[0].text;

      try {
        const specResponse = JSON.parse(specText);
        if (specResponse.success) {
          artifacts.spec = specResponse.cid;
        }
      } catch {
        // Not JSON, must be an error message
        errors.push(`Spec validation failed: ${specText}`);
      }
    } else {
      errors.push('Missing spec file');
    }

    // Phase 2: Implementation no longer required

    // Phase 3: Submit conformance artifacts
    const conformanceTypes = ['interface', 'docs', 'test', 'manager', 'dependency', 'build', 'log'];

    for (const type of conformanceTypes) {
      if (files[type]) {
        const conformanceResult = await submitArtifactOperation(files[type], 'conformance');
        const conformanceText = conformanceResult.content[0].text;

        try {
          const conformanceResponse = JSON.parse(conformanceText);
          if (conformanceResponse.success) {
            artifacts[type] = conformanceResponse.cid;
          }
        } catch {
          // Not JSON, must be an error message
          errors.push(`${type} conformance validation failed: ${conformanceText}`);
        }
      } else {
        errors.push(`Missing ${type} conformance file`);
      }
    }

    // Phase 4: Check for errors before submitting manifest
    if (errors.length > 0) {
      return {
        content: [
          {
            type: 'text',
            text: `❌ Failed to validate artifacts:\n${errors.join('\n')}`,
          },
        ],
      };
    }

    // Phase 5: Submit manifest to create component
    const manifestResult = await submitManifestOperation(namespace, artifacts, specDir);
    return manifestResult;

  } catch (error) {
    return {
      content: [
        {
          type: 'text',
          text: `Create error: ${error instanceof Error ? error.message : 'Unknown error'}`,
        },
      ],
    };
  }
}