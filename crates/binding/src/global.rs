use std::marker::PhantomData;

/// One of the game's global variables.
pub struct Global<T: 'static> {
    ptr: *mut T,
    marker: PhantomData<*mut T>,
}

// This is only a declaration; the pointer is formed on access.
unsafe impl<T> Sync for Global<T> {}
unsafe impl<T> Send for Global<T> {}

impl<T> Global<T> {
    /// A global at `ptr`, which [`globals!`](crate::globals) takes from the linked game code.
    pub const fn linked(ptr: *mut T) -> Self {
        Self {
            ptr,
            marker: PhantomData,
        }
    }

    pub fn ptr(&self) -> *mut T {
        self.ptr
    }

    /// # Safety
    /// `T` must be the real type at this address.
    pub unsafe fn get(&self) -> T
    where
        T: Copy,
    {
        unsafe { self.ptr.read() }
    }

    /// # Safety
    /// As [`Global::get`].
    pub unsafe fn set(&self, value: T) {
        unsafe { self.ptr.write(value) }
    }

    /// # Safety
    /// As [`Global::get`].
    pub unsafe fn as_ref(&self) -> Option<&'static T> {
        unsafe { self.ptr.as_ref() }
    }

    /// # Safety
    /// As [`Global::as_ref`], and the usual aliasing rules: don't hold two of these at once.
    pub unsafe fn as_mut(&self) -> Option<&'static mut T> {
        unsafe { self.ptr.as_mut() }
    }
}
