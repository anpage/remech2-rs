//! AI-generated tests for ripping audio tracks to FLAC

use std::fs;
use std::io::Cursor;
use std::path::{Path, PathBuf};

use cd_da_reader::{AudioSectorReader, CdReaderError};
use gamedisc::audio::{Audio, AudioSink, FlacSink, rip_track, toc_from_tracks};
use gamedisc::bincue::BinCueSource;
use gamedisc::{Error, SectorSource, Track, TrackKind, open_image_audio};

const RAW: usize = 2352;

fn track(number: u8, kind: TrackKind, start_lba: u32, sectors: u32) -> Track {
    Track {
        number,
        kind,
        start_lba,
        sectors,
    }
}

/// Distinct bytes for every sector, so misplaced reads show up
fn pcm(first_lba: u32, count: u32) -> Vec<u8> {
    (first_lba..first_lba + count)
        .flat_map(|lba| (0..RAW).map(move |i| (lba as usize * 7 + i) as u8))
        .collect()
}

/// Interleaved 16-bit stereo: a 440 Hz sine on the left, 660 Hz on the right, with both extremes
fn sine(sectors: u32) -> Vec<i16> {
    let frames = sectors as usize * RAW / 4;
    let mut samples: Vec<i16> = (0..frames)
        .flat_map(|n| {
            let t = n as f64 / 44_100.0;
            let left = (t * 440.0 * std::f64::consts::TAU).sin() * 20_000.0;
            let right = (t * 660.0 * std::f64::consts::TAU).sin() * 20_000.0;
            [left as i16, right as i16]
        })
        .collect();
    samples[0] = i16::MIN;
    samples[1] = i16::MAX;
    samples
}

fn to_bytes(samples: &[i16]) -> Vec<u8> {
    samples.iter().flat_map(|s| s.to_le_bytes()).collect()
}

/// An in-memory disc: a data track then two audio tracks
struct MemSource {
    tracks: Vec<Track>,
    raw: Vec<u8>,
    fail_at: Option<u32>,
}

impl MemSource {
    /// Data at 0..10, track 2 at 10..210, track 3 at 210..260
    fn disc() -> Self {
        Self {
            tracks: vec![
                track(1, TrackKind::Mode1, 0, 10),
                track(2, TrackKind::Audio, 10, 200),
                track(3, TrackKind::Audio, 210, 50),
            ],
            raw: pcm(0, 260),
            fail_at: None,
        }
    }
}

impl SectorSource for MemSource {
    fn tracks(&self) -> &[Track] {
        &self.tracks
    }

    fn read_raw(&self, lba: u32, count: u32, out: &mut [u8]) -> Result<(), Error> {
        let start = lba as usize * RAW;
        let end = start + count as usize * RAW;
        let fails = self.fail_at.is_some_and(|f| lba <= f && f < lba + count);
        if end > self.raw.len() || fails {
            return Err(Error::OutOfRange { lba, count });
        }
        out.copy_from_slice(&self.raw[start..end]);
        Ok(())
    }
}

/// Records what the ripper hands it
#[derive(Default)]
struct VecSink {
    pcm: Vec<u8>,
    writes: usize,
    finished: usize,
    written_after_finish: bool,
}

impl AudioSink for VecSink {
    fn write_pcm(&mut self, pcm: &[u8]) -> Result<(), Error> {
        self.written_after_finish |= self.finished > 0;
        self.pcm.extend_from_slice(pcm);
        self.writes += 1;
        Ok(())
    }

    fn finish(&mut self) -> Result<(), Error> {
        self.finished += 1;
        Ok(())
    }
}

/// Rips into a VecSink and returns it with every progress report
fn rip<R: AudioSectorReader>(
    reader: &R,
    tracks: &[Track],
    number: u8,
) -> Result<(VecSink, Vec<(u32, u32)>), Error> {
    let toc = toc_from_tracks(tracks);
    let mut sink = VecSink::default();
    let mut progress = vec![];
    rip_track(reader, &toc, number, &mut sink, &mut |done, total| {
        progress.push((done, total))
    })?;
    Ok((sink, progress))
}

fn scratch(name: &str) -> PathBuf {
    let dir = Path::new(env!("CARGO_TARGET_TMPDIR"))
        .join("audio")
        .join(name);
    let _ = fs::remove_dir_all(&dir);
    fs::create_dir_all(&dir).unwrap();
    dir
}

/// Decodes a FLAC file, checking it is CD audio
fn decode_flac(bytes: &[u8]) -> Vec<i16> {
    let mut reader = claxon::FlacReader::new(Cursor::new(bytes)).expect("not a FLAC stream");
    let info = reader.streaminfo();
    assert_eq!(
        (info.sample_rate, info.channels, info.bits_per_sample),
        (44_100, 2, 16)
    );
    let samples: Vec<i16> = reader
        .samples()
        .map(|s| i16::try_from(s.unwrap()).expect("sample out of 16-bit range"))
        .collect();
    assert_eq!(
        info.samples,
        Some(samples.len() as u64 / 2),
        "STREAMINFO sample count"
    );
    samples
}

// Table of contents

#[test]
fn toc_mirrors_the_tracks() {
    let source = MemSource::disc();
    let toc = toc_from_tracks(source.tracks());

    assert_eq!(
        (toc.first_track, toc.last_track, toc.leadout_lba),
        (1, 3, 260)
    );
    let tracks: Vec<_> = toc
        .tracks
        .iter()
        .map(|t| (t.number, t.start_lba, t.start_msf, t.is_audio))
        .collect();
    // MSF includes the two-second lead-in: LBA 210 + 150 = 360 = 4 s 60 f
    assert_eq!(
        tracks,
        [
            (1, 0, (0, 2, 0), false),
            (2, 10, (0, 2, 10), true),
            (3, 210, (0, 4, 60), true),
        ]
    );
}

#[test]
fn toc_keeps_the_first_track_number() {
    let toc = toc_from_tracks(&[
        track(5, TrackKind::Audio, 0, 100),
        track(6, TrackKind::Audio, 100, 50),
    ]);

    assert_eq!(
        (toc.first_track, toc.last_track, toc.leadout_lba),
        (5, 6, 150)
    );
}

// Audio wrapper

#[test]
fn audio_reads_raw_sectors() {
    let source = MemSource::disc();
    let audio = Audio(&source);

    assert_eq!(audio.read_audio_sectors(12, 3).unwrap(), pcm(12, 3));
    assert!(matches!(
        audio.read_audio_sectors(259, 2),
        Err(Error::OutOfRange { lba: 259, count: 2 })
    ));
}

// rip_track

#[test]
fn track_runs_to_the_next_track() {
    let source = MemSource::disc();
    let (sink, _) = rip(&Audio(&source), source.tracks(), 2).unwrap();

    assert_eq!(sink.pcm, pcm(10, 200));
}

#[test]
fn last_track_runs_to_the_leadout() {
    let source = MemSource::disc();
    let (sink, _) = rip(&Audio(&source), source.tracks(), 3).unwrap();

    assert_eq!(sink.pcm, pcm(210, 50));
}

#[test]
fn reads_one_second_at_a_time_and_reports_progress() {
    let source = MemSource::disc();
    let (sink, progress) = rip(&Audio(&source), source.tracks(), 2).unwrap();

    assert_eq!(sink.writes, 3);
    assert_eq!(progress, [(75, 200), (150, 200), (200, 200)]);
}

#[test]
fn sink_is_finished_once_after_the_last_write() {
    let source = MemSource::disc();
    let (sink, _) = rip(&Audio(&source), source.tracks(), 2).unwrap();

    assert_eq!(sink.finished, 1);
    assert!(!sink.written_after_finish);
}

#[test]
fn data_track_is_refused() {
    let source = MemSource::disc();
    let toc = toc_from_tracks(source.tracks());
    let mut sink = VecSink::default();

    let result = rip_track(&Audio(&source), &toc, 1, &mut sink, &mut |_, _| {});
    assert!(
        matches!(result, Err(Error::NotAudioTrack { track: 1 })),
        "{result:?}"
    );
    assert_eq!((sink.writes, sink.finished), (0, 0));
}

#[test]
fn missing_track() {
    let source = MemSource::disc();
    let result = rip(&Audio(&source), source.tracks(), 9);

    assert!(matches!(result, Err(Error::Drive(_))));
}

#[test]
fn source_errors_stop_the_rip_unfinished() {
    let mut source = MemSource::disc();
    source.fail_at = Some(100);
    let toc = toc_from_tracks(source.tracks());
    let mut sink = VecSink::default();

    let result = rip_track(&Audio(&source), &toc, 2, &mut sink, &mut |_, _| {});
    // cd-da-reader wraps our error as its backend error
    assert!(
        matches!(result, Err(Error::Drive(CdReaderError::Backend(_)))),
        "{result:?}"
    );
    assert_eq!(sink.finished, 0);
}

// FLAC

#[test]
fn flac_round_trips_sample_exact() {
    let samples = sine(160);
    let mut flac = FlacSink::new(vec![]);
    for chunk in to_bytes(&samples).chunks(75 * RAW) {
        flac.write_pcm(chunk).unwrap();
    }
    flac.finish().unwrap();
    let bytes = flac.into_inner();

    assert_eq!(&bytes[..4], b"fLaC");
    assert!(
        bytes.len() < samples.len() * 2,
        "FLAC should be smaller than the PCM"
    );
    assert_eq!(decode_flac(&bytes), samples);
}

#[test]
fn flac_of_a_partial_block() {
    // Fewer samples than one FLAC block
    let samples = sine(1);
    let mut flac = FlacSink::new(vec![]);
    flac.write_pcm(&to_bytes(&samples)).unwrap();
    flac.finish().unwrap();

    assert_eq!(decode_flac(&flac.into_inner()), samples);
}

// From a disc image

fn write_disc(dir: &Path, music: &[u8]) -> PathBuf {
    let data = vec![0; 10 * RAW];
    fs::write(dir.join("game.bin"), [&data[..], music].concat()).unwrap();
    let cue = dir.join("game.cue");
    fs::write(
        &cue,
        "FILE \"game.bin\" BINARY\n  TRACK 01 MODE1/2352\n    INDEX 01 00:00:00\n  TRACK 02 AUDIO\n    INDEX 01 00:00:10\n",
    )
    .unwrap();
    cue
}

#[test]
fn bin_cue_to_flac() {
    let dir = scratch("bin_cue_to_flac");
    let samples = sine(160);
    let source = BinCueSource::open(write_disc(&dir, &to_bytes(&samples))).unwrap();
    let toc = toc_from_tracks(source.tracks());

    let mut flac = FlacSink::new(vec![]);
    rip_track(&Audio(&source), &toc, 2, &mut flac, &mut |_, _| {}).unwrap();

    assert_eq!(decode_flac(&flac.into_inner()), samples);
}

// open_image_audio

#[test]
fn audio_sources_can_move_between_threads() {
    fn assert_send<T: Send>() {}
    assert_send::<Box<dyn SectorSource + Send>>();
}

#[test]
fn open_image_audio_reads_a_cue() {
    let dir = scratch("open_image_audio_reads_a_cue");
    let samples = sine(160);
    let cue = write_disc(&dir, &to_bytes(&samples));
    let upper = dir.join("GAME.CUE");
    fs::rename(&cue, &upper).unwrap();

    let source = open_image_audio(&upper)
        .unwrap_or_else(|e| panic!("{e:?}"))
        .expect("a .cue can carry audio");
    assert_eq!(
        source.tracks(),
        [
            track(1, TrackKind::Mode1, 0, 10),
            track(2, TrackKind::Audio, 10, 160),
        ]
    );

    let toc = toc_from_tracks(source.tracks());
    let mut flac = FlacSink::new(vec![]);
    rip_track(&Audio(&*source), &toc, 2, &mut flac, &mut |_, _| {}).unwrap();
    assert_eq!(decode_flac(&flac.into_inner()), samples);
}

#[test]
fn iso_images_have_no_audio() {
    let dir = scratch("iso_images_have_no_audio");
    for name in ["game.iso", "GAME.ISO"] {
        fs::write(dir.join(name), vec![0; 40 * 2048]).unwrap();

        let result = open_image_audio(dir.join(name));
        assert!(matches!(result, Ok(None)), "{name}");
    }
}

#[test]
fn open_image_audio_rejects_other_extensions() {
    let dir = scratch("open_image_audio_rejects_other_extensions");
    for name in ["game.bin", "game.mds", "game"] {
        let path = dir.join(name);
        fs::write(&path, pcm(0, 10)).unwrap();

        let result = open_image_audio(&path);
        assert!(
            matches!(result, Err(Error::UnsupportedImage(ref p)) if *p == path),
            "{name}"
        );
    }
}

#[test]
fn open_image_audio_missing_cue() {
    let dir = scratch("open_image_audio_missing_cue");

    let result = open_image_audio(dir.join("missing.cue"));
    assert!(matches!(result, Err(Error::Io(_))));
}
