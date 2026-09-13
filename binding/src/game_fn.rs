use std::marker::PhantomData;

use retour::Function;

use super::module::ModuleBase;

/// A function inside the game that we call but don't hook.
pub struct GameFn<F: Function> {
    module: &'static ModuleBase,
    rva: usize,
    marker: PhantomData<F>,
}

impl<F: Function> GameFn<F> {
    pub const fn new(module: &'static ModuleBase, rva: usize) -> Self {
        Self {
            module,
            rva,
            marker: PhantomData,
        }
    }

    /// # Safety
    /// The module must be loaded and the signature must match the real one.
    pub unsafe fn get(&self) -> F {
        unsafe { F::from_ptr(self.module.resolve(self.rva).cast_const().cast()) }
    }
}
