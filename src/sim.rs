use std::{
    ffi::{CString, c_char, c_void},
    sync::RwLock,
    time::Instant,
};

use anyhow::{Context, Result, bail};
use rand::Rng;
use retour::{GenericDetour, RawDetour};
use windows::{
    Win32::{
        Foundation::{FALSE, FreeLibrary, HMODULE, HWND, RECT, TRUE},
        Graphics::Gdi::BITMAPINFO,
        Media::Multimedia::{
            MCI_FORMAT_TMSF, MCI_FROM, MCI_MODE_OPEN, MCI_MODE_PAUSE, MCI_MODE_PLAY, MCI_MODE_STOP,
            MCI_OPEN, MCI_OPEN_PARMSA, MCI_OPEN_TYPE, MCI_PLAY, MCI_PLAY_PARMS, MCI_SET,
            MCI_SET_PARMS, MCI_SET_TIME_FORMAT, MCI_STATUS, MCI_STATUS_ITEM, MCI_STATUS_MODE,
            MCI_STATUS_PARMS, MCI_TO, mciSendCommandA,
        },
        System::LibraryLoader::{GetProcAddress, LoadLibraryA},
        UI::WindowsAndMessaging::{
            DispatchMessageA, MSG, PM_REMOVE, PeekMessageA, TranslateMessage, WM_QUIT, WaitMessage,
        },
    },
    core::{BOOL, s},
};

use crate::{
    WindowProc,
    ail::Ail,
    common::{HeapFreeFunc, debug_log, fake_heap_free},
    hooker::hook_function,
};

type SimMainProc = unsafe extern "stdcall" fn(
    HMODULE,
    u32,
    *const c_char,
    *const *const c_void,
    BOOL,
    HWND,
) -> i32;

#[repr(C)]
struct SomeDDrawStruct {
    rect: *mut RECT,
    unknown1: i32,
    unknown2: i32,
    bitmap_info: *mut *mut BITMAPINFO,
    unknown3: u32,
}

#[repr(C)]
#[derive(Clone)]
struct WeirdRectStruct {
    some_ddraw_struct: *mut SomeDDrawStruct,
    x1: i32,
    y2: i32,
    x2: i32,
    y1: i32,
}

type DrawModeInitFunc = unsafe extern "cdecl" fn(*mut SomeDDrawStruct, i32, i32) -> i32;
type DrawModeDeInitFunc = unsafe extern "cdecl" fn() -> i32;
type DrawModeBlitFlipFunc = unsafe extern "cdecl" fn() -> i32;
type DrawModeBlitRectFunc = unsafe extern "cdecl" fn(i32, i32, i32, i32) -> i32;
type DrawModeStretchBlitFunc = unsafe extern "cdecl" fn(i32, i32, i32, i32) -> i32;

#[repr(C)]
struct DrawMode {
    index: u32,
    some_index_to_related_struct: i32,
    initialized: i32,
    unknown1: u32,
    init_func: DrawModeInitFunc,
    deinit_func: DrawModeDeInitFunc,
    blit_flip_func: DrawModeBlitFlipFunc,
    blit_rect_func: DrawModeBlitRectFunc,
    stretch_blit_func: DrawModeStretchBlitFunc,
    unknown2: u32,
}

#[repr(C)]
enum AudioCdStatus {
    _Unknown = 0,
    _Open = 1,
    Stopped = 2,
    Playing = 3,
    Paused = 4,
    Error = 5,
}

#[repr(C)]
struct CdAudioTracks {
    first_track: u32,
    number_of_tracks: u32,
    track_positions: *mut u32,
}

#[repr(C)]
#[derive(Clone, Default)]
struct CdAudioPosition {
    track: u32,
    minute: u32,
    second: u32,
    frame: u32,
}

impl From<CdAudioPosition> for u32 {
    fn from(position: CdAudioPosition) -> u32 {
        position.track
            + ((position.minute & 0xFF) << 8)
            + ((position.second & 0xFF) << 16)
            + (position.frame << 24)
    }
}

// Functions to hook
static DEBUG_LOG_HOOK: RwLock<Option<RawDetour>> = RwLock::new(None);

type GameTickTimerCallbackFunc = unsafe extern "stdcall" fn(u32);
static GAME_TICK_TIMER_CALLBACK_HOOK: RwLock<Option<GenericDetour<GameTickTimerCallbackFunc>>> =
    RwLock::new(None);

static SUP_ANIM_TIMER_CALLBACK_HOOK: RwLock<Option<RawDetour>> = RwLock::new(None);

type IntegerOverflowHappensHereFunc = unsafe extern "cdecl" fn(i32, i32, i32) -> i32;
static INTEGER_OVERFLOW_HAPPENS_HERE_HOOK: RwLock<
    Option<GenericDetour<IntegerOverflowHappensHereFunc>>,
> = RwLock::new(None);

type SetGameResolutionFunc = unsafe extern "cdecl" fn(*mut c_char);
static SET_GAME_RESOLUTION_HOOK: RwLock<Option<GenericDetour<SetGameResolutionFunc>>> =
    RwLock::new(None);

type BlitFunc = unsafe extern "stdcall" fn();
static BLIT_HOOK: RwLock<Option<GenericDetour<BlitFunc>>> = RwLock::new(None);

type InitCdAudioFunc = unsafe extern "stdcall" fn() -> u32;
static INIT_CD_AUDIO_HOOK: RwLock<Option<GenericDetour<InitCdAudioFunc>>> = RwLock::new(None);

type GetCdAudioAuxDeviceFunc = unsafe extern "stdcall" fn() -> i32;
static GET_CD_AUDIO_AUX_DEVICE_HOOK: RwLock<Option<GenericDetour<GetCdAudioAuxDeviceFunc>>> =
    RwLock::new(None);

type CloseCdAudioFunc = unsafe extern "stdcall" fn() -> i32;
static CLOSE_CD_AUDIO_HOOK: RwLock<Option<GenericDetour<CloseCdAudioFunc>>> = RwLock::new(None);

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

type HandleMessagesFunc = unsafe extern "stdcall" fn();
static HANDLE_MESSAGES_HOOK: RwLock<Option<GenericDetour<HandleMessagesFunc>>> = RwLock::new(None);

type RandomIntBelowFunc = unsafe extern "cdecl" fn(i32) -> i32;
static RANDOM_INT_BELOW_HOOK: RwLock<Option<GenericDetour<RandomIntBelowFunc>>> = RwLock::new(None);

// Global variables
static mut G_TICKS_CHECK: *mut u32 = std::ptr::null_mut();
static mut G_TICKS_1: *mut u32 = std::ptr::null_mut();
static mut G_TICKS_2: *mut u32 = std::ptr::null_mut();
static mut G_GAME_WINDOW_WIDTH: *mut u32 = std::ptr::null_mut();
static mut G_GAME_WINDOW_HEIGHT: *mut u32 = std::ptr::null_mut();
static mut G_BLIT_GLOBAL_1: *mut BOOL = std::ptr::null_mut();
static mut G_WINDOW_ACTIVE: *mut BOOL = std::ptr::null_mut();
static mut G_CURRENT_DRAW_MODE: *mut *mut DrawMode = std::ptr::null_mut();
static mut G_STRETCH_BLIT_SOURCE_RECT: *mut WeirdRectStruct = std::ptr::null_mut();
static mut G_STRETCH_BLIT_OTHER_SOURCE_RECT: *mut WeirdRectStruct = std::ptr::null_mut();
static mut G_BLIT_GLOBAL_2: *mut u32 = std::ptr::null_mut();
static mut G_BLIT_GLOBAL_3: *mut u32 = std::ptr::null_mut();
static mut G_WIDTH_SCALE: *mut i32 = std::ptr::null_mut();
static mut G_SOME_POINTER: *mut *mut i32 = std::ptr::null_mut();
static mut G_CD_AUDIO_DEVICE: *mut u32 = std::ptr::null_mut();
static mut G_CD_AUDIO_AUX_DEVICE: *mut i32 = std::ptr::null_mut();
static mut G_CD_AUDIO_GLOBAL_1: *mut u32 = std::ptr::null_mut();
static mut G_CD_AUDIO_GLOBAL_2: *mut u32 = std::ptr::null_mut();
static mut G_CD_AUDIO_INITIALIZED: *mut u32 = std::ptr::null_mut();
static mut G_AUDIO_CD_STATUS: *mut AudioCdStatus = std::ptr::null_mut();
static mut G_CD_AUDIO_TRACK_DATA: *mut CdAudioTracks = std::ptr::null_mut();
static mut G_PAUSED_CD_AUDIO_POSITION: *mut CdAudioPosition = std::ptr::null_mut();
static mut G_CD_AUDIO_VOLUME: *mut i32 = std::ptr::null_mut();
static mut G_SHOULD_QUIT: *mut BOOL = std::ptr::null_mut();

/// Cache the CD audio device to reuse between sim launches.
/// Windows 11 crashes if we try to close the CD audio device.
static mut CD_AUDIO_DEVICE: u32 = u32::MAX;

static mut LOADED: bool = false;

pub struct Sim {
    ail: Ail,
    module: HMODULE,
}

impl Sim {
    pub fn new() -> Result<Self> {
        if unsafe { LOADED } {
            bail!("Can't load shell more than once");
        }

        let module = unsafe { LoadLibraryA(s!("MW2.DLL"))? };
        let base_address = module.0 as usize;

        unsafe {
            G_TICKS_CHECK = (base_address + 0x000ad008) as *mut u32;
            G_TICKS_1 = (base_address + 0x000ad20c) as *mut u32;
            G_TICKS_2 = (base_address + 0x000ad210) as *mut u32;
            G_GAME_WINDOW_WIDTH = (base_address + 0x000acb6c) as *mut u32;
            G_GAME_WINDOW_HEIGHT = (base_address + 0x000acb70) as *mut u32;
            G_BLIT_GLOBAL_1 = (base_address + 0x00176ebc) as *mut BOOL;
            G_WINDOW_ACTIVE = (base_address + 0x000acb74) as *mut BOOL;
            G_CURRENT_DRAW_MODE = (base_address + 0x000b1774) as *mut *mut DrawMode;
            G_STRETCH_BLIT_SOURCE_RECT = (base_address + 0x00176ed0) as *mut WeirdRectStruct;
            G_STRETCH_BLIT_OTHER_SOURCE_RECT = (base_address + 0x000bdff8) as *mut WeirdRectStruct;
            G_BLIT_GLOBAL_2 = (base_address + 0x000a5f18) as *mut u32;
            G_BLIT_GLOBAL_3 = (base_address + 0x000a5a24) as *mut u32;
            G_WIDTH_SCALE = (base_address + 0x000e9610) as *mut i32;
            G_SOME_POINTER = (base_address + 0x000a6cc0) as *mut *mut i32;
            G_CD_AUDIO_DEVICE = (base_address + 0x000aa278) as *mut u32;
            G_CD_AUDIO_AUX_DEVICE = (base_address + 0x000aa27c) as *mut i32;
            G_CD_AUDIO_GLOBAL_1 = (base_address + 0x000beca8) as *mut u32;
            G_CD_AUDIO_GLOBAL_2 = (base_address + 0x000becac) as *mut u32;
            G_CD_AUDIO_INITIALIZED = (base_address + 0x000aa28c) as *mut u32;
            G_AUDIO_CD_STATUS = (base_address + 0x000becc0) as *mut AudioCdStatus;
            G_CD_AUDIO_TRACK_DATA = (base_address + 0x000aa280) as *mut CdAudioTracks;
            G_PAUSED_CD_AUDIO_POSITION = (base_address + 0x000becb0) as *mut CdAudioPosition;
            G_CD_AUDIO_VOLUME = (base_address + 0x000a14a4) as *mut i32;
            G_SHOULD_QUIT = (base_address + 0x000acb18) as *mut BOOL;

            let heap_free_thunk = (base_address + 0x001834d0) as *mut HeapFreeFunc;
            *heap_free_thunk = fake_heap_free;

            *DEBUG_LOG_HOOK.write().unwrap() = {
                let hook = RawDetour::new(
                    (base_address + 0x00017982) as *const (),
                    debug_log as *const (),
                )?;
                hook.enable()?;
                Some(hook)
            };

            *GAME_TICK_TIMER_CALLBACK_HOOK.write().unwrap() = {
                let target: GameTickTimerCallbackFunc =
                    std::mem::transmute(base_address + 0x00067ed8);
                Some(hook_function(target, Self::game_tick_timer_callback)?)
            };

            *SUP_ANIM_TIMER_CALLBACK_HOOK.write().unwrap() = {
                let hook = RawDetour::new(
                    (base_address + 0x00003f3d) as *const (),
                    Self::sup_anim_timer_callback as *const (),
                )?;
                hook.enable()?;
                Some(hook)
            };

            *INTEGER_OVERFLOW_HAPPENS_HERE_HOOK.write().unwrap() = {
                let target: IntegerOverflowHappensHereFunc =
                    std::mem::transmute(base_address + 0x000035a0);
                Some(hook_function(target, Self::integer_overflow_happens_here)?)
            };

            *SET_GAME_RESOLUTION_HOOK.write().unwrap() = {
                let target: SetGameResolutionFunc = std::mem::transmute(base_address + 0x00067e23);
                Some(hook_function(target, Self::set_game_resolution)?)
            };

            *BLIT_HOOK.write().unwrap() = {
                let target: BlitFunc = std::mem::transmute(base_address + 0x00012e15);
                Some(hook_function(target, Self::blit)?)
            };

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

            *HANDLE_MESSAGES_HOOK.write().unwrap() = {
                let target: HandleMessagesFunc = std::mem::transmute(base_address + 0x00067bbc);
                Some(hook_function(target, Self::handle_messages)?)
            };

            *RANDOM_INT_BELOW_HOOK.write().unwrap() = {
                let target: RandomIntBelowFunc = std::mem::transmute(base_address + 0x000736b3);
                Some(hook_function(target, Self::random_int_below)?)
            };

            let ail = Ail::new()?;

            LOADED = true;

            Ok(Self { ail, module })
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

    /// This is the callback executed by Miles Sound System (AIL) with a 181Hz timer to update the game's internal ticks.
    /// The game uses these ticks to update the game state, including calculating delta time (ticks) between frames.
    /// The original function was corrupting the stack with my custom AIL time_proc.
    /// Replacing it with this freshly recompiled copy fixed the problem (for now?)
    unsafe extern "stdcall" fn game_tick_timer_callback(_: u32) {
        unsafe {
            if *G_TICKS_CHECK & 0x200 == 0 {
                *G_TICKS_1 += 1;
            }
            if *G_TICKS_CHECK & 0x100 == 0 {
                *G_TICKS_2 += 1;
            }
        }
    }

    /// I'm not sure what exactly this does yet, but it's related to the loading screen with the dropship: "sup anim"
    /// This is the same situation as the tick timer callback: Hooking it to avoid stack corruption.
    unsafe extern "stdcall" fn sup_anim_timer_callback(_: u32) {
        unsafe {
            let original: unsafe extern "stdcall" fn() = std::mem::transmute(
                SUP_ANIM_TIMER_CALLBACK_HOOK
                    .read()
                    .unwrap()
                    .as_ref()
                    .unwrap()
                    .trampoline(),
            );
            original();
        }
    }

    /// This function is used all over the game to perform ((a * b) / c).
    /// It sometimes overflows and sometimes divides by zero, especially when the FPS is too high.
    ///
    /// TODO: Fix that, probably.
    unsafe extern "cdecl" fn integer_overflow_happens_here(a: i32, b: i32, c: i32) -> i32 {
        let a = a as i64;
        let b = b as i64;
        let c = c as i64;
        a.wrapping_mul(b).wrapping_div(c) as i32
    }

    /// The game decides which resolution to use based on the DLL name passed to this function.
    /// This is presumably a leftover from the DOS version of the game, possibly to preserve config file compatibility.
    unsafe extern "cdecl" fn set_game_resolution(resolution: *mut c_char) {
        unsafe {
            // "MCGA.DLL"
            *G_GAME_WINDOW_WIDTH = 320;
            *G_GAME_WINDOW_HEIGHT = 200;

            let resolution = std::ffi::CStr::from_ptr(resolution)
                .to_str()
                .unwrap()
                .to_uppercase();
            if resolution == "VESA480.DLL" {
                *G_GAME_WINDOW_WIDTH = 640;
                *G_GAME_WINDOW_HEIGHT = 480;
            } else if resolution == "VESA768.DLL" {
                *G_GAME_WINDOW_WIDTH = 1024;
                *G_GAME_WINDOW_HEIGHT = 768;
            }
        }
    }

    /// This function is called every frame to draw the game.
    /// For now, we hook it in order to limit the framerate to a resonable 45 FPS.
    /// Any higher and your jumpjet fuel will not recharge reliably and if unconstrained, the game's physics will break.
    unsafe extern "stdcall" fn blit() {
        unsafe {
            static LAST_INSTANT: RwLock<Option<Instant>> = RwLock::new(None);

            {
                let mut last_instant = LAST_INSTANT.write().unwrap();
                if last_instant.is_none() {
                    *last_instant = Some(Instant::now());
                }

                // Limit framerate to 45 FPS
                // Spin until 1/45th of a second has passed
                while last_instant.unwrap().elapsed().as_secs_f64() < 1.0 / 45.0 {
                    std::thread::yield_now();
                }
                *last_instant = Some(Instant::now());
            }

            if *G_BLIT_GLOBAL_1 == FALSE {
                if *G_WINDOW_ACTIVE == TRUE {
                    ((**G_CURRENT_DRAW_MODE).blit_flip_func)();
                }
            } else {
                ((**G_CURRENT_DRAW_MODE).stretch_blit_func)(
                    (*G_STRETCH_BLIT_SOURCE_RECT).x1 + 1,
                    (*G_STRETCH_BLIT_SOURCE_RECT).y2 + 1,
                    (*G_STRETCH_BLIT_SOURCE_RECT).x2,
                    (*G_STRETCH_BLIT_SOURCE_RECT).y1,
                );

                *G_STRETCH_BLIT_SOURCE_RECT = (*G_STRETCH_BLIT_OTHER_SOURCE_RECT).clone();

                *G_BLIT_GLOBAL_2 = *G_BLIT_GLOBAL_3;
                *G_BLIT_GLOBAL_1 = FALSE;
            }
        }
    }

    /// This function initializes the CD audio device for the game's background music.
    /// We hook it to work around bugs in modern Windows' MCI implementation.
    unsafe extern "stdcall" fn init_cd_audio() -> u32 {
        unsafe {
            if CD_AUDIO_DEVICE != u32::MAX {
                *G_CD_AUDIO_DEVICE = CD_AUDIO_DEVICE;
                *G_CD_AUDIO_INITIALIZED = 1;
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

            *G_CD_AUDIO_DEVICE = mci_open_parms.wDeviceID;
            CD_AUDIO_DEVICE = *G_CD_AUDIO_DEVICE;

            let mut mci_set_parms = MCI_SET_PARMS {
                dwTimeFormat: MCI_FORMAT_TMSF,
                ..Default::default()
            };
            let mci_set_error = mciSendCommandA(
                *G_CD_AUDIO_DEVICE,
                MCI_SET,
                Some(MCI_SET_TIME_FORMAT as usize),
                Some(&mut mci_set_parms as *mut _ as usize),
            );
            if mci_set_error != 0 {
                return 1;
            }

            *G_CD_AUDIO_AUX_DEVICE = Self::get_cd_audio_aux_device();
            0
        }
    }

    unsafe extern "stdcall" fn get_cd_audio_aux_device() -> i32 {
        unsafe {
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
    unsafe extern "stdcall" fn close_cd_audio() -> i32 {
        0
    }

    unsafe extern "cdecl" fn play_cd_audio(from: u32, to: u32) {
        unsafe {
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
                *G_CD_AUDIO_DEVICE,
                MCI_PLAY,
                Some(flags as usize),
                Some(&mut mci_play_parms as *mut _ as usize),
            );
        }
    }

    unsafe extern "stdcall" fn pause_cd_audio() {
        unsafe {
            PAUSE_CD_AUDIO_HOOK.read().unwrap().as_ref().unwrap().call();
        }
    }

    unsafe extern "stdcall" fn resume_cd_audio() {
        unsafe {
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
            *G_CD_AUDIO_GLOBAL_1 = 0;
            *G_CD_AUDIO_GLOBAL_2 = 0;

            let init_cd_audio_result = Self::init_cd_audio();
            if init_cd_audio_result != 0 {
                return 0;
            }

            *G_CD_AUDIO_INITIALIZED = 1;

            *G_AUDIO_CD_STATUS = Self::get_cd_status();

            match *G_AUDIO_CD_STATUS {
                AudioCdStatus::_Open => {
                    *G_CD_AUDIO_INITIALIZED = 0;
                }
                AudioCdStatus::Stopped => {
                    Self::get_cd_audio_tracks(G_CD_AUDIO_TRACK_DATA);
                }
                AudioCdStatus::Playing => {
                    Self::get_cd_audio_tracks(G_CD_AUDIO_TRACK_DATA);
                }
                AudioCdStatus::Paused => {
                    Self::get_cd_audio_tracks(G_CD_AUDIO_TRACK_DATA);
                    Self::get_cd_audio_position(G_PAUSED_CD_AUDIO_POSITION);
                }
                AudioCdStatus::Error | AudioCdStatus::_Unknown => {}
            }

            Self::set_cd_audio_volume(*G_CD_AUDIO_VOLUME);
            1
        }
    }

    unsafe extern "cdecl" fn get_cd_status() -> AudioCdStatus {
        unsafe {
            if *G_CD_AUDIO_INITIALIZED == 0 {
                return AudioCdStatus::Error;
            }

            let mut mci_status_parms = MCI_STATUS_PARMS {
                dwItem: MCI_STATUS_MODE as u32,
                ..Default::default()
            };
            let mci_status_error = mciSendCommandA(
                *G_CD_AUDIO_DEVICE,
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
        unsafe {
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
            GET_CD_AUDIO_POSITION_HOOK
                .read()
                .unwrap()
                .as_ref()
                .unwrap()
                .call(cd_audio_position)
        }
    }

    unsafe extern "cdecl" fn set_cd_audio_volume(volume: i32) -> i32 {
        unsafe {
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
            if *G_CD_AUDIO_INITIALIZED == 0 {
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
                    *position = (*G_PAUSED_CD_AUDIO_POSITION).clone();
                }
                AudioCdStatus::Error | AudioCdStatus::_Unknown => {}
            }
        }
    }

    /// The game would reset to the first track of the CD when you unpause.
    /// This starts playback again from the saved pause position instead;
    unsafe extern "stdcall" fn cd_audio_toggle_paused() {
        unsafe {
            if *G_CD_AUDIO_INITIALIZED == 0 {
                return;
            }

            let cd_status = Self::get_cd_status();
            match cd_status {
                AudioCdStatus::_Open => {}
                AudioCdStatus::Stopped => {
                    let position = (*G_PAUSED_CD_AUDIO_POSITION).clone().into();
                    Self::play_cd_audio(position, 0);
                }
                AudioCdStatus::Playing => {
                    Self::update_cd_audio_position(G_PAUSED_CD_AUDIO_POSITION);
                    Self::pause_cd_audio();
                }
                AudioCdStatus::Paused => {
                    Self::resume_cd_audio();
                }
                AudioCdStatus::Error | AudioCdStatus::_Unknown => {}
            }
        }
    }

    /// The original function had a loop that was causing bad stuttering when the mouse was moved.
    unsafe extern "stdcall" fn handle_messages() {
        unsafe {
            if *G_WINDOW_ACTIVE == FALSE {
                let _ = WaitMessage();
            }

            if *G_SHOULD_QUIT == FALSE {
                let mut msg: MSG = MSG::default();

                if PeekMessageA(&mut msg as *mut MSG, Some(HWND::default()), 0, 0, PM_REMOVE).into()
                {
                    if msg.hwnd == HWND::default() || msg.message != WM_QUIT {
                        let _ = TranslateMessage(&msg);
                        DispatchMessageA(&msg);
                    } else {
                        *G_SHOULD_QUIT = TRUE;
                    }
                }
            }
        }
    }

    /// Returns a truly pseudorandom number instead of picking from the pregenerated table.
    /// This fixes the chance to explode if you're overheating because the pregenerated random
    /// numbers had a chance to never return a number < 3 when modulo with a fixed DeltaTime.
    ///
    /// TODO: This could break multiplayer. Look into another solution if it causes desync.
    unsafe extern "cdecl" fn random_int_below(max: i32) -> i32 {
        rand::rng().random_range(0..max)
    }
}

impl Drop for Sim {
    fn drop(&mut self) {
        unsafe {
            crate::SIM_WINDOW_PROC = None;
            DEBUG_LOG_HOOK.write().unwrap().take();
            GAME_TICK_TIMER_CALLBACK_HOOK.write().unwrap().take();
            SUP_ANIM_TIMER_CALLBACK_HOOK.write().unwrap().take();
            INTEGER_OVERFLOW_HAPPENS_HERE_HOOK.write().unwrap().take();
            SET_GAME_RESOLUTION_HOOK.write().unwrap().take();
            BLIT_HOOK.write().unwrap().take();
            INIT_CD_AUDIO_HOOK.write().unwrap().take();
            GET_CD_AUDIO_AUX_DEVICE_HOOK.write().unwrap().take();
            CLOSE_CD_AUDIO_HOOK.write().unwrap().take();
            PLAY_CD_AUDIO_HOOK.write().unwrap().take();
            PAUSE_CD_AUDIO_HOOK.write().unwrap().take();
            RESUME_CD_AUDIO_HOOK.write().unwrap().take();
            START_CD_AUDIO_HOOK.write().unwrap().take();
            GET_CD_STATUS_HOOK.write().unwrap().take();
            GET_CD_AUDIO_TRACKS_HOOK.write().unwrap().take();
            GET_CD_AUDIO_POSITION_HOOK.write().unwrap().take();
            SET_CD_AUDIO_VOLUME_HOOK.write().unwrap().take();
            DEINIT_CD_AUDIO_HOOK.write().unwrap().take();
            UPDATE_CD_AUDIO_POSITION_HOOK.write().unwrap().take();
            CD_AUDIO_TOGGLE_PAUSED_HOOK.write().unwrap().take();
            HANDLE_MESSAGES_HOOK.write().unwrap().take();
            RANDOM_INT_BELOW_HOOK.write().unwrap().take();
            self.ail.unhook();
            FreeLibrary(self.module).unwrap();
            LOADED = false;
        }
    }
}
