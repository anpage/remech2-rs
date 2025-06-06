use std::{ffi::c_void, sync::RwLock};

use anyhow::Result;
use retour::GenericDetour;
use windows::Win32::{
    Foundation::{HANDLE, HWND, RECT},
    Graphics::Gdi::BITMAPINFO,
    System::Memory::{HEAP_FLAGS, HeapAlloc, HeapFree},
    UI::WindowsAndMessaging::{
        AdjustWindowRect, GetSystemMetrics, HWND_NOTOPMOST, HWND_TOP, HWND_TOPMOST, SM_CXSCREEN,
        SM_CYSCREEN, SWP_FRAMECHANGED, SWP_NOZORDER, SetWindowPos, WINDOW_STYLE,
    },
};

use crate::{hooker::hook_function, sim::drawmode::custom_drawmode::CustomDrawMode};

#[repr(C)]
pub struct PixelBuffer {
    pub data: *mut c_void,
    pub width: i32,
    pub height: i32,
    pub bitmap_info: *mut BITMAPINFO,
    pub unknown: u32,
}

#[repr(C)]
#[derive(Clone, Copy)]
pub struct PaletteColor {
    pub red: u8,
    pub green: u8,
    pub blue: u8,
}

// Draw Mode Functions
type GdiBeginFunc = unsafe extern "stdcall" fn(*mut PixelBuffer, i32, i32) -> i32;
static mut GDI_BEGIN_HOOK: Option<GenericDetour<GdiBeginFunc>> = None;

type GdiEndFunc = unsafe extern "stdcall" fn() -> i32;
static mut GDI_END_HOOK: Option<GenericDetour<GdiEndFunc>> = None;

type GdiBlitFlipFunc = unsafe extern "stdcall" fn() -> i32;
static mut GDI_BLIT_FLIP_HOOK: Option<GenericDetour<GdiBlitFlipFunc>> = None;

type GdiBitBltRectFunc = unsafe extern "stdcall" fn(i32, i32, i32, i32) -> i32;
static mut GDI_BIT_BLT_RECT_HOOK: Option<GenericDetour<GdiBitBltRectFunc>> = None;

type GdiStretchBlitFunc = unsafe extern "stdcall" fn(i32, i32, i32, i32) -> i32;
static mut GDI_STRETCH_BLIT_HOOK: Option<GenericDetour<GdiStretchBlitFunc>> = None;

// Draw Mode Extension Functions
type GdiSetPaletteFunc = unsafe extern "stdcall" fn(i32, i32, *const PaletteColor) -> i32;
static mut GDI_SET_PALETTE_HOOK: Option<GenericDetour<GdiSetPaletteFunc>> = None;

type GdiSetPaletteWithBrightnessFunc = unsafe extern "stdcall" fn(*mut c_void) -> i32;
static mut GDI_SET_PALETTE_WITH_BRIGHTNESS_HOOK: Option<
    GenericDetour<GdiSetPaletteWithBrightnessFunc>,
> = None;

// type GdiBlendPalettesFunc = unsafe extern "stdcall" fn(*mut c_void, i32) -> i32;
// static mut GDI_BLEND_PALETTES_HOOK: Option<GenericDetour<GdiBlendPalettesFunc>> = None;

type GdiSwapBuffersFunc = unsafe extern "stdcall" fn() -> i32;
static mut GDI_SWAP_BUFFERS_HOOK: Option<GenericDetour<GdiSwapBuffersFunc>> = None;

static mut G_CURRENT_DRAW_MODE_EXTENSION: *mut *mut c_void = std::ptr::null_mut();
static mut G_PRIMARY_HEAP: *mut HANDLE = std::ptr::null_mut();
static mut G_BITS_TO_BLIT: *mut *mut u8 = std::ptr::null_mut();
static mut G_GDI_BLIT_BITMAP_INFO: *mut BITMAPINFO = std::ptr::null_mut();
static mut G_GAME_WINDOW: *mut HWND = std::ptr::null_mut();
static mut G_CURRENT_PIXEL_BUFFER: *mut *mut PixelBuffer = std::ptr::null_mut();
static mut G_DISPLAY_BRIGHTNESS: *mut u32 = std::ptr::null_mut();
static mut G_GAMMA_TABLE: *mut [u8; 1024] = std::ptr::null_mut();
static mut G_PALETTE_COLORS: *mut [PaletteColor; 256] = std::ptr::null_mut();
static mut G_PALETTE_COLORS_PRE_BRIGHTNESS: *mut [PaletteColor; 256] = std::ptr::null_mut();

pub unsafe fn hook_functions(base_address: usize) -> Result<()> {
    unsafe {
        G_CURRENT_DRAW_MODE_EXTENSION = (base_address + 0x000b1770) as *mut *mut c_void;
        G_PRIMARY_HEAP = (base_address + 0x000acb68) as *mut HANDLE;
        G_BITS_TO_BLIT = (base_address + 0x000b1a88) as *mut *mut u8;
        G_GDI_BLIT_BITMAP_INFO = (base_address + 0x000c28a0) as *mut BITMAPINFO;
        G_GAME_WINDOW = (base_address + 0x000acb60) as *mut HWND;
        G_CURRENT_PIXEL_BUFFER = (base_address + 0x000b1784) as *mut *mut PixelBuffer;
        G_DISPLAY_BRIGHTNESS = (base_address + 0x000a9468) as *mut u32;
        G_GAMMA_TABLE = (base_address + 0x000e99a0) as *mut [u8; 1024];
        G_PALETTE_COLORS = (base_address + 0x000b1788) as *mut [PaletteColor; 256];
        G_PALETTE_COLORS_PRE_BRIGHTNESS = (base_address + 0x000e96a0) as *mut [PaletteColor; 256];

        GDI_BEGIN_HOOK = {
            let target: GdiBeginFunc = std::mem::transmute(base_address + 0x0006de70);
            Some(hook_function(target, begin)?)
        };

        GDI_END_HOOK = {
            let target: GdiEndFunc = std::mem::transmute(base_address + 0x0006dffe);
            Some(hook_function(target, end)?)
        };

        GDI_BLIT_FLIP_HOOK = {
            let target: GdiBlitFlipFunc = std::mem::transmute(base_address + 0x0006e197);
            Some(hook_function(target, blit_flip)?)
        };

        GDI_BIT_BLT_RECT_HOOK = {
            let target: GdiBitBltRectFunc = std::mem::transmute(base_address + 0x0006e21f);
            Some(hook_function(target, bit_blt_rect)?)
        };

        GDI_STRETCH_BLIT_HOOK = {
            let target: GdiStretchBlitFunc = std::mem::transmute(base_address + 0x0006e357);
            Some(hook_function(target, stretch_blit)?)
        };

        GDI_SET_PALETTE_HOOK = {
            let target: GdiSetPaletteFunc = std::mem::transmute(base_address + 0x0006e5d8);
            Some(hook_function(target, set_palette)?)
        };

        GDI_SET_PALETTE_WITH_BRIGHTNESS_HOOK = {
            let target: GdiSetPaletteWithBrightnessFunc =
                std::mem::transmute(base_address + 0x0006e633);
            Some(hook_function(target, set_palette_with_brightness)?)
        };

        // Leave this out for now because the vanilla function works fine
        // GDI_BLEND_PALETTES_HOOK = {
        //     let target: GdiBlendPalettesFunc = std::mem::transmute(base_address + 0x0006e6d0);
        //     Some(hook_function(target, blend_palettes)?)
        // };

        GDI_SWAP_BUFFERS_HOOK = {
            let target: GdiSwapBuffersFunc = std::mem::transmute(base_address + 0x0006e94f);
            Some(hook_function(target, swap_buffers)?)
        };
    }
    Ok(())
}

static CUSTOM_DRAW_MODE: RwLock<Option<CustomDrawMode>> = RwLock::new(None);

pub const WINDOW_WIDTH: i32 = 1920;
pub const WINDOW_HEIGHT: i32 = 1080;

pub unsafe extern "stdcall" fn begin(
    pixel_buffer: *mut PixelBuffer,
    width: i32,
    height: i32,
) -> i32 {
    tracing::trace!("GdiBegin called with width: {}, height: {}!", width, height);

    unsafe {
        let pixel_buf = HeapAlloc(
            *G_PRIMARY_HEAP,
            HEAP_FLAGS(9),
            (height * width * 2) as usize,
        );
        (*pixel_buffer).data = pixel_buf;
        if pixel_buf.is_null() {
            return 2;
        }

        G_BITS_TO_BLIT.write_volatile(pixel_buf as *mut u8);

        (*pixel_buffer).width = width;
        (*pixel_buffer).height = height;
        (*pixel_buffer).bitmap_info = G_GDI_BLIT_BITMAP_INFO;

        let desktop_width = GetSystemMetrics(SM_CXSCREEN);
        let desktop_height = GetSystemMetrics(SM_CYSCREEN);

        // let _ = SetWindowLongA(
        //     *G_GAME_WINDOW,
        //     GWL_STYLE,
        //     0x80000000u32 as i32 & 0xfff7ffffu32 as i32 | 0x10000000u32 as i32,
        // );

        let mut rect = RECT {
            left: 0,
            top: 0,
            right: WINDOW_WIDTH,
            bottom: WINDOW_HEIGHT,
        };

        let _ = AdjustWindowRect(&mut rect, WINDOW_STYLE(0xca0000), false);
        // let _ = AdjustWindowRect(&mut rect, WINDOW_STYLE(0x80000000), false);

        let _ = SetWindowPos(
            *G_GAME_WINDOW,
            Some(HWND_TOP),
            (desktop_width - (rect.right - rect.left)) / 2,
            (desktop_height - (rect.bottom - rect.top)) / 2,
            rect.right - rect.left,
            rect.bottom - rect.top,
            SWP_FRAMECHANGED | SWP_NOZORDER,
        );

        let mut custom_draw_mode = CUSTOM_DRAW_MODE.write().unwrap();
        *custom_draw_mode = CustomDrawMode::new(*G_GAME_WINDOW).ok();

        tracing::trace!("GdiBegin finish");

        0
    }
}

pub unsafe extern "stdcall" fn end() -> i32 {
    tracing::trace!("GdiEnd called");

    unsafe {
        if *G_BITS_TO_BLIT != std::ptr::null_mut() {
            let _ = HeapFree(
                *G_PRIMARY_HEAP,
                HEAP_FLAGS(1),
                Some(*G_BITS_TO_BLIT as *mut c_void),
            );
            G_BITS_TO_BLIT.write_volatile(std::ptr::null_mut());
        }

        (**G_CURRENT_PIXEL_BUFFER).data = std::ptr::null_mut();
    }

    0
}

pub unsafe extern "stdcall" fn blit_flip() -> i32 {
    tracing::trace!("GdiBlitFlip called");

    let width = unsafe { (**G_CURRENT_PIXEL_BUFFER).width } + 1;
    let height = unsafe { (**G_CURRENT_PIXEL_BUFFER).height };

    if width <= 0 || height <= 0 {
        tracing::warn!(
            "GdiBlitFlip called with invalid dimensions: {}x{}",
            width,
            height
        );
        return 0;
    }

    let bits_to_blit = unsafe { *G_BITS_TO_BLIT };
    let pixel_slice =
        unsafe { std::slice::from_raw_parts(bits_to_blit, (width * height) as usize) };

    let mut custom_draw_mode = CUSTOM_DRAW_MODE.write().unwrap();
    if let Some(ref mut draw_mode) = *custom_draw_mode {
        draw_mode.draw(pixel_slice, width as usize, height as usize);
    }

    0
}

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

    if palette_colors.is_null() || start < 0 || start > 255 || count < 1 || (256 - start) < count {
        return -1;
    }

    for i in 0..count as isize {
        unsafe {
            let color = palette_colors.offset(i);
            let index = (start + i as i32) as usize;
            (*G_PALETTE_COLORS)[index] = *color;
        }
    }

    0
}

pub unsafe extern "stdcall" fn set_palette_with_brightness(palette_data: *mut c_void) -> i32 {
    tracing::trace!(
        "GdiSetPaletteWithBrightness called with palette_data: {:?}",
        palette_data
    );

    let palette = unsafe {
        std::slice::from_raw_parts(palette_data as *const PaletteColor, 256) // 256 colors * 3 bytes each
    };

    let brightness = unsafe { *G_DISPLAY_BRIGHTNESS } as usize;

    for i in 0..256 {
        unsafe {
            let PaletteColor { red, green, blue } = palette[i];
            (*G_PALETTE_COLORS_PRE_BRIGHTNESS)[i] = PaletteColor { red, green, blue };
            (*G_PALETTE_COLORS)[i] = PaletteColor {
                red: (*G_GAMMA_TABLE)[red as usize + brightness * 64],
                green: (*G_GAMMA_TABLE)[green as usize + brightness * 64],
                blue: (*G_GAMMA_TABLE)[blue as usize + brightness * 64],
            };
        }
    }

    let mut custom_draw_mode = CUSTOM_DRAW_MODE.write().unwrap();
    if let Some(ref mut draw_mode) = *custom_draw_mode {
        draw_mode.set_palette(unsafe { G_PALETTE_COLORS.as_ref().unwrap() });
    }

    unsafe {
        (**G_CURRENT_PIXEL_BUFFER).data = *G_BITS_TO_BLIT as *mut c_void;
    }

    0
}

pub unsafe extern "stdcall" fn swap_buffers() -> i32 {
    tracing::trace!("GdiSwapBuffers called");

    unsafe {
        (**G_CURRENT_PIXEL_BUFFER).data = *G_BITS_TO_BLIT as *mut c_void;
    }

    0
}

pub unsafe fn unhook_functions() {
    unsafe {
        GDI_BEGIN_HOOK = None;
        GDI_END_HOOK = None;
        GDI_BLIT_FLIP_HOOK = None;
        GDI_BIT_BLT_RECT_HOOK = None;
        GDI_STRETCH_BLIT_HOOK = None;
        GDI_SET_PALETTE_HOOK = None;
        GDI_SET_PALETTE_WITH_BRIGHTNESS_HOOK = None;
        // GDI_BLEND_PALETTES_HOOK = None;
        GDI_SWAP_BUFFERS_HOOK = None;
    }
}
