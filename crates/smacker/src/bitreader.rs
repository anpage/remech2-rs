//! LSB-first bit reader.
//!
//! The first bit of the stream is the least significant bit of the first
//! byte; multi-bit reads accumulate later bits in higher positions. Every
//! read is bounds-checked against the underlying slice — no reliance on
//! padded buffers.

use crate::Error;

/// An LSB-first bit reader over a byte slice.
#[derive(Debug, Clone)]
pub struct BitReader<'a> {
    data: &'a [u8],
    /// Absolute bit position within `data`.
    pos: usize,
}

impl<'a> BitReader<'a> {
    /// Create a reader over `data`.
    #[must_use]
    pub const fn new(data: &'a [u8]) -> Self {
        Self { data, pos: 0 }
    }

    /// Number of bits not yet consumed.
    #[must_use]
    pub fn bits_remaining(&self) -> usize {
        self.data.len() * 8 - self.pos
    }

    /// Current bit position.
    #[must_use]
    pub fn position(&self) -> usize {
        self.pos
    }

    /// Whole bytes consumed, rounding up (used for the "bitstream ends
    /// within 4 bytes of the chunk end" validation checks).
    #[must_use]
    pub fn bytes_consumed(&self) -> usize {
        self.pos.div_ceil(8)
    }

    /// Read a single bit.
    ///
    /// # Errors
    /// Returns [`Error::BitstreamExhausted`] when no bits remain.
    pub fn read_bit(&mut self) -> Result<u32, Error> {
        if self.bits_remaining() < 1 {
            return Err(Error::BitstreamExhausted);
        }
        let bit = (self.data[self.pos / 8] >> (self.pos % 8)) & 1;
        self.pos += 1;
        Ok(u32::from(bit))
    }

    /// Read `n` bits (0..=32), LSB-first.
    ///
    /// # Errors
    /// Returns [`Error::BitstreamExhausted`] when fewer than `n` bits remain.
    pub fn read_bits(&mut self, n: u32) -> Result<u32, Error> {
        debug_assert!(n <= 32);
        if (n as usize) > self.bits_remaining() {
            return Err(Error::BitstreamExhausted);
        }
        let mut value: u32 = 0;
        let mut got = 0u32;
        while got < n {
            let byte = self.pos / 8;
            let shift = self.pos % 8;
            let take = (n - got).min(8 - u32::try_from(shift).unwrap_or(0));
            let mask = (1u32 << take) - 1;
            let bits = (u32::from(self.data[byte]) >> shift) & mask;
            value |= bits << got;
            self.pos += take as usize;
            got += take;
        }
        Ok(value)
    }

    /// Discard `n` bits.
    ///
    /// # Errors
    /// Returns [`Error::BitstreamExhausted`] when fewer than `n` bits remain.
    pub fn skip_bits(&mut self, n: u32) -> Result<(), Error> {
        if (n as usize) > self.bits_remaining() {
            return Err(Error::BitstreamExhausted);
        }
        self.pos += n as usize;
        Ok(())
    }

    /// The underlying slice (for end-of-chunk position checks).
    #[must_use]
    pub fn data(&self) -> &'a [u8] {
        self.data
    }
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn lsb_first_single_bits() {
        let mut r = BitReader::new(&[0b1010_0110]);
        assert_eq!(r.bits_remaining(), 8);
        for expected in [0, 1, 1, 0, 0, 1, 0, 1] {
            assert_eq!(r.read_bit().unwrap(), expected);
        }
        assert_eq!(r.read_bit(), Err(Error::BitstreamExhausted));
    }

    #[test]
    fn multi_bit_read_crosses_bytes() {
        // 0b0101_1001, 0b1100_0011 — read 5 then 11 bits.
        let mut r = BitReader::new(&[0x59, 0xC3, 0x0F]);
        assert_eq!(r.read_bits(5).unwrap(), 0b0001_1001);
        // next 11 bits: remaining 3 of byte 0 (010), then all 8 of byte 1
        assert_eq!(r.read_bits(11).unwrap(), 0x61A);
        assert_eq!(r.bits_remaining(), 8);
        assert_eq!(r.bytes_consumed(), 2);
    }

    #[test]
    fn read_32_bits() {
        let mut r = BitReader::new(&[0x01, 0x02, 0x03, 0x04, 0x05]);
        assert_eq!(r.read_bits(32).unwrap(), 0x0403_0201);
        assert_eq!(r.bits_remaining(), 8);
    }

    #[test]
    fn over_read_errors() {
        let mut r = BitReader::new(&[0xFF]);
        assert_eq!(r.read_bits(9), Err(Error::BitstreamExhausted));
        assert_eq!(r.read_bits(8).unwrap(), 0xFF);
        assert_eq!(r.read_bits(1), Err(Error::BitstreamExhausted));
    }

    #[test]
    fn skip_bits_bounds_checked() {
        let mut r = BitReader::new(&[0xFF, 0x00]);
        r.skip_bits(4).unwrap();
        assert_eq!(r.read_bits(4).unwrap(), 0xF);
        assert_eq!(r.skip_bits(9), Err(Error::BitstreamExhausted));
    }

    #[test]
    fn empty_reader() {
        let mut r = BitReader::new(&[]);
        assert_eq!(r.bits_remaining(), 0);
        assert_eq!(r.read_bit(), Err(Error::BitstreamExhausted));
    }
}
