use std::sync::{LazyLock, Mutex};

use mw2_sys::shared::MECH_RAND_MAX;
use rand::{Rng, SeedableRng, rngs::StdRng};

static RNG: LazyLock<Mutex<StdRng>> = LazyLock::new(|| Mutex::new(StdRng::seed_from_u64(1)));

#[unsafe(export_name = "MechSRand")]
pub extern "C" fn srand(seed: u32) {
    *RNG.lock().unwrap() = StdRng::seed_from_u64(seed.into());
}

#[unsafe(export_name = "MechRand")]
pub extern "C" fn rand() -> i32 {
    RNG.lock().unwrap().random_range(0..=MECH_RAND_MAX as i32)
}
