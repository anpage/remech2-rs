use std::{ffi::c_void, sync::RwLock};

use binding::{globals, macros::hook, patches};
use windows::{
    Win32::{
        Foundation::{HANDLE, HWND},
        Graphics::Gdi::BITMAPINFO,
        System::Memory::{HEAP_FLAGS, HeapAlloc, HeapFree},
    },
    core::BOOL,
};

use crate::{
    WINDOW_HEIGHT, WINDOW_WIDTH,
    drawmode::PaletteColor,
    sim::{MODULE, drawmode::custom_drawmode::CustomDrawMode, types::DrawModeExtension},
};

#[repr(C)]
pub struct PixelBuffer {
    pub data: *mut c_void,
    pub width: i32,
    pub height: i32,
    pub bitmap_info: *mut BITMAPINFO,
    pub unknown: u32,
}

globals!(
    pub(in crate::sim) static G_CURRENT_DRAW_MODE_EXTENSION: *mut DrawModeExtension = 0x000b1770;
    static G_PRIMARY_HEAP: HANDLE = 0x000acb68;
    static G_BITS_TO_BLIT: *mut u8 = 0x000b1a88;
    static G_GDI_BLIT_BITMAP_INFO: BITMAPINFO = 0x000c28a0;
    static G_GAME_WINDOW: HWND = 0x000acb60;
    static G_CURRENT_PIXEL_BUFFER: *mut PixelBuffer = 0x000b1784;
    static G_DISPLAY_BRIGHTNESS: u32 = 0x000a9468;
    static G_GAMMA_TABLE: [u8; 1024] = 0x000e99a0;
    static G_PALETTE_COLORS: [PaletteColor; 256] = 0x000b1788;
    static G_PALETTE_COLORS_PRE_BRIGHTNESS: [PaletteColor; 256] = 0x000e96a0;
    static G_MOUSE_CAPTURED: BOOL = 0x000ad244;
    pub static G_MOUSE_NEEDS_CENTERING: BOOL = 0x000ad248;
);

static CUSTOM_DRAW_MODE: RwLock<Option<CustomDrawMode>> = RwLock::new(None);

#[hook(rva = 0x0007762f)]
pub unsafe extern "cdecl" fn adjust_window_size(_draw_mode_ext: *mut c_void) {
    tracing::trace!("AdjustWindowSize called");
}

#[hook(rva = 0x0006de70)]
pub unsafe extern "stdcall" fn begin(
    pixel_buffer: *mut PixelBuffer,
    width: i32,
    height: i32,
) -> i32 {
    tracing::trace!("GdiBegin called with width: {}, height: {}!", width, height);

    unsafe {
        let pixel_buf = HeapAlloc(
            G_PRIMARY_HEAP.get(),
            HEAP_FLAGS(9),
            (height * width * 2) as usize,
        );
        (*pixel_buffer).data = pixel_buf;
        if pixel_buf.is_null() {
            return 2;
        }

        G_BITS_TO_BLIT.ptr().write_volatile(pixel_buf as *mut u8);

        (*pixel_buffer).width = width - 1;
        (*pixel_buffer).height = height - 1;
        (*pixel_buffer).bitmap_info = G_GDI_BLIT_BITMAP_INFO.ptr();

        let mut custom_draw_mode = CUSTOM_DRAW_MODE.write().unwrap();
        *custom_draw_mode =
            CustomDrawMode::new(G_GAME_WINDOW.get(), WINDOW_WIDTH, WINDOW_HEIGHT).ok();

        tracing::trace!("GdiBegin finish");

        0
    }
}

#[hook(rva = 0x0006dffe)]
pub unsafe extern "stdcall" fn end() -> i32 {
    tracing::trace!("GdiEnd called");

    unsafe {
        if !G_BITS_TO_BLIT.get().is_null() {
            let _ = HeapFree(
                G_PRIMARY_HEAP.get(),
                HEAP_FLAGS(1),
                Some(G_BITS_TO_BLIT.get() as *mut c_void),
            );
            G_BITS_TO_BLIT.ptr().write_volatile(std::ptr::null_mut());
        }

        (*(G_CURRENT_PIXEL_BUFFER.get())).data = std::ptr::null_mut();

        CUSTOM_DRAW_MODE.write().unwrap().take();
    }

    0
}

#[hook(rva = 0x0006e197)]
pub unsafe extern "stdcall" fn blit_flip() -> i32 {
    tracing::trace!("GdiBlitFlip called");

    let width = unsafe { (*(G_CURRENT_PIXEL_BUFFER.get())).width + 1 };
    let height = unsafe { (*(G_CURRENT_PIXEL_BUFFER.get())).height + 1 };

    if width <= 0 || height <= 0 {
        tracing::warn!(
            "GdiBlitFlip called with invalid dimensions: {}x{}",
            width,
            height
        );
        return 0;
    }

    let bits_to_blit = unsafe { G_BITS_TO_BLIT.get() };
    let pixel_slice =
        unsafe { std::slice::from_raw_parts(bits_to_blit, (width * height) as usize) };

    let mut custom_draw_mode = CUSTOM_DRAW_MODE.write().unwrap();
    if let Some(ref mut draw_mode) = *custom_draw_mode {
        draw_mode.draw(
            pixel_slice,
            width as usize,
            height as usize,
            unsafe { WINDOW_WIDTH },
            unsafe { WINDOW_HEIGHT },
        );
    }

    0
}

#[hook(rva = 0x0006e21f)]
pub unsafe extern "stdcall" fn bit_blt_rect(
    x_dest: i32,
    y_dest: i32,
    x2_dest: i32,
    y2_dest: i32,
) -> i32 {
    tracing::trace!(
        "GdiBitBltRect called with x_dest: {}, y_dest: {}, x2_dest: {}, y2_dest: {}",
        x_dest,
        y_dest,
        x2_dest,
        y2_dest
    );
    0
}

#[hook(rva = 0x0006e357)]
pub unsafe extern "stdcall" fn stretch_blit(
    x_src: i32,
    y_src2: i32,
    x_src2: i32,
    y_src_inverted: i32,
) -> i32 {
    tracing::trace!(
        "GdiStretchBlit called with x_src: {}, y_src2: {}, x_src2: {}, y_src_inverted: {}",
        x_src,
        y_src2,
        x_src2,
        y_src_inverted
    );
    0
}

#[hook(rva = 0x0006e5d8)]
pub unsafe extern "stdcall" fn set_palette(
    start: i32,
    count: i32,
    palette_colors: *const PaletteColor,
) -> i32 {
    tracing::trace!(
        "GdiSetPalette called with start: {}, count: {}, palette_colors: {:?}",
        start,
        count,
        palette_colors
    );

    if palette_colors.is_null() || !(0..=255).contains(&start) || count < 1 || (256 - start) < count
    {
        return -1;
    }

    for i in 0..count as isize {
        unsafe {
            let color = palette_colors.offset(i);
            let index = (start + i as i32) as usize;
            (*(G_PALETTE_COLORS.ptr()))[index] = *color;
        }
    }

    0
}

#[hook(rva = 0x0006e633)]
pub unsafe extern "stdcall" fn set_palette_with_brightness(palette_data: *mut c_void) -> i32 {
    tracing::trace!(
        "GdiSetPaletteWithBrightness called with palette_data: {:?}",
        palette_data
    );

    let palette = unsafe {
        std::slice::from_raw_parts(palette_data as *const PaletteColor, 256) // 256 colors * 3 bytes each
    };

    let brightness = unsafe { G_DISPLAY_BRIGHTNESS.get() } as usize;

    for (i, color) in palette.iter().enumerate().take(256) {
        unsafe {
            (*(G_PALETTE_COLORS_PRE_BRIGHTNESS.ptr()))[i] = *color;
            let PaletteColor { red, green, blue } = *color;
            (*(G_PALETTE_COLORS.ptr()))[i] = PaletteColor {
                red: (*(G_GAMMA_TABLE.ptr()))[red as usize + brightness * 64],
                green: (*(G_GAMMA_TABLE.ptr()))[green as usize + brightness * 64],
                blue: (*(G_GAMMA_TABLE.ptr()))[blue as usize + brightness * 64],
            };
        }
    }

    let mut custom_draw_mode = CUSTOM_DRAW_MODE.write().unwrap();
    if let Some(ref mut draw_mode) = *custom_draw_mode {
        draw_mode.set_palette(unsafe { G_PALETTE_COLORS.as_ref().unwrap() });
    }

    unsafe {
        (*(G_CURRENT_PIXEL_BUFFER.get())).data = G_BITS_TO_BLIT.get() as *mut c_void;
    }

    0
}

// Leave this out for now because the vanilla function works fine
// #[hook(rva = 0x0006e6d0)]
// unsafe extern "stdcall" fn(*mut c_void, i32) -> i32 {}

#[hook(rva = 0x0006e94f)]
pub unsafe extern "stdcall" fn swap_buffers() -> i32 {
    tracing::trace!("GdiSwapBuffers called");

    unsafe {
        (*(G_CURRENT_PIXEL_BUFFER.get())).data = G_BITS_TO_BLIT.get() as *mut c_void;
    }

    0
}

patches!(
    pub(in crate::sim) static PATCHES = [
        hook adjust_window_size,
        hook begin,
        hook end,
        hook blit_flip,
        hook bit_blt_rect,
        hook stretch_blit,
        hook set_palette,
        hook set_palette_with_brightness,
        hook swap_buffers,
    ];
);
