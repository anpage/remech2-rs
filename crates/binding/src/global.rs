use std::marker::PhantomData;

use super::module::ModuleBase;

/// One of the game's global variables.
pub struct Global<T: 'static> {
    location: Location<T>,
    marker: PhantomData<*mut T>,
}

enum Location<T> {
    /// A variable of the linked game code
    Linked(*mut T),
    /// An RVA into one of the original DLLs
    Rva {
        module: &'static ModuleBase,
        rva: usize,
    },
}

// This is only a declaration; the pointer is formed on access.
unsafe impl<T> Sync for Global<T> {}
unsafe impl<T> Send for Global<T> {}

impl<T> Global<T> {
    pub const fn new(module: &'static ModuleBase, rva: usize) -> Self {
        Self {
            location: Location::Rva { module, rva },
            marker: PhantomData,
        }
    }

    /// A global at `ptr`, which [`globals!`](crate::globals) takes from the linked game code.
    pub const fn linked(ptr: *mut T) -> Self {
        Self {
            location: Location::Linked(ptr),
            marker: PhantomData,
        }
    }

    /// Null until the module is loaded, for a global addressed by RVA.
    pub fn ptr(&self) -> *mut T {
        match self.location {
            Location::Linked(ptr) => ptr,
            Location::Rva { module, rva } => module.resolve(rva).cast(),
        }
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
