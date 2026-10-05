use std::{
    collections::HashMap,
    sync::{LazyLock, Mutex},
};

use remech2_sys::sim::{self, Mech};

/// A full jumpjet tank, in ticks.
const JUMPJET_FUEL_MAX: i32 = 1810;

/// The fuel value the recharge branch of `LateUpdateMechC` will leave behind, or `None` if it's not going to recharge.
unsafe fn jumpjet_recharge_result(mech: &Mech, delta_time: i32) -> Option<i32> {
    if mech.m_flags & 0x200 != 0 {
        return None;
    }
    let fuel = mech.m_jumpFuel;
    if !(0..JUMPJET_FUEL_MAX).contains(&fuel) {
        return None;
    }

    let steering = unsafe { mech.m_player.as_ref()?.m_steering.as_ref()? };
    if steering.m_jumpJetEnabled != 0 && mech.m_powerState == 2 {
        return None;
    }

    Some(fuel + delta_time / 4)
}

/// Jumpjet fuel ticks the game truncated away. HashMap for tracking multiple mechs.
static JUMPJET_FUEL_CARRY: LazyLock<Mutex<HashMap<usize, i32>>> =
    LazyLock::new(|| Mutex::new(HashMap::new()));

/// Recharges jumpjet fuel at the same rate whatever the framerate.
#[unsafe(export_name = "LateUpdateMech")]
pub unsafe extern "C" fn late_update_mech(mech: *mut Mech) {
    if mech.is_null() {
        return;
    }
    let delta_time = unsafe { sim::g_deltaTime };

    let expected_fuel = unsafe { jumpjet_recharge_result(&*mech, delta_time) };

    unsafe { sim::LateUpdateMechC(mech) };

    let mut carries = JUMPJET_FUEL_CARRY.lock().unwrap();
    let carry = carries.entry(mech.addr()).or_insert(0);
    let mech = unsafe { &mut *mech };

    if expected_fuel != Some(mech.m_jumpFuel) {
        *carry = 0;
        return;
    }

    // Calculate the remainder of the fuel recharge calculation and apply it if it exceeds 4 ticks
    *carry += delta_time % 4;
    if *carry >= 4 {
        *carry -= 4;
        if mech.m_jumpFuel < JUMPJET_FUEL_MAX {
            mech.m_jumpFuel += 1;
        }
    }
}
