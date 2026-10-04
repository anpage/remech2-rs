use std::ffi::{CString, c_char};

use anyhow::{Context, Result, bail};
use windows::{
    Win32::{
        Foundation::{FreeLibrary, HMODULE, HWND},
        System::LibraryLoader::{GetProcAddress, LoadLibraryA},
    },
    core::s,
};

use binding::{
    macros::patch_groups,
    module::ModuleBase,
    patch::{apply_groups, revert_groups},
};

use crate::ail::Ail;

mod audio;
mod database;
mod dialog;
mod overlay;
mod screens;
mod smacker;
mod win32;

pub use overlay::OverlayUi;

pub static MODULE: ModuleBase = ModuleBase::new("MW2SHELL.DLL");

patch_groups! {
    static PATCH_GROUPS = [
        database,
        dialog,
        overlay::mouse,
        screens::debrief,
        screens::debug,
        screens::main_menu,
        screens::mechlab,
        screens::roster,
        screens::settings,
        smacker,
        win32,
    ];
}

type ShellMainProc = unsafe extern "stdcall" fn(HMODULE, i32, *const c_char, i32, HWND) -> i32;

pub struct Shell {
    ail: Ail,
    module: HMODULE,
}

impl Shell {
    pub fn new() -> Result<Self> {
        if MODULE.is_loaded() {
            bail!("Can't load shell more than once");
        }

        let module = unsafe { LoadLibraryA(s!("MW2SHELL.DLL"))? };

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

    pub fn shell_main(&self, intro_or_sim: &str, window: HWND) -> Result<i32> {
        let shell_main = unsafe {
            let shell_main =
                GetProcAddress(self.module, s!("ShellMain")).context("Couldn't find ShellMain")?;
            std::mem::transmute::<*const (), ShellMainProc>(shell_main as *const ())
        };

        let intro_or_sim = CString::new(intro_or_sim).context("CString::new failed")?;

        // Load the saved volumes up front instead of waiting for the settings screen
        unsafe { (audio::LOAD_SOUND_CONFIG.get())() };

        let result = unsafe { shell_main(self.module, 0, intro_or_sim.as_ptr(), 1, window) };

        if result == -1 {
            bail!("REMECH 2 is unable to locate necessary program components.");
        }

        Ok(result)
    }
}

impl Drop for Shell {
    fn drop(&mut self) {
        revert_groups(PATCH_GROUPS);

        unsafe {
            self.ail.unhook();
            MODULE.clear();
            FreeLibrary(self.module).unwrap();
        }
    }
}
