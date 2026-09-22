//! The stream-level decoder: container, palette, video, and audio wired
//! together behind a small frame-at-a-time API.
//!
//! The model matches `SMACKW32.DLL`'s entry points. Opening a file loads the
//! first frame's chunk and applies its palette record, so the palette is
//! valid before anything is drawn; [`Decoder::decode_frame`] renders the
//! currently loaded chunk; [`Decoder::next_frame`] advances to the following
//! chunk and applies its palette record. `SmackDoFrame` and `SmackNextFrame`
//! map onto those two directly, and re-rendering the same frame is
//! idempotent because skipped blocks are simply left alone in the internal
//! frame buffer.

use alloc::vec;
use alloc::vec::Vec;

use crate::audio::AudioDecoder;
use crate::header::{FRAME_TYPE_PALETTE, Header, TRACK_COUNT};
use crate::huffman::HuffmanTables;
use crate::palette::{PALETTE_ENTRIES, Palette, split_record};
use crate::video::VideoDecoder;
use crate::{AudioInfo, Error, VideoInfo};

/// A Smacker stream opened over its whole file image.
///
/// Skipped blocks read back from an internal frame buffer rather than from
/// the caller's destination, so the destination may be drawn over between
/// frames without corrupting the next one.
#[derive(Debug, Clone)]
pub struct Decoder<'a> {
    src: &'a [u8],
    header: Header<'a>,
    video: VideoDecoder,
    palette: Palette,
    audio: [Option<AudioDecoder>; TRACK_COUNT],
    /// Index of the frame chunk currently loaded.
    frame: usize,
    /// Index of the frame whose palette record was last applied, if any.
    /// Re-priming the same frame does not count as a palette change.
    palette_frame: Option<usize>,
    /// Whether the currently loaded frame introduced a new palette.
    new_palette: bool,
}

impl<'a> Decoder<'a> {
    /// Open a stream over the complete file image and load its first frame.
    ///
    /// # Errors
    /// Fails on a bad signature, a malformed header or frame table, a
    /// malformed Huffman tree chunk, or a malformed palette record on the
    /// first frame.
    pub fn open(src: &'a [u8]) -> Result<Self, Error> {
        let header = Header::parse(src)?;
        let tables = HuffmanTables::parse(header.tree_data(), header.tree_sizes())?;
        let info = header.video_info();

        let mut audio: [Option<AudioDecoder>; TRACK_COUNT] = Default::default();
        for (t, slot) in audio.iter_mut().enumerate() {
            if let Some(track) = header.audio_track(t) {
                *slot = Some(AudioDecoder::new(track)?);
            }
        }

        let mut decoder = Self {
            src,
            header,
            video: VideoDecoder::new(&info, tables),
            palette: Palette::new(),
            audio,
            frame: 0,
            palette_frame: None,
            new_palette: false,
        };
        decoder.prime()?;
        Ok(decoder)
    }

    /// Width, height, frame count, and per-frame display time.
    #[must_use]
    pub fn video_info(&self) -> VideoInfo {
        self.header.video_info()
    }

    /// Parameters of audio track `track`, or `None` when the stream has no
    /// such track. The `SmackSoundInTrack` analogue.
    #[must_use]
    pub fn audio_track(&self, track: usize) -> Option<AudioInfo> {
        self.header.audio_track(track)
    }

    /// The current palette, 6-bit components as the original DLL hands them
    /// to the game — not expanded to 8 bits.
    #[must_use]
    pub fn palette(&self) -> &[[u8; 3]; PALETTE_ENTRIES] {
        self.palette.current()
    }

    /// Whether the currently loaded frame carried a palette record that had
    /// not already been applied. The `Smack::NewPalette` analogue.
    #[must_use]
    pub const fn palette_changed(&self) -> bool {
        self.new_palette
    }

    /// Index of the currently loaded frame.
    #[must_use]
    pub fn frame_index(&self) -> u32 {
        u32::try_from(self.frame).unwrap_or(u32::MAX)
    }

    /// Display time of one frame in units of 10 µs, using `SmackOpen`'s
    /// conversion: negative rate → `-rate`, positive → `rate * 100`, zero →
    /// zero (no delay).
    #[must_use]
    pub fn frame_time_10us(&self) -> u32 {
        self.header.video_info().frame_time.0
    }

    /// Decode the current frame and blit it into `dst`, whose rows are
    /// `pitch` bytes apart. With `flip` set the frame is written bottom-up,
    /// matching `SmackToBuffer`'s vertical-flip flag.
    ///
    /// Calling this more than once for the same frame is harmless: block
    /// decoding never reads back from the destination.
    ///
    /// # Errors
    /// Fails on a malformed or truncated frame chunk, a bitstream that runs
    /// out mid-frame, or a `dst` too small for `pitch` and the frame height.
    pub fn decode_frame(&mut self, dst: &mut [u8], pitch: usize, flip: bool) -> Result<(), Error> {
        let chunk = self.header.frame_chunk(self.src, self.frame)?;
        let bits = self.header.video_subchunk(chunk, self.frame)?;
        self.video.decode_frame(bits, dst, pitch, flip)?;
        Ok(())
    }

    /// The most recently decoded frame as `width * height` palette indices,
    /// row-major with no padding.
    #[must_use]
    pub fn frame(&self) -> &[u8] {
        self.video.frame()
    }

    /// Advance to the next frame chunk and apply its palette record.
    ///
    /// Returns `false` when the stream wrapped back to frame 0 instead of
    /// advancing, `true` otherwise.
    ///
    /// # Errors
    /// Fails on a truncated frame chunk or a malformed palette record.
    pub fn next_frame(&mut self) -> Result<bool, Error> {
        let wrapped = self.frame + 1 >= self.header.table_entries();
        self.frame = if wrapped { 0 } else { self.frame + 1 };
        self.prime()?;
        Ok(!wrapped)
    }

    /// Seek to `frame`, replaying from the start when necessary.
    ///
    /// The corpus has no keyframes, so a rewind means decoding every frame
    /// from 0 up to the target to rebuild the frame buffer and palette;
    /// seeking forward replays only the frames in between. Seeking to the
    /// current frame just re-applies its palette record, which — as in the
    /// original — does not count as a palette change.
    ///
    /// # Errors
    /// Fails when `frame` is past the end of the stream, or on any decode
    /// error encountered while replaying.
    pub fn seek(&mut self, frame: u32) -> Result<(), Error> {
        let target = frame as usize;
        if target >= self.header.table_entries() {
            return Err(Error::BadFrameIndex(frame));
        }

        if target == self.frame {
            // Staying put: re-apply the palette record, as the original's
            // re-prime does. Applying the same frame's record twice is not a
            // palette change.
            return self.prime();
        }

        if target < self.frame {
            self.video.reset();
            self.palette = Palette::new();
            self.palette_frame = None;
            self.frame = 0;
            self.prime()?;
        }

        // Replay the frames in between so the frame buffer and palette carry
        // the state a linear playthrough would have reached.
        while self.frame < target {
            let chunk = self.header.frame_chunk(self.src, self.frame)?;
            let bits = self.header.video_subchunk(chunk, self.frame)?;
            self.video.decode(bits)?;
            self.frame += 1;
            self.prime()?;
        }
        Ok(())
    }

    /// Decode the current frame's audio for `track` into `dst` and return
    /// the number of PCM bytes it represents. The `SmackGetTrackData`
    /// analogue; returns 0 when the frame carries no audio for that track.
    ///
    /// Samples are 8-bit unsigned or 16-bit signed little-endian, channels
    /// interleaved, matching the track's header entry.
    ///
    /// # Errors
    /// Fails on a malformed sub-chunk, a format mismatch with the track's
    /// header entry, or a `dst` smaller than the packet's declared
    /// uncompressed size.
    pub fn audio_data(&mut self, track: usize, dst: &mut [u8]) -> Result<usize, Error> {
        if track >= TRACK_COUNT {
            return Err(Error::InvalidFrameChunk("track index out of range"));
        }
        let Some(decoder) = self.audio[track].as_mut() else {
            return Ok(0);
        };
        let chunk = self.header.frame_chunk(self.src, self.frame)?;
        let Some(sub) = self.header.audio_subchunk(chunk, self.frame, track)? else {
            return Ok(0);
        };
        Ok(decoder.decode(sub, dst)?.bytes_written)
    }

    /// Decode every frame's audio for `track` into one contiguous PCM
    /// buffer, without disturbing this decoder's position.
    ///
    /// Returns `None` when the stream has no such track. Frames whose audio
    /// packet is absent contribute silence of the declared length, so the
    /// result stays in step with the frame clock.
    ///
    /// # Errors
    /// Fails on any malformed frame chunk or audio sub-chunk.
    pub fn decode_track(&self, track: usize) -> Result<Option<Vec<u8>>, Error> {
        if track >= TRACK_COUNT {
            return Err(Error::InvalidFrameChunk("track index out of range"));
        }
        let Some(info) = self.header.audio_track(track) else {
            return Ok(None);
        };
        let mut decoder = AudioDecoder::new(info)?;
        let mut scratch = vec![0u8; info.max_unpacked_size as usize];
        let mut pcm = Vec::new();

        for i in 0..self.header.table_entries() {
            let chunk = self.header.frame_chunk(self.src, i)?;
            let Some(sub) = self.header.audio_subchunk(chunk, i, track)? else {
                continue;
            };
            // A packet with its present bit clear reports a length but
            // writes nothing; zero the scratch first so it contributes
            // silence rather than the previous packet's tail.
            scratch.fill(0);
            let written = decoder.decode(sub, &mut scratch)?.bytes_written;
            pcm.extend_from_slice(&scratch[..written]);
        }
        Ok(Some(pcm))
    }

    /// Load the current frame's chunk far enough to apply its palette
    /// record, and update the palette-changed flag.
    fn prime(&mut self) -> Result<(), Error> {
        if self.header.frame_type(self.frame) & FRAME_TYPE_PALETTE == 0 {
            self.new_palette = false;
            return Ok(());
        }
        let chunk = self.header.frame_chunk(self.src, self.frame)?;
        let (ops, _) = split_record(chunk)?;
        self.palette.apply_record(ops)?;
        self.new_palette = self.palette_frame != Some(self.frame);
        self.palette_frame = Some(self.frame);
        Ok(())
    }
}
