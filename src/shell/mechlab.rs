use std::{
    ffi::{CStr, CString, c_char, c_void},
    fs,
};

use binding::{game_fns, globals, macros::hook, patches};

use super::MODULE;

game_fns!(
    /// Loads a named file out of MW2.PRJ.
    static LOAD_FILE_FROM_PRJ: unsafe extern "thiscall" fn(*mut c_void, *const c_char, i32) -> i32 =
        0x0002e346;
);

globals!(
    static G_MECH_VARIANT_FILENAMES: [[c_char; 13]; 200] = 0x00079d80;
    static G_PRJ_OBJECT: c_void = 0x00071230;
);

/// Loads the list of mech variants from the MW2.PRJ file and any user variants from the filesystem.
/// Patched to avoid a bug where the game was using an older Win32 API
#[hook(rva = 0x0000c8b8)]
unsafe extern "cdecl" fn load_mech_variant_list(mech_type: *const c_char) {
    unsafe {
        const SLOT_LEN: usize = 13;

        let write_slot = |dst: *mut c_char, name: &[u8]| {
            debug_assert!(name.len() < SLOT_LEN);
            let dst = dst.cast::<u8>();
            std::ptr::write_bytes(dst, 0, SLOT_LEN);
            std::ptr::copy_nonoverlapping(name.as_ptr(), dst, name.len());
        };

        // Create "MEK" folder if it doesn't exist
        if let Err(e) = fs::create_dir_all("MEK") {
            tracing::warn!("load_mech_variant_list: cannot create MEK dir: {e}");
        }

        let Ok(mech_type) = CStr::from_ptr(mech_type).to_str() else {
            tracing::warn!("load_mech_variant_list: non-UTF8 mech_type");
            return;
        };

        // Clear the list
        let filenames = &mut *(G_MECH_VARIANT_FILENAMES.ptr());
        filenames.fill([0; SLOT_LEN as _]);

        // Make sure we have at least the default variant
        write_slot(
            filenames[0].as_mut_ptr(),
            format!("{mech_type}00std").as_bytes(),
        );

        // Load the built-in mech variants from the MW2.PRJ file into the next 99 indices
        for (i, slot) in filenames[1..100].iter_mut().enumerate() {
            let variant = format!("{mech_type}{:02}std", i + 1);
            let c_variant = CString::new(variant.as_str()).expect("no interior NUL");

            let result = (LOAD_FILE_FROM_PRJ.get())(G_PRJ_OBJECT.ptr(), c_variant.as_ptr(), 6);

            if result > -1 {
                write_slot(slot.as_mut_ptr(), variant.as_bytes());
            }
        }

        // Find all user-defined mech variants from the filesystem and load their names into index 100 and higher
        let Ok(files) = fs::read_dir("MEK") else {
            return;
        };

        for file in files.flatten() {
            let Some(name) = file.file_name().to_str().map(str::to_owned) else {
                continue;
            };

            let is_match = name.len() == 12
                && name[..3].eq_ignore_ascii_case(mech_type)
                && name[3..5].bytes().all(|b| b.is_ascii_digit())
                && name[5..].eq_ignore_ascii_case("usr.mek");
            if !is_match {
                continue;
            }

            let n: usize = name[3..5].parse().expect("two ASCII digits");
            let i = 100 + n;
            if let Some(slot) = filenames.get_mut(i) {
                write_slot(slot.as_mut_ptr(), &name.as_bytes()[..8]);
            }
        }
    }
}

patches!(
    pub(super) static PATCHES = [
        hook load_mech_variant_list,
    ];
);
