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

    fn next_input(&mut self) -> Option<f32> {
        loop {
            if self.status != Status::Playing {
                return None;
            }

            let cur = self.cur;

            {
                let slot = &mut self.slots[cur];
                if slot.pos < slot.data.len() {
                    let sample = (slot.data[slot.pos] as f32 - 128.0) / 128.0;
                    slot.pos += 1;
                    return Some(sample);
                }
            }

            if self.slots[cur].data.is_empty() && self.loop_count != 1 {
                return None;
            }

            match self.loop_count {
                0 => self.slots[cur].pos = 0, // replay this buffer forever
                1 => {
                    let other = cur ^ 1;
                    if self.slots[other].is_last {
                        self.status = Status::Done;
                        self.pending_eos = true; // drained by the hooked AIL_serve
                        return None;
                    }
                    if self.slots[other].data.is_empty() || self.slots[other].pos != 0 {
                        // Underrun
                        return None;
                    }
                    self.cur = other;
                    self.slots[other].pos = 0;
                }
                n => {
                    self.loop_count = n - 1;
                    self.slots[cur].pos = 0;
                }
            }
        }
    }
}

/// Output samples rendered per lock acquisition.
const BLOCK: usize = 1024;

pub struct Voice {
    state: Arc<Mutex<VoiceState>>,
    out_rate: u32,
    buf: Vec<f32>,
    pos: usize,

    phase: f64,
    prev: f32,
    next: f32,
}

impl Voice {
    pub fn new(state: Arc<Mutex<VoiceState>>, out_rate: u32) -> Self {
        Self {
            state,
            out_rate,
            buf: vec![0.0; BLOCK],
            pos: BLOCK, // forces a render on the first poll
            phase: 1.0, // forces prev/next to be primed on the first frame
            prev: 0.0,
            next: 0.0,
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

        let step = state.rate.max(1) as f64 / self.out_rate as f64;

        for frame in self.buf.chunks_exact_mut(2) {
            while self.phase >= 1.0 {
                self.prev = self.next;
                self.next = state.next_input().unwrap_or(0.0);
                self.phase -= 1.0;
            }

            let sample = self.prev + (self.next - self.prev) * self.phase as f32;
            self.phase += step;

            frame[0] = sample * gain_l;
            frame[1] = sample * gain_r;
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
        None
    }

    fn channels(&self) -> u16 {
        2
    }

    fn sample_rate(&self) -> u32 {
        self.out_rate
    }

    fn total_duration(&self) -> Option<std::time::Duration> {
        None
    }
}
