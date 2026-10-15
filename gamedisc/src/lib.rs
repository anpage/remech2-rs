pub mod cue;

#[derive(Debug, thiserror::Error)]
pub enum Error {}

pub trait SectorSource {
    fn tracks(&self) -> &[Track];
    /// Read `count` raw 2352-byte sectors starting at `lba` into `out`
    fn read_raw(&self, lba: u32, count: u32, out: &mut [u8]) -> Result<(), Error>;
}

pub enum TrackKind {
    Audio,
    Mode1,
    Mode2Form1,
}

pub struct Track {
    /// Starts at 1
    pub number: u8,
    pub kind: TrackKind,
    /// The LBA of INDEX 01
    pub start_lba: u32,
    /// The number of sectors from `start_lba` up to the next track's `start_lba` or the lead-out
    pub sectors: u32,
}
