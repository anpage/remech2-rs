//! Error type for the Smacker decoder.
//!
//! Everything in this crate fails through this enum; the release profile is
//! `panic = "abort"`, so no code path may panic on malformed input.

use core::fmt;

/// All recoverable failure modes of the decoder.
#[derive(Debug, Clone, PartialEq, Eq)]
pub enum Error {
    /// Input ended before a complete header could be read.
    UnexpectedEof {
        /// Byte offset at which the read was attempted.
        offset: usize,
        /// How many bytes were needed.
        needed: usize,
    },
    /// The 4-byte signature was not `SMK2` (SMK4 is rejected, like
    /// `SmackOpen` in the original DLL).
    BadSignature([u8; 4]),
    /// Header flags other than the ring-frame bit are set. The corpus only
    /// contains `flags == 0`; Y-interlace / Y-doubling are unimplemented
    /// extension seams.
    UnsupportedFlags(u32),
    /// Width or height is not a multiple of 4, or is zero.
    BadDimensions {
        /// Frame width in pixels.
        width: u32,
        /// Frame height in pixels.
        height: u32,
    },
    /// A table (frame sizes, frame types) or the tree bitstream extends
    /// past the end of the input.
    TruncatedTable,
    /// A frame chunk extends past the end of the input.
    TruncatedFrame {
        /// Index of the offending frame.
        frame: u32,
    },
    /// A bitstream read ran out of bits.
    BitstreamExhausted,
    /// A Huffman tree was malformed: over-subscribed code lengths, too
    /// many leaves, excessive depth, a big tree exceeding its declared
    /// size, or a table walk running out of bounds. The static str
    /// identifies the specific failure.
    InvalidHuffmanTree(&'static str),
    /// All four header trees were marked absent; at least one must exist.
    NoTreesPresent,
    /// A palette record was malformed: bad length byte, op stream exhausted
    /// before 256 entries were written, or an op reading/writing out of
    /// range. The static str identifies the specific failure.
    InvalidPaletteRecord(&'static str),
    /// The frame index passed to `seek` is out of range.
    BadFrameIndex(u32),
    /// A frame chunk's internal layout was malformed: an audio sub-chunk
    /// with a size field smaller than its own header or extending past the
    /// end of the chunk. The static str identifies the specific failure.
    InvalidFrameChunk(&'static str),
    /// An audio sub-chunk was malformed: bad size fields, a format flag
    /// mismatch with the track's header entry, a non-divisible sample
    /// count, or a truncated bitstream. The static str identifies the
    /// specific failure.
    InvalidAudioPacket(&'static str),
    /// The caller-supplied output buffer is too small for the requested
    /// frame, pitch, and flip combination.
    OutputBufferTooSmall {
        /// Bytes required.
        needed: usize,
        /// Bytes supplied.
        got: usize,
    },
}

impl fmt::Display for Error {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        match self {
            Self::UnexpectedEof { offset, needed } => {
                write!(
                    f,
                    "unexpected end of input at offset {offset:#x} (needed {needed} bytes)"
                )
            }
            Self::BadSignature(sig) => {
                write!(
                    f,
                    "bad signature {} (expected SMK2)",
                    core::str::from_utf8(sig).unwrap_or("????")
                )
            }
            Self::UnsupportedFlags(flags) => write!(f, "unsupported header flags {flags:#x}"),
            Self::BadDimensions { width, height } => {
                write!(
                    f,
                    "bad frame dimensions {width}x{height} (must be multiples of 4)"
                )
            }
            Self::TruncatedTable => write!(f, "header tables extend past end of input"),
            Self::TruncatedFrame { frame } => write!(f, "frame {frame} extends past end of input"),
            Self::BitstreamExhausted => write!(f, "bitstream exhausted"),
            Self::InvalidHuffmanTree(why) => write!(f, "invalid Huffman tree: {why}"),
            Self::NoTreesPresent => write!(f, "all four header trees are absent"),
            Self::InvalidPaletteRecord(why) => write!(f, "invalid palette record: {why}"),
            Self::BadFrameIndex(frame) => write!(f, "frame index {frame} out of range"),
            Self::InvalidFrameChunk(why) => write!(f, "invalid frame chunk: {why}"),
            Self::InvalidAudioPacket(why) => write!(f, "invalid audio packet: {why}"),
            Self::OutputBufferTooSmall { needed, got } => {
                write!(f, "output buffer too small: need {needed} bytes, got {got}")
            }
        }
    }
}

#[cfg(feature = "std")]
impl std::error::Error for Error {}
