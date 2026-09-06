use std::sync::{Arc, Mutex};

use rodio::{OutputStream, OutputStreamBuilder, Sink};

struct Inner {
    stream: OutputStream,
}

#[derive(Clone)]
pub struct Driver(Arc<Mutex<Inner>>);

impl Driver {
    pub fn new(channels: u16, _sample_rate: u32) -> Option<Self> {
        let stream = OutputStreamBuilder::from_default_device()
            .and_then(|b| b.with_channels(channels).open_stream())
            .inspect_err(|e| tracing::error!("failed to open audio stream: {e}"))
            .ok()?;
        Some(Driver(Arc::new(Mutex::new(Inner { stream }))))
    }

    pub fn connect_new_sink(&self) -> Sink {
        let stream_handle = &self.0.lock().unwrap().stream;
        Sink::connect_new(stream_handle.mixer())
    }
}
