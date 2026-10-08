use std::{ffi::CString, sync::Mutex};

use anyhow::{Context, Result};

use remech2_sys::shared::MissionReport;

mod audio;
mod dialog;
mod handoff;
mod overlay;
mod screens;
mod smacker;

pub use handoff::with_mission_launch;
pub use overlay::OverlayUi;

static MISSION_REPORT: Mutex<Option<MissionReport>> = Mutex::new(None);

pub fn set_mission_report(report: MissionReport) {
    *MISSION_REPORT.lock().unwrap() = Some(report);
}

pub fn mission_report() -> MissionReport {
    MISSION_REPORT.lock().unwrap().unwrap_or_default()
}

/// Runs the shell until it quits or hands off to the sim.
/// Returns its exit code: 3 to run the sim, 255 to leave the game.
pub fn run(intro_or_sim: &str) -> Result<i32> {
    let intro_or_sim = CString::new(intro_or_sim).context("CString::new failed")?;
    // ShellMain resets the C globals; a prompt left open by the last run goes with them
    dialog::clear();
    Ok(unsafe { remech2_sys::shell::ShellMain(intro_or_sim.as_ptr().cast_mut()) })
}
