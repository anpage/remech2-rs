//! Video frame decoding: the block-command main loop.
//!
//! A frame is a grid of 4×4 pixel blocks, decoded in raster order from an
//! LSB-first bitstream of block commands. Each command starts with a symbol
//! from the TYPE tree; bits 2–7 of that symbol index a run-length table,
//! bits 0–1 select the block kind, and (for FILL) bits 8+ carry a palette
//! index:
//!
//! - **MONO** — a two-color block: a color pair from the MCLR tree and a
//!   16-bit bitmap from the MMAP tree, one nibble per row, LSB first. A set
//!   bit selects the high color, a clear bit the low one.
//! - **FULL** — every pixel coded: per row, two 16-bit values from the FULL
//!   tree, assembled into one little-endian dword with the first value in
//!   the high half. The first therefore lands on pixels 2–3 and the second
//!   on pixels 0–1, low byte leftmost within each pair. SMK2 only has this
//!   mode; the SMK4 v4 modes are an extension seam.
//! - **SKIP** — the block keeps its previous contents.
//! - **FILL** — the block is filled solidly with the command's color.
//!
//! SKIP means the decoder must keep the previous frame around: decoding
//! goes into an internal frame buffer that persists across frames, and the
//! result is then blitted to the caller's destination with an explicit
//! pitch and optional vertical flip (the `SmackToBuffer` semantics).
//!
//! Runs are clamped to the remaining block count — an over-long final run
//! is truncated, not an error. Frame chunks are dword-aligned, so the
//! bitstream normally ends 0–4 bytes short of the chunk end; the trailing
//! padding is not consumed and not an error.

use crate::{BitReader, Error, HuffmanTables, VideoInfo};
use alloc::vec::Vec;

/// Run lengths for TYPE-symbol bits 2–7: indices 0–58 map to runs of 1–59,
/// indices 59–63 to 128, 256, 512, 1024, 2048.
const RUN_LENGTHS: [u16; 64] = {
    let mut table = [0u16; 64];
    let mut i = 0;
    while i <= 58 {
        table[i] = i as u16 + 1;
        i += 1;
    }
    table[59] = 128;
    table[60] = 256;
    table[61] = 512;
    table[62] = 1024;
    table[63] = 2048;
    table
};

/// Block kinds, from bits 0–1 of a TYPE symbol.
const BLOCK_MONO: u32 = 0;
const BLOCK_FULL: u32 = 1;
const BLOCK_SKIP: u32 = 2;
const BLOCK_FILL: u32 = 3;

/// Stateful video decoder: owns the Huffman tables (their recency caches
/// mutate as symbols decode) and the previous-frame buffer SKIP blocks
/// depend on.
#[derive(Debug, Clone)]
pub struct VideoDecoder {
    tables: HuffmanTables,
    /// The last decoded frame, `width * height` palette indices.
    frame: Vec<u8>,
    width: usize,
    height: usize,
}

impl VideoDecoder {
    /// A decoder for a stream with the given video parameters and Huffman
    /// tables. The frame buffer starts zeroed (palette index 0 everywhere).
    #[must_use]
    pub fn new(info: &VideoInfo, tables: HuffmanTables) -> Self {
        let width = info.width as usize;
        let height = info.height as usize;
        Self {
            tables,
            frame: alloc::vec![0; width * height],
            width,
            height,
        }
    }

    /// The most recently decoded frame as `width * height` palette indices,
    /// row-major with no padding.
    #[must_use]
    pub fn frame(&self) -> &[u8] {
        &self.frame
    }

    /// Zero the frame buffer, putting the decoder back in the state it had
    /// before the first frame. Needed when replaying a stream from the
    /// start, since skipped blocks would otherwise carry pixels over from
    /// the previous playthrough.
    pub fn reset(&mut self) {
        self.frame.fill(0);
    }

    /// Decode one frame from its video bitstream into the internal frame
    /// buffer. Returns the number of bytes consumed (rounded up to a whole
    /// byte); the chunk's dword-alignment padding of 0–4 bytes is expected
    /// to remain.
    ///
    /// # Errors
    /// Fails when the bitstream runs out before all blocks are decoded or a
    /// Huffman table walk fails.
    pub fn decode(&mut self, bitstream: &[u8]) -> Result<usize, Error> {
        let bw = self.width / 4;
        let blocks = bw * (self.height / 4);

        self.tables.mmap.reset_cache();
        self.tables.mclr.reset_cache();
        self.tables.full.reset_cache();
        self.tables.type_.reset_cache();

        let mut br = BitReader::new(bitstream);
        let mut block = 0usize;
        while block < blocks {
            let ty = self.tables.type_.decode_symbol(&mut br)?;
            let run = usize::from(RUN_LENGTHS[((ty >> 2) & 0x3F) as usize]).min(blocks - block);
            match ty & 3 {
                BLOCK_MONO => {
                    for _ in 0..run {
                        self.decode_mono(&mut br, block, bw)?;
                        block += 1;
                    }
                }
                BLOCK_FULL => {
                    for _ in 0..run {
                        self.decode_full(&mut br, block, bw)?;
                        block += 1;
                    }
                }
                BLOCK_SKIP => block += run,
                BLOCK_FILL => {
                    let color = (ty >> 8) as u8;
                    for _ in 0..run {
                        self.fill_block(block, bw, color);
                        block += 1;
                    }
                }
                _ => unreachable!("block kind is two bits"),
            }
        }
        Ok(br.bytes_consumed())
    }

    /// Blit the internal frame into `dst` with the given row pitch. When
    /// `flip` is set the frame is written bottom-up (row 0 lands on the
    /// last row of the destination), matching `SmackToBuffer`'s
    /// vertical-flip flag.
    ///
    /// # Errors
    /// Fails when `dst` is smaller than `pitch * (height - 1) + width`.
    pub fn blit_to(&self, dst: &mut [u8], pitch: usize, flip: bool) -> Result<(), Error> {
        let needed = pitch
            .checked_mul(self.height.saturating_sub(1))
            .and_then(|n| n.checked_add(self.width))
            .ok_or(Error::OutputBufferTooSmall {
                needed: usize::MAX,
                got: dst.len(),
            })?;
        if dst.len() < needed {
            return Err(Error::OutputBufferTooSmall {
                needed,
                got: dst.len(),
            });
        }
        for row in 0..self.height {
            let dst_row = if flip { self.height - 1 - row } else { row };
            let start = dst_row * pitch;
            dst[start..start + self.width]
                .copy_from_slice(&self.frame[row * self.width..(row + 1) * self.width]);
        }
        Ok(())
    }

    /// Decode one frame and blit it to `dst` in one call. Returns the
    /// number of bitstream bytes consumed, like [`Self::decode`].
    ///
    /// # Errors
    /// Fails on a decode error or when `dst` is too small.
    pub fn decode_frame(
        &mut self,
        bitstream: &[u8],
        dst: &mut [u8],
        pitch: usize,
        flip: bool,
    ) -> Result<usize, Error> {
        let consumed = self.decode(bitstream)?;
        self.blit_to(dst, pitch, flip)?;
        Ok(consumed)
    }

    /// Byte offset of block `b`'s top-left pixel in the frame buffer.
    const fn block_offset(&self, b: usize, bw: usize) -> usize {
        (b / bw) * 4 * self.width + (b % bw) * 4
    }

    /// MONO block: a color pair from MCLR and a 16-bit bitmap from MMAP,
    /// one nibble per row, LSB first; a set bit picks the high color.
    fn decode_mono(&mut self, br: &mut BitReader<'_>, b: usize, bw: usize) -> Result<(), Error> {
        let clr = self.tables.mclr.decode_symbol(br)?;
        let hi = (clr >> 8) as u8;
        let lo = clr as u8;
        let map = self.tables.mmap.decode_symbol(br)?;
        let base = self.block_offset(b, bw);
        for row in 0..4 {
            let nibble = map >> (4 * row);
            let line = &mut self.frame[base + row * self.width..base + row * self.width + 4];
            for (j, px) in line.iter_mut().enumerate() {
                *px = if nibble & (1 << j) != 0 { hi } else { lo };
            }
        }
        Ok(())
    }

    /// FULL block (SMK2 mode): per row, two 16-bit values from the FULL
    /// tree. The pair is stored as one little-endian dword with the *first*
    /// value in its high half, so the first covers the block's right-hand
    /// pixels 2–3 and the second its left-hand pixels 0–1. Within each
    /// value the low byte is the left pixel of its pair.
    fn decode_full(&mut self, br: &mut BitReader<'_>, b: usize, bw: usize) -> Result<(), Error> {
        let base = self.block_offset(b, bw);
        for row in 0..4 {
            let right = self.tables.full.decode_symbol(br)?;
            let left = self.tables.full.decode_symbol(br)?;
            let line = &mut self.frame[base + row * self.width..base + row * self.width + 4];
            line[0] = left as u8;
            line[1] = (left >> 8) as u8;
            line[2] = right as u8;
            line[3] = (right >> 8) as u8;
        }
        Ok(())
    }

    /// FILL block: solid color.
    fn fill_block(&mut self, b: usize, bw: usize, color: u8) {
        let base = self.block_offset(b, bw);
        for row in 0..4 {
            self.frame[base + row * self.width..base + row * self.width + 4].fill(color);
        }
    }
}

#[cfg(test)]
mod tests {
    use super::*;
    use crate::HeaderTree;
    use alloc::vec;
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

        fn finish(self) -> Vec<u8> {
            self.bytes
        }
    }

    /// A tree that yields `value` without consuming any bits.
    fn constant_tree(value: u32) -> HeaderTree {
        HeaderTree::test_tree(vec![value, 0, 0, 0], [1, 2, 3])
    }

    /// A four-leaf tree: bit pattern 00 → a, 01 → b, 10 → c, 11 → d
    /// (first bit is the root).
    fn four_leaf_tree(a: u32, b: u32, c: u32, d: u32) -> HeaderTree {
        // Preorder: root, left node, a, b, right node, c, d. Internal
        // entries store the entry count of their left subtree.
        const NODE: u32 = 0x8000_0000;
        HeaderTree::test_tree(
            vec![NODE | 3, NODE | 1, a, b, NODE | 1, c, d, 0, 0, 0],
            [7, 8, 9],
        )
    }

    fn tables_with_type(type_tree: HeaderTree) -> HuffmanTables {
        HuffmanTables {
            mmap: constant_tree(0),
            mclr: constant_tree(0),
            full: constant_tree(0),
            type_: type_tree,
        }
    }

    fn decoder(tables: HuffmanTables, width: u32, height: u32) -> VideoDecoder {
        let info = VideoInfo {
            width,
            height,
            frames: 1,
            frame_time: crate::FrameTime10us(0),
        };
        VideoDecoder::new(&info, tables)
    }

    #[test]
    fn run_length_table() {
        assert_eq!(RUN_LENGTHS[0], 1);
        assert_eq!(RUN_LENGTHS[58], 59);
        assert_eq!(RUN_LENGTHS[59], 128);
        assert_eq!(RUN_LENGTHS[60], 256);
        assert_eq!(RUN_LENGTHS[61], 512);
        assert_eq!(RUN_LENGTHS[62], 1024);
        assert_eq!(RUN_LENGTHS[63], 2048);
    }

    /// 8×8 frame (2×2 blocks): FILL color 5, SKIP, MONO, FULL.
    #[test]
    fn decodes_all_four_block_kinds() {
        let fill_5 = (5 << 8) | BLOCK_FILL;
        let type_tree = four_leaf_tree(fill_5, BLOCK_SKIP, BLOCK_MONO, BLOCK_FULL);
        let mut tables = tables_with_type(type_tree);
        tables.mclr = constant_tree(0x0A0B); // hi = 0x0A, lo = 0x0B
        tables.mmap = constant_tree(0x0005); // row 0: hi, lo, hi, lo; rows 1–3: lo
        tables.full = constant_tree(0x1122); // px: 0x22, 0x11, 0x22, 0x11

        let mut w = BitWriter::new();
        w.push_bit(false);
        w.push_bit(false); // FILL
        w.push_bit(false);
        w.push_bit(true); // SKIP
        w.push_bit(true);
        w.push_bit(false); // MONO
        w.push_bit(true);
        w.push_bit(true); // FULL
        let stream = w.finish();

        let mut dec = decoder(tables, 8, 8);
        let consumed = dec.decode(&stream).unwrap();
        assert_eq!(consumed, 1);
        let f = dec.frame();

        // Block 0: solid 5.
        assert!(f[..4].iter().all(|&p| p == 5));
        assert!(f[8..12].iter().all(|&p| p == 5));
        // Block 1 (offset x=4): skipped, stays 0.
        assert!(f[4..8].iter().all(|&p| p == 0));
        // Block 2 (row 4, x=0): MONO.
        assert_eq!(&f[4 * 8..4 * 8 + 4], &[0x0A, 0x0B, 0x0A, 0x0B]);
        assert!(f[5 * 8..5 * 8 + 4].iter().all(|&p| p == 0x0B));
        // Block 3 (row 4, x=4): FULL.
        assert_eq!(&f[4 * 8 + 4..4 * 8 + 8], &[0x22, 0x11, 0x22, 0x11]);
        assert_eq!(&f[7 * 8 + 4..7 * 8 + 8], &[0x22, 0x11, 0x22, 0x11]);
    }

    /// An over-long run is clamped to the remaining block count. A constant
    /// TYPE tree consumes no bits, so even an empty stream decodes.
    #[test]
    fn overlong_run_is_clamped() {
        let fill_7_run_2048 = (7 << 8) | (63 << 2) | BLOCK_FILL;
        let tables = tables_with_type(constant_tree(fill_7_run_2048));
        let mut dec = decoder(tables, 8, 8);
        assert_eq!(dec.decode(&[]).unwrap(), 0);
        assert!(dec.frame().iter().all(|&p| p == 7));
    }

    /// SKIP preserves the previous frame's contents across decodes.
    #[test]
    fn skip_preserves_previous_frame() {
        let type_tree = four_leaf_tree((9 << 8) | BLOCK_FILL, BLOCK_SKIP, BLOCK_SKIP, BLOCK_SKIP);
        let tables = tables_with_type(type_tree);
        let mut dec = decoder(tables, 4, 4);

        // Frame 1: fill the single block with 9.
        let mut w = BitWriter::new();
        w.push_bit(false);
        w.push_bit(false);
        dec.decode(&w.finish()).unwrap();
        assert!(dec.frame().iter().all(|&p| p == 9));

        // Frame 2: skip it — content persists.
        let mut w = BitWriter::new();
        w.push_bit(false);
        w.push_bit(true);
        dec.decode(&w.finish()).unwrap();
        assert!(dec.frame().iter().all(|&p| p == 9));
    }

    #[test]
    fn exhausted_type_stream_errors() {
        // A real (bit-consuming) TYPE tree against an empty stream.
        let tables = tables_with_type(four_leaf_tree(0, 1, 2, 3));
        let mut dec = decoder(tables, 4, 4);
        assert_eq!(dec.decode(&[]), Err(Error::BitstreamExhausted));
    }

    #[test]
    fn blit_honors_pitch_and_flip() {
        let tables = tables_with_type(constant_tree((3 << 8) | BLOCK_FILL));
        let mut dec = decoder(tables, 4, 8);
        dec.decode(&[]).unwrap();

        // Mark row 0 distinctly by hand to observe the flip.
        for p in &mut dec.frame[..4] {
            *p = 1;
        }

        let pitch = 6;
        let mut dst = vec![0xEE; pitch * 8];
        dec.blit_to(&mut dst, pitch, false).unwrap();
        assert_eq!(&dst[0..4], &[1; 4]); // row 0 at top
        assert_eq!(&dst[4..6], &[0xEE; 2]); // padding untouched
        assert_eq!(&dst[pitch..pitch + 4], &[3; 4]);

        let mut dst = vec![0xEE; pitch * 8];
        dec.blit_to(&mut dst, pitch, true).unwrap();
        assert_eq!(&dst[7 * pitch..7 * pitch + 4], &[1; 4]); // row 0 at bottom
        assert_eq!(&dst[6 * pitch..6 * pitch + 4], &[3; 4]);
    }

    #[test]
    fn blit_rejects_small_buffer() {
        let tables = tables_with_type(constant_tree(BLOCK_FILL));
        let mut dec = decoder(tables, 4, 4);
        dec.decode(&[]).unwrap();
        let mut dst = vec![0u8; 15];
        assert!(matches!(
            dec.blit_to(&mut dst, 4, false),
            Err(Error::OutputBufferTooSmall {
                needed: 16,
                got: 15
            })
        ));
    }
}
