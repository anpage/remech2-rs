use std::time::Duration;

use anyhow::Result;
use rodio::{OutputStream, OutputStreamBuilder, Sink, buffer::SamplesBuffer};
use smacker::AudioInfo;

pub struct Sound {
    _stream: OutputStream,
    sink: Sink,
}

impl Sound {
    /// Open a stream and play `pcm`.
    /// Interleaved unsigned 8-bit or signed little-endian 16-bit samples, depending on the track's format.
    pub fn start(pcm: &[u8], info: AudioInfo) -> Result<Self> {
        let samples = to_f32(pcm, info.bits);
        let stream = OutputStreamBuilder::open_default_stream()?;
        let sink = Sink::connect_new(stream.mixer());
        sink.append(SamplesBuffer::new(
            u16::from(info.channels.max(1)),
            info.rate.max(1),
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
