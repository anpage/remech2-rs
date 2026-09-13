use std::sync::atomic::Ordering;

use binding::{macros::hook, patches};

use crate::sim::{
    G_DELTA_TIME,
    stats::{PROXIMITY_FUSES_SUPPRESSED, ZERO_LENGTH_FRAMES_SKIPPED},
    timing::TICKS_PER_IDEAL_FRAME,
};

use super::MODULE;

/// A projectile in flight.
#[repr(C, packed)]
struct Shot {
    _unknown1: [u8; 0x20],
    /// Ticks since launch
    age: i32,
    _unknown2: [u8; 0x14],
    flags: u32,
}

/// Set when a guided missile is within 101 units of its lock.
/// It makes the shot detonate on the locked target without running any collision test.
const SHOT_PROXIMITY_FUSE: u32 = 32768;

/// Whether a shot that has just aged to `age` ticks crossed a 4-tick boundary doing so.
/// Baiscally, whether a 45 FPS sim would have sampled it on this frame.
fn crossed_ideal_frame_boundary(age: i32) -> bool {
    let previous_age = age.saturating_sub(unsafe { G_DELTA_TIME.get() });
    age.div_euclid(TICKS_PER_IDEAL_FRAME) != previous_age.div_euclid(TICKS_PER_IDEAL_FRAME)
}

/// Steers a guided missile toward its lock and arms its proximity fuse.
/// We keep the 45 FPS sampling density: let the fuse arm only on the frame where the
/// shot's age crosses a 4-tick boundary. At `DeltaTime >= 4` every frame crosses one,
/// which leaves the "ideal" 45 FPS framerate and anything below it untouched.
#[hook(rva = 0x0006ae5a)]
unsafe extern "cdecl" fn guide_missile_to_target(shot: *mut Shot, x: i32, y: i32, z: i32) {
    unsafe {
        original(shot, x, y, z);
    }

    let shot = unsafe { &mut *shot };
    if shot.flags & SHOT_PROXIMITY_FUSE != 0 && !crossed_ideal_frame_boundary(shot.age) {
        shot.flags &= !SHOT_PROXIMITY_FUSE;
        PROXIMITY_FUSES_SUPPRESSED.fetch_add(1, Ordering::Relaxed);
    }
}

/// Advances every shot in flight.
/// Skipped entirely on a zero-length frame.
///
/// Frames that finish inside a single 181 Hz tick result in `DeltaTime == 0`, and the shot updater
/// doesn't properly handle this condition. The zero elapsed time causes it to fall into a point test
/// that doesn't consider who shot the missile, so the missile detonates immediately on the mech who shot it.
#[hook(rva = 0x0006a486)]
unsafe extern "cdecl" fn update_all_shots() {
    unsafe {
        if G_DELTA_TIME.get() == 0 {
            ZERO_LENGTH_FRAMES_SKIPPED.fetch_add(1, Ordering::Relaxed);
            return;
        }

        original();
    }
}

patches!(
    pub(super) static PATCHES = [
        hook guide_missile_to_target,
        hook update_all_shots,
    ];
);
