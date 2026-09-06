use std::sync::{Arc, Mutex};

use rodio::{OutputStream, OutputStreamBuilder, mixer::Mixer};

struct Inner {
    // Keep stream alive inside the driver
    #[allow(dead_code)]
    stream: OutputStream,
    mixer: Mixer,
    mono: bool,
}

#[derive(Clone)]
pub struct Driver(Arc<Mutex<Inner>>);

impl Driver {
    pub fn new(channels: u16, _sample_rate: u32) -> Option<Self> {
        let stream = OutputStreamBuilder::from_default_device()
            .and_then(|b| b.with_channels(channels).open_stream())
            .inspect_err(|e| tracing::error!("failed to open audio stream: {e}"))
            .ok()?;
        let mixer = stream.mixer().clone();
        Some(Driver(Arc::new(Mutex::new(Inner {
            stream,
            mixer,
            mono: channels == 1,
        }))))
    }

    pub fn mixer(&self) -> Mixer {
        self.0.lock().unwrap().mixer.clone()
    }

    pub fn is_mono(&self) -> bool {
        self.0.lock().unwrap().mono
    }
}
