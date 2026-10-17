use std::{
    fs::File,
    io,
    path::{Path, PathBuf},
};

use crate::{
    bincue::BinCueSource,
    cue::{CueError, CueMode},
    iso::{IsoFs, LogicalBlocks},
};

pub mod audio;
pub mod bincue;
pub mod cue;
pub mod iso;

const RAW_SECTOR: usize = 2352;
const SYNC: [u8; 12] = [
    0x00, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0x00,
];

#[derive(Debug, thiserror::Error)]
pub enum Error {
    #[error("i/o error")]
    Io(#[from] io::Error),
    #[error(transparent)]
    Cue(#[from] CueError),
    #[error("{0} is an audio file, not raw sectors")]
    NotBinary(PathBuf),
    #[error("{0} does not hold a whole number of sectors")]
    PartialSector(PathBuf),
    #[error("sectors {lba}..+{count} are past the end of the disc")]
    OutOfRange { lba: u32, count: u32 },
    #[error("disc has no data track")]
    NoDataTrack,
    #[error("image has no ISO 9660 directory tree")]
    NoDirectoryTree,
    #[error("ISO 9660 error")]
    Iso(#[from] hadris_iso::sync::Error),
    #[error("drive error")]
    Drive(#[from] cd_da_reader::CdReaderError),
    #[error("track {track} is not an audio track")]
    NotAudioTrack { track: u8 },
    #[error("FLAC encoder error: {0}")]
    Flac(String),
    #[error("{0} is not a supported disc image")]
    UnsupportedImage(PathBuf),
}

pub trait SectorSource {
    fn tracks(&self) -> &[Track];
    /// Read `count` raw 2352-byte sectors starting at `lba` into `out`
    fn read_raw(&self, lba: u32, count: u32, out: &mut [u8]) -> Result<(), Error>;
}

impl<S: SectorSource + ?Sized> SectorSource for &S {
    fn tracks(&self) -> &[Track] {
        (**self).tracks()
    }

    fn read_raw(&self, lba: u32, count: u32, out: &mut [u8]) -> Result<(), Error> {
        (**self).read_raw(lba, count, out)
    }
}

#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub enum TrackKind {
    Audio,
    Mode1,
    Mode2Form1,
}

impl From<CueMode> for TrackKind {
    fn from(value: CueMode) -> Self {
        match value {
            CueMode::Audio => TrackKind::Audio,
            CueMode::Mode1Raw | CueMode::Mode1Cooked => TrackKind::Mode1,
            CueMode::Mode2Raw => TrackKind::Mode2Form1,
        }
    }
}

#[derive(Debug, Clone, PartialEq, Eq)]
pub struct Track {
    /// Starts at 1
    pub number: u8,
    pub kind: TrackKind,
    /// The LBA of INDEX 01
    pub start_lba: u32,
    /// The number of sectors from `start_lba` up to the next track's `start_lba` or the lead-out
    pub sectors: u32,
}

/// Read + Seek that can be moved to another thread
pub trait ReadSeek: io::Read + io::Seek + Send {}

impl<T: io::Read + io::Seek + Send> ReadSeek for T {}

/// Opens the data track of a `.cue` or `.iso` image
pub fn open_image(path: impl AsRef<Path>) -> Result<IsoFs<Box<dyn ReadSeek>>, Error> {
    let path = path.as_ref();
    let extension = path.extension().map(|e| e.to_ascii_lowercase());

    let data: Box<dyn ReadSeek> = match extension.as_ref().and_then(|e| e.to_str()) {
        Some("iso") => Box::new(io::BufReader::new(File::open(path)?)),
        Some("cue") => Box::new(LogicalBlocks::new(BinCueSource::open(path)?)?),
        _ => return Err(Error::UnsupportedImage(path.to_owned())),
    };

    IsoFs::open(data)
}
