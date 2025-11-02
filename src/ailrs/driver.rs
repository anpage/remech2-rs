use std::sync::{Arc, Mutex};

use rodio::{OutputStream, OutputStreamBuilder, Sink};

struct Inner {
    stream: OutputStream,
}

#[derive(Clone)]
pub struct Driver(Arc<Mutex<Inner>>);

impl Driver {
    pub fn new() -> Self {
        let stream = OutputStreamBuilder::open_default_stream().unwrap();
        Driver(Arc::new(Mutex::new(Inner { stream })))
    }

    pub fn connect_new_sink(&self) -> Sink {
        let stream_handle = &self.0.lock().unwrap().stream;
        Sink::connect_new(stream_handle.mixer())
    }
}
