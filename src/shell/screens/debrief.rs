use std::ffi::c_char;
use std::sync::Mutex;

use mw2_sys::shell::{self, ArchiveReader, ButtonMenu, TMPackDataBase};

use super::{
    CAMPAIGN_LENGTH, Campaign, OUTCOME_SUCCESS, Screen, ScreenArgs, ShellMsg, allocate, delete,
    run,
};
use crate::shell::overlay::confirm;

/// The viewer's tick returns this to stay open. It's the archives screen's id.
const VIEWER_STAY: i32 = 0x40b;

unsafe fn outcome() -> i32 {
    unsafe { shell::g_missionResults.m_outcome }
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
            let viewer = shell::g_aftermathReader;
            if !viewer.is_null() {
                return self.tick_aftermath(viewer, args);
            }

            shell::Page_TypeStep(shell::g_debriefPage);

            if self.confirm_pending {
                // The original blocked in `ShowDialog` here.
                let replay = confirm::answer()?;
                confirm::close();
                self.confirm_pending = false;
                if !replay {
                    return None;
                }
                *shell::g_currentPilot = std::ptr::read(&raw const shell::g_pilotBeforeMission);
                shell::SavePilotRoster();
                return Some(ShellMsg::MISSION_BRIEFING);
            }

            let mouse = shell::g_mouseState.as_ref()?;

            let hit = shell::ButtonMenu_HitTest(shell::g_debriefMenu, mouse.m_x, mouse.m_y);
            if mouse.m_leftPressed != 1 {
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
                    if outcome() != OUTCOME_SUCCESS {
                        return Some(ShellMsg::MISSION_BRIEFING);
                    }
                    confirm::open(&["Are you Sure?"]);
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

            delete(&raw mut shell::g_debriefPage, shell::Page_Page_destructor);
            delete(&raw mut shell::g_debriefMenu, shell::ButtonMenu_ButtonMenu_destructor);
            delete(&raw mut shell::g_aftermathReader, shell::ArchiveReader_ArchiveReader_destructor);

            shell::VideoDriver_ClearGlyphs(shell::g_videoDriver, 1);
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
            if outcome() != OUTCOME_SUCCESS {
                return ShellMsg::MISSION_SELECT;
            }

            let mission = (*shell::g_currentPilot).m_mission;
            if mission >= CAMPAIGN_LENGTH {
                return ShellMsg::FINALE;
            }
            let campaign = shell::g_campaignMissions[clan as usize];
            *args.scenario = (*campaign.add(mission as usize)).m_scenario;
            ShellMsg::MISSION_SELECT
        }
    }

    /// Swaps the debrief's buttons for the aftermath viewer's.
    unsafe fn open_aftermath(&self, state: Campaign) {
        unsafe {
            shell::Page_Hide(shell::g_debriefPage);

            // The original leaves this dangling until the viewer closes.
            delete(&raw mut shell::g_debriefMenu, shell::ButtonMenu_ButtonMenu_destructor);

            let layout = &*&raw const shell::g_aftermathScreens[state as usize];
            let viewer = allocate::<ArchiveReader>();
            if viewer.is_null() {
                return;
            }
            shell::ArchiveReader_ArchiveReader(
                viewer,
                c"".as_ptr().cast_mut(),
                shell::g_archiveFont,
                -1,
                0,
                std::ptr::null_mut(),
                shell::g_debriefPages,
                layout.m_buttons,
                layout.m_count,
            );
            shell::g_aftermathReader = viewer;
        }
    }

    unsafe fn tick_aftermath(
        &self,
        viewer: *mut ArchiveReader,
        args: &mut ScreenArgs,
    ) -> Option<ShellMsg> {
        unsafe {
            let state = args.campaign()?;
            let next = shell::ArchiveReader_Run(viewer);
            if next == VIEWER_STAY {
                return None;
            }

            delete(&raw mut shell::g_aftermathReader, shell::ArchiveReader_ArchiveReader_destructor);

            let next = ShellMsg(next as u32);
            if next == ShellMsg::EXIT_TO_DESKTOP || next == ShellMsg::MAIN_MENU {
                return Some(next);
            }

            let layout = &*&raw const shell::g_debriefScreens[state as usize];
            let buttons = allocate::<ButtonMenu>();
            if !buttons.is_null() {
                shell::ButtonMenu_ButtonMenu(
                    buttons,
                    shell::g_videoDriver,
                    shell::g_defaultFont,
                    0,
                    layout.m_buttons,
                    layout.m_count,
                );
                shell::g_debriefMenu = buttons;
            }
            shell::Page_Restart(shell::g_debriefPage);
            None
        }
    }
}

static STATE: Mutex<Option<Debrief>> = Mutex::new(None);

#[unsafe(export_name = "MissionDebriefCallback")]
pub unsafe extern "C" fn debrief(
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
