use std::{
    collections::HashMap,
    io::Cursor,
    sync::{Arc, Mutex},
};

use rodio::{
    Decoder, Sink, Source, conversions::SampleTypeConverter, source::EmptyCallback,
    static_buffer::StaticSamplesBuffer,
};

use crate::ailrs::{
    interface::SampleHandle,
    pcm_source::PcmSource,
    storage::{DriverKey, get_driver},
};

pub trait EosCallback: Fn() + Send {
    fn clone_box(&self) -> Box<dyn EosCallback>;
}

impl<T> EosCallback for T
where
    T: 'static + Fn() + Clone + Send,
{
    fn clone_box(&self) -> Box<dyn EosCallback> {
        Box::new(self.clone())
    }
}

impl Clone for Box<dyn EosCallback> {
    fn clone(&self) -> Self {
        (**self).clone_box()
    }
}

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
    first_buffer: Vec<u8>,
    second_buffer: Vec<u8>,
    eos_callback: Option<Box<dyn EosCallback>>,
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
            first_buffer: vec![],
            second_buffer: vec![],
            eos_callback: None,
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
        let inner = self.0.lock().unwrap();
        // let buffer = match buffer {
        //     Buffer::First => &mut inner.first_buffer,
        //     Buffer::Second => &mut inner.second_buffer,
        // };
        // buffer.clear();
        // buffer.extend_from_slice(data);

        let source = PcmSource::new(data);

        if let Some(ref sink) = inner.sink {
            sink.append(source.low_pass(5500));
            if let Some(callback) = inner.eos_callback.clone() {
                sink.append(EmptyCallback::new(callback));
            }
        }
    }

    pub fn register_eos_callback(&self, callback: Box<dyn EosCallback>) {
        let mut inner = self.0.lock().unwrap();
        inner.eos_callback = Some(callback);
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
