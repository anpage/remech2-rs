//! Palette record decoding.
//!
//! A frame whose type byte has the palette bit set opens its chunk with a
//! palette record: a length byte (the record's total length in dwords,
//! *including* the length byte itself) followed by a stream of ops. The ops
//! build a new 256-entry palette, reading from the previous palette where
//! directed:
//!
//! - `b & 0x80` — carry over `(b & 0x7F) + 1` entries from the previous
//!   palette at the same index.
//! - `b & 0x40` — the next byte is a source index; copy `(b & 0x3F) + 1`
//!   entries from the previous palette starting there.
//! - otherwise — `b` and the following two bytes are a raw RGB triplet,
//!   written verbatim.
//!
//! Components are 6-bit (0–63) and are passed through unexpanded, matching
//! what `SMACKW32` hands to the game.
//!
//! The op loop in the original DLL runs until 256 entries have been written
//! with no bound against the record length; a malformed record can therefore
//! run off the end. Here the record length is a hard bound: running out of
//! ops early, or writing past 256 entries, is an error.

use crate::Error;

/// Number of entries in a palette.
pub const PALETTE_ENTRIES: usize = 256;

/// A 256-entry RGB palette with 6-bit components. Each record decodes
/// against the palette produced by the previous record, so decoding goes
/// into a fresh buffer that then becomes current (a copy-from-index op may
/// read entries an earlier op of the same record already overwrote, so
/// decoding in place is not an option).
#[derive(Debug, Clone)]
pub struct Palette {
    current: [[u8; 3]; PALETTE_ENTRIES],
}

impl Default for Palette {
    fn default() -> Self {
        Self::new()
    }
}

impl Palette {
    /// A zeroed palette.
    #[must_use]
    pub const fn new() -> Self {
        Self {
            current: [[0; 3]; PALETTE_ENTRIES],
        }
    }

    /// The most recently decoded palette (6-bit components).
    #[must_use]
    pub const fn current(&self) -> &[[u8; 3]; PALETTE_ENTRIES] {
        &self.current
    }

    /// Apply the ops of a palette record, producing a new current palette.
    /// `ops` is the record body with the length byte already stripped (see
    /// [`split_record`]); the slice length is the hard bound on the op
    /// stream.
    ///
    /// # Errors
    /// Fails when the ops run out before 256 entries are written, when an
    /// op would write past entry 255, or when a copy-from-index op reads
    /// past the end of the previous palette.
    pub fn apply_record(&mut self, ops: &[u8]) -> Result<(), Error> {
        let mut next = [[0u8; 3]; PALETTE_ENTRIES];
        let mut written = 0usize;
        let mut pos = 0usize;

        let mut take = || {
            let b = ops.get(pos).copied();
            pos += usize::from(b.is_some());
            b.ok_or(Error::InvalidPaletteRecord("op stream exhausted"))
        };

        while written < PALETTE_ENTRIES {
            let op = take()?;
            if op & 0x80 != 0 {
                // Carry over entries from the previous palette, same index.
                let count = usize::from(op & 0x7F) + 1;
                let end = written
                    .checked_add(count)
                    .filter(|&e| e <= PALETTE_ENTRIES)
                    .ok_or(Error::InvalidPaletteRecord("carry op overflows palette"))?;
                next[written..end].copy_from_slice(&self.current[written..end]);
                written = end;
            } else if op & 0x40 != 0 {
                // Copy entries from the previous palette at a source index.
                let count = usize::from(op & 0x3F) + 1;
                let src = usize::from(take()?);
                let src_end = src
                    .checked_add(count)
                    .filter(|&e| e <= PALETTE_ENTRIES)
                    .ok_or(Error::InvalidPaletteRecord("copy op reads out of range"))?;
                let dst_end = written
                    .checked_add(count)
                    .filter(|&e| e <= PALETTE_ENTRIES)
                    .ok_or(Error::InvalidPaletteRecord("copy op overflows palette"))?;
                next[written..dst_end].copy_from_slice(&self.current[src..src_end]);
                written = dst_end;
            } else {
                // Raw triplet: this byte plus two more, verbatim.
                let g = take()?;
                let b = take()?;
                next[written] = [op, g, b];
                written += 1;
            }
        }

        self.current = next;
        Ok(())
    }
}

/// Split a palette record off the front of a frame chunk. The first byte
/// multiplied by 4 is the record length *including* that byte.
///
/// Returns the record body (ops, without the length byte) and the remainder
/// of the chunk.
///
/// # Errors
/// Fails when the chunk is empty, the length byte is 0, or the record
/// extends past the end of the chunk.
pub fn split_record(chunk: &[u8]) -> Result<(&[u8], &[u8]), Error> {
    let &len_dwords = chunk
        .first()
        .ok_or(Error::InvalidPaletteRecord("missing length byte"))?;
    let len = usize::from(len_dwords)
        .checked_mul(4)
        .ok_or(Error::InvalidPaletteRecord("record length overflow"))?;
    if len == 0 {
        return Err(Error::InvalidPaletteRecord("zero record length"));
    }
    let record = chunk
        .get(..len)
        .ok_or(Error::InvalidPaletteRecord("record extends past chunk end"))?;
    Ok((&record[1..], &chunk[len..]))
}

#[cfg(test)]
mod tests {
    use super::*;

    /// Build ops that write the whole palette as raw triplets from a
    /// counter, so entry `i` is `[v, v, v]` with `v = i % 64` (raw op bytes
    /// must stay below 0x40 to not read as carry/copy ops — as do real
    /// 6-bit components).
    fn raw_ops() -> Vec<u8> {
        let mut ops = Vec::new();
        for i in 0..PALETTE_ENTRIES {
            let v = (i % 64) as u8;
            ops.extend_from_slice(&[v, v, v]);
        }
        ops
    }

    /// Expected value of entry `i` after applying `raw_ops()`.
    fn raw_value(i: usize) -> u8 {
        (i % 64) as u8
    }

    #[test]
    fn raw_triplets_fill_palette() {
        let mut pal = Palette::new();
        pal.apply_record(&raw_ops()).unwrap();
        for (i, entry) in pal.current().iter().enumerate() {
            assert_eq!(*entry, [raw_value(i); 3]);
        }
    }

    #[test]
    fn carry_copies_previous_at_same_index() {
        let mut pal = Palette::new();
        pal.apply_record(&raw_ops()).unwrap();
        // Second record: carry all 256 entries (0x80 | 127 = 0xFF, twice).
        pal.apply_record(&[0xFF, 0xFF]).unwrap();
        for (i, entry) in pal.current().iter().enumerate() {
            assert_eq!(*entry, [raw_value(i); 3]);
        }
    }

    #[test]
    fn copy_from_index_reads_previous() {
        let mut pal = Palette::new();
        pal.apply_record(&raw_ops()).unwrap();
        // Copy 64 entries from source index 64, 64 more from 128 (the
        // count field is 6 bits, so one op copies at most 64), then carry
        // the remaining 128.
        pal.apply_record(&[0x40 | 63, 64, 0x40 | 63, 128, 0xFF])
            .unwrap();
        for (i, entry) in pal.current().iter().enumerate() {
            let expect = if i < 128 {
                raw_value(i + 64)
            } else {
                raw_value(i)
            };
            assert_eq!(*entry, [expect; 3]);
        }
    }

    #[test]
    fn exhausted_ops_error_instead_of_overreading() {
        let mut pal = Palette::new();
        // One raw triplet short of a full palette.
        let ops = &raw_ops()[..raw_ops().len() - 3];
        assert_eq!(
            pal.apply_record(ops),
            Err(Error::InvalidPaletteRecord("op stream exhausted"))
        );
    }

    #[test]
    fn carry_past_256_errors() {
        let mut pal = Palette::new();
        // 255 raw triplets, then a carry of 2.
        let mut ops = raw_ops();
        ops.truncate(ops.len() - 3);
        ops.push(0x80 | 1);
        assert_eq!(
            pal.apply_record(&ops),
            Err(Error::InvalidPaletteRecord("carry op overflows palette"))
        );
    }

    #[test]
    fn copy_source_past_256_errors() {
        let mut pal = Palette::new();
        // Copy 2 entries starting at source index 255.
        assert_eq!(
            pal.apply_record(&[0x40 | 1, 255]),
            Err(Error::InvalidPaletteRecord("copy op reads out of range"))
        );
    }

    #[test]
    fn split_record_uses_length_byte() {
        // Length byte 2 → record is 8 bytes total, 7 ops bytes.
        let chunk = [2u8, 1, 2, 3, 4, 5, 6, 7, 0xAA, 0xBB];
        let (ops, rest) = split_record(&chunk).unwrap();
        assert_eq!(ops, &[1, 2, 3, 4, 5, 6, 7]);
        assert_eq!(rest, &[0xAA, 0xBB]);
    }

    #[test]
    fn split_record_rejects_bad_lengths() {
        assert!(matches!(
            split_record(&[]),
            Err(Error::InvalidPaletteRecord(_))
        ));
        assert_eq!(
            split_record(&[0, 1, 2, 3]),
            Err(Error::InvalidPaletteRecord("zero record length"))
        );
        // Declares 12 bytes but the chunk has 4.
        assert_eq!(
            split_record(&[3, 1, 2, 3]),
            Err(Error::InvalidPaletteRecord("record extends past chunk end"))
        );
    }
}
