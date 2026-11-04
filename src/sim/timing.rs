use std::{sync::RwLock, time::Instant};

use remech2_sys::sim::{self, EasedValue};

use crate::settings;

/// Ticks per frame at the ideal 45 FPS, which is what every sim system seems to be tuned for.
const TICKS_PER_IDEAL_FRAME: i32 = 4;

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

/// Ideal 4-tick intervals completed during this frame.
/// Sums to the elapsed ticks over time.
pub(super) fn ideal_frame_ticks() -> i32 {
    let (now, delta_time) = unsafe { (sim::g_currentClock, sim::g_deltaTime) };

    let crossed = now.div_euclid(TICKS_PER_IDEAL_FRAME)
        - now
            .wrapping_sub(delta_time)
            .div_euclid(TICKS_PER_IDEAL_FRAME);

    crossed * TICKS_PER_IDEAL_FRAME
}

#[unsafe(export_name = "IdealFrameTicks")]
pub extern "C" fn ideal_frame_ticks_c() -> i32 {
    ideal_frame_ticks()
}

/// Whether the game clock crossed a 4-tick boundary this frame.
/// Basically, whether a 45 FPS sim would have run a frame here.
pub(super) fn crossed_ideal_frame() -> bool {
    ideal_frame_ticks() != 0
}

#[unsafe(export_name = "CrossedIdealFrame")]
pub extern "C" fn crossed_ideal_frame_c() -> i32 {
    i32::from(crossed_ideal_frame())
}

/// Closes 1 / 2^m_shift of the distance to the target once per ideal frame.
/// We do this to keep HUD gauges from settling quicker at higher framerates.
#[unsafe(export_name = "UpdateEasedValue")]
pub unsafe extern "C" fn update_eased_value(value: *mut EasedValue) -> i32 {
    let value = unsafe { &mut *value };

    if crossed_ideal_frame() {
        let delta = value.m_target.wrapping_sub(value.m_value);
        value.m_value = value.m_value.wrapping_add(delta >> value.m_shift);
    }

    value.m_value
}
