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

pub mod debrief;
pub mod debug;
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
    pub db: *mut c_void,
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

/// The result of a single objective as the sim reported it
#[repr(C)]
pub struct Objective {
    /// `0` failed, `1` successful
    pub status: i32,
    /// `0` default, `1` primary, `2` secondary, `4` tertiary, `8` return
    pub kind: i32,
    unknown1: i32,
    /// Seconds, or negative when the objective was never reached
    pub time: i32,
    unknown2: i32,
    pub name: [c_char; 32],
}

/// `MW2MSN.CFG`, written by the sim and read by the debrief screen's entry function.
#[repr(C)]
pub struct MissionResults {
    /// `MW2M`
    pub magic: [c_char; 4],
    /// How many `objectives` the sim filled in.
    pub objective_count: i32,
    unknown: [i32; 2],
    pub outcome: i32,
    pub objectives: [Objective; 48],
}

impl MissionResults {
    pub const SUCCESS: i32 = 2;
    pub const FAILED: i32 = 3;
}

/// The whole file
const _: () = assert!(size_of::<MissionResults>() == 0x9d4);

/// One of the 20 pilot records in `MW2REG.CFG`.
#[repr(C)]
#[derive(Clone, Copy)]
pub struct Pilot {
    /// `1` once the slot holds a pilot
    pub in_use: i32,
    /// The pilot the roster resumes with
    pub is_active: i32,
    pub clan: i32,
    pub mission: i32,
    /// Capped at 8
    pub rank: i32,
    pub honor: i32,
    unknown: [i32; 4],
    /// Up to 14 characters, typed in on the roster screen
    pub callsign: [c_char; 16],
    /// The roster's label graphic for `callsign`, live only while the roster is up.
    /// `SavePilotRoster` writes it to `MW2REG.CFG` as junk and `LoadPilotRoster` zeroes it again.
    label: *mut c_void,
}

const _: () = assert!(size_of::<Pilot>() == 0x3c);

/// One row of a clan's campaign table.
#[repr(C, packed(1))]
#[derive(Clone, Copy)]
struct Mission {
    /// e.g. `yellSCN1`. Handed to mission select through `scenario`
    scenario: *mut c_char,
    /// Set for the campaigns' Trials of Position
    is_trial: u8,
    /// e.g. `Pyre Light`.
    title: *const c_char,
}

/// Missions in each clan's campaign.
const CAMPAIGN_LENGTH: i32 = 16;

globals!(
    static G_SHELL_CALLBACK: *mut c_void = 0x00062978;
    static G_WND: HWND = 0x000965ec;
    pub static G_MOUSE_STATE: *mut MouseState = 0x00071204;
    /// The active pilot, or null when none is selected
    pub static G_PILOT: *mut Pilot = 0x00071370;
    /// Zeroed when there's no `MW2MSN.CFG`
    pub static G_MISSION_RESULTS: MissionResults = 0x000780e0;
    static G_CAMPAIGN_MISSIONS: [*const Mission; 2] = 0x0006fdd0;
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
    static BUTTONS_HIT_TEST: unsafe extern "thiscall" fn(*mut c_void, i32, i32) -> i32 = 0x000489e9;
    static BUTTONS_DROP: unsafe extern "fastcall" fn(*mut c_void) = 0x0004883e;
    static AUDIO_SAMPLE_DROP: unsafe extern "thiscall" fn(*mut c_void) = 0x0003d50f;
    /// Writes `MW2REG.CFG`
    static SAVE_PILOTS: unsafe extern "cdecl" fn() = 0x0002dbec;
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
