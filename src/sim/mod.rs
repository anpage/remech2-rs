use std::{
    ffi::{CString, c_char, c_void},
    sync::{Mutex, RwLock},
    time::Instant,
};

use anyhow::{Context, Result, bail};
use rand::Rng;
use retour::{GenericDetour, RawDetour};
use windows::{
    Win32::{
        Foundation::{FALSE, FreeLibrary, HMODULE, HWND, TRUE},
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
    sim::drawmode::hooks::PixelBuffer,
};

pub mod drawmode;

type SimMainProc = unsafe extern "stdcall" fn(
    HMODULE,
    u32,
    *const c_char,
    *const *const c_void,
    BOOL,
    HWND,
) -> i32;

/// A render context: the pixel buffer to draw into plus the rect within that buffer
/// where all the drawing happens.
#[repr(C)]
#[derive(Clone)]
struct RenderTarget {
    pixel_buffer: *mut PixelBuffer,
    left: i32,
    top: i32,
    right: i32,
    bottom: i32,
}

impl RenderTarget {
    /// Widens the target rect to the whole framebuffer
    fn cover_framebuffer(&mut self) {
        unsafe {
            self.left = 0;
            self.top = 0;
            self.right = (*G_GAME_WINDOW_WIDTH as i32 - 1).max(0);
            self.bottom = (*G_GAME_WINDOW_HEIGHT as i32 - 1).max(0);
        }
    }
}

type DrawModeInitFunc = unsafe extern "cdecl" fn(*mut PixelBuffer, i32, i32) -> i32;
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
struct CdAudioTracks {
    first_track: u32,
    number_of_tracks: u32,
    track_positions: *mut u32,
}

#[repr(C)]
struct GameWindowGeometry {
    width: i32,
    height: i32,
    unknown1: i32,
    unknown2: i32,
    unknown3: i32,
    unknown4: i32,
}

/// The camera, which the game calls "Eyepoint"
#[repr(C)]
struct Eyepoint {
    position: [i32; 3],
    rotation: [i32; 3],
    /// Horizontal FOV in 16.16, where `tan(fov / 2) == 1 / fov_x`
    fov_x: i32,
    unknown1: [i32; 4],
    viewport_left: i32,
    viewport_right: i32,
    viewport_top: i32,
    viewport_bottom: i32,
    hither_clip_plane: i32,
    yon_clip_plane: i32,
    /// `pixel width / pixel height` in 16.16
    pixel_aspect_ratio: i32,
}

/// The cockpit layout table passed to LoadCockpitLayout. It runs to at least
/// index 0x22; only the entries the detour needs are named.
#[repr(C)]
struct CockpitLayout {
    /// The cockpit's 3D viewport, in normalised 16.16 coordinates
    viewport: *mut RenderTarget,
    unknown1: *mut c_void,
    /// Render target table slot the viewport is copied into
    render_target_slot: i32,
}

/// A 2D point with normalised 16.16 components
#[repr(C)]
#[derive(Clone, Copy)]
struct Point {
    x: i32,
    y: i32,
}

// Functions to hook
// static DEBUG_LOG_HOOK: RwLock<Option<RawDetour>> = RwLock::new(None);

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

type InitGameWindowGeometryFunc = unsafe extern "cdecl" fn() -> i32;
static INIT_GAME_WINDOW_GEOMETRY_HOOK: RwLock<Option<GenericDetour<InitGameWindowGeometryFunc>>> =
    RwLock::new(None);

type ScaleRectToScreenFunc = unsafe extern "cdecl" fn(
    *mut PixelBuffer,
    *mut RenderTarget,
    *mut RenderTarget,
) -> *mut RenderTarget;
static SCALE_RECT_TO_SCREEN_HOOK: RwLock<Option<GenericDetour<ScaleRectToScreenFunc>>> =
    RwLock::new(None);

type ScalePointToScreenFunc =
    unsafe extern "cdecl" fn(*mut PixelBuffer, *mut Point, *mut Point) -> *mut Point;
static SCALE_POINT_TO_SCREEN_HOOK: RwLock<Option<GenericDetour<ScalePointToScreenFunc>>> =
    RwLock::new(None);

type CenterRectOnScreenFunc = unsafe extern "cdecl" fn(
    *mut PixelBuffer,
    *mut RenderTarget,
    *mut RenderTarget,
) -> *mut RenderTarget;
static CENTER_RECT_ON_SCREEN_HOOK: RwLock<Option<GenericDetour<CenterRectOnScreenFunc>>> =
    RwLock::new(None);

type ApplyEyepointFovFunc = unsafe extern "cdecl" fn(i32);
static APPLY_EYEPOINT_FOV_HOOK: RwLock<Option<GenericDetour<ApplyEyepointFovFunc>>> =
    RwLock::new(None);

type LoadCockpitLayoutFunc = unsafe extern "cdecl" fn(i32, *mut CockpitLayout);
static LOAD_COCKPIT_LAYOUT_HOOK: RwLock<Option<GenericDetour<LoadCockpitLayoutFunc>>> =
    RwLock::new(None);

type SelectRenderTargetFunc = unsafe extern "cdecl" fn(i32);
static SELECT_RENDER_TARGET_HOOK: RwLock<Option<GenericDetour<SelectRenderTargetFunc>>> =
    RwLock::new(None);

type SetupEyepointProjectionFunc = unsafe extern "cdecl" fn(*mut Eyepoint);
static SETUP_EYEPOINT_PROJECTION_HOOK: RwLock<Option<GenericDetour<SetupEyepointProjectionFunc>>> =
    RwLock::new(None);

type SetResFunc = unsafe extern "cdecl" fn();
static SET_RES_HOOK: RwLock<Option<GenericDetour<SetResFunc>>> = RwLock::new(None);

type BlitFunc = unsafe extern "stdcall" fn();
static BLIT_HOOK: RwLock<Option<GenericDetour<BlitFunc>>> = RwLock::new(None);

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

type HandleMessagesFunc = unsafe extern "stdcall" fn();
static HANDLE_MESSAGES_HOOK: RwLock<Option<GenericDetour<HandleMessagesFunc>>> = RwLock::new(None);

type RandomIntBelowFunc = unsafe extern "cdecl" fn(i32) -> i32;
static RANDOM_INT_BELOW_HOOK: RwLock<Option<GenericDetour<RandomIntBelowFunc>>> = RwLock::new(None);

type ToggleFullscreenFunc = unsafe extern "stdcall" fn();
static TOGGLE_FULLSCREEN_HOOK: RwLock<Option<GenericDetour<ToggleFullscreenFunc>>> =
    RwLock::new(None);

type NextClockFunc = unsafe extern "stdcall" fn();
static NEXT_CLOCK_HOOK: RwLock<Option<GenericDetour<NextClockFunc>>> = RwLock::new(None);

// Global variables
static mut G_TICKS_CHECK: *mut u32 = std::ptr::null_mut();
static mut G_TICKS_1: *mut u32 = std::ptr::null_mut();
static mut G_TICKS_2: *mut u32 = std::ptr::null_mut();
static mut G_GAME_WINDOW_WIDTH: *mut u32 = std::ptr::null_mut();
static mut G_GAME_WINDOW_HEIGHT: *mut u32 = std::ptr::null_mut();
static mut G_GAME_WINDOW_GEOMETRY: *mut *mut GameWindowGeometry = std::ptr::null_mut();
static mut G_SCREEN_W_MINUS_1: *mut i32 = std::ptr::null_mut();
static mut G_SCREEN_H_MINUS_1: *mut i32 = std::ptr::null_mut();
const RENDER_TARGET_COUNT: usize = 11;
static mut G_RENDER_TARGET_TABLE: *mut [RenderTarget; RENDER_TARGET_COUNT] = std::ptr::null_mut();

/// a * b in 16.16 fixed-point
fn fmul16(a: i32, b: i32) -> i32 {
    ((a as i64 * b as i64 + 0x8000) >> 16) as i32
}

/// Offset to center the HUD box inside the framebuffer
fn hud_origin() -> (i32, i32) {
    unsafe {
        let x0 = (*G_GAME_WINDOW_WIDTH as i32 - (**G_GAME_WINDOW_GEOMETRY).width) / 2;
        let y0 = (*G_GAME_WINDOW_HEIGHT as i32 - (**G_GAME_WINDOW_GEOMETRY).height) / 2;
        (x0, y0)
    }
}

/// Slot `slot` of the game's render target table, if that slot exists
fn render_target(slot: i32) -> Option<&'static mut RenderTarget> {
    let slot = usize::try_from(slot).ok()?;
    unsafe { G_RENDER_TARGET_TABLE.as_mut()?.get_mut(slot) }
}

/// The cockpit's 3D scene slot
static mut SCENE_SLOT: i32 = -1;

static mut G_BLIT_GLOBAL_1: *mut BOOL = std::ptr::null_mut();
pub static mut G_WINDOW_ACTIVE: *mut BOOL = std::ptr::null_mut();
static mut G_CURRENT_DRAW_MODE: *mut *mut DrawMode = std::ptr::null_mut();
static mut G_STRETCH_BLIT_SOURCE_RECT: *mut RenderTarget = std::ptr::null_mut();
static mut G_STRETCH_BLIT_OTHER_SOURCE_RECT: *mut RenderTarget = std::ptr::null_mut();
static mut G_BLIT_GLOBAL_2: *mut u32 = std::ptr::null_mut();
static mut G_BLIT_GLOBAL_3: *mut u32 = std::ptr::null_mut();
static mut G_WIDTH_SCALE: *mut i32 = std::ptr::null_mut();
static mut G_HORIZON_HAZE_THICKNESS: *mut i32 = std::ptr::null_mut();
static mut G_EYEPOINT: *mut *mut Eyepoint = std::ptr::null_mut();
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
static mut G_DELTA_TIME: *mut i32 = std::ptr::null_mut();

/// Cache the CD audio device to reuse between sim launches.
/// Windows 11 crashes if we try to close the CD audio device.
static mut CD_AUDIO_DEVICE: u32 = u32::MAX;

/// Our own CD audio player that actually plays tracks from files.
static CD_AUDIO_PLAYER: Mutex<Option<CdAudioPlayer>> = Mutex::new(None);

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

        let flee_option = (base_address + 0x000a1ad0) as *mut [u8; 7];
        unsafe {
            flee_option.write_volatile(*(b"Desktop"));
        }

        let flee_title = (base_address + 0x000a1b08) as *mut [u8; 7];
        unsafe {
            flee_title.write_volatile(*(b"DESKTOP"));
        }

        unsafe {
            G_TICKS_CHECK = (base_address + 0x000ad008) as *mut u32;
            G_TICKS_1 = (base_address + 0x000ad20c) as *mut u32;
            G_TICKS_2 = (base_address + 0x000ad210) as *mut u32;
            G_GAME_WINDOW_WIDTH = (base_address + 0x000acb6c) as *mut u32;
            G_GAME_WINDOW_HEIGHT = (base_address + 0x000acb70) as *mut u32;
            G_GAME_WINDOW_GEOMETRY = (base_address + 0x00176eb4) as *mut *mut GameWindowGeometry;
            G_SCREEN_W_MINUS_1 = (base_address + 0x00176ee4) as *mut i32;
            G_SCREEN_H_MINUS_1 = (base_address + 0x00176ec0) as *mut i32;
            G_RENDER_TARGET_TABLE =
                (base_address + 0x00181a60) as *mut [RenderTarget; RENDER_TARGET_COUNT];
            G_BLIT_GLOBAL_1 = (base_address + 0x00176ebc) as *mut BOOL;
            G_WINDOW_ACTIVE = (base_address + 0x000acb74) as *mut BOOL;
            G_CURRENT_DRAW_MODE = (base_address + 0x000b1774) as *mut *mut DrawMode;
            G_STRETCH_BLIT_SOURCE_RECT = (base_address + 0x00176ed0) as *mut RenderTarget;
            G_STRETCH_BLIT_OTHER_SOURCE_RECT = (base_address + 0x000bdff8) as *mut RenderTarget;
            G_BLIT_GLOBAL_2 = (base_address + 0x000a5f18) as *mut u32;
            G_BLIT_GLOBAL_3 = (base_address + 0x000a5a24) as *mut u32;
            G_WIDTH_SCALE = (base_address + 0x000e9610) as *mut i32;
            G_HORIZON_HAZE_THICKNESS = (base_address + 0x000a6d30) as *mut i32;
            G_EYEPOINT = (base_address + 0x000a6cc0) as *mut *mut Eyepoint;
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
            G_DELTA_TIME = (base_address + 0x000ba550) as *mut i32;

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

            *INIT_GAME_WINDOW_GEOMETRY_HOOK.write().unwrap() = {
                let target: InitGameWindowGeometryFunc =
                    std::mem::transmute(base_address + 0x00012720);
                Some(hook_function(target, Self::init_game_window_geometry)?)
            };

            *SCALE_RECT_TO_SCREEN_HOOK.write().unwrap() = {
                let target: ScaleRectToScreenFunc = std::mem::transmute(base_address + 0x00056920);
                Some(hook_function(target, Self::scale_rect_to_screen)?)
            };

            *SCALE_POINT_TO_SCREEN_HOOK.write().unwrap() = {
                let target: ScalePointToScreenFunc = std::mem::transmute(base_address + 0x00056ae4);
                Some(hook_function(target, Self::scale_point_to_screen)?)
            };

            *CENTER_RECT_ON_SCREEN_HOOK.write().unwrap() = {
                let target: CenterRectOnScreenFunc = std::mem::transmute(base_address + 0x00056e22);
                Some(hook_function(target, Self::center_rect_on_screen)?)
            };

            *APPLY_EYEPOINT_FOV_HOOK.write().unwrap() = {
                let target: ApplyEyepointFovFunc = std::mem::transmute(base_address + 0x00011455);
                Some(hook_function(target, Self::apply_eyepoint_fov)?)
            };

            *LOAD_COCKPIT_LAYOUT_HOOK.write().unwrap() = {
                let target: LoadCockpitLayoutFunc = std::mem::transmute(base_address + 0x0003dab0);
                Some(hook_function(target, Self::load_cockpit_layout)?)
            };

            *SELECT_RENDER_TARGET_HOOK.write().unwrap() = {
                let target: SelectRenderTargetFunc = std::mem::transmute(base_address + 0x0000242f);
                Some(hook_function(target, Self::select_render_target)?)
            };

            *SETUP_EYEPOINT_PROJECTION_HOOK.write().unwrap() = {
                let target: SetupEyepointProjectionFunc =
                    std::mem::transmute(base_address + 0x0004bc2e);
                Some(hook_function(target, Self::setup_eyepoint_projection)?)
            };

            *SET_RES_HOOK.write().unwrap() = {
                let target: SetResFunc = std::mem::transmute(base_address + 0x0005d4d3);
                Some(hook_function(target, Self::set_res)?)
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

            *HANDLE_MESSAGES_HOOK.write().unwrap() = {
                let target: HandleMessagesFunc = std::mem::transmute(base_address + 0x00067bbc);
                Some(hook_function(target, Self::handle_messages)?)
            };

            *RANDOM_INT_BELOW_HOOK.write().unwrap() = {
                let target: RandomIntBelowFunc = std::mem::transmute(base_address + 0x000736b3);
                Some(hook_function(target, Self::random_int_below)?)
            };

            *TOGGLE_FULLSCREEN_HOOK.write().unwrap() = {
                let target: ToggleFullscreenFunc = std::mem::transmute(base_address + 0x00077392);
                Some(hook_function(target, Self::toggle_fullscreen)?)
            };

            *NEXT_CLOCK_HOOK.write().unwrap() = {
                let target: NextClockFunc = std::mem::transmute(base_address + 0x0007ce2c);
                Some(hook_function(target, Self::next_clock)?)
            };

            drawmode::hook_functions(base_address)?;

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
    /// It would sometimes overflow and sometimes divide by zero, especially when the FPS is too high.
    unsafe extern "cdecl" fn integer_overflow_happens_here(a: i32, b: i32, c: i32) -> i32 {
        if c == 0 {
            tracing::error!("integer_overflow_happens_here: division by zero (a={a}, b={b})");
            std::process::abort();
        }
        (a as i64 * b as i64 / c as i64) as i32
    }

    /// The game decides which resolution to use based on the DLL name passed to this function.
    /// This is presumably a leftover from the DOS version of the game, possibly to preserve config file compatibility.
    unsafe extern "cdecl" fn set_game_resolution(resolution: *mut c_char) {
        let widescreen = SETTINGS.get_bool("video", "widescreen", false);
        unsafe {
            // "MCGA.DLL"
            if widescreen {
                *G_GAME_WINDOW_WIDTH = 427;
                *G_GAME_WINDOW_HEIGHT = 240;
            } else {
                *G_GAME_WINDOW_WIDTH = 320;
                *G_GAME_WINDOW_HEIGHT = 240;
            }

            let resolution = std::ffi::CStr::from_ptr(resolution)
                .to_string_lossy()
                .to_uppercase();
            if resolution == "VESA480.DLL" {
                *G_GAME_WINDOW_WIDTH = if widescreen { 854 } else { 640 };
                *G_GAME_WINDOW_HEIGHT = 480;
            } else if resolution == "VESA768.DLL" {
                *G_GAME_WINDOW_WIDTH = if widescreen { 1366 } else { 1024 };
                *G_GAME_WINDOW_HEIGHT = 768;
            }
        }
    }

    /// Allocates GameWindowGeometry and caches the W-1/H-1 scale globals used by every HUD-scaling function.
    /// Depending on the configured resolution, we force the window size to match the HUD box.
    unsafe extern "cdecl" fn init_game_window_geometry() -> i32 {
        unsafe {
            let (width, height) = match *G_GAME_WINDOW_HEIGHT {
                480 => (640, 480),
                768 => (1024, 768),
                _ => (320, 240),
            };

            let ok = INIT_GAME_WINDOW_GEOMETRY_HOOK
                .read()
                .unwrap()
                .as_ref()
                .unwrap()
                .call();
            if ok != 0 && !G_GAME_WINDOW_GEOMETRY.is_null() && !(*G_GAME_WINDOW_GEOMETRY).is_null()
            {
                (**G_GAME_WINDOW_GEOMETRY).width = width;
                (**G_GAME_WINDOW_GEOMETRY).height = height;
                *G_SCREEN_W_MINUS_1 = width - 1;
                *G_SCREEN_H_MINUS_1 = height - 1;
            }
            ok
        }
    }

    /// Maps normalised 16.16 HUD coordinates onto [0, W-1].
    /// Hooked to add the HUD origin to keep it centered in its own box.
    /// `src` and `dst` are usually the same pointer, so read the whole rect before writing any of it back.
    unsafe extern "cdecl" fn scale_rect_to_screen(
        _pixel_buffer: *mut PixelBuffer,
        src: *mut RenderTarget,
        dst: *mut RenderTarget,
    ) -> *mut RenderTarget {
        if src.is_null() || dst.is_null() {
            return dst;
        }
        let (x0, y0) = hud_origin();
        let rect = unsafe { (*src).clone() };
        unsafe {
            (*dst).left = x0 + fmul16(*G_SCREEN_W_MINUS_1, rect.left);
            (*dst).top = y0 + fmul16(*G_SCREEN_H_MINUS_1, rect.top);
            (*dst).right = x0 + fmul16(*G_SCREEN_W_MINUS_1, rect.right);
            (*dst).bottom = y0 + fmul16(*G_SCREEN_H_MINUS_1, rect.bottom);
        }
        dst
    }

    /// Same as `scale_rect_to_screen` but for a single point.
    unsafe extern "cdecl" fn scale_point_to_screen(
        _pixel_buffer: *mut PixelBuffer,
        src: *mut Point,
        dst: *mut Point,
    ) -> *mut Point {
        if src.is_null() || dst.is_null() {
            return dst;
        }
        let (x0, y0) = hud_origin();
        let point = unsafe { *src };
        unsafe {
            (*dst).x = x0 + fmul16(*G_SCREEN_W_MINUS_1, point.x);
            (*dst).y = y0 + fmul16(*G_SCREEN_H_MINUS_1, point.y);
        }
        dst
    }

    /// Centers a fixed-size rect on screen
    unsafe extern "cdecl" fn center_rect_on_screen(
        _pixel_buffer: *mut PixelBuffer,
        src: *mut RenderTarget,
        dst: *mut RenderTarget,
    ) -> *mut RenderTarget {
        if src.is_null() || dst.is_null() {
            return dst;
        }
        let (x0, y0) = hud_origin();
        let rect = unsafe { (*src).clone() };
        let width = rect.right - rect.left + 1;
        let height = rect.bottom - rect.top + 1;
        unsafe {
            let left = x0 + (*G_SCREEN_W_MINUS_1 - width - 1) / 2;
            let top = y0 + (*G_SCREEN_H_MINUS_1 - height - 1) / 2;
            (*dst).left = left;
            (*dst).top = top;
            (*dst).right = left + width - 1;
            (*dst).bottom = top + height - 1;
        }
        dst
    }

    /// Rewrites eyepoint->fovX every frame from the game's FOV globals.
    /// We keep the game's raw value in a shadow so its internal logic always sees the original raw value,
    /// then we always apply the Hor+ correction.
    unsafe extern "cdecl" fn apply_eyepoint_fov(reset: i32) {
        static mut LAST_RAW_FOV: i32 = 0x10000;
        unsafe {
            let cam = *G_EYEPOINT;
            if cam.is_null() {
                APPLY_EYEPOINT_FOV_HOOK
                    .read()
                    .unwrap()
                    .as_ref()
                    .unwrap()
                    .call(reset);
                return;
            }
            let correction = {
                let width = *G_GAME_WINDOW_WIDTH as i64;
                let height = *G_GAME_WINDOW_HEIGHT as i64;
                if width <= 0 || height <= 0 {
                    0x10000
                } else {
                    let aspect = (width << 16) / height;
                    (((0x15555i64) << 16) / aspect) as i32
                }
            };
            (*cam).fov_x = LAST_RAW_FOV;
            APPLY_EYEPOINT_FOV_HOOK
                .read()
                .unwrap()
                .as_ref()
                .unwrap()
                .call(reset);
            LAST_RAW_FOV = (*cam).fov_x;
            (*cam).fov_x = ((LAST_RAW_FOV as i64 * correction as i64) >> 16) as i32;
        }
    }

    /// Copies the cockpit's 3D viewport rect into render-target slot `layout.render_target_slot`.
    /// We widen that slot to the full framebuffer so the 3D view covers the whole width behind the centered 4:3 HUD.
    unsafe extern "cdecl" fn load_cockpit_layout(cockpit: i32, layout: *mut CockpitLayout) {
        unsafe {
            LOAD_COCKPIT_LAYOUT_HOOK
                .read()
                .unwrap()
                .as_ref()
                .unwrap()
                .call(cockpit, layout);
            if layout.is_null() {
                return;
            }
            let slot = (*layout).render_target_slot;
            if let Some(target) = render_target(slot) {
                SCENE_SLOT = slot;
                target.cover_framebuffer();
            }
        }
    }

    /// Copies slot `slot` of the render-target table into StretchBlitSourceRect and the eyepoint.
    /// We substitute the full-screen rect for scene slots only, just before they are consumed
    unsafe extern "cdecl" fn select_render_target(slot: i32) {
        unsafe {
            if (slot == 0 || slot == SCENE_SLOT)
                && let Some(target) = render_target(slot)
            {
                target.cover_framebuffer();
            }
            SELECT_RENDER_TARGET_HOOK
                .read()
                .unwrap()
                .as_ref()
                .unwrap()
                .call(slot);
        }
    }

    /// Recomputes the projection from the eyepoint struct.
    /// We force pixel_aspect_ratio to be square each time.
    unsafe extern "cdecl" fn setup_eyepoint_projection(cam: *mut Eyepoint) {
        unsafe {
            if !cam.is_null() {
                (*cam).pixel_aspect_ratio = 0x10000;
            }
            SETUP_EYEPOINT_PROJECTION_HOOK
                .read()
                .unwrap()
                .as_ref()
                .unwrap()
                .call(cam);
        }
    }

    /// Rescales every 320x200-authored table for the current resolution.
    /// We subtract the HUD origin offset from the horizon haze thickness to avoid stretching the sky vertically.
    /// This is kind of a hack. We should probably reimplement this function entirely.
    unsafe extern "cdecl" fn set_res() {
        unsafe {
            SET_RES_HOOK.read().unwrap().as_ref().unwrap().call();
            let (x0, _) = hud_origin();
            *G_HORIZON_HAZE_THICKNESS -= x0;
        }
    }

    /// This function is called every frame to draw the game.
    unsafe extern "stdcall" fn blit() {
        unsafe {
            if *G_BLIT_GLOBAL_1 == FALSE {
                if *G_WINDOW_ACTIVE == TRUE {
                    ((**G_CURRENT_DRAW_MODE).blit_flip_func)();
                }
            } else {
                ((**G_CURRENT_DRAW_MODE).stretch_blit_func)(
                    (*G_STRETCH_BLIT_SOURCE_RECT).left + 1,
                    (*G_STRETCH_BLIT_SOURCE_RECT).top + 1,
                    (*G_STRETCH_BLIT_SOURCE_RECT).right,
                    (*G_STRETCH_BLIT_SOURCE_RECT).bottom,
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
                    *G_CD_AUDIO_DEVICE = 1;
                    *G_CD_AUDIO_AUX_DEVICE = Self::get_cd_audio_aux_device();
                    *G_CD_AUDIO_INITIALIZED = 1;
                    return 0;
                }
                if source == CdSource::Files {
                    // files were explicitly requested but unavailable
                    return 1;
                }
            }

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
                *G_CD_AUDIO_DEVICE,
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
            if let Some(player) = CD_AUDIO_PLAYER.lock().unwrap().as_mut() {
                return player.state();
            }

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
                *G_CD_AUDIO_INITIALIZED = 0;
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
                    *position = *G_PAUSED_CD_AUDIO_POSITION;
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
                    let position = (*G_PAUSED_CD_AUDIO_POSITION).into();
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
        if max <= 0 {
            tracing::error!("random_int_below: max <= 0 (max={max})");
            std::process::abort();
        }
        rand::rng().random_range(0..max)
    }

    unsafe extern "stdcall" fn toggle_fullscreen() {
        // Do nothing because we handle this in the custom window proc
    }

    /// We hook this in order to limit the framerate to the configured value.
    unsafe extern "stdcall" fn next_clock() {
        let framerate_limit = SETTINGS.get_int("video", "framerate_limit", 45);

        if framerate_limit > 0 {
            static LAST_INSTANT: RwLock<Option<Instant>> = RwLock::new(None);
            let frame_time = 1.0 / framerate_limit as f64;
            let mut last_instant = LAST_INSTANT.write().unwrap();
            let last = *last_instant.get_or_insert_with(Instant::now);
            while last.elapsed().as_secs_f64() < frame_time {
                std::thread::yield_now();
            }
            *last_instant = Some(Instant::now());
        }

        unsafe {
            NEXT_CLOCK_HOOK.read().unwrap().as_ref().unwrap().call();
        }
    }
}

impl Drop for Sim {
    fn drop(&mut self) {
        unsafe {
            ailrs::shutdown();
            crate::SIM_WINDOW_PROC = None;
            GAME_TICK_TIMER_CALLBACK_HOOK.write().unwrap().take();
            SUP_ANIM_TIMER_CALLBACK_HOOK.write().unwrap().take();
            INTEGER_OVERFLOW_HAPPENS_HERE_HOOK.write().unwrap().take();
            SET_GAME_RESOLUTION_HOOK.write().unwrap().take();
            INIT_GAME_WINDOW_GEOMETRY_HOOK.write().unwrap().take();
            SCALE_RECT_TO_SCREEN_HOOK.write().unwrap().take();
            SCALE_POINT_TO_SCREEN_HOOK.write().unwrap().take();
            CENTER_RECT_ON_SCREEN_HOOK.write().unwrap().take();
            APPLY_EYEPOINT_FOV_HOOK.write().unwrap().take();
            LOAD_COCKPIT_LAYOUT_HOOK.write().unwrap().take();
            SELECT_RENDER_TARGET_HOOK.write().unwrap().take();
            SETUP_EYEPOINT_PROJECTION_HOOK.write().unwrap().take();
            SET_RES_HOOK.write().unwrap().take();
            BLIT_HOOK.write().unwrap().take();
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
            HANDLE_MESSAGES_HOOK.write().unwrap().take();
            RANDOM_INT_BELOW_HOOK.write().unwrap().take();
            TOGGLE_FULLSCREEN_HOOK.write().unwrap().take();
            NEXT_CLOCK_HOOK.write().unwrap().take();
            drawmode::unhook_functions();
            self.ail.unhook();
            CD_AUDIO_PLAYER.lock().unwrap().take();
            FreeLibrary(self.module).unwrap();
            LOADED = false;
        }
    }
}
