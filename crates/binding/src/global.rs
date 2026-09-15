use std::marker::PhantomData;

use super::module::ModuleBase;

/// One of the game's global variables, addressed by its RVA.
pub struct Global<T: 'static> {
    module: &'static ModuleBase,
    rva: usize,
    marker: PhantomData<*mut T>,
}

// This is only a declaration; the pointer is formed on access.
unsafe impl<T> Sync for Global<T> {}
unsafe impl<T> Send for Global<T> {}

impl<T> Global<T> {
    pub const fn new(module: &'static ModuleBase, rva: usize) -> Self {
        Self {
            module,
            rva,
            marker: PhantomData,
        }
    }

    /// Null until the module is loaded.
    pub fn ptr(&self) -> *mut T {
        self.module.resolve(self.rva).cast()
    }

    /// # Safety
    /// The module must be loaded, and `T` must be the real type at this address.
    pub unsafe fn get(&self) -> T
    where
        T: Copy,
    {
        unsafe { self.ptr().read() }
    }

    /// # Safety
    /// As [`Global::get`].
    pub unsafe fn set(&self, value: T) {
        unsafe { self.ptr().write(value) }
    }

    /// # Safety
    /// As [`Global::get`], minus the load check.
    /// Returns `None` when the module isn't loaded.
    pub unsafe fn as_ref(&self) -> Option<&'static T> {
        unsafe { self.ptr().as_ref() }
    }

    /// # Safety
    /// As [`Global::as_ref`], and the usual aliasing rules: don't hold two of these at once.
    pub unsafe fn as_mut(&self) -> Option<&'static mut T> {
        unsafe { self.ptr().as_mut() }
    }
}
