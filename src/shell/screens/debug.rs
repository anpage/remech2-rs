use std::ffi::{c_char, c_void};
use std::sync::atomic::{AtomicI32, Ordering};

use egui::{Button, Ui};
use mw2_sys::shell::{self, MissionResults};

use super::{CAMPAIGN_LENGTH, Campaign, MissionResults as Outcome, ShellMsg};
use crate::{messages, shell::dialog};

pub const JUMP_TO_SCREEN: u32 = 0x8100;

/// Overrides the outcome the file gave, unless it's zero.
static OUTCOME_OVERRIDE: AtomicI32 = AtomicI32::new(0);

const NEEDS_PILOT: &str = "Select a pilot in the roster first";

/// Returns the scenario of the mission the pilot is on so that the debug menu can jump to the debrief.
unsafe fn current_scenario(campaign: Campaign) -> Option<*mut c_char> {
    unsafe {
        let missions = match campaign {
            Campaign::Wolf | Campaign::JadeFalcon => shell::g_campaignMissions[campaign as usize],
            Campaign::TrialsOfGrievance => return None,
        };
        let pilot = shell::g_currentPilot.as_ref()?;
        let mission = pilot.m_mission.clamp(0, CAMPAIGN_LENGTH - 1) as usize;
        Some((*missions.add(mission)).m_scenario)
    }
}

/// Mimics the menu's New Alliance option to jump to another screen.
unsafe fn jump(msg: u32) {
    unsafe {
        shell::CloseMenuFunction();
        let Some(screen) = shell::g_screenFunction else {
            messages::post(msg, msg as usize, 0);
            return;
        };
        if msg == ShellMsg::MISSION_DEBRIEF.0
            && let Some(campaign) = Campaign::from_raw(shell::g_selectedCampaign)
            && let Some(scenario) = current_scenario(campaign)
        {
            shell::g_scenario = scenario;
        }

        screen(
            shell::g_mw2Database,
            &raw mut shell::g_selectedCampaign,
            &raw mut shell::g_pilotChosen,
            &raw mut shell::g_scenario,
            msg as i32,
        );
    }
}

fn request_jump(msg: ShellMsg) {
    messages::post(JUMP_TO_SCREEN, msg.0 as usize, 0);
}

pub fn menu(ui: &mut Ui) {
    let has_pilot = unsafe { !shell::g_currentPilot.is_null() };

    ui.label("Go to");
    if ui.button("Main Menu").clicked() {
        request_jump(ShellMsg::MAIN_MENU);
    }
    if ui.button("Pilot Roster").clicked() {
        request_jump(ShellMsg::PILOT_ROSTER);
    }
    for (label, msg) in [
        ("Mechbay", ShellMsg::MECHBAY),
        ("Mission Debrief", ShellMsg::MISSION_DEBRIEF),
    ] {
        if ui
            .add_enabled(has_pilot, Button::new(label))
            .on_disabled_hover_text(NEEDS_PILOT)
            .clicked()
        {
            request_jump(msg);
        }
    }

    ui.separator();
    ui.label("Campaign");
    let mut campaign = unsafe { shell::g_selectedCampaign };
    ui.radio_value(&mut campaign, 0, "Wolf");
    ui.radio_value(&mut campaign, 1, "Jade Falcon");
    ui.radio_value(&mut campaign, 2, "Trials of Grievance");
    unsafe { shell::g_selectedCampaign = campaign };

    ui.separator();
    ui.label("Debrief outcome");
    let mut outcome = OUTCOME_OVERRIDE.load(Ordering::Relaxed);
    ui.radio_value(&mut outcome, 0, "From MW2MSN.CFG");
    ui.radio_value(&mut outcome, Outcome::SUCCESS, "Won");
    ui.radio_value(&mut outcome, Outcome::FAILED, "Lost");
    OUTCOME_OVERRIDE.store(outcome, Ordering::Relaxed);
}

/// Handles the debug menu's jumps and holds back transitions while a prompt is up
#[unsafe(export_name = "ShellHandleMessage")]
pub unsafe extern "C" fn shell_handle_message(message: u32, wparam: usize, lparam: isize) -> isize {
    if message == JUMP_TO_SCREEN {
        unsafe { jump(wparam as u32) };
        return 0;
    }
    if dialog::park_transition(message, wparam) {
        return 0;
    }
    unsafe { shell::ShellHandleMessageC(message, wparam, lparam) }
}

/// Reads `MW2MSN.CFG` for the debrief's entry function
#[unsafe(export_name = "ReadMissionResults")]
pub unsafe extern "C" fn read_mission_results(results: *mut c_void) {
    unsafe { shell::ReadMissionResultsC(results) };
    let outcome = OUTCOME_OVERRIDE.load(Ordering::Relaxed);
    if outcome != 0
        && let Some(results) = unsafe { results.cast::<MissionResults>().as_mut() }
    {
        results.m_outcome = outcome;
    }
}
