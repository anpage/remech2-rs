use std::ffi::CString;

use anyhow::{Context, Result};

use binding::{macros::patch_groups, module::ModuleBase};

mod audio;
mod dialog;
mod overlay;
mod screens;
mod smacker;

pub use overlay::OverlayUi;

pub static MODULE: ModuleBase = ModuleBase::new("MW2SHELL.DLL");

patch_groups! {
    static PATCH_GROUPS = [
        screens::debrief,
        screens::debug,
        screens::main_menu,
        screens::mechlab,
        screens::roster,
        screens::settings,
    ];
}

/// Runs the shell until it quits or hands off to the sim.
/// Returns its exit code: 3 to run the sim, 255 to leave the game.
pub fn run(intro_or_sim: &str) -> Result<i32> {
    let intro_or_sim = CString::new(intro_or_sim).context("CString::new failed")?;
    Ok(unsafe { mw2_sys::shell::ShellMain(intro_or_sim.as_ptr().cast_mut()) })
}
