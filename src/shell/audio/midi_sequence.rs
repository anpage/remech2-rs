use std::io::Cursor;

use rodio::Sink;

use crate::{midi_source::MidiSource, xmi::XmiFile};

use super::audio_subsystem::AudioSubsystem;

pub struct MidiSequence {
    sink: Sink,

    /// Volume in the range 0..128
    volume: i32,
}

impl MidiSequence {
    pub fn new(subsystem: *mut AudioSubsystem, data: &[u8]) -> Self {
        let midi_file = {
            let xmi_file = XmiFile::new(Cursor::new(data)).unwrap();
            xmi_file.to_smf_file()
        };

        let source = MidiSource::new(&midi_file[..]).unwrap();
        let driver = unsafe { (*subsystem).get_digital_driver().unwrap() };
        let sink = Sink::try_new(&driver).unwrap();
        sink.pause();
        sink.append(source);

        Self { sink, volume: 127 }
    }

    pub fn start(&mut self) {
        self.apply_current_volume();
        self.sink.play();
    }

    pub fn stop(&mut self) {
        self.sink.pause();
    }

    pub fn apply_current_volume(&mut self) {
        self.set_volume(self.volume);
    }

    pub fn set_volume(&mut self, volume: i32) {
        let volume = volume.clamp(0, 127);
        self.volume = volume;
        self.sink.set_volume(volume as f32 / 127.0);
    }
}
