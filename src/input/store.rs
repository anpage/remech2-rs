use std::{
    fs,
    path::{Path, PathBuf},
};

use anyhow::{Context as _, Result, bail};

use super::{defaults, profile::Profile};
use crate::settings::SETTINGS;

const INPUT_DIR: &str = "input";
const EXTENSION: &str = "toml";

pub const DEFAULT_PROFILE: &str = "Default";

/// Whether `name` is the built-in profile
pub fn is_default(name: &str) -> bool {
    name.eq_ignore_ascii_case(DEFAULT_PROFILE)
}

pub fn valid_name(name: &str) -> bool {
    let reserved = ["CON", "PRN", "AUX", "NUL"];
    let device_name = |stem: &str| {
        let upper = stem.to_ascii_uppercase();
        reserved.contains(&upper.as_str())
            || ((upper.starts_with("COM") || upper.starts_with("LPT"))
                && upper.len() == 4
                && upper.as_bytes()[3].is_ascii_digit())
    };
    !name.trim().is_empty()
        && name.len() <= 64
        && name.trim() == name
        && !name.starts_with('.')
        && !name.ends_with('.')
        && !name
            .chars()
            .any(|c| c.is_control() || r#"<>:"/\|?*"#.contains(c))
        && !device_name(name)
}

fn file_in(dir: &Path, name: &str) -> Result<PathBuf> {
    if !valid_name(name) {
        bail!("{name:?} can't be used as a name");
    }

    Ok(dir.join(format!("{name}.{EXTENSION}")))
}

fn names_in(dir: &Path) -> Vec<String> {
    let Ok(entries) = fs::read_dir(dir) else {
        return Vec::new();
    };
    let mut names: Vec<String> = entries
        .filter_map(|entry| {
            let path = entry.ok()?.path();
            if path.extension()? != EXTENSION {
                return None;
            }
            Some(path.file_stem()?.to_str()?.to_owned())
        })
        .filter(|name| valid_name(name))
        .collect();
    names.sort_by_key(|name| name.to_lowercase());
    names
}

fn write_atomically(path: &Path, text: &str) -> Result<()> {
    if let Some(dir) = path.parent() {
        fs::create_dir_all(dir).with_context(|| format!("Couldn't create {}", dir.display()))?;
    }
    let mut temporary = path.as_os_str().to_owned();
    temporary.push(".tmp");
    let temporary = PathBuf::from(temporary);
    fs::write(&temporary, text)
        .with_context(|| format!("Couldn't write {}", temporary.display()))?;
    fs::rename(&temporary, path).with_context(|| format!("Couldn't replace {}", path.display()))
}

pub struct Store {
    root: PathBuf,
}

impl Default for Store {
    fn default() -> Self {
        Self::new(INPUT_DIR)
    }
}

impl Store {
    pub fn new(root: impl Into<PathBuf>) -> Self {
        Self { root: root.into() }
    }

    pub fn root(&self) -> &Path {
        &self.root
    }

    pub fn profiles(&self) -> Vec<String> {
        let mut names = names_in(&self.root);
        names.retain(|name| !is_default(name));
        names.insert(0, DEFAULT_PROFILE.to_owned());
        names
    }

    pub fn unused_name(&self, base: &str) -> String {
        (1..)
            .map(|n| match n {
                1 => base.to_owned(),
                n => format!("{base} {n}"),
            })
            .find(|name| !self.has_profile(name))
            .unwrap()
    }

    pub fn load_profile(&self, name: &str) -> Result<(Profile, Vec<String>)> {
        if is_default(name) {
            return Ok((defaults::classic(), Vec::new()));
        }
        let path = file_in(&self.root, name)?;
        let text = fs::read_to_string(&path)
            .with_context(|| format!("Couldn't read {}", path.display()))?;
        Profile::from_toml(&text).with_context(|| format!("Couldn't read {}", path.display()))
    }

    pub fn save_profile(&self, name: &str, profile: &Profile) -> Result<()> {
        if is_default(name) {
            bail!("The default controls can't be changed");
        }
        write_atomically(&file_in(&self.root, name)?, &profile.to_toml()?)
    }

    pub fn has_profile(&self, name: &str) -> bool {
        self.profiles()
            .iter()
            .any(|existing| existing.eq_ignore_ascii_case(name))
    }

    pub fn delete_profile(&self, name: &str) -> Result<()> {
        if is_default(name) {
            bail!("The default controls can't be deleted");
        }
        let path = file_in(&self.root, name)?;
        fs::remove_file(&path).with_context(|| format!("Couldn't delete {}", path.display()))
    }

    pub fn rename_profile(&self, from: &str, to: &str) -> Result<()> {
        if is_default(from) || is_default(to) {
            bail!("The default controls can't be renamed or replaced");
        }
        let source = file_in(&self.root, from)?;
        let target = file_in(&self.root, to)?;
        let case_only = from.eq_ignore_ascii_case(to);
        if self.has_profile(to) && !case_only {
            bail!("There's already a profile called {to:?}");
        }
        let rename = |from: &Path, to: &Path| {
            fs::rename(from, to)
                .with_context(|| format!("Couldn't rename {} to {}", from.display(), to.display()))
        };
        if case_only {
            // A file system that ignores case sees that as renaming the file to itself
            let mut between = target.as_os_str().to_owned();
            between.push(".renaming");
            let between = PathBuf::from(between);
            rename(&source, &between)?;
            rename(&between, &target)
        } else {
            rename(&source, &target)
        }
    }

    pub fn load_or_default(&self, name: &str) -> (String, Profile) {
        if !is_default(name) {
            match self.load_profile(name) {
                Ok((profile, warnings)) => {
                    for warning in warnings {
                        tracing::warn!("Input profile {name:?}: {warning}");
                    }
                    return (name.to_owned(), profile);
                }
                Err(e) => tracing::error!("{e:#}; using the default controls"),
            }
        }
        (DEFAULT_PROFILE.to_owned(), defaults::classic())
    }
}

pub fn active_profile_name() -> String {
    SETTINGS
        .get(Some("input"), "profile")
        .filter(|name| valid_name(name))
        .unwrap_or_else(|| DEFAULT_PROFILE.to_owned())
}

pub fn set_active_profile_name(name: &str) {
    SETTINGS.set_string("input", "profile", name);
}

pub fn load_active_profile() -> (String, Profile) {
    Store::default().load_or_default(&active_profile_name())
}
