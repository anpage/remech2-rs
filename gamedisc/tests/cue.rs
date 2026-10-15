//! AI-generated tests for the cue sheet parser

use gamedisc::cue::{CueError, CueFile, CueFileKind, CueMode, CueSheet, CueTrack};

/// mm:ss:ff to sectors, for readable expectations
fn msf(m: u32, s: u32, f: u32) -> u32 {
    (m * 60 + s) * 75 + f
}

fn parse(text: &str) -> CueSheet {
    CueSheet::parse(text).unwrap_or_else(|e| panic!("parse failed: {e}\n{text}"))
}

macro_rules! assert_err {
    ($text:expr, $pat:pat $(if $guard:expr)?) => {{
        let text: &str = $text;
        match CueSheet::parse(text) {
            Err(e) => assert!(matches!(e, $pat $(if $guard)?), "got {e:?} for:\n{text}"),
            Ok(sheet) => panic!("expected an error, parsed {sheet:?} from:\n{text}"),
        }
    }};
}

fn track(number: u8, mode: CueMode, index00: Option<u32>, index01: u32) -> CueTrack {
    CueTrack {
        number,
        mode,
        index00,
        index01,
    }
}

fn audio(number: u8, index00: Option<u32>, index01: u32) -> CueTrack {
    track(number, CueMode::Audio, index00, index01)
}

fn file(path: &str, kind: CueFileKind, tracks: Vec<CueTrack>) -> CueFile {
    CueFile {
        path: path.to_owned(),
        kind,
        tracks,
    }
}

/// A FILE line followed by one valid audio track
fn one_track(file_line: &str) -> String {
    format!("{file_line}\n  TRACK 01 AUDIO\n    INDEX 01 00:00:00\n")
}

/// The kind parsed from a FILE line
fn kind_of(file_line: &str) -> CueFileKind {
    parse(&one_track(file_line)).files[0].kind
}

// Layouts

#[test]
fn single_file_mixed_mode() {
    let sheet = parse(
        r#"FILE "MECH2.BIN" BINARY
  TRACK 01 MODE1/2352
    INDEX 01 00:00:00
  TRACK 02 AUDIO
    INDEX 00 44:51:60
    INDEX 01 44:53:60
  TRACK 03 AUDIO
    INDEX 01 48:32:17
"#,
    );

    let tracks = vec![
        track(1, CueMode::Mode1Raw, None, 0),
        audio(2, Some(msf(44, 51, 60)), msf(44, 53, 60)),
        audio(3, None, msf(48, 32, 17)),
    ];
    assert_eq!(
        sheet.files,
        [file("MECH2.BIN", CueFileKind::Binary, tracks)]
    );
}

#[test]
fn split_layout_one_file_per_track() {
    // Every file restarts at 00:00:00, so ordering is only checked within a file
    let sheet = parse(
        r#"FILE "Game (Track 1).bin" BINARY
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
"#,
    );

    let binary = CueFileKind::Binary;
    assert_eq!(
        sheet.files,
        [
            file(
                "Game (Track 1).bin",
                binary,
                vec![track(1, CueMode::Mode1Raw, None, 0)]
            ),
            file("Game (Track 2).bin", binary, vec![audio(2, Some(0), 150)]),
            file("Game (Track 3).bin", binary, vec![audio(3, Some(0), 150)]),
        ]
    );
}

// Carry-over (EAC "gaps appended")

#[test]
fn eac_gaps_appended_carries_track_into_next_file() {
    let sheet = parse(
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
    );

    let wave = CueFileKind::Wave;
    assert_eq!(
        sheet.files,
        [
            file("01.wav", wave, vec![audio(1, None, 0)]),
            file("02.wav", wave, vec![audio(2, None, 0)]),
            file("03.wav", wave, vec![audio(3, None, 0)]),
        ]
    );
}

#[test]
fn hidden_track_pregap_file_is_dropped() {
    // The first file holds only track 1's pregap, which belongs to no music file
    let sheet = parse(
        r#"FILE "00 Hidden.wav" WAVE
  TRACK 01 AUDIO
    INDEX 00 00:00:00
FILE "01.wav" WAVE
    INDEX 01 00:00:00
  TRACK 02 AUDIO
    INDEX 00 03:58:20
FILE "02.wav" WAVE
    INDEX 01 00:00:00
"#,
    );

    let wave = CueFileKind::Wave;
    assert_eq!(
        sheet.files,
        [
            file("01.wav", wave, vec![audio(1, None, 0)]),
            file("02.wav", wave, vec![audio(2, None, 0)]),
        ]
    );
}

#[test]
fn carry_over_needs_an_index00() {
    // A track with no indices at all is not a gaps-appended track
    assert_err!(
        r#"FILE "01.wav" WAVE
  TRACK 01 AUDIO
    INDEX 01 00:00:00
  TRACK 02 AUDIO
FILE "02.wav" WAVE
    INDEX 01 00:00:00
"#,
        CueError::MissingIndex01 { line: 4, track: 2 }
    );
}

#[test]
fn carry_over_only_at_file_boundaries() {
    // At a TRACK line
    assert_err!(
        r#"FILE "01.wav" WAVE
  TRACK 01 AUDIO
    INDEX 01 00:00:00
  TRACK 02 AUDIO
    INDEX 00 03:58:20
  TRACK 03 AUDIO
    INDEX 01 04:00:00
"#,
        CueError::MissingIndex01 { line: 4, track: 2 }
    );
    // At the end of the sheet
    assert_err!(
        r#"FILE "01.wav" WAVE
  TRACK 01 AUDIO
    INDEX 01 00:00:00
  TRACK 02 AUDIO
    INDEX 00 03:58:20
"#,
        CueError::MissingIndex01 { line: 4, track: 2 }
    );
}

#[test]
fn file_without_tracks_is_still_an_error_without_carry_over() {
    // Only a file emptied by carry-over may be dropped
    assert_err!(
        r#"FILE "00 Hidden.wav" WAVE
FILE "01.wav" WAVE
  TRACK 01 AUDIO
    INDEX 01 00:00:00
"#,
        CueError::FileMissingTracks { line: 1 }
    );
}

// Leniency

#[test]
fn lowercase_keywords_and_values() {
    let sheet = parse(
        r#"file "game.bin" binary
  track 01 mode1/2352
    index 01 00:00:00
  Track 02 Audio
    Index 01 01:00:00
"#,
    );

    let tracks = vec![
        track(1, CueMode::Mode1Raw, None, 0),
        audio(2, None, msf(1, 0, 0)),
    ];
    assert_eq!(sheet.files, [file("game.bin", CueFileKind::Binary, tracks)]);
}

#[test]
fn crlf_tabs_blank_lines_and_trailing_whitespace() {
    let sheet = parse(
        "FILE \"a.bin\" BINARY\r\n\r\n\t  TRACK 01 AUDIO \r\n\t\tINDEX 01 00:00:00\t\r\n\r\n",
    );

    assert_eq!(
        sheet.files,
        [file("a.bin", CueFileKind::Binary, vec![audio(1, None, 0)])]
    );
}

#[test]
fn line_numbers_count_blank_lines_and_crlf() {
    assert_err!(
        "\r\nFILE a.bin BINARY\r\n\r\n  INDEX 01 00:00:00\r\n",
        CueError::IndexBeforeTrack { line: 4 }
    );
}

#[test]
fn byte_order_mark() {
    let sheet = parse("\u{feff}FILE \"a.bin\" BINARY\n  TRACK 01 AUDIO\n    INDEX 01 00:00:00\n");

    assert_eq!(
        sheet.files,
        [file("a.bin", CueFileKind::Binary, vec![audio(1, None, 0)])]
    );
}

#[test]
fn byte_order_mark_keeps_line_numbers() {
    assert_err!(
        "\u{feff}FILE a.bin BINARY\n  INDEX 01 00:00:00\n",
        CueError::IndexBeforeTrack { line: 2 }
    );
}

#[test]
fn file_names() {
    let cases = [
        ("FILE game.bin BINARY", "game.bin"),
        (
            r#"FILE "My Game (Disc 1).bin" BINARY"#,
            "My Game (Disc 1).bin",
        ),
        // Resolving against the cue's directory is a later step
        (
            r#"FILE "C:\Games\MW2\MECH2.BIN" BINARY"#,
            r"C:\Games\MW2\MECH2.BIN",
        ),
    ];
    for (line, expected) in cases {
        assert_eq!(parse(&one_track(line)).files[0].path, expected, "{line}");
    }
}

#[test]
fn ignored_commands() {
    let sheet = parse(
        r#"REM GENRE Game
REM COMMENT "ExactAudioCopy v1.6"
CATALOG 0000000000000
CDTEXTFILE "disc.cdt"
PERFORMER "Activision"
TITLE "MechWarrior 2"
SONGWRITER "Someone"
FILE "game.bin" BINARY
  TRACK 01 MODE1/2352
    FLAGS DCP
    ISRC USRC17607839
    TITLE "Data"
    INDEX 01 00:00:00
    POSTGAP 00:02:00
  TRACK 02 AUDIO
    PERFORMER "Someone"
    PREGAP 00:02:00
    INDEX 01 10:00:00
"#,
    );

    // PREGAP is not stored in the file and must not become an INDEX 00
    let tracks = [
        track(1, CueMode::Mode1Raw, None, 0),
        audio(2, None, msf(10, 0, 0)),
    ];
    assert_eq!(sheet.files[0].tracks, tracks);
}

#[test]
fn unknown_commands_that_start_with_keywords_are_ignored() {
    let sheet = parse(
        r#"FILENAME something
FILE "game.bin" BINARY
  TRACKS 2
  TRACK 01 AUDIO
    INDEXED yes
    INDEX 01 00:00:00
"#,
    );

    assert_eq!(sheet.files[0].tracks, [audio(1, None, 0)]);
}

#[test]
fn higher_indices_are_ignored() {
    let sheet = parse(
        r#"FILE "game.bin" BINARY
  TRACK 01 AUDIO
    INDEX 01 00:00:00
    INDEX 02 00:30:00
    INDEX 02 00:30:00
    INDEX 03 00:10:00
  TRACK 02 AUDIO
    INDEX 01 01:00:00
"#,
    );

    assert_eq!(
        sheet.files[0].tracks,
        [audio(1, None, 0), audio(2, None, msf(1, 0, 0))]
    );
}

#[test]
fn equal_index01_on_consecutive_tracks_is_allowed() {
    let sheet = parse(
        r#"FILE "game.bin" BINARY
  TRACK 01 AUDIO
    INDEX 01 01:00:00
  TRACK 02 AUDIO
    INDEX 01 01:00:00
"#,
    );

    let start = msf(1, 0, 0);
    assert_eq!(
        sheet.files[0].tracks,
        [audio(1, None, start), audio(2, None, start)]
    );
}

#[test]
fn largest_valid_time() {
    let sheet = parse("FILE a.bin BINARY\n  TRACK 01 AUDIO\n    INDEX 01 99:59:74\n");

    assert_eq!(sheet.files[0].tracks, [audio(1, None, msf(99, 59, 74))]);
}

// FILE types

#[test]
fn kind_from_extension_when_type_is_missing() {
    let cases = [
        ("a.bin", CueFileKind::Binary),
        ("a.BIN", CueFileKind::Binary),
        ("a.img", CueFileKind::Binary),
        ("a.iso", CueFileKind::Binary),
        ("a.wav", CueFileKind::Wave),
        ("a.WAV", CueFileKind::Wave),
        ("a.flac", CueFileKind::Flac),
        ("a.mp3", CueFileKind::Mp3),
        ("a.ogg", CueFileKind::Ogg),
        // The extension comes from the last component, whichever separator is used
        (r"C:\My.Games\MW2\track02.flac", CueFileKind::Flac),
        (r"..\Music.Rips\track02.WAV", CueFileKind::Wave),
        ("sub.dir/track02.ogg", CueFileKind::Ogg),
    ];
    for (name, expected) in cases {
        assert_eq!(kind_of(&format!(r#"FILE "{name}""#)), expected, "{name}");
    }
}

#[test]
fn declared_audio_type_defers_to_extension() {
    let cases = [
        ("WAVE", "a.flac", CueFileKind::Flac),
        ("MP3", "a.ogg", CueFileKind::Ogg),
        ("WAVE", "a.wav", CueFileKind::Wave),
        ("APE", "a.flac", CueFileKind::Flac),
        ("WAVPACK", "a.wav", CueFileKind::Wave),
        ("AIFF", "a.mp3", CueFileKind::Mp3),
    ];
    for (kind, name, expected) in cases {
        assert_eq!(
            kind_of(&format!(r#"FILE "{name}" {kind}"#)),
            expected,
            "{name} {kind}"
        );
    }
}

#[test]
fn declared_binary_wins_over_extension() {
    assert_eq!(kind_of(r#"FILE "a.dat" BINARY"#), CueFileKind::Binary);
}

#[test]
fn motorola_is_rejected() {
    assert_err!(
        &one_track(r#"FILE "a.bin" MOTOROLA"#),
        CueError::UnsupportedFileType { line: 1, ref kind } if kind == "MOTOROLA"
    );
}

#[test]
fn unsupported_extension() {
    for line in [r#"FILE "a.ape""#, r#"FILE "a.ape" WAVE"#] {
        assert_err!(
            &one_track(line),
            CueError::UnsupportedFileType { line: 1, .. }
        );
    }
}

#[test]
fn missing_extension() {
    // Dots in directories don't count, with either separator
    for line in [
        r#"FILE "audio""#,
        r#"FILE "C:\My.Games\audio""#,
        r#"FILE "sub.dir/audio""#,
    ] {
        assert_err!(
            &one_track(line),
            CueError::UnsupportedFileType { line: 1, ref kind } if kind == "(no extension)"
        );
    }
}

// TRACK modes

#[test]
fn supported_modes() {
    let sheet = parse(
        r#"FILE "game.bin" BINARY
  TRACK 01 MODE1/2352
    INDEX 01 00:00:00
  TRACK 02 MODE1/2048
    INDEX 01 01:00:00
  TRACK 03 MODE2/2352
    INDEX 01 02:00:00
  TRACK 04 AUDIO
    INDEX 01 03:00:00
"#,
    );

    let tracks = [
        track(1, CueMode::Mode1Raw, None, 0),
        track(2, CueMode::Mode1Cooked, None, msf(1, 0, 0)),
        track(3, CueMode::Mode2Raw, None, msf(2, 0, 0)),
        audio(4, None, msf(3, 0, 0)),
    ];
    assert_eq!(sheet.files[0].tracks, tracks);
}

#[test]
fn unsupported_modes_keep_original_text() {
    for mode in [
        "MODE2/2336",
        "CDI/2336",
        "CDI/2352",
        "CDG",
        "Mode1/2336",
        "WEIRD",
    ] {
        let text = format!("FILE \"game.bin\" BINARY\n  TRACK 01 {mode}\n    INDEX 01 00:00:00\n");
        assert_err!(&text, CueError::UnsupportedMode { line: 2, mode: ref m } if m == mode);
    }
}

#[test]
fn data_mode_in_audio_file() {
    for (name, mode) in [
        ("a.wav", "MODE1/2352"),
        ("a.flac", "MODE1/2048"),
        ("a.mp3", "MODE2/2352"),
    ] {
        let text = format!("FILE \"{name}\"\n  TRACK 01 {mode}\n    INDEX 01 00:00:00\n");
        assert_err!(&text, CueError::DataModeInAudioFile { line: 2, number: 1 });
    }
}

// Structure

#[test]
fn empty_sheets() {
    for text in ["", "\n  \r\n\t\n", "REM nothing here\nTITLE \"Empty\"\n"] {
        assert_err!(text, CueError::MissingFiles);
    }
}

#[test]
fn file_without_tracks() {
    // In the middle
    assert_err!(
        r#"FILE "a.bin" BINARY
  TRACK 01 AUDIO
    INDEX 01 00:00:00
FILE "b.bin" BINARY
FILE "c.bin" BINARY
  TRACK 02 AUDIO
    INDEX 01 00:00:00
"#,
        CueError::FileMissingTracks { line: 4 }
    );
    // At the end
    assert_err!(
        r#"FILE "a.bin" BINARY
  TRACK 01 AUDIO
    INDEX 01 00:00:00
FILE "b.bin" BINARY
"#,
        CueError::FileMissingTracks { line: 4 }
    );
}

#[test]
fn track_before_file() {
    assert_err!(
        "REM header\nTRACK 01 AUDIO\nFILE a.bin BINARY\n",
        CueError::TrackBeforeFile { line: 2 }
    );
}

#[test]
fn index_before_track() {
    assert_err!(
        "FILE a.bin BINARY\n  INDEX 01 00:00:00\n  TRACK 01 AUDIO\n",
        CueError::IndexBeforeTrack { line: 2 }
    );
}

#[test]
fn missing_index01() {
    // Followed by another track
    assert_err!(
        r#"FILE "a.bin" BINARY
  TRACK 01 AUDIO
    INDEX 00 00:00:00
  TRACK 02 AUDIO
    INDEX 01 01:00:00
"#,
        CueError::MissingIndex01 { line: 2, track: 1 }
    );
    // At the end
    assert_err!(
        r#"FILE "a.bin" BINARY
  TRACK 01 AUDIO
    INDEX 01 00:00:00
  TRACK 02 AUDIO
"#,
        CueError::MissingIndex01 { line: 4, track: 2 }
    );
}

// INDEX order and repeats

#[test]
fn index00_after_index01() {
    assert_err!(
        r#"FILE "a.bin" BINARY
  TRACK 01 AUDIO
    INDEX 00 00:05:00
    INDEX 01 00:02:00
"#,
        CueError::IndexOrder { line: 2 }
    );
}

#[test]
fn index01_before_previous_track() {
    assert_err!(
        r#"FILE "a.bin" BINARY
  TRACK 01 AUDIO
    INDEX 01 05:00:00
  TRACK 02 AUDIO
    INDEX 01 04:00:00
"#,
        CueError::IndexOrder { line: 4 }
    );
}

#[test]
fn pregap_overlaps_previous_track() {
    assert_err!(
        r#"FILE "a.bin" BINARY
  TRACK 01 AUDIO
    INDEX 01 05:00:00
  TRACK 02 AUDIO
    INDEX 00 04:00:00
    INDEX 01 06:00:00
"#,
        CueError::IndexOrder { line: 4 }
    );
}

#[test]
fn repeated_index01() {
    assert_err!(
        r#"FILE "a.bin" BINARY
  TRACK 01 AUDIO
    INDEX 01 00:00:00
    INDEX 01 00:02:00
"#,
        CueError::DuplicateIndex { line: 4, index: 1 }
    );
}

#[test]
fn repeated_index00() {
    assert_err!(
        r#"FILE "a.bin" BINARY
  TRACK 01 AUDIO
    INDEX 00 00:00:00
    INDEX 00 00:01:00
    INDEX 01 00:02:00
"#,
        CueError::DuplicateIndex { line: 4, index: 0 }
    );
}

#[test]
fn same_index_on_different_tracks_is_not_a_repeat() {
    let sheet = parse(
        r#"FILE "a.bin" BINARY
  TRACK 01 AUDIO
    INDEX 00 00:00:00
    INDEX 01 00:02:00
  TRACK 02 AUDIO
    INDEX 00 01:00:00
    INDEX 01 01:02:00
"#,
    );

    let tracks = [
        audio(1, Some(0), msf(0, 2, 0)),
        audio(2, Some(msf(1, 0, 0)), msf(1, 2, 0)),
    ];
    assert_eq!(sheet.files[0].tracks, tracks);
}

// TRACK numbers

#[test]
fn first_track_may_be_above_one() {
    let sheet = parse(
        r#"FILE "a.bin" BINARY
  TRACK 02 AUDIO
    INDEX 01 00:00:00
  TRACK 03 AUDIO
    INDEX 01 01:00:00
"#,
    );

    assert_eq!(
        sheet.files[0].tracks,
        [audio(2, None, 0), audio(3, None, msf(1, 0, 0))]
    );
}

#[test]
fn track_99_is_allowed() {
    let sheet = parse("FILE a.bin BINARY\n  TRACK 99 AUDIO\n    INDEX 01 00:00:00\n");

    assert_eq!(sheet.files[0].tracks, [audio(99, None, 0)]);
}

#[test]
fn track_number_out_of_range() {
    for number in [0u8, 100, 255] {
        let text = format!("FILE a.bin BINARY\n  TRACK {number:02} AUDIO\n    INDEX 01 00:00:00\n");
        assert_err!(&text, CueError::InvalidTrackNumber { line: 2, number: n } if n == number);
    }
}

#[test]
fn track_number_after_99() {
    assert_err!(
        r#"FILE "a.bin" BINARY
  TRACK 99 AUDIO
    INDEX 01 00:00:00
  TRACK 100 AUDIO
    INDEX 01 01:00:00
"#,
        CueError::InvalidTrackNumber {
            line: 4,
            number: 100
        }
    );
}

#[test]
fn duplicate_track_number() {
    assert_err!(
        r#"FILE "a.bin" BINARY
  TRACK 01 AUDIO
    INDEX 01 00:00:00
  TRACK 01 AUDIO
    INDEX 01 01:00:00
"#,
        CueError::TrackOrder { line: 4, number: 1 }
    );
}

#[test]
fn decreasing_track_number() {
    assert_err!(
        r#"FILE "a.bin" BINARY
  TRACK 02 AUDIO
    INDEX 01 00:00:00
  TRACK 01 AUDIO
    INDEX 01 01:00:00
"#,
        CueError::TrackOrder { line: 4, number: 1 }
    );
}

#[test]
fn gap_in_track_numbers() {
    // CDRWIN: all track numbers after the first must be sequential
    assert_err!(
        r#"FILE "a.bin" BINARY
  TRACK 01 AUDIO
    INDEX 01 00:00:00
  TRACK 03 AUDIO
    INDEX 01 01:00:00
"#,
        CueError::TrackOrder { line: 4, number: 3 }
    );
}

#[test]
fn track_order_spans_files() {
    assert_err!(
        r#"FILE "a.bin" BINARY
  TRACK 02 AUDIO
    INDEX 01 00:00:00
FILE "b.bin" BINARY
  TRACK 02 AUDIO
    INDEX 01 00:00:00
"#,
        CueError::TrackOrder { line: 5, number: 2 }
    );
}

// Syntax

#[test]
fn syntax_errors_report_line_and_text() {
    let bad_lines = [
        "INDEX 01 00:60:00",
        "INDEX 01 00:00:75",
        "INDEX 01 100:00:00",
        "INDEX 01 0a:00:00",
        "INDEX 01 00:00",
        "INDEX 01 -1:00:00",
        "INDEX 01",
        "INDEX xx 00:00:00",
        "INDEX 01 00:00:00 extra",
        "TRACK 02",
        "TRACK xx AUDIO",
        "TRACK 256 AUDIO",
        "TRACK 02 AUDIO extra",
        r#"FILE "b.bin" BINARY extra"#,
        r#"FILE "unterminated.bin BINARY"#,
        "FILE",
        "TRACK",
        "INDEX",
    ];
    let mut failures = vec![];
    for bad in bad_lines {
        // The track is already complete, so a wrongly accepted line can't trip a track check first
        let text =
            format!("FILE \"a.bin\" BINARY\n  TRACK 01 AUDIO\n    INDEX 01 00:00:00\n    {bad}\n");
        match CueSheet::parse(&text) {
            Err(CueError::Syntax { line: 4, ref text }) if text.trim() == bad => {}
            Err(e) => failures.push(format!("{bad:?}: got {e:?}")),
            Ok(_) => failures.push(format!("{bad:?}: parsed without error")),
        }
    }
    assert!(
        failures.is_empty(),
        "expected syntax errors on line 4:\n{}",
        failures.join("\n")
    );
}

#[test]
fn empty_quoted_name() {
    for line in [r#"FILE "" BINARY"#, r#"FILE """#] {
        assert_err!(&one_track(line), CueError::Syntax { line: 1, .. });
    }
}
