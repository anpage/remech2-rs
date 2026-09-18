use std::ffi::{c_char, c_void};
use std::sync::Mutex;

use binding::{game_fns, globals};
use tracing::error;
use windows::Win32::{
    Foundation::{HWND, LPARAM, WPARAM},
    UI::WindowsAndMessaging::PostMessageA,
};

use crate::shell::drawmode::hooks::MouseState;

use super::MODULE;

pub mod main_menu;

#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub struct ShellMsg(pub u32);

impl ShellMsg {
    pub const HELP_EXIT: Self = Self(0x401);
    pub const EXIT_TO_DESKTOP: Self = Self(0x402);
    pub const TICK: Self = Self(0x404);
    pub const MISSION_BRIEFING: Self = Self(0x406);
    pub const CLAN_HALL: Self = Self(0x407);
    pub const MISSION_DEBRIEF: Self = Self(0x409);
    pub const ARCHIVES: Self = Self(0x40b);
    pub const TRIAL_SETUP: Self = Self(0x40d);
    pub const MAIN_MENU: Self = Self(0x40e);
    pub const MECHBAY: Self = Self(0x40f);
    pub const LAUNCH: Self = Self(0x410);
    pub const MISSION_SELECT: Self = Self(0x411);
    pub const PILOT_ROSTER: Self = Self(0x412);
    pub const MECH_CONFIG: Self = Self(0x413);
    pub const TRAINING: Self = Self(0x414);
    pub const LANDING: Self = Self(0x415);
    pub const FINALE: Self = Self(0x416);
    pub const SHUTDOWN: Self = Self(0x41e);
    pub const INITIAL_KICK: Self = Self(0x420);
}

#[derive(Debug, Clone, Copy, PartialEq, Eq)]
#[repr(i32)]
pub enum ShellState {
    Wolf = 0,
    JadeFalcon = 1,
    Trial = 2,
}

impl ShellState {
    pub fn from_raw(raw: i32) -> Option<Self> {
        match raw {
            0 => Some(Self::Wolf),
            1 => Some(Self::JadeFalcon),
            2 => Some(Self::Trial),
            _ => None,
        }
    }
}

/// The args that get passed to every screen callback.
pub struct ScreenArgs {
    pub db: *mut c_void,
    pub shell_state: *mut i32,
    pub state_byte: *mut u8,
    pub state_word: *mut *mut c_char,
}

impl ScreenArgs {
    pub unsafe fn shell_state(&self) -> Option<ShellState> {
        ShellState::from_raw(unsafe { *self.shell_state })
    }

    pub unsafe fn set_shell_state(&mut self, state: ShellState) {
        unsafe { *self.shell_state = state as i32 };
    }
}

pub trait Screen: Default {
    /// Posted as `wParam` alongside the exit message.
    const ID: ShellMsg;

    /// Handles one frame. Returns the message to leave with, or `None` to stay.
    fn tick(&mut self, args: &mut ScreenArgs) -> Option<ShellMsg>;

    /// Frees what the entry function allocated.
    /// Runs once, before the exit message is posted, whether the exit came from `tick` or from
    /// `ShellWindowProc`/`ShellMain`.
    fn teardown(&mut self, args: &mut ScreenArgs);
}

globals!(
    static G_SHELL_CALLBACK: *mut c_void = 0x00062978;
    static G_WND: HWND = 0x000965ec;
    pub static G_MOUSE_STATE: *mut MouseState = 0x00071204;
);

game_fns!(
    static UNREGISTER_SCREEN_FUNCTION: unsafe extern "cdecl" fn(*mut c_void) = 0x000108fd;
    pub static ALLOCATE: unsafe extern "cdecl" fn(usize) -> *mut c_void = 0x00049c60;
    pub static DEALLOCATE: unsafe extern "cdecl" fn(*mut c_void) = 0x00049c50;
    pub static GET_DB_ITEM: unsafe extern "thiscall" fn(
        *mut c_void,
        i32,
        *mut *mut c_void,
        *mut i32,
    ) -> i32 = 0x00048051;
    pub static FREE_ANIMATIONS: unsafe extern "cdecl" fn() = 0x00016f45;
);

pub unsafe fn run<S: Screen>(state: &Mutex<Option<S>>, mut args: ScreenArgs, msg: u32) {
    let mut msg = ShellMsg(msg);

    let Ok(mut guard) = state.try_lock() else {
        error!("screen {:?} re-entered with {msg:?}", S::ID);
        debug_assert!(false);
        return;
    };
    let screen = guard.get_or_insert_with(S::default);

    if msg == ShellMsg::TICK
        && let Some(next) = screen.tick(&mut args)
    {
        msg = next;
    }

    if msg != ShellMsg::TICK {
        screen.teardown(&mut args);
        *guard = None;

        unsafe {
            if let Err(e) = PostMessageA(
                Some(G_WND.get()),
                msg.0,
                WPARAM(S::ID.0 as usize),
                LPARAM(0),
            ) {
                error!("screen {:?}: posting {msg:?} failed: {e}", S::ID);
            }
            // Pass what's registered (the original address), not our own fn.
            (UNREGISTER_SCREEN_FUNCTION.get())(G_SHELL_CALLBACK.get());
        }
    }
}
