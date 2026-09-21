use std::ffi::{c_char, c_void};
use std::ptr::null_mut;
use std::sync::Mutex;

use binding::{game_fns, globals, macros::hook, patches};

use super::{
    ALLOCATE, BUTTONS_DROP, BUTTONS_HIT_TEST, Campaign, DEALLOCATE, G_MISSION_RESULTS, G_PILOT,
    MissionResults, Pilot, Screen, ScreenArgs, ShellMsg, run,
};
use crate::shell::MODULE;
use crate::shell::drawmode::confirm;
use crate::shell::drawmode::hooks::G_CURRENT_MOUSE_STATE;
use crate::shell::screens::{
    CAMPAIGN_LENGTH, G_CAMPAIGN_MISSIONS, SAVE_PILOTS, ScreenButton, ScreenLayout,
};

globals!(
    static G_BUTTONS: *mut c_void = 0x0005b040;
    /// The first page of the report
    static G_PAGE: *mut c_void = 0x0005b044;
    /// Every page of the report
    static G_PAGES: *mut c_void = 0x0005b048;
    /// The aftermath viewer, while it's open
    static G_AFTERMATH: *mut c_void = 0x0005b04c;
    static G_DEBRIEF_LAYOUTS: [ScreenLayout; 3] = 0x0006ff00;
    static G_AFTERMATH_LAYOUTS: [ScreenLayout; 3] = 0x0006ff30;
    static G_VIDEO_DRIVER: *mut c_void = 0x00071208;
    static G_SHELL_SELECTED: *mut c_void = 0x0007120c;
    static G_SHELL_INACTIVE: *mut c_void = 0x00071224;
    /// The pilot as it was before the entry function applied this mission's results
    static G_PILOT_SNAPSHOT: Pilot = 0x00077fa0;
);

game_fns!(
    static BUTTONS_NEW: unsafe extern "thiscall" fn(
        *mut c_void,
        *mut c_void,
        *mut c_void,
        i32,
        *const ScreenButton,
        i32,
    ) -> *mut c_void = 0x000485f0;
    static PAGE_TICK: unsafe extern "thiscall" fn(*mut c_void) = 0x00045a2b;
    static PAGE_STOP: unsafe extern "thiscall" fn(*mut c_void) = 0x00045ab0;
    static PAGE_START: unsafe extern "thiscall" fn(*mut c_void) = 0x0004596f;
    static PAGE_DROP: unsafe extern "thiscall" fn(*mut c_void) = 0x00045be7;
    /// The archives screen's page viewer:
    /// `(this, archive, style, page, open_archive, db, pages, table, count)`.
    static ARCHIVE_VIEWER_NEW: unsafe extern "thiscall" fn(
        *mut c_void,
        *const c_char,
        *mut c_void,
        i32,
        u8,
        *mut c_void,
        *mut c_void,
        *const ScreenButton,
        i32,
    ) -> *mut c_void = 0x0002a490;
    static ARCHIVE_VIEWER_TICK: unsafe extern "thiscall" fn(*mut c_void) -> i32 = 0x00029ade;
    static ARCHIVE_VIEWER_DROP: unsafe extern "thiscall" fn(*mut c_void) = 0x0002a7b0;
    /// Empties both sprite lists. `1` frees the sprites, `0` only stops them.
    static VIDEO_DRIVER_CLEAR_SPRITES: unsafe extern "thiscall" fn(*mut c_void, u8) = 0x000077b4;
);

/// The viewer's tick returns this to stay open. It's the archives screen's id.
const VIEWER_STAY: i32 = 0x40b;

/// Returns the scenario of the mission the pilot is on so that the debug menu can jump to this screen.
pub(super) unsafe fn current_scenario(campaign: Campaign) -> Option<*mut c_char> {
    unsafe {
        // Trials of Grievance mode has no campaign and never reaches the debrief.
        let missions = match campaign {
            Campaign::Wolf | Campaign::JadeFalcon => G_CAMPAIGN_MISSIONS.get()[campaign as usize],
            Campaign::TrialsOfGrievance => return None,
        };
        let pilot = G_PILOT.get().as_ref()?;
        let mission = pilot.mission.clamp(0, CAMPAIGN_LENGTH - 1) as usize;
        Some((*missions.add(mission)).scenario)
    }
}

unsafe fn outcome() -> i32 {
    unsafe { (*G_MISSION_RESULTS.ptr()).outcome }
}

#[derive(Default)]
struct Debrief {
    /// The replay prompt is open
    confirm_pending: bool,
}

impl Screen for Debrief {
    const ID: ShellMsg = ShellMsg::MISSION_DEBRIEF;

    fn tick(&mut self, args: &mut ScreenArgs) -> Option<ShellMsg> {
        unsafe {
            let viewer = G_AFTERMATH.get();
            if !viewer.is_null() {
                return self.tick_aftermath(viewer, args);
            }

            (PAGE_TICK.get())(G_PAGE.get());

            if self.confirm_pending {
                // The original blocked in `ShowDialog` here.
                let replay = confirm::answer()?;
                confirm::close();
                self.confirm_pending = false;
                if !replay {
                    return None;
                }
                *G_PILOT.get() = G_PILOT_SNAPSHOT.get();
                (SAVE_PILOTS.get())();
                return Some(ShellMsg::MISSION_BRIEFING);
            }

            let mouse = G_CURRENT_MOUSE_STATE.get().as_ref()?;

            let hit = (BUTTONS_HIT_TEST.get())(G_BUTTONS.get(), mouse.pos_x, mouse.pos_y);
            if mouse.left_pressed.0 != 1 {
                return None;
            }

            match hit {
                // EXIT
                0 => Some(self.next_mission(args)),
                // AFTERMATH
                1 => {
                    self.open_aftermath(args.campaign()?);
                    None
                }
                // REPLAY: replaying the missing undoes the success, so ask first
                2 => {
                    if outcome() != MissionResults::SUCCESS {
                        return Some(ShellMsg::MISSION_BRIEFING);
                    }
                    confirm::open("Are you Sure?", &[]);
                    self.confirm_pending = true;
                    None
                }
                _ => None,
            }
        }
    }

    fn teardown(&mut self, _args: &mut ScreenArgs) {
        unsafe {
            // `ShellWindowProc` can tear the screen down under an open prompt.
            if self.confirm_pending {
                confirm::close();
            }

            let page = G_PAGE.get();
            if !page.is_null() {
                (PAGE_DROP.get())(page);
                (DEALLOCATE.get())(page);
            }
            G_PAGE.set(null_mut());

            let buttons = G_BUTTONS.get();
            if !buttons.is_null() {
                (BUTTONS_DROP.get())(buttons);
                (DEALLOCATE.get())(buttons);
            }
            G_BUTTONS.set(null_mut());

            let viewer = G_AFTERMATH.get();
            if !viewer.is_null() {
                (ARCHIVE_VIEWER_DROP.get())(viewer);
                (DEALLOCATE.get())(viewer);
            }
            G_AFTERMATH.set(null_mut());

            (VIDEO_DRIVER_CLEAR_SPRITES.get())(G_VIDEO_DRIVER.get(), 1);
        }
    }
}

impl Debrief {
    /// Moves to the next mission if the current one was won, or to the finale if it was the last one.
    unsafe fn next_mission(&self, args: &mut ScreenArgs) -> ShellMsg {
        unsafe {
            let Some(clan @ (Campaign::Wolf | Campaign::JadeFalcon)) = args.campaign() else {
                return ShellMsg::MISSION_SELECT;
            };
            if outcome() != MissionResults::SUCCESS {
                return ShellMsg::MISSION_SELECT;
            }

            let mission = (*G_PILOT.get()).mission;
            if mission >= CAMPAIGN_LENGTH {
                return ShellMsg::FINALE;
            }
            let campaign = G_CAMPAIGN_MISSIONS.get()[clan as usize];
            *args.scenario = (*campaign.add(mission as usize)).scenario;
            ShellMsg::MISSION_SELECT
        }
    }

    /// Swaps the debrief's buttons for the aftermath viewer's.
    unsafe fn open_aftermath(&self, state: Campaign) {
        unsafe {
            (PAGE_STOP.get())(G_PAGE.get());

            let buttons = G_BUTTONS.get();
            if !buttons.is_null() {
                (BUTTONS_DROP.get())(buttons);
                (DEALLOCATE.get())(buttons);
            }
            // The original leaves this dangling until the viewer closes.
            G_BUTTONS.set(null_mut());

            let layout = G_AFTERMATH_LAYOUTS.get()[state as usize];
            let viewer = (ALLOCATE.get())(0x1b9);
            if viewer.is_null() {
                return;
            }
            let viewer = (ARCHIVE_VIEWER_NEW.get())(
                viewer,
                c"".as_ptr(),
                G_SHELL_INACTIVE.get(),
                -1,
                0,
                null_mut(),
                G_PAGES.get(),
                layout.table,
                layout.count,
            );
            G_AFTERMATH.set(viewer);
        }
    }

    unsafe fn tick_aftermath(
        &self,
        viewer: *mut c_void,
        args: &mut ScreenArgs,
    ) -> Option<ShellMsg> {
        unsafe {
            let state = args.campaign()?;
            let next = (ARCHIVE_VIEWER_TICK.get())(viewer);
            if next == VIEWER_STAY {
                return None;
            }

            (ARCHIVE_VIEWER_DROP.get())(viewer);
            (DEALLOCATE.get())(viewer);
            G_AFTERMATH.set(null_mut());

            let next = ShellMsg(next as u32);
            if next == ShellMsg::EXIT_TO_DESKTOP || next == ShellMsg::MAIN_MENU {
                return Some(next);
            }

            let layout = G_DEBRIEF_LAYOUTS.get()[state as usize];
            let buttons = (ALLOCATE.get())(0x10d);
            if !buttons.is_null() {
                let buttons = (BUTTONS_NEW.get())(
                    buttons,
                    G_VIDEO_DRIVER.get(),
                    G_SHELL_SELECTED.get(),
                    0,
                    layout.table,
                    layout.count,
                );
                G_BUTTONS.set(buttons);
            }
            (PAGE_START.get())(G_PAGE.get());
            None
        }
    }
}

static STATE: Mutex<Option<Debrief>> = Mutex::new(None);

#[hook(rva = 0x0000287f)]
unsafe extern "cdecl" fn debrief(
    db: *mut c_void,
    campaign: *mut i32,
    pilot_chosen: *mut u8,
    scenario: *mut *mut c_char,
    msg: u32,
) {
    unsafe {
        run(
            &STATE,
            ScreenArgs {
                db,
                campaign,
                pilot_chosen,
                scenario,
            },
            msg,
        );
    }
}

patches!(
    pub(in crate::shell) static PATCHES = [
        hook debrief,
    ];
);
