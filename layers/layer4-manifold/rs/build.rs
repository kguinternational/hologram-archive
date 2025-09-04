//! Build script for Atlas Manifold Layer 4
//!
//! Configures linking with Layer 2 (Conservation) and Layer 3 (Resonance) libraries
//! and sets up necessary system library dependencies.

use std::env;
use std::path::PathBuf;

fn main() {
    println!("cargo:rerun-if-changed=build.rs");
    println!("cargo:rerun-if-env-changed=ATLAS_LAYER2_PATH");
    println!("cargo:rerun-if-env-changed=ATLAS_LAYER3_PATH");

    // Get library paths from environment or use defaults
    let layer2_path =
        env::var("ATLAS_LAYER2_PATH").unwrap_or_else(|_| "../../layer2-conservation".to_string());
    let layer3_path =
        env::var("ATLAS_LAYER3_PATH").unwrap_or_else(|_| "../../layer3-resonance".to_string());

    // Convert to absolute paths
    let _layer2_lib_path = PathBuf::from(&layer2_path).join("lib");
    let _layer3_build_path = PathBuf::from(&layer3_path).join("build");

    // Add library search paths - use main lib directory for all Atlas libraries
    let manifest_dir = PathBuf::from(env::var("CARGO_MANIFEST_DIR").unwrap());
    let hologram_root = manifest_dir
        .parent() // rs
        .unwrap()
        .parent() // layer4-manifold
        .unwrap()
        .parent() // layers
        .unwrap()
        .to_path_buf();

    let main_lib_path = hologram_root.join("lib");

    if main_lib_path.exists() {
        println!("cargo:rustc-link-search=native={}", main_lib_path.display());
        // Link Atlas libraries in dependency order (most fundamental first)
        println!("cargo:rustc-link-lib=static=atlas-core");
        println!("cargo:rustc-link-lib=static=atlas-boundary");
        println!("cargo:rustc-link-lib=static=atlas-conservation");
        println!("cargo:rustc-link-lib=static=atlas-resonance");
    } else {
        println!(
            "cargo:warning=Main library path not found: {}",
            main_lib_path.display()
        );
    }

    // Link system libraries
    println!("cargo:rustc-link-lib=dylib=c");
    println!("cargo:rustc-link-lib=dylib=m");

    // Set up include paths for bindgen if needed in the future
    let layer2_include_path = PathBuf::from(&layer2_path).join("include");
    let layer3_include_path = PathBuf::from(&layer3_path).join("include");

    if layer2_include_path.exists() {
        println!("cargo:include={}", layer2_include_path.display());
    }
    if layer3_include_path.exists() {
        println!("cargo:include={}", layer3_include_path.display());
    }
}
