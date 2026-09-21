use std::{
    ffi::{c_char, c_void},
    sync::RwLock,
};

use binding::{game_fns, globals, macros::hook, patches, thunks};
use windows::{
    Win32::{
        Foundation::{HANDLE, HWND, POINT},
        Graphics::Gdi::{BITMAPINFO, ScreenToClient},
        Media::timeGetTime,
        System::Memory::{HEAP_FLAGS, HeapAlloc, HeapFree},
        UI::{
            Input::KeyboardAndMouse::{GetAsyncKeyState, VK_LBUTTON, VK_MBUTTON, VK_RBUTTON},
            WindowsAndMessaging::{GetCursorPos, IsWindow},
        },
    },
    core::BOOL,
};

use crate::{
    WINDOW_HEIGHT, WINDOW_WIDTH,
    shell::{
        MODULE,
        drawmode::custom_drawmode::{CustomDrawMode, OverlayMouseState, PaletteColor},
    },
};

const CURSOR_GRAPHIC_SIZE: usize = 423;

#[repr(C, packed(2))]
#[derive(Debug)]
pub struct PixelBuffer {
    pub data: *mut u8,
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

#[repr(C, packed(1))]
pub struct VideoDriver {
    pub unknown: [u8; 22],
    pub collection_1: *mut *mut c_void,
    pub collection_2: *mut *mut c_void,
    pub should_set_palette: i32,
    pub set_palette_unknown: u32,
    pub unknown2: [u8; 8],
    pub pixel_buffer: PixelBuffer,
    pub back_buffer: *mut u8,
    pub pix_buf_width: i32,
    pub pix_buf_height: i32,
    pub unknown3: [u8; 8],
    pub p_pixel_buffer: *mut PixelBuffer,
    pub other_x2: i32,
    pub other_y2: i32,
    pub other_x1: i32,
    pub other_y1: i32,
    pub p_back_buffer: *mut *mut c_char,
    pub initial_other_x2: i32,
    pub initial_other_y2: i32,
    pub pixel_buffer_width: i32,
    pub pixel_buffer_height: i32,
    pub other_p_pixel_buffer: *mut *mut PixelBuffer,
    pub x1: i32,
    pub y1: i32,
    pub x2: i32,
    pub y2: i32,
    pub palette: [PaletteColor; 256],
    pub width: i32,
    pub height: i32,
    pub width_minus_one: i32,
    pub height_minus_one: i32,
    pub unknown4: u32,
    pub unknown5: u32,
    pub use_back_buffer: i32,
}

globals!(
    // static G_CURRENT_DRAW_MODE_EXTENSION: *mut c_void = 0x00062cc8;
    // static G_VIDEO_DRIVER: *mut VideoDriver = 0x00071208;
    static G_PRIMARY_HEAP: HANDLE = 0x0006a9f4;
    static G_BITS_TO_BLIT: *mut u8 = 0x00062fe0;
    static G_GDI_BLIT_BITMAP_INFO: BITMAPINFO = 0x00096a64;
    pub(in crate::shell) static G_WINDOW: HWND = 0x000965ec;
    static G_CURRENT_PIXEL_BUFFER: *mut PixelBuffer = 0x00062cdc;
    static G_DISPLAY_BRIGHTNESS: u32 = 0x000717a4;
    static G_GAMMA_TABLE: [u8; 1024] = 0x000961d0;
    static G_PALETTE_COLORS: [PaletteColor; 256] = 0x00062ce0;
    static G_PALETTE_COLORS_PRE_BRIGHTNESS: [PaletteColor; 256] = 0x00095ed0;
    pub(in crate::shell) static G_CURRENT_MOUSE_STATE: *mut MouseState = 0x00071204;
    static G_SOME_PALETTE_FLAG: u32 = 0x0005c2a4;
    pub(in crate::shell) static G_CURSOR_GRAPHIC: *mut [u8; CURSOR_GRAPHIC_SIZE] = 0x00071200;
);

game_fns!(
    static SOME_PALETTE_FUNC: unsafe extern "stdcall" fn() = 0x00005eea;
    static VIDEO_DRIVER_ACTIVATE_FRAMEBUFFER: unsafe extern "thiscall" fn(*mut VideoDriver) =
        0x000077ea;
);

thunks! {
    /// `ShowCursor`, so the overlay draws the cursor instead of Windows.
    static SHOW_CURSOR_THUNK = [
        0x000995d0 => show_cursor,
    ];
}

static CUSTOM_DRAW_MODE: RwLock<Option<CustomDrawMode>> = RwLock::new(None);
static DRAW_CALLED: RwLock<bool> = RwLock::new(false);

/// Drops the overlay's GPU resources when the shell unloads.
pub(in crate::shell) fn shutdown() {
    CUSTOM_DRAW_MODE.write().unwrap().take();
}

#[hook(rva = 0x00010a30)]
unsafe extern "cdecl" fn init_draw_mode(
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
        original(
            5,
            allow_fallback,
            output_pixel_buffer,
            width,
            height,
            show_menu,
        )
    }
}

#[hook(rva = 0x000112da)]
unsafe extern "cdecl" fn adjust_window_size(_draw_mode_ext: *mut c_void) {
    tracing::trace!("AdjustWindowSize called");
}

#[hook(rva = 0x00011071)]
unsafe extern "stdcall" fn toggle_fullscreen() {
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
        / (shell_width / SHELL_LOGICAL_WIDTH);
    let shell_y = (y as f32 - (window_height as f32 - shell_height) / 2.0)
        / (shell_height / SHELL_LOGICAL_HEIGHT);

    (shell_x as i32, shell_y as i32)
}

pub unsafe fn get_mouse_state() -> OverlayMouseState {
    let mut cursor_pos = POINT { x: 0, y: 0 };
    unsafe { GetCursorPos(&mut cursor_pos).unwrap() };
    let _ = unsafe { ScreenToClient(G_WINDOW.get(), &mut cursor_pos) };

    let key_state = unsafe { GetAsyncKeyState(VK_LBUTTON.0 as i32) as u16 };
    let left_down = (key_state & 0x8000) != 0;

    let key_state = unsafe { GetAsyncKeyState(VK_RBUTTON.0 as i32) as u16 };
    let right_down = (key_state & 0x8000) != 0;

    let key_state = unsafe { GetAsyncKeyState(VK_MBUTTON.0 as i32) as u16 };
    let middle_down = (key_state & 0x8000) != 0;

    OverlayMouseState {
        pos_x: cursor_pos.x,
        pos_y: cursor_pos.y,
        left_down,
        right_down,
        middle_down,
    }
}

pub fn update_global_mouse_state(mouse_state: &OverlayMouseState) {
    if G_CURRENT_MOUSE_STATE.ptr().is_null() {
        return;
    }

    let Some(state) = (unsafe { G_CURRENT_MOUSE_STATE.get().as_mut() }) else {
        return;
    };

    let left_down_previous = state.left_down;
    let right_down_previous = state.right_down;
    let middle_down_previous = state.middle_down;

    let mut cursor_inside_window = true;
    if (mouse_state.pos_x < 0 || mouse_state.pos_x >= unsafe { WINDOW_WIDTH })
        || (mouse_state.pos_y < 0 || mouse_state.pos_y >= unsafe { WINDOW_HEIGHT })
    {
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
            mouse_state.pos_x,
            mouse_state.pos_y,
            unsafe { WINDOW_WIDTH },
            unsafe { WINDOW_HEIGHT },
        );

        if mouse_state.left_down {
            state.left_down = BOOL(1);
            left_down_current = BOOL(1);
        } else {
            state.left_down = BOOL(0);
        }
        if mouse_state.right_down {
            state.right_down = BOOL(1);
            right_down_current = BOOL(1);
        } else {
            state.right_down = BOOL(0);
        }
        if mouse_state.middle_down {
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

/// Disable the game's own mouse state reading function to handle mouse state after egui draws.
/// This allows us to intercept mouse events and update the global mouse state selectively.
#[hook(rva = 0x0003aac5)]
unsafe extern "fastcall" fn read_mouse_state(_mouse_state: *mut MouseState) {
    tracing::trace!("ReadMouseState called");

    let custom_draw_mode = CUSTOM_DRAW_MODE.read().unwrap();
    if let Some(ref draw_mode) = *custom_draw_mode {
        draw_mode.update_mouse_state();
    }
}

/// This function is called to draw the shell, as opposed to FMVs.
/// It's been modified to always draw the entire screen so that egui gets rendered every frame.
#[hook(rva = 0x00006502)]
unsafe extern "fastcall" fn video_driver_draw_shell(this: *mut VideoDriver) {
    tracing::trace!("VideoDriverDrawShell called");

    unsafe {
        if (*this).x1 < (*this).other_x2 {
            (*this).x1 = (*this).other_x2;
        }
        if (*this).y1 < (*this).other_y2 {
            (*this).y1 = (*this).other_y2;
        }
        if (*this).x2 > (*this).other_x1 {
            (*this).x2 = (*this).other_x1;
        }
        if (*this).y2 > (*this).other_y1 {
            (*this).y2 = (*this).other_y1;
        }

        if (*this).should_set_palette == 0 {
            bit_blt_rect((*this).x1, (*this).y1, (*this).x2, (*this).y2);
        } else {
            if (*this).set_palette_unknown == 0 {
                if (*this).use_back_buffer == 0 {
                    set_palette(
                        0,
                        256,
                        &(*this).palette as *const _,
                        (*this).set_palette_unknown,
                    );
                    bit_blt_rect(
                        (*this).other_x2,
                        (*this).other_y2,
                        (*this).other_x1,
                        (*this).other_y1,
                    );
                } else {
                    (*this).back_buffer.copy_from(
                        (*this).pixel_buffer.data,
                        ((*this).width * (*this).height) as usize,
                    );
                    (VIDEO_DRIVER_ACTIVATE_FRAMEBUFFER.get())(this);
                    set_palette(
                        0,
                        256,
                        &(*this).palette as *const _,
                        (*this).set_palette_unknown,
                    );
                    (*this).pixel_buffer.data.copy_from(
                        (*this).back_buffer,
                        ((*this).width * (*this).height) as usize,
                    );
                    blit_flip();
                    (*this).use_back_buffer = 0;
                }
            } else if G_SOME_PALETTE_FLAG.get() == 0 {
                (VIDEO_DRIVER_ACTIVATE_FRAMEBUFFER.get())(this);
                set_palette(
                    0,
                    256,
                    &(*this).palette as *const _,
                    (*this).set_palette_unknown,
                );
                (*this).pixel_buffer.data.copy_from(
                    (*this).back_buffer,
                    ((*this).width * (*this).height) as usize,
                );
                blit_flip();
            } else {
                (SOME_PALETTE_FUNC.get())();
                blit_flip();
                set_palette(
                    0,
                    256,
                    &(*this).palette as *const _,
                    (*this).set_palette_unknown,
                );
                blit_flip();
                G_SOME_PALETTE_FLAG.set(0);
            }
            (*this).should_set_palette = 0;
        }

        (*this).x1 = (*this).other_x1;
        (*this).y1 = (*this).other_y1;
        (*this).x2 = (*this).other_x2;
        (*this).y2 = (*this).other_y2;
    }
}

unsafe extern "stdcall" fn show_cursor(show: BOOL) -> i32 {
    tracing::trace!("ShowCursor called with show: {}", show.0);

    let mut custom_draw_mode = CUSTOM_DRAW_MODE.write().unwrap();
    if let Some(ref mut draw_mode) = *custom_draw_mode {
        draw_mode.show_cursor(show.0 != 0);
    }

    if show.0 == 0 { -1 } else { 1 }
}

#[hook(rva = 0x00030b90)]
unsafe extern "stdcall" fn begin(pixel_buffer: *mut PixelBuffer, width: i32, height: i32) -> i32 {
    tracing::trace!(
        "GdiBegin called with pixel_buffer: {:?}, width: {}, height: {}!",
        pixel_buffer,
        width,
        height
    );

    unsafe {
        let pixel_buf = HeapAlloc(
            G_PRIMARY_HEAP.get(),
            HEAP_FLAGS(9),
            (height * width * 2) as usize,
        ) as *mut u8;
        (*pixel_buffer).data = pixel_buf;
        if pixel_buf.is_null() {
            return 2;
        }

        G_BITS_TO_BLIT.ptr().write_volatile(pixel_buf);

        (*pixel_buffer).width = width;
        (*pixel_buffer).height = height;
        (*pixel_buffer).bitmap_info = G_GDI_BLIT_BITMAP_INFO.ptr();

        *CUSTOM_DRAW_MODE.write().unwrap() =
            CustomDrawMode::new(G_WINDOW.get(), WINDOW_WIDTH, WINDOW_HEIGHT).ok();

        tracing::trace!("GdiBegin finish");

        0
    }
}

#[hook(rva = 0x00030d31)]
unsafe extern "stdcall" fn end() -> i32 {
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

        (*G_CURRENT_PIXEL_BUFFER.get()).data = std::ptr::null_mut();

        CUSTOM_DRAW_MODE.write().unwrap().take();
    }

    0
}

#[hook(rva = 0x00030ef9)]
unsafe extern "stdcall" fn blit_flip() -> i32 {
    tracing::trace!("GdiBlitFlip called");

    let called = { *DRAW_CALLED.read().unwrap() };
    if called {
        return 0;
    }

    {
        *DRAW_CALLED.write().unwrap() = true;
    }

    if G_WINDOW.ptr().is_null() || !unsafe { IsWindow(Some(G_WINDOW.get())) }.as_bool() {
        return 0;
    }

    let width = unsafe { (*G_CURRENT_PIXEL_BUFFER.get()).width } + 1;
    let height = unsafe { (*G_CURRENT_PIXEL_BUFFER.get()).height };

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

    {
        let mut custom_draw_mode = CUSTOM_DRAW_MODE.write().unwrap();
        if let Some(ref mut draw_mode) = *custom_draw_mode {
            draw_mode.draw(
                pixel_slice,
                width as usize,
                height as usize,
                unsafe { WINDOW_WIDTH },
                unsafe { WINDOW_HEIGHT },
                unsafe { G_WINDOW.get() },
            );
        }
    }

    {
        *DRAW_CALLED.write().unwrap() = false;
    }

    tracing::trace!("GdiBlitFlip finished");

    0
}

#[hook(rva = 0x00030f77)]
unsafe extern "stdcall" fn bit_blt_rect(
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

#[hook(rva = 0x00031095)]
unsafe extern "stdcall" fn stretch_blit(x1: i32, y1: i32, x2: i32, y2: i32) -> i32 {
    tracing::trace!(
        "GdiStretchBlit called with (x1, y1): ({}, {}), (x2, y2): ({}, {})",
        x1,
        y1,
        x2,
        y2
    );

    let called = { *DRAW_CALLED.read().unwrap() };
    if called {
        return 0;
    }

    {
        *DRAW_CALLED.write().unwrap() = true;
    }

    if G_WINDOW.ptr().is_null() || !unsafe { IsWindow(Some(G_WINDOW.get())) }.as_bool() {
        return 0;
    }

    let buffer_width = unsafe { (*G_CURRENT_PIXEL_BUFFER.get()).width } + 1;
    let buffer_height = unsafe { (*G_CURRENT_PIXEL_BUFFER.get()).height };

    let bits_to_blit = unsafe { G_BITS_TO_BLIT.get() };
    let pixel_slice = unsafe {
        std::slice::from_raw_parts(bits_to_blit, (buffer_width * buffer_height) as usize)
    };

    let rect_pixel_slice = pixel_slice
        .chunks_exact(buffer_width as usize)
        .skip(y1 as usize)
        .take((y2 - y1) as usize)
        .flat_map(|row| row[x1 as usize..x2 as usize].to_vec())
        .collect::<Vec<u8>>();

    {
        let mut custom_draw_mode = CUSTOM_DRAW_MODE.write().unwrap();
        if let Some(ref mut draw_mode) = *custom_draw_mode {
            draw_mode.draw(
                &rect_pixel_slice,
                (x2 - x1) as usize,
                (y2 - y1) as usize,
                unsafe { WINDOW_WIDTH },
                unsafe { WINDOW_HEIGHT },
                unsafe { G_WINDOW.get() },
            );
        }
    }

    {
        *DRAW_CALLED.write().unwrap() = false;
    }

    0
}

#[hook(rva = 0x00031591)]
unsafe extern "cdecl" fn set_palette(
    start: i32,
    count: i32,
    palette_colors: *const PaletteColor,
    _unknown: u32,
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
            (*G_PALETTE_COLORS.ptr())[index] = *color;
        }
    }

    let mut custom_draw_mode = CUSTOM_DRAW_MODE.write().unwrap();
    if let Some(ref mut draw_mode) = *custom_draw_mode {
        draw_mode.set_palette_6bit(unsafe { G_PALETTE_COLORS.as_ref() }.unwrap());
    }

    0
}

#[hook(rva = 0x0003162c)]
unsafe extern "stdcall" fn set_palette_with_brightness(palette_data: *mut c_void) -> i32 {
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
            (*G_PALETTE_COLORS_PRE_BRIGHTNESS.ptr())[i] = *color;
            let PaletteColor { red, green, blue } = *color;
            (*G_PALETTE_COLORS.ptr())[i] = PaletteColor {
                red: (*G_GAMMA_TABLE.ptr())[red as usize + brightness * 64],
                green: (*G_GAMMA_TABLE.ptr())[green as usize + brightness * 64],
                blue: (*G_GAMMA_TABLE.ptr())[blue as usize + brightness * 64],
            };
        }
    }

    {
        let mut custom_draw_mode = CUSTOM_DRAW_MODE.write().unwrap();
        if let Some(ref mut draw_mode) = *custom_draw_mode {
            draw_mode.set_palette(unsafe { G_PALETTE_COLORS.as_ref() }.unwrap());
        }
    }

    unsafe {
        (*G_CURRENT_PIXEL_BUFFER.get()).data = G_BITS_TO_BLIT.get();
    }

    0
}

// Leave this out for now because the vanilla function works fine
// #[hook(rva = 0x000316c9)]
// unsafe extern "stdcall" fn blend_palettes(_: *mut c_void, _: i32) -> i32 {}

#[hook(rva = 0x00031948)]
unsafe extern "stdcall" fn swap_buffers() -> i32 {
    tracing::trace!("GdiSwapBuffers called");

    unsafe {
        (*G_CURRENT_PIXEL_BUFFER.get()).data = G_BITS_TO_BLIT.get();
    }

    0
}

patches!(
    pub(in crate::shell) static PATCHES = [
        hook init_draw_mode,
        hook adjust_window_size,
        hook toggle_fullscreen,
        hook read_mouse_state,
        hook video_driver_draw_shell,
        hook begin,
        hook end,
        hook blit_flip,
        hook bit_blt_rect,
        hook stretch_blit,
        hook set_palette,
        hook set_palette_with_brightness,
        hook swap_buffers,
        patch SHOW_CURSOR_THUNK,
    ];
);
