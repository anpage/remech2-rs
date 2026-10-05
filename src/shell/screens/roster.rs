use std::ffi::c_char;
use std::ptr::null_mut;
use std::sync::Mutex;

use remech2_sys::shell::{self, PilotRecord, ScreenField, TMPackDataBase};

use super::{Campaign, Screen, ScreenArgs, ShellMsg, delete, field_index, run};
use crate::mech_rand::rand;
use crate::shell::overlay::{confirm, menu};

/// Button ids, in the order the roster's layout table lists them
const BTN_MAIN_MENU: i32 = 0;
/// Ids `1..=SLOT_COUNT` are the roster's rows
const SLOT_COUNT: i32 = 10;
const BTN_CLAN_HALL: i32 = 0xb;
const BTN_TERMINATE: i32 = 0xc;
const BTN_SELECT_MISSION: i32 = 0xd;
const BTN_BACK: i32 = 0xe;

/// Where a row's callsign sits, matching the labels `ShowPilotCallsigns` builds
const CALLSIGN_X: i32 = 42;
const FIRST_ROW_Y: i32 = 92;
const ROW_HEIGHT: i32 = 35;
const CALLSIGN_MAX_CHARS: i32 = 14;
const CALLSIGN_MAX_WIDTH: i32 = 300;

/// A new pilot starts somewhere in `HONOR..HONOR * 2`
const HONOR: f64 = 1000.0;
const RAND_MAX: f64 = remech2_sys::shared::MECH_RAND_MAX as f64;

/// The selected pilot's stats
fn pilot_stats() -> *mut ScreenField {
    (&raw mut shell::g_pilotRecordFields).cast()
}

/// The missions the pilot has flown, shown in the stats' place
fn mission_rows() -> *mut ScreenField {
    (&raw mut shell::g_missionListFields).cast()
}

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

            let mouse = shell::g_mouseState.as_ref()?;
            let (pos_x, pos_y) = (mouse.m_x, mouse.m_y);
            let mut hit = shell::ButtonMenu_HitTest(shell::g_rosterMenu, pos_x, pos_y);
            if mouse.m_leftPressed != 1 {
                return None;
            }

            let mut msg = self.launch_mission(args, pos_x, pos_y);

            loop {
                match hit {
                    BTN_MAIN_MENU => {
                        shell::g_currentPilot = null_mut();
                        msg = Some(ShellMsg::MAIN_MENU);
                    }
                    1..=SLOT_COUNT => {
                        let pilot = shell::g_rosterPilots[(hit - 1) as usize];
                        if (*pilot).m_inUse == 0 {
                            self.create(hit, pilot);
                            let mouse = shell::g_mouseState.as_ref()?;
                            if mouse.m_leftDown == 1 {
                                hit = shell::ButtonMenu_HitTest(
                                    shell::g_rosterMenu,
                                    mouse.m_x,
                                    mouse.m_y,
                                );
                                continue;
                            }
                        } else {
                            shell::g_currentPilot = pilot;
                            shell::SetActivePilot(pilot);
                            if shell::g_mouseState.as_ref()?.m_doubleClicked != 0 {
                                // A double click chooses the pilot
                                *args.pilot_chosen = 1;
                                msg = Some(ShellMsg::CLAN_HALL);
                            } else {
                                self.select(pilot);
                            }
                        }
                    }
                    BTN_CLAN_HALL => {
                        if !shell::g_currentPilot.is_null() {
                            *args.pilot_chosen = 1;
                            msg = Some(ShellMsg::CLAN_HALL);
                        }
                    }
                    BTN_TERMINATE => {
                        if !shell::g_currentPilot.is_null() {
                            confirm::open(&["Terminate MechWarrior?"]);
                            self.confirm_pending = true;
                        }
                    }
                    BTN_SELECT_MISSION => {
                        let buttons = shell::g_rosterMenu;
                        shell::ButtonMenu_DisableButton(buttons, BTN_SELECT_MISSION);
                        shell::ButtonMenu_EnableButton(buttons, BTN_BACK);
                        shell::HideFields(pilot_stats());
                        shell::ShowFields(mission_rows());
                        shell::g_missionListShown = 1;
                    }
                    BTN_BACK => {
                        let buttons = shell::g_rosterMenu;
                        // Only reachable with the list open, which needs a pilot.
                        if let Some(pilot) = shell::g_currentPilot.as_ref()
                            && pilot.m_mission != 0
                        {
                            shell::ButtonMenu_EnableButton(buttons, BTN_SELECT_MISSION);
                        }
                        shell::ButtonMenu_DisableButton(buttons, BTN_BACK);
                        shell::HideFields(mission_rows());
                        shell::g_missionListShown = 0;
                        shell::ShowFields(pilot_stats());
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

            shell::HideFields(mission_rows());
            shell::HideFields(pilot_stats());
            shell::SavePilotRoster();

            delete(
                &raw mut shell::g_rosterMenu,
                shell::ButtonMenu_ButtonMenu_destructor,
            );
            delete(
                &raw mut shell::g_rosterSound,
                shell::AudioSample_AudioSample_destructor,
            );

            shell::HidePilotCallsigns();
        }
    }
}

impl Roster {
    /// Replays a mission the pilot has already flown, if the list is open and a row was clicked
    unsafe fn launch_mission(&self, args: &mut ScreenArgs, x: i32, y: i32) -> Option<ShellMsg> {
        unsafe {
            if shell::g_missionListShown == 0 {
                return None;
            }
            let Some(clan @ (Campaign::Wolf | Campaign::JadeFalcon)) = args.campaign() else {
                return None;
            };
            let pilot = shell::g_currentPilot.as_mut()?;
            let row = shell::FindFieldAt(mission_rows(), x, y).as_ref()?;
            if field_index(row) >= pilot.m_mission {
                return None;
            }

            let campaign = shell::g_campaignMissions[clan as usize];
            let scenario = (*campaign.add(field_index(row) as usize)).m_scenario;
            *args.scenario = scenario;

            shell::SelectStar(0, 0, 3, 1, 100);
            shell::ShellApplyMissionUiInfo(scenario, 1, 0);
            shell::SelectStar(1, 0, 0, 0, 100);
            shell::SelectStar(0, -1, -1, -1, -1);
            shell::SetStarMech(0, null_mut(), pilot.m_callsign.as_mut_ptr());
            Some(ShellMsg::LAUNCH)
        }
    }

    /// Shows the selected pilot's stats and enables the stuff that need a pilot selected
    unsafe fn select(&self, pilot: *mut PilotRecord) {
        unsafe {
            let buttons = shell::g_rosterMenu;
            shell::ButtonMenu_EnableButton(buttons, BTN_CLAN_HALL);
            shell::ButtonMenu_EnableButton(buttons, BTN_TERMINATE);
            // Nothing to replay until the pilot has finished a mission.
            if (*pilot).m_mission == 0 {
                shell::ButtonMenu_DisableButton(buttons, BTN_SELECT_MISSION);
            } else {
                shell::ButtonMenu_EnableButton(buttons, BTN_SELECT_MISSION);
            }
            shell::ButtonMenu_DisableButton(buttons, BTN_BACK);

            shell::HideFields(mission_rows());
            shell::g_missionListShown = 0;
            shell::HideFields(pilot_stats());
            shell::ShowFields(pilot_stats());
        }
    }

    /// Names a new pilot in the empty slot that was clicked. Leaves the slot alone if the name comes back empty.
    unsafe fn create(&self, button: i32, pilot: *mut PilotRecord) {
        unsafe {
            let buttons = shell::g_rosterMenu;
            for id in [BTN_CLAN_HALL, BTN_TERMINATE, BTN_SELECT_MISSION, BTN_BACK] {
                shell::ButtonMenu_DisableButton(buttons, id);
            }
            shell::HideFields(mission_rows());
            shell::g_missionListShown = 0;
            shell::HideFields(pilot_stats());
            shell::g_currentPilot = null_mut();

            let callsign = (*pilot).m_callsign.as_mut_ptr();
            *callsign = 0;
            // Pumps the shell's loop itself, so the screen is frozen until this returns
            {
                let _menu = menu::lock();
                shell::EditTextField(
                    shell::g_titleFont,
                    CALLSIGN_X,
                    (button - 1) * ROW_HEIGHT + FIRST_ROW_Y,
                    callsign,
                    null_mut(),
                    CALLSIGN_MAX_CHARS,
                    CALLSIGN_MAX_WIDTH,
                );
            }
            shell::UppercaseString(callsign);
            if *callsign == 0 {
                return;
            }

            shell::g_currentPilot = pilot;
            let pilot = &mut *pilot;
            pilot.m_inUse = 1;
            pilot.m_mission = 0;
            pilot.m_rank = 0;
            pilot.m_honor = (HONOR + f64::from(rand()) / RAND_MAX * HONOR) as i32;
            pilot.m_kills = 0;
            pilot.m_hits = 0;
            pilot.m_shotsFired = 0;
            pilot.m_unk0x24 = 0;

            shell::SetActivePilot(pilot);
            shell::ShowPilotCallsigns();
            shell::ButtonMenu_EnableButton(buttons, BTN_CLAN_HALL);
            shell::ButtonMenu_EnableButton(buttons, BTN_TERMINATE);
            shell::ShowFields(pilot_stats());
            shell::g_newPilotRegistered = 1;
        }
    }

    /// Frees the selected pilot's slot and everything that hung off the selection
    unsafe fn terminate(&self) {
        unsafe {
            shell::ClearPilot(shell::g_currentPilot);
            shell::g_newPilotRegistered = 0;
            shell::HideFields(pilot_stats());
            shell::HideFields(mission_rows());
            shell::g_missionListShown = 0;
            shell::g_currentPilot = null_mut();

            let buttons = shell::g_rosterMenu;
            for id in [BTN_CLAN_HALL, BTN_TERMINATE, BTN_SELECT_MISSION, BTN_BACK] {
                shell::ButtonMenu_DisableButton(buttons, id);
            }
        }
    }
}

static STATE: Mutex<Option<Roster>> = Mutex::new(None);

#[unsafe(export_name = "PilotRosterCallback")]
pub unsafe extern "C" fn roster(
    db: *mut TMPackDataBase,
    campaign: *mut i32,
    pilot_chosen: *mut u8,
    scenario: *mut *mut c_char,
    msg: i32,
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
            msg as u32,
        );
    }
}
