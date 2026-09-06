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

/// Direct Form I biquad, RBJ cookbook coefficients.
#[derive(Default)]
struct Biquad {
    b0: f32,
    b1: f32,
    b2: f32,
    a1: f32,
    a2: f32,
    x1: f32,
    x2: f32,
    y1: f32,
    y2: f32,
}

impl Biquad {
    fn set_low_pass(&mut self, freq: f32, fs: f32, q: f32) {
        let w0 = std::f32::consts::TAU * (freq / fs).clamp(1.0e-4, 0.45);
        let (sin, cos) = w0.sin_cos();
        let alpha = sin / (2.0 * q);
        let a0 = 1.0 + alpha;
        self.b1 = (1.0 - cos) / a0;
        self.b0 = self.b1 / 2.0;
        self.b2 = self.b0;
        self.a1 = (-2.0 * cos) / a0;
        self.a2 = (1.0 - alpha) / a0;
    }

    fn process(&mut self, x: f32) -> f32 {
        let y = self.b0 * x + self.b1 * self.x1 + self.b2 * self.x2
            - self.a1 * self.y1
            - self.a2 * self.y2;
        self.x2 = self.x1;
        self.x1 = x;
        self.y2 = self.y1;
        self.y1 = y;
        y
    }
}

/// Butterworth Q values for a 4th-order response as two cascaded biquads.
const BUTTERWORTH_Q: [f32; 2] = [0.541_196_1, 1.306_562_9];

/// Reconstruction cutoff as a fraction of the voice's playback rate.
const CUTOFF_RATIO: f32 = 0.45;

/// One-pole highpass at ~20 Hz.
/// Should filter out the pops caused by samples starting and stopping.
struct DcBlocker {
    x1: f32,
    y1: f32,
    r: f32,
}

impl DcBlocker {
    fn new(fs: f32) -> Self {
        Self {
            x1: 0.0,
            y1: 0.0,
            r: 1.0 - std::f32::consts::TAU * 20.0 / fs,
        }
    }

    fn process(&mut self, x: f32) -> f32 {
        let y = x - self.x1 + self.r * self.y1;
        self.x1 = x;
        self.y1 = y;
        y
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

    lpf: [Biquad; 2],
    lpf_rate: u32,

    dc: DcBlocker,

    gain_l: f32,
    gain_r: f32,
    gain_coef: f32,

    env: f32,
    env_coef: f32,
    feeding: bool,
}

impl Voice {
    pub fn new(state: Arc<Mutex<VoiceState>>, out_rate: u32) -> Self {
        let fs = out_rate as f32;
        Self {
            state,
            out_rate,
            buf: vec![0.0; BLOCK],
            pos: BLOCK, // forces a render on the first poll
            phase: 1.0, // forces prev/next to be primed on the first frame
            prev: 0.0,
            next: 0.0,
            lpf: Default::default(),
            lpf_rate: 0,
            dc: DcBlocker::new(fs),
            gain_l: 0.0,
            gain_r: 0.0,
            gain_coef: 1.0 - (-1.0 / (0.005 * fs)).exp(), // ~5 ms
            env: 0.0,
            env_coef: 1.0 - (-1.0 / (0.0015 * fs)).exp(), // ~1.5 ms
            feeding: false,
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
        let target_l = gain * (1.0 - pan);
        let target_r = gain * pan;

        let rate = state.rate.max(1);
        let step = rate as f64 / self.out_rate as f64;

        if rate != self.lpf_rate {
            let fc = CUTOFF_RATIO * rate as f32;
            let fs = self.out_rate as f32;
            for (biquad, q) in self.lpf.iter_mut().zip(BUTTERWORTH_Q) {
                biquad.set_low_pass(fc, fs, q);
            }
            self.lpf_rate = rate;
        }

        for frame in self.buf.as_chunks_mut::<2>().0 {
            while self.phase >= 1.0 {
                self.prev = self.next;
                match state.next_input() {
                    Some(sample) => {
                        self.next = sample;
                        self.feeding = true;
                    }
                    // No input this instant: stopped, done, or underrunning.
                    None => {
                        self.next = 0.0;
                        self.feeding = false;
                    }
                }
                self.phase -= 1.0;
            }

            let sample = self.prev + (self.next - self.prev) * self.phase as f32;
            self.phase += step;

            let sample = self.dc.process(sample);
            let sample = self
                .lpf
                .iter_mut()
                .fold(sample, |sample, biquad| biquad.process(sample));

            let target_env = if self.feeding { 1.0 } else { 0.0 };
            self.env += (target_env - self.env) * self.env_coef;
            self.gain_l += (target_l - self.gain_l) * self.gain_coef;
            self.gain_r += (target_r - self.gain_r) * self.gain_coef;

            let sample = sample * self.env;
            frame[0] = sample * self.gain_l;
            frame[1] = sample * self.gain_r;
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
