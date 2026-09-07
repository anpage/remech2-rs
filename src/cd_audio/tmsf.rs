use std::time::Duration;

/// A position on the CD, matching the game's `CdAudioPosition` struct.
/// Frames are 1/75 second, per the Red Book standard.
#[repr(C)]
#[derive(Clone, Copy, Debug, Default, Eq, PartialEq)]
pub struct CdAudioPosition {
    pub track: u32,
    pub minute: u32,
    pub second: u32,
    pub frame: u32,
}

impl CdAudioPosition {
    pub const FRAMES_PER_SECOND: u32 = 75;
    pub const FRAMES_PER_MINUTE: u32 = 60 * Self::FRAMES_PER_SECOND;

    /// Position within the track, ignoring the track number.
    pub fn track_offset(&self) -> Duration {
        let frames = self.frame
            + self.second * Self::FRAMES_PER_SECOND
            + self.minute * Self::FRAMES_PER_MINUTE;
        Duration::from_secs_f64(frames as f64 / Self::FRAMES_PER_SECOND as f64)
    }

    /// Converts a track number and offset into a `CdAudioPosition`.
    pub fn from_track_offset(track: u32, offset: Duration) -> Self {
        let frames = (offset.as_secs_f64() * Self::FRAMES_PER_SECOND as f64) as u32;
        Self {
            track,
            minute: frames / Self::FRAMES_PER_MINUTE,
            second: (frames / Self::FRAMES_PER_SECOND) % 60,
            frame: frames % Self::FRAMES_PER_SECOND,
        }
    }
}

/// Unpack from the game's TMSF format (as used by MCI).
impl From<u32> for CdAudioPosition {
    fn from(tmsf: u32) -> Self {
        Self {
            track: tmsf & 0xFF,
            minute: (tmsf >> 8) & 0xFF,
            second: (tmsf >> 16) & 0xFF,
            frame: (tmsf >> 24) & 0xFF,
        }
    }
}

/// Pack into the game's TMSF format.
impl From<CdAudioPosition> for u32 {
    fn from(position: CdAudioPosition) -> u32 {
        (position.track & 0xFF)
            | ((position.minute & 0xFF) << 8)
            | ((position.second & 0xFF) << 16)
            | ((position.frame & 0xFF) << 24)
    }
}
