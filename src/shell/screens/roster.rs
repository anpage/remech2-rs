use std::ffi::{c_char, c_void};
use std::ptr::{null, null_mut};
use std::sync::Mutex;

use binding::{game_fns, globals, macros::hook, patches};

use super::{
    BUTTONS_DROP, BUTTONS_HIT_TEST, CLICKABLES_HIDE, CLICKABLES_HIT_TEST, CLICKABLES_SHOW,
    Campaign, Clickable, DEALLOCATE, G_CAMPAIGN_MISSIONS, G_PILOT, G_SHELL_BUTTON1, Pilot,
    SAVE_PILOTS, Screen, ScreenArgs, ShellMsg, run,
};
use crate::shell::MODULE;
use crate::shell::audio::AUDIO_SAMPLE_DROP;
use crate::shell::drawmode::hooks::G_CURRENT_MOUSE_STATE;
use crate::shell::drawmode::{confirm, menu};

globals!(
    /// The selected pilot's stats
    static G_PILOT_STATS: Clickable = 0x00063c78;
    /// The missions the pilot has flown, shown in the stats' place
    static G_MISSION_ROWS: Clickable = 0x00063dd8;
    /// Whether the mission list is the group on screen
    static G_MISSION_LIST_OPEN: i32 = 0x00064120;
    /// The one-shot sample the entry function starts
    static G_SAMPLE: *mut c_void = 0x0006411c;
    static G_BUTTONS: *mut c_void = 0x0007cda0;
    /// The ten `MW2REG.CFG` rows this clan's roster shows, indexed by button id - 1
    static G_SLOTS: [*mut Pilot; 10] = 0x0007cda8;
    /// Set for a pilot who has just been created, for the clan hall's induction speech
    static G_NEW_PILOT: i32 = 0x00071374;
);

game_fns!(
    static BUTTON_ENABLE: unsafe extern "thiscall" fn(*mut c_void, i32) = 0x00048cc1;
    static BUTTON_DISABLE: unsafe extern "thiscall" fn(*mut c_void, i32) = 0x00048d65;
    /// Clears every slot's active flag, then sets this pilot's
    static SET_ACTIVE_PILOT: unsafe extern "cdecl" fn(*mut Pilot) = 0x00014d3e;
    /// Frees the slot: clears `in_use` and the callsign, and drops the row's label
    static TERMINATE_PILOT: unsafe extern "cdecl" fn(*mut Pilot) = 0x00014caa;
    /// Rebuilds all ten callsign labels
    static REBUILD_ROSTER_LABELS: unsafe extern "cdecl" fn() = 0x00014b60;
    /// Frees all ten callsign labels
    static FREE_ROSTER_LABELS: unsafe extern "cdecl" fn() = 0x00014c1e;
    /// Edits `text` in place, running its own loop until Enter, Escape or a click:
    /// `(style, x, y, text, colour, max_chars, max_width)`
    ///
    /// Returns 0 for Escape, but writes what was typed either way, which is why the caller only looks at `text`
    static EDIT_TEXT: unsafe extern "cdecl" fn(
        *mut c_void,
        i32,
        i32,
        *mut c_char,
        i32,
        i32,
        i32,
    ) -> i32 = 0x00044451;
    static STR_UPPER: unsafe extern "cdecl" fn(*mut c_char) -> *mut c_char = 0x000309b6;
    /// Points variant registration at one of the two mech lists and overwrites its header:
    /// `(list, slot, capacity, count, limit)`, where `-1` leaves a field alone.
    static SELECT_VARIANT_LIST: unsafe extern "cdecl" fn(i32, i32, i32, i32, i32) = 0x00003175;
    /// Loads the scenario's project data: the mission filename, and with `lances` set, the two starting lances.
    /// `briefing` also plays the plan video.
    static LOAD_SCENARIO: unsafe extern "cdecl" fn(*const c_char, i32, i32) = 0x00037cf7;
    /// `(slot, filename, name)`. The roster only ever sets the name.
    static REGISTER_MECH_VARIANT: unsafe extern "cdecl" fn(
        i32,
        *const c_char,
        *const c_char,
    ) -> i32 = 0x00002de7;
    /// The shell's CRT `rand`, seeded by the entry function
    static RAND: unsafe extern "cdecl" fn() -> i32 = 0x0004aaf0;
);

/// Button ids, in the order the roster's layout table lists them
const BTN_MAIN_MENU: i32 = 0;
/// Ids `1..=SLOT_COUNT` are the roster's rows
const SLOT_COUNT: i32 = 10;
const BTN_CLAN_HALL: i32 = 0xb;
const BTN_TERMINATE: i32 = 0xc;
const BTN_SELECT_MISSION: i32 = 0xd;
const BTN_BACK: i32 = 0xe;

/// Where a row's callsign sits, matching the labels `REBUILD_ROSTER_LABELS` builds
const CALLSIGN_X: i32 = 42;
const FIRST_ROW_Y: i32 = 92;
const ROW_HEIGHT: i32 = 35;
const CALLSIGN_MAX_CHARS: i32 = 14;
const CALLSIGN_MAX_WIDTH: i32 = 300;

/// A new pilot starts somewhere in `HONOR..HONOR * 2`
const HONOR: f64 = 1000.0;
const RAND_MAX: f64 = 32767.0;

#[derive(Default)]
struct Roster {
    /// The terminate prompt is open
    confirm_pending: bool,
}

impl Screen for Roster {
    const ID: ShellMsg = ShellMsg::PILOT_ROSTER;

    fn tick(&mut self, args: &mut ScreenArgs) -> Option<ShellMsg> {
        unsafe {
            if self.confirm_pending {
                let terminate = confirm::answer()?;
                confirm::close();
                self.confirm_pending = false;
                if terminate {
                    self.terminate();
                }
                return None;
            }

            // TODO: Maybe consider bringing back quick tips

            let mouse = G_CURRENT_MOUSE_STATE.get().as_ref()?;
            let (pos_x, pos_y) = (mouse.pos_x, mouse.pos_y);
            let mut hit = (BUTTONS_HIT_TEST.get())(G_BUTTONS.get(), pos_x, pos_y);
            if mouse.left_pressed.0 != 1 {
                return None;
            }

            let mut msg = self.launch_mission(args, pos_x, pos_y);

            loop {
                match hit {
                    BTN_MAIN_MENU => {
                        G_PILOT.set(null_mut());
                        msg = Some(ShellMsg::MAIN_MENU);
                    }
                    1..=SLOT_COUNT => {
                        let pilot = G_SLOTS.get()[(hit - 1) as usize];
                        if (*pilot).in_use == 0 {
                            self.create(hit, pilot);
                            let mouse = G_CURRENT_MOUSE_STATE.get().as_ref()?;
                            if mouse.left_down.0 == 1 {
                                hit = (BUTTONS_HIT_TEST.get())(
                                    G_BUTTONS.get(),
                                    mouse.pos_x,
                                    mouse.pos_y,
                                );
                                continue;
                            }
                        } else {
                            G_PILOT.set(pilot);
                            (SET_ACTIVE_PILOT.get())(pilot);
                            if G_CURRENT_MOUSE_STATE.get().as_ref()?.double_clicked != 0 {
                                // A double click chooses the pilot
                                *args.pilot_chosen = 1;
                                msg = Some(ShellMsg::CLAN_HALL);
                            } else {
                                self.select(pilot);
                            }
                        }
                    }
                    BTN_CLAN_HALL => {
                        if !G_PILOT.get().is_null() {
                            *args.pilot_chosen = 1;
                            msg = Some(ShellMsg::CLAN_HALL);
                        }
                    }
                    BTN_TERMINATE => {
                        if !G_PILOT.get().is_null() {
                            confirm::open(&["Terminate MechWarrior?"]);
                            self.confirm_pending = true;
                        }
                    }
                    BTN_SELECT_MISSION => {
                        let buttons = G_BUTTONS.get();
                        (BUTTON_DISABLE.get())(buttons, BTN_SELECT_MISSION);
                        (BUTTON_ENABLE.get())(buttons, BTN_BACK);
                        (CLICKABLES_HIDE.get())(G_PILOT_STATS.ptr());
                        (CLICKABLES_SHOW.get())(G_MISSION_ROWS.ptr());
                        G_MISSION_LIST_OPEN.set(1);
                    }
                    BTN_BACK => {
                        let buttons = G_BUTTONS.get();
                        // Only reachable with the list open, which needs a pilot.
                        if let Some(pilot) = G_PILOT.get().as_ref()
                            && pilot.mission != 0
                        {
                            (BUTTON_ENABLE.get())(buttons, BTN_SELECT_MISSION);
                        }
                        (BUTTON_DISABLE.get())(buttons, BTN_BACK);
                        (CLICKABLES_HIDE.get())(G_MISSION_ROWS.ptr());
                        G_MISSION_LIST_OPEN.set(0);
                        (CLICKABLES_SHOW.get())(G_PILOT_STATS.ptr());
                    }
                    _ => {}
                }
                return msg;
            }
        }
    }

    fn teardown(&mut self, _args: &mut ScreenArgs) {
        unsafe {
            if self.confirm_pending {
                confirm::close();
            }

            (CLICKABLES_HIDE.get())(G_MISSION_ROWS.ptr());
            (CLICKABLES_HIDE.get())(G_PILOT_STATS.ptr());
            (SAVE_PILOTS.get())();

            let buttons = G_BUTTONS.get();
            if !buttons.is_null() {
                (BUTTONS_DROP.get())(buttons);
                (DEALLOCATE.get())(buttons);
            }
            G_BUTTONS.set(null_mut());

            let sample = G_SAMPLE.get();
            if !sample.is_null() {
                (AUDIO_SAMPLE_DROP.get())(sample);
                (DEALLOCATE.get())(sample);
            }
            G_SAMPLE.set(null_mut());

            (FREE_ROSTER_LABELS.get())();
        }
    }
}

impl Roster {
    /// Replays a mission the pilot has already flown, if the list is open and a row was clicked
    unsafe fn launch_mission(&self, args: &mut ScreenArgs, x: i32, y: i32) -> Option<ShellMsg> {
        unsafe {
            if G_MISSION_LIST_OPEN.get() == 0 {
                return None;
            }
            let Some(clan @ (Campaign::Wolf | Campaign::JadeFalcon)) = args.campaign() else {
                return None;
            };
            let pilot = G_PILOT.get().as_ref()?;
            let row = (CLICKABLES_HIT_TEST.get())(G_MISSION_ROWS.ptr(), x, y).as_ref()?;
            if row.index() >= pilot.mission {
                return None;
            }

            let campaign = G_CAMPAIGN_MISSIONS.get()[clan as usize];
            let scenario = (*campaign.add(row.index() as usize)).scenario;
            *args.scenario = scenario;

            (SELECT_VARIANT_LIST.get())(0, 0, 3, 1, 100);
            (LOAD_SCENARIO.get())(scenario, 1, 0);
            (SELECT_VARIANT_LIST.get())(1, 0, 0, 0, 100);
            (SELECT_VARIANT_LIST.get())(0, -1, -1, -1, -1);
            (REGISTER_MECH_VARIANT.get())(0, null(), pilot.callsign.as_ptr());
            Some(ShellMsg::LAUNCH)
        }
    }

    /// Shows the selected pilot's stats and enables the stuff that need a pilot selected
    unsafe fn select(&self, pilot: *mut Pilot) {
        unsafe {
            let buttons = G_BUTTONS.get();
            (BUTTON_ENABLE.get())(buttons, BTN_CLAN_HALL);
            (BUTTON_ENABLE.get())(buttons, BTN_TERMINATE);
            // Nothing to replay until the pilot has finished a mission.
            if (*pilot).mission == 0 {
                (BUTTON_DISABLE.get())(buttons, BTN_SELECT_MISSION);
            } else {
                (BUTTON_ENABLE.get())(buttons, BTN_SELECT_MISSION);
            }
            (BUTTON_DISABLE.get())(buttons, BTN_BACK);

            (CLICKABLES_HIDE.get())(G_MISSION_ROWS.ptr());
            G_MISSION_LIST_OPEN.set(0);
            (CLICKABLES_HIDE.get())(G_PILOT_STATS.ptr());
            (CLICKABLES_SHOW.get())(G_PILOT_STATS.ptr());
        }
    }

    /// Names a new pilot in the empty slot that was clicked. Leaves the slot alone if the name comes back empty.
    unsafe fn create(&self, button: i32, pilot: *mut Pilot) {
        unsafe {
            let buttons = G_BUTTONS.get();
            for id in [BTN_CLAN_HALL, BTN_TERMINATE, BTN_SELECT_MISSION, BTN_BACK] {
                (BUTTON_DISABLE.get())(buttons, id);
            }
            (CLICKABLES_HIDE.get())(G_MISSION_ROWS.ptr());
            G_MISSION_LIST_OPEN.set(0);
            (CLICKABLES_HIDE.get())(G_PILOT_STATS.ptr());
            G_PILOT.set(null_mut());

            let callsign = (*pilot).callsign.as_mut_ptr();
            *callsign = 0;
            // Pumps the shell's loop itself, so the screen is frozen until this returns
            {
                let _menu = menu::lock();
                (EDIT_TEXT.get())(
                    G_SHELL_BUTTON1.get(),
                    CALLSIGN_X,
                    (button - 1) * ROW_HEIGHT + FIRST_ROW_Y,
                    callsign,
                    0,
                    CALLSIGN_MAX_CHARS,
                    CALLSIGN_MAX_WIDTH,
                );
            }
            (STR_UPPER.get())(callsign);
            if *callsign == 0 {
                return;
            }

            G_PILOT.set(pilot);
            let pilot = &mut *pilot;
            pilot.in_use = 1;
            pilot.mission = 0;
            pilot.rank = 0;
            pilot.honor = (HONOR + f64::from((RAND.get())()) / RAND_MAX * HONOR) as i32;
            pilot.unknown = [0; 4];

            (SET_ACTIVE_PILOT.get())(pilot);
            (REBUILD_ROSTER_LABELS.get())();
            (BUTTON_ENABLE.get())(buttons, BTN_CLAN_HALL);
            (BUTTON_ENABLE.get())(buttons, BTN_TERMINATE);
            (CLICKABLES_SHOW.get())(G_PILOT_STATS.ptr());
            G_NEW_PILOT.set(1);
        }
    }

    /// Frees the selected pilot's slot and everything that hung off the selection
    unsafe fn terminate(&self) {
        unsafe {
            (TERMINATE_PILOT.get())(G_PILOT.get());
            G_NEW_PILOT.set(0);
            (CLICKABLES_HIDE.get())(G_PILOT_STATS.ptr());
            (CLICKABLES_HIDE.get())(G_MISSION_ROWS.ptr());
            G_MISSION_LIST_OPEN.set(0);
            G_PILOT.set(null_mut());

            let buttons = G_BUTTONS.get();
            for id in [BTN_CLAN_HALL, BTN_TERMINATE, BTN_SELECT_MISSION, BTN_BACK] {
                (BUTTON_DISABLE.get())(buttons, id);
            }
        }
    }
}

static STATE: Mutex<Option<Roster>> = Mutex::new(None);

#[hook(rva = 0x0001534c)]
unsafe extern "cdecl" fn roster(
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
        hook roster,
    ];
);
