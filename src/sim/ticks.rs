use std::{
    sync::Mutex,
    time::{Duration, Instant},
};

/// The PC timer's rate
const TIMER_HZ: u128 = 1_193_182;
/// The divisor that the original game used to scale the timer rate
const TIMER_DIVISOR: u128 = 6556;

/// Handles to each counter
const HANDLES: usize = 64;

/// The bit of a handle, or of `AllocTicks`'s and `PauseTimer`'s flags, for the first counter
const FIRST: u32 = 0x80;
/// The bit of `PauseTimer`'s flags for the second
const SECOND: u32 = 0x100;

static TICKS: Mutex<Ticks> = Mutex::new(Ticks::new());

/// A counter that only advances while the timer is started and it isn't paused
struct Counter {
    paused: bool,
    /// How long it ran before `running_since`
    elapsed: Duration,
    /// When it last began running
    running_since: Option<Instant>,
    /// Where each allocated handle started counting
    bases: [Option<i32>; HANDLES],
}

impl Counter {
    const fn new() -> Self {
        Self {
            paused: false,
            elapsed: Duration::ZERO,
            running_since: None,
            bases: [None; HANDLES],
        }
    }

    fn set_running(&mut self, running: bool, now: Instant) {
        if let Some(since) = self.running_since.take() {
            self.elapsed += now.saturating_duration_since(since);
        }
        if running {
            self.running_since = Some(now);
        }
    }

    fn ticks(&self, now: Instant) -> i32 {
        let running = self
            .running_since
            .map_or(Duration::ZERO, |since| now.saturating_duration_since(since));
        let nanos = (self.elapsed + running).as_nanos();

        (nanos * TIMER_HZ / (TIMER_DIVISOR * 1_000_000_000)) as i32
    }

    fn alloc(&mut self, now: Instant) -> Option<usize> {
        let slot = self.bases.iter().position(Option::is_none)?;
        self.bases[slot] = Some(self.ticks(now));
        Some(slot)
    }

    /// 0 for a handle that isn't allocated
    fn get(&self, slot: usize, now: Instant) -> i32 {
        match self.bases.get(slot) {
            Some(Some(base)) => self.ticks(now).wrapping_sub(*base),
            _ => 0,
        }
    }

    /// Make an allocated handle read `ticks` now
    fn set(&mut self, slot: usize, ticks: i32, now: Instant) {
        let from = self.ticks(now).wrapping_sub(ticks);
        if let Some(Some(base)) = self.bases.get_mut(slot) {
            *base = from;
        }
    }

    fn free(&mut self, slot: usize) {
        if let Some(base) = self.bases.get_mut(slot) {
            *base = None;
        }
    }
}

struct Ticks {
    started: bool,
    first: Counter,
    second: Counter,
}

impl Ticks {
    const fn new() -> Self {
        Self {
            started: false,
            first: Counter::new(),
            second: Counter::new(),
        }
    }

    /// The counter a handle, or `AllocTicks`'s flags, is for
    fn counter(&mut self, handle: u32) -> &mut Counter {
        if handle & FIRST != 0 {
            &mut self.first
        } else {
            &mut self.second
        }
    }

    fn set_started(&mut self, started: bool, now: Instant) {
        self.started = started;
        for counter in [&mut self.first, &mut self.second] {
            counter.set_running(started && !counter.paused, now);
        }
    }

    fn alloc(&mut self, flags: u32, now: Instant) -> i16 {
        match self.counter(flags).alloc(now) {
            Some(slot) => (slot as u32 | (flags & FIRST)) as i16,
            None => -1,
        }
    }

    fn get(&mut self, handle: u32, now: Instant) -> i32 {
        self.counter(handle).get(slot(handle), now)
    }

    fn set(&mut self, handle: u32, ticks: i32, now: Instant) {
        self.counter(handle).set(slot(handle), ticks, now);
    }

    fn free(&mut self, handle: u32) {
        self.counter(handle).free(slot(handle));
    }

    fn pause(&mut self, flags: u32, paused: bool, now: Instant) {
        let started = self.started;
        for (bit, counter) in [(FIRST, &mut self.first), (SECOND, &mut self.second)] {
            if flags & bit != 0 {
                counter.paused = paused;
                counter.set_running(started && !paused, now);
            }
        }
    }
}

/// A handle's index into its counter's table
fn slot(handle: u32) -> usize {
    (handle & !FIRST) as usize
}

/// Starts both counters
#[unsafe(export_name = "StartTicks")]
pub extern "C" fn start() {
    TICKS.lock().unwrap().set_started(true, Instant::now());
}

/// Stops both counters but keeps their values
#[unsafe(export_name = "StopTicks")]
pub extern "C" fn stop() {
    TICKS.lock().unwrap().set_started(false, Instant::now());
}

#[unsafe(export_name = "AllocTicks")]
pub extern "C" fn alloc(flags: u32) -> i16 {
    TICKS.lock().unwrap().alloc(flags, Instant::now())
}

#[unsafe(export_name = "GetTicks")]
pub extern "C" fn get(handle: u32) -> i32 {
    TICKS.lock().unwrap().get(handle, Instant::now())
}

#[unsafe(export_name = "ResetTicks")]
pub extern "C" fn reset(handle: u32) {
    TICKS.lock().unwrap().set(handle, 0, Instant::now());
}

#[unsafe(export_name = "SetTicks")]
pub extern "C" fn set(handle: u32, ticks: i32) {
    TICKS.lock().unwrap().set(handle, ticks, Instant::now());
}

#[unsafe(export_name = "FreeTicks")]
pub extern "C" fn free(handle: u32) {
    TICKS.lock().unwrap().free(handle);
}

/// Only the low word of `paused` counts
#[unsafe(export_name = "Sim_PauseTimer")]
pub extern "C" fn pause(flags: i32, paused: i32) {
    TICKS
        .lock()
        .unwrap()
        .pause(flags as u32, paused as u16 != 0, Instant::now());
}
