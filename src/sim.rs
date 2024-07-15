use std::{
    ffi::{c_char, c_void, CString},
    time::Instant,
};

use anyhow::{Context, Result};
use rand::Rng;
use retour::{GenericDetour, RawDetour};
use windows::{
    core::s,
    Win32::{
        Foundation::{
            FreeLibrary, BOOL, FALSE, HMODULE, HWND, LPARAM, LRESULT, RECT, TRUE, WPARAM,
        },
        Graphics::Gdi::BITMAPINFO,
        System::LibraryLoader::{GetProcAddress, LoadLibraryA},
        UI::WindowsAndMessaging::{
            DispatchMessageA, PeekMessageA, TranslateMessage, WaitMessage, MSG,
            PEEK_MESSAGE_REMOVE_TYPE, WM_QUIT,
        },
    },
};

use crate::{
    ail::Ail,
    common::{debug_log, fake_heap_free, HeapFreeFunc},
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
    Unknown = 0,
    Open = 1,
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
struct CdAudioPosition {
    track: u32,
    minute: u32,
    second: u32,
    frame: u32,
}

// Functions to hook
static mut DEBUG_LOG_HOOK: Option<RawDetour> = None;

type GameTickTimerCallbackFunc = unsafe extern "stdcall" fn(u32);
static mut GAME_TICK_TIMER_CALLBACK_HOOK: Option<GenericDetour<GameTickTimerCallbackFunc>> = None;

static mut SUP_ANIM_TIMER_CALLBACK_HOOK: Option<RawDetour> = None;

type IntegerOverflowHappensHereFunc = unsafe extern "cdecl" fn(i32, i32, i32) -> i32;
static mut INTEGER_OVERFLOW_HAPPENS_HERE_HOOK: Option<
    GenericDetour<IntegerOverflowHappensHereFunc>,
> = None;

type SetGameResolutionFunc = unsafe extern "cdecl" fn(*mut c_char);
static mut SET_GAME_RESOLUTION_HOOK: Option<GenericDetour<SetGameResolutionFunc>> = None;

type BlitFunc = unsafe extern "stdcall" fn();
static mut BLIT_HOOK: Option<GenericDetour<BlitFunc>> = None;

type InitCdAudioFunc = unsafe extern "stdcall" fn() -> u32;
static mut INIT_CD_AUDIO_HOOK: Option<GenericDetour<InitCdAudioFunc>> = None;

type GetGameCdNumberFunc = unsafe extern "stdcall" fn() -> i32;
static mut GET_GAME_CD_NUMBER_HOOK: Option<GenericDetour<GetGameCdNumberFunc>> = None;

type GetCdAudioAuxDeviceFunc = unsafe extern "stdcall" fn() -> i32;
static mut GET_CD_AUDIO_AUX_DEVICE_HOOK: Option<GenericDetour<GetCdAudioAuxDeviceFunc>> = None;

type CloseCdAudioFunc = unsafe extern "stdcall" fn() -> i32;
static mut CLOSE_CD_AUDIO_HOOK: Option<GenericDetour<CloseCdAudioFunc>> = None;

type PlayCdAudioFunc = unsafe extern "cdecl" fn(u32, u32);
static mut PLAY_CD_AUDIO_HOOK: Option<GenericDetour<PlayCdAudioFunc>> = None;

type PauseCdAudioFunc = unsafe extern "stdcall" fn();
static mut PAUSE_CD_AUDIO_HOOK: Option<GenericDetour<PauseCdAudioFunc>> = None;

type ResumeCdAudioFunc = unsafe extern "stdcall" fn();
static mut RESUME_CD_AUDIO_HOOK: Option<GenericDetour<ResumeCdAudioFunc>> = None;

type StartCdAudioFunc = unsafe extern "stdcall" fn() -> i32;
static mut START_CD_AUDIO_HOOK: Option<GenericDetour<StartCdAudioFunc>> = None;

type GetCdStatusFunc = unsafe extern "cdecl" fn() -> AudioCdStatus;
static mut GET_CD_STATUS_HOOK: Option<GenericDetour<GetCdStatusFunc>> = None;

type GetCdAudioTracksFunc = unsafe extern "cdecl" fn(*mut CdAudioTracks) -> i32;
static mut GET_CD_AUDIO_TRACKS_HOOK: Option<GenericDetour<GetCdAudioTracksFunc>> = None;

type GetCdAudioPositionFunc = unsafe extern "cdecl" fn(*mut CdAudioPosition);
static mut GET_CD_AUDIO_POSITION_HOOK: Option<GenericDetour<GetCdAudioPositionFunc>> = None;

type SetCdAudioVolumeFunc = unsafe extern "cdecl" fn(i32) -> i32;
static mut SET_CD_AUDIO_VOLUME_HOOK: Option<GenericDetour<SetCdAudioVolumeFunc>> = None;

type DeInitCdAudioFunc = unsafe extern "stdcall" fn();
static mut DE_INIT_CD_AUDIO_HOOK: Option<GenericDetour<DeInitCdAudioFunc>> = None;

type UpdateCdAudioPositionFunc = unsafe extern "cdecl" fn(*mut CdAudioPosition);
static mut UPDATE_CD_AUDIO_POSITION_HOOK: Option<GenericDetour<UpdateCdAudioPositionFunc>> = None;

type CdAudioTogglePausedFunc = unsafe extern "stdcall" fn();
static mut CD_AUDIO_TOGGLE_PAUSED_HOOK: Option<GenericDetour<CdAudioTogglePausedFunc>> = None;

type HandleMessagesFunc = unsafe extern "stdcall" fn();
static mut HANDLE_MESSAGES_HOOK: Option<GenericDetour<HandleMessagesFunc>> = None;

type RandomIntBelowFunc = unsafe extern "cdecl" fn(i32) -> i32;
static mut RANDOM_INT_BELOW_HOOK: Option<GenericDetour<RandomIntBelowFunc>> = None;

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
static mut G_CD_AUDIO_DEVICE: *mut i32 = std::ptr::null_mut();
static mut G_CD_AUDIO_AUX_DEVICE: *mut i32 = std::ptr::null_mut();
static mut G_CD_AUDIO_GLOBAL_1: *mut u32 = std::ptr::null_mut();
static mut G_CD_AUDIO_GLOBAL_2: *mut u32 = std::ptr::null_mut();
static mut G_CD_AUDIO_INITIALIZED: *mut u32 = std::ptr::null_mut();
static mut G_AUDIO_CD_STATUS: *mut AudioCdStatus = std::ptr::null_mut();
static mut G_CD_AUDIO_TRACK_DATA: *mut CdAudioTracks = std::ptr::null_mut();
static mut G_PAUSED_CD_AUDIO_POSITION: *mut CdAudioPosition = std::ptr::null_mut();
static mut G_CD_AUDIO_VOLUME: *mut i32 = std::ptr::null_mut();
static mut G_MESSAGES_HANDLED: *mut BOOL = std::ptr::null_mut();

pub struct Sim {
    _ail: Ail,
    module: HMODULE,
}

impl Sim {
    pub fn new() -> Result<Self> {
        let module = unsafe { LoadLibraryA(s!("MW2.DLL"))? };
        let base_address = module.0 as usize;

        let window_proc = unsafe {
            GetProcAddress(module, s!("SimWindowProc")).context("Couldn't find SimWindowProc")?
        };
        unsafe {
            crate::SIM_WINDOW_PROC = Some(std::mem::transmute::<
                *const (),
                unsafe extern "system" fn(HWND, u32, WPARAM, LPARAM) -> LRESULT,
            >(window_proc as *const ()));
        }

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
            G_CD_AUDIO_DEVICE = (base_address + 0x000aa278) as *mut i32;
            G_CD_AUDIO_AUX_DEVICE = (base_address + 0x000aa27c) as *mut i32;
            G_CD_AUDIO_GLOBAL_1 = (base_address + 0x000beca8) as *mut u32;
            G_CD_AUDIO_GLOBAL_2 = (base_address + 0x000becac) as *mut u32;
            G_CD_AUDIO_INITIALIZED = (base_address + 0x000aa28c) as *mut u32;
            G_AUDIO_CD_STATUS = (base_address + 0x000becc0) as *mut AudioCdStatus;
            G_CD_AUDIO_TRACK_DATA = (base_address + 0x000aa280) as *mut CdAudioTracks;
            G_PAUSED_CD_AUDIO_POSITION = (base_address + 0x000becb0) as *mut CdAudioPosition;
            G_CD_AUDIO_VOLUME = (base_address + 0x000a14a4) as *mut i32;
            G_MESSAGES_HANDLED = (base_address + 0x000acb18) as *mut BOOL;

            let heap_free_thunk = (base_address + 0x001834d0) as *mut HeapFreeFunc;
            *heap_free_thunk = fake_heap_free;

            DEBUG_LOG_HOOK = {
                let hook = RawDetour::new(
                    (base_address + 0x00017982) as *const (),
                    debug_log as *const (),
                )?;
                hook.enable()?;
                Some(hook)
            };

            GAME_TICK_TIMER_CALLBACK_HOOK = {
                let target: GameTickTimerCallbackFunc =
                    std::mem::transmute(base_address + 0x00067ed8);
                Some(hook_function(target, Self::game_tick_timer_callback)?)
            };

            SUP_ANIM_TIMER_CALLBACK_HOOK = {
                let hook = RawDetour::new(
                    (base_address + 0x00003f3d) as *const (),
                    Self::sup_anim_timer_callback as *const (),
                )?;
                hook.enable()?;
                Some(hook)
            };

            INTEGER_OVERFLOW_HAPPENS_HERE_HOOK = {
                let target: IntegerOverflowHappensHereFunc =
                    std::mem::transmute(base_address + 0x000035a0);
                Some(hook_function(target, Self::integer_overflow_happens_here)?)
            };

            SET_GAME_RESOLUTION_HOOK = {
                let target: SetGameResolutionFunc = std::mem::transmute(base_address + 0x00067e23);
                Some(hook_function(target, Self::set_game_resolution)?)
            };

            BLIT_HOOK = {
                let target: BlitFunc = std::mem::transmute(base_address + 0x00012e15);
                Some(hook_function(target, Self::blit)?)
            };

            // INIT_CD_AUDIO_HOOK = {
            //     let target: InitCdAudioFunc = std::mem::transmute(base_address + 0x0005a8b5);
            //     Some(hook_function(target, Self::init_cd_audio)?)
            // };

            // GET_GAME_CD_NUMBER_HOOK = {
            //     let target: GetGameCdNumberFunc = std::mem::transmute(base_address + 0x00002df5);
            //     Some(hook_function(target, Self::get_game_cd_number)?)
            // };

            HANDLE_MESSAGES_HOOK = {
                let target: HandleMessagesFunc = std::mem::transmute(base_address + 0x00067bbc);
                Some(hook_function(target, Self::handle_messages)?)
            };

            RANDOM_INT_BELOW_HOOK = {
                let target: RandomIntBelowFunc = std::mem::transmute(base_address + 0x000736b3);
                Some(hook_function(target, Self::random_int_below)?)
            };

            let _ail = Ail::new()?;

            Ok(Self { _ail, module })
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

        Ok(result)
    }

    unsafe extern "stdcall" fn game_tick_timer_callback(_: u32) {
        if *G_TICKS_CHECK & 0x200 == 0 {
            *G_TICKS_1 += 1;
        }
        if *G_TICKS_CHECK & 0x100 == 0 {
            *G_TICKS_2 += 1;
        }
    }

    unsafe extern "stdcall" fn sup_anim_timer_callback(_: u32) {
        let original: unsafe extern "stdcall" fn() =
            std::mem::transmute(SUP_ANIM_TIMER_CALLBACK_HOOK.as_ref().unwrap().trampoline());
        original();
    }

    unsafe extern "cdecl" fn integer_overflow_happens_here(a: i32, b: i32, c: i32) -> i32 {
        let a = a as i64;
        let b = b as i64;
        let c = c as i64;
        a.wrapping_mul(b).wrapping_div(c) as i32
    }

    unsafe extern "cdecl" fn set_game_resolution(resolution: *mut c_char) {
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

    unsafe extern "stdcall" fn blit() {
        static mut LAST_INSTANT: Option<Instant> = None;
        if LAST_INSTANT.is_none() {
            LAST_INSTANT = Some(Instant::now());
        }

        // Limit framerate to 45 FPS
        // Spin until 1/45th of a second has passed
        while LAST_INSTANT.unwrap().elapsed().as_secs_f64() < 1.0 / 45.0 {
            std::thread::yield_now();
        }
        LAST_INSTANT = Some(Instant::now());

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

    unsafe extern "stdcall" fn handle_messages() {
        if *G_WINDOW_ACTIVE == FALSE {
            let _ = WaitMessage();
        }

        if *G_MESSAGES_HANDLED == FALSE {
            let mut msg: MSG = MSG::default();
            let message_available = PeekMessageA(
                &mut msg as *mut MSG,
                HWND::default(),
                0,
                0,
                PEEK_MESSAGE_REMOVE_TYPE(1),
            );

            if message_available == TRUE {
                if msg.hwnd == HWND::default() || msg.message != WM_QUIT {
                    let _ = TranslateMessage(&msg);
                    DispatchMessageA(&msg);
                } else {
                    *G_MESSAGES_HANDLED = TRUE;
                }
            }
        }
    }

    unsafe extern "cdecl" fn random_int_below(max: i32) -> i32 {
        rand::thread_rng().gen_range(0..max)
    }
}

impl Drop for Sim {
    fn drop(&mut self) {
        unsafe {
            DEBUG_LOG_HOOK = None;
            GAME_TICK_TIMER_CALLBACK_HOOK = None;
            SUP_ANIM_TIMER_CALLBACK_HOOK = None;
            INTEGER_OVERFLOW_HAPPENS_HERE_HOOK = None;
            SET_GAME_RESOLUTION_HOOK = None;
            BLIT_HOOK = None;

            HANDLE_MESSAGES_HOOK = None;
            RANDOM_INT_BELOW_HOOK = None;
            FreeLibrary(self.module).unwrap();
        }
    }
}
