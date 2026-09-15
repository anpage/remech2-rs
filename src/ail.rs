use std::{
    ffi::{c_char, c_void},
    sync::RwLock,
};

use anyhow::Result;
use binding::{
    globals,
    macros::{hook, patch_groups, patches},
    module::ModuleBase,
    patch::{apply_groups, revert_groups},
};
use windows::{
    Win32::{
        Media::{
            Audio::{HWAVEOUT, WAVEHDR},
            MM_WOM_DONE,
        },
        System::LibraryLoader::GetModuleHandleA,
    },
    core::s,
};

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

/// WAIL32.DLL gets loaded as a dependnecy of the sim and shell.
/// We only ever attach to the copy they loaded.
pub static MODULE: ModuleBase = ModuleBase::new("WAIL32.DLL");

globals!(
    static G_LAST_FINISHED_WAVE_HDR: *mut WAVEHDR = 0x0001ba10;
    static G_LAST_FINISHED_WAVE_HDR_USER: *mut WaveHdrUser = 0x0001ba0c;
    static G_WAVE_OUT_PROC_GLOBAL_THING: u32 = 0x0001ba04;
    static G_PERIOD: i32 = 0x0001b7fc;
    static G_COUNTER: u32 = 0x0001b810;
    static G_TIMERS: *mut SomeTimerStruct = 0x0001b7f8;
    static G_GLOBAL_3: u32 = 0x0001b804;
    static G_TIME_PROC_LOCKED: i32 = 0x00019030;
    static G_GLOBAL_5: u32 = 0x0001c59c;
    static G_NUM_TIMERS: u32 = 0x0001b800;
);

static ALLOCATED_BLOCKS: RwLock<Vec<usize>> = RwLock::new(Vec::<usize>::new());

/// Replacement for AIL's waveOutOpen callback that doesn't try to suspend the main thread
#[hook(rva = 0x00008e6d)]
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
            G_LAST_FINISHED_WAVE_HDR.set(wave_hdr);
            if (*wave_hdr).dwUser != 0 {
                let user = *((*wave_hdr).dwUser as *mut *mut WaveHdrUser);
                G_LAST_FINISHED_WAVE_HDR_USER.set(user);
                if (*user).unknown6 != 0 {
                    G_WAVE_OUT_PROC_GLOBAL_THING.set((*user).unknown4);
                    let wave_hdrs = (*user).unknown3;
                    *wave_hdrs.offset(G_WAVE_OUT_PROC_GLOBAL_THING.get() as isize) = wave_hdr;
                    G_WAVE_OUT_PROC_GLOBAL_THING
                        .set((G_WAVE_OUT_PROC_GLOBAL_THING.get() + 1) % (*user).unknown2);
                    (*user).unknown4 = G_WAVE_OUT_PROC_GLOBAL_THING.get();
                }
            }
        }
    }
}

/// Hooked to keep track of the allocated blocks
#[hook(rva = 0x0000845f)]
unsafe extern "stdcall" fn file_read(file_name: *const c_char, buffer: *mut c_void) -> *mut c_void {
    unsafe {
        let result = original(file_name, buffer);
        if result.is_null() {
            return result;
        }
        let mut allocated_blocks = ALLOCATED_BLOCKS.write().unwrap();
        if !allocated_blocks.contains(&(result as usize)) {
            allocated_blocks.push(result as usize);
        }
        result
    }
}

/// Only try to free blocks that we know haven't been freed yet
#[hook(rva = 0x00001f14)]
unsafe extern "stdcall" fn mem_free_lock(lp_mem: *mut c_void) {
    unsafe {
        if lp_mem.is_null() {
            return;
        }
        let mut allocated_blocks = ALLOCATED_BLOCKS.write().unwrap();
        if allocated_blocks.contains(&(lp_mem as usize)) {
            original(lp_mem);
            allocated_blocks.retain(|&x| x != lp_mem as usize);
        }
    }
}

/// Passed to timeSetEvent in AIL to handle timers.
/// This also had calls to SuspendThread that needed to be removed.
#[hook(rva = 0x000011c6)]
unsafe extern "stdcall" fn time_proc(
    _u_timer_id: u32,
    _u_msg: u32,
    _dw_user: *mut c_void,
    _dw1: *mut c_void,
    _dw2: *mut c_void,
) {
    unsafe {
        if G_TIMERS.get().is_null() {
            return;
        }

        let timers = std::slice::from_raw_parts_mut(G_TIMERS.get(), G_NUM_TIMERS.get() as usize);

        G_COUNTER.set(G_COUNTER.get() + 1);

        if G_GLOBAL_3.get() > 0 || G_TIME_PROC_LOCKED.get() == 1 {
            return;
        }

        G_TIME_PROC_LOCKED.set(1);
        G_GLOBAL_5.set(G_GLOBAL_5.get() + 1);
        for timer in timers {
            if timer.state == 2 {
                timer.accumulated_time += G_PERIOD.get();
                if timer.accumulated_time >= timer.next_proc_time {
                    timer.accumulated_time -= timer.next_proc_time;
                    (timer.callback)(timer.user);
                }
            }
        }
        G_GLOBAL_5.set(G_GLOBAL_5.get() - 1);
        G_TIME_PROC_LOCKED.set(0);
    }
}

patches!(
    static PATCHES = [
        hook wave_out_proc,
        hook file_read,
        hook mem_free_lock,
        hook time_proc,
    ];
);

patch_groups! {
    static PATCH_GROUPS = [self];
}

pub struct Ail {}

impl Ail {
    pub fn new() -> Result<Self> {
        let module = unsafe { GetModuleHandleA(s!("WAIL32.DLL"))? };
        MODULE.set(module.0 as usize);

        if let Err(e) = unsafe { apply_groups(PATCH_GROUPS) } {
            MODULE.clear();
            return Err(e);
        }

        Ok(Self {})
    }

    pub fn unhook(&mut self) {
        revert_groups(PATCH_GROUPS);
        MODULE.clear();
    }
}
