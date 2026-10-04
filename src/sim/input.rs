use std::{
    collections::HashMap,
    ptr,
    sync::{LazyLock, Mutex},
};

use mw2_sys::sim::{self, AnalogBinding, InputAxis};

use crate::sim::timing::TICKS_PER_IDEAL_FRAME;

/// The ends of an axis's travel.
const AXIS_POSITION_MAX: i32 = 65536;

/// The fastest an axis may ramp, in units per ideal frame.
const AXIS_RATE_MAX: i32 = 32768;

/// Axis travel the ideal-frame scaling truncated away, keyed by axis address.
static AXIS_POSITION_CARRY: LazyLock<Mutex<HashMap<usize, i32>>> =
    LazyLock::new(|| Mutex::new(HashMap::new()));

/// One frame of an axis ramp, `direction` being 1 for the increase key and -1 for decrease.
unsafe fn ramp_axis(axis: *mut InputAxis, direction: i32) {
    let delta_time = unsafe { sim::g_deltaTime };
    let step = (3 * delta_time).wrapping_shl(unsafe { (*axis).m_rampShift } as u32);

    let rate_ptr = unsafe { (*axis).m_rate };
    let mut rate = unsafe { ptr::read_unaligned(rate_ptr) };
    // Pressing the opposite key restarts the ramp from a standstill
    if rate * direction < 0 {
        rate = 0;
    }
    rate = (rate + direction * step).clamp(-AXIS_RATE_MAX, AXIS_RATE_MAX);
    unsafe { ptr::write_unaligned(rate_ptr, rate) };

    // The rate is one ideal frame's worth of travel, so scale it to the frame we actually
    // got and keep the remainder for the next one
    let mut carries = AXIS_POSITION_CARRY.lock().unwrap();
    let carry = carries.entry(axis as usize).or_insert(0);
    let travel = rate * delta_time + *carry;
    *carry = travel.rem_euclid(TICKS_PER_IDEAL_FRAME);

    let position = unsafe { (*axis).m_position } + travel.div_euclid(TICKS_PER_IDEAL_FRAME);
    unsafe { (*axis).m_position = position.clamp(-AXIS_POSITION_MAX, AXIS_POSITION_MAX) };
}

/// Ramps a key-driven input axis (throttle, steering, torso twist) at the same rate
/// whatever the framerate.
///
/// We hide two key pointers from the original so it skips its own integration, keeping
/// its handling of the centre and analog branches, then integrate against elapsed time here.
#[unsafe(export_name = "UpdateAxisFromKeys")]
pub unsafe extern "C" fn update_axis_from_keys(binding: *mut AnalogBinding) -> i32 {
    let Some(binding) = (unsafe { binding.as_mut() }) else {
        return 0;
    };
    let axis = binding.m_axis;
    let (plus_held, minus_held) = unsafe { ((*axis).m_plusHeld, (*axis).m_minusHeld) };
    // The original only looks at the keys if nothing else has driven the axis yet
    let keys_apply = unsafe { (*axis).m_driven } == 0;
    let rate = unsafe { ptr::read_unaligned((*axis).m_rate) };

    unsafe {
        (*axis).m_plusHeld = ptr::null_mut();
        (*axis).m_minusHeld = ptr::null_mut();
    }
    let mut updated = unsafe { sim::UpdateAxisFromKeysC(binding) };
    unsafe {
        (*axis).m_plusHeld = plus_held;
        (*axis).m_minusHeld = minus_held;
        ptr::write_unaligned((*axis).m_rate, rate);
    }

    if keys_apply {
        let held = |key: *const i8| unsafe { key.as_ref().is_some_and(|held| *held != 0) };
        if held(plus_held) {
            unsafe { ramp_axis(axis, 1) };
            updated = 1;
        }
        if held(minus_held) {
            unsafe { ramp_axis(axis, -1) };
            updated = 1;
        }
        if updated == 0 {
            unsafe { ptr::write_unaligned((*axis).m_rate, 0) };
            AXIS_POSITION_CARRY.lock().unwrap().remove(&(axis as usize));
        }
        unsafe { (*axis).m_driven = updated as i8 };
    }

    updated
}
