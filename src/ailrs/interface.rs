use std::{ffi::c_void, num::NonZero, slice};

use tracing::{Level, instrument};
use windows::Win32::Media::Audio::{WAVE_FORMAT_PCM, WAVEFORMATEX};

use crate::ailrs::{
    sample::Buffer,
    storage::{create_driver, create_sample, get_sample, release_sample},
};

#[repr(C)]
#[derive(Clone, Copy, Debug)]
pub struct DriverHandle(Option<NonZero<u32>>);

impl DriverHandle {
    pub fn new(handle: u32) -> Self {
        Self(NonZero::new(handle))
    }

    pub fn id(self) -> Option<NonZero<u32>> {
        self.0
    }
}

#[repr(C)]
#[derive(Clone, Copy, Debug)]
pub struct SampleHandle(Option<NonZero<u32>>);

impl SampleHandle {
    pub fn new(handle: u32) -> Self {
        Self(NonZero::new(handle))
    }

    pub fn id(self) -> Option<NonZero<u32>> {
        self.0
    }
}

#[instrument(level = Level::DEBUG)]
pub unsafe extern "stdcall" fn allocate_file_sample(
    driver: DriverHandle,
    data: *const u8,
    _: i32,
) -> SampleHandle {
    tracing::debug!("Called");
    SampleHandle::new(0)
}

#[instrument(level = Level::DEBUG)]
pub unsafe extern "stdcall" fn allocate_sample_handle(driver: DriverHandle) -> SampleHandle {
    tracing::debug!("Called");
    create_sample(driver)
}

#[instrument(level = Level::DEBUG)]
pub unsafe extern "stdcall" fn end_sample(sample: SampleHandle) {
    tracing::debug!("Called");
    let Some(sample) = get_sample(sample) else {
        return;
    };
    sample.end();
}

#[instrument(level = Level::DEBUG)]
pub unsafe extern "stdcall" fn init_sample(sample: SampleHandle) {
    tracing::debug!("Called");
    let Some(sample) = get_sample(sample) else {
        return;
    };
    sample.init();
}

#[instrument(level = Level::DEBUG)]
pub unsafe extern "stdcall" fn load_sample_buffer(
    sample: SampleHandle,
    buff_num: u32,
    buffer: *const u8,
    len: u32,
) {
    tracing::debug!("Called");
    if buffer.is_null() || len == 0 {
        return;
    }

    let buff_num: Buffer = match buff_num {
        0 => Buffer::First,
        1 => Buffer::Second,
        _ => return,
    };

    let Some(sample) = get_sample(sample) else {
        return;
    };

    let data = unsafe { slice::from_raw_parts(buffer, len as usize) };

    sample.load_buffer(buff_num, data)
}

#[instrument(level = Level::DEBUG)]
pub unsafe extern "stdcall" fn register_eos_callback(
    sample_handle: SampleHandle,
    callback: unsafe extern "stdcall" fn(SampleHandle),
) {
    tracing::debug!("Called");
    let Some(sample) = get_sample(sample_handle) else {
        return;
    };
    sample.register_eos_callback(Box::new(move || unsafe { callback(sample_handle) }))
}

#[instrument(level = Level::DEBUG)]
pub unsafe extern "stdcall" fn release_sample_handle(sample: SampleHandle) {
    tracing::debug!("Called");
    release_sample(sample);
}

#[instrument(level = Level::DEBUG)]
pub unsafe extern "stdcall" fn resume_sample(sample: SampleHandle) {
    tracing::debug!("Called");
    let Some(sample) = get_sample(sample) else {
        return;
    };
    sample.resume();
}

#[instrument(level = Level::DEBUG)]
pub unsafe extern "stdcall" fn sample_buffer_ready(sample: SampleHandle) -> i32 {
    tracing::debug!("Called");
    let Some(sample) = get_sample(sample) else {
        return -1;
    };
    match sample.buffer_ready() {
        Buffer::First => 0,
        Buffer::Second => 1,
    }
}

#[instrument(level = Level::DEBUG)]
pub unsafe extern "stdcall" fn sample_user_data(sample: SampleHandle, index: u32) -> i32 {
    tracing::debug!("Called");
    let Some(sample) = get_sample(sample) else {
        return 0;
    };
    sample.user_data(index)
}

#[instrument(level = Level::DEBUG)]
pub unsafe extern "stdcall" fn set_preference(key: u32, value: u32) {
    tracing::debug!("Called");
    // TODO: Figure out preferences and what they mean
}

#[instrument(level = Level::DEBUG)]
pub unsafe extern "stdcall" fn set_sample_loop_count(sample: SampleHandle, loop_count: u32) {
    tracing::debug!("Called");
    let Some(sample) = get_sample(sample) else {
        return;
    };
    sample.set_loop_count(loop_count);
}

#[instrument(level = Level::DEBUG)]
pub unsafe extern "stdcall" fn set_sample_pan(sample: SampleHandle, pan: i32) {
    tracing::debug!("Called");
    let Some(sample) = get_sample(sample) else {
        return;
    };
    sample.set_pan(pan);
}

#[instrument(level = Level::DEBUG)]
pub unsafe extern "stdcall" fn set_sample_playback_rate(sample: SampleHandle, playback_rate: i32) {
    tracing::debug!("Called");
    let Some(sample) = get_sample(sample) else {
        return;
    };
    sample.set_playback_rate(playback_rate);
}

#[instrument(level = Level::DEBUG)]
pub unsafe extern "stdcall" fn set_sample_type(sample: SampleHandle, format: i32, flags: u32) {
    tracing::debug!("Called");
    let Some(sample) = get_sample(sample) else {
        return;
    };
    sample.set_type(format, flags);
}

#[instrument(level = Level::DEBUG)]
pub unsafe extern "stdcall" fn set_sample_user_data(
    sample: SampleHandle,
    index: u32,
    user_data: i32,
) {
    tracing::debug!("Called");
    let Some(sample) = get_sample(sample) else {
        return;
    };
    sample.set_user_data(index, user_data);
}

#[instrument(level = Level::DEBUG)]
pub unsafe extern "stdcall" fn set_sample_volume(sample: SampleHandle, volume: i32) {
    tracing::debug!("Called");
    let Some(sample) = get_sample(sample) else {
        return;
    };
    sample.set_volume(volume);
}

#[instrument(level = Level::INFO)]
pub unsafe extern "stdcall" fn start_sample(sample: SampleHandle) {
    tracing::info!("Called");
    let Some(sample) = get_sample(sample) else {
        return;
    };
    sample.start();
}

#[instrument(level = Level::DEBUG)]
pub unsafe extern "stdcall" fn stop_sample(sample: SampleHandle) {
    tracing::debug!("Called");
    let Some(sample) = get_sample(sample) else {
        return;
    };
    sample.stop();
}

#[instrument(level = Level::DEBUG)]
pub unsafe extern "stdcall" fn wave_out_open(
    dig_driver_out: *mut DriverHandle,
    _: *mut c_void,
    device_id: u32,
    wave_format: *const WAVEFORMATEX,
) -> i32 {
    tracing::debug!("Called");

    if dig_driver_out.is_null() || wave_format.is_null() {
        return -1;
    }

    let wave_format = unsafe { *wave_format };

    let WAVEFORMATEX {
        wFormatTag,
        nChannels,
        nSamplesPerSec,
        ..
    } = wave_format;

    if wFormatTag as u32 != WAVE_FORMAT_PCM {
        tracing::error!("Unsupported wave format");
        return -1;
    }

    let driver = create_driver(nChannels, nSamplesPerSec);
    unsafe {
        *dig_driver_out = driver;
    }
    if driver.id().is_none() {
        return -1;
    }
    0
}
