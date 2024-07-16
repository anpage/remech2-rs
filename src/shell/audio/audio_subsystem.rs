use std::ptr::NonNull;

use anyhow::Result;
use rodio::{OutputStream, OutputStreamHandle};

use super::midi_sequence::MidiSequence;

pub struct AudioSubsystem {
    _stream: Option<OutputStream>,
    stream_handle: Option<OutputStreamHandle>,
    current_midi_sequence: Option<NonNull<MidiSequence>>,
}

impl AudioSubsystem {
    pub fn new() -> Self {
        Self {
            _stream: None,
            stream_handle: None,
            current_midi_sequence: None,
        }
    }

    pub fn get_digital_driver(&mut self) -> Result<OutputStreamHandle> {
        if let Some(stream_handle) = &self.stream_handle {
            Ok(stream_handle.clone())
        } else {
            let (_stream, stream_handle) = OutputStream::try_default()?;
            self._stream = Some(_stream);
            self.stream_handle = Some(stream_handle.clone());
            Ok(stream_handle)
        }
    }

    pub fn close_digital_driver(&mut self) {
        self.stream_handle = None;
        self._stream = None;
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
