use binding::{globals, macros::hook, patches, thunks};
use windows::{
    Win32::{Foundation::HWND, Media::timeGetTime},
    core::BOOL,
};

use crate::shell::{MODULE, drawmode::overlay_ui};

const CURSOR_GRAPHIC_SIZE: usize = 423;

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

#[derive(Clone, Debug, Default)]
pub struct OverlayMouseState {
    pub pos_x: f32,
    pub pos_y: f32,
    pub left_down: bool,
    pub right_down: bool,
    pub middle_down: bool,
}

globals!(
    pub(in crate::shell) static G_WINDOW: HWND = 0x000965ec;
    pub(in crate::shell) static G_CURRENT_MOUSE_STATE: *mut MouseState = 0x00071204;
    pub(in crate::shell) static G_CURSOR_GRAPHIC: *mut [u8; CURSOR_GRAPHIC_SIZE] = 0x00071200;
);

thunks! {
    /// `ShowCursor`, so the overlay draws the cursor instead of Windows.
    static SHOW_CURSOR_THUNK = [
        0x000995d0 => show_cursor,
    ];
}

/// translates the cursor position from the client window to the shell's coordinate system
fn cursor_window_to_shell(x: f32, y: f32, window_width: f32, window_height: f32) -> (i32, i32) {
    const SHELL_LOGICAL_WIDTH: f32 = 640.;
    const SHELL_LOGICAL_HEIGHT: f32 = 480.;

    let aspect_ratio = 4.0 / 3.0;
    let mut shell_width = window_width;
    let mut shell_height = window_height;
    if shell_width / shell_height > aspect_ratio {
        shell_width = shell_height * aspect_ratio;
    } else {
        shell_height = shell_width / aspect_ratio;
    }

    let shell_x = (x - (window_width - shell_width) / 2.0) / (shell_width / SHELL_LOGICAL_WIDTH);
    let shell_y =
        (y - (window_height - shell_height) / 2.0) / (shell_height / SHELL_LOGICAL_HEIGHT);

    (shell_x as i32, shell_y as i32)
}

/// Writes the mouse state the shell reads.
/// The position and `window_size` are in egui points.
pub fn update_global_mouse_state(mouse_state: &OverlayMouseState, window_size: [f32; 2]) {
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
    if (mouse_state.pos_x < 0.0 || mouse_state.pos_x >= window_size[0])
        || (mouse_state.pos_y < 0.0 || mouse_state.pos_y >= window_size[1])
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
            window_size[0],
            window_size[1],
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

/// Disable the game's own mouse state reading function: the overlay writes the mouse state when it draws.
/// This allows us to intercept mouse events and update the global mouse state selectively.
#[hook(rva = 0x0003aac5)]
unsafe extern "fastcall" fn read_mouse_state(_mouse_state: *mut MouseState) {
    tracing::trace!("ReadMouseState called");
}

unsafe extern "stdcall" fn show_cursor(show: BOOL) -> i32 {
    tracing::trace!("ShowCursor called with show: {}", show.0);

    overlay_ui::show_cursor(show.0 != 0);

    if show.0 == 0 { -1 } else { 1 }
}

patches!(
    pub(in crate::shell) static PATCHES = [
        hook read_mouse_state,
        patch SHOW_CURSOR_THUNK,
    ];
);
