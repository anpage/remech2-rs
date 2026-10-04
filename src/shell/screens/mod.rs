use std::ffi::{c_char, c_int, c_void};
use std::sync::Mutex;

use mw2_sys::shell::{self, ScreenField, TMPackDataBase};
use tracing::error;

use crate::messages;

pub mod debrief;
pub mod debug;
pub mod main_menu;
pub mod mechlab;
pub mod roster;
pub mod settings;

#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub struct ShellMsg(pub u32);

impl ShellMsg {
    pub const _HELP_EXIT: Self = Self(0x401);
    pub const EXIT_TO_DESKTOP: Self = Self(0x402);
    pub const TICK: Self = Self(0x404);
    pub const MISSION_BRIEFING: Self = Self(0x406);
    pub const CLAN_HALL: Self = Self(0x407);
    pub const MISSION_DEBRIEF: Self = Self(0x409);
    pub const _ARCHIVES: Self = Self(0x40b);
    pub const TRIAL_SETUP: Self = Self(0x40d);
    pub const MAIN_MENU: Self = Self(0x40e);
    pub const MECHBAY: Self = Self(0x40f);
    pub const LAUNCH: Self = Self(0x410);
    pub const MISSION_SELECT: Self = Self(0x411);
    pub const PILOT_ROSTER: Self = Self(0x412);
    pub const MECH_CONFIG: Self = Self(0x413);
    pub const _TRAINING: Self = Self(0x414);
    pub const LANDING: Self = Self(0x415);
    pub const FINALE: Self = Self(0x416);
    pub const _SHUTDOWN: Self = Self(0x41e);
    pub const _INITIAL_KICK: Self = Self(0x420);
}

#[derive(Debug, Clone, Copy, PartialEq, Eq)]
#[repr(i32)]
pub enum Campaign {
    Wolf = 0,
    JadeFalcon = 1,
    /// Not to be confused with the trial missions inside each clan's campaign
    TrialsOfGrievance = 2,
}

impl Campaign {
    pub fn from_raw(raw: i32) -> Option<Self> {
        match raw {
            0 => Some(Self::Wolf),
            1 => Some(Self::JadeFalcon),
            2 => Some(Self::TrialsOfGrievance),
            _ => None,
        }
    }
}

/// The args that get passed to every screen callback.
pub struct ScreenArgs {
    pub db: *mut TMPackDataBase,
    pub campaign: *mut i32,
    pub pilot_chosen: *mut u8,
    pub scenario: *mut *mut c_char,
}

impl ScreenArgs {
    pub unsafe fn campaign(&self) -> Option<Campaign> {
        Campaign::from_raw(unsafe { *self.campaign })
    }

    pub unsafe fn set_campaign(&mut self, campaign: Campaign) {
        unsafe { *self.campaign = campaign as i32 };
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

/// `MissionResults::m_outcome` of a won mission
pub const OUTCOME_SUCCESS: i32 = 2;
/// `MissionResults::m_outcome` of a lost mission
pub const OUTCOME_FAILED: i32 = 3;

/// Missions in each clan's campaign.
const CAMPAIGN_LENGTH: i32 = 16;

unsafe extern "C" {
    fn malloc(size: usize) -> *mut c_void;
    fn free(ptr: *mut c_void);
    /// The C library's, which the shell seeds
    fn rand() -> c_int;
}

unsafe fn allocate<T>() -> *mut T {
    unsafe { malloc(size_of::<T>()).cast() }
}

unsafe fn delete<T>(object: *mut *mut T, destructor: unsafe extern "thiscall" fn(*mut T)) {
    let ptr = unsafe { object.replace(std::ptr::null_mut()) };
    if !ptr.is_null() {
        unsafe {
            destructor(ptr);
            free(ptr.cast());
        }
    }
}

fn field_index(field: &ScreenField) -> i32 {
    field.m_data as usize as i32
}

unsafe fn click_field(field: *mut ScreenField) {
    unsafe {
        if let Some(click) = (*field).m_click {
            click(field);
        }
    }
}

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

        messages::post(msg.0, S::ID.0 as usize, 0);
        unsafe { shell::UnregisterScreenFunction(shell::g_screenFunction) };
    }
}
