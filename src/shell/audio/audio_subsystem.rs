use std::ptr::NonNull;

use anyhow::Result;
use rodio::Player;

use crate::audio;

use super::midi_sequence::MidiSequence;

pub struct AudioSubsystem {
    current_midi_sequence: Option<NonNull<MidiSequence>>,
}

impl AudioSubsystem {
    pub fn new() -> Self {
        Self {
            current_midi_sequence: None,
        }
    }

    pub fn get_sink(&mut self) -> Result<Player> {
        Ok(Player::connect_new(&audio::primary()?))
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
