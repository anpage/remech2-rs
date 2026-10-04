use binding::macros::{globals, hook, patches};

use crate::settings::SETTINGS;

use super::MODULE;

#[repr(C)]
pub struct GameWindowGeometry {
    pub(crate) width: i32,
    pub(crate) height: i32,
    unknown1: i32,
    unknown2: i32,
    unknown3: i32,
    unknown4: i32,
}

globals!(
    pub(crate) static G_GAME_WINDOW_WIDTH: u32 = 0x000acb6c;
    pub(crate) static G_GAME_WINDOW_HEIGHT: u32 = 0x000acb70;
    pub(crate) static G_GAME_WINDOW_GEOMETRY: *mut GameWindowGeometry = 0x00176eb4;
    pub(crate) static G_SCREEN_W_MINUS_1: i32 = 0x00176ee4;
    pub(crate) static G_SCREEN_H_MINUS_1: i32 = 0x00176ec0;
);

/// The game decides which resolution to use based on the DLL name passed to this function.
/// This is presumably a leftover from the DOS version of the game, possibly to preserve config file compatibility.
#[hook(rva = 0x00067e23)]
unsafe extern "cdecl" fn set_game_resolution(resolution: *mut std::ffi::c_char) {
    let widescreen = SETTINGS.get_bool("video", "widescreen", false);
    unsafe {
        // "MCGA.DLL"
        if widescreen {
            G_GAME_WINDOW_WIDTH.set(427);
            G_GAME_WINDOW_HEIGHT.set(240);
        } else {
            G_GAME_WINDOW_WIDTH.set(320);
            G_GAME_WINDOW_HEIGHT.set(240);
        }

        let resolution = std::ffi::CStr::from_ptr(resolution)
            .to_string_lossy()
            .to_uppercase();
        if resolution == "VESA480.DLL" {
            G_GAME_WINDOW_WIDTH.set(if widescreen { 854 } else { 640 });
            G_GAME_WINDOW_HEIGHT.set(480);
        } else if resolution == "VESA768.DLL" {
            G_GAME_WINDOW_WIDTH.set(if widescreen { 1366 } else { 1024 });
            G_GAME_WINDOW_HEIGHT.set(768);
        }
    }
}

/// Allocates GameWindowGeometry and caches the W-1/H-1 scale globals used by every HUD-scaling function.
/// Depending on the configured resolution, we force the window size to match the HUD box.
#[hook(rva = 0x00012720)]
unsafe extern "cdecl" fn init_game_window_geometry() -> i32 {
    unsafe {
        let (width, height) = match G_GAME_WINDOW_HEIGHT.get() {
            480 => (640, 480),
            768 => (1024, 768),
            _ => (320, 240),
        };

        let res = original();
        if res != 0 && !G_GAME_WINDOW_GEOMETRY.get().is_null() {
            (*(G_GAME_WINDOW_GEOMETRY).get()).width = width;
            (*(G_GAME_WINDOW_GEOMETRY).get()).height = height;
            G_SCREEN_W_MINUS_1.set(width - 1);
            G_SCREEN_H_MINUS_1.set(height - 1);
        }
        res
    }
}

patches!(
    pub(super) static PATCHES = [
        hook set_game_resolution,
        hook init_game_window_geometry,
    ];
);
