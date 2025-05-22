use std::{
    ffi::{c_char, c_void},
    sync::RwLock,
};

use anyhow::Result;
use retour::GenericDetour;
use windows::{
    Win32::{
        Media::{
            Audio::{HWAVEOUT, WAVEHDR},
            MM_WOM_DONE,
        },
        System::LibraryLoader::*,
    },
    core::s,
};

use crate::hooker::hook_function;

#[repr(C)]
struct WaveHdrUser {
    unknown1: [u8; 20],
    unknown2: u32,
    unknown3: *mut *mut WAVEHDR,
    unknown4: u32,
    unknown5: [u8; 40],
    unknown6: i32,
}

type AilCallback = unsafe extern "stdcall" fn(u32);

#[repr(C)]
struct SomeTimerStruct {
    state: u32,
    callback: AilCallback,
    user: u32,
    accumulated_time: i32,
    next_proc_time: i32,
}

type WaveOutProc = unsafe extern "stdcall" fn(HWAVEOUT, u32, usize, usize, usize);
static WAVE_OUT_HOOK: RwLock<Option<GenericDetour<WaveOutProc>>> = RwLock::new(None);

type FileReadFunc = unsafe extern "stdcall" fn(*const c_char, *mut c_void) -> *mut c_void;
static FILE_READ_HOOK: RwLock<Option<GenericDetour<FileReadFunc>>> = RwLock::new(None);

type MemFreeLockFunc = unsafe extern "stdcall" fn(*mut c_void);
static MEM_FREE_LOCK_HOOK: RwLock<Option<GenericDetour<MemFreeLockFunc>>> = RwLock::new(None);

type TimeProc = unsafe extern "stdcall" fn(u32, u32, *mut c_void, *mut c_void, *mut c_void);
static TIME_HOOK: RwLock<Option<GenericDetour<TimeProc>>> = RwLock::new(None);

static mut G_LAST_FINISHED_WAVE_HDR: *mut *mut WAVEHDR = std::ptr::null_mut();
static mut G_LAST_FINISHED_WAVE_HDR_USER: *mut *mut WaveHdrUser = std::ptr::null_mut();
static mut G_WAVE_OUT_PROC_GLOBAL_THING: *mut u32 = std::ptr::null_mut();

static mut G_PERIOD: *mut i32 = std::ptr::null_mut();
static mut G_COUNTER: *mut u32 = std::ptr::null_mut();
static mut G_TIMERS: *mut *mut SomeTimerStruct = std::ptr::null_mut();
static mut G_GLOBAL_3: *mut u32 = std::ptr::null_mut();
static mut G_TIME_PROC_LOCKED: *mut i32 = std::ptr::null_mut();
static mut G_GLOBAL_5: *mut u32 = std::ptr::null_mut();
static mut G_NUM_TIMERS: *mut u32 = std::ptr::null_mut();

static ALLOCATED_BLOCKS: RwLock<Vec<usize>> = RwLock::new(Vec::<usize>::new());

pub struct Ail {}

impl Ail {
    pub fn new() -> Result<Self> {
        let module = unsafe { GetModuleHandleA(s!("WAIL32.DLL"))? };
        let base_address = module.0 as usize;

        unsafe {
            G_LAST_FINISHED_WAVE_HDR = (base_address + 0x0001ba10) as *mut *mut WAVEHDR;
            G_LAST_FINISHED_WAVE_HDR_USER = (base_address + 0x0001ba0c) as *mut *mut WaveHdrUser;
            G_WAVE_OUT_PROC_GLOBAL_THING = (base_address + 0x0001ba04) as *mut u32;

            G_PERIOD = (base_address + 0x0001b7fc) as *mut i32;
            G_COUNTER = (base_address + 0x0001b810) as *mut u32;
            G_TIMERS = (base_address + 0x0001b7f8) as *mut *mut SomeTimerStruct;
            G_GLOBAL_3 = (base_address + 0x0001b804) as *mut u32;
            G_TIME_PROC_LOCKED = (base_address + 0x00019030) as *mut i32;
            G_GLOBAL_5 = (base_address + 0x0001c59c) as *mut u32;
            G_NUM_TIMERS = (base_address + 0x0001b800) as *mut u32;

            *WAVE_OUT_HOOK.write().unwrap() = {
                let wave_out: WaveOutProc = std::mem::transmute(base_address + 0x00008e6d);
                Some(hook_function(wave_out, Self::wave_out_proc)?)
            };

            *FILE_READ_HOOK.write().unwrap() = {
                let file_read: FileReadFunc = std::mem::transmute(base_address + 0x0000845f);
                Some(hook_function(file_read, Self::file_read)?)
            };

            *MEM_FREE_LOCK_HOOK.write().unwrap() = {
                let mem_free_lock: MemFreeLockFunc = std::mem::transmute(base_address + 0x00001f14);
                Some(hook_function(mem_free_lock, Self::mem_free_lock)?)
            };

            *TIME_HOOK.write().unwrap() = {
                let time_proc: TimeProc = std::mem::transmute(base_address + 0x000011c6);
                Some(hook_function(time_proc, Self::time_proc)?)
            };
        }

        Ok(Self {})
    }

    /// Replacement for AIL's waveOutOpen callback that doesn't try to suspend the main thread
    unsafe extern "stdcall" fn wave_out_proc(
        _h_wave_out: HWAVEOUT,
        u_msg: u32,
        _dw_instance: usize,
        dw_param1: usize,
        _dw_param2: usize,
    ) {
        unsafe {
            if u_msg == MM_WOM_DONE {
                let wave_hdr = dw_param1 as *mut WAVEHDR;
                *G_LAST_FINISHED_WAVE_HDR = wave_hdr;
                if (*wave_hdr).dwUser != 0 {
                    *G_LAST_FINISHED_WAVE_HDR_USER = *((*wave_hdr).dwUser as *mut *mut WaveHdrUser);
                    if (**G_LAST_FINISHED_WAVE_HDR_USER).unknown6 != 0 {
                        *G_WAVE_OUT_PROC_GLOBAL_THING = (**G_LAST_FINISHED_WAVE_HDR_USER).unknown4;
                        let wave_hdrs = (**G_LAST_FINISHED_WAVE_HDR_USER).unknown3;
                        *wave_hdrs.offset((*G_WAVE_OUT_PROC_GLOBAL_THING) as isize) = wave_hdr;
                        *G_WAVE_OUT_PROC_GLOBAL_THING = (*G_WAVE_OUT_PROC_GLOBAL_THING + 1)
                            % (**G_LAST_FINISHED_WAVE_HDR_USER).unknown2;
                        (**G_LAST_FINISHED_WAVE_HDR_USER).unknown4 = *G_WAVE_OUT_PROC_GLOBAL_THING;
                    }
                }
            }
        }
    }

    /// Hooked to keep track of the allocated blocks
    unsafe extern "stdcall" fn file_read(
        file_name: *const c_char,
        buffer: *mut c_void,
    ) -> *mut c_void {
        unsafe {
            let mut allocated_blocks = ALLOCATED_BLOCKS.write().unwrap();
            let result = FILE_READ_HOOK
                .read()
                .unwrap()
                .as_ref()
                .unwrap()
                .call(file_name, buffer);
            if !allocated_blocks.contains(&(result as usize)) {
                allocated_blocks.push(result as usize);
            }
            result
        }
    }

    /// Only try to free blocks that we know haven't been freed yet
    unsafe extern "stdcall" fn mem_free_lock(lp_mem: *mut c_void) {
        unsafe {
            let mut allocated_blocks = ALLOCATED_BLOCKS.write().unwrap();
            if allocated_blocks.contains(&(lp_mem as usize)) {
                MEM_FREE_LOCK_HOOK
                    .read()
                    .unwrap()
                    .as_ref()
                    .unwrap()
                    .call(lp_mem);
                allocated_blocks.retain(|&x| x != lp_mem as usize);
            }
        }
    }

    /// Passed to timeSetEvent in AIL to handle timers.
    /// This also had calls to SuspendThread that needed to be removed.
    unsafe extern "stdcall" fn time_proc(
        _u_timer_id: u32,
        _u_msg: u32,
        _dw_user: *mut c_void,
        _dw1: *mut c_void,
        _dw2: *mut c_void,
    ) {
        unsafe {
            if (*G_TIMERS).is_null() {
                return;
            }

            let timers = std::slice::from_raw_parts_mut(*G_TIMERS, (*G_NUM_TIMERS) as usize);

            *G_COUNTER += 1;

            if *G_GLOBAL_3 > 0 || *G_TIME_PROC_LOCKED == 1 {
                return;
            }

            *G_TIME_PROC_LOCKED = 1;
            *G_GLOBAL_5 += 1;
            for timer in timers {
                if timer.state == 2 {
                    timer.accumulated_time += *G_PERIOD;
                    if timer.accumulated_time >= timer.next_proc_time {
                        timer.accumulated_time -= timer.next_proc_time;
                        (timer.callback)(timer.user);
                    }
                }
            }
            *G_GLOBAL_5 -= 1;
            *G_TIME_PROC_LOCKED = 0;
        }
    }

    pub fn unhook(&mut self) {
        WAVE_OUT_HOOK.write().unwrap().take();
        FILE_READ_HOOK.write().unwrap().take();
        MEM_FREE_LOCK_HOOK.write().unwrap().take();
        TIME_HOOK.write().unwrap().take();
    }
}
