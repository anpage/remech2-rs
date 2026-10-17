use std::{io, path::PathBuf};

use crate::cue::{CueError, CueMode};

pub mod bincue;
pub mod cue;

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
}

pub trait SectorSource {
    fn tracks(&self) -> &[Track];
    /// Read `count` raw 2352-byte sectors starting at `lba` into `out`
    fn read_raw(&self, lba: u32, count: u32, out: &mut [u8]) -> Result<(), Error>;
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
