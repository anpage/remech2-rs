use std::ffi::{CStr, CString, c_char};
use std::ptr;

use remech2_sys::shell::{self, ScreenField, TextGlyph};

use crate::resolution::Resolution;
use crate::settings;

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
    let driver = CStr::from_bytes_until_nul(name.map(|c| c as u8).as_slice())
        .map(|name| name.to_string_lossy().into_owned())
        .unwrap_or_default();
    let (width, height) = Resolution::from_driver(&driver).frame_size();
    let Ok(label) = CString::new(format!("~{width}x{height}")) else {
        return ptr::null_mut();
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

#[unsafe(export_name = "DrawWidescreenOption")]
pub unsafe extern "C" fn draw_widescreen_option(row: *mut ScreenField) -> *mut TextGlyph {
    let Some(row) = (unsafe { row.as_ref() }) else {
        return ptr::null_mut();
    };
    let label = if settings::get().video.widescreen {
        c"~ON"
    } else {
        c"~OFF"
    };
    unsafe {
        shell::Font_AddText(
            shell::g_titleFont,
            row.m_left + row.m_width / 2,
            row.m_top,
            label.as_ptr().cast_mut(),
            ptr::null_mut(),
        )
    }
}

#[unsafe(export_name = "ToggleWidescreen")]
pub unsafe extern "C" fn toggle_widescreen(_row: *mut ScreenField) {
    settings::update(|settings| settings.video.widescreen = !settings.video.widescreen);
}

#[unsafe(export_name = "DrawOptionLabel")]
pub unsafe extern "C" fn draw_option_label(label: *mut ScreenField) -> *mut TextGlyph {
    let Some(label) = (unsafe { label.as_ref() }) else {
        return ptr::null_mut();
    };
    let text = label.m_data.cast::<c_char>();
    if text.is_null() {
        return ptr::null_mut();
    }
    unsafe {
        shell::VideoDriver_RestoreBackground(
            shell::g_videoDriver,
            label.m_left,
            label.m_top,
            label.m_width,
            label.m_height,
        );
        let width = shell::Font_GetTextWidth(shell::g_titleFont, text);
        shell::Font_AddText(
            shell::g_titleFont,
            label.m_left + label.m_width - width,
            label.m_top,
            text,
            ptr::null_mut(),
        )
    }
}
