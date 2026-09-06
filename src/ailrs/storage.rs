use std::sync::{LazyLock, Mutex};

use slotmap::{Key, KeyData, SlotMap, new_key_type};

use crate::ailrs::{
    driver::Driver,
    interface::{DriverHandle, SampleHandle},
    sample::Sample,
};

new_key_type! { pub struct DriverKey; }

type DriverSlotMap = SlotMap<DriverKey, Driver>;
static DRIVERS: LazyLock<Mutex<DriverSlotMap>> = LazyLock::new(|| Mutex::new(SlotMap::with_key()));

fn driver_key(handle: DriverHandle) -> Option<DriverKey> {
    Some(KeyData::from_ffi(handle.id()?.get()).into())
}

pub fn create_driver(channels: u16, sample_rate: u32) -> DriverHandle {
    let Some(driver) = Driver::new(channels, sample_rate) else {
        return DriverHandle::new(0);
    };
    let key = DRIVERS.lock().unwrap().insert(driver);
    DriverHandle::new(key.data().as_ffi())
}

pub fn get_driver(key: DriverKey) -> Option<Driver> {
    DRIVERS.lock().unwrap().get(key).cloned()
}

new_key_type! { pub struct SampleKey; }

type SampleSlotMap = SlotMap<SampleKey, Sample>;
static SAMPLES: LazyLock<Mutex<SampleSlotMap>> = LazyLock::new(|| Mutex::new(SlotMap::with_key()));

fn sample_key(handle: SampleHandle) -> Option<SampleKey> {
    Some(KeyData::from_ffi(handle.id()?.get()).into())
}

pub fn create_sample(driver_handle: DriverHandle) -> SampleHandle {
    let Some(driver) = driver_key(driver_handle) else {
        return SampleHandle::new(0);
    };
    if !DRIVERS.lock().unwrap().contains_key(driver) {
        return SampleHandle::new(0);
    }
    let key = SAMPLES.lock().unwrap().insert(Sample::new(driver));
    SampleHandle::new(key.data().as_ffi())
}

pub fn get_sample(handle: SampleHandle) -> Option<Sample> {
    SAMPLES.lock().unwrap().get(sample_key(handle)?).cloned()
}

pub fn release_sample(handle: SampleHandle) {
    if let Some(key) = sample_key(handle) {
        SAMPLES.lock().unwrap().remove(key);
    }
}

pub fn shutdown() {
    SAMPLES.lock().unwrap().clear();
    DRIVERS.lock().unwrap().clear();
}
