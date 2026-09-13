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
    ailrs::{
        self,
        interface::{
            allocate_file_sample, allocate_sample_handle, end_sample, init_sample,
            load_sample_buffer, register_eos_callback, release_sample_handle, resume_sample,
            sample_buffer_ready, sample_user_data, serve, set_preference, set_sample_loop_count,
            set_sample_pan, set_sample_playback_rate, set_sample_type, set_sample_user_data,
            set_sample_volume, start_sample, stop_sample, wave_out_open,
        },
    },
    common::{HeapFreeFunc, fake_heap_free},
    sim::{
        timing::G_DELTA_TIME,
        types::RenderTarget,
        window::{G_GAME_WINDOW_HEIGHT, G_GAME_WINDOW_WIDTH},
    },
};

mod camera;
mod cd_audio;
pub mod drawmode;
mod hud;
mod input;
mod jumpjets;
mod math;
mod shots;
mod stats;
mod timing;
mod types;
pub mod window;

pub static MODULE: ModuleBase = ModuleBase::new("MW2.DLL");

patch_groups! {
    static PATCH_GROUPS = [
        camera,
        cd_audio,
        drawmode::hooks,
        hud,
        input,
        jumpjets,
        math,
        shots,
        timing,
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
            bail!("Can't load shell more than once");
        }

        let module = unsafe { LoadLibraryA(s!("MW2.DLL"))? };
        let base_address = module.0 as usize;

        MODULE.set(base_address);

        if let Err(e) = unsafe { apply_groups(PATCH_GROUPS) } {
            MODULE.clear();
            unsafe {
                let _ = FreeLibrary(module);
            }
            return Err(e);
        }

        let flee_option = (base_address + 0x000a1ad0) as *mut [u8; 7];
        unsafe {
            flee_option.write_volatile(*(b"Desktop"));
        }

        let flee_title = (base_address + 0x000a1b08) as *mut [u8; 7];
        unsafe {
            flee_title.write_volatile(*(b"DESKTOP"));
        }

        unsafe {
            let heap_free_thunk = (base_address + 0x001834d0) as *mut HeapFreeFunc;
            *heap_free_thunk = fake_heap_free;

            // AIL replacement

            let ail_allocate_file_sample_thunk = (base_address + 0x001836e8) as *mut usize;
            *ail_allocate_file_sample_thunk = allocate_file_sample as *const () as usize;

            let ail_allocate_sample_handle_thunk = (base_address + 0x00183654) as *mut usize;
            *ail_allocate_sample_handle_thunk = allocate_sample_handle as *const () as usize;

            let ail_end_sample_thunk = (base_address + 0x00183674) as *mut usize;
            *ail_end_sample_thunk = end_sample as *const () as usize;

            let ail_init_sample_thunk = (base_address + 0x001836dc) as *mut usize;
            *ail_init_sample_thunk = init_sample as *const () as usize;

            let ail_load_sample_buffer_thunk = (base_address + 0x00183690) as *mut usize;
            *ail_load_sample_buffer_thunk = load_sample_buffer as *const () as usize;

            let ail_register_eos_callback_thunk = (base_address + 0x001836d8) as *mut usize;
            *ail_register_eos_callback_thunk = register_eos_callback as *const () as usize;

            let ail_release_sample_handle_thunk = (base_address + 0x001836ec) as *mut usize;
            *ail_release_sample_handle_thunk = release_sample_handle as *const () as usize;

            let ail_resume_sample_thunk = (base_address + 0x0018366c) as *mut usize;
            *ail_resume_sample_thunk = resume_sample as *const () as usize;

            let ail_sample_buffer_ready_thunk = (base_address + 0x00183650) as *mut usize;
            *ail_sample_buffer_ready_thunk = sample_buffer_ready as *const () as usize;

            let ail_sample_user_data_thunk = (base_address + 0x00183664) as *mut usize;
            *ail_sample_user_data_thunk = sample_user_data as *const () as usize;

            let ail_set_preference_thunk = (base_address + 0x00183698) as *mut usize;
            *ail_set_preference_thunk = set_preference as *const () as usize;

            let ail_set_sample_loop_count_thunk = (base_address + 0x001836e4) as *mut usize;
            *ail_set_sample_loop_count_thunk = set_sample_loop_count as *const () as usize;

            let ail_set_sample_pan_thunk = (base_address + 0x0018368c) as *mut usize;
            *ail_set_sample_pan_thunk = set_sample_pan as *const () as usize;

            let ail_set_sample_playback_rate_thunk = (base_address + 0x001836d0) as *mut usize;
            *ail_set_sample_playback_rate_thunk = set_sample_playback_rate as *const () as usize;

            let ail_set_sample_type_thunk = (base_address + 0x001836d4) as *mut usize;
            *ail_set_sample_type_thunk = set_sample_type as *const () as usize;

            let ail_set_sample_user_data_thunk = (base_address + 0x00183678) as *mut usize;
            *ail_set_sample_user_data_thunk = set_sample_user_data as *const () as usize;

            let ail_set_sample_volume_thunk = (base_address + 0x00183668) as *mut usize;
            *ail_set_sample_volume_thunk = set_sample_volume as *const () as usize;

            let ail_start_sample_thunk = (base_address + 0x001836e0) as *mut usize;
            *ail_start_sample_thunk = start_sample as *const () as usize;

            let ail_stop_sample_thunk = (base_address + 0x00183670) as *mut usize;
            *ail_stop_sample_thunk = stop_sample as *const () as usize;

            let ail_wave_out_open_thunk = (base_address + 0x001836b0) as *mut usize;
            *ail_wave_out_open_thunk = wave_out_open as *const () as usize;

            let ail_serve_thunk = (base_address + 0x001836b4) as *mut usize;
            *ail_serve_thunk = serve as *const () as usize;

            Ok(Self {
                ail: Ail::new()?,
                module,
            })
        }
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
