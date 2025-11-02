use std::{
    collections::HashMap,
    sync::{Arc, Mutex},
};

use rodio::Sink;

use crate::ailrs::{
    interface::SampleHandle,
    storage::{DriverKey, get_driver},
};

#[derive(Clone, Copy)]
pub enum Buffer {
    First,
    Second,
}

struct Inner {
    driver: DriverKey,
    buffer_free: Buffer,
    sink: Option<Sink>,
    user_data: HashMap<u32, i32>,
}

#[derive(Clone)]
pub struct Sample(Arc<Mutex<Inner>>);

impl Sample {
    pub fn new(driver: DriverKey) -> Self {
        return Self(Arc::new(Mutex::new(Inner {
            driver,
            buffer_free: Buffer::First,
            sink: None,
            user_data: HashMap::new(),
        })));
    }

    pub fn end(&self) {
        // TODO
    }

    pub fn init(&self) {
        let mut inner = self.0.lock().unwrap();
        let Some(driver) = get_driver(inner.driver) else {
            return;
        };
        (*inner).sink = Some(driver.connect_new_sink());
    }

    pub fn load_buffer(&self, buffer: Buffer, data: &[u8]) {
        // TODO
    }

    pub fn register_eos_callback<F>(&self, callback: F)
    where
        F: Fn(SampleHandle),
    {
        // TODO
    }

    pub fn resume(&self) {
        // TODO
    }

    pub fn buffer_ready(&self) -> Buffer {
        let inner = self.0.lock().unwrap();
        inner.buffer_free
    }

    pub fn user_data(&self, index: u32) -> i32 {
        let inner = self.0.lock().unwrap();
        if let Some(user_data) = inner.user_data.get(&index) {
            *user_data
        } else {
            -1
        }
    }

    pub fn set_loop_count(&self, loop_count: u32) {
        // TODO
    }

    pub fn set_pan(&self, pan: i32) {
        // TODO
    }

    pub fn set_playback_rate(&self, playback_rate: i32) {
        // TODO
    }

    pub fn set_type(&self, format: i32, flags: u32) {
        // TODO
    }

    pub fn set_user_data(&self, index: u32, user_data: i32) {
        let mut inner = self.0.lock().unwrap();
        inner.user_data.insert(index, user_data);
    }

    pub fn set_volume(&self, volume: i32) {
        // TODO
    }

    pub fn start(&self) {
        // TODO
    }

    pub fn stop(&self) {
        // TODO
    }
}
