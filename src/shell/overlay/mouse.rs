use binding::globals;
use windows::core::BOOL;

use crate::shell::MODULE;

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
    pub(in crate::shell) static G_CURRENT_MOUSE_STATE: *mut MouseState = 0x00071204;
    pub(in crate::shell) static G_CURSOR_GRAPHIC: *mut [u8; CURSOR_GRAPHIC_SIZE] = 0x00071200;
);
