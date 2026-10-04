use std::{ffi::CString, ptr};

use anyhow::{Context, Result};

use binding::{macros::patch_groups, module::ModuleBase};

use crate::{
    ailrs,
    sim::{
        timing::G_DELTA_TIME,
        types::RenderTarget,
        window::{G_GAME_WINDOW_HEIGHT, G_GAME_WINDOW_WIDTH},
    },
};

mod audio;
mod camera;
mod cd_audio;
mod hud;
mod input;
mod jumpjets;
mod math;
mod menu;
mod overlay;
mod shots;
mod stats;
mod ticks;
mod timing;
mod types;
mod win32;
pub mod window;

pub use overlay::OverlayUi;

pub static MODULE: ModuleBase = ModuleBase::new("MW2.DLL");

patch_groups! {
    static PATCH_GROUPS = [
        audio,
        camera,
        hud,
        input,
        jumpjets,
        math,
        menu,
        shots,
        timing,
        win32,
        window,
    ];
}

/// Runs a mission.
/// Returns the sim's exit code: 255 to leave the game, anything else to go back to the shell.
pub fn run(cmd_line: &str) -> Result<i32> {
    let cmd_line = CString::new(cmd_line).context("CString::new failed")?;
    let result = unsafe { mw2_sys::sim::SimMain(cmd_line.as_ptr().cast_mut(), ptr::null_mut()) };
    ailrs::shutdown();
    Ok(result)
}
