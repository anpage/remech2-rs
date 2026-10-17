//! AI-generated tests for reading raw sectors from a BIN/CUE image

use std::fs;
use std::path::{Path, PathBuf};

use gamedisc::bincue::BinCueSource;
use gamedisc::cue::{CueError, resolve_file};
use gamedisc::{Error, SectorSource, Track, TrackKind};

const RAW: usize = 2352;
const COOKED: usize = 2048;
const SYNC: [u8; 12] = [
    0x00, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0x00,
];

/// An empty directory of its own for each test
fn scratch(name: &str) -> PathBuf {
    let dir = Path::new(env!("CARGO_TARGET_TMPDIR"))
        .join("bincue")
        .join(name);
    let _ = fs::remove_dir_all(&dir);
    fs::create_dir_all(&dir).unwrap();
    dir
}

/// `count` sectors of `size` bytes, each filled with `tag` and starting with its index
fn sectors(tag: u8, count: u32, size: usize) -> Vec<u8> {
    let mut bytes = vec![tag; count as usize * size];
    for (i, sector) in bytes.chunks_mut(size).enumerate() {
        sector[..4].copy_from_slice(&(i as u32).to_le_bytes());
    }
    bytes
}

fn write_bin(dir: &Path, name: &str, tag: u8, count: u32, size: usize) -> Vec<u8> {
    let bytes = sectors(tag, count, size);
    fs::write(dir.join(name), &bytes).unwrap();
    bytes
}

fn write_cue(dir: &Path, text: &str) -> PathBuf {
    let path = dir.join("game.cue");
    fs::write(&path, text).unwrap();
    path
}

fn open(cue: &Path) -> BinCueSource {
    BinCueSource::open(cue).unwrap_or_else(|e| panic!("open failed: {e:?}"))
}

fn open_err(cue: &Path) -> Error {
    match BinCueSource::open(cue) {
        Err(e) => e,
        Ok(source) => panic!("expected an error, got tracks {:?}", source.tracks()),
    }
}

fn read(source: &BinCueSource, lba: u32, count: u32) -> Vec<u8> {
    let mut out = vec![0; count as usize * RAW];
    source
        .read_raw(lba, count, &mut out)
        .unwrap_or_else(|e| panic!("read_raw({lba}, {count}) failed: {e:?}"));
    out
}

fn track(number: u8, kind: TrackKind, start_lba: u32, sectors: u32) -> Track {
    Track {
        number,
        kind,
        start_lba,
        sectors,
    }
}

#[test]
fn source_can_be_shared_between_threads() {
    fn assert_send_sync<T: Send + Sync>() {}
    assert_send_sync::<BinCueSource>();
}

// Layouts

#[test]
fn single_file() {
    let dir = scratch("single_file");
    let bin = write_bin(&dir, "game.bin", 1, 100, RAW);
    let cue = write_cue(
        &dir,
        r#"FILE "game.bin" BINARY
  TRACK 01 MODE1/2352
    INDEX 01 00:00:00
  TRACK 02 AUDIO
    INDEX 00 00:00:60
    INDEX 01 00:01:00
"#,
    );

    let source = open(&cue);

    assert_eq!(
        source.tracks(),
        [
            track(1, TrackKind::Mode1, 0, 75),
            track(2, TrackKind::Audio, 75, 25),
        ]
    );
    assert_eq!(read(&source, 0, 100), bin);
    assert_eq!(read(&source, 70, 10), bin[70 * RAW..80 * RAW]);
    assert_eq!(read(&source, 99, 1), bin[99 * RAW..]);
}

#[test]
fn split_files() {
    let dir = scratch("split_files");
    let one = write_bin(&dir, "Game (Track 1).bin", 1, 10, RAW);
    let two = write_bin(&dir, "Game (Track 2).bin", 2, 8, RAW);
    let three = write_bin(&dir, "Game (Track 3).bin", 3, 6, RAW);
    let cue = write_cue(
        &dir,
        r#"FILE "Game (Track 1).bin" BINARY
  TRACK 01 MODE1/2352
    INDEX 01 00:00:00
FILE "Game (Track 2).bin" BINARY
  TRACK 02 AUDIO
    INDEX 00 00:00:00
    INDEX 01 00:00:02
FILE "Game (Track 3).bin" BINARY
  TRACK 03 AUDIO
    INDEX 00 00:00:00
    INDEX 01 00:00:02
"#,
    );

    let source = open(&cue);

    // Each pregap is stored in the next file but belongs to the previous track
    assert_eq!(
        source.tracks(),
        [
            track(1, TrackKind::Mode1, 0, 12),
            track(2, TrackKind::Audio, 12, 8),
            track(3, TrackKind::Audio, 20, 4),
        ]
    );
    assert_eq!(
        read(&source, 0, 24),
        [&one[..], &two[..], &three[..]].concat()
    );
    assert_eq!(
        read(&source, 8, 4),
        [&one[8 * RAW..], &two[..2 * RAW]].concat()
    );
    assert_eq!(read(&source, 10, 1), two[..RAW]);
    assert_eq!(
        read(&source, 17, 2),
        [&two[7 * RAW..], &three[..RAW]].concat()
    );
}

#[test]
fn cooked_data_track_reads_as_raw_mode1_sectors() {
    let dir = scratch("cooked_data_track");
    let data = write_bin(&dir, "data.iso", 1, 1200, COOKED);
    let audio = write_bin(&dir, "audio.bin", 2, 5, RAW);
    let cue = write_cue(
        &dir,
        r#"FILE "data.iso" BINARY
  TRACK 01 MODE1/2048
    INDEX 01 00:00:00
FILE "audio.bin" BINARY
  TRACK 02 AUDIO
    INDEX 01 00:00:00
"#,
    );

    let source = open(&cue);

    assert_eq!(
        source.tracks(),
        [
            track(1, TrackKind::Mode1, 0, 1200),
            track(2, TrackKind::Audio, 1200, 5),
        ]
    );

    // Sync, BCD address of LBA + 150, mode 1, user data, zeroed EDC/ECC
    let expect = |lba: u32, msf: [u8; 3]| {
        let user = &data[lba as usize * COOKED..][..COOKED];
        [
            &SYNC[..],
            &msf[..],
            &[1][..],
            user,
            &[0; RAW - 16 - COOKED][..],
        ]
        .concat()
    };
    assert_eq!(read(&source, 0, 1), expect(0, [0x00, 0x02, 0x00]));
    // 1124 + 150 = 1274 = 16 * 75 + 74
    assert_eq!(read(&source, 1124, 1), expect(1124, [0x00, 0x16, 0x74]));
    assert_eq!(
        read(&source, 1199, 2),
        [expect(1199, [0x00, 0x17, 0x74]), audio[..RAW].to_vec()].concat()
    );
}

// read_raw bounds

#[test]
fn reads_stop_at_the_leadout() {
    let dir = scratch("reads_stop_at_the_leadout");
    write_bin(&dir, "game.bin", 1, 20, RAW);
    let cue = write_cue(
        &dir,
        "FILE \"game.bin\" BINARY\n  TRACK 01 AUDIO\n    INDEX 01 00:00:00\n",
    );
    let source = open(&cue);

    read(&source, 15, 5);
    read(&source, 20, 0);

    let mut out = vec![0; 2 * RAW];
    for (lba, count) in [(19, 2), (20, 1), (100, 1), (u32::MAX, 2)] {
        let result = source.read_raw(lba, count, &mut out[..count as usize * RAW]);
        assert!(
            matches!(result, Err(Error::OutOfRange { lba: l, count: c }) if l == lba && c == count),
            "read_raw({lba}, {count}) gave {result:?}"
        );
    }
}

// File names

#[test]
fn names_resolve_case_insensitively_by_final_component() {
    let dir = scratch("names_resolve_case_insensitively");
    fs::write(dir.join("mech2.bin"), b"").unwrap();
    let expected = dir.join("mech2.bin");

    for name in [
        "mech2.bin",
        "MECH2.BIN",
        "Mech2.Bin",
        r"C:\Dumps\MECH2.BIN",
        "/elsewhere/dumps/MECH2.BIN",
        "dumps/Mech2.bin",
    ] {
        assert_eq!(resolve_file(&dir, name).unwrap(), expected, "{name}");
    }
}

#[cfg(not(any(windows, target_os = "macos")))]
#[test]
fn exact_name_wins_over_case_insensitive_match() {
    let dir = scratch("exact_name_wins");
    fs::write(dir.join("a.bin"), b"").unwrap();
    fs::write(dir.join("A.BIN"), b"").unwrap();

    assert_eq!(resolve_file(&dir, "a.bin").unwrap(), dir.join("a.bin"));
    assert_eq!(resolve_file(&dir, "A.BIN").unwrap(), dir.join("A.BIN"));
}

#[test]
fn missing_file_reports_where_it_looked() {
    let dir = scratch("missing_file_reports");
    fs::write(dir.join("other.bin"), b"").unwrap();

    let result = resolve_file(&dir, r"C:\Dumps\GAME.BIN");
    assert!(
        matches!(result, Err(CueError::FileNotFound(ref p)) if *p == dir.join("GAME.BIN")),
        "{result:?}"
    );
}

#[test]
fn open_resolves_names() {
    let dir = scratch("open_resolves_names");
    let bin = write_bin(&dir, "mech2.bin", 1, 4, RAW);
    let cue = write_cue(
        &dir,
        "FILE \"D:\\RIPS\\MECH2.BIN\" BINARY\n  TRACK 01 AUDIO\n    INDEX 01 00:00:00\n",
    );

    assert_eq!(read(&open(&cue), 0, 4), bin);
}

#[test]
fn utf8_cue_with_byte_order_mark() {
    let dir = scratch("utf8_cue_with_bom");
    let bin = write_bin(&dir, "café.bin", 1, 2, RAW);
    let cue = write_cue(
        &dir,
        "\u{feff}FILE \"café.bin\" BINARY\n  TRACK 01 AUDIO\n    INDEX 01 00:00:00\n",
    );

    assert_eq!(read(&open(&cue), 0, 2), bin);
}

#[test]
fn latin1_cue_falls_back_from_utf8() {
    let dir = scratch("latin1_cue");
    let bin = write_bin(&dir, "café.bin", 1, 2, RAW);
    let cue = dir.join("game.cue");
    fs::write(
        &cue,
        b"FILE \"caf\xe9.bin\" BINARY\r\n  TRACK 01 AUDIO\r\n    INDEX 01 00:00:00\r\n",
    )
    .unwrap();

    assert_eq!(read(&open(&cue), 0, 2), bin);
}

// Errors

#[test]
fn missing_cue() {
    let dir = scratch("missing_cue");
    let err = open_err(&dir.join("nothing.cue"));
    assert!(matches!(err, Error::Io(_)), "{err:?}");
}

#[test]
fn missing_bin() {
    let dir = scratch("missing_bin");
    let cue = write_cue(
        &dir,
        "FILE \"GAME.BIN\" BINARY\n  TRACK 01 AUDIO\n    INDEX 01 00:00:00\n",
    );

    let err = open_err(&cue);
    assert!(
        matches!(err, Error::Cue(CueError::FileNotFound(ref p)) if *p == dir.join("GAME.BIN")),
        "{err:?}"
    );
}

#[test]
fn bad_sheet() {
    let dir = scratch("bad_sheet");
    let cue = write_cue(&dir, "FILE \"game.bin\" BINARY\n  TRACK 01 AUDIO\n");

    let err = open_err(&cue);
    assert!(
        matches!(
            err,
            Error::Cue(CueError::MissingIndex01 { line: 2, track: 1 })
        ),
        "{err:?}"
    );
}

#[test]
fn audio_files_are_not_sector_sources() {
    let dir = scratch("audio_files");
    write_bin(&dir, "data.bin", 1, 10, RAW);
    fs::write(dir.join("02.wav"), b"RIFF").unwrap();
    let cue = write_cue(
        &dir,
        r#"FILE "data.bin" BINARY
  TRACK 01 MODE1/2352
    INDEX 01 00:00:00
FILE "02.wav" WAVE
  TRACK 02 AUDIO
    INDEX 01 00:00:00
"#,
    );

    let err = open_err(&cue);
    assert!(
        matches!(err, Error::NotBinary(ref p) if *p == dir.join("02.wav")),
        "{err:?}"
    );
}

#[test]
fn partial_trailing_sector() {
    for (mode, size) in [("MODE1/2352", RAW), ("MODE1/2048", COOKED)] {
        let dir = scratch(&format!("partial_trailing_sector_{}", size));
        let mut bin = sectors(1, 10, size);
        bin.extend_from_slice(&[0; 100]);
        fs::write(dir.join("game.bin"), &bin).unwrap();
        let cue = write_cue(
            &dir,
            &format!("FILE \"game.bin\" BINARY\n  TRACK 01 {mode}\n    INDEX 01 00:00:00\n"),
        );

        let err = open_err(&cue);
        assert!(
            matches!(err, Error::PartialSector(ref p) if *p == dir.join("game.bin")),
            "{mode}: {err:?}"
        );
    }
}

#[test]
fn track_past_end_of_bin() {
    let dir = scratch("track_past_end_of_bin");
    write_bin(&dir, "game.bin", 1, 50, RAW);
    let cue = write_cue(
        &dir,
        r#"FILE "game.bin" BINARY
  TRACK 01 MODE1/2352
    INDEX 01 00:00:00
  TRACK 02 AUDIO
    INDEX 01 00:01:00
"#,
    );

    let err = open_err(&cue);
    assert!(
        matches!(err, Error::Cue(CueError::TrackPastEndOfFile { track: 2 })),
        "{err:?}"
    );
}
