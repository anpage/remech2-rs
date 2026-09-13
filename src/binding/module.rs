use std::{
    ffi::c_void,
    sync::atomic::{AtomicUsize, Ordering},
};

/// The load address of one of the game's DLLs.
///
/// Everything addressed by RVA (globals, hooks, thunks) resolves through one of
/// these, so a binding can be declared anywhere in the tree without being handed a
/// base address at init time.
pub struct ModuleBase {
    name: &'static str,
    address: AtomicUsize,
}

impl ModuleBase {
    pub const fn new(name: &'static str) -> Self {
        Self {
            name,
            address: AtomicUsize::new(0),
        }
    }

    pub fn name(&self) -> &'static str {
        self.name
    }

    pub fn set(&self, address: usize) {
        self.address.store(address, Ordering::Release);
    }

    pub fn clear(&self) {
        self.address.store(0, Ordering::Release);
    }

    pub fn is_loaded(&self) -> bool {
        self.address.load(Ordering::Acquire) != 0
    }

    /// The address `rva` bytes into the module, or null if it isn't loaded.
    pub fn resolve(&self, rva: usize) -> *mut c_void {
        match self.address.load(Ordering::Acquire) {
            0 => std::ptr::null_mut(),
            base => (base + rva) as *mut c_void,
        }
    }
}
