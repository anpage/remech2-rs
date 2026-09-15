use std::ffi::{CString, c_char, c_void};

use anyhow::{Context, Result, bail};
use windows::{
    Win32::{
        Foundation::{FreeLibrary, HMODULE, HWND},
        System::LibraryLoader::{GetProcAddress, LoadLibraryA},
    },
    core::{BOOL, s},
};

use binding::{
    macros::patch_groups,
    module::ModuleBase,
    patch::{apply_groups, revert_groups},
};

use crate::{
    WindowProc,
    ail::Ail,
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
pub mod drawmode;
mod hud;
mod input;
mod jumpjets;
mod math;
mod menu;
mod shots;
mod stats;
mod timing;
mod types;
mod win32;
pub mod window;

pub static MODULE: ModuleBase = ModuleBase::new("MW2.DLL");

patch_groups! {
    static PATCH_GROUPS = [
        audio,
        camera,
        cd_audio,
        drawmode::hooks,
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

type SimMainProc = unsafe extern "stdcall" fn(
    HMODULE,
    u32,
    *const c_char,
    *const *const c_void,
    BOOL,
    HWND,
) -> i32;

pub struct Sim {
    ail: Ail,
    module: HMODULE,
}

impl Sim {
    pub fn new() -> Result<Self> {
        if MODULE.is_loaded() {
            bail!("Can't load sim more than once");
        }

        let module = unsafe { LoadLibraryA(s!("MW2.DLL"))? };

        match unsafe { Self::install(module) } {
            Ok(ail) => Ok(Self { ail, module }),
            Err(e) => {
                revert_groups(PATCH_GROUPS);
                MODULE.clear();
                unsafe {
                    let _ = FreeLibrary(module);
                }
                Err(e)
            }
        }
    }

    unsafe fn install(module: HMODULE) -> Result<Ail> {
        MODULE.set(module.0 as usize);
        unsafe { apply_groups(PATCH_GROUPS)? };

        Ail::new()
    }

    pub fn sim_main(
        &self,
        cmd_line: &str,
        unknown: *const *const c_void,
        is_net_game: BOOL,
        window: HWND,
    ) -> Result<i32> {
        let sim_main = unsafe {
            let sim_main =
                GetProcAddress(self.module, s!("SimMain")).context("Couldn't find SimMain")?;
            std::mem::transmute::<*const (), SimMainProc>(sim_main as *const ())
        };

        let cmd_line = CString::new(cmd_line).context("CString::new failed")?;
        let result = unsafe {
            sim_main(
                self.module,
                0,
                cmd_line.as_ptr(),
                unknown,
                is_net_game,
                window,
            )
        };

        if result == -1 {
            bail!("REMECH 2 is unable to locate necessary program components.");
        }

        Ok(result)
    }

    pub fn window_proc(&self) -> Result<WindowProc> {
        unsafe {
            let window_proc = GetProcAddress(self.module, s!("SimWindowProc"))
                .context("Couldn't find SimWindowProc")?;
            Ok(std::mem::transmute::<
                unsafe extern "system" fn() -> isize,
                WindowProc,
            >(window_proc))
        }
    }
}

impl Drop for Sim {
    fn drop(&mut self) {
        unsafe {
            ailrs::shutdown();
            revert_groups(PATCH_GROUPS);
            self.ail.unhook();
            MODULE.clear();
            FreeLibrary(self.module).unwrap();
        }
    }
}
