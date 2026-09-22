use binding::{globals, macros::hook, patches};

use crate::sim::{
    RenderTarget,
    drawmode::hooks::PixelBuffer,
    types::{Point, fmul16},
    window::{
        G_GAME_WINDOW_GEOMETRY, G_GAME_WINDOW_HEIGHT, G_GAME_WINDOW_WIDTH, G_SCREEN_H_MINUS_1,
        G_SCREEN_W_MINUS_1,
    },
};

use super::MODULE;

globals!(
    static G_HORIZON_HAZE_THICKNESS: i32 = 0x000a6d30;
);

/// Offset to center the HUD box inside the framebuffer
pub(super) fn hud_origin() -> (i32, i32) {
    unsafe {
        let x0 = (G_GAME_WINDOW_WIDTH.get() as i32 - (*(G_GAME_WINDOW_GEOMETRY.get())).width) / 2;
        let y0 = (G_GAME_WINDOW_HEIGHT.get() as i32 - (*(G_GAME_WINDOW_GEOMETRY.get())).height) / 2;
        (x0, y0)
    }
}

/// Maps normalised 16.16 HUD coordinates onto [0, W-1].
/// Hooked to add the HUD origin to keep it centered in its own box.
/// `src` and `dst` are usually the same pointer, so read the whole rect before writing any of it back.
#[hook(rva = 0x00056920)]
unsafe extern "cdecl" fn scale_rect_to_screen(
    _pixel_buffer: *mut PixelBuffer,
    src: *mut RenderTarget,
    dst: *mut RenderTarget,
) -> *mut RenderTarget {
    if src.is_null() || dst.is_null() {
        return dst;
    }
    let (x0, y0) = hud_origin();
    let rect = unsafe { (*src).clone() };
    unsafe {
        (*dst).left = x0 + fmul16(G_SCREEN_W_MINUS_1.get(), rect.left);
        (*dst).top = y0 + fmul16(G_SCREEN_H_MINUS_1.get(), rect.top);
        (*dst).right = x0 + fmul16(G_SCREEN_W_MINUS_1.get(), rect.right);
        (*dst).bottom = y0 + fmul16(G_SCREEN_H_MINUS_1.get(), rect.bottom);
    }
    dst
}

/// Same as `scale_rect_to_screen` but for a single point.
#[hook(rva = 0x00056ae4)]
unsafe extern "cdecl" fn scale_point_to_screen(
    _pixel_buffer: *mut PixelBuffer,
    src: *mut Point,
    dst: *mut Point,
) -> *mut Point {
    if src.is_null() || dst.is_null() {
        return dst;
    }
    let (x0, y0) = hud_origin();
    let point = unsafe { *src };
    unsafe {
        (*dst).x = x0 + fmul16(G_SCREEN_W_MINUS_1.get(), point.x);
        (*dst).y = y0 + fmul16(G_SCREEN_H_MINUS_1.get(), point.y);
    }
    dst
}

/// Centers a fixed-size rect on screen
#[hook(rva = 0x00056e22)]
unsafe extern "cdecl" fn center_rect_on_screen(
    _pixel_buffer: *mut PixelBuffer,
    src: *mut RenderTarget,
    dst: *mut RenderTarget,
) -> *mut RenderTarget {
    if src.is_null() || dst.is_null() {
        return dst;
    }
    let (x0, y0) = hud_origin();
    let rect = unsafe { (*src).clone() };
    let width = rect.right - rect.left + 1;
    let height = rect.bottom - rect.top + 1;
    unsafe {
        let left = x0 + (G_SCREEN_W_MINUS_1.get() - width - 1) / 2;
        let top = y0 + (G_SCREEN_H_MINUS_1.get() - height - 1) / 2;
        (*dst).left = left;
        (*dst).top = top;
        (*dst).right = left + width - 1;
        (*dst).bottom = top + height - 1;
    }
    dst
}

/// Rescales every 320x200-authored table for the current resolution.
/// We subtract the HUD origin offset from the horizon haze thickness to avoid stretching the sky vertically.
/// This is kind of a hack. We should probably reimplement this function entirely.
#[hook(rva = 0x0005d4d3)]
unsafe extern "cdecl" fn set_res() {
    unsafe {
        original();
        let (x0, _) = hud_origin();
        G_HORIZON_HAZE_THICKNESS.set(G_HORIZON_HAZE_THICKNESS.get() - x0);
    }
}

patches!(
    pub(super) static PATCHES = [
        hook scale_rect_to_screen,
        hook scale_point_to_screen,
        hook center_rect_on_screen,
        hook set_res,
    ];
);
