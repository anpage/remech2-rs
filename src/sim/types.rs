use std::ffi::c_void;

use crate::sim::{hud::hud_origin, window::G_GAME_WINDOW_GEOMETRY};

use super::{G_GAME_WINDOW_HEIGHT, G_GAME_WINDOW_WIDTH, drawmode::hooks::PixelBuffer};

/// A render context: the pixel buffer to draw into plus the rect within that buffer
/// where all the drawing happens.
#[repr(C)]
#[derive(Clone)]
pub(super) struct RenderTarget {
    pub pixel_buffer: *mut PixelBuffer,
    pub left: i32,
    pub top: i32,
    pub right: i32,
    pub bottom: i32,
}

impl RenderTarget {
    /// Widens the target rect to the whole framebuffer
    pub fn cover_framebuffer(&mut self) {
        unsafe {
            self.left = 0;
            self.top = 0;
            self.right = (G_GAME_WINDOW_WIDTH.get() as i32 - 1).max(0);
            self.bottom = (G_GAME_WINDOW_HEIGHT.get() as i32 - 1).max(0);
        }
    }

    /// Shrinks the target rect to the centered 4:3 HUD box
    pub fn cover_hud_box(&mut self) {
        unsafe {
            let geometry = G_GAME_WINDOW_GEOMETRY.get();
            if geometry.is_null() {
                return;
            }
            let (x0, y0) = hud_origin();
            self.left = x0;
            self.top = y0;
            self.right = x0 + (*geometry).width - 1;
            self.bottom = y0 + (*geometry).height - 1;
        }
    }
}

type DrawModeInitFunc = unsafe extern "cdecl" fn(*mut PixelBuffer, i32, i32) -> i32;
type DrawModeDeInitFunc = unsafe extern "cdecl" fn() -> i32;
type DrawModeBlitFlipFunc = unsafe extern "cdecl" fn() -> i32;
type DrawModeBlitRectFunc = unsafe extern "cdecl" fn(i32, i32, i32, i32) -> i32;
type DrawModeStretchBlitFunc = unsafe extern "cdecl" fn(i32, i32, i32, i32) -> i32;

#[repr(C)]
pub(super) struct DrawMode {
    index: u32,
    some_index_to_related_struct: i32,
    initialized: i32,
    unknown1: u32,
    init_func: DrawModeInitFunc,
    deinit_func: DrawModeDeInitFunc,
    pub blit_flip_func: DrawModeBlitFlipFunc,
    blit_rect_func: DrawModeBlitRectFunc,
    pub stretch_blit_func: DrawModeStretchBlitFunc,
    unknown2: u32,
}

#[repr(C)]
pub(super) struct DrawModeExtension {
    index: i32,
    window_mode: u32,
    gwl_style: u32,
    begin_func: *mut c_void,
    end_func: *mut c_void,
    set_palette_func: *mut c_void,
    unknown1: *mut c_void,
    unknown2: *mut c_void,
    /// Locks the display buffer for drawing; 0 on success. The GDI mode's
    /// implementation is the one `drawmode::hooks` replaces as `swap_buffers`.
    pub lock_display_buffer_func: unsafe extern "stdcall" fn() -> i32,
    unknown3: u32,
}

/// A 2D point with normalised 16.16 components
#[repr(C)]
#[derive(Clone, Copy)]
pub(super) struct Point {
    pub x: i32,
    pub y: i32,
}

/// a * b in 16.16 fixed-point
pub(super) fn fmul16(a: i32, b: i32) -> i32 {
    ((a as i64 * b as i64 + 0x8000) >> 16) as i32
}
