use std::{
    ffi::c_int,
    sync::atomic::{AtomicBool, Ordering},
};

use mw2_sys::shared::{c_mechMouseLeft, c_mechMouseMiddle, c_mechMouseRight};
use winit::{
    dpi::PhysicalPosition,
    event::{ElementState, MouseButton},
    window::{CursorGrabMode, Window},
};

use crate::drawmode::frame_rect;

use super::{State, with};

pub struct Mouse {
    /// Where the cursor is in the window, in physical pixels
    cursor: Option<PhysicalPosition<f64>>,
    buttons: u32,
    moved: bool,
    /// The size of the frame last presented
    frame_size: Option<[usize; 2]>,
    /// Whether the game wants the cursor kept inside the frame
    grab: bool,
    /// Where the cursor is in the frame while it is grabbed
    grabbed_position: [f64; 2],
}

impl Mouse {
    pub fn new() -> Self {
        Self {
            cursor: None,
            buttons: 0,
            moved: false,
            frame_size: None,
            grab: false,
            grabbed_position: [0.0; 2],
        }
    }

    pub fn set_frame_size(&mut self, frame_size: [usize; 2]) {
        self.frame_size = Some(frame_size);
    }

    pub fn cursor_moved(&mut self, position: Option<PhysicalPosition<f64>>) {
        self.cursor = position;
        self.moved = position.is_some();
    }

    /// The `lParam` of a mouse move message, if the cursor has moved since the last call
    pub fn take_move(&mut self, window: &Window) -> Option<isize> {
        if !std::mem::take(&mut self.moved) {
            return None;
        }
        let [x, y] = self.position(window)?;
        Some(((y as u16 as isize) << 16) | x as u16 as isize)
    }

    pub fn button(&mut self, button: MouseButton, state: ElementState) {
        let bit = match button {
            MouseButton::Left => c_mechMouseLeft,
            MouseButton::Middle => c_mechMouseMiddle,
            MouseButton::Right => c_mechMouseRight,
            _ => return,
        };
        match state {
            ElementState::Pressed => self.buttons |= bit,
            ElementState::Released => self.buttons &= !bit,
        }
    }

    /// The mouse moved by `delta`
    pub fn motion(&mut self, window: &Window, delta: (f64, f64)) {
        let (Some(frame_size), true) = (self.frame_size, self.grab) else {
            return;
        };
        let Some((_, size)) = self.frame_rect(window) else {
            return;
        };

        let delta = [delta.0, delta.1];
        for axis in 0..2 {
            let position = self.grabbed_position[axis]
                + delta[axis] * frame_size[axis] as f64 / f64::from(size[axis]);
            self.grabbed_position[axis] = position.clamp(0.0, (frame_size[axis] - 1) as f64);
        }
    }

    /// The window gained or lost the focus
    pub fn focused(&mut self, window: &Window, focused: bool) {
        if !focused {
            self.buttons = 0;
        }
        if self.grab {
            apply_grab(window, focused);
        }
    }

    /// The origin and size of the frame in the window, in physical pixels
    fn frame_rect(&self, window: &Window) -> Option<([f32; 2], [f32; 2])> {
        let window_size = window.inner_size();
        frame_rect(
            [window_size.width as f32, window_size.height as f32],
            self.frame_size?,
        )
    }

    fn position(&self, window: &Window) -> Option<[i32; 2]> {
        if self.grab {
            return Some(self.grabbed_position.map(|v| v as i32));
        }

        let cursor = self.cursor?;
        let frame_size = self.frame_size?;
        let (origin, size) = self.frame_rect(window)?;
        let cursor = [cursor.x, cursor.y];
        Some(std::array::from_fn(|axis| {
            let position = (cursor[axis] - f64::from(origin[axis])) * frame_size[axis] as f64
                / f64::from(size[axis]);
            position.floor() as i32
        }))
    }

    fn set_position(&mut self, window: &Window, position: [i32; 2]) {
        if self.grab {
            self.grabbed_position = position.map(f64::from);
            return;
        }

        let (Some(frame_size), Some((origin, size))) = (self.frame_size, self.frame_rect(window))
        else {
            return;
        };
        let [x, y] = std::array::from_fn(|axis| {
            f64::from(origin[axis])
                + (f64::from(position[axis]) + 0.5) * f64::from(size[axis])
                    / frame_size[axis] as f64
        });
        let cursor = PhysicalPosition::new(x, y);
        if window.set_cursor_position(cursor).is_ok() {
            self.cursor = Some(cursor);
        }
    }

    fn set_grab(&mut self, window: &Window, grab: bool, focused: bool) {
        if grab == self.grab {
            return;
        }
        if grab {
            // Carry on from where the cursor is
            let centre = self
                .frame_size
                .map_or([0.0; 2], |size| size.map(|v| v as f64 / 2.0));
            self.grabbed_position = self
                .position(window)
                .map_or(centre, |position| position.map(f64::from));
        }
        self.grab = grab;
        apply_grab(window, grab && focused);
    }
}

fn apply_grab(window: &Window, grab: bool) {
    let result = if grab {
        window
            .set_cursor_grab(CursorGrabMode::Confined)
            .or_else(|_| window.set_cursor_grab(CursorGrabMode::Locked))
    } else {
        window.set_cursor_grab(CursorGrabMode::None)
    };
    if let Err(error) = result {
        tracing::warn!("couldn't change the cursor grab: {error}");
    }
}

static CAPTURED: AtomicBool = AtomicBool::new(false);
static HIDDEN: AtomicBool = AtomicBool::new(false);

pub fn capture_pointer(captured: bool) {
    CAPTURED.store(captured, Ordering::Relaxed);
}

pub fn cursor_hidden() -> bool {
    HIDDEN.load(Ordering::Relaxed)
}

impl State {
    fn game_has_pointer(&self) -> bool {
        self.mouse.grab
            || !(CAPTURED.load(Ordering::Relaxed) || self.egui_ctx.is_pointer_over_area())
    }
}

#[unsafe(export_name = "MechMouseGetPosition")]
pub unsafe extern "C" fn get_position(x: *mut i32, y: *mut i32) -> c_int {
    let position = with(|app| {
        let state = &app.state;
        if !state.game_has_pointer() {
            return None;
        }
        state.mouse.position(state.window.as_ref()?)
    })
    .flatten();

    let Some([position_x, position_y]) = position else {
        return 0;
    };
    unsafe {
        if let Some(x) = x.as_mut() {
            *x = position_x;
        }
        if let Some(y) = y.as_mut() {
            *y = position_y;
        }
    }
    1
}

#[unsafe(export_name = "MechMouseSetPosition")]
pub extern "C" fn set_position(x: i32, y: i32) {
    with(|app| {
        let state = &mut app.state;
        if let Some(window) = &state.window {
            state.mouse.set_position(window, [x, y]);
        }
    });
}

#[unsafe(export_name = "MechMouseButtons")]
pub extern "C" fn buttons() -> u32 {
    with(|app| {
        if app.state.game_has_pointer() {
            app.state.mouse.buttons
        } else {
            0
        }
    })
    .unwrap_or(0)
}

#[unsafe(export_name = "MechMouseGrab")]
pub extern "C" fn grab(grab: c_int) {
    with(|app| {
        let state = &mut app.state;
        if let Some(window) = &state.window {
            state.mouse.set_grab(window, grab != 0, state.focused);
        }
    });
}

#[unsafe(export_name = "MechMouseShowCursor")]
pub extern "C" fn show_cursor(show: c_int) -> c_int {
    let shown = !HIDDEN.swap(show == 0, Ordering::Relaxed);
    shown.into()
}
