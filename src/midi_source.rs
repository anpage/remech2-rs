use std::{io::Read, sync::Arc};

use anyhow::Result;
use rodio::Source;
use rustysynth::{
    MidiFile, MidiFileLoopType, MidiFileSequencer, SoundFont, Synthesizer, SynthesizerSettings,
};

enum StereoChannel {
    Left,
    Right,
}

/// A rodio source that uses RustySynth to play a MIDI file.
///
/// A little specialized for ReMech2 because it explicitly uses the XMI-style loop events and always loops.
pub struct MidiSource {
    sequencer: MidiFileSequencer,
    last_channel: StereoChannel,
    right_sample: f32,
}

impl MidiSource {
    pub fn new<T: Read>(mut midi_file: T) -> Result<Self> {
        // TODO: Allow specifying a sound font file.
        let sf2 = include_bytes!("../GeneralUser GS v1.471.sf2");
        let sound_font = Arc::new(SoundFont::new(&mut &sf2[..])?);

        // The "FinalFantasy" loop type is identical to XMI. Only one song (Clan Wolf's training screen) uses it.
        let midi_file = Arc::new(MidiFile::new_with_loop_type(
            &mut midi_file,
            MidiFileLoopType::FinalFantasy,
        )?);

        let settings = SynthesizerSettings::new(44100);
        let synthesizer = Synthesizer::new(&sound_font, &settings)?;
        let mut sequencer = MidiFileSequencer::new(synthesizer);

        // All of the shell's background songs are intended to loop, so just always do it.
        sequencer.play(&midi_file, true);

        Ok(Self {
            sequencer,
            last_channel: StereoChannel::Right,
            right_sample: 0.0,
        })
    }
}

impl Iterator for MidiSource {
    type Item = f32;

    fn next(&mut self) -> Option<Self::Item> {
        if self.sequencer.end_of_sequence() {
            return None;
        }

        // Rodio takes interleaved samples but RustySynth gives both channels at once,
        // so we need to cache the right channel and alternate.
        match self.last_channel {
            StereoChannel::Left => {
                self.last_channel = StereoChannel::Right;
                Some(self.right_sample)
            }
            StereoChannel::Right => {
                let mut left = [0.0; 1];
                let mut right = [0.0; 1];
                self.sequencer.render(&mut left, &mut right);
                self.right_sample = right[0];
                self.last_channel = StereoChannel::Left;
                Some(left[0])
            }
        }
    }
}

impl Source for MidiSource {
    fn current_frame_len(&self) -> Option<usize> {
        // We never change sample rate or channels, so frames are infinite.
        None
    }

    fn channels(&self) -> u16 {
        // Always stereo.
        2
    }

    fn sample_rate(&self) -> u32 {
        // Always 44.1 kHz.
        44100
    }

    fn total_duration(&self) -> Option<std::time::Duration> {
        // The MIDI music loops forever, so duration is infinite.
        None
    }
}

#[cfg(test)]
mod tests {
    use crate::{midi_source::MidiSource, xmi::XmiFile};
    use rodio::{OutputStream, Sink};
    use std::{fs::File, io::BufReader};

    #[test]
    fn test_midi() {
        let (_stream, stream_handle) = OutputStream::try_default().unwrap();
        let sink = Sink::try_new(&stream_handle).unwrap();

        let midi_file = {
            // TODO: Get the test data from the DATABASE.MW2 file.
            let xmi_file = File::open("data/dumped_42.xmi").unwrap();
            let xmi_file = XmiFile::new(BufReader::new(xmi_file)).unwrap();
            xmi_file.to_smf_file()
        };

        let source = MidiSource::new(&midi_file[..]).unwrap();

        sink.append(source);
        sink.play();
        sink.sleep_until_end();
    }
}
