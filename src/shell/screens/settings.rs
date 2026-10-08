use std::ffi::{CString, c_char};
use std::ptr;

use remech2_sys::shell::{self, ScreenField, TextGlyph};

use crate::settings;

#[unsafe(export_name = "DrawResolutionOption")]
pub unsafe extern "C" fn draw_resolution_option(row: *mut ScreenField) -> *mut TextGlyph {
    let Some(row) = (unsafe { row.as_ref() }) else {
        return ptr::null_mut();
    };

    let (width, height) = settings::get().video.render_resolution.frame_size();

    let Ok(label) = CString::new(format!("~{width}x{height}")) else {
        return ptr::null_mut();
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

/// Cycles the resolution row
#[unsafe(export_name = "ToggleRenderResolution")]
pub unsafe extern "C" fn toggle_render_resolution(_row: *mut ScreenField) {
    settings::update(|settings| {
        settings.video.render_resolution = settings.video.render_resolution.next();
    });
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
