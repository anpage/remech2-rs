use std::{ffi::CString, ptr};

use anyhow::{Context, Result};

use crate::{ailrs, sim::timing::G_DELTA_TIME};

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
pub fn run(cmd_line: &str) -> Result<i32> {
    let cmd_line = CString::new(cmd_line).context("CString::new failed")?;
    let result = unsafe { mw2_sys::sim::SimMain(cmd_line.as_ptr().cast_mut(), ptr::null_mut()) };
    ailrs::shutdown();
    Ok(result)
}
