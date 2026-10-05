use std::ffi::CString;

use anyhow::{Context, Result};

mod audio;
mod dialog;
mod overlay;
mod screens;
mod smacker;

pub use overlay::OverlayUi;

/// Runs the shell until it quits or hands off to the sim.
/// Returns its exit code: 3 to run the sim, 255 to leave the game.
pub fn run(intro_or_sim: &str) -> Result<i32> {
    let intro_or_sim = CString::new(intro_or_sim).context("CString::new failed")?;
    // ShellMain resets the C globals; a prompt left open by the last run goes with them
    dialog::clear();
    Ok(unsafe { mw2_sys::shell::ShellMain(intro_or_sim.as_ptr().cast_mut()) })
}
