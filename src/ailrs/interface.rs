use std::{ffi::c_void, num::NonZero};

use tracing::{Level, instrument};
use windows::Win32::Media::Audio::WAVEFORMATEX;

#[repr(C)]
#[derive(Clone, Copy, Debug)]
pub struct DriverHandle(Option<NonZero<u32>>);

#[repr(C)]
#[derive(Clone, Copy, Debug)]
pub struct SampleHandle(Option<NonZero<u32>>);

#[instrument(level = Level::DEBUG)]
pub unsafe extern "stdcall" fn allocate_file_sample(
    driver: DriverHandle,
    data: *const u8,
    param_3: i32,
) -> SampleHandle {
    tracing::debug!("Called");
    SampleHandle(NonZero::new(1))
}

#[instrument(level = Level::DEBUG)]
pub unsafe extern "stdcall" fn allocate_sample_handle(driver: DriverHandle) -> SampleHandle {
    tracing::debug!("Called");
    SampleHandle(NonZero::new(2))
}

#[instrument(level = Level::DEBUG)]
pub unsafe extern "stdcall" fn end_sample(sample: SampleHandle) {
    tracing::debug!("Called");
}

#[instrument(level = Level::DEBUG)]
pub unsafe extern "stdcall" fn init_sample(sample: SampleHandle) {
    tracing::debug!("Called");
}

#[instrument(level = Level::DEBUG)]
pub unsafe extern "stdcall" fn load_sample_buffer(
    sample: SampleHandle,
    param_2: u32,
    data: *const u8,
    param_4: u32,
) {
    tracing::debug!("Called");
}

#[instrument(level = Level::DEBUG)]
pub unsafe extern "stdcall" fn register_eos_callback(
    sample: SampleHandle,
    callback: unsafe extern "stdcall" fn(SampleHandle),
) {
    tracing::debug!("Called");
}

#[instrument(level = Level::DEBUG)]
pub unsafe extern "stdcall" fn release_sample_handle(sample: SampleHandle) {
    tracing::debug!("Called");
}

#[instrument(level = Level::DEBUG)]
pub unsafe extern "stdcall" fn resume_sample(sample: SampleHandle) {
    tracing::debug!("Called");
}

#[instrument(level = Level::DEBUG)]
pub unsafe extern "stdcall" fn sample_buffer_ready(sample: SampleHandle) -> i32 {
    tracing::debug!("Called");
    0
}

#[instrument(level = Level::DEBUG)]
pub unsafe extern "stdcall" fn sample_user_data(sample: SampleHandle, index: u32) -> u32 {
    tracing::debug!("Called");
    0
}

#[instrument(level = Level::DEBUG)]
pub unsafe extern "stdcall" fn set_preference(key: u32, value: u32) {
    tracing::debug!("Called");
}

#[instrument(level = Level::DEBUG)]
pub unsafe extern "stdcall" fn set_sample_loop_count(sample: SampleHandle, loop_count: u32) {
    tracing::debug!("Called");
}

#[instrument(level = Level::DEBUG)]
pub unsafe extern "stdcall" fn set_sample_pan(sample: SampleHandle, pan: i32) {
    tracing::debug!("Called");
}

#[instrument(level = Level::DEBUG)]
pub unsafe extern "stdcall" fn set_sample_playback_rate(sample: SampleHandle, playback_rate: i32) {
    tracing::debug!("Called");
}

#[instrument(level = Level::DEBUG)]
pub unsafe extern "stdcall" fn set_sample_type(sample: SampleHandle, format: i32, flags: u32) {
    tracing::debug!("Called");
}

#[instrument(level = Level::DEBUG)]
pub unsafe extern "stdcall" fn set_sample_user_data(sample: SampleHandle, user_data: u32) {
    tracing::debug!("Called");
}

#[instrument(level = Level::DEBUG)]
pub unsafe extern "stdcall" fn set_sample_volume(sample: SampleHandle, volume: i32) {
    tracing::debug!("Called");
}

#[instrument(level = Level::DEBUG)]
pub unsafe extern "stdcall" fn start_sample(sample: SampleHandle) {
    tracing::debug!("Called");
}

#[instrument(level = Level::DEBUG)]
pub unsafe extern "stdcall" fn stop_sample(sample: SampleHandle) {
    tracing::debug!("Called");
}

#[instrument(level = Level::DEBUG)]
pub unsafe extern "stdcall" fn wave_out_open(
    dig_driver_out: *mut SampleHandle,
    _: *mut c_void,
    device_id: u32,
    wave_format: *const WAVEFORMATEX,
) -> u32 {
    tracing::debug!("Called");
    unsafe {
        *dig_driver_out = SampleHandle(NonZero::new(1));
    }
    return 0;
}
