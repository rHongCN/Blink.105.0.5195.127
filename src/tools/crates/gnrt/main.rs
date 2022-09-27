// Copyright 2022 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

use gnrt_lib::*;

use crates::ThirdPartyCrate;
use manifest::*;

use std::collections::HashSet;
use std::fs;
use std::io::{self, Write};
use std::path::Path;

fn main() {
    let args = clap::Command::new("gnrt")
        .about("Generate GN build rules from third_party/rust crates")
        .arg(
            clap::Arg::new("output_cargo_toml")
                .help("Output third_party/rust/Cargo.toml then exit immediately"),
        )
        .get_matches();

    let paths = paths::ChromiumPaths::new().unwrap();

    let manifest_contents =
        String::from_utf8(fs::read(paths.third_party.join("third_party.toml")).unwrap()).unwrap();
    let third_party_manifest: ThirdPartyManifest = toml::de::from_str(&manifest_contents).unwrap();

    // Traverse our third-party directory to collect the set of vendored crates.
    // Used to generate Cargo.toml [patch] sections, and later to check against
    // `cargo metadata`'s dependency resolution to ensure we have all the crates
    // we need. We sort `crates` for a stable ordering of [patch] sections.
    let mut crates = crates::collect_third_party_crates(paths.third_party.clone()).unwrap();
    crates.sort_unstable_by(|a, b| a.0.cmp(&b.0));

    // Generate a fake Cargo.toml for dependency resolution.
    let cargo_manifest = generate_fake_cargo_toml(
        third_party_manifest,
        crates.iter().map(|(c, _)| {
            let crate_path = paths.third_party.join(c.crate_path()).canonicalize().unwrap();
            manifest::PatchSpecification {
                package_name: c.name.clone(),
                patch_name: c.patch_name(),
                path: crate_path,
            }
        }),
    );

    if args.is_present("output_cargo_toml") {
        println!("{}", toml::ser::to_string(&cargo_manifest).unwrap());
        return;
    }

    // Create a fake package: Cargo.toml and an empty main.rs. This allows cargo
    // to construct a full dependency graph as if Chrome were a cargo package.
    write!(
        io::BufWriter::new(fs::File::create(paths.third_party.join("Cargo.toml")).unwrap()),
        "# {}\n\n{}",
        AUTOGENERATED_FILE_HEADER,
        toml::to_string(&cargo_manifest).unwrap()
    )
    .unwrap();
    create_dirs_if_needed(&paths.third_party.join("src")).unwrap();
    write!(
        io::BufWriter::new(fs::File::create(paths.third_party.join("src/main.rs")).unwrap()),
        "// {}",
        AUTOGENERATED_FILE_HEADER
    )
    .unwrap();

    // Run `cargo metadata` and process the output to get a list of crates we
    // depend on.
    let mut command = cargo_metadata::MetadataCommand::new();
    command.current_dir(paths.third_party.clone());
    let dependencies = deps::collect_dependencies(&command.exec().unwrap());

    // Compare cargo's dependency resolution with the crates we have on disk.
    let present_crates: HashSet<&ThirdPartyCrate> = crates.iter().map(|(c, _)| c).collect();
    let mut unused_crates: HashSet<&ThirdPartyCrate> = present_crates.clone();
    let mut missing_deps: Vec<&deps::ThirdPartyDep> = Vec::new();
    for dep in dependencies.iter() {
        let dep_crate = ThirdPartyCrate { name: dep.package_name.clone(), epoch: dep.epoch };

        if present_crates.contains(&dep_crate) {
            assert!(unused_crates.remove(&dep_crate));
        } else {
            missing_deps.push(dep);
        }
    }

    for dep in missing_deps.iter() {
        println!("Missing dependency: {} {}", dep.package_name, dep.epoch);
        for edge in dep.dependency_path.iter() {
            println!("    {edge}");
        }
    }

    for c in unused_crates.iter() {
        println!("Crate not used: {c}");
    }

    for nonlocal_dep in dependencies.iter().filter(|dep| !dep.is_local) {
        println!(
            "Error: dependency {} {} resolved to an upstream source. The checked-in version likely \
                does not satisfy another crate's dependency version requirement. Resolved \
                version: {}",
            nonlocal_dep.package_name, nonlocal_dep.epoch, nonlocal_dep.version
        );
    }
}

fn create_dirs_if_needed(path: &Path) -> io::Result<()> {
    if path.is_dir() {
        return Ok(());
    }

    if let Some(parent) = path.parent() {
        create_dirs_if_needed(parent)?;
    }

    fs::create_dir(path)
}

/// A message prepended to autogenerated files. Notes this tool generated it and
/// not to edit directly.
static AUTOGENERATED_FILE_HEADER: &'static str = "!!! DO NOT EDIT -- Autogenerated by gnrt from third_party.toml. Edit that file instead. See tools/crates/gnrt.";
