//! Container header and frame-table parsing.
//!
//! The Smacker file header is 104 bytes (`0x68`), followed by the frame-size
//! table, the frame-type table, the Huffman tree bitstream, and finally the
//! frame chunks back to back. Note that the four declared tree sizes live at
//! offset `0x38` but the tree bitstream itself does **not** follow them — it
//! comes after both frame tables.

use crate::Error;

/// Size of the fixed portion of the file header.
const FIXED_HEADER_SIZE: usize = 0x68;

const SIGNATURE: &[u8; 4] = b"SMK2";

/// Bit 0 of the header flags field: an extra "ring" frame entry exists in
/// the frame tables.
const FLAG_RING_FRAME: u32 = 0x1;
/// Bits we know how to handle at all (Y-interlace / Y-doubling are an
/// unimplemented extension seam; the corpus never sets them).
const KNOWN_FLAGS: u32 = FLAG_RING_FRAME;

/// Number of audio tracks in the container layout.
pub const TRACK_COUNT: usize = 7;

/// Frame-type byte bit: the frame chunk starts with a palette record.
pub const FRAME_TYPE_PALETTE: u8 = 0x01;
/// Frame-type byte bit for audio track `t`: `1 << (1 + t)`.
pub const FRAME_TYPE_TRACK_BASE: u8 = 0x02;

/// A 10 µs time unit, as produced by `SmackOpen`'s frame-rate conversion.
#[derive(Debug, Clone, Copy, PartialEq, Eq, PartialOrd, Ord)]
pub struct FrameTime10us(pub u32);

/// Static video parameters from the file header.
#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub struct VideoInfo {
    /// Frame width in pixels (multiple of 4).
    pub width: u32,
    /// Frame height in pixels (multiple of 4).
    pub height: u32,
    /// Number of frames (excluding any ring frame).
    pub frames: u32,
    /// Per-frame display time in 10 µs units (0 = no delay).
    pub frame_time: FrameTime10us,
}

/// Audio parameters for one track, decoded from the rate/flags word:
/// bit 31 compressed, bit 30 present, bit 29 16-bit,
/// bit 28 stereo, low 24 bits sample rate.
#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub struct AudioInfo {
    /// Sample rate in Hz (0 if the track is absent).
    pub rate: u32,
    /// Bits per sample: 8 or 16.
    pub bits: u8,
    /// Channels: 1 or 2.
    pub channels: u8,
    /// Whether the track is Smacker-DPCM compressed (bit 31).
    pub compressed: bool,
    /// Maximum decoded buffer size from the header's per-track table.
    pub max_unpacked_size: u32,
}

/// The parsed file header plus views into the frame tables and tree
/// bitstream. Borrows the source buffer.
#[derive(Debug, Clone)]
pub struct Header<'a> {
    video: VideoInfo,
    flags: u32,
    /// Entries in the frame-size / frame-type tables (frame count + ring).
    table_entries: usize,
    /// Raw frame-size table: `table_entries` little-endian u32s.
    frame_sizes: &'a [u8],
    /// Frame-type table: `table_entries` bytes.
    frame_types: &'a [u8],
    /// Huffman tree bitstream, located after both tables.
    tree_data: &'a [u8],
    /// Declared tree sizes from offset 0x38: MMAP, MCLR, FULL, TYPE.
    tree_sizes: [u32; 4],
    /// Per-track decoded audio info.
    audio: [Option<AudioInfo>; TRACK_COUNT],
    /// Offset of the first frame chunk.
    chunks_start: usize,
}

impl<'a> Header<'a> {
    /// Parse the header, frame tables, and tree bitstream location from the
    /// start of `src`. Pure and total: every failure is an `Err`, never a
    /// panic.
    ///
    /// # Errors
    /// Fails on short input, a non-`SMK2` signature, non-multiple-of-4
    /// dimensions, unknown flags, or tables/tree data extending past the end
    /// of the input.
    pub fn parse(src: &'a [u8]) -> Result<Self, Error> {
        let h = src.get(..FIXED_HEADER_SIZE).ok_or(Error::UnexpectedEof {
            offset: 0,
            needed: FIXED_HEADER_SIZE,
        })?;

        let mut sig = [0u8; 4];
        sig.copy_from_slice(&h[0x00..0x04]);
        if &sig != SIGNATURE {
            return Err(Error::BadSignature(sig));
        }

        let width = le_u32(h, 0x04);
        let height = le_u32(h, 0x08);
        let frames = le_u32(h, 0x0C);
        let frame_rate = le_u32(h, 0x10).cast_signed();
        let flags = le_u32(h, 0x14);
        let tree_size = le_u32(h, 0x34) as usize;
        let tree_sizes = [
            le_u32(h, 0x38),
            le_u32(h, 0x3C),
            le_u32(h, 0x40),
            le_u32(h, 0x44),
        ];

        if width == 0 || height == 0 || !width.is_multiple_of(4) || !height.is_multiple_of(4) {
            return Err(Error::BadDimensions { width, height });
        }
        if flags & !KNOWN_FLAGS != 0 {
            return Err(Error::UnsupportedFlags(flags));
        }

        let ring = usize::from(flags & FLAG_RING_FRAME != 0);
        let table_entries = (frames as usize)
            .checked_add(ring)
            .ok_or(Error::TruncatedTable)?;

        // Frame-size table at 0x68, then frame-type table, then the tree
        // bitstream of `tree_size` bytes.
        let sizes_start = FIXED_HEADER_SIZE;
        let sizes_len = table_entries.checked_mul(4).ok_or(Error::TruncatedTable)?;
        let types_start = sizes_start
            .checked_add(sizes_len)
            .ok_or(Error::TruncatedTable)?;
        let tree_start = types_start
            .checked_add(table_entries)
            .ok_or(Error::TruncatedTable)?;
        let chunks_start = tree_start
            .checked_add(tree_size)
            .ok_or(Error::TruncatedTable)?;

        let frame_sizes = src
            .get(sizes_start..types_start)
            .ok_or(Error::TruncatedTable)?;
        let frame_types = src
            .get(types_start..tree_start)
            .ok_or(Error::TruncatedTable)?;
        let tree_data = src
            .get(tree_start..chunks_start)
            .ok_or(Error::TruncatedTable)?;

        let mut audio = [None; TRACK_COUNT];
        for (t, slot) in audio.iter_mut().enumerate() {
            let flags_word = le_u32(h, 0x48 + 4 * t);
            // Bit 30: track present (read this way by SmackSoundInTrack).
            if flags_word & 0x4000_0000 == 0 {
                continue;
            }
            let rate = flags_word & 0x00FF_FFFF;
            if rate == 0 {
                continue;
            }
            *slot = Some(AudioInfo {
                rate,
                bits: if flags_word & 0x2000_0000 != 0 { 16 } else { 8 },
                channels: if flags_word & 0x1000_0000 != 0 { 2 } else { 1 },
                compressed: flags_word & 0x8000_0000 != 0,
                max_unpacked_size: le_u32(h, 0x18 + 4 * t),
            });
        }

        Ok(Self {
            video: VideoInfo {
                width,
                height,
                frames,
                frame_time: frame_time_from_rate(frame_rate),
            },
            flags,
            table_entries,
            frame_sizes,
            frame_types,
            tree_data,
            tree_sizes,
            audio,
            chunks_start,
        })
    }

    /// Static video parameters.
    #[must_use]
    pub fn video_info(&self) -> VideoInfo {
        self.video
    }

    /// Raw header flags word.
    #[must_use]
    pub fn flags(&self) -> u32 {
        self.flags
    }

    /// Audio parameters for track `t`, if present (the
    /// `SmackSoundInTrack` analogue).
    #[must_use]
    pub fn audio_track(&self, track: usize) -> Option<AudioInfo> {
        self.audio.get(track).copied().flatten()
    }

    /// The Huffman tree bitstream.
    #[must_use]
    pub fn tree_data(&self) -> &'a [u8] {
        self.tree_data
    }

    /// Declared sizes of the MMAP, MCLR, FULL, TYPE trees (offset 0x38).
    #[must_use]
    pub fn tree_sizes(&self) -> [u32; 4] {
        self.tree_sizes
    }

    /// Frame-size table entry for frame `i`, with the low two flag bits
    /// cleared: the byte length of the frame chunk.
    #[must_use]
    pub fn frame_size(&self, i: usize) -> u32 {
        debug_assert!(i < self.table_entries);
        le_u32(self.frame_sizes, 4 * i) & !3
    }

    /// Whether frame `i` has the keyframe bit set (bit 0 of its size-table
    /// entry). The corpus contains no keyframes.
    #[must_use]
    pub fn is_keyframe(&self, i: usize) -> bool {
        debug_assert!(i < self.table_entries);
        le_u32(self.frame_sizes, 4 * i) & 1 != 0
    }

    /// Frame-type byte for frame `i`.
    #[must_use]
    pub fn frame_type(&self, i: usize) -> u8 {
        debug_assert!(i < self.table_entries);
        self.frame_types[i]
    }

    /// Number of table entries (frame count + ring frame).
    #[must_use]
    pub fn table_entries(&self) -> usize {
        self.table_entries
    }

    /// Byte offset where frame chunks begin.
    #[must_use]
    pub fn chunks_start(&self) -> usize {
        self.chunks_start
    }

    /// The frame chunk (palette record + audio sub-chunks + video
    /// bitstream) for frame `i`, bounds-checked against the source.
    ///
    /// # Errors
    /// Fails when `i` is out of range or the chunk extends past the end of
    /// `src`.
    pub fn frame_chunk(&self, src: &'a [u8], i: usize) -> Result<&'a [u8], Error> {
        let frame = u32::try_from(i).unwrap_or(u32::MAX);
        if i >= self.table_entries {
            return Err(Error::BadFrameIndex(frame));
        }
        let mut offset = self.chunks_start;
        for f in 0..i {
            offset = offset
                .checked_add(self.frame_size(f) as usize)
                .ok_or(Error::TruncatedFrame { frame })?;
        }
        let end = offset
            .checked_add(self.frame_size(i) as usize)
            .ok_or(Error::TruncatedFrame { frame })?;
        src.get(offset..end).ok_or(Error::TruncatedFrame { frame })
    }

    /// The audio sub-chunk for `track` in frame chunk `i`, *including* its
    /// 4-byte self-inclusive size field. Returns `None` when the frame-type
    /// byte does not have the track's bit set.
    ///
    /// # Errors
    /// Fails when `i` or `track` is out of range, the palette record is
    /// malformed, or a preceding sub-chunk's size field is smaller than its
    /// own header or extends past the end of the chunk.
    pub fn audio_subchunk(
        &self,
        chunk: &'a [u8],
        i: usize,
        track: usize,
    ) -> Result<Option<&'a [u8]>, Error> {
        if i >= self.table_entries {
            return Err(Error::BadFrameIndex(u32::try_from(i).unwrap_or(u32::MAX)));
        }
        if track >= TRACK_COUNT {
            return Err(Error::InvalidFrameChunk("track index out of range"));
        }
        let ftype = self.frame_type(i);
        let mut rest = chunk;
        if ftype & FRAME_TYPE_PALETTE != 0 {
            let (_, r) = crate::palette::split_record(rest)?;
            rest = r;
        }
        for t in 0..TRACK_COUNT {
            if ftype & (FRAME_TYPE_TRACK_BASE << t) == 0 {
                continue;
            }
            let header = rest
                .get(..4)
                .ok_or(Error::InvalidFrameChunk("missing audio sub-chunk size"))?;
            let size = le_u32(header, 0) as usize;
            if size < 4 {
                return Err(Error::InvalidFrameChunk("audio sub-chunk size underflows"));
            }
            let sub = rest.get(..size).ok_or(Error::InvalidFrameChunk(
                "audio sub-chunk extends past chunk end",
            ))?;
            if t == track {
                return Ok(Some(sub));
            }
            rest = &rest[size..];
        }
        Ok(None)
    }

    /// The video bitstream of frame chunk `i`: the chunk with its palette
    /// record (if the frame-type palette bit is set) and every audio
    /// sub-chunk (one per set track bit, each prefixed by a 4-byte size
    /// that includes itself) stripped off the front.
    ///
    /// # Errors
    /// Fails when `i` is out of range, the palette record is malformed, or
    /// an audio sub-chunk's size field is smaller than its own header or
    /// extends past the end of the chunk.
    pub fn video_subchunk(&self, chunk: &'a [u8], i: usize) -> Result<&'a [u8], Error> {
        if i >= self.table_entries {
            return Err(Error::BadFrameIndex(u32::try_from(i).unwrap_or(u32::MAX)));
        }
        let ftype = self.frame_type(i);
        let mut rest = chunk;
        if ftype & FRAME_TYPE_PALETTE != 0 {
            let (_, r) = crate::palette::split_record(rest)?;
            rest = r;
        }
        for t in 0..TRACK_COUNT {
            if ftype & (FRAME_TYPE_TRACK_BASE << t) == 0 {
                continue;
            }
            let header = rest
                .get(..4)
                .ok_or(Error::InvalidFrameChunk("missing audio sub-chunk size"))?;
            let size = le_u32(header, 0) as usize;
            if size < 4 {
                return Err(Error::InvalidFrameChunk("audio sub-chunk size underflows"));
            }
            rest = rest.get(size..).ok_or(Error::InvalidFrameChunk(
                "audio sub-chunk extends past chunk end",
            ))?;
        }
        Ok(rest)
    }
}

/// `SmackOpen`'s frame-rate conversion to 10 µs units:
/// negative → `-rate`; positive → `rate * 100`; zero → `0` (no delay).
#[must_use]
pub fn frame_time_from_rate(rate: i32) -> FrameTime10us {
    match rate.cmp(&0) {
        core::cmp::Ordering::Less => FrameTime10us(rate.unsigned_abs()),
        core::cmp::Ordering::Greater => {
            FrameTime10us(u32::try_from(rate).unwrap_or(0).saturating_mul(100))
        }
        core::cmp::Ordering::Equal => FrameTime10us(0),
    }
}

fn le_u32(buf: &[u8], offset: usize) -> u32 {
    u32::from_le_bytes([
        buf[offset],
        buf[offset + 1],
        buf[offset + 2],
        buf[offset + 3],
    ])
}

#[cfg(test)]
mod tests {
    use super::*;

    fn minimal_header(width: u32, height: u32, frames: u32, rate: i32) -> Vec<u8> {
        let mut h = vec![0u8; FIXED_HEADER_SIZE];
        h[0..4].copy_from_slice(b"SMK2");
        h[0x04..0x08].copy_from_slice(&width.to_le_bytes());
        h[0x08..0x0C].copy_from_slice(&height.to_le_bytes());
        h[0x0C..0x10].copy_from_slice(&frames.to_le_bytes());
        h[0x10..0x14].copy_from_slice(&rate.to_le_bytes());
        h
    }

    #[test]
    fn frame_time_conversion() {
        assert_eq!(frame_time_from_rate(-6666), FrameTime10us(6666));
        assert_eq!(frame_time_from_rate(-10000), FrameTime10us(10000));
        assert_eq!(frame_time_from_rate(50), FrameTime10us(5000));
        assert_eq!(frame_time_from_rate(0), FrameTime10us(0));
    }

    #[test]
    fn rejects_bad_signature() {
        let mut h = minimal_header(320, 200, 1, -10000);
        h[0..4].copy_from_slice(b"SMK4");
        assert!(matches!(Header::parse(&h), Err(Error::BadSignature(_))));
    }

    #[test]
    fn rejects_short_input() {
        assert!(matches!(
            Header::parse(&[0u8; 10]),
            Err(Error::UnexpectedEof { .. })
        ));
    }

    #[test]
    fn rejects_non_multiple_of_4() {
        let h = minimal_header(321, 200, 1, -10000);
        assert!(matches!(
            Header::parse(&h),
            Err(Error::BadDimensions {
                width: 321,
                height: 200
            })
        ));
    }

    #[test]
    fn rejects_unsupported_flags() {
        let mut h = minimal_header(320, 200, 1, -10000);
        h[0x14..0x18].copy_from_slice(&2u32.to_le_bytes()); // Y-interlace
        assert!(matches!(Header::parse(&h), Err(Error::UnsupportedFlags(2))));
    }

    #[test]
    fn parses_tables_and_tree_stream() {
        let frames = 2u32;
        let mut h = minimal_header(320, 200, frames, -6666);
        // One present, compressed, 8-bit mono track at 22050 Hz.
        h[0x48..0x4C].copy_from_slice(&(0xC000_0000u32 | 0x5622).to_le_bytes());
        h[0x18..0x1C].copy_from_slice(&4096u32.to_le_bytes());
        // Tree bitstream of 7 bytes declared at 0x34.
        h[0x34..0x38].copy_from_slice(&7u32.to_le_bytes());
        // Frame sizes (with flag bits) and types.
        h.extend_from_slice(&12u32.to_le_bytes()); // frame 0: 12 bytes, no keyframe
        h.extend_from_slice(&(9u32 | 1).to_le_bytes()); // frame 1: 8 bytes, keyframe
        h.extend_from_slice(&[FRAME_TYPE_PALETTE, 0x00]);
        h.extend_from_slice(&[0xAB; 7]); // tree bitstream
        h.extend_from_slice(&[0x11; 12]); // frame 0 chunk
        h.extend_from_slice(&[0x22; 8]); // frame 1 chunk

        let hdr = Header::parse(&h).unwrap();
        assert_eq!(hdr.video_info().width, 320);
        assert_eq!(hdr.video_info().frames, 2);
        assert_eq!(hdr.video_info().frame_time, FrameTime10us(6666));
        assert_eq!(hdr.frame_size(0), 12);
        assert_eq!(hdr.frame_size(1), 8);
        assert!(!hdr.is_keyframe(0));
        assert!(hdr.is_keyframe(1));
        assert_eq!(hdr.frame_type(0), FRAME_TYPE_PALETTE);
        assert_eq!(hdr.tree_data(), &[0xAB; 7]);
        let audio = hdr.audio_track(0).unwrap();
        assert_eq!(audio.rate, 0x5622);
        assert_eq!(audio.bits, 8);
        assert_eq!(audio.channels, 1);
        assert!(audio.compressed);
        assert_eq!(audio.max_unpacked_size, 4096);
        assert!(hdr.audio_track(1).is_none());

        assert_eq!(hdr.frame_chunk(&h, 0).unwrap(), &[0x11; 12]);
        assert_eq!(hdr.frame_chunk(&h, 1).unwrap(), &[0x22; 8]);
        assert!(matches!(
            hdr.frame_chunk(&h, 2),
            Err(Error::BadFrameIndex(2))
        ));
    }

    #[test]
    fn truncated_frame_errors_instead_of_panicking() {
        let mut h = minimal_header(320, 200, 1, -10000);
        h.extend_from_slice(&999u32.to_le_bytes());
        h.extend_from_slice(&[0u8]);
        let hdr = Header::parse(&h).unwrap();
        assert_eq!(
            hdr.frame_chunk(&h, 0),
            Err(Error::TruncatedFrame { frame: 0 })
        );
    }

    #[test]
    fn ring_frame_adds_table_entry() {
        let mut h = minimal_header(320, 200, 1, -10000);
        h[0x14..0x18].copy_from_slice(&1u32.to_le_bytes()); // ring frame
        h.extend_from_slice(&4u32.to_le_bytes());
        h.extend_from_slice(&4u32.to_le_bytes()); // ring entry
        h.extend_from_slice(&[0u8, 0u8]);
        h.extend_from_slice(&[0x33; 8]);
        let hdr = Header::parse(&h).unwrap();
        assert_eq!(hdr.table_entries(), 2);
        assert_eq!(hdr.frame_chunk(&h, 1).unwrap().len(), 4);
    }
}
