#[allow(unused_imports)]
use anyhow::{Context as _, Result};

/// Implemented by [`patches!`] for every hook it registers.
///
/// `#[hook]` emits an assertion that its own marker implements this, so a hook
/// that never reaches a `patches!` list fails to compile instead of quietly
/// never being installed. The dead-code lint can't be relied on for this: a hook
/// called from anywhere else is live whether or not it's registered.
///
/// [`patches!`]: crate::macros::patches
#[diagnostic::on_unimplemented(
    message = "`{Self}` is declared with `#[hook]` but never registered",
    label = "not in any `patches!` list",
    note = "add `hook <name>` to the `patches!` list at the bottom of this file"
)]
pub trait Registered {}

/// Implemented by [`patch_groups!`] for every module whose patches it lists.
///
/// The counterpart to [`Registered`] one level up: a `patches!` list that no
/// `patch_groups!` names is never applied, so every hook in that file compiles,
/// registers, and silently does nothing.
///
/// [`patch_groups!`]: crate::macros::patch_groups
#[diagnostic::on_unimplemented(
    message = "this module's `patches!` list is never applied",
    label = "not named by any `patch_groups!`",
    note = "add this module to the `patch_groups!` list in its parent module"
)]
pub trait Grouped {}

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
