use std::sync::{
    Mutex,
    atomic::{AtomicUsize, Ordering},
};

use anyhow::{Result, bail};
use retour::{Function, GenericDetour, RawDetour};

use super::{module::ModuleBase, patch::Patch};

/// A detour over one of the game's functions.
pub struct Hook<F: Function> {
    module: &'static ModuleBase,
    rva: usize,
    name: &'static str,
    detour: F,
    /// Trampoline address, cached so hot paths don't take a lock. 0 = not installed.
    original: AtomicUsize,
    state: Mutex<Option<GenericDetour<F>>>,
}

impl<F: Function> Hook<F> {
    pub const fn new(
        module: &'static ModuleBase,
        rva: usize,
        name: &'static str,
        detour: F,
    ) -> Self {
        Self {
            module,
            rva,
            name,
            detour,
            original: AtomicUsize::new(0),
            state: Mutex::new(None),
        }
    }

    /// The function this hook replaced.
    ///
    /// # Safety
    /// The hook must be installed, and calling the original carries whatever
    /// requirements that function has.
    pub unsafe fn original(&self) -> F {
        let address = self.original.load(Ordering::Acquire);
        assert!(address != 0, "{} ran with its hook uninstalled", self.name);
        unsafe { F::from_ptr(address as *const ()) }
    }
}

impl<F: Function> Patch for Hook<F> {
    fn name(&self) -> &'static str {
        self.name
    }

    unsafe fn apply(&self) -> Result<()> {
        let target = self.module.resolve(self.rva);
        if target.is_null() {
            bail!("{} is not loaded", self.module.name());
        }

        let mut state = self.state.lock().unwrap();
        if state.is_some() {
            bail!("{} is already hooked", self.name);
        }

        let detour =
            unsafe { GenericDetour::new(F::from_ptr(target.cast_const().cast()), self.detour)? };
        unsafe { detour.enable()? };

        self.original
            .store(detour.trampoline() as *const () as usize, Ordering::Release);
        *state = Some(detour);
        Ok(())
    }

    fn revert(&self) {
        let mut state = self.state.lock().unwrap();
        self.original.store(0, Ordering::Release);
        drop(state.take());
    }
}

// A detour at an address that isn't a function entry point.
pub struct RawHook {
    module: &'static ModuleBase,
    rva: usize,
    name: &'static str,
    detour: *const (),
    original: AtomicUsize,
    state: Mutex<Option<RawDetour>>,
}

// The detour pointer is only read, never written.
unsafe impl Sync for RawHook {}
unsafe impl Send for RawHook {}

impl RawHook {
    pub const fn new(
        module: &'static ModuleBase,
        rva: usize,
        name: &'static str,
        detour: *const (),
    ) -> Self {
        Self {
            module,
            rva,
            name,
            detour,
            original: AtomicUsize::new(0),
            state: Mutex::new(None),
        }
    }

    /// The code this hook displaced, as a function of the caller's choosing.
    ///
    /// # Safety
    /// The hook must be installed and `F` must match what's really there.
    pub unsafe fn original<F: Function>(&self) -> F {
        let address = self.original.load(Ordering::Acquire);
        assert!(address != 0, "{} ran with its hook uninstalled", self.name);
        unsafe { F::from_ptr(address as *const ()) }
    }
}

impl Patch for RawHook {
    fn name(&self) -> &'static str {
        self.name
    }

    unsafe fn apply(&self) -> Result<()> {
        let target = self.module.resolve(self.rva);
        if target.is_null() {
            bail!("{} is not loaded", self.module.name());
        }

        let mut state = self.state.lock().unwrap();
        if state.is_some() {
            bail!("{} is already hooked", self.name);
        }

        let detour = unsafe { RawDetour::new(target.cast_const().cast(), self.detour)? };
        unsafe { detour.enable()? };

        self.original
            .store(detour.trampoline() as *const () as usize, Ordering::Release);
        *state = Some(detour);
        Ok(())
    }

    fn revert(&self) {
        let mut state = self.state.lock().unwrap();
        self.original.store(0, Ordering::Release);
        drop(state.take());
    }
}
