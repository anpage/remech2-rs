use std::sync::{LazyLock, Mutex};

use slotmap::{KeyData, SlotMap, new_key_type};

use crate::ailrs::{
    driver::Driver,
    interface::{DriverHandle, SampleHandle},
    sample::Sample,
};

new_key_type! { pub struct DriverKey; }

type DriverSlotMap = SlotMap<DriverKey, Driver>;
static DRIVERS: LazyLock<Mutex<DriverSlotMap>> = LazyLock::new(|| Mutex::new(SlotMap::with_key()));

pub fn create_driver(channels: u16, sample_rate: u32) -> DriverHandle {
    let ffi_handle = DRIVERS
        .lock()
        .unwrap()
        .insert(Driver::new(channels, sample_rate))
        .0
        .as_ffi();
    DriverHandle::new(ffi_handle)
}

pub fn driver_exists(handle: DriverHandle) -> bool {
    DRIVERS
        .lock()
        .unwrap()
        .contains_key(DriverKey(KeyData::from_ffi(handle.raw())))
}

pub fn get_driver(key: DriverKey) -> Option<Driver> {
    DRIVERS.lock().unwrap().get(key).cloned()
}

new_key_type! { pub struct SampleKey; }

type SampleSlotMap = SlotMap<SampleKey, Sample>;
static SAMPLES: LazyLock<Mutex<SampleSlotMap>> = LazyLock::new(|| Mutex::new(SlotMap::with_key()));

pub fn create_sample(driver_handle: DriverHandle) -> SampleHandle {
    if !driver_exists(driver_handle) {
        return SampleHandle::new(0);
    }

    let ffi_handle = SAMPLES
        .lock()
        .unwrap()
        .insert(Sample::new(DriverKey(KeyData::from_ffi(
            driver_handle.raw(),
        ))))
        .0
        .as_ffi();

    SampleHandle::new(ffi_handle)
}

pub fn get_sample(handle: SampleHandle) -> Option<Sample> {
    SAMPLES
        .lock()
        .unwrap()
        .get(SampleKey(KeyData::from_ffi(handle.raw())))
        .cloned()
}

pub fn release_sample(handle: SampleHandle) {
    SAMPLES
        .lock()
        .unwrap()
        .remove(SampleKey(KeyData::from_ffi(handle.raw())));
}

pub fn shutdown() {
    SAMPLES.lock().unwrap().clear();
    DRIVERS.lock().unwrap().clear();
}
