use std::sync::{Arc, Mutex};

use crate::ailrs::storage::DriverKey;

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

    pub fn driver_key(&self) -> DriverKey {
        self.inner.lock().unwrap().driver
    }

    pub fn buffer_free(&self) -> Buffer {
        self.inner.lock().unwrap().buffer_free
    }

    pub fn set_buffer_free(&self, buffer: Buffer) {
        let mut inner = self.inner.lock().unwrap();
        inner.buffer_free = buffer;
    }
}
