#[allow(unused_imports)]
use anyhow::{Context as _, Result};

/// Something we do to the game's memory when its DLL loads.
pub trait Patch: Sync {
    fn name(&self) -> &'static str;

    /// # Safety
    /// The target module must be loaded.
    unsafe fn apply(&self) -> Result<()>;

    fn revert(&self);
}

/// A group of same-typed patches applied and reverted as one unit.
pub struct Table<T: Patch + 'static> {
    name: &'static str,
    items: &'static [T],
}

impl<T: Patch> Table<T> {
    pub const fn new(name: &'static str, items: &'static [T]) -> Self {
        Self { name, items }
    }
}

impl<T: Patch> Patch for Table<T> {
    fn name(&self) -> &'static str {
        self.name
    }

    unsafe fn apply(&self) -> Result<()> {
        for (i, item) in self.items.iter().enumerate() {
            if let Err(e) = unsafe { item.apply() } {
                for done in self.items[..i].iter().rev() {
                    done.revert();
                }
                return Err(e.context(format!("applying {}", item.name())));
            }
        }
        Ok(())
    }

    fn revert(&self) {
        for item in self.items.iter().rev() {
            item.revert();
        }
    }
}

/// Applies every patch, undoing the ones that took if any of them fails.
///
/// # Safety
/// Every module the patches target must be loaded.
pub unsafe fn apply_groups(groups: &[&[&'static dyn Patch]]) -> Result<()> {
    let mut applied: Vec<&'static dyn Patch> = Vec::new();

    for patch in groups.iter().flat_map(|group| group.iter()) {
        match unsafe { patch.apply() } {
            Ok(()) => applied.push(*patch),
            Err(e) => {
                for done in applied.iter().rev() {
                    done.revert();
                }
                return Err(e.context(format!("applying {}", patch.name())));
            }
        }
    }

    Ok(())
}

pub fn revert_groups(groups: &[&[&'static dyn Patch]]) {
    for patch in groups.iter().rev().flat_map(|group| group.iter().rev()) {
        patch.revert();
    }
}
