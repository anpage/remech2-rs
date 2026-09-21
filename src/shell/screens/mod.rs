use std::ffi::{c_char, c_void};
use std::sync::Mutex;

use binding::{game_fns, globals};
use tracing::error;
use windows::Win32::{
    Foundation::{LPARAM, WPARAM},
    UI::WindowsAndMessaging::PostMessageA,
};

use crate::shell::drawmode::hooks::G_WINDOW;

use super::MODULE;

pub mod debrief;
pub mod debug;
pub mod main_menu;
pub mod settings;

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

/// A screen's button table, one per clan.
#[repr(C)]
#[derive(Clone, Copy)]
pub struct ScreenLayout {
    pub table: *const ScreenButton,
    pub count: i32,
    /// LZ-compressed backdrop, drawn with its own palette
    pub backdrop_item: i32,
    unknown: i32,
}

/// One row of a [`ScreenLayout`]'s table, which the button manager turns into a button.
#[repr(C)]
#[derive(Clone, Copy)]
pub struct ScreenButton {
    left: i32,
    top: i32,
    right: i32,
    bottom: i32,
    label_x: i32,
    label_y: i32,
    /// A leading `<` tells the button manager to build a label graphic.
    /// Without one nothing is drawn, as on the main menu, where the text is part of the backdrop.
    ///
    /// A `~` after the `<` centers the label on `label_x` rather than starting there.
    label: *const c_char,
}

/// One row of a clickable group: a rect, the graphic drawn in it, a builder, a click handler and a
/// payload. A group is an array of these, running to a row with a negative `left`. The layout pass
/// writes back into the array, so it's both the table and the live state.
///
/// Not to be confused with [`ScreenButton`], which is the button manager's read-only table row.
/// The settings screen's options are `Clickable`s too: its `resolution_label` is a `build` and its
/// `resolution_toggle` an `on_click`.
#[repr(C)]
pub struct Clickable {
    pub left: i32,
    /// Negative for a row placed relative to the one above it
    pub top: i32,
    pub width: i32,
    pub height: i32,
    unknown: [u8; 8],
    /// What `build` made, freed when the group is hidden
    graphic: *mut c_void,
    /// Builds `graphic` from the row
    build: *mut c_void,
    /// Runs when the row is clicked, and null on display-only rows, which the hit test skips. The
    /// roster's mission rows park an empty stub here just to be clickable.
    on_click: *mut c_void,
    /// Per-group payload; read it through [`Clickable::index`] or [`Clickable::buffer`]
    value: *mut c_void,
    unknown2: u32,
}

const _: () = assert!(size_of::<Clickable>() == 0x2c);

impl Clickable {
    /// The payload as an index, e.g. the mission a roster list row replays.
    pub fn index(&self) -> i32 {
        self.value as usize as i32
    }

    /// The payload as a pointer to what the row edits, e.g. a settings value buffer.
    pub fn buffer<T>(&self) -> *mut T {
        self.value.cast()
    }
}

globals!(
    static G_SHELL_CALLBACK: *mut c_void = 0x00062978;
    /// The active pilot, or null when none is selected
    pub static G_PILOT: *mut Pilot = 0x00071370;
    /// Zeroed when there's no `MW2MSN.CFG`
    pub static G_MISSION_RESULTS: MissionResults = 0x000780e0;
    static G_CAMPAIGN_MISSIONS: [*const Mission; 2] = 0x0006fdd0;
    /// The style text graphics are built with
    pub static G_SHELL_BUTTON1: *mut c_void = 0x00071214;
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
    /// Writes `MW2REG.CFG`
    static SAVE_PILOTS: unsafe extern "cdecl" fn() = 0x0002dbec;
    /// Frees a clickable group's label graphics
    pub static CLICKABLES_HIDE: unsafe extern "cdecl" fn(*mut Clickable) = 0x00007ac8;
    /// Lays a clickable group out and builds its label graphics
    pub static CLICKABLES_SHOW: unsafe extern "cdecl" fn(*mut Clickable) = 0x000078cd;
    /// The clickable row under the cursor, or null. Skips rows without an `on_click`.
    pub static CLICKABLES_HIT_TEST: unsafe extern "cdecl" fn(
        *mut Clickable,
        i32,
        i32,
    ) -> *mut Clickable = 0x0000b5ed;
    /// Builds a text graphic and hands it to the video driver's `collection_2`
    pub static SHELL_LABEL_NEW: unsafe extern "thiscall" fn(
        *mut c_void,
        i32,
        i32,
        *const c_char,
        u32,
    ) -> *mut *mut c_void = 0x0000544e;
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
                Some(G_WINDOW.get()),
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
