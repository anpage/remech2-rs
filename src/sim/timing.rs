use std::{sync::RwLock, time::Instant};

use binding::macros::{globals, hook, patches};

use crate::settings::SETTINGS;

use super::MODULE;

/// Ticks per frame at the ideal 45 FPS, which is what every sim system seems to be tuned for.
pub(super) const TICKS_PER_IDEAL_FRAME: i32 = 4;

globals!(
    pub(super) static G_DELTA_TIME: i32 = 0x000ba550;
);

/// We hook this in order to limit the framerate to the configured value.
#[hook(rva = 0x0007ce2c)]
unsafe extern "stdcall" fn next_clock() {
    let framerate_limit = SETTINGS.get_int("video", "framerate_limit", 45);

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

    unsafe {
        original();
    }
}

patches!(
    pub(super) static PATCHES = [
        hook next_clock,
    ];
);
