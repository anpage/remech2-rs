use std::{ffi::c_void, sync::RwLock};

use anyhow::Result;
use retour::GenericDetour;
use windows::{
    Win32::{
        Foundation::{HANDLE, HWND, POINT, RECT},
        Graphics::Gdi::{BITMAPINFO, ScreenToClient},
        Media::timeGetTime,
        System::Memory::{HEAP_FLAGS, HeapAlloc, HeapFree},
        UI::{
            Input::KeyboardAndMouse::{GetAsyncKeyState, VK_LBUTTON, VK_MBUTTON, VK_RBUTTON},
            WindowsAndMessaging::{
                AdjustWindowRect, GetCursorPos, GetSystemMetrics, HWND_TOP, SM_CXSCREEN,
                SM_CYSCREEN, SWP_FRAMECHANGED, SWP_NOZORDER, SetWindowPos, WS_OVERLAPPEDWINDOW,
            },
        },
    },
    core::BOOL,
};

use crate::{
    WINDOW_HEIGHT, WINDOW_WIDTH,
    custom_drawmode::{CustomDrawMode, PaletteColor},
    hooker::hook_function,
};

#[repr(C, packed(2))]
pub struct PixelBuffer {
    pub data: *mut c_void,
    pub width: i32,
    pub height: i32,
    pub bitmap_info: *mut BITMAPINFO,
    pub unknown: u32,
}

#[repr(C, packed(1))]
#[derive(Debug)]
pub struct MouseState {
    pub unknown1: u32,
    pub unknown2: u32,
    pub unknown3: u32,
    pub unknown4: u32,
    pub left_pressed: BOOL,
    pub right_pressed: BOOL,
    pub middle_pressed: BOOL,
    pub double_clicked: u8,
    pub unknown5: u16,
    pub last_clicked: u32,
    pub unknown6: u32,
    pub unknown7: u32,
    pub pos_x: i32,
    pub pos_y: i32,
    pub left_down: BOOL,
    pub right_down: BOOL,
    pub middle_down: BOOL,
    pub some_flag: u32,
}

type InitDrawModeFunc =
    unsafe extern "cdecl" fn(i32, i32, *mut PixelBuffer, i32, i32, BOOL) -> BOOL;
static INIT_DRAW_MODE_HOOK: RwLock<Option<GenericDetour<InitDrawModeFunc>>> = RwLock::new(None);

type AdjustWindowSizeFunc = unsafe extern "cdecl" fn(*mut c_void);
static mut ADJUST_WINDOW_SIZE_HOOK: Option<GenericDetour<AdjustWindowSizeFunc>> = None;

type ToggleFullscreenFunc = unsafe extern "stdcall" fn();
static mut TOGGLE_FULLSCREEN_HOOK: Option<GenericDetour<ToggleFullscreenFunc>> = None;

type ReadMouseStateFunc = unsafe extern "fastcall" fn(*mut MouseState);
static mut READ_MOUSE_STATE_HOOK: Option<GenericDetour<ReadMouseStateFunc>> = None;

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
static mut G_WINDOW: *mut HWND = std::ptr::null_mut();
static mut G_CURRENT_PIXEL_BUFFER: *mut *mut PixelBuffer = std::ptr::null_mut();
static mut G_DISPLAY_BRIGHTNESS: *mut u32 = std::ptr::null_mut();
static mut G_GAMMA_TABLE: *mut [u8; 1024] = std::ptr::null_mut();
static mut G_PALETTE_COLORS: *mut [PaletteColor; 256] = std::ptr::null_mut();
static mut G_PALETTE_COLORS_PRE_BRIGHTNESS: *mut [PaletteColor; 256] = std::ptr::null_mut();

pub unsafe fn hook_functions(base_address: usize) -> Result<()> {
    unsafe {
        G_CURRENT_DRAW_MODE_EXTENSION = (base_address + 0x00062cc8) as *mut *mut c_void;
        G_PRIMARY_HEAP = (base_address + 0x0006a9f4) as *mut HANDLE;
        G_BITS_TO_BLIT = (base_address + 0x00062fe0) as *mut *mut u8;
        G_GDI_BLIT_BITMAP_INFO = (base_address + 0x00096a64) as *mut BITMAPINFO;
        G_WINDOW = (base_address + 0x000965ec) as *mut HWND;
        G_CURRENT_PIXEL_BUFFER = (base_address + 0x00062cdc) as *mut *mut PixelBuffer;
        G_DISPLAY_BRIGHTNESS = (base_address + 0x000717a4) as *mut u32;
        G_GAMMA_TABLE = (base_address + 0x000961d0) as *mut [u8; 1024];
        G_PALETTE_COLORS = (base_address + 0x00062ce0) as *mut [PaletteColor; 256];
        G_PALETTE_COLORS_PRE_BRIGHTNESS = (base_address + 0x00095ed0) as *mut [PaletteColor; 256];

        *INIT_DRAW_MODE_HOOK.write().unwrap() = {
            let target: InitDrawModeFunc = std::mem::transmute(base_address + 0x00010a30);
            Some(hook_function(target, init_draw_mode)?)
        };

        ADJUST_WINDOW_SIZE_HOOK = {
            let target: AdjustWindowSizeFunc = std::mem::transmute(base_address + 0x000112da);
            Some(hook_function(target, adjust_window_size)?)
        };

        TOGGLE_FULLSCREEN_HOOK = {
            let target: ToggleFullscreenFunc = std::mem::transmute(base_address + 0x00011071);
            Some(hook_function(target, toggle_fullscreen)?)
        };

        READ_MOUSE_STATE_HOOK = {
            let target: ReadMouseStateFunc = std::mem::transmute(base_address + 0x0003aac5);
            Some(hook_function(target, read_mouse_state)?)
        };

        GDI_BEGIN_HOOK = {
            let target: GdiBeginFunc = std::mem::transmute(base_address + 0x00030b90);
            Some(hook_function(target, begin)?)
        };

        GDI_END_HOOK = {
            let target: GdiEndFunc = std::mem::transmute(base_address + 0x00030d31);
            Some(hook_function(target, end)?)
        };

        GDI_BLIT_FLIP_HOOK = {
            let target: GdiBlitFlipFunc = std::mem::transmute(base_address + 0x00030ef9);
            Some(hook_function(target, blit_flip)?)
        };

        GDI_BIT_BLT_RECT_HOOK = {
            let target: GdiBitBltRectFunc = std::mem::transmute(base_address + 0x00030f77);
            Some(hook_function(target, bit_blt_rect)?)
        };

        GDI_STRETCH_BLIT_HOOK = {
            let target: GdiStretchBlitFunc = std::mem::transmute(base_address + 0x00031095);
            Some(hook_function(target, stretch_blit)?)
        };

        GDI_SET_PALETTE_HOOK = {
            let target: GdiSetPaletteFunc = std::mem::transmute(base_address + 0x00031591);
            Some(hook_function(target, set_palette)?)
        };

        GDI_SET_PALETTE_WITH_BRIGHTNESS_HOOK = {
            let target: GdiSetPaletteWithBrightnessFunc =
                std::mem::transmute(base_address + 0x0003162c);
            Some(hook_function(target, set_palette_with_brightness)?)
        };

        // Leave this out for now because the vanilla function works fine
        // GDI_BLEND_PALETTES_HOOK = {
        //     let target: GdiBlendPalettesFunc = std::mem::transmute(base_address + 0x000316c9);
        //     Some(hook_function(target, blend_palettes)?)
        // };

        GDI_SWAP_BUFFERS_HOOK = {
            let target: GdiSwapBuffersFunc = std::mem::transmute(base_address + 0x00031948);
            Some(hook_function(target, swap_buffers)?)
        };
    }
    Ok(())
}

static CUSTOM_DRAW_MODE: RwLock<Option<CustomDrawMode>> = RwLock::new(None);

pub unsafe extern "cdecl" fn init_draw_mode(
    draw_mode_index: i32,
    allow_fallback: i32,
    output_pixel_buffer: *mut PixelBuffer,
    width: i32,
    height: i32,
    show_menu: BOOL,
) -> BOOL {
    tracing::trace!(
        "InitDrawMode called with index: {}, allow_fallback: {}, width: {}, height: {}, show_menu: {}",
        draw_mode_index,
        allow_fallback,
        width,
        height,
        show_menu.0
    );
    unsafe {
        INIT_DRAW_MODE_HOOK.read().unwrap().as_ref().unwrap().call(
            5,
            allow_fallback,
            output_pixel_buffer,
            width,
            height,
            show_menu,
        )
    }
}

pub unsafe extern "cdecl" fn adjust_window_size(_draw_mode_ext: *mut c_void) {
    tracing::trace!("AdjustWindowSize called");
}

pub unsafe extern "stdcall" fn toggle_fullscreen() {
    tracing::trace!("ToggleFullscreen called");
}

/// translates the cursor position from the client window to the shell's coordinate system
fn cursor_window_to_shell(x: i32, y: i32, window_width: i32, window_height: i32) -> (i32, i32) {
    const SHELL_LOGICAL_WIDTH: f32 = 640.;
    const SHELL_LOGICAL_HEIGHT: f32 = 480.;

    let aspect_ratio = 4.0 / 3.0;
    let mut shell_width = window_width as f32;
    let mut shell_height = window_height as f32;
    if shell_width / shell_height > aspect_ratio {
        shell_width = shell_height * aspect_ratio;
    } else {
        shell_height = shell_width / aspect_ratio;
    }

    let shell_x = (x as f32 - (window_width as f32 - shell_width) / 2.0)
        / (shell_width as f32 / SHELL_LOGICAL_WIDTH);
    let shell_y = (y as f32 - (window_height as f32 - shell_height) / 2.0)
        / (shell_height as f32 / SHELL_LOGICAL_HEIGHT);

    (shell_x as i32, shell_y as i32)
}

pub unsafe extern "fastcall" fn read_mouse_state(mouse_state: *mut MouseState) {
    tracing::trace!("ReadMouseState called");

    let state = unsafe { mouse_state.as_mut().expect("mouse_state to not be null") };

    let left_down_previous = state.left_down;
    let right_down_previous = state.right_down;
    let middle_down_previous = state.middle_down;

    let mut cursor_pos = POINT { x: 0, y: 0 };
    if let Err(e) = unsafe { GetCursorPos(&mut cursor_pos) } {
        tracing::error!("GetCursorPos failed: {:?}", e);
        return;
    }

    let _ = unsafe { ScreenToClient(*G_WINDOW, &mut cursor_pos) };
    let mut cursor_inside_window = true;
    if cursor_pos.x < 0 || cursor_pos.x >= unsafe { WINDOW_WIDTH } {
        cursor_inside_window = false;
    } else if cursor_pos.y < 0 || cursor_pos.y >= unsafe { WINDOW_HEIGHT } {
        cursor_inside_window = false;
    }

    if !cursor_inside_window {
        return;
    }

    let mut left_down_current = BOOL(0);
    let mut right_down_current = BOOL(0);
    let mut middle_down_current = BOOL(0);

    if state.some_flag != 0 {
        (state.pos_x, state.pos_y) = cursor_window_to_shell(
            cursor_pos.x,
            cursor_pos.y,
            unsafe { WINDOW_WIDTH },
            unsafe { WINDOW_HEIGHT },
        );

        let key_state = unsafe { GetAsyncKeyState(VK_LBUTTON.0 as i32) as u16 };
        if (key_state & 0x8000) != 0 {
            state.left_down = BOOL(1);
            left_down_current = BOOL(1);
        } else {
            state.left_down = BOOL(0);
        }
        let key_state = unsafe { GetAsyncKeyState(VK_RBUTTON.0 as i32) as u16 };
        if (key_state & 0x8000) != 0 {
            state.right_down = BOOL(1);
            right_down_current = BOOL(1);
        } else {
            state.right_down = BOOL(0);
        }
        let key_state = unsafe { GetAsyncKeyState(VK_MBUTTON.0 as i32) as u16 };
        if (key_state & 0x8000) != 0 {
            state.middle_down = BOOL(1);
            middle_down_current = BOOL(1);
        } else {
            state.middle_down = BOOL(0);
        }
    }

    if left_down_current == BOOL(1) && left_down_previous == BOOL(0) {
        state.left_pressed = BOOL(1);
    } else {
        state.left_pressed = BOOL(0);
    }
    if right_down_current == BOOL(1) && right_down_previous == BOOL(0) {
        state.right_pressed = BOOL(1);
    } else {
        state.right_pressed = BOOL(0);
    }
    if middle_down_current == BOOL(1) && middle_down_previous == BOOL(0) {
        state.middle_pressed = BOOL(1);
    } else {
        state.middle_pressed = BOOL(0);
    }

    state.double_clicked = 0;
    if left_down_current == BOOL(1) && left_down_previous == BOOL(0) {
        let current_time = unsafe { timeGetTime() };
        if current_time - state.last_clicked < 201 {
            state.double_clicked = 1;
        } else {
            state.last_clicked = unsafe { timeGetTime() };
        }
    }
}

pub unsafe extern "stdcall" fn begin(
    pixel_buffer: *mut PixelBuffer,
    width: i32,
    height: i32,
) -> i32 {
    tracing::trace!(
        "GdiBegin called with pixel_buffer: {:?}, width: {}, height: {}!",
        pixel_buffer,
        width,
        height
    );

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

        let mut custom_draw_mode = CUSTOM_DRAW_MODE.write().unwrap();
        *custom_draw_mode = CustomDrawMode::new(*G_WINDOW, WINDOW_WIDTH, WINDOW_HEIGHT).ok();

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

        CUSTOM_DRAW_MODE.write().unwrap().take();
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
        draw_mode.draw(
            pixel_slice,
            width as usize,
            height as usize,
            unsafe { WINDOW_WIDTH },
            unsafe { WINDOW_HEIGHT },
        );
    }

    tracing::trace!("GdiBlitFlip finished");

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

    unsafe { blit_flip() }
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

    unsafe { blit_flip() }
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

    let mut custom_draw_mode = CUSTOM_DRAW_MODE.write().unwrap();
    if let Some(ref mut draw_mode) = *custom_draw_mode {
        draw_mode.set_palette(unsafe { G_PALETTE_COLORS.as_ref().unwrap() });
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
        *INIT_DRAW_MODE_HOOK.write().unwrap() = None;
        ADJUST_WINDOW_SIZE_HOOK = None;
        TOGGLE_FULLSCREEN_HOOK = None;
        READ_MOUSE_STATE_HOOK = None;
        GDI_BEGIN_HOOK = None;
        GDI_END_HOOK = None;
        GDI_BLIT_FLIP_HOOK = None;
        GDI_BIT_BLT_RECT_HOOK = None;
        GDI_STRETCH_BLIT_HOOK = None;
        GDI_SET_PALETTE_HOOK = None;
        GDI_SET_PALETTE_WITH_BRIGHTNESS_HOOK = None;
        // GDI_BLEND_PALETTES_HOOK = None;
        GDI_SWAP_BUFFERS_HOOK = None;
        CUSTOM_DRAW_MODE.write().unwrap().take();
    }
}
