use std::{ffi::c_void, num::NonZero, slice};

use tracing::{Level, instrument};

use crate::ailrs::storage::{create_driver, create_sample, get_sample, release_sample};

#[repr(transparent)]
#[derive(Clone, Copy, Debug)]
pub struct DriverHandle(Option<NonZero<u64>>);

impl DriverHandle {
    pub fn new(handle: u64) -> Self {
        Self(NonZero::new(handle))
    }

    pub fn id(self) -> Option<NonZero<u64>> {
        self.0
    }
}

#[repr(transparent)]
#[derive(Clone, Copy, Debug)]
pub struct SampleHandle(Option<NonZero<u64>>);

impl SampleHandle {
    pub fn new(handle: u64) -> Self {
        Self(NonZero::new(handle))
    }

    pub fn id(self) -> Option<NonZero<u64>> {
        self.0
    }
}

#[instrument(level = Level::DEBUG)]
#[unsafe(export_name = "AIL_allocate_file_sample")]
pub unsafe extern "system" fn allocate_file_sample(
    driver: DriverHandle,
    data: *const u8,
    _: i32,
) -> SampleHandle {
    SampleHandle::new(0)
}

#[instrument(level = Level::DEBUG)]
#[unsafe(export_name = "AIL_allocate_sample_handle")]
pub unsafe extern "system" fn allocate_sample_handle(driver: DriverHandle) -> SampleHandle {
    create_sample(driver)
}

#[instrument(level = Level::DEBUG)]
#[unsafe(export_name = "AIL_end_sample")]
pub unsafe extern "system" fn end_sample(sample: SampleHandle) {
    let Some(sample) = get_sample(sample) else {
        return;
    };
    sample.end();
}

#[instrument(level = Level::DEBUG)]
#[unsafe(export_name = "AIL_init_sample")]
pub unsafe extern "system" fn init_sample(sample: SampleHandle) {
    let Some(sample) = get_sample(sample) else {
        return;
    };
    sample.init();
}

#[instrument(level = Level::DEBUG)]
#[unsafe(export_name = "AIL_load_sample_buffer")]
pub unsafe extern "system" fn load_sample_buffer(
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
#[unsafe(export_name = "AIL_register_EOS_callback")]
pub unsafe extern "system" fn register_eos_callback(
    sample_handle: SampleHandle,
    callback: Option<unsafe extern "system" fn(SampleHandle)>,
) -> Option<unsafe extern "system" fn(SampleHandle)> {
    get_sample(sample_handle)?.register_eos_callback(callback)
}

#[instrument(level = Level::DEBUG)]
#[unsafe(export_name = "AIL_release_sample_handle")]
pub unsafe extern "system" fn release_sample_handle(sample: SampleHandle) {
    release_sample(sample);
}

#[instrument(level = Level::DEBUG)]
#[unsafe(export_name = "AIL_resume_sample")]
pub unsafe extern "system" fn resume_sample(sample: SampleHandle) {
    let Some(sample) = get_sample(sample) else {
        return;
    };
    sample.resume();
}

#[unsafe(export_name = "AIL_sample_buffer_ready")]
pub unsafe extern "system" fn sample_buffer_ready(sample: SampleHandle) -> i32 {
    let Some(sample) = get_sample(sample) else {
        return -1;
    };
    sample.buffer_ready()
}

#[unsafe(export_name = "AIL_sample_user_data")]
pub unsafe extern "system" fn sample_user_data(sample: SampleHandle, index: u32) -> isize {
    let Some(sample) = get_sample(sample) else {
        return 0;
    };
    sample.user_data(index)
}

#[unsafe(export_name = "AIL_set_preference")]
pub unsafe extern "system" fn set_preference(_key: u32, _value: u32) {
    // TODO: Figure out preferences and what they mean
}

#[unsafe(export_name = "AIL_set_sample_loop_count")]
pub unsafe extern "system" fn set_sample_loop_count(sample: SampleHandle, loop_count: u32) {
    let Some(sample) = get_sample(sample) else {
        return;
    };
    sample.set_loop_count(loop_count);
}

#[unsafe(export_name = "AIL_set_sample_pan")]
pub unsafe extern "system" fn set_sample_pan(sample: SampleHandle, pan: i32) {
    let Some(sample) = get_sample(sample) else {
        return;
    };
    sample.set_pan(pan);
}

#[unsafe(export_name = "AIL_set_sample_playback_rate")]
pub unsafe extern "system" fn set_sample_playback_rate(sample: SampleHandle, playback_rate: i32) {
    let Some(sample) = get_sample(sample) else {
        return;
    };
    sample.set_playback_rate(playback_rate);
}

#[unsafe(export_name = "AIL_set_sample_type")]
pub unsafe extern "system" fn set_sample_type(sample: SampleHandle, format: i32, flags: u32) {
    let Some(sample) = get_sample(sample) else {
        return;
    };
    sample.set_type(format, flags);
}

#[unsafe(export_name = "AIL_set_sample_user_data")]
pub unsafe extern "system" fn set_sample_user_data(
    sample: SampleHandle,
    index: u32,
    user_data: isize,
) {
    let Some(sample) = get_sample(sample) else {
        return;
    };
    sample.set_user_data(index, user_data);
}

#[unsafe(export_name = "AIL_set_sample_volume")]
pub unsafe extern "system" fn set_sample_volume(sample: SampleHandle, volume: i32) {
    let Some(sample) = get_sample(sample) else {
        return;
    };
    sample.set_volume(volume);
}

#[unsafe(export_name = "AIL_start_sample")]
pub unsafe extern "system" fn start_sample(sample: SampleHandle) {
    let Some(sample) = get_sample(sample) else {
        return;
    };
    sample.start();
}

#[unsafe(export_name = "AIL_stop_sample")]
pub unsafe extern "system" fn stop_sample(sample: SampleHandle) {
    let Some(sample) = get_sample(sample) else {
        return;
    };
    sample.stop();
}

#[unsafe(export_name = "AIL_waveOutOpen")]
pub unsafe extern "system" fn wave_out_open(
    dig_driver_out: *mut DriverHandle,
    _: *mut c_void,
    _device_id: u32,
    channels: u16,
    samples_per_sec: u32,
) -> i32 {
    if dig_driver_out.is_null() {
        return -1;
    }

    let driver = create_driver(channels, samples_per_sec);
    unsafe {
        *dig_driver_out = driver;
    }
    if driver.id().is_none() {
        return -1;
    }
    0
}

/// Releases every sample and driver
#[unsafe(export_name = "AIL_shutdown")]
pub unsafe extern "system" fn shutdown() {
    crate::ailrs::storage::shutdown();
}

#[unsafe(export_name = "AIL_serve")]
pub unsafe extern "system" fn serve() {
    for (handle, callback) in crate::ailrs::storage::drain_pending_eos() {
        unsafe { callback(handle) };
    }
}
