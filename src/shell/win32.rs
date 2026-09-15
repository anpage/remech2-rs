use binding::{patches, raw_detour, thunks};

use crate::common::{debug_log, fake_heap_free, fake_set_menu};

use super::MODULE;

thunks! {
    static WIN32_THUNKS = [
        0x0009952c => fake_heap_free,
        0x000995bc => fake_set_menu,
    ];
}

raw_detour! {
    /// The shell's printf-style logger. Variadic, so it can't be a `#[hook]`.
    static DEBUG_LOG at 0x00017982 => debug_log;
}

patches!(
    pub(super) static PATCHES = [
        patch WIN32_THUNKS,
        patch DEBUG_LOG,
    ];
);
