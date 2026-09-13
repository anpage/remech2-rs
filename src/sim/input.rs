use std::{
    collections::HashMap,
    sync::{LazyLock, Mutex},
};

use binding::{macros::hook, patches};

use crate::sim::{G_DELTA_TIME, timing::TICKS_PER_IDEAL_FRAME};

use super::MODULE;

/// One binding of a physical control onto a virtual input axis.
#[repr(C, packed)]
struct AxisBinding {
    _unknown1: [u8; 0x14],
    axis: *mut InputAxis,
}

/// A virtual input axis: throttle, steering, torso twist and so on.
#[repr(C, packed)]
struct InputAxis {
    /// The variable this axis's mapped value is written to each frame
    _output: *mut i32,
    /// Ramp speed while a key is held, in axis units per ideal frame
    rate: *mut i32,
    _unknown1: [u8; 0xd],
    /// Nonzero once something has driven this axis this frame
    updated: u8,
    _unknown2: [u8; 4],
    /// Where the axis currently sits, `-0x10000..=0x10000`
    position: i32,
    /// Per-axis sensitivity, as a left shift on the ramp step
    ramp_shift: u8,
    _unknown3: [u8; 3],
    /// Nonzero while the "increase" key is held
    plus_held: *const u8,
    /// Nonzero while the "decrease" key is held
    minus_held: *const u8,
}

/// The ends of an axis's travel.
const AXIS_POSITION_MAX: i32 = 65536;

/// The fastest an axis may ramp, in units per ideal frame.
const AXIS_RATE_MAX: i32 = 32768;

/// Axis travel the ideal-frame scaling truncated away, keyed by axis address.
static AXIS_POSITION_CARRY: LazyLock<Mutex<HashMap<usize, i32>>> =
    LazyLock::new(|| Mutex::new(HashMap::new()));

/// One frame of an axis ramp, `direction` being 1 for the increase key and -1 for decrease.
unsafe fn ramp_axis(axis: *mut InputAxis, direction: i32) {
    let delta_time = unsafe { G_DELTA_TIME.get() };
    let step = (3 * delta_time).wrapping_shl(unsafe { (*axis).ramp_shift } as u32);

    let rate_ptr = unsafe { (*axis).rate };
    let mut rate = unsafe { std::ptr::read_unaligned(rate_ptr) };
    // Pressing the opposite key restarts the ramp from a standstill
    if rate * direction < 0 {
        rate = 0;
    }
    rate = (rate + direction * step).clamp(-AXIS_RATE_MAX, AXIS_RATE_MAX);
    unsafe { std::ptr::write_unaligned(rate_ptr, rate) };

    // The rate is one ideal frame's worth of travel, so scale it to the frame we actually
    // got and keep the remainder for the next one
    let mut carries = AXIS_POSITION_CARRY.lock().unwrap();
    let carry = carries.entry(axis as usize).or_insert(0);
    let travel = rate * delta_time + *carry;
    *carry = travel.rem_euclid(TICKS_PER_IDEAL_FRAME);

    let position = unsafe { (*axis).position } + travel.div_euclid(TICKS_PER_IDEAL_FRAME);
    unsafe { (*axis).position = position.clamp(-AXIS_POSITION_MAX, AXIS_POSITION_MAX) };
}

/// Ramps a key-driven input axis (throttle, steering, torso twist) at the same rate
/// whatever the framerate.
///
/// We hide two key pointers from the original so it skips its own integration, keeping
/// its handling of the centre and analog branches, then integrate against elapsed time here.
#[hook(rva = 0x0007aecc)]
unsafe extern "cdecl" fn update_axis_from_keys(binding: *mut AxisBinding) -> i32 {
    let axis = unsafe { (*binding).axis };
    let (plus_held, minus_held) = unsafe { ((*axis).plus_held, (*axis).minus_held) };
    // The original only looks at the keys if nothing else has driven the axis yet
    let keys_apply = unsafe { (*axis).updated } == 0;
    let rate = unsafe { std::ptr::read_unaligned((*axis).rate) };

    unsafe {
        (*axis).plus_held = std::ptr::null();
        (*axis).minus_held = std::ptr::null();
    }
    let mut updated = unsafe { original(binding) };
    unsafe {
        (*axis).plus_held = plus_held;
        (*axis).minus_held = minus_held;
        std::ptr::write_unaligned((*axis).rate, rate);
    }

    if keys_apply {
        let held = |key: *const u8| unsafe { key.as_ref().is_some_and(|held| *held != 0) };
        if held(plus_held) {
            unsafe { ramp_axis(axis, 1) };
            updated = 1;
        }
        if held(minus_held) {
            unsafe { ramp_axis(axis, -1) };
            updated = 1;
        }
        if updated == 0 {
            unsafe { std::ptr::write_unaligned((*axis).rate, 0) };
            AXIS_POSITION_CARRY.lock().unwrap().remove(&(axis as usize));
        }
        unsafe { (*axis).updated = updated as u8 };
    }

    updated
}

patches!(
    pub(super) static PATCHES = [
        hook update_axis_from_keys,
    ];
);
