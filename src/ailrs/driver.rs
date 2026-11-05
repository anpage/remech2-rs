use rodio::{ChannelCount, SampleRate, mixer::Mixer};
use tracing::error;

use crate::audio;

#[derive(Clone)]
pub struct Driver {
    mixer: Mixer,
    rate: SampleRate,
    mono: bool,
}

impl Driver {
    pub fn new(channels: u16, _sample_rate: u32) -> Option<Self> {
        let channels = ChannelCount::new(channels)?;
        let mixer = audio::primary()
            .inspect_err(|e| error!("failed to open audio stream: {e}"))
            .ok()?;
        let rate = audio::sample_rate()?;

        Some(Self {
            mixer,
            rate,
            mono: channels.get() == 1,
        })
    }

    pub fn mixer(&self) -> Mixer {
        self.mixer.clone()
    }

    pub fn is_mono(&self) -> bool {
        self.mono
    }

    /// The device's actual output rate. Voices handle resampling to this.
    pub fn sample_rate(&self) -> SampleRate {
        self.rate
    }
}
