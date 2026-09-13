use anyhow::{Result, bail};

use super::{module::ModuleBase, patch::Patch};

/// An import thunk in the game's data that we overwrite with our own function.
pub struct Thunk {
    module: &'static ModuleBase,
    rva: usize,
    name: &'static str,
    replacement: *const (),
}

unsafe impl Sync for Thunk {}
unsafe impl Send for Thunk {}

impl Thunk {
    pub const fn new(
        module: &'static ModuleBase,
        rva: usize,
        name: &'static str,
        replacement: *const (),
    ) -> Self {
        Self {
            module,
            rva,
            name,
            replacement,
        }
    }
}

impl Patch for Thunk {
    fn name(&self) -> &'static str {
        self.name
    }

    unsafe fn apply(&self) -> Result<()> {
        let slot = self.module.resolve(self.rva).cast::<usize>();
        if slot.is_null() {
            bail!("{} is not loaded", self.module.name());
        }
        unsafe { slot.write(self.replacement as usize) };
        Ok(())
    }

    /// Nothing to do: the module is freed wholesale and nothing reads these slots between our overwrite and the unload.
    fn revert(&self) {}
}

/// A blob of bytes we overwrite in the game's data.
pub struct DataPatch {
    module: &'static ModuleBase,
    rva: usize,
    name: &'static str,
    bytes: &'static [u8],
}

impl DataPatch {
    pub const fn new(
        module: &'static ModuleBase,
        rva: usize,
        name: &'static str,
        bytes: &'static [u8],
    ) -> Self {
        Self {
            module,
            rva,
            name,
            bytes,
        }
    }
}

impl Patch for DataPatch {
    fn name(&self) -> &'static str {
        self.name
    }

    unsafe fn apply(&self) -> Result<()> {
        let dst = self.module.resolve(self.rva).cast::<u8>();
        if dst.is_null() {
            bail!("{} is not loaded", self.module.name());
        }
        unsafe { std::ptr::copy_nonoverlapping(self.bytes.as_ptr(), dst, self.bytes.len()) };
        Ok(())
    }

    /// As [`Thunk::revert`].
    fn revert(&self) {}
}
