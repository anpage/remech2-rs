use windows::{
    Win32::{
        Foundation::{FALSE, HWND, TRUE},
        UI::WindowsAndMessaging::{
            DispatchMessageA, MSG, PM_REMOVE, PeekMessageA, TranslateMessage, WM_QUIT, WaitMessage,
        },
    },
    core::BOOL,
};

use binding::macros::{globals, hook, patches};

use crate::{
    settings::SETTINGS,
    sim::{G_CURRENT_DRAW_MODE, G_SHOULD_QUIT, RenderTarget},
};

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
    pub(crate) static G_WINDOW_ACTIVE: BOOL = 0x000acb74;
    static G_STRETCH_BLIT_SOURCE_RECT: RenderTarget = 0x00176ed0;
    static G_STRETCH_BLIT_OTHER_SOURCE_RECT: RenderTarget = 0x000bdff8;
    static G_BLIT_GLOBAL_1: BOOL = 0x00176ebc;
    static G_BLIT_GLOBAL_2: u32 = 0x000a5f18;
    static G_BLIT_GLOBAL_3: u32 = 0x000a5a24;
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

/// This function is called every frame to draw the game.
#[hook(rva = 0x00012e15)]
unsafe extern "stdcall" fn blit() {
    unsafe {
        if G_BLIT_GLOBAL_1.get() == FALSE {
            if G_WINDOW_ACTIVE.get() == TRUE {
                ((*G_CURRENT_DRAW_MODE.get()).blit_flip_func)();
            }
        } else {
            ((*G_CURRENT_DRAW_MODE.get()).stretch_blit_func)(
                (*G_STRETCH_BLIT_SOURCE_RECT.ptr()).left + 1,
                (*G_STRETCH_BLIT_SOURCE_RECT.ptr()).top + 1,
                (*G_STRETCH_BLIT_SOURCE_RECT.ptr()).right,
                (*G_STRETCH_BLIT_SOURCE_RECT.ptr()).bottom,
            );

            G_STRETCH_BLIT_SOURCE_RECT
                .set(G_STRETCH_BLIT_OTHER_SOURCE_RECT.as_ref().unwrap().clone());

            G_BLIT_GLOBAL_2.set(G_BLIT_GLOBAL_3.get());
            G_BLIT_GLOBAL_1.set(FALSE);
        }
    }
}

/// The original function had a loop that was causing bad stuttering when the mouse was moved.
#[hook(rva = 0x00067bbc)]
unsafe extern "stdcall" fn handle_messages() {
    unsafe {
        if G_WINDOW_ACTIVE.get() == FALSE {
            let _ = WaitMessage();
        }

        if G_SHOULD_QUIT.get() == FALSE {
            let mut msg: MSG = MSG::default();

            if PeekMessageA(&mut msg as *mut MSG, Some(HWND::default()), 0, 0, PM_REMOVE).into() {
                if msg.hwnd == HWND::default() || msg.message != WM_QUIT {
                    let _ = TranslateMessage(&msg);
                    DispatchMessageA(&msg);
                } else {
                    G_SHOULD_QUIT.set(TRUE);
                }
            }
        }
    }
}

#[hook(rva = 0x00077392)]
unsafe extern "stdcall" fn toggle_fullscreen() {
    // Do nothing because we handle this in the custom window proc
}

patches!(
    pub(super) static PATCHES = [
        hook set_game_resolution,
        hook init_game_window_geometry,
        hook blit,
        hook handle_messages,
        hook toggle_fullscreen,
    ];
);
