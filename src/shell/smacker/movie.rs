use std::{
    ffi::c_void,
    path::PathBuf,
    sync::mpsc::{self, Receiver, TryRecvError},
    thread,
    time::{Duration, Instant},
};

use anyhow::{Context, Result, bail};
use smacker::{AudioInfo, Decoder, TRACK_COUNT};
use tracing::warn;

use super::sound::Sound;

/// Size in bytes of the file header the `Smack` structure starts with
const HEADER_LEN: usize = 104;

/// Size in bytes of the structure's two palette buffers
const PALETTE_LEN: usize = 768;

/// Bit `9 + n` selects track `n` in open flags and track masks
const TRACK_FLAG_BASE: u32 = 0x0000_0200;

#[repr(C)]
pub struct Smack {
    header: [u8; HEADER_LEN],
    /// non-zero when this frame introduced a palette
    new_palette: u32,
    /// 1 when the current palette is `palette_a`, 2 when `palette_b`
    pal_type: u32,
    /// first palette buffer, 6-bit RGB triplets
    palette_a: [u8; PALETTE_LEN],
    _reserved_a: u32,
    /// second palette buffer, 6-bit RGB triplets
    palette_b: [u8; PALETTE_LEN],
    _reserved_b: u32,
    /// index of the frame currently loaded
    frame_num: u32,
}

struct Destination {
    buf: *mut u8,
    left: u32,
    top: u32,
    pitch: u32,
    dest_height: u32,
    flip: bool,
}

#[repr(C)]
pub struct Movie {
    public: Smack,
    state: State,
}

struct State {
    /// Declared before `data` because it's borrowed here
    decoder: Decoder<'static>,
    dest: Option<Destination>,
    sound_track: Option<usize>,
    sound: Option<Sound>,
    sound_rx: Option<Receiver<Option<Sound>>>,
    audio_deadline: Duration,
    deadline: Option<Instant>,
    frame_released: bool,
    /// The whole file. `decoder` holds slices into it, so it must not move
    #[allow(dead_code)]
    data: Box<[u8]>,
    path: PathBuf,
}

impl Movie {
    /// Read and open `path`
    pub fn open(path: &str, flags: u32) -> Result<Box<Self>> {
        let data: Box<[u8]> = std::fs::read(path)
            .with_context(|| format!("couldn't read {path}"))?
            .into_boxed_slice();

        let src: &'static [u8] = unsafe { std::slice::from_raw_parts(data.as_ptr(), data.len()) };
        let decoder = Decoder::open(src).map_err(|e| anyhow::anyhow!("{path}: {e}"))?;

        let Some(header) = data.get(..HEADER_LEN) else {
            bail!("{path}: header truncated");
        };
        let mut header_bytes = [0u8; HEADER_LEN];
        header_bytes.copy_from_slice(header);

        let sound_track = (0..TRACK_COUNT)
            .find(|&t| flags & (TRACK_FLAG_BASE << t) != 0 && decoder.audio_track(t).is_some());
        let mut movie = Box::new(Self {
            public: Smack {
                header: header_bytes,
                new_palette: 0,
                pal_type: 1,
                palette_a: [0; PALETTE_LEN],
                _reserved_a: 0,
                palette_b: [0; PALETTE_LEN],
                _reserved_b: 0,
                frame_num: 0,
            },
            state: State {
                decoder,
                dest: None,
                sound_track,
                sound: None,
                sound_rx: None,
                audio_deadline: Duration::ZERO,
                deadline: None,
                frame_released: false,
                data,
                path: PathBuf::from(path),
            },
        });

        movie.publish_frame_state();
        Ok(movie)
    }

    /// Sets where the decoded frames should go
    pub fn set_destination(
        &mut self,
        buf: *mut c_void,
        left: u32,
        top: u32,
        pitch: u32,
        dest_height: u32,
        flip: bool,
    ) {
        self.state.dest = Some(Destination {
            buf: buf.cast::<u8>(),
            left,
            top,
            pitch,
            dest_height,
            flip,
        });
    }

    /// Decode the current frame into the set destination
    pub fn do_frame(&mut self) -> Result<()> {
        let Some(dest) = self.state.dest.as_ref() else {
            return Ok(());
        };

        if dest.buf.is_null() {
            bail!("destination buffer is null");
        }

        let info = self.state.decoder.video_info();
        let (width, height) = (info.width as usize, info.height as usize);
        let pitch = dest.pitch as usize;
        let (left, top) = (dest.left as usize, dest.top as usize);
        if pitch < width || height == 0 {
            bail!("destination pitch {pitch} too small for a {width}x{height} frame");
        }

        let first_row = if dest.flip {
            top.checked_add(height)
                .and_then(|bottom| (dest.dest_height as usize).checked_sub(bottom))
                .context("flipped frame does not fit the destination")?
        } else {
            top
        };
        let start = first_row
            .checked_mul(pitch)
            .and_then(|o| o.checked_add(left))
            .context("destination offset overflows")?;
        let len = (height - 1)
            .checked_mul(pitch)
            .and_then(|o| o.checked_add(width))
            .context("destination span overflows")?;

        let dst = unsafe { std::slice::from_raw_parts_mut(dest.buf.add(start), len) };
        self.state
            .decoder
            .decode_frame(dst, pitch, dest.flip)
            .map_err(|e| anyhow::anyhow!("frame {}: {e}", self.public.frame_num))?;
        Ok(())
    }

    /// Advance to the next frame and apply its palette record.
    pub fn next_frame(&mut self) -> Result<()> {
        self.state
            .decoder
            .next_frame()
            .map_err(|e| anyhow::anyhow!("{e}"))?;
        self.publish_frame_state();
        Ok(())
    }

    /// Seeks to the given frame
    pub fn goto(&mut self, frame: u32) -> Result<()> {
        let target = if frame == 0 {
            self.state.decoder.frame_index()
        } else {
            frame - 1
        };
        self.state
            .decoder
            .seek(target)
            .map_err(|e| anyhow::anyhow!("seek to frame {target}: {e}"))?;
        self.resync_audio(target);
        self.publish_frame_state();
        Ok(())
    }

    /// Jumps the audio to the target frame
    fn resync_audio(&mut self, target: u32) {
        let Some(sound) = self.state.sound.as_ref() else {
            return;
        };
        let at = self.frame_duration().saturating_mul(target);
        self.state.audio_deadline = if sound.seek(at) { at } else { sound.position() };
    }

    /// How long one frame is on screen
    fn frame_duration(&self) -> Duration {
        Duration::from_micros(u64::from(self.state.decoder.frame_time_10us()) * 10)
    }

    /// Waits for the current frame's display time to finish
    pub fn wait(&mut self) -> u32 {
        if self.state.frame_released {
            return 0;
        }
        let step = self.frame_duration();
        self.start_sound();
        self.poll_sound();

        if let Some(sound) = self.state.sound.as_ref() {
            if step.is_zero() {
                self.state.frame_released = true;
                return 0;
            }

            let pos = sound.position();
            if pos < self.state.audio_deadline {
                return 1;
            }

            let mut next = self.state.audio_deadline.saturating_add(step);
            if pos > next.saturating_add(step) {
                next = pos.saturating_add(step);
            }

            self.state.audio_deadline = next;
            self.state.frame_released = true;

            return 0;
        }

        let now = Instant::now();
        match self.state.deadline {
            None => {
                self.state.deadline = Some(now + step);
                1
            }
            Some(deadline) if deadline <= now => {
                let mut next = deadline + step;
                if now.saturating_duration_since(next) > step {
                    next = now + step;
                }
                self.state.deadline = Some(next);
                if !step.is_zero() {
                    self.state.frame_released = true;
                }
                0
            }
            Some(_) => 1,
        }
    }

    /// Whether any track selected by `trackflags` exists in this file
    pub fn sound_in_track(&self, trackflags: u32) -> bool {
        (0..TRACK_COUNT).any(|t| {
            trackflags & (TRACK_FLAG_BASE << t) != 0 && self.state.decoder.audio_track(t).is_some()
        })
    }

    /// Decode the current frame's audio for the first track in `trackflags` into `dst`
    pub unsafe fn get_track_data(&mut self, dst: *mut c_void, trackflags: u32) -> Result<u32> {
        let Some(track) = (0..TRACK_COUNT).find(|&t| trackflags & (TRACK_FLAG_BASE << t) != 0)
        else {
            return Ok(0);
        };

        let Some(info) = self.state.decoder.audio_track(track) else {
            return Ok(0);
        };

        if dst.is_null() {
            bail!("track data destination is null");
        }

        let out = unsafe {
            std::slice::from_raw_parts_mut(dst.cast::<u8>(), info.max_unpacked_size as usize)
        };

        let written = self
            .state
            .decoder
            .audio_data(track, out)
            .map_err(|e| anyhow::anyhow!("{e}"))?;

        Ok(u32::try_from(written).unwrap_or(u32::MAX))
    }

    /// Start audio on the first paced frame.
    /// A worker thread decodes the audio while the movie paces on the wall clock
    fn start_sound(&mut self) {
        if self.state.sound.is_some() || self.state.sound_rx.is_some() {
            return;
        }

        let Some(track) = self.state.sound_track.take() else {
            return;
        };

        let Some(info) = self.state.decoder.audio_track(track) else {
            return;
        };

        let path = self.state.path.clone();
        let (tx, rx) = mpsc::channel();
        thread::spawn(move || {
            let _ = tx.send(load_sound(&path, track, info));
        });

        self.state.sound_rx = Some(rx);
    }

    /// Collect the worker thread's sound.
    /// If it is ready, sync it to the video and switch pacing to the audio clock.
    fn poll_sound(&mut self) {
        let Some(rx) = self.state.sound_rx.as_ref() else {
            return;
        };

        let arrived = match rx.try_recv() {
            Ok(arrived) => arrived,
            Err(TryRecvError::Empty) => return,
            Err(TryRecvError::Disconnected) => None,
        };

        self.state.sound_rx = None;

        let Some(sound) = arrived else {
            return;
        };

        let at = self
            .frame_duration()
            .saturating_mul(self.state.decoder.frame_index());

        self.state.audio_deadline = if sound.seek(at) { at } else { sound.position() };
        self.state.sound = Some(sound);
    }

    /// Copy the decoder's per-frame state into the structure the game reads
    fn publish_frame_state(&mut self) {
        self.public.frame_num = self.state.decoder.frame_index();
        self.state.frame_released = false;

        if !self.state.decoder.palette_changed() {
            self.public.new_palette = 0;
            return;
        }

        let mut flat = [0u8; PALETTE_LEN];
        let (triplets, _) = flat.as_chunks_mut::<3>();
        for (out, entry) in triplets.iter_mut().zip(self.state.decoder.palette()) {
            *out = *entry;
        }

        if self.public.pal_type == 1 {
            self.public.palette_b = flat;
            self.public.pal_type = 2;
        } else {
            self.public.palette_a = flat;
            self.public.pal_type = 1;
        }

        self.public.new_palette = 1;
    }
}

/// The audio worker thread.
/// Decoding the whole track and opening the device are too slow for the game thread.
fn load_sound(path: &std::path::Path, track: usize, info: AudioInfo) -> Option<Sound> {
    let start = || -> Result<Option<Sound>> {
        let data =
            std::fs::read(path).with_context(|| format!("couldn't read {}", path.display()))?;

        let decoder =
            Decoder::open(&data).map_err(|e| anyhow::anyhow!("{}: {e}", path.display()))?;

        let Some(pcm) = decoder
            .decode_track(track)
            .map_err(|e| anyhow::anyhow!("{}: {e}", path.display()))?
        else {
            return Ok(None);
        };

        Sound::start(&pcm, info).map(Some)
    };
    match start() {
        Ok(sound) => sound,
        Err(e) => {
            warn!("smacker: couldn't start FMV audio: {e:#}");
            None
        }
    }
}
