import * as crypto from 'crypto';
import * as fs from 'fs';
import * as path from 'path';

export class ArtifactStore {
  private artifactDir: string;
  private artifacts: Map<string, any> = new Map();

  constructor(artifactDir: string = path.join(process.cwd(), '.artifacts')) {
    this.artifactDir = artifactDir;
    this.ensureArtifactDir();
  }

  /**
   * Ensure artifact directory exists
   */
  private ensureArtifactDir(): void {
    if (!fs.existsSync(this.artifactDir)) {
      fs.mkdirSync(this.artifactDir, { recursive: true });
    }
  }

  /**
   * Generate CID for content using SHA256
   */
  public generateCID(content: any): string {
    // Canonical JSON encoding (sorted keys)
    const canonical = JSON.stringify(content, Object.keys(content).sort());
    const hash = crypto.createHash('sha256');
    hash.update(canonical);
    return `cid:${hash.digest('hex')}`;
  }

  /**
   * Store artifact with CID
   */
  public storeArtifact(content: any): string {
    const cid = this.generateCID(content);
    const artifactPath = path.join(this.artifactDir, cid);

    // Store in memory
    this.artifacts.set(cid, content);

    // Store on disk
    fs.writeFileSync(artifactPath, JSON.stringify(content, null, 2));

    return cid;
  }

  /**
   * Retrieve artifact by CID
   */
  public getArtifact(cid: string): any | null {
    // Check memory first
    if (this.artifacts.has(cid)) {
      return this.artifacts.get(cid);
    }

    // Check disk
    const artifactPath = path.join(this.artifactDir, cid);
    if (fs.existsSync(artifactPath)) {
      const content = JSON.parse(fs.readFileSync(artifactPath, 'utf-8'));
      this.artifacts.set(cid, content);
      return content;
    }

    return null;
  }

  /**
   * Check if artifact exists
   */
  public hasArtifact(cid: string): boolean {
    if (this.artifacts.has(cid)) {
      return true;
    }

    const artifactPath = path.join(this.artifactDir, cid);
    return fs.existsSync(artifactPath);
  }

  /**
   * Remove artifact
   */
  public removeArtifact(cid: string): boolean {
    // Remove from memory
    this.artifacts.delete(cid);

    // Remove from disk
    const artifactPath = path.join(this.artifactDir, cid);
    if (fs.existsSync(artifactPath)) {
      fs.unlinkSync(artifactPath);
      return true;
    }

    return false;
  }

  /**
   * Clear all artifacts (garbage collection)
   */
  public clearArtifacts(): void {
    // Clear memory
    this.artifacts.clear();

    // Clear disk
    if (fs.existsSync(this.artifactDir)) {
      const files = fs.readdirSync(this.artifactDir);
      for (const file of files) {
        if (file.startsWith('cid:')) {
          fs.unlinkSync(path.join(this.artifactDir, file));
        }
      }
    }
  }

  /**
   * List all artifact CIDs
   */
  public listArtifacts(): string[] {
    const cids = new Set<string>();

    // From memory
    for (const cid of this.artifacts.keys()) {
      cids.add(cid);
    }

    // From disk
    if (fs.existsSync(this.artifactDir)) {
      const files = fs.readdirSync(this.artifactDir);
      for (const file of files) {
        if (file.startsWith('cid:')) {
          cids.add(file);
        }
      }
    }

    return Array.from(cids);
  }
}