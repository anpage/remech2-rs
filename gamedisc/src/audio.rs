use std::io::Write;

use cd_da_reader::{
    AudioSectorReader, Toc, TrackBounds, lba_to_msf, open_track_stream_with_bounds,
};
use flacenc::{component::BitRepr, error::Verify};

use crate::{Error, RAW_SECTOR, SectorSource, Track, TrackKind};

/// One second of audio
const CHUNK_SECTORS: u32 = 75;

/// Wrapper to allow cd-da-reader to read audio from SectorSource
pub struct Audio<S>(pub S);

impl<S: SectorSource> AudioSectorReader for Audio<S> {
    type Error = Error;
    fn read_audio_sectors(&self, start_lba: u32, count: u32) -> Result<Vec<u8>, Error> {
        let mut pcm = vec![0; count as usize * RAW_SECTOR];
        self.0.read_raw(start_lba, count, &mut pcm)?;
        Ok(pcm)
    }
}

/// cd-da-reader's table of contents for these tracks
pub fn toc_from_tracks(tracks: &[Track]) -> Toc {
    Toc {
        first_track: tracks.first().map_or(1, |t| t.number),
        last_track: tracks.last().map_or(1, |t| t.number),
        leadout_lba: tracks.last().map_or(0, |t| t.start_lba + t.sectors),
        tracks: tracks
            .iter()
            .map(|t| cd_da_reader::Track {
                number: t.number,
                start_lba: t.start_lba,
                start_msf: lba_to_msf(t.start_lba),
                is_audio: t.kind == TrackKind::Audio,
            })
            .collect(),
    }
}

/// 16-bit little-endian stereo at 44.1 kHz
pub trait AudioSink {
    fn write_pcm(&mut self, pcm: &[u8]) -> Result<(), Error>;
    fn finish(&mut self) -> Result<(), Error>;
}

pub struct FlacSink<W> {
    out: W,
    /// Interleaved left/right samples
    samples: Vec<i32>,
}

impl<W: Write> FlacSink<W> {
    pub fn new(out: W) -> Self {
        Self {
            out,
            samples: vec![],
        }
    }

    pub fn into_inner(self) -> W {
        self.out
    }
}

fn encode_flac(samples: &[i32]) -> Result<Vec<u8>, Error> {
    let config = flacenc::config::Encoder::default()
        .into_verified()
        .map_err(|e| Error::Flac(format!("{e:?}")))?;

    let source = flacenc::source::MemSource::from_samples(samples, 2, 16, 44_100);

    let mut stream = flacenc::encode_with_fixed_block_size(&config, source, config.block_size)
        .map_err(|e| Error::Flac(format!("{e:?}")))?;

    let info = stream.stream_info_mut();

    info.set_block_sizes(info.max_block_size(), info.max_block_size())
        .map_err(|e| Error::Flac(format!("{e:?}")))?;

    let mut sink = flacenc::bitsink::ByteSink::new();

    stream
        .write(&mut sink)
        .map_err(|e| Error::Flac(format!("{e:?}")))?;

    Ok(sink.as_slice().to_vec())
}

impl<W: Write> AudioSink for FlacSink<W> {
    fn write_pcm(&mut self, pcm: &[u8]) -> Result<(), Error> {
        self.samples.extend(
            pcm.as_chunks::<2>()
                .0
                .iter()
                .map(|b| i32::from(i16::from_le_bytes(*b))),
        );
        Ok(())
    }

    fn finish(&mut self) -> Result<(), Error> {
        let samples = std::mem::take(&mut self.samples);
        self.out.write_all(&encode_flac(&samples)?)?;
        Ok(())
    }
}

pub fn rip_track<R: AudioSectorReader>(
    reader: &R,
    toc: &Toc,
    track: u8,
    sink: &mut dyn AudioSink,
    progress: &mut dyn FnMut(u32, u32),
) -> Result<(), Error> {
    if toc.tracks.iter().any(|t| t.number == track && !t.is_audio) {
        return Err(Error::NotAudioTrack { track });
    }

    let mut stream = open_track_stream_with_bounds(reader, toc, track, TrackBounds::Gapless)?
        .with_sectors_per_chunk(CHUNK_SECTORS);
    let total = stream.total_sectors();

    while let Some(pcm) = stream.next_chunk()? {
        sink.write_pcm(&pcm)?;
        progress(stream.current_sector(), total);
    }

    sink.finish()
}
