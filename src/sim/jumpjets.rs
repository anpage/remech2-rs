use std::{
    collections::HashMap,
    sync::{LazyLock, Mutex},
};

use binding::{macros::hook, patches};

use crate::sim::G_DELTA_TIME;

use super::MODULE;

/// A player's mech.
#[repr(C, packed)]
struct Player {
    game_object: *mut GameObject,
    _unknown1: [u8; 0x9c],
    /// The mech's power state. `2` is powered up and under the pilot's control.
    shutdown_state: i32,
    _unknown2: [u8; 0x1c],
    /// Jumpjet fuel in ticks. Negative means this mech has no jumpjets at all.
    jumpjet_fuel: i32,
    _unknown3: [u8; 0x48],
    overheat_flags: u16,
}

/// A player's entry in the sim's object list.
#[repr(C, packed)]
struct GameObject {
    _unknown1: [u8; 0x4c],
    input: *mut CockpitInput,
}

/// One frame's worth of cockpit input.
#[repr(C, packed)]
struct CockpitInput {
    _unknown1: [u8; 0x1d],
    /// Nonzero while the jumpjet key is held.
    jumpjet_held: u8,
}

/// A full jumpjet tank, in ticks.
const JUMPJET_FUEL_MAX: i32 = 1810;

/// The fuel value the recharge branch of `FuncWithJumpjetCalc` will leave behind, or `None` if it's not going to recharge.
unsafe fn jumpjet_recharge_result(player: &Player, delta_time: i32) -> Option<i32> {
    if player.overheat_flags & 0x200 != 0 {
        return None;
    }
    if player.jumpjet_fuel < 0 || player.jumpjet_fuel >= JUMPJET_FUEL_MAX {
        return None;
    }

    let input = unsafe { player.game_object.as_ref()?.input.as_ref()? };
    if input.jumpjet_held != 0 && player.shutdown_state == 2 {
        return None;
    }

    Some(player.jumpjet_fuel + delta_time / 4)
}

/// Jumpjet fuel ticks the game truncated away. HashMap for tracking multiple players.
static JUMPJET_FUEL_CARRY: LazyLock<Mutex<HashMap<usize, i32>>> =
    LazyLock::new(|| Mutex::new(HashMap::new()));

/// Recharges jumpjet fuel at the same rate whatever the framerate.
#[hook(rva = 0x000180cd)]
unsafe extern "cdecl" fn func_with_jumpjet_calc(player: *mut Player) {
    let delta_time = unsafe { G_DELTA_TIME.get() };

    let expected_fuel = unsafe { jumpjet_recharge_result(&*player, delta_time) };

    unsafe {
        original(player);
    }

    let mut carries = JUMPJET_FUEL_CARRY.lock().unwrap();
    let carry = carries.entry(player.addr()).or_insert(0);
    let player = unsafe { &mut *player };

    if expected_fuel != Some(player.jumpjet_fuel) {
        *carry = 0;
        return;
    }

    // Calculate the remainder of the fuel recharge calculation and apply it if it exceeds 4 ticks
    *carry += unsafe { G_DELTA_TIME.get() } % 4;
    if *carry >= 4 {
        *carry -= 4;
        if player.jumpjet_fuel < JUMPJET_FUEL_MAX {
            player.jumpjet_fuel += 1;
        }
    }
}

patches!(
    pub(super) static PATCHES = [
        hook func_with_jumpjet_calc,
    ];
);
