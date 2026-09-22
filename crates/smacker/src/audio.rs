//! Audio decoding: Smacker DPCM and the uncompressed copy path.
//!
//! A frame chunk carries one audio sub-chunk per track whose frame-type bit
//! is set: a 4-byte self-inclusive size, then — for compressed tracks — a
//! 4-byte uncompressed size (`unp_size`) followed by an LSB-first DPCM
//! bitstream. Uncompressed tracks store raw PCM directly after the size.
//!
//! The compressed bitstream opens with three flag bits: a present bit (clear
//! → the packet carries no audio), a stereo bit, and a 16-bit bit; the
//! latter two must match the track's header entry. Then come
//! `1 << (bits + stereo)` small Huffman trees — one per byte-lane per
//! channel — each surrounded by a discarded bit, with one more discarded
//! bit after the last tree. The initial predictors follow: one sample per
//! channel, read in *descending* channel order, 16-bit predictors
//! big-endian. They are emitted as the first samples (ascending channel
//! order); every later sample adds a decoded delta to its channel's
//! predictor with wrapping arithmetic (no clipping).
//!
//! Tree-to-lane mapping for interleaved sample `i`: channel `i & stereo`;
//! 16-bit samples use trees `2 * channel` (low byte) and `2 * channel + 1`
//! (high byte), 8-bit samples use tree `channel`.

use crate::huffman::{ByteTree, decode_small_tree};
use crate::{AudioInfo, BitReader, Error};

/// Result of decoding one audio sub-chunk.
#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub struct DecodedAudio {
    /// Bytes of PCM written to the destination. For a packet whose present
    /// bit is clear this is the declared `unp_size` even though nothing was
    /// written — `SmackGetTrackData` reports it the same way, and the
    /// destination keeps its previous contents.
    pub bytes_written: usize,
    /// Bytes of the sub-chunk consumed, counting from its size field.
    /// Sub-chunks are dword-aligned, so this normally lands 0–4 bytes short
    /// of the sub-chunk end.
    pub bytes_consumed: usize,
}

/// Per-track audio decoder. Stateless across packets: each packet carries
/// its own trees and initial predictors.
#[derive(Debug, Clone)]
pub struct AudioDecoder {
    info: AudioInfo,
}

impl AudioDecoder {
    /// A decoder for the given track parameters.
    ///
    /// # Errors
    /// Fails when the track has other than 1–2 channels or 8–16 bits per
    /// sample.
    pub fn new(info: AudioInfo) -> Result<Self, Error> {
        if info.channels == 0 || info.channels > 2 {
            return Err(Error::InvalidAudioPacket("channels must be 1 or 2"));
        }
        if info.bits != 8 && info.bits != 16 {
            return Err(Error::InvalidAudioPacket("bits must be 8 or 16"));
        }
        Ok(Self { info })
    }

    /// Decode one audio sub-chunk (starting at its 4-byte size field) into
    /// `dst`. Uncompressed tracks are copied verbatim minus the size field.
    ///
    /// # Errors
    /// Fails on malformed size fields, a format-flag mismatch with the
    /// track's header entry, a sample count that does not divide evenly, an
    /// exhausted bitstream, or a `dst` smaller than the declared
    /// uncompressed size.
    pub fn decode(&mut self, subchunk: &[u8], dst: &mut [u8]) -> Result<DecodedAudio, Error> {
        let header = subchunk
            .get(..4)
            .ok_or(Error::InvalidAudioPacket("missing sub-chunk size"))?;
        let size = u32::from_le_bytes([header[0], header[1], header[2], header[3]]) as usize;
        if size < 4 {
            return Err(Error::InvalidAudioPacket("sub-chunk size underflows"));
        }
        let packet = subchunk
            .get(..size)
            .ok_or(Error::InvalidAudioPacket("sub-chunk extends past input"))?
            .get(4..)
            .ok_or(Error::InvalidAudioPacket("sub-chunk size underflows"))?;

        if !self.info.compressed {
            // Raw PCM: everything after the size field.
            if dst.len() < packet.len() {
                return Err(Error::OutputBufferTooSmall {
                    needed: packet.len(),
                    got: dst.len(),
                });
            }
            dst[..packet.len()].copy_from_slice(packet);
            return Ok(DecodedAudio {
                bytes_written: packet.len(),
                bytes_consumed: 4 + packet.len(),
            });
        }

        let (unp_size, stream_bytes) = self.decode_dpcm(packet, dst)?;
        Ok(DecodedAudio {
            bytes_written: unp_size,
            bytes_consumed: 4 + 4 + stream_bytes,
        })
    }

    /// Decode the DPCM packet (`unp_size` dword plus bitstream), returning
    /// the declared uncompressed size and the bitstream bytes consumed
    /// (rounded up to a whole byte).
    fn decode_dpcm(&mut self, packet: &[u8], dst: &mut [u8]) -> Result<(usize, usize), Error> {
        let head = packet
            .get(..4)
            .ok_or(Error::InvalidAudioPacket("missing uncompressed size"))?;
        let unp_size = u32::from_le_bytes([head[0], head[1], head[2], head[3]]);
        if unp_size > 0x100_0000 {
            return Err(Error::InvalidAudioPacket("uncompressed size too large"));
        }
        if packet.len() <= 4 {
            return Err(Error::InvalidAudioPacket("empty bitstream"));
        }
        let unp_size = unp_size as usize;
        if dst.len() < unp_size {
            return Err(Error::OutputBufferTooSmall {
                needed: unp_size,
                got: dst.len(),
            });
        }

        let mut br = BitReader::new(&packet[4..]);
        if br.read_bit()? == 0 {
            // No audio this frame; the destination keeps its previous
            // contents but the declared size is still reported.
            return Ok((unp_size, br.bytes_consumed()));
        }
        let stereo = br.read_bit()? != 0;
        let bits16 = br.read_bit()? != 0;
        if stereo != (self.info.channels == 2) || bits16 != (self.info.bits == 16) {
            return Err(Error::InvalidAudioPacket("format flags mismatch header"));
        }

        let channels = usize::from(self.info.channels);
        let bytes_per_sample = usize::from(bits16) + 1;
        if !unp_size.is_multiple_of(channels * bytes_per_sample) {
            return Err(Error::InvalidAudioPacket(
                "sample count does not divide evenly",
            ));
        }

        // One tree per byte-lane per channel, each surrounded by discarded
        // bits (there is no presence flag, unlike the video header trees).
        let tree_count = 1usize << (usize::from(bits16) + usize::from(stereo));
        let mut trees = alloc::vec::Vec::with_capacity(tree_count);
        for _ in 0..tree_count {
            br.skip_bits(1)?;
            trees.push(decode_small_tree(&mut br)?);
            br.skip_bits(1)?;
        }

        if br.bits_remaining() < channels * bytes_per_sample * 8 {
            return Err(Error::BitstreamExhausted);
        }

        if bits16 {
            decode_16bit(&mut br, &trees, channels, unp_size, dst)?;
        } else {
            decode_8bit(&mut br, &trees, channels, unp_size, dst)?;
        }
        Ok((unp_size, br.bytes_consumed()))
    }
}

/// 8-bit DPCM: one tree per channel, 8-bit predictors read in descending
/// channel order, wrapping deltas.
fn decode_8bit(
    br: &mut BitReader<'_>,
    trees: &[ByteTree],
    channels: usize,
    unp_size: usize,
    dst: &mut [u8],
) -> Result<(), Error> {
    let mut predictors = [0u8; 2];
    for ch in (0..channels).rev() {
        predictors[ch] = br.read_bits(8)? as u8;
    }
    dst[..channels].copy_from_slice(&predictors[..channels]);
    for (i, out) in dst[..unp_size].iter_mut().enumerate().skip(channels) {
        let ch = i & (channels - 1);
        let delta = trees[ch].decode(br)?;
        predictors[ch] = predictors[ch].wrapping_add(delta);
        *out = predictors[ch];
    }
    Ok(())
}

/// 16-bit DPCM: two trees per channel (low/high byte), big-endian
/// predictors read in descending channel order, wrapping 16-bit deltas
/// emitted as little-endian signed samples.
fn decode_16bit(
    br: &mut BitReader<'_>,
    trees: &[ByteTree],
    channels: usize,
    unp_size: usize,
    dst: &mut [u8],
) -> Result<(), Error> {
    let mut predictors = [0u16; 2];
    for ch in (0..channels).rev() {
        let hi = br.read_bits(8)?;
        let lo = br.read_bits(8)?;
        predictors[ch] = ((hi << 8) | lo) as u16;
    }
    let samples = unp_size / 2;
    for ch in 0..channels {
        dst[ch * 2..ch * 2 + 2].copy_from_slice(&predictors[ch].to_le_bytes());
    }
    for i in channels..samples {
        let ch = i & (channels - 1);
        let lo = u32::from(trees[2 * ch].decode(br)?);
        let hi = u32::from(trees[2 * ch + 1].decode(br)?);
        let delta = (lo | (hi << 8)) as u16;
        predictors[ch] = predictors[ch].wrapping_add(delta);
        dst[i * 2..i * 2 + 2].copy_from_slice(&predictors[ch].to_le_bytes());
    }
    Ok(())
}

#[cfg(test)]
mod tests {
    use super::*;
    use alloc::vec::Vec;

    /// LSB-first bit writer for hand-crafted streams.
    struct BitWriter {
        bytes: Vec<u8>,
        pos: usize,
    }

    impl BitWriter {
        fn new() -> Self {
            Self {
                bytes: Vec::new(),
                pos: 0,
            }
        }

        fn push_bit(&mut self, bit: bool) {
            if self.pos / 8 == self.bytes.len() {
                self.bytes.push(0);
            }
            if bit {
                let (byte, shift) = (self.pos / 8, self.pos % 8);
                self.bytes[byte] |= 1 << shift;
            }
            self.pos += 1;
        }

        fn push_bits(&mut self, value: u32, n: u32) {
            for k in 0..n {
                self.push_bit(value & (1 << k) != 0);
            }
        }

        /// A single-leaf small tree: leaf bit 0 plus 8 value bits.
        fn push_constant_tree(&mut self, value: u8) {
            self.push_bit(false);
            self.push_bits(u32::from(value), 8);
        }

        /// A two-leaf small tree: code 0 → `a`, code 1 → `b`.
        fn push_two_leaf_tree(&mut self, a: u8, b: u8) {
            self.push_bit(true); // internal node
            self.push_constant_tree(a);
            self.push_constant_tree(b);
        }

        fn finish(self) -> Vec<u8> {
            self.bytes
        }
    }

    fn info(bits: u8, channels: u8) -> AudioInfo {
        AudioInfo {
            rate: 22050,
            bits,
            channels,
            compressed: true,
            max_unpacked_size: 0,
        }
    }

    /// Wrap a bitstream into a sub-chunk: size, `unp_size`, stream.
    fn subchunk(unp_size: u32, stream: &[u8]) -> Vec<u8> {
        let size = 4 + 4 + stream.len() as u32;
        let mut v = Vec::new();
        v.extend_from_slice(&size.to_le_bytes());
        v.extend_from_slice(&unp_size.to_le_bytes());
        v.extend_from_slice(stream);
        v
    }

    #[test]
    fn decodes_8bit_mono() {
        let mut w = BitWriter::new();
        w.push_bit(true); // present
        w.push_bit(false); // mono
        w.push_bit(false); // 8-bit
        w.push_bit(false); // skip
        w.push_two_leaf_tree(0x01, 0xFF); // deltas +1 / -1
        w.push_bit(false); // skip
        w.push_bits(100, 8); // predictor
        w.push_bit(false); // +1
        w.push_bit(true); // -1
        w.push_bit(false); // +1
        let sub = subchunk(4, &w.finish());

        let mut dec = AudioDecoder::new(info(8, 1)).unwrap();
        let mut dst = [0u8; 4];
        let r = dec.decode(&sub, &mut dst).unwrap();
        assert_eq!(r.bytes_written, 4);
        assert_eq!(r.bytes_consumed, sub.len());
        assert_eq!(dst, [100, 101, 100, 101]);
    }

    #[test]
    fn decodes_16bit_stereo() {
        let mut w = BitWriter::new();
        w.push_bit(true); // present
        w.push_bit(true); // stereo
        w.push_bit(true); // 16-bit
        // Trees: ch0 lo +1, ch0 hi 0, ch1 lo 0xFF, ch1 hi 0xFF (delta -1).
        for value in [0x01, 0x00, 0xFF, 0xFF] {
            w.push_bit(false); // skip
            w.push_constant_tree(value);
            w.push_bit(false); // skip
        }
        // Predictors, descending channel order, big-endian: ch1 = 1000, ch0 = 500.
        w.push_bits(0x03, 8);
        w.push_bits(0xE8, 8);
        w.push_bits(0x01, 8);
        w.push_bits(0xF4, 8);
        // Constant trees consume no sample bits.
        let sub = subchunk(16, &w.finish());

        let mut dec = AudioDecoder::new(info(16, 2)).unwrap();
        let mut dst = [0u8; 16];
        let r = dec.decode(&sub, &mut dst).unwrap();
        assert_eq!(r.bytes_written, 16);
        assert_eq!(r.bytes_consumed, sub.len());
        let samples: Vec<i16> = dst
            .as_chunks::<2>()
            .0
            .iter()
            .map(|&c| i16::from_le_bytes(c))
            .collect();
        assert_eq!(samples, [500, 1000, 501, 999, 502, 998, 503, 997]);
    }

    #[test]
    fn absent_audio_reports_size_without_writing() {
        let mut w = BitWriter::new();
        w.push_bit(false); // not present
        let sub = subchunk(12, &w.finish());
        let mut dec = AudioDecoder::new(info(8, 1)).unwrap();
        let mut dst = [0xAA; 12];
        let r = dec.decode(&sub, &mut dst).unwrap();
        assert_eq!(r.bytes_written, 12);
        assert_eq!(r.bytes_consumed, 9); // size + unp_size + 1 byte
        assert_eq!(dst, [0xAA; 12]);
    }

    #[test]
    fn format_flag_mismatch_errors() {
        let mut w = BitWriter::new();
        w.push_bit(true);
        w.push_bit(true); // stereo, but the track is mono
        w.push_bit(false);
        let sub = subchunk(4, &w.finish());
        let mut dec = AudioDecoder::new(info(8, 1)).unwrap();
        let mut dst = [0u8; 4];
        assert_eq!(
            dec.decode(&sub, &mut dst),
            Err(Error::InvalidAudioPacket("format flags mismatch header"))
        );
    }

    #[test]
    fn uncompressed_track_is_copied() {
        let mut i = info(8, 1);
        i.compressed = false;
        let mut dec = AudioDecoder::new(i).unwrap();
        let pcm = [1u8, 2, 3, 4, 5];
        let mut sub = Vec::new();
        sub.extend_from_slice(&(4 + pcm.len() as u32).to_le_bytes());
        sub.extend_from_slice(&pcm);
        let mut dst = [0u8; 8];
        let r = dec.decode(&sub, &mut dst).unwrap();
        assert_eq!(r.bytes_written, 5);
        assert_eq!(r.bytes_consumed, sub.len());
        assert_eq!(&dst[..5], &pcm);
    }

    #[test]
    fn rejects_bad_setup_and_sizes() {
        assert!(AudioDecoder::new(info(8, 3)).is_err());
        assert!(AudioDecoder::new(info(24, 1)).is_err());

        let mut dec = AudioDecoder::new(info(8, 1)).unwrap();
        let mut dst = [0u8; 4];
        // unp_size not divisible by channels * bytes.
        let sub = subchunk(3, &[0x07, 0x00]);
        assert!(matches!(
            dec.decode(&sub, &mut dst),
            Err(Error::InvalidAudioPacket(_))
        ));
        // dst too small.
        let sub = subchunk(100, &[0x07, 0x00]);
        assert!(matches!(
            dec.decode(&sub, &mut dst),
            Err(Error::OutputBufferTooSmall { .. })
        ));
        // Truncated sub-chunk.
        assert!(matches!(
            dec.decode(&[2, 0], &mut dst),
            Err(Error::InvalidAudioPacket(_))
        ));
    }
}
