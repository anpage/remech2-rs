use std::{
    ffi::{CString, c_char, c_void},
    sync::{Mutex, RwLock},
};

use anyhow::{Context, Result, bail};
use retour::GenericDetour;
use windows::{
    Win32::{
        Foundation::{FreeLibrary, HMODULE, HWND},
        Media::Multimedia::{
            MCI_FORMAT_TMSF, MCI_FROM, MCI_MODE_OPEN, MCI_MODE_PAUSE, MCI_MODE_PLAY, MCI_MODE_STOP,
            MCI_OPEN, MCI_OPEN_PARMSA, MCI_OPEN_TYPE, MCI_PLAY, MCI_PLAY_PARMS, MCI_SET,
            MCI_SET_PARMS, MCI_SET_TIME_FORMAT, MCI_STATUS, MCI_STATUS_ITEM, MCI_STATUS_MODE,
            MCI_STATUS_PARMS, MCI_TO, mciSendCommandA,
        },
        System::LibraryLoader::{GetProcAddress, LoadLibraryA},
    },
    core::{BOOL, s},
};

use binding::{
    macros::globals,
    module::ModuleBase,
    patch::{Patch, apply_groups, revert_groups},
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
    cd_audio::{AudioCdStatus, CdAudioPlayer, MAX_TRACK, source::CdSource, tmsf::CdAudioPosition},
    common::{HeapFreeFunc, fake_heap_free},
    hooker::hook_function,
    settings::SETTINGS,
    sim::{
        timing::G_DELTA_TIME,
        types::{DrawMode, RenderTarget},
        window::{G_GAME_WINDOW_HEIGHT, G_GAME_WINDOW_WIDTH},
    },
};

mod camera;
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

static PATCH_GROUPS: &[&[&'static dyn Patch]] = &[
    camera::PATCHES,
    drawmode::hooks::PATCHES,
    hud::PATCHES,
    input::PATCHES,
    jumpjets::PATCHES,
    math::PATCHES,
    shots::PATCHES,
    timing::PATCHES,
    window::PATCHES,
];

type SimMainProc = unsafe extern "stdcall" fn(
    HMODULE,
    u32,
    *const c_char,
    *const *const c_void,
    BOOL,
    HWND,
) -> i32;

#[repr(C)]
struct CdAudioTracks {
    first_track: u32,
    number_of_tracks: u32,
    track_positions: *mut u32,
}

// Functions to hook
// static DEBUG_LOG_HOOK: RwLock<Option<RawDetour>> = RwLock::new(None);

type InitCdAudioFunc = unsafe extern "stdcall" fn() -> u32;
static INIT_CD_AUDIO_HOOK: RwLock<Option<GenericDetour<InitCdAudioFunc>>> = RwLock::new(None);

type GetCdAudioAuxDeviceFunc = unsafe extern "stdcall" fn() -> i32;
static GET_CD_AUDIO_AUX_DEVICE_HOOK: RwLock<Option<GenericDetour<GetCdAudioAuxDeviceFunc>>> =
    RwLock::new(None);

type CloseCdAudioFunc = unsafe extern "stdcall" fn() -> i32;
static CLOSE_CD_AUDIO_HOOK: RwLock<Option<GenericDetour<CloseCdAudioFunc>>> = RwLock::new(None);

type StopCdAudioFunc = unsafe extern "stdcall" fn();
static STOP_CD_AUDIO_HOOK: RwLock<Option<GenericDetour<StopCdAudioFunc>>> = RwLock::new(None);

type PlayCdAudioFunc = unsafe extern "cdecl" fn(u32, u32);
static PLAY_CD_AUDIO_HOOK: RwLock<Option<GenericDetour<PlayCdAudioFunc>>> = RwLock::new(None);

type PauseCdAudioFunc = unsafe extern "stdcall" fn();
static PAUSE_CD_AUDIO_HOOK: RwLock<Option<GenericDetour<PauseCdAudioFunc>>> = RwLock::new(None);

type ResumeCdAudioFunc = unsafe extern "stdcall" fn();
static RESUME_CD_AUDIO_HOOK: RwLock<Option<GenericDetour<ResumeCdAudioFunc>>> = RwLock::new(None);

type StartCdAudioFunc = unsafe extern "stdcall" fn() -> i32;
static START_CD_AUDIO_HOOK: RwLock<Option<GenericDetour<StartCdAudioFunc>>> = RwLock::new(None);

type GetCdStatusFunc = unsafe extern "cdecl" fn() -> AudioCdStatus;
static GET_CD_STATUS_HOOK: RwLock<Option<GenericDetour<GetCdStatusFunc>>> = RwLock::new(None);

type GetCdAudioTracksFunc = unsafe extern "cdecl" fn(*mut CdAudioTracks) -> i32;
static GET_CD_AUDIO_TRACKS_HOOK: RwLock<Option<GenericDetour<GetCdAudioTracksFunc>>> =
    RwLock::new(None);

type GetCdAudioPositionFunc = unsafe extern "cdecl" fn(*mut CdAudioPosition);
static GET_CD_AUDIO_POSITION_HOOK: RwLock<Option<GenericDetour<GetCdAudioPositionFunc>>> =
    RwLock::new(None);

type GetCdAudioVolumeFunc = unsafe extern "cdecl" fn() -> i32;
static GET_CD_AUDIO_VOLUME_HOOK: RwLock<Option<GenericDetour<GetCdAudioVolumeFunc>>> =
    RwLock::new(None);

type SetCdAudioVolumeFunc = unsafe extern "cdecl" fn(i32) -> i32;
static SET_CD_AUDIO_VOLUME_HOOK: RwLock<Option<GenericDetour<SetCdAudioVolumeFunc>>> =
    RwLock::new(None);

type DeInitCdAudioFunc = unsafe extern "stdcall" fn();
static DEINIT_CD_AUDIO_HOOK: RwLock<Option<GenericDetour<DeInitCdAudioFunc>>> = RwLock::new(None);

type UpdateCdAudioPositionFunc = unsafe extern "cdecl" fn(*mut CdAudioPosition);
static UPDATE_CD_AUDIO_POSITION_HOOK: RwLock<Option<GenericDetour<UpdateCdAudioPositionFunc>>> =
    RwLock::new(None);

type CdAudioTogglePausedFunc = unsafe extern "stdcall" fn();
static CD_AUDIO_TOGGLE_PAUSED_HOOK: RwLock<Option<GenericDetour<CdAudioTogglePausedFunc>>> =
    RwLock::new(None);

// Global variables
globals! {
    static G_CURRENT_DRAW_MODE: *mut DrawMode = 0x000b1774;
    // static G_WIDTH_SCALE: i32 = 0x000e9610;

    static G_CD_AUDIO_DEVICE: u32 = 0x000aa278;
    static G_CD_AUDIO_AUX_DEVICE: i32 = 0x000aa27c;
    static G_CD_AUDIO_GLOBAL_1: u32 = 0x000beca8;
    static G_CD_AUDIO_GLOBAL_2: u32 = 0x000becac;
    static G_CD_AUDIO_INITIALIZED: u32 = 0x000aa28c;
    static G_AUDIO_CD_STATUS: AudioCdStatus = 0x000becc0;
    static G_CD_AUDIO_TRACK_DATA: CdAudioTracks = 0x000aa280;
    static G_PAUSED_CD_AUDIO_POSITION: CdAudioPosition = 0x000becb0;
    static G_CD_AUDIO_VOLUME: i32 = 0x000a14a4;

    static G_SHOULD_QUIT: BOOL = 0x000acb18;
}

/// Cache the CD audio device to reuse between sim launches.
/// Windows 11 crashes if we try to close the CD audio device.
static mut CD_AUDIO_DEVICE: u32 = u32::MAX;

/// Our own CD audio player that actually plays tracks from files.
static CD_AUDIO_PLAYER: Mutex<Option<CdAudioPlayer>> = Mutex::new(None);

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

            *INIT_CD_AUDIO_HOOK.write().unwrap() = {
                let target: InitCdAudioFunc = std::mem::transmute(base_address + 0x0005a8b5);
                Some(hook_function(target, Self::init_cd_audio)?)
            };

            *GET_CD_AUDIO_AUX_DEVICE_HOOK.write().unwrap() = {
                let target: GetCdAudioAuxDeviceFunc =
                    std::mem::transmute(base_address + 0x0005a7a0);
                Some(hook_function(target, Self::get_cd_audio_aux_device)?)
            };

            *CLOSE_CD_AUDIO_HOOK.write().unwrap() = {
                let target: CloseCdAudioFunc = std::mem::transmute(base_address + 0x0005aa0a);
                Some(hook_function(target, Self::close_cd_audio)?)
            };

            *STOP_CD_AUDIO_HOOK.write().unwrap() = {
                let target: StopCdAudioFunc = std::mem::transmute(base_address + 0x0005aa94);
                Some(hook_function(target, Self::stop_cd_audio)?)
            };

            *PLAY_CD_AUDIO_HOOK.write().unwrap() = {
                let target: PlayCdAudioFunc = std::mem::transmute(base_address + 0x0005aabe);
                Some(hook_function(target, Self::play_cd_audio)?)
            };

            *PAUSE_CD_AUDIO_HOOK.write().unwrap() = {
                let target: PauseCdAudioFunc = std::mem::transmute(base_address + 0x0005aa40);
                Some(hook_function(target, Self::pause_cd_audio)?)
            };

            *RESUME_CD_AUDIO_HOOK.write().unwrap() = {
                let target: ResumeCdAudioFunc = std::mem::transmute(base_address + 0x0005aa6a);
                Some(hook_function(target, Self::resume_cd_audio)?)
            };

            *START_CD_AUDIO_HOOK.write().unwrap() = {
                let target: StartCdAudioFunc = std::mem::transmute(base_address + 0x0005ab0b);
                Some(hook_function(target, Self::start_cd_audio)?)
            };

            *GET_CD_STATUS_HOOK.write().unwrap() = {
                let target: GetCdStatusFunc = std::mem::transmute(base_address + 0x0005ac33);
                Some(hook_function(target, Self::get_cd_status)?)
            };

            *GET_CD_AUDIO_TRACKS_HOOK.write().unwrap() = {
                let target: GetCdAudioTracksFunc = std::mem::transmute(base_address + 0x0005b49e);
                Some(hook_function(target, Self::get_cd_audio_tracks)?)
            };

            *GET_CD_AUDIO_POSITION_HOOK.write().unwrap() = {
                let target: GetCdAudioPositionFunc = std::mem::transmute(base_address + 0x0005b61d);
                Some(hook_function(target, Self::get_cd_audio_position)?)
            };

            *GET_CD_AUDIO_VOLUME_HOOK.write().unwrap() = {
                let target: GetCdAudioVolumeFunc = std::mem::transmute(base_address + 0x0005b6e5);
                Some(hook_function(target, Self::get_cd_audio_volume)?)
            };

            *SET_CD_AUDIO_VOLUME_HOOK.write().unwrap() = {
                let target: SetCdAudioVolumeFunc = std::mem::transmute(base_address + 0x0005b734);
                Some(hook_function(target, Self::set_cd_audio_volume)?)
            };

            *DEINIT_CD_AUDIO_HOOK.write().unwrap() = {
                let target: DeInitCdAudioFunc = std::mem::transmute(base_address + 0x0005abff);
                Some(hook_function(target, Self::deinit_cd_audio)?)
            };

            *UPDATE_CD_AUDIO_POSITION_HOOK.write().unwrap() = {
                let target: UpdateCdAudioPositionFunc =
                    std::mem::transmute(base_address + 0x0005af07);
                Some(hook_function(target, Self::update_cd_audio_position)?)
            };

            *CD_AUDIO_TOGGLE_PAUSED_HOOK.write().unwrap() = {
                let target: CdAudioTogglePausedFunc =
                    std::mem::transmute(base_address + 0x0005ad5e);
                Some(hook_function(target, Self::cd_audio_toggle_paused)?)
            };

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

    /// This function initializes the CD audio device for the game's background music.
    /// We hook it to work around bugs in modern Windows' MCI implementation.
    unsafe extern "stdcall" fn init_cd_audio() -> u32 {
        unsafe {
            let source =
                CdSource::from_str(&SETTINGS.get(Some("audio"), "cd_source").unwrap_or_default());

            if source != CdSource::Mci {
                let have_player = {
                    let mut guard = CD_AUDIO_PLAYER.lock().unwrap();
                    if guard.is_none() {
                        match CdAudioPlayer::new() {
                            Ok(p) => *guard = Some(p),
                            Err(e) => {
                                if source == CdSource::Files {
                                    tracing::error!("cd_source=files but player init failed: {e}");
                                } else {
                                    tracing::warn!("CD audio files unavailable, using MCI: {e}");
                                }
                            }
                        }
                    }
                    guard.is_some()
                };

                if have_player {
                    G_CD_AUDIO_DEVICE.set(1);
                    G_CD_AUDIO_AUX_DEVICE.set(Self::get_cd_audio_aux_device());
                    G_CD_AUDIO_INITIALIZED.set(1);
                    return 0;
                }
                if source == CdSource::Files {
                    // files were explicitly requested but unavailable
                    return 1;
                }
            }

            if CD_AUDIO_DEVICE != u32::MAX {
                G_CD_AUDIO_DEVICE.set(CD_AUDIO_DEVICE);
                G_CD_AUDIO_INITIALIZED.set(1);
                return 0;
            }

            let mut mci_open_parms = MCI_OPEN_PARMSA {
                lpstrDeviceType: s!("cdaudio"),
                ..Default::default()
            };
            let mci_open_error = mciSendCommandA(
                0,
                MCI_OPEN,
                Some(MCI_OPEN_TYPE as usize),
                Some(&mut mci_open_parms as *mut _ as usize),
            );
            if mci_open_error != 0 {
                return 1;
            }

            G_CD_AUDIO_DEVICE.set(mci_open_parms.wDeviceID);
            CD_AUDIO_DEVICE = G_CD_AUDIO_DEVICE.get();

            let mut mci_set_parms = MCI_SET_PARMS {
                dwTimeFormat: MCI_FORMAT_TMSF,
                ..Default::default()
            };
            let mci_set_error = mciSendCommandA(
                G_CD_AUDIO_DEVICE.get(),
                MCI_SET,
                Some(MCI_SET_TIME_FORMAT as usize),
                Some(&mut mci_set_parms as *mut _ as usize),
            );
            if mci_set_error != 0 {
                return 1;
            }

            G_CD_AUDIO_AUX_DEVICE.set(Self::get_cd_audio_aux_device());
            0
        }
    }

    unsafe extern "stdcall" fn get_cd_audio_aux_device() -> i32 {
        unsafe {
            if CD_AUDIO_PLAYER.lock().unwrap().is_some() {
                return 0;
            }

            GET_CD_AUDIO_AUX_DEVICE_HOOK
                .read()
                .unwrap()
                .as_ref()
                .unwrap()
                .call()
        }
    }

    /// Windows 11 was throwing an error if the CD device was closed.
    /// Now we just cache the device and re-use it between sim launches.
    /// This doesn't apply if we're playing music from files.
    unsafe extern "stdcall" fn close_cd_audio() -> i32 {
        if let Some(player) = CD_AUDIO_PLAYER.lock().unwrap().as_mut() {
            player.stop();
        }
        0
    }

    unsafe extern "stdcall" fn stop_cd_audio() {
        unsafe {
            if let Some(player) = CD_AUDIO_PLAYER.lock().unwrap().as_mut() {
                player.stop();
                return;
            }
            STOP_CD_AUDIO_HOOK.read().unwrap().as_ref().unwrap().call();
        }
    }

    unsafe extern "cdecl" fn play_cd_audio(from: u32, to: u32) {
        unsafe {
            if let Some(player) = CD_AUDIO_PLAYER.lock().unwrap().as_mut() {
                if let Err(e) = player.play_tmsf(from, to) {
                    tracing::error!("play_tmsf failed: {e}");
                }
                return;
            }

            let mut flags = MCI_FROM;

            let mut mci_play_parms = MCI_PLAY_PARMS {
                dwFrom: from,
                ..Default::default()
            };
            if to != 0 {
                flags = MCI_FROM | MCI_TO;
                mci_play_parms.dwTo = to;
            }
            let _ = mciSendCommandA(
                G_CD_AUDIO_DEVICE.get(),
                MCI_PLAY,
                Some(flags as usize),
                Some(&mut mci_play_parms as *mut _ as usize),
            );
        }
    }

    unsafe extern "stdcall" fn pause_cd_audio() {
        unsafe {
            if let Some(player) = CD_AUDIO_PLAYER.lock().unwrap().as_mut() {
                player.pause();
                return;
            }
            PAUSE_CD_AUDIO_HOOK.read().unwrap().as_ref().unwrap().call();
        }
    }

    unsafe extern "stdcall" fn resume_cd_audio() {
        unsafe {
            if let Some(player) = CD_AUDIO_PLAYER.lock().unwrap().as_mut() {
                player.resume();
                return;
            }
            RESUME_CD_AUDIO_HOOK
                .read()
                .unwrap()
                .as_ref()
                .unwrap()
                .call();
        }
    }

    unsafe extern "stdcall" fn start_cd_audio() -> i32 {
        unsafe {
            G_CD_AUDIO_GLOBAL_1.set(0);
            G_CD_AUDIO_GLOBAL_2.set(0);

            let init_cd_audio_result = Self::init_cd_audio();
            if init_cd_audio_result != 0 {
                return 0;
            }

            G_CD_AUDIO_INITIALIZED.set(1);

            G_AUDIO_CD_STATUS.set(Self::get_cd_status());

            match G_AUDIO_CD_STATUS.get() {
                AudioCdStatus::_Open => {
                    G_CD_AUDIO_INITIALIZED.set(0);
                }
                AudioCdStatus::Stopped => {
                    Self::get_cd_audio_tracks(G_CD_AUDIO_TRACK_DATA.ptr());
                }
                AudioCdStatus::Playing => {
                    Self::get_cd_audio_tracks(G_CD_AUDIO_TRACK_DATA.ptr());
                }
                AudioCdStatus::Paused => {
                    Self::get_cd_audio_tracks(G_CD_AUDIO_TRACK_DATA.ptr());
                    Self::get_cd_audio_position(G_PAUSED_CD_AUDIO_POSITION.ptr());
                }
                AudioCdStatus::Error | AudioCdStatus::_Unknown => {}
            }

            Self::set_cd_audio_volume(G_CD_AUDIO_VOLUME.get());
            1
        }
    }

    unsafe extern "cdecl" fn get_cd_status() -> AudioCdStatus {
        unsafe {
            if let Some(player) = CD_AUDIO_PLAYER.lock().unwrap().as_mut() {
                return player.state();
            }

            if G_CD_AUDIO_INITIALIZED.get() == 0 {
                return AudioCdStatus::Error;
            }

            let mut mci_status_parms = MCI_STATUS_PARMS {
                dwItem: MCI_STATUS_MODE as u32,
                ..Default::default()
            };
            let mci_status_error = mciSendCommandA(
                G_CD_AUDIO_DEVICE.get(),
                MCI_STATUS,
                Some(MCI_STATUS_ITEM as usize),
                Some(&mut mci_status_parms as *mut _ as usize),
            );
            if mci_status_error != 0 {
                Self::deinit_cd_audio();
                return AudioCdStatus::Error;
            }

            match mci_status_parms.dwReturn as u32 {
                // Some CD emulation software reports MCI_MODE_OPEN when stopped
                MCI_MODE_OPEN | MCI_MODE_STOP => AudioCdStatus::Stopped,
                MCI_MODE_PLAY => AudioCdStatus::Playing,
                MCI_MODE_PAUSE => AudioCdStatus::Paused,
                _ => AudioCdStatus::Error,
            }
        }
    }

    unsafe extern "cdecl" fn get_cd_audio_tracks(cd_audio_tracks: *mut CdAudioTracks) -> i32 {
        static mut TRACK_POSITIONS: [u32; MAX_TRACK as usize + 1] = [0; MAX_TRACK as usize + 1];

        unsafe {
            if let Some(player) = CD_AUDIO_PLAYER.lock().unwrap().as_mut() {
                let out = &mut *cd_audio_tracks;
                out.first_track = 1;
                out.number_of_tracks = player.tracks().last().map_or_default(|t| t.number);

                for track in out.first_track..=out.number_of_tracks + 1 {
                    let pos = CdAudioPosition {
                        track,
                        ..Default::default()
                    };
                    (*std::ptr::addr_of_mut!(TRACK_POSITIONS))
                        [(track - out.first_track) as usize] = pos.into();
                }

                out.track_positions = std::ptr::addr_of_mut!(TRACK_POSITIONS) as *mut u32;
                return 0;
            }

            GET_CD_AUDIO_TRACKS_HOOK
                .read()
                .unwrap()
                .as_ref()
                .unwrap()
                .call(cd_audio_tracks)
        }
    }

    unsafe extern "cdecl" fn get_cd_audio_position(cd_audio_position: *mut CdAudioPosition) {
        unsafe {
            if let Some(player) = CD_AUDIO_PLAYER.lock().unwrap().as_mut() {
                if !cd_audio_position.is_null() {
                    *cd_audio_position = player.cd_position();
                }
                return;
            }

            GET_CD_AUDIO_POSITION_HOOK
                .read()
                .unwrap()
                .as_ref()
                .unwrap()
                .call(cd_audio_position)
        }
    }

    unsafe extern "cdecl" fn get_cd_audio_volume() -> i32 {
        unsafe {
            if let Some(player) = CD_AUDIO_PLAYER.lock().unwrap().as_ref() {
                return player.volume();
            }

            GET_CD_AUDIO_VOLUME_HOOK
                .read()
                .unwrap()
                .as_ref()
                .unwrap()
                .call()
        }
    }

    unsafe extern "cdecl" fn set_cd_audio_volume(volume: i32) -> i32 {
        unsafe {
            if let Some(player) = CD_AUDIO_PLAYER.lock().unwrap().as_mut() {
                player.set_volume(volume);
                return 1;
            }

            SET_CD_AUDIO_VOLUME_HOOK
                .read()
                .unwrap()
                .as_ref()
                .unwrap()
                .call(volume)
        }
    }

    unsafe extern "stdcall" fn deinit_cd_audio() {
        unsafe {
            if CD_AUDIO_PLAYER.lock().unwrap().take().is_some() {
                G_CD_AUDIO_INITIALIZED.set(0);
                return;
            }

            DEINIT_CD_AUDIO_HOOK
                .read()
                .unwrap()
                .as_ref()
                .unwrap()
                .call();
        }
    }

    unsafe extern "cdecl" fn update_cd_audio_position(position: *mut CdAudioPosition) {
        unsafe {
            if G_CD_AUDIO_INITIALIZED.get() == 0 {
                return;
            }

            let cd_status = Self::get_cd_status();
            match cd_status {
                AudioCdStatus::_Open | AudioCdStatus::Stopped => {
                    *position = CdAudioPosition::default();
                }
                AudioCdStatus::Playing => {
                    Self::get_cd_audio_position(position);
                }
                AudioCdStatus::Paused => {
                    *position = G_PAUSED_CD_AUDIO_POSITION.get();
                }
                AudioCdStatus::Error | AudioCdStatus::_Unknown => {}
            }
        }
    }

    /// The game would reset to the first track of the CD when you unpause.
    /// This starts playback again from the saved pause position instead;
    unsafe extern "stdcall" fn cd_audio_toggle_paused() {
        unsafe {
            if G_CD_AUDIO_INITIALIZED.get() == 0 {
                return;
            }

            let cd_status = Self::get_cd_status();
            match cd_status {
                AudioCdStatus::_Open => {}
                AudioCdStatus::Stopped => {
                    let position = (G_PAUSED_CD_AUDIO_POSITION.get()).into();
                    Self::play_cd_audio(position, 0);
                }
                AudioCdStatus::Playing => {
                    Self::update_cd_audio_position(G_PAUSED_CD_AUDIO_POSITION.ptr());
                    Self::pause_cd_audio();
                }
                AudioCdStatus::Paused => {
                    Self::resume_cd_audio();
                }
                AudioCdStatus::Error | AudioCdStatus::_Unknown => {}
            }
        }
    }
}

impl Drop for Sim {
    fn drop(&mut self) {
        unsafe {
            ailrs::shutdown();
            crate::SIM_WINDOW_PROC = None;
            revert_groups(PATCH_GROUPS);

            INIT_CD_AUDIO_HOOK.write().unwrap().take();
            GET_CD_AUDIO_AUX_DEVICE_HOOK.write().unwrap().take();
            CLOSE_CD_AUDIO_HOOK.write().unwrap().take();
            STOP_CD_AUDIO_HOOK.write().unwrap().take();
            PLAY_CD_AUDIO_HOOK.write().unwrap().take();
            PAUSE_CD_AUDIO_HOOK.write().unwrap().take();
            RESUME_CD_AUDIO_HOOK.write().unwrap().take();
            START_CD_AUDIO_HOOK.write().unwrap().take();
            GET_CD_STATUS_HOOK.write().unwrap().take();
            GET_CD_AUDIO_TRACKS_HOOK.write().unwrap().take();
            GET_CD_AUDIO_POSITION_HOOK.write().unwrap().take();
            GET_CD_AUDIO_VOLUME_HOOK.write().unwrap().take();
            SET_CD_AUDIO_VOLUME_HOOK.write().unwrap().take();
            DEINIT_CD_AUDIO_HOOK.write().unwrap().take();
            UPDATE_CD_AUDIO_POSITION_HOOK.write().unwrap().take();
            CD_AUDIO_TOGGLE_PAUSED_HOOK.write().unwrap().take();

            self.ail.unhook();
            CD_AUDIO_PLAYER.lock().unwrap().take();
            MODULE.clear();
            FreeLibrary(self.module).unwrap();
        }
    }
}
