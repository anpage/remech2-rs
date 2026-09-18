use std::ffi::{CString, c_char};

use anyhow::{Context, Result, bail};
use windows::{
    Win32::{
        Foundation::{FreeLibrary, HMODULE, HWND},
        System::LibraryLoader::{GetModuleHandleA, GetProcAddress, LoadLibraryA},
    },
    core::s,
};

use binding::{
    macros::patch_groups,
    module::ModuleBase,
    patch::{apply_groups, revert_groups},
};

use crate::{WindowProc, ail::Ail};

mod audio;
mod database;
mod drawmode;
mod mechlab;
mod registry;
mod screens;
mod settings_ui;
mod smacker;
mod win32;

pub static MODULE: ModuleBase = ModuleBase::new("MW2SHELL.DLL");

patch_groups! {
    static PATCH_GROUPS = [
        audio::hooks,
        database,
        drawmode::hooks,
        mechlab,
        registry,
        screens::main_menu,
        settings_ui,
        win32,
    ];
}

pub static SMACK_MODULE: ModuleBase = ModuleBase::new("SMACKW32.DLL");

patch_groups! {
    static SMACK_PATCH_GROUPS = [
        smacker,
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
                revert_groups(SMACK_PATCH_GROUPS);
                revert_groups(PATCH_GROUPS);
                SMACK_MODULE.clear();
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

        let smack_module = unsafe { GetModuleHandleA(s!("SMACKW32.DLL"))? };
        SMACK_MODULE.set(smack_module.0 as usize);
        unsafe { apply_groups(SMACK_PATCH_GROUPS)? };

        Ail::new()
    }

    pub fn shell_main(&self, intro_or_sim: &str, window: HWND) -> Result<i32> {
        let shell_main = unsafe {
            let shell_main =
                GetProcAddress(self.module, s!("ShellMain")).context("Couldn't find ShellMain")?;
            std::mem::transmute::<*const (), ShellMainProc>(shell_main as *const ())
        };

        let intro_or_sim = CString::new(intro_or_sim).context("CString::new failed")?;
        let result = unsafe { shell_main(self.module, 0, intro_or_sim.as_ptr(), 1, window) };

        if result == -1 {
            bail!("REMECH 2 is unable to locate necessary program components.");
        }

        Ok(result)
    }

    pub fn window_proc(&self) -> Result<WindowProc> {
        unsafe {
            let window_proc = GetProcAddress(self.module, s!("ShellWindowProc"))
                .context("Couldn't find ShellWindowProc")?;
            Ok(std::mem::transmute::<
                unsafe extern "system" fn() -> isize,
                WindowProc,
            >(window_proc))
        }
    }
}

impl Drop for Shell {
    fn drop(&mut self) {
        revert_groups(SMACK_PATCH_GROUPS);
        revert_groups(PATCH_GROUPS);
        drawmode::hooks::shutdown();

        unsafe {
            self.ail.unhook();
            SMACK_MODULE.clear();
            MODULE.clear();
            FreeLibrary(self.module).unwrap();
        }
    }
}
