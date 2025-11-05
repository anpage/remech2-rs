use std::sync::{Arc, Mutex};

use rodio::{OutputStream, OutputStreamBuilder, Sink};

struct Inner {
    stream: OutputStream,
}

#[derive(Clone)]
pub struct Driver(Arc<Mutex<Inner>>);

impl Driver {
    pub fn new(channels: u16, _sample_rate: u32) -> Self {
        let stream = OutputStreamBuilder::from_default_device()
            .unwrap()
            .with_channels(channels)
            // .with_sample_rate(48000)
            .open_stream()
            .unwrap();
        Driver(Arc::new(Mutex::new(Inner { stream })))
    }

    pub fn connect_new_sink(&self) -> Sink {
        let stream_handle = &self.0.lock().unwrap().stream;
        Sink::connect_new(stream_handle.mixer())
    }
}
