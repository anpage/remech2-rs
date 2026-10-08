use std::{
    collections::BTreeMap,
    ffi::{CStr, CString, c_char, c_int, c_void},
    ptr, slice,
    sync::Mutex,
};

use anyhow::{Result, bail};
use remech2_sys::{
    shared::{BwdBuffer, MissionLaunch},
    shell::SimHandoffState,
};

/// The shell's state across a mission
static SIM_HANDOFF: Mutex<Option<SimHandoffState>> = Mutex::new(None);

/// The .bwd files the shell built for the sim
static BWD_FILES: Mutex<BTreeMap<CString, Vec<u8>>> = Mutex::new(BTreeMap::new());

#[unsafe(export_name = "MechSaveSimHandoff")]
pub unsafe extern "C" fn save_sim_handoff(state: *const SimHandoffState) {
    if !state.is_null() {
        *SIM_HANDOFF.lock().unwrap() = Some(unsafe { ptr::read(state) });
    }
}

#[unsafe(export_name = "MechLoadSimHandoff")]
pub unsafe extern "C" fn load_sim_handoff(state: *mut SimHandoffState) -> c_int {
    let saved = SIM_HANDOFF.lock().unwrap();
    match saved.as_ref() {
        Some(saved) if !state.is_null() => {
            unsafe { ptr::copy_nonoverlapping(saved, state, 1) };
            1
        }
        _ => 0,
    }
}

#[unsafe(export_name = "MechKeepBwd")]
pub unsafe extern "C" fn keep_bwd(name: *const c_char, data: *const c_void, size: u32) {
    if name.is_null() || data.is_null() {
        return;
    }
    let name = unsafe { CStr::from_ptr(name) }
        .to_string_lossy()
        .to_lowercase();
    let Ok(name) = CString::new(name) else {
        return;
    };
    let data = unsafe { slice::from_raw_parts(data.cast::<u8>(), size as usize) }.to_vec();
    BWD_FILES.lock().unwrap().insert(name, data);
}

/// Runs `run` with the sim's command line and the .bwd files
pub fn with_mission_launch<T>(run: impl FnOnce(&str, &MissionLaunch) -> T) -> Result<T> {
    let handoff = SIM_HANDOFF.lock().unwrap();
    let Some(state) = handoff.as_ref() else {
        bail!("the shell handed off no mission");
    };
    let cmd_line = unsafe { CStr::from_ptr(state.m_cmdLine.as_ptr()) }
        .to_string_lossy()
        .into_owned();

    drop(handoff);

    let files = BWD_FILES.lock().unwrap();
    let bwds: Vec<BwdBuffer> = files
        .iter()
        .map(|(name, data)| BwdBuffer {
            m_name: name.as_ptr(),
            m_data: data.as_ptr().cast(),
            m_size: data.len() as u32,
        })
        .collect();
    let launch = MissionLaunch {
        m_bwds: bwds.as_ptr(),
        m_bwdCount: bwds.len() as u32,
    };
    Ok(run(&cmd_line, &launch))
}
