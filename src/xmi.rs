use std::{
    io::{BufWriter, Read, Seek, SeekFrom, Write},
    iter::Peekable,
};

use anyhow::Result;

#[derive(Debug)]
pub struct XmiFile {
    sequences: Vec<XmiSequence>,
}

impl XmiFile {
    pub fn new<T: Read + Seek>(mut data: T) -> Result<Self> {
        let mut buf_4 = [0; 4];
        data.read_exact(&mut buf_4)?;
        if buf_4 != *b"FORM" {
            return Err(anyhow::anyhow!("Invalid XMI file"));
        }

        data.seek(SeekFrom::Current(4))?;

        data.read_exact(&mut buf_4)?;
        if buf_4 != *b"XDIR" {
            return Err(anyhow::anyhow!("Invalid XMI file"));
        }

        data.read_exact(&mut buf_4)?;
        if buf_4 != *b"INFO" {
            return Err(anyhow::anyhow!("Invalid XMI file"));
        }

        data.seek(SeekFrom::Current(4))?;

        let mut buf_2 = [0; 2];
        data.read_exact(&mut buf_2)?;
        let num_songs = i16::from_le_bytes(buf_2);

        data.read_exact(&mut buf_4)?;
        if buf_4 != *b"CAT " {
            return Err(anyhow::anyhow!("Invalid XMI file"));
        }

        data.seek(SeekFrom::Current(4))?;

        data.read_exact(&mut buf_4)?;
        if buf_4 != *b"XMID" {
            return Err(anyhow::anyhow!("Invalid XMI file"));
        }

        let mut sequences = Vec::with_capacity(num_songs as usize);

        for _ in 0..num_songs {
            let mut sequence = XmiSequence {
                patches: Vec::new(),
                events: Vec::new(),
            };

            data.read_exact(&mut buf_4)?;
            if buf_4 != *b"FORM" {
                return Err(anyhow::anyhow!("Invalid XMI file"));
            }

            data.seek(SeekFrom::Current(4))?;

            data.read_exact(&mut buf_4)?;
            if buf_4 != *b"XMID" {
                return Err(anyhow::anyhow!("Invalid XMI file"));
            }

            data.read_exact(&mut buf_4)?;
            if buf_4 == *b"TIMB" {
                data.seek(SeekFrom::Current(4))?;

                data.read_exact(&mut buf_2)?;
                let num_patches = u16::from_le_bytes(buf_2);

                for _ in 0..num_patches {
                    let mut buf = [0; 2];
                    data.read_exact(&mut buf)?;
                    sequence.patches.push(XmiPatch {
                        _patch: buf[0],
                        _bank: buf[1],
                    });
                }

                data.seek(SeekFrom::Current(4))?;
            }

            data.read_exact(&mut buf_4)?;
            let track_data_size = u32::from_be_bytes(buf_4);

            let mut track_data = vec![0; track_data_size as usize];
            data.read_exact(&mut track_data)?;

            let mut events: Vec<XmiEvent> = Vec::new();
            let mut data_iter = track_data.into_iter().peekable();
            loop {
                let event = Self::parse_xmi_event(&mut data_iter);
                if let Some(event) = event {
                    events.push(event);
                } else {
                    break;
                }
            }

            sequence.events = events;
            sequences.push(sequence);
        }

        Ok(Self { sequences })
    }

    fn parse_xmi_event<T: Iterator<Item = u8>>(data: &mut Peekable<T>) -> Option<XmiEvent> {
        let next_byte = *data.peek()?;

        let mut delta = 0;

        if next_byte & 0x80 == 0 {
            // XMI variable length format
            loop {
                let byte = *data.peek()?;
                if byte & 0x80 != 0 {
                    break;
                }
                delta += byte as u32;
                data.next();
            }
        }

        let event_byte = data.next()?;

        if event_byte == 0xFF {
            let meta_type = data.next()?;

            let length = Self::parse_midi_length(data);

            let mut meta_data = Vec::with_capacity(length as usize);
            for _ in 0..length {
                meta_data.push(data.next()?);
            }

            let meta_message = match meta_type {
                0x00 => MetaMessage::TrackNumber(if meta_data.len() >= 2 {
                    Some(u16::from_be_bytes([meta_data[0], meta_data[1]]))
                } else {
                    None
                }),
                0x01 => MetaMessage::Text(meta_data),
                0x02 => MetaMessage::Copyright(meta_data),
                0x03 => MetaMessage::TrackName(meta_data),
                0x04 => MetaMessage::InstrumentName(meta_data),
                0x05 => MetaMessage::Lyric(meta_data),
                0x06 => MetaMessage::Marker(meta_data),
                0x07 => MetaMessage::CuePoint(meta_data),
                0x08 => MetaMessage::ProgramName(meta_data),
                0x09 => MetaMessage::DeviceName(meta_data),
                0x20 if !meta_data.is_empty() => MetaMessage::MidiChannel(meta_data[0]),
                0x21 if !meta_data.is_empty() => MetaMessage::MidiPort(meta_data[0]),
                0x2F => MetaMessage::EndOfTrack,
                0x51 if meta_data.len() >= 3 => MetaMessage::Tempo(u32::from_be_bytes([
                    0,
                    meta_data[0],
                    meta_data[1],
                    meta_data[2],
                ])),
                0x58 if meta_data.len() >= 4 => MetaMessage::TimeSignature(
                    meta_data[0],
                    meta_data[1],
                    meta_data[2],
                    meta_data[3],
                ),
                0x59 => MetaMessage::KeySignature(meta_data[0] as i8, meta_data[1] != 0),
                0x7F => MetaMessage::SequencerSpecific(meta_data),
                _ => MetaMessage::Unknown(meta_type, meta_data),
            };

            return Some(XmiEvent {
                delta,
                kind: XmiEventKind::Meta(meta_message),
            });
        }

        let event_number = (event_byte & 0x7F) >> 4;
        let channel = event_byte & 0x0F;

        let event = match event_number {
            0x1 => {
                let key = data.next()?;
                let vel = data.next()?;
                let duration = Self::parse_midi_length(data);
                XmiEventKind::Midi {
                    channel,
                    message: XmiMidiMessage::NoteOn { key, vel, duration },
                }
            }
            0x2 => {
                let key = data.next()?;
                let vel = data.next()?;
                XmiEventKind::Midi {
                    channel,
                    message: XmiMidiMessage::Aftertouch { key, vel },
                }
            }
            0x3 => {
                let controller = data.next()?;
                let value = data.next()?;
                XmiEventKind::Midi {
                    channel,
                    message: XmiMidiMessage::Controller { controller, value },
                }
            }
            0x4 => {
                let program = data.next()?;
                XmiEventKind::Midi {
                    channel,
                    message: XmiMidiMessage::ProgramChange { program },
                }
            }
            0x5 => {
                let vel = data.next()?;
                XmiEventKind::Midi {
                    channel,
                    message: XmiMidiMessage::ChannelAftertouch { vel },
                }
            }
            0x6 => {
                let lsb = data.next()?;
                let msb = data.next()?;
                XmiEventKind::Midi {
                    channel,
                    message: XmiMidiMessage::PitchBend {
                        bend: PitchBend((msb as u16) << 7 | lsb as u16),
                    },
                }
            }
            0x7 => {
                let length = Self::parse_midi_length(data);
                let mut sysex = Vec::with_capacity(length as usize);
                for _ in 0..length {
                    sysex.push(data.next()?);
                }
                XmiEventKind::SysEx(sysex)
            }
            _ => return None,
        };

        Some(XmiEvent { delta, kind: event })
    }

    fn parse_midi_length<T: Iterator<Item = u8>>(data: &mut Peekable<T>) -> u32 {
        let mut length: u32 = 0;
        for _ in 0..3 {
            let byte = data.next().unwrap();
            length |= (byte & 0x7F) as u32;
            if byte & 0x80 == 0 {
                break;
            }
            length <<= 7;
        }
        length
    }

    pub fn to_smf_file(&self) -> Vec<u8> {
        let mut file = BufWriter::new(Vec::new());

        // signature
        file.write_all(b"MThd").unwrap();
        // header length
        file.write_all(&6u32.to_be_bytes()).unwrap();
        // format
        file.write_all(&0u16.to_be_bytes()).unwrap();
        // number of tracks
        file.write_all(&1u16.to_be_bytes()).unwrap();
        // time division
        file.write_all(&60u16.to_be_bytes()).unwrap();

        // track
        file.write_all(b"MTrk").unwrap();

        let track_events_data = self.build_track_events_data();
        let track_length = track_events_data.len() as u32;

        // track length
        file.write_all(&track_length.to_be_bytes()).unwrap();

        // track events
        file.write_all(&track_events_data).unwrap();

        file.into_inner().unwrap()
    }

    fn build_track_events_data(&self) -> Vec<u8> {
        let mut data = Vec::new();

        for sequence in &self.sequences {
            let mut active_notes: Vec<(u8, u32, u8)> = Vec::new();
            for event in &sequence.events {
                let mut delta = event.delta;

                let mut active_notes_to_remove = Vec::new();
                let mut time_added_by_note_off = 0;
                active_notes.sort_by(|a, b| a.1.cmp(&b.1));
                for (key, duration, channel) in active_notes.iter_mut() {
                    *duration -= time_added_by_note_off;
                    if *duration > delta {
                        *duration -= delta;
                    } else {
                        // Add note off event
                        data.extend(Self::make_midi_length(*duration));
                        data.push(0x80u8 | *channel);
                        data.push(*key);
                        data.push(0);

                        // Remove note from active notes
                        active_notes_to_remove.push((*key, *channel));

                        time_added_by_note_off += *duration;
                        delta -= *duration;
                    }
                }

                for (key, channel) in active_notes_to_remove {
                    active_notes.retain(|(k, _, c)| *k != key || *c != channel);
                }

                if let XmiEventKind::Meta(MetaMessage::Tempo(_)) = &event.kind {
                    continue;
                }

                data.extend(Self::make_midi_length(delta));

                match &event.kind {
                    XmiEventKind::Midi { channel, message } => match message {
                        XmiMidiMessage::NoteOn { key, vel, duration } => {
                            data.push(0x90 | channel);
                            data.push(*key);
                            data.push(*vel);
                            active_notes.push((*key, *duration, *channel));
                        }
                        XmiMidiMessage::Aftertouch { key, vel } => {
                            data.push(0xA0 | channel);
                            data.push(*key);
                            data.push(*vel);
                        }
                        XmiMidiMessage::Controller { controller, value } => {
                            data.push(0xB0 | channel);
                            data.push(*controller);
                            data.push(*value);
                        }
                        XmiMidiMessage::ProgramChange { program } => {
                            data.push(0xC0 | channel);
                            data.push(*program);
                        }
                        XmiMidiMessage::ChannelAftertouch { vel } => {
                            data.push(0xD0 | channel);
                            data.push(*vel);
                        }
                        XmiMidiMessage::PitchBend { bend } => {
                            data.push(0xE0 | channel);
                            data.push(bend.0 as u8);
                            data.push((bend.0 >> 7) as u8);
                        }
                    },
                    XmiEventKind::SysEx(sysex) => {
                        data.push(0xF0);
                        data.extend(Self::make_midi_length(sysex.len() as u32));
                        data.extend(sysex);
                    }
                    XmiEventKind::Meta(meta) => {
                        data.push(0xFF);
                        match meta {
                            MetaMessage::TrackNumber(track) => {
                                data.push(0x00);
                                data.extend(Self::make_midi_length(2));
                                if let Some(track) = track {
                                    data.extend(track.to_be_bytes());
                                }
                            }
                            MetaMessage::Text(text) => {
                                data.push(0x01);
                                data.extend(Self::make_midi_length(text.len() as u32));
                                data.extend(text);
                            }
                            MetaMessage::Copyright(text) => {
                                data.push(0x02);
                                data.extend(Self::make_midi_length(text.len() as u32));
                                data.extend(text);
                            }
                            MetaMessage::TrackName(text) => {
                                data.push(0x03);
                                data.extend(Self::make_midi_length(text.len() as u32));
                                data.extend(text);
                            }
                            MetaMessage::InstrumentName(text) => {
                                data.push(0x04);
                                data.extend(Self::make_midi_length(text.len() as u32));
                                data.extend(text);
                            }
                            MetaMessage::Lyric(text) => {
                                data.push(0x05);
                                data.extend(Self::make_midi_length(text.len() as u32));
                                data.extend(text);
                            }
                            MetaMessage::Marker(text) => {
                                data.push(0x06);
                                data.extend(Self::make_midi_length(text.len() as u32));
                                data.extend(text);
                            }
                            MetaMessage::CuePoint(text) => {
                                data.push(0x07);
                                data.extend(Self::make_midi_length(text.len() as u32));
                                data.extend(text);
                            }
                            MetaMessage::ProgramName(text) => {
                                data.push(0x08);
                                data.extend(Self::make_midi_length(text.len() as u32));
                                data.extend(text);
                            }
                            MetaMessage::DeviceName(text) => {
                                data.push(0x09);
                                data.extend(Self::make_midi_length(text.len() as u32));
                                data.extend(text);
                            }
                            MetaMessage::MidiChannel(channel) => {
                                data.push(0x20);
                                data.extend(Self::make_midi_length(1));
                                data.push(*channel);
                            }
                            MetaMessage::MidiPort(port) => {
                                data.push(0x21);
                                data.extend(Self::make_midi_length(1));
                                data.push(*port);
                            }
                            MetaMessage::EndOfTrack => {
                                data.push(0x2F);
                                data.extend(Self::make_midi_length(0));
                            }
                            MetaMessage::Tempo(_tempo) => {
                                // skip tempo
                            }
                            MetaMessage::TimeSignature(numerator, denominator, clocks, notes) => {
                                data.push(0x58);
                                data.extend(Self::make_midi_length(4));
                                data.push(*numerator);
                                data.push(*denominator);
                                data.push(*clocks);
                                data.push(*notes);
                            }
                            MetaMessage::KeySignature(key, minor) => {
                                data.push(0x59);
                                data.extend(Self::make_midi_length(2));
                                data.push(*key as u8);
                                data.push(if *minor { 1 } else { 0 });
                            }
                            MetaMessage::SequencerSpecific(meta_data) => {
                                data.push(0x7F);
                                data.extend(Self::make_midi_length(meta_data.len() as u32));
                                data.extend(meta_data);
                            }
                            MetaMessage::Unknown(meta_type, meta_data) => {
                                data.push(*meta_type);
                                data.extend(Self::make_midi_length(meta_data.len() as u32));
                                data.extend(meta_data);
                            }
                        }
                    }
                }
            }
        }

        data
    }

    fn make_midi_length(length: u32) -> Vec<u8> {
        let mut bytes = Vec::new();
        let mut length = length;
        loop {
            let byte = (length & 0x7F) as u8;
            length >>= 7;
            if length == 0 {
                bytes.push(byte);
                break;
            }
            bytes.push(byte);
        }
        bytes.iter_mut().skip(1).for_each(|byte| *byte |= 0x80);
        bytes.reverse();
        bytes
    }
}

#[derive(Debug)]
struct XmiSequence {
    patches: Vec<XmiPatch>,
    events: Vec<XmiEvent>,
}

#[derive(Debug)]
struct XmiPatch {
    _patch: u8,
    _bank: u8,
}

#[derive(Debug)]
struct XmiEvent {
    delta: u32,
    kind: XmiEventKind,
}

#[derive(Debug)]
enum XmiEventKind {
    Midi {
        channel: u8,
        message: XmiMidiMessage,
    },
    SysEx(Vec<u8>),
    Meta(MetaMessage),
}

#[derive(Debug)]
enum XmiMidiMessage {
    NoteOn { key: u8, vel: u8, duration: u32 },
    Aftertouch { key: u8, vel: u8 },
    Controller { controller: u8, value: u8 },
    ProgramChange { program: u8 },
    ChannelAftertouch { vel: u8 },
    PitchBend { bend: PitchBend },
}

#[derive(Debug)]
struct PitchBend(u16);

#[derive(Debug)]
enum MetaMessage {
    TrackNumber(Option<u16>),
    Text(Vec<u8>),
    Copyright(Vec<u8>),
    TrackName(Vec<u8>),
    InstrumentName(Vec<u8>),
    Lyric(Vec<u8>),
    Marker(Vec<u8>),
    CuePoint(Vec<u8>),
    ProgramName(Vec<u8>),
    DeviceName(Vec<u8>),
    MidiChannel(u8),
    MidiPort(u8),
    EndOfTrack,
    Tempo(u32),
    TimeSignature(u8, u8, u8, u8),
    KeySignature(i8, bool),
    SequencerSpecific(Vec<u8>),
    Unknown(u8, Vec<u8>),
}

#[cfg(test)]
mod tests {
    use std::io::BufReader;

    use super::*;

    #[test]
    fn test_parse_xmi() {
        // TODO: Get the test data from the DATABASE.MW2 file.
        const SONG_FILENAMES: [&str; 8] = [
            "data/dumped_42.xmi",
            "data/dumped_43.xmi",
            "data/dumped_44.xmi",
            "data/dumped_45.xmi",
            "data/dumped_46.xmi",
            "data/dumped_47.xmi",
            "data/dumped_48.xmi",
            "data/dumped_49.xmi",
        ];

        for path in SONG_FILENAMES.iter() {
            let file = std::fs::File::open(path).unwrap();
            let xmi = XmiFile::new(BufReader::new(file)).unwrap();
            let smf = xmi.to_smf_file();

            let filename = path.split('/').last().unwrap();

            std::fs::write(format!("{filename}.mid"), &smf).unwrap();
        }
    }
}
