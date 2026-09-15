use binding::{patches, thunks};

use crate::common::fake_heap_free;

use super::MODULE;

thunks! {
    static WIN32_THUNKS = [
        0x001834d0 => fake_heap_free,
    ];
}

patches!(
    pub(super) static PATCHES = [
        patch WIN32_THUNKS,
    ];
);
