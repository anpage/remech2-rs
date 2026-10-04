use std::ffi::{CStr, c_char};

use binding::globals;
use mw2_sys::sim::{self, GameWindowGeometry, Point};

globals!(
    static G_GAME_WINDOW_WIDTH: i32 = sim::g_gameWindowWidth;
    static G_GAME_WINDOW_HEIGHT: i32 = sim::g_gameWindowHeight;
    static G_ART_RESOLUTION: i32 = sim::g_artResolution;
    static G_ART_RESOLUTION_SIZES: [Point; 3] = sim::g_artResolutionSizes;
);

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
        G_GAME_WINDOW_WIDTH.set(width);
        G_GAME_WINDOW_HEIGHT.set(height);
    }
}

/// Picks the art resolution closest to the window's size.
#[unsafe(export_name = "ChooseArtResolution")]
pub unsafe extern "C" fn choose_art_resolution(geometry: *mut GameWindowGeometry) {
    let Some(geometry) = (unsafe { geometry.as_ref() }) else {
        return;
    };
    let Some(sizes) = (unsafe { G_ART_RESOLUTION_SIZES.as_ref() }) else {
        return;
    };
    let distance = |size: &Point| {
        (size.m_x - (geometry.m_width - 1)).abs() + (size.m_y - (geometry.m_height - 1)).abs()
    };
    if let Some(closest) = (0..sizes.len()).min_by_key(|&i| distance(&sizes[i])) {
        unsafe { G_ART_RESOLUTION.set(closest as i32) };
    }
}
