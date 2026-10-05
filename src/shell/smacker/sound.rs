use std::time::Duration;

use anyhow::Result;
use remech2_sys::shell;
use rodio::{OutputStream, OutputStreamBuilder, Sink, buffer::SamplesBuffer};

/// How an audio track's samples are laid out
#[derive(Clone, Copy)]
pub struct AudioFormat {
    pub rate: u32,
    pub bits: u8,
    pub channels: u8,
}

pub struct Sound {
    _stream: OutputStream,
    sink: Sink,
}

impl Sound {
    /// Open a stream and play `pcm`.
    /// Interleaved unsigned 8-bit or signed little-endian 16-bit samples, depending on the track's format.
    pub fn start(pcm: &[u8], format: AudioFormat) -> Result<Self> {
        let samples = to_f32(pcm, format.bits);
        let stream = OutputStreamBuilder::open_default_stream()?;
        let sink = Sink::connect_new(stream.mixer());

        // Set volume based on the global effects volume
        // The original game didn't do this and FMVs were always full volume
        let volume =
            unsafe { shell::g_soundConfig.m_effectsVolume }.clamp(0, 0x10000) as f32 / 65536.0;
        sink.set_volume(volume);

        sink.append(SamplesBuffer::new(
            u16::from(format.channels.max(1)),
            format.rate.max(1),
            samples,
        ));
        sink.play();
        Ok(Self {
            _stream: stream,
            sink,
        })
    }

    pub fn position(&self) -> Duration {
        self.sink.get_pos()
    }

    /// Jump playback to `pos`. Returns whether the seek took.
    pub fn seek(&self, pos: Duration) -> bool {
        self.sink.try_seek(pos).is_ok()
    }
}

/// Convert interleaved PCM to the mixer's f32 samples
fn to_f32(pcm: &[u8], bits: u8) -> Vec<f32> {
    if bits == 16 {
        let (samples, _) = pcm.as_chunks::<2>();
        samples
            .iter()
            .map(|&s| f32::from(i16::from_le_bytes(s)) / 32768.0)
            .collect()
    } else {
        pcm.iter()
            .map(|&s| (f32::from(s) - 128.0) / 128.0)
            .collect()
    }
}
