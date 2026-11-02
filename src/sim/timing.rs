use std::{sync::RwLock, time::Instant};

use remech2_sys::sim;

use crate::settings;

/// Ticks per frame at the ideal 45 FPS, which is what every sim system seems to be tuned for.
pub(super) const TICKS_PER_IDEAL_FRAME: i32 = 4;

/// Wrapped to limit the framerate to the configured value.
#[unsafe(export_name = "NextClock")]
pub extern "C" fn next_clock() {
    let framerate_limit = settings::get().video.framerate_limit;

    if framerate_limit > 0 {
        static LAST_INSTANT: RwLock<Option<Instant>> = RwLock::new(None);
        let frame_time = 1.0 / framerate_limit as f64;
        let mut last_instant = LAST_INSTANT.write().unwrap();
        let last = *last_instant.get_or_insert_with(Instant::now);
        while last.elapsed().as_secs_f64() < frame_time {
            std::thread::yield_now();
        }
        *last_instant = Some(Instant::now());
    }

    unsafe { sim::NextClockC() };
}

/// The constant used everywhere in the original code to scale by deltaTime
const SIM_TICKS_PER_SECOND: i64 = 181;

#[unsafe(export_name = "SinkStep")]
pub extern "C" fn sink_step(delta: i32, shift: i32) -> i32 {
    let (now, delta_time) = unsafe { (sim::g_currentClock, sim::g_deltaTime) };

    let unit = SIM_TICKS_PER_SECOND << shift.clamp(0, 32);
    let at = |clock: i32| i64::from(delta) * i64::from(clock) / unit;

    (at(now) - at(now.wrapping_sub(delta_time))) as i32
}
