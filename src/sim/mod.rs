use std::{ffi::CString, ptr};

use anyhow::{Context, Result};

use remech2_sys::shared::{MissionLaunch, MissionReport};

use crate::ailrs;

mod camera;
mod cd_audio;
mod input;
mod jumpjets;
mod math;
mod overlay;
mod shots;
mod stats;
mod ticks;
mod timing;
mod window;

pub use overlay::OverlayUi;

/// Runs a mission.
/// Returns the sim's exit code: 255 to leave the game, anything else to go back to the shell.
pub fn run(cmd_line: &str, launch: &MissionLaunch, report: &mut MissionReport) -> Result<i32> {
    let cmd_line = CString::new(cmd_line).context("CString::new failed")?;
    // SimMain resets the C globals, and with them any ticks handles they held
    ticks::clear();
    let result = unsafe {
        remech2_sys::sim::SimMain(
            cmd_line.as_ptr().cast_mut(),
            launch,
            ptr::null_mut(),
            report,
        )
    };
    ailrs::shutdown();
    Ok(result)
}
