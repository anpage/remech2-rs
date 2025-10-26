use std::ptr::NonNull;

use anyhow::Result;
use rodio::{OutputStream, OutputStreamBuilder, Sink};

use super::midi_sequence::MidiSequence;

pub struct AudioSubsystem {
    stream_handle: Option<OutputStream>,
    current_midi_sequence: Option<NonNull<MidiSequence>>,
}

impl AudioSubsystem {
    pub fn new() -> Self {
        Self {
            stream_handle: None,
            current_midi_sequence: None,
        }
    }

    pub fn get_sink(&mut self) -> Result<Sink> {
        if let Some(stream_handle) = &self.stream_handle {
            Ok(Sink::connect_new(stream_handle.mixer()))
        } else {
            let stream_handle = OutputStreamBuilder::open_default_stream()?;
            let sink = Sink::connect_new(stream_handle.mixer());
            self.stream_handle = Some(stream_handle);
            Ok(sink)
        }
    }

    pub fn close_digital_driver(&mut self) {
        self.stream_handle.take();
    }

    pub fn apply_midi_volume(&mut self) {
        if let Some(mut midi_sequence) = self.current_midi_sequence {
            unsafe { midi_sequence.as_mut().apply_current_volume() }
        }
    }

    pub fn active_sequence_count(&self) -> u32 {
        0
    }
}
