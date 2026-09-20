use std::ffi::{c_char, c_void};
use std::sync::atomic::{AtomicI32, Ordering};

use binding::{game_fns, globals, macros::hook, patches};
use egui::{Button, Ui};
use tracing::error;
use windows::Win32::{
    Foundation::{LPARAM, WPARAM},
    UI::WindowsAndMessaging::{PostMessageA, WM_APP},
};

use super::{Campaign, G_PILOT, G_SHELL_CALLBACK, G_WND, MissionResults, ShellMsg};
use crate::shell::MODULE;
use crate::shell::screens::debrief;

pub const JUMP_TO_SCREEN: u32 = WM_APP + 0x100;

type ScreenFn = unsafe extern "cdecl" fn(*mut c_void, *mut i32, *mut u8, *mut *mut c_char, u32);

globals!(
    static G_DB: *mut c_void = 0x0007122c;
    static G_SELECTED_CAMPAIGN: i32 = 0x0007cc88;
    static G_PILOT_CHOSEN: u8 = 0x0007cc8c;
    static G_SCENARIO: *mut c_char = 0x0007cc84;
);

game_fns!(
    /// Closes whatever the menu bar opened, e.g. Combat Variables.
    static CLEAR_MENU_CALLBACK: unsafe extern "cdecl" fn() = 0x000109f9;
);

/// Overrides the outcome the file gave, unless it's zero.
static OUTCOME_OVERRIDE: AtomicI32 = AtomicI32::new(0);

const NEEDS_PILOT: &str = "Select a pilot in the roster first";

/// Mimics the menu's New Alliance option to jump to another screen.
pub unsafe fn jump(msg: u32) {
    unsafe {
        (CLEAR_MENU_CALLBACK.get())();
        let callback = G_SHELL_CALLBACK.get();
        if callback.is_null() {
            if let Err(e) = PostMessageA(Some(G_WND.get()), msg, WPARAM(msg as usize), LPARAM(0)) {
                error!("debug jump to {msg:#x} failed: {e}");
            }
            return;
        }
        if msg == ShellMsg::MISSION_DEBRIEF.0
            && let Some(campaign) = Campaign::from_raw(G_SELECTED_CAMPAIGN.get())
            && let Some(scenario) = debrief::current_scenario(campaign)
        {
            G_SCENARIO.set(scenario);
        }

        let callback: ScreenFn = std::mem::transmute(callback);
        callback(
            G_DB.get(),
            G_SELECTED_CAMPAIGN.ptr(),
            G_PILOT_CHOSEN.ptr(),
            G_SCENARIO.ptr(),
            msg,
        );
    }
}

fn request_jump(msg: ShellMsg) {
    unsafe {
        if let Err(e) = PostMessageA(
            Some(G_WND.get()),
            JUMP_TO_SCREEN,
            WPARAM(msg.0 as usize),
            LPARAM(0),
        ) {
            error!("debug jump to {msg:?} failed: {e}");
        }
    }
}

pub fn menu(ui: &mut Ui) {
    let has_pilot = unsafe { !G_PILOT.get().is_null() };

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
    let mut campaign = unsafe { G_SELECTED_CAMPAIGN.get() };
    ui.radio_value(&mut campaign, 0, "Wolf");
    ui.radio_value(&mut campaign, 1, "Jade Falcon");
    ui.radio_value(&mut campaign, 2, "Trials of Grievance");
    unsafe { G_SELECTED_CAMPAIGN.set(campaign) };

    ui.separator();
    ui.label("Debrief outcome");
    let mut outcome = OUTCOME_OVERRIDE.load(Ordering::Relaxed);
    ui.radio_value(&mut outcome, 0, "From MW2MSN.CFG");
    ui.radio_value(&mut outcome, MissionResults::SUCCESS, "Won");
    ui.radio_value(&mut outcome, MissionResults::FAILED, "Lost");
    OUTCOME_OVERRIDE.store(outcome, Ordering::Relaxed);
}

/// Reads `MW2MSN.CFG` for the debrief's entry function.
#[hook(rva = 0x000021a6)]
unsafe extern "cdecl" fn read_mission_results(results: *mut MissionResults) {
    unsafe {
        original(results);
        let outcome = OUTCOME_OVERRIDE.load(Ordering::Relaxed);
        if outcome != 0 {
            (*results).outcome = outcome;
        }
    }
}

patches!(
    pub(in crate::shell) static PATCHES = [
        hook read_mission_results,
    ];
);
