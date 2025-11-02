use std::{ffi::c_void, num::NonZero};

use tracing::{Level, instrument};
use windows::Win32::Media::Audio::WAVEFORMATEX;

use crate::ailrs::{
    sample::Buffer,
    storage::{create_driver, create_sample, get_sample},
};

#[repr(C)]
#[derive(Clone, Copy, Debug)]
pub struct DriverHandle(Option<NonZero<u32>>);

impl DriverHandle {
    pub fn new(handle: u32) -> Self {
        Self(NonZero::new(handle))
    }

    pub fn raw(&self) -> u32 {
        if let Some(raw) = self.0 {
            return raw.get();
        } else {
            0
        }
    }
}

#[repr(C)]
#[derive(Clone, Copy, Debug)]
pub struct SampleHandle(Option<NonZero<u32>>);

impl SampleHandle {
    pub fn new(handle: u32) -> Self {
        Self(NonZero::new(handle))
    }

    pub fn raw(&self) -> u32 {
        if let Some(raw) = self.0 {
            return raw.get();
        } else {
            0
        }
    }
}

#[instrument(level = Level::DEBUG)]
pub unsafe extern "stdcall" fn allocate_file_sample(
    driver: DriverHandle,
    data: *const u8,
    _: i32,
) -> SampleHandle {
    tracing::debug!("Called");
    create_sample(driver)
}

#[instrument(level = Level::DEBUG)]
pub unsafe extern "stdcall" fn allocate_sample_handle(driver: DriverHandle) -> SampleHandle {
    tracing::debug!("Called");
    create_sample(driver)
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
    buff_num: u32,
    buffer: *const u8,
    len: u32,
) {
    tracing::debug!("Called");

    let Some(sample) = get_sample(sample) else {
        return;
    };

    let buffer_free = sample.buffer_free();

    match (buffer_free, buff_num) {
        (Buffer::First, 0) => {
            sample.set_buffer_free(Buffer::Second);
        }
        (Buffer::Second, 1) => {
            sample.set_buffer_free(Buffer::First);
        }
        _ => {}
    }
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

    let Some(sample) = get_sample(sample) else {
        return -1;
    };

    match sample.buffer_free() {
        Buffer::First => 0,
        Buffer::Second => 1,
    }
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

#[instrument(level = Level::INFO)]
pub unsafe extern "stdcall" fn start_sample(sample: SampleHandle) {
    tracing::info!("Called");

    if let Some(sample) = get_sample(sample) {
        tracing::info!("Sample's driver key: {:?}", sample.driver_key());
    }
}

#[instrument(level = Level::DEBUG)]
pub unsafe extern "stdcall" fn stop_sample(sample: SampleHandle) {
    tracing::debug!("Called");
}

#[instrument(level = Level::DEBUG)]
pub unsafe extern "stdcall" fn wave_out_open(
    dig_driver_out: *mut DriverHandle,
    _: *mut c_void,
    device_id: u32,
    wave_format: *const WAVEFORMATEX,
) -> i32 {
    tracing::debug!("Called");

    if dig_driver_out.is_null() {
        return -1;
    }

    let driver = create_driver();
    unsafe {
        *dig_driver_out = driver;
    }
    return 0;
}
