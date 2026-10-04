use std::ffi::{CStr, c_char};
use std::ptr;

use mw2_sys::shell::{self, ScreenField, TextGlyph};

const DRIVER_NAME_SIZE: usize = 15;

/// The video mode a row's value names, as its fifth character.
const VESA_640: c_char = b'4' as c_char;
const VESA_1024: c_char = b'7' as c_char;

/// The resolution row's video driver name
unsafe fn driver_name<'a>(row: *mut ScreenField) -> Option<&'a mut [c_char; DRIVER_NAME_SIZE]> {
    unsafe {
        row.as_ref()?
            .m_data
            .cast::<[c_char; DRIVER_NAME_SIZE]>()
            .as_mut()
    }
}

#[unsafe(export_name = "DrawResolutionOption")]
pub unsafe extern "C" fn draw_resolution_option(row: *mut ScreenField) -> *mut TextGlyph {
    let Some(name) = (unsafe { driver_name(row) }) else {
        return ptr::null_mut();
    };
    let label = match name[4] {
        VESA_640 => c"~640x480",
        VESA_1024 => c"~1024x768",
        _ => c"~320x240",
    };

    unsafe {
        let row = &*row;
        shell::Font_AddText(
            shell::g_titleFont,
            row.m_left + row.m_width / 2,
            row.m_top,
            label.as_ptr().cast_mut(),
            ptr::null_mut(),
        )
    }
}

/// Cycles the resolution row
#[unsafe(export_name = "ToggleVesaDriver")]
pub unsafe extern "C" fn toggle_vesa_driver(row: *mut ScreenField) {
    let Some(name) = (unsafe { driver_name(row) }) else {
        return;
    };
    let next: &CStr = match name[4] {
        VESA_640 => c"vesa768.dll",
        VESA_1024 => c"",
        _ => c"vesa480.dll",
    };

    // As strncpy: the rest of the buffer is zeroed
    name.fill(0);
    for (dst, &src) in name.iter_mut().zip(next.to_bytes()) {
        *dst = src as c_char;
    }
}
