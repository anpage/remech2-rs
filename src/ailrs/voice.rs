use std::sync::{Arc, Mutex};

use rodio::Source;

use crate::ailrs::interface::SampleHandle;

const DEFAULT_VOLUME: u8 = 127;
const DEFAULT_RATE: u32 = 11025;

#[derive(Clone, Copy, Debug, Default, PartialEq, Eq)]
pub enum Status {
    #[default]
    Done,
    Playing,
    Stopped,
}

/// One half of the double buffer. `data` is a copy of the game's buffer.
/// AIL played out of buffers owned by the game.
#[derive(Default)]
struct Slot {
    data: Vec<u8>,
    pos: usize,
    is_last: bool,
}

#[derive(Default)]
pub struct VoiceState {
    slots: [Slot; 2],
    cur: usize,
    /// The value `cur` had when a buffer was last handed out: an edge detector.
    /// `-2` is "both slots free", `-1` is "slot 0 issued".
    mark: i32,
    pub loop_count: u32,
    pub status: Status,
    /// Cleared by `release_sample_handle`. It's the only thing that makes the
    /// `Source` return `None` and so unregister from the mixer.
    pub alive: bool,
    pub rate: u32,
    pub volume: u8,
    pub pan: u8,
    pub user_data: [i32; 16],
    /// Set by the mixer thread, drained by the hooked `AIL_serve`.
    pub pending_eos: bool,
    pub eos_callback: Option<unsafe extern "stdcall" fn(SampleHandle)>,
    driver_mono: bool,
}

impl VoiceState {
    pub fn new(driver_mono: bool) -> Self {
        let mut state = Self {
            driver_mono,
            alive: true,
            ..Default::default()
        };
        state.init();
        state
    }

    /// Analog for `AIL_init_sample`, which dliberately does not clear `user_data`
    pub fn init(&mut self) {
        self.slots = Default::default();
        // If slot 1's `is_last` is true, the sample is fired in one shot out of slot 0.
        // `buffer_ready` clears this and switches to double-buffered streaming.
        self.slots[1].is_last = true;
        self.cur = 0;
        self.mark = -2;
        self.loop_count = 1;
        self.rate = DEFAULT_RATE;
        self.volume = DEFAULT_VOLUME;
        self.pan = if self.driver_mono { 0 } else { 64 };
        self.status = Status::Done;
        self.pending_eos = false;
        self.eos_callback = None;
    }

    /// Analog for `AIL_sample_buffer_ready`.
    /// Every call consumes the edge, so calling it twice without an intervening load returns `-1`.
    pub fn buffer_ready(&mut self) -> i32 {
        match self.mark {
            -2 => {
                self.slots[1].is_last = false;
                self.mark = -1;
                0
            }
            -1 => {
                self.mark = self.cur as i32;
                1
            }
            m if m == self.cur as i32 => -1,
            _ => {
                self.mark = self.cur as i32;
                (self.cur ^ 1) as i32
            }
        }
    }

    /// Analog for `AIL_load_sample_buffer`.
    /// An empty `data` is the per-slot end-of-stream marker and must not be dropped.
    pub fn load_buffer(&mut self, slot: usize, data: &[u8]) {
        let s = &mut self.slots[slot];
        s.is_last = data.is_empty();
        s.data.clear();
        s.data.extend_from_slice(data);
        s.pos = 0;

        if !data.is_empty() && self.status != Status::Playing {
            self.status = Status::Playing;
        }
    }
}

/// Samples rendered per lock acquisition.
/// Also the value reported by `current_span_len`, which is what lets a mid-stream
/// `set_sample_playback_rate` take effect: rodio's `UniformSourceIterator` re-reads
/// rate and channels at every span boundary.
/// Must stay even so a stereo frame is never split across spans.
const BLOCK: usize = 1024;

pub struct Voice {
    state: Arc<Mutex<VoiceState>>,
    buf: Vec<f32>,
    pos: usize,
}

impl Voice {
    pub fn new(state: Arc<Mutex<VoiceState>>) -> Self {
        Self {
            state,
            buf: vec![0.0; BLOCK],
            pos: BLOCK, // forces a render on the first poll
        }
    }

    fn render_block(&mut self) -> bool {
        let mut state = self.state.lock().unwrap();
        if !state.alive {
            return false;
        }

        let gain = state.volume as f32 / 127.0;
        // Linear is close enough at 8-bit source quality, probably.
        // TODO: Maybe research how AIL calculated this.
        let pan = state.pan as f32 / 127.0;
        let gain_l = gain * (1.0 - pan);
        let gain_r = gain * pan;

        let mut out = 0;
        while out < BLOCK {
            if state.status != Status::Playing {
                break;
            }

            let cur = state.cur;

            {
                let slot = &mut state.slots[cur];
                if slot.pos < slot.data.len() {
                    let sample = (slot.data[slot.pos] as f32 - 128.0) / 128.0;
                    slot.pos += 1;
                    self.buf[out] = sample * gain_l;
                    self.buf[out + 1] = sample * gain_r;
                    out += 2;
                    continue;
                }
            }

            if state.slots[cur].data.is_empty() && state.loop_count != 1 {
                break;
            }

            match state.loop_count {
                0 => state.slots[cur].pos = 0, // replay this buffer forever
                1 => {
                    let other = cur ^ 1;
                    if state.slots[other].is_last {
                        state.status = Status::Done;
                        state.pending_eos = true; // drained by the hooked AIL_serve
                        break;
                    }
                    if state.slots[other].data.is_empty() || state.slots[other].pos != 0 {
                        // Underrun
                        break;
                    }
                    state.cur = other;
                    state.slots[other].pos = 0;
                }
                n => {
                    state.loop_count = n - 1;
                    state.slots[cur].pos = 0;
                }
            }
        }

        // Silence-fill on underrun and keep the voice registered
        for sample in &mut self.buf[out..] {
            *sample = 0.0;
        }
        true
    }
}

impl Iterator for Voice {
    type Item = f32;

    fn next(&mut self) -> Option<f32> {
        if self.pos == BLOCK {
            if !self.render_block() {
                return None;
            }
            self.pos = 0;
        }
        let sample = self.buf[self.pos];
        self.pos += 1;
        Some(sample)
    }
}

impl Source for Voice {
    fn current_span_len(&self) -> Option<usize> {
        Some(BLOCK)
    }

    fn channels(&self) -> u16 {
        2
    }

    fn sample_rate(&self) -> u32 {
        self.state.lock().unwrap().rate
    }

    fn total_duration(&self) -> Option<std::time::Duration> {
        None
    }
}
