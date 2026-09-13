use std::ffi::c_void;

use super::{
    DrawModeBlitFlipFunc, DrawModeBlitRectFunc, DrawModeDeInitFunc, DrawModeInitFunc,
    DrawModeStretchBlitFunc, G_GAME_WINDOW_HEIGHT, G_GAME_WINDOW_WIDTH,
    drawmode::hooks::PixelBuffer,
};

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
}

/// The camera, which the game calls "Eyepoint"
#[repr(C)]
pub(super) struct Eyepoint {
    position: [i32; 3],
    rotation: [i32; 3],
    /// Horizontal FOV in 16.16, where `tan(fov / 2) == 1 / fov_x`
    pub fov_x: i32,
    unknown1: [i32; 4],
    viewport_left: i32,
    viewport_right: i32,
    viewport_top: i32,
    viewport_bottom: i32,
    hither_clip_plane: i32,
    yon_clip_plane: i32,
    /// `pixel width / pixel height` in 16.16
    pub pixel_aspect_ratio: i32,
}

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

/// The cockpit layout table passed to LoadCockpitLayout. It runs to at least
/// index 0x22; only the entries the detour needs are named.
#[repr(C)]
pub(super) struct CockpitLayout {
    /// The cockpit's 3D viewport, in normalised 16.16 coordinates
    viewport: *mut RenderTarget,
    unknown1: *mut c_void,
    /// Render target table slot the viewport is copied into
    pub render_target_slot: i32,
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
