//! AI-generated tests for laying a cue sheet's files out in one disc-wide LBA space

use gamedisc::cue::{CueError, CueMode, CueSheet, FileSpan, Layout};
use gamedisc::{Track, TrackKind};

const SINGLE: &str = r#"FILE "MECH2.BIN" BINARY
  TRACK 01 MODE1/2352
    INDEX 01 00:00:00
  TRACK 02 AUDIO
    INDEX 00 44:51:60
    INDEX 01 44:53:60
  TRACK 03 AUDIO
    INDEX 01 48:32:17
"#;

const SPLIT: &str = r#"FILE "Game (Track 1).bin" BINARY
  TRACK 01 MODE1/2352
    INDEX 01 00:00:00
FILE "Game (Track 2).bin" BINARY
  TRACK 02 AUDIO
    INDEX 00 00:00:00
    INDEX 01 00:02:00
FILE "Game (Track 3).bin" BINARY
  TRACK 03 AUDIO
    INDEX 00 00:00:00
    INDEX 01 00:02:00
"#;

fn layout(text: &str, file_sectors: &[u32]) -> Layout {
    let sheet = CueSheet::parse(text).unwrap_or_else(|e| panic!("parse failed: {e}\n{text}"));
    sheet
        .layout(file_sectors)
        .unwrap_or_else(|e| panic!("layout failed: {e}\n{text}"))
}

fn layout_err(text: &str, file_sectors: &[u32]) -> CueError {
    let sheet = CueSheet::parse(text).unwrap_or_else(|e| panic!("parse failed: {e}\n{text}"));
    match sheet.layout(file_sectors) {
        Err(e) => e,
        Ok(layout) => panic!("expected an error for {file_sectors:?}, got {layout:?}"),
    }
}

fn track(number: u8, kind: TrackKind, start_lba: u32, sectors: u32) -> Track {
    Track {
        number,
        kind,
        start_lba,
        sectors,
    }
}

fn span(start_lba: u32, sectors: u32) -> FileSpan {
    FileSpan { start_lba, sectors }
}

#[test]
fn sector_sizes() {
    assert_eq!(CueMode::Audio.sector_size(), 2352);
    assert_eq!(CueMode::Mode1Raw.sector_size(), 2352);
    assert_eq!(CueMode::Mode1Cooked.sector_size(), 2048);
    assert_eq!(CueMode::Mode2Raw.sector_size(), 2352);
}

#[test]
fn single_file_mixed_mode() {
    // 44:53:60 = (44 * 60 + 53) * 75 + 60 = 202035
    // 48:32:17 = (48 * 60 + 32) * 75 + 17 = 218417
    // Track 2's pregap from 44:51:60 stays inside track 1
    let layout = layout(SINGLE, &[230_000]);

    assert_eq!(
        layout.tracks,
        [
            track(1, TrackKind::Mode1, 0, 202_035),
            track(2, TrackKind::Audio, 202_035, 218_417 - 202_035),
            track(3, TrackKind::Audio, 218_417, 230_000 - 218_417),
        ]
    );
    assert_eq!(layout.files, [span(0, 230_000)]);
    assert_eq!(layout.leadout_lba, 230_000);
}

#[test]
fn split_files_are_laid_end_to_end() {
    // Each later file opens with its track's 150-sector pregap, which belongs to the previous track
    let layout = layout(SPLIT, &[1000, 650, 550]);

    assert_eq!(
        layout.tracks,
        [
            track(1, TrackKind::Mode1, 0, 1150),
            track(2, TrackKind::Audio, 1150, 650),
            track(3, TrackKind::Audio, 1800, 400),
        ]
    );
    assert_eq!(
        layout.files,
        [span(0, 1000), span(1000, 650), span(1650, 550)]
    );
    assert_eq!(layout.leadout_lba, 2200);
}

#[test]
fn several_tracks_in_later_files() {
    let layout = layout(
        r#"FILE "data.bin" BINARY
  TRACK 01 MODE1/2352
    INDEX 01 00:00:00
FILE "audio.bin" BINARY
  TRACK 02 AUDIO
    INDEX 01 00:00:00
  TRACK 03 AUDIO
    INDEX 00 00:10:00
    INDEX 01 00:12:00
"#,
        &[500, 2000],
    );

    assert_eq!(
        layout.tracks,
        [
            track(1, TrackKind::Mode1, 0, 500),
            track(2, TrackKind::Audio, 500, 900),
            track(3, TrackKind::Audio, 1400, 1100),
        ]
    );
    assert_eq!(layout.files, [span(0, 500), span(500, 2000)]);
    assert_eq!(layout.leadout_lba, 2500);
}

#[test]
fn carried_over_tracks_start_at_their_new_file() {
    let layout = layout(
        r#"FILE "01.wav" WAVE
  TRACK 01 AUDIO
    INDEX 01 00:00:00
  TRACK 02 AUDIO
    INDEX 00 03:58:20
FILE "02.wav" WAVE
    INDEX 01 00:00:00
  TRACK 03 AUDIO
    INDEX 00 04:10:00
FILE "03.wav" WAVE
    INDEX 01 00:00:00
"#,
        &[17_000, 18_000, 19_000],
    );

    assert_eq!(
        layout.tracks,
        [
            track(1, TrackKind::Audio, 0, 17_000),
            track(2, TrackKind::Audio, 17_000, 18_000),
            track(3, TrackKind::Audio, 35_000, 19_000),
        ]
    );
    assert_eq!(layout.leadout_lba, 54_000);
}

#[test]
fn track_kinds() {
    let layout = layout(
        r#"FILE "a.iso" BINARY
  TRACK 01 MODE1/2048
    INDEX 01 00:00:00
FILE "b.bin" BINARY
  TRACK 02 MODE2/2352
    INDEX 01 00:00:00
FILE "c.bin" BINARY
  TRACK 03 AUDIO
    INDEX 01 00:00:00
"#,
        &[10, 20, 30],
    );

    assert_eq!(
        layout.tracks,
        [
            track(1, TrackKind::Mode1, 0, 10),
            track(2, TrackKind::Mode2Form1, 10, 20),
            track(3, TrackKind::Audio, 30, 30),
        ]
    );
}

#[test]
fn first_track_number_is_kept() {
    let layout = layout(
        "FILE \"a.bin\" BINARY\n  TRACK 05 AUDIO\n    INDEX 01 00:00:00\n",
        &[100],
    );

    assert_eq!(layout.tracks, [track(5, TrackKind::Audio, 0, 100)]);
}

#[test]
fn track_must_start_inside_its_file() {
    // INDEX 01 on the file's last sector leaves a one-sector track
    assert_eq!(layout(SINGLE, &[218_418]).tracks[2].sectors, 1);

    // INDEX 01 at or past the end leaves nothing; the first such track is reported
    assert!(matches!(
        layout_err(SINGLE, &[218_417]),
        CueError::TrackPastEndOfFile { track: 3 }
    ));
    assert!(matches!(
        layout_err(SINGLE, &[202_035]),
        CueError::TrackPastEndOfFile { track: 2 }
    ));
    assert!(matches!(
        layout_err(SINGLE, &[0]),
        CueError::TrackPastEndOfFile { track: 1 }
    ));

    // A split file holding only its pregap
    assert!(matches!(
        layout_err(SPLIT, &[1000, 150, 550]),
        CueError::TrackPastEndOfFile { track: 2 }
    ));
}
