use std::{env, path::Path, process::Command};

fn main() {
    // The version only changes when the checked-out commit does, so don't
    // re-run for every source edit.
    if let Some(git_dir) = git(&["rev-parse", "--absolute-git-dir"]) {
        for file in ["HEAD", "refs", "packed-refs"] {
            let path = Path::new(&git_dir).join(file);
            if path.exists() {
                println!("cargo::rerun-if-changed={}", path.display());
            }
        }
    }
    println!("cargo::rerun-if-env-changed=REMECH2_VERSION");
    println!("cargo::rustc-env=REMECH2_VERSION={}", version());
}

fn version() -> String {
    let package_version = env::var("CARGO_PKG_VERSION").unwrap();

    // Release builds get their version handed to them by CI, where the tag is
    // more trustworthy than whatever refs happen to have been fetched.
    if let Ok(version) = env::var("REMECH2_VERSION") {
        let version = version.trim().trim_start_matches('v');
        if !version.is_empty() {
            return version.to_owned();
        }
    }

    // On a tag this is just the version, otherwise it's the commit hash,
    // optionally prefixed with the last tag it can reach.
    match git(&["describe", "--tags", "--always", "--dirty", "--abbrev=7"]) {
        Some(described) => described.trim_start_matches('v').to_owned(),
        None => package_version,
    }
}

fn git(args: &[&str]) -> Option<String> {
    let output = Command::new("git").args(args).output().ok()?;
    if !output.status.success() {
        return None;
    }
    let stdout = String::from_utf8(output.stdout).ok()?.trim().to_owned();
    (!stdout.is_empty()).then_some(stdout)
}
