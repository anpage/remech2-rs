use std::sync::{Arc, Mutex};

use crate::ailrs::{interface::SampleHandle, storage::DriverKey};

#[derive(Clone, Copy)]
pub enum Buffer {
    First,
    Second,
}

struct SampleInner {
    driver: DriverKey,
    buffer_free: Buffer,
}

#[derive(Clone)]
pub struct Sample {
    inner: Arc<Mutex<SampleInner>>,
}

impl Sample {
    pub fn new(driver: DriverKey) -> Self {
        return Self {
            inner: Arc::new(Mutex::new(SampleInner {
                driver,
                buffer_free: Buffer::First,
            })),
        };
    }

    pub fn end(&self) {
        todo!()
    }

    pub fn init(&self) {
        todo!()
    }

    pub fn load_buffer(&self, buffer: Buffer, data: &[u8]) {
        todo!()
    }

    pub fn register_eos_callback<F>(&self, callback: F)
    where
        F: Fn(SampleHandle),
    {
        todo!()
    }

    pub fn resume(&self) {
        todo!()
    }

    pub fn buffer_ready(&self) -> Buffer {
        todo!()
    }

    pub fn user_data(&self, index: u32) -> i32 {
        todo!()
    }

    pub fn set_loop_count(&self, loop_count: u32) {
        todo!()
    }

    pub fn set_pan(&self, pan: i32) {
        todo!()
    }

    pub fn set_playback_rate(&self, playback_rate: i32) {
        todo!()
    }

    pub fn set_type(&self, format: i32, flags: u32) {
        todo!()
    }

    pub fn set_user_data(&self, index: u32, user_data: i32) {
        todo!()
    }

    pub fn set_volume(&self, volume: i32) {
        todo!()
    }

    pub fn start(&self) {
        todo!()
    }

    pub fn stop(&self) {
        todo!()
    }
}
