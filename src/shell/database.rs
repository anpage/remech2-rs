use std::ffi::c_void;

use binding::{globals, macros::hook, patches};

use super::MODULE;

globals!(
    // static G_DATABASE_MW2: *mut c_void = 0x0007122c;
);

/// Called by the game to load a file from DATABASE.MW2 and LZ decompress it.
///
/// Hooking it for now to allow for reimplementation later.
#[hook(rva = 0x0004813f)]
unsafe extern "fastcall" fn get_db_item_lz(
    db: *mut c_void,
    unused: *mut c_void,
    index: i32,
    midi_data: *mut *mut u8,
    midi_data_size: *mut usize,
) -> i32 {
    unsafe { original(db, unused, index, midi_data, midi_data_size) }
}

patches!(
    pub(super) static PATCHES = [
        hook get_db_item_lz,
    ];
);
