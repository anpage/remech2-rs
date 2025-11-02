use std::sync::{LazyLock, Mutex};

use slotmap::{KeyData, SlotMap, new_key_type};

use crate::ailrs::{
    driver::Driver, interface::DriverHandle, interface::SampleHandle, sample::Sample,
};

new_key_type! { pub struct DriverKey; }

type DriverSlotMap = SlotMap<DriverKey, Driver>;
static DRIVERS: LazyLock<Mutex<DriverSlotMap>> = LazyLock::new(|| Mutex::new(SlotMap::with_key()));

pub fn create_driver() -> DriverHandle {
    let ffi_handle = DRIVERS.lock().unwrap().insert(Driver {}).0.as_ffi();
    DriverHandle::new(ffi_handle)
}

pub fn driver_exists(handle: DriverHandle) -> bool {
    DRIVERS
        .lock()
        .unwrap()
        .contains_key(DriverKey(KeyData::from_ffi(handle.raw())))
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
