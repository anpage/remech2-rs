use std::{io::Cursor, ptr::NonNull};

use rodio::Sink;

use crate::{midi_source::MidiSource, shell::audio::G_MIDI_VOLUME, xmi::XmiFile};

use super::audio_subsystem::AudioSubsystem;

pub struct MidiSequence {
    subsystem: *mut AudioSubsystem,
    sink: Sink,

    /// Volume as a percentage, 0..=100
    volume: i32,
}

impl MidiSequence {
    pub fn new(subsystem: *mut AudioSubsystem, data: &[u8]) -> Self {
        let midi_file = {
            let xmi_file = XmiFile::new(Cursor::new(data)).unwrap();
            xmi_file.to_smf_file()
        };

        let source = MidiSource::new(&midi_file[..]).unwrap();
        let sink = unsafe { (*subsystem).get_sink().unwrap() };
        sink.pause();
        sink.append(source);

        Self {
            subsystem,
            sink,
            volume: 50,
        }
    }

    pub fn start(&mut self) {
        self.apply_current_volume();
        self.sink.play();
        let this = NonNull::new(self as *mut Self);
        if let Some(subsystem) = unsafe { self.subsystem.as_mut() } {
            subsystem.set_current_midi_sequence(this);
        }
    }

    pub fn stop(&mut self) {
        self.sink.pause();
    }

    pub fn apply_current_volume(&mut self) {
        let scaled = unsafe { (G_MIDI_VOLUME.get() as i64 * self.volume as i64) >> 16 };
        let seq_volume = (scaled as f64 * 1.27) as i32;
        self.sink
            .set_volume(seq_volume.clamp(0, 127) as f32 / 127.0);
    }

    pub fn set_volume(&mut self, volume: i32) {
        self.volume = volume;
        self.apply_current_volume();
    }
}

impl Drop for MidiSequence {
    fn drop(&mut self) {
        let this = NonNull::new(self as *mut Self);
        if let Some(subsystem) = unsafe { self.subsystem.as_mut() }
            && subsystem.current_midi_sequence() == this
        {
            subsystem.set_current_midi_sequence(None);
        }
    }
}
