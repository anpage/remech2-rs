//! AI-generated tests for logical block access and ISO 9660 lookup

use std::fs;
use std::io::{Cursor, ErrorKind, Read, Seek, SeekFrom};
use std::path::{Path, PathBuf};
use std::sync::{Arc, Mutex};

use gamedisc::bincue::BinCueSource;
use gamedisc::iso::{IsoFs, LogicalBlocks};
use gamedisc::{Error, ReadSeek, SectorSource, Track, TrackKind, open_image};
use hadris_iso::file::EntryType;
use hadris_iso::joliet::JolietLevel;
use hadris_iso::read::{IsoImage, PathSeparator};
use hadris_iso::write::options::{CreationFeatures, IsoFormatOptions};
use hadris_iso::write::{File as IsoFile, InputFiles, IsoImageWriter};

const RAW: usize = 2352;
const BLOCK: usize = 2048;
const SYNC: [u8; 12] = [
    0x00, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0x00,
];

// Raw sectors

fn bcd(n: u32) -> u8 {
    (((n / 10) << 4) | (n % 10)) as u8
}

/// A raw data sector: mode 1 has user data at 16, mode 2 form 1 at 24 after the subheader
fn raw_sector(lba: u32, mode: u8, user: &[u8], synced: bool) -> Vec<u8> {
    assert_eq!(user.len(), BLOCK);
    let frames = lba + 150;
    let mut sector = vec![0; RAW];
    if synced {
        sector[..12].copy_from_slice(&SYNC);
    }
    sector[12..15].copy_from_slice(&[bcd(frames / 4500), bcd(frames / 75 % 60), bcd(frames % 75)]);
    sector[15] = mode;
    let offset = match mode {
        1 => 16,
        2 => {
            // Subheader, twice: file, channel, submode (data), coding
            sector[16..24].copy_from_slice(&[0, 0, 0x08, 0, 0, 0, 0x08, 0]);
            24
        }
        _ => unreachable!(),
    };
    sector[offset..offset + BLOCK].copy_from_slice(user);
    sector
}

/// Wraps 2048-byte blocks in raw sectors starting at `first_lba`
fn raw_track(user: &[u8], mode: u8, first_lba: u32, synced: bool) -> Vec<u8> {
    user.chunks(BLOCK)
        .zip(first_lba..)
        .flat_map(|(block, lba)| raw_sector(lba, mode, block, synced))
        .collect()
}

/// `count` blocks, each filled with `tag` and starting with its index
fn blocks(tag: u8, count: u32) -> Vec<u8> {
    let mut bytes = vec![tag; count as usize * BLOCK];
    for (i, block) in bytes.chunks_mut(BLOCK).enumerate() {
        block[..4].copy_from_slice(&(i as u32).to_le_bytes());
    }
    bytes
}

fn audio(count: u32) -> Vec<u8> {
    vec![0x55; count as usize * RAW]
}

fn track(number: u8, kind: TrackKind, start_lba: u32, sectors: u32) -> Track {
    Track {
        number,
        kind,
        start_lba,
        sectors,
    }
}

/// An in-memory disc that records every read
struct MemSource {
    tracks: Vec<Track>,
    raw: Vec<u8>,
    fail_at: Option<u32>,
    reads: Mutex<Vec<(u32, u32)>>,
}

impl MemSource {
    fn new(tracks: Vec<Track>, raw: Vec<u8>) -> Self {
        assert_eq!(raw.len() % RAW, 0);
        Self {
            tracks,
            raw,
            fail_at: None,
            reads: Mutex::new(vec![]),
        }
    }

    /// True if every read stayed inside `range`
    fn reads_within(&self, range: std::ops::Range<u32>) -> bool {
        self.reads
            .lock()
            .unwrap()
            .iter()
            .all(|&(lba, count)| range.start <= lba && lba + count <= range.end)
    }
}

impl SectorSource for MemSource {
    fn tracks(&self) -> &[Track] {
        &self.tracks
    }

    fn read_raw(&self, lba: u32, count: u32, out: &mut [u8]) -> Result<(), Error> {
        self.reads.lock().unwrap().push((lba, count));
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

fn logical<S: SectorSource>(source: S) -> LogicalBlocks<S> {
    LogicalBlocks::new(source).unwrap_or_else(|e| panic!("LogicalBlocks::new failed: {e:?}"))
}

fn read_all(reader: &mut impl Read) -> Vec<u8> {
    let mut all = vec![];
    reader.read_to_end(&mut all).unwrap();
    all
}

// ISO images

fn file(name: &str, contents: &[u8]) -> IsoFile {
    IsoFile::File {
        name: Arc::new(name.to_owned()),
        contents: contents.to_vec(),
    }
}

fn dir(name: &str, children: Vec<IsoFile>) -> IsoFile {
    IsoFile::Directory {
        name: Arc::new(name.to_owned()),
        children,
    }
}

/// An ISO image sized to the volume space its primary volume descriptor declares
fn iso(files: Vec<IsoFile>, features: CreationFeatures) -> Vec<u8> {
    let options = IsoFormatOptions {
        volume_name: "MECH2".to_owned(),
        system_id: None,
        volume_set_id: None,
        publisher_id: None,
        preparer_id: None,
        application_id: None,
        sector_size: BLOCK,
        path_separator: PathSeparator::ForwardSlash,
        features,
        strict_charset: false,
    };
    let input = InputFiles {
        path_separator: PathSeparator::ForwardSlash,
        files,
    };
    let mut buffer = Cursor::new(vec![]);
    IsoImageWriter::create(&mut buffer, input, options).expect("failed to write ISO");
    let mut bytes = buffer.into_inner();
    let pvd = 16 * BLOCK;
    let volume_space = u32::from_le_bytes(bytes[pvd + 80..pvd + 84].try_into().unwrap());
    bytes.resize(volume_space as usize * BLOCK, 0);
    bytes
}

/// Spans several 64 KiB extraction chunks
fn big() -> Vec<u8> {
    (0..200_000u32).map(|i| (i * 7 % 251) as u8).collect()
}

fn mech2_iso() -> Vec<u8> {
    iso(
        vec![
            file("OLD_HERC.DRV", b"hercules"),
            dir(
                "MECH2",
                vec![
                    file("MW2.PRJ", b"project"),
                    file("BIG.DAT", &big()),
                    dir("SUB", vec![file("DEEP.TXT", b"deep")]),
                ],
            ),
        ],
        CreationFeatures::default(),
    )
}

fn open_fs<D: Read + Seek>(data: D) -> IsoFs<D> {
    IsoFs::open(data).unwrap_or_else(|e| panic!("IsoFs::open failed: {e:?}"))
}

fn extract<D: Read + Seek>(fs: &IsoFs<D>, path: &str) -> Option<Vec<u8>> {
    let mut out = vec![];
    let written = fs
        .extract(path, &mut out)
        .unwrap_or_else(|e| panic!("extract({path}) failed: {e:?}"));
    match written {
        Some(n) => {
            assert_eq!(n, out.len() as u64, "{path}: reported size");
            Some(out)
        }
        None => {
            assert!(out.is_empty(), "{path}: wrote {} bytes but found nothing", out.len());
            None
        }
    }
}

/// The checks every MechWarrior 2 test image should pass
fn assert_mech2_contents<D: Read + Seek>(fs: &IsoFs<D>) {
    assert!(fs.has_file("OLD_HERC.DRV").unwrap());
    assert_eq!(extract(fs, "OLD_HERC.DRV").unwrap(), b"hercules");
    assert_eq!(extract(fs, "MECH2/MW2.PRJ").unwrap(), b"project");
    assert_eq!(extract(fs, "MECH2/BIG.DAT").unwrap(), big());
}

// Files on disk

fn scratch(name: &str) -> PathBuf {
    let dir = Path::new(env!("CARGO_TARGET_TMPDIR")).join("iso").join(name);
    let _ = fs::remove_dir_all(&dir);
    fs::create_dir_all(&dir).unwrap();
    dir
}

fn msf(sectors: u32) -> String {
    format!("{:02}:{:02}:{:02}", sectors / 4500, sectors / 75 % 60, sectors % 75)
}

fn find(haystack: &[u8], needle: &[u8]) -> usize {
    haystack
        .windows(needle.len())
        .position(|w| w == needle)
        .expect("pattern not in image")
}

// LogicalBlocks: sector layouts

#[test]
fn mode1_user_data() {
    let user = blocks(1, 4);
    let source = MemSource::new(vec![track(1, TrackKind::Mode1, 0, 4)], raw_track(&user, 1, 0, true));

    assert_eq!(read_all(&mut logical(&source)), user);
}

#[test]
fn mode2_form1_user_data() {
    let user = blocks(1, 4);
    let source = MemSource::new(
        vec![track(1, TrackKind::Mode2Form1, 0, 4)],
        raw_track(&user, 2, 0, true),
    );

    assert_eq!(read_all(&mut logical(&source)), user);
}

#[test]
fn sector_header_wins_over_track_kind() {
    let user = blocks(1, 4);

    let source = MemSource::new(vec![track(1, TrackKind::Mode1, 0, 4)], raw_track(&user, 2, 0, true));
    assert_eq!(read_all(&mut logical(&source)), user, "mode 2 sectors on a MODE1 track");

    let source = MemSource::new(
        vec![track(1, TrackKind::Mode2Form1, 0, 4)],
        raw_track(&user, 1, 0, true),
    );
    assert_eq!(read_all(&mut logical(&source)), user, "mode 1 sectors on a MODE2 track");
}

#[test]
fn track_kind_is_the_fallback_without_sync() {
    let user = blocks(1, 4);

    let source = MemSource::new(vec![track(1, TrackKind::Mode1, 0, 4)], raw_track(&user, 1, 0, false));
    assert_eq!(read_all(&mut logical(&source)), user, "MODE1");

    let source = MemSource::new(
        vec![track(1, TrackKind::Mode2Form1, 0, 4)],
        raw_track(&user, 2, 0, false),
    );
    assert_eq!(read_all(&mut logical(&source)), user, "MODE2");
}

// LogicalBlocks: track choice and bounds

#[test]
fn first_data_track_is_used() {
    // An audio track before the data, as on some enhanced CDs
    let user = blocks(1, 4);
    let raw = [audio(3), raw_track(&user, 1, 3, true)].concat();
    let source = MemSource::new(
        vec![
            track(1, TrackKind::Audio, 0, 3),
            track(2, TrackKind::Mode1, 3, 4),
        ],
        raw,
    );

    assert_eq!(read_all(&mut logical(&source)), user);
    assert!(source.reads_within(3..7), "{:?}", source.reads.lock().unwrap());
}

#[test]
fn reads_stop_at_the_track_end() {
    let user = blocks(1, 4);
    let raw = [raw_track(&user, 1, 0, true), audio(3)].concat();
    let source = MemSource::new(
        vec![
            track(1, TrackKind::Mode1, 0, 4),
            track(2, TrackKind::Audio, 4, 3),
        ],
        raw,
    );

    assert_eq!(read_all(&mut logical(&source)), user);
    assert!(source.reads_within(0..4), "{:?}", source.reads.lock().unwrap());
}

#[test]
fn no_data_track() {
    let source = MemSource::new(vec![track(1, TrackKind::Audio, 0, 3)], audio(3));

    assert!(matches!(LogicalBlocks::new(&source), Err(Error::NoDataTrack)));
}

#[test]
fn source_errors_become_io_errors() {
    let user = blocks(1, 4);
    let mut source = MemSource::new(vec![track(1, TrackKind::Mode1, 0, 4)], raw_track(&user, 1, 0, true));
    source.fail_at = Some(2);
    let mut blocks = logical(&source);

    let mut buf = [0; 16];
    blocks.seek(SeekFrom::Start(2 * BLOCK as u64)).unwrap();
    assert!(blocks.read(&mut buf).is_err());
}

// LogicalBlocks: Read and Seek

#[test]
fn reads_cross_sector_boundaries() {
    let user = blocks(1, 4);
    let source = MemSource::new(vec![track(1, TrackKind::Mode1, 0, 4)], raw_track(&user, 1, 0, true));
    let mut blocks = logical(&source);

    let start = 2 * BLOCK - 3;
    assert_eq!(blocks.seek(SeekFrom::Start(start as u64)).unwrap(), start as u64);
    let mut buf = [0; 3000];
    blocks.read_exact(&mut buf).unwrap();
    assert_eq!(buf[..], user[start..start + 3000]);
    assert_eq!(blocks.stream_position().unwrap(), (start + 3000) as u64);
}

#[test]
fn seeking() {
    let user = blocks(1, 4);
    let source = MemSource::new(vec![track(1, TrackKind::Mode1, 0, 4)], raw_track(&user, 1, 0, true));
    let mut blocks = logical(&source);
    let len = 4 * BLOCK as u64;
    let mut buf = [0; 10];

    assert_eq!(blocks.seek(SeekFrom::End(0)).unwrap(), len);
    assert_eq!(blocks.read(&mut buf).unwrap(), 0);

    assert_eq!(blocks.seek(SeekFrom::End(-5)).unwrap(), len - 5);
    assert_eq!(blocks.read_exact(&mut buf).unwrap_err().kind(), ErrorKind::UnexpectedEof);

    assert_eq!(blocks.seek(SeekFrom::Start(100)).unwrap(), 100);
    assert_eq!(blocks.seek(SeekFrom::Current(-40)).unwrap(), 60);
    blocks.read_exact(&mut buf).unwrap();
    assert_eq!(buf[..], user[60..70]);

    // Past the end is allowed and reads nothing
    assert_eq!(blocks.seek(SeekFrom::Start(len + 1000)).unwrap(), len + 1000);
    assert_eq!(blocks.read(&mut buf).unwrap(), 0);

    // Before the start is not
    assert_eq!(
        blocks.seek(SeekFrom::Current(-10_000_000)).unwrap_err().kind(),
        ErrorKind::InvalidInput
    );
}

// IsoFs: lookup

#[test]
fn plain_iso() {
    let fs = open_fs(Cursor::new(mech2_iso()));

    assert_mech2_contents(&fs);
    assert_eq!(extract(&fs, "MECH2/SUB/DEEP.TXT").unwrap(), b"deep");
}

#[test]
fn paths_are_case_insensitive_with_either_separator() {
    let fs = open_fs(Cursor::new(mech2_iso()));

    for path in [
        "old_herc.drv",
        "Old_Herc.Drv",
        "MECH2/MW2.PRJ",
        r"MECH2\MW2.PRJ",
        "mech2/mw2.prj",
        "/MECH2/MW2.PRJ",
        r"\MECH2\SUB\DEEP.TXT",
        "MECH2//SUB/DEEP.TXT",
    ] {
        assert!(fs.has_file(path).unwrap(), "{path}");
    }
}

#[test]
fn only_files_are_found() {
    let fs = open_fs(Cursor::new(mech2_iso()));

    for path in [
        "",
        "/",
        "MECH2",
        "MECH2/",
        "MECH2/SUB",
        "MISSING.TXT",
        "MECH2/MISSING.TXT",
        "MISSING/MW2.PRJ",
        "OLD_HERC.DRV/MW2.PRJ",
        "MW2.PRJ",
    ] {
        assert!(!fs.has_file(path).unwrap(), "{path}");
        assert_eq!(extract(&fs, path), None, "{path}");
    }
}

#[test]
fn empty_file() {
    let fs = open_fs(Cursor::new(iso(
        vec![file("OLD_HERC.DRV", b"hercules"), file("EMPTY.DAT", b"")],
        CreationFeatures::default(),
    )));

    assert!(fs.has_file("EMPTY.DAT").unwrap());
    assert_eq!(extract(&fs, "EMPTY.DAT").unwrap(), b"");
}

#[test]
fn joliet_tree_is_not_used() {
    // Joliet keeps the mixed case and compares it exactly; the plain tree does not
    let bytes = iso(
        vec![
            file("Old_Herc.drv", b"hercules"),
            dir("Mech2", vec![file("Mw2.prj", b"project")]),
        ],
        CreationFeatures::joliet(JolietLevel::Level3),
    );
    let image = IsoImage::open(Cursor::new(bytes.clone())).unwrap();
    assert!(
        image
            .root_dirs()
            .iter()
            .any(|root| matches!(root.entry_type(), EntryType::Joliet { .. })),
        "test image has no Joliet tree"
    );

    let fs = open_fs(Cursor::new(bytes));

    assert!(fs.has_file("OLD_HERC.DRV").unwrap());
    assert_eq!(extract(&fs, "MECH2/MW2.PRJ").unwrap(), b"project");
}

#[test]
fn not_an_iso() {
    assert!(IsoFs::open(Cursor::new(vec![0; 40 * BLOCK])).is_err());
    assert!(IsoFs::open(Cursor::new(vec![])).is_err());
}

// IsoFs over a disc image

#[test]
fn raw_data_track_in_memory() {
    let iso = mech2_iso();
    let sectors = (iso.len() / BLOCK) as u32;
    let source = MemSource::new(
        vec![
            track(1, TrackKind::Mode1, 0, sectors),
            track(2, TrackKind::Audio, sectors, 10),
        ],
        [raw_track(&iso, 1, 0, true), audio(10)].concat(),
    );

    assert_mech2_contents(&open_fs(logical(&source)));
}

#[test]
fn single_file_bin_cue() {
    let dir = scratch("single_file_bin_cue");
    let iso = mech2_iso();
    let sectors = (iso.len() / BLOCK) as u32;
    fs::write(
        dir.join("MECH2.BIN"),
        [raw_track(&iso, 1, 0, true), audio(300)].concat(),
    )
    .unwrap();
    fs::write(
        dir.join("MECH2.CUE"),
        format!(
            "FILE \"MECH2.BIN\" BINARY\n  TRACK 01 MODE1/2352\n    INDEX 01 00:00:00\n  TRACK 02 AUDIO\n    INDEX 00 {}\n    INDEX 01 {}\n",
            msf(sectors),
            msf(sectors + 150),
        ),
    )
    .unwrap();

    let source = BinCueSource::open(dir.join("MECH2.CUE")).unwrap();
    assert_mech2_contents(&open_fs(logical(source)));
}

#[test]
fn split_bin_cue() {
    let dir = scratch("split_bin_cue");
    let iso = mech2_iso();
    fs::write(dir.join("Track 1.bin"), raw_track(&iso, 1, 0, true)).unwrap();
    fs::write(dir.join("Track 2.bin"), audio(300)).unwrap();
    fs::write(
        dir.join("game.cue"),
        r#"FILE "Track 1.bin" BINARY
  TRACK 01 MODE1/2352
    INDEX 01 00:00:00
FILE "Track 2.bin" BINARY
  TRACK 02 AUDIO
    INDEX 00 00:00:00
    INDEX 01 00:02:00
"#,
    )
    .unwrap();

    let source = BinCueSource::open(dir.join("game.cue")).unwrap();
    assert_mech2_contents(&open_fs(logical(source)));
}

#[test]
fn cooked_iso_in_cue() {
    let dir = scratch("cooked_iso_in_cue");
    fs::write(dir.join("data.iso"), mech2_iso()).unwrap();
    fs::write(dir.join("audio.bin"), audio(300)).unwrap();
    fs::write(
        dir.join("game.cue"),
        r#"FILE "data.iso" BINARY
  TRACK 01 MODE1/2048
    INDEX 01 00:00:00
FILE "audio.bin" BINARY
  TRACK 02 AUDIO
    INDEX 01 00:00:00
"#,
    )
    .unwrap();

    let source = BinCueSource::open(dir.join("game.cue")).unwrap();
    assert_mech2_contents(&open_fs(logical(source)));
}

#[test]
fn file_past_the_track_end_fails() {
    // Stretch BIG.DAT's directory record 10 sectors past the data track, into the audio after it.
    // The writer puts the path tables after the files, so cutting the image short would break open().
    let mut iso = mech2_iso();
    let sectors = (iso.len() / BLOCK) as u32;
    let name_at = find(&iso, b"BIG.DAT");
    assert!(
        matches!(iso[name_at - 1], 7 | 9),
        "BIG.DAT should be a directory record name"
    );
    let record = name_at - 33;
    let extent = u32::from_le_bytes(iso[record + 2..record + 6].try_into().unwrap());
    let len = (sectors - extent + 10) * BLOCK as u32;
    iso[record + 10..record + 14].copy_from_slice(&len.to_le_bytes());
    iso[record + 14..record + 18].copy_from_slice(&len.to_be_bytes());

    let dir = scratch("file_past_the_track_end_fails");
    fs::write(
        dir.join("game.bin"),
        [raw_track(&iso, 1, 0, true), audio(300)].concat(),
    )
    .unwrap();
    fs::write(
        dir.join("game.cue"),
        format!(
            "FILE \"game.bin\" BINARY\n  TRACK 01 MODE1/2352\n    INDEX 01 00:00:00\n  TRACK 02 AUDIO\n    INDEX 01 {}\n",
            msf(sectors),
        ),
    )
    .unwrap();

    let source = BinCueSource::open(dir.join("game.cue")).unwrap();
    let fs = open_fs(logical(source));

    assert!(fs.has_file("MECH2/BIG.DAT").unwrap());
    let mut out = vec![];
    assert!(fs.extract("MECH2/BIG.DAT", &mut out).is_err());
}

// open_image

#[test]
fn opened_images_can_move_between_threads() {
    fn assert_send<T: Send>() {}
    assert_send::<IsoFs<Box<dyn ReadSeek>>>();
}

#[test]
fn open_image_reads_an_iso() {
    let dir = scratch("open_image_reads_an_iso");
    fs::write(dir.join("MECH2.ISO"), mech2_iso()).unwrap();

    let fs = open_image(dir.join("MECH2.ISO")).unwrap_or_else(|e| panic!("{e:?}"));
    assert_mech2_contents(&fs);
}

#[test]
fn open_image_reads_a_cue() {
    let dir = scratch("open_image_reads_a_cue");
    let iso = mech2_iso();
    let sectors = (iso.len() / BLOCK) as u32;
    fs::write(
        dir.join("mech2.bin"),
        [raw_track(&iso, 1, 0, true), audio(300)].concat(),
    )
    .unwrap();
    fs::write(
        dir.join("Mech2.Cue"),
        format!(
            "FILE \"mech2.bin\" BINARY\n  TRACK 01 MODE1/2352\n    INDEX 01 00:00:00\n  TRACK 02 AUDIO\n    INDEX 01 {}\n",
            msf(sectors),
        ),
    )
    .unwrap();

    let fs = open_image(dir.join("Mech2.Cue")).unwrap_or_else(|e| panic!("{e:?}"));
    assert_mech2_contents(&fs);
}

#[test]
fn open_image_rejects_other_extensions() {
    let dir = scratch("open_image_rejects_other_extensions");
    for name in ["game.mds", "game.bin", "game"] {
        let path = dir.join(name);
        fs::write(&path, mech2_iso()).unwrap();

        let result = open_image(&path);
        assert!(
            matches!(result, Err(Error::UnsupportedImage(ref p)) if *p == path),
            "{name}"
        );
    }
}

#[test]
fn open_image_errors() {
    let dir = scratch("open_image_errors");

    let missing = open_image(dir.join("missing.iso"));
    assert!(matches!(missing, Err(Error::Io(_))), "missing .iso");

    let missing = open_image(dir.join("missing.cue"));
    assert!(matches!(missing, Err(Error::Io(_))), "missing .cue");

    fs::write(dir.join("music.bin"), audio(300)).unwrap();
    fs::write(
        dir.join("music.cue"),
        "FILE \"music.bin\" BINARY\n  TRACK 01 AUDIO\n    INDEX 01 00:00:00\n",
    )
    .unwrap();
    assert!(matches!(open_image(dir.join("music.cue")), Err(Error::NoDataTrack)));

    fs::write(dir.join("zeros.iso"), vec![0; 40 * BLOCK]).unwrap();
    assert!(matches!(open_image(dir.join("zeros.iso")), Err(Error::Iso(_))));
}
