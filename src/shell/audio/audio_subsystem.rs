use std::ptr::NonNull;

use anyhow::Result;
use rodio::{DeviceSinkBuilder, MixerDeviceSink, Player};

use super::midi_sequence::MidiSequence;

pub struct AudioSubsystem {
    stream_handle: Option<MixerDeviceSink>,
    current_midi_sequence: Option<NonNull<MidiSequence>>,
}

impl AudioSubsystem {
    pub fn new() -> Self {
        Self {
            stream_handle: None,
            current_midi_sequence: None,
        }
    }

    pub fn get_sink(&mut self) -> Result<Player> {
        if let Some(stream_handle) = &self.stream_handle {
            Ok(Player::connect_new(stream_handle.mixer()))
        } else {
            let stream_handle = DeviceSinkBuilder::open_default_sink()?;
            let sink = Player::connect_new(stream_handle.mixer());
            self.stream_handle = Some(stream_handle);
            Ok(sink)
        }
    }

    pub fn apply_midi_volume(&mut self) {
        if let Some(mut midi_sequence) = self.current_midi_sequence {
            unsafe { midi_sequence.as_mut().apply_current_volume() }
        }
    }

    pub fn current_midi_sequence(&self) -> Option<NonNull<MidiSequence>> {
        self.current_midi_sequence
    }

    pub fn set_current_midi_sequence(&mut self, sequence: Option<NonNull<MidiSequence>>) {
        self.current_midi_sequence = sequence;
    }
}
