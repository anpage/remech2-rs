use std::{ffi::c_void, num::NonZero, slice};

use tracing::{Level, instrument};
use windows::Win32::Media::Audio::{WAVE_FORMAT_PCM, WAVEFORMATEX};

use crate::ailrs::storage::{create_driver, create_sample, get_sample, release_sample};

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
    SampleHandle::new(0)
}

#[instrument(level = Level::DEBUG)]
pub unsafe extern "stdcall" fn allocate_sample_handle(driver: DriverHandle) -> SampleHandle {
    create_sample(driver)
}

#[instrument(level = Level::DEBUG)]
pub unsafe extern "stdcall" fn end_sample(sample: SampleHandle) {
    let Some(sample) = get_sample(sample) else {
        return;
    };
    sample.end();
}

#[instrument(level = Level::DEBUG)]
pub unsafe extern "stdcall" fn init_sample(sample: SampleHandle) {
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
    if buff_num > 1 {
        return;
    }

    let Some(sample) = get_sample(sample) else {
        return;
    };

    let data = if buffer.is_null() {
        &[]
    } else {
        unsafe { slice::from_raw_parts(buffer, len as usize) }
    };

    sample.load_buffer(buff_num as usize, data)
}

#[instrument(level = Level::DEBUG)]
pub unsafe extern "stdcall" fn register_eos_callback(
    sample_handle: SampleHandle,
    callback: Option<unsafe extern "stdcall" fn(SampleHandle)>,
) -> Option<unsafe extern "stdcall" fn(SampleHandle)> {
    get_sample(sample_handle)?.register_eos_callback(callback)
}

#[instrument(level = Level::DEBUG)]
pub unsafe extern "stdcall" fn release_sample_handle(sample: SampleHandle) {
    release_sample(sample);
}

#[instrument(level = Level::DEBUG)]
pub unsafe extern "stdcall" fn resume_sample(sample: SampleHandle) {
    let Some(sample) = get_sample(sample) else {
        return;
    };
    sample.resume();
}

pub unsafe extern "stdcall" fn sample_buffer_ready(sample: SampleHandle) -> i32 {
    let Some(sample) = get_sample(sample) else {
        return -1;
    };
    sample.buffer_ready()
}

pub unsafe extern "stdcall" fn sample_user_data(sample: SampleHandle, index: u32) -> i32 {
    let Some(sample) = get_sample(sample) else {
        return 0;
    };
    sample.user_data(index)
}

pub unsafe extern "stdcall" fn set_preference(_key: u32, _value: u32) {
    // TODO: Figure out preferences and what they mean
}

pub unsafe extern "stdcall" fn set_sample_loop_count(sample: SampleHandle, loop_count: u32) {
    let Some(sample) = get_sample(sample) else {
        return;
    };
    sample.set_loop_count(loop_count);
}

pub unsafe extern "stdcall" fn set_sample_pan(sample: SampleHandle, pan: i32) {
    let Some(sample) = get_sample(sample) else {
        return;
    };
    sample.set_pan(pan);
}

pub unsafe extern "stdcall" fn set_sample_playback_rate(sample: SampleHandle, playback_rate: i32) {
    let Some(sample) = get_sample(sample) else {
        return;
    };
    sample.set_playback_rate(playback_rate);
}

pub unsafe extern "stdcall" fn set_sample_type(sample: SampleHandle, format: i32, flags: u32) {
    let Some(sample) = get_sample(sample) else {
        return;
    };
    sample.set_type(format, flags);
}

pub unsafe extern "stdcall" fn set_sample_user_data(
    sample: SampleHandle,
    index: u32,
    user_data: i32,
) {
    let Some(sample) = get_sample(sample) else {
        return;
    };
    sample.set_user_data(index, user_data);
}

pub unsafe extern "stdcall" fn set_sample_volume(sample: SampleHandle, volume: i32) {
    let Some(sample) = get_sample(sample) else {
        return;
    };
    sample.set_volume(volume);
}

pub unsafe extern "stdcall" fn start_sample(sample: SampleHandle) {
    let Some(sample) = get_sample(sample) else {
        return;
    };
    sample.start();
}

pub unsafe extern "stdcall" fn stop_sample(sample: SampleHandle) {
    let Some(sample) = get_sample(sample) else {
        return;
    };
    sample.stop();
}

pub unsafe extern "stdcall" fn wave_out_open(
    dig_driver_out: *mut DriverHandle,
    _: *mut c_void,
    _device_id: u32,
    wave_format: *const WAVEFORMATEX,
) -> i32 {
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

pub unsafe extern "stdcall" fn serve() {
    for (handle, callback) in crate::ailrs::storage::drain_pending_eos() {
        unsafe { callback(handle) };
    }
}
