use std::ffi::{CStr, c_char};

use mw2_sys::sim::{self, GameWindowGeometry, g_artResolutionSizes};

/// The game decides which resolution to use based on the DLL name passed to this function.
/// This is presumably a leftover from the DOS version of the game, possibly to preserve config file compatibility.
#[unsafe(export_name = "SetGameResolution")]
pub unsafe extern "C" fn set_game_resolution(driver: *mut c_char) {
    let driver = if driver.is_null() {
        String::new()
    } else {
        unsafe { CStr::from_ptr(driver) }
            .to_string_lossy()
            .to_ascii_uppercase()
    };
    let (width, height) = match driver.as_str() {
        "VESA480.DLL" => (640, 480),
        "VESA768.DLL" => (1024, 768),
        // "MCGA.DLL"
        _ => (320, 240),
    };
    unsafe {
        sim::g_gameWindowWidth = width;
        sim::g_gameWindowHeight = height;
    }
}

/// Picks the art resolution closest to the window's size.
#[unsafe(export_name = "ChooseArtResolution")]
pub unsafe extern "C" fn choose_art_resolution(geometry: *mut GameWindowGeometry) {
    let Some(geometry) = (unsafe { geometry.as_ref() }) else {
        return;
    };
    let sizes = &raw const g_artResolutionSizes;
    let distance = |i: usize| {
        let (x, y) = unsafe { ((*sizes)[i].m_x, (*sizes)[i].m_y) };
        (x - (geometry.m_width - 1)).abs() + (y - (geometry.m_height - 1)).abs()
    };
    if let Some(closest) = (0..3).min_by_key(|&i| distance(i)) {
        unsafe { sim::g_artResolution = closest as i32 };
    }
}
