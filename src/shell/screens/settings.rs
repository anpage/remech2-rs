use std::ffi::{c_char, c_void};

use binding::{macros::hook, patches};

use super::{Clickable, G_SHELL_BUTTON1, SHELL_LABEL_NEW};
use crate::shell::MODULE;

/// The video mode a row's `value` buffer names, as its fifth character.
const VESA_640: c_char = b'4' as c_char;
const VESA_1024: c_char = b'7' as c_char;

/// Builds the resolution row's label. One of the `build` callbacks in the screen's
/// [`Clickable`] group at `0x10070da8`.
#[hook(rva = 0x000435e9)]
unsafe extern "cdecl" fn resolution_label(row: *mut Clickable) -> *mut *mut c_void {
    unsafe {
        let label = match (*(*row).buffer::<[c_char; 15]>())[4] {
            VESA_640 => c"~640x480",
            VESA_1024 => c"~1024x768",
            _ => c"~320x200",
        };

        (SHELL_LABEL_NEW.get())(
            G_SHELL_BUTTON1.get(),
            (*row).left + (*row).width / 2,
            (*row).top,
            label.as_ptr(),
            0,
        )
    }
}

/// Cycles the resolution row. The screen's `on_click` for that row.
#[hook(rva = 0x00043703)]
unsafe extern "cdecl" fn resolution_toggle(row: *mut Clickable) {
    unsafe {
        let value = (*row).buffer::<[c_char; 15]>();
        match (*value)[4] {
            VESA_640 => {
                std::ptr::copy_nonoverlapping(c"vesa768.dll".as_ptr(), (*value).as_mut_ptr(), 12)
            }
            VESA_1024 => (*value).fill(0),
            _ => std::ptr::copy_nonoverlapping(c"vesa480.dll".as_ptr(), (*value).as_mut_ptr(), 12),
        }
    }
}

patches!(
    pub(in crate::shell) static PATCHES = [
        hook resolution_label,
        hook resolution_toggle,
    ];
);
