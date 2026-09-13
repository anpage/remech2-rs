use std::sync::atomic::{AtomicU32, Ordering};

use rand::Rng;

use binding::macros::{hook, patches};

use super::MODULE;

/// This function is used all over the game to perform ((a * b) / c).
/// It would sometimes overflow and sometimes divide by zero, especially when the FPS is too high.
#[hook(rva = 0x000035a0)]
unsafe extern "cdecl" fn mul_div_64(a: i32, b: i32, c: i32) -> i32 {
    if c == 0 {
        tracing::error!("mul_div_64: division by zero (a={a}, b={b})");
        std::process::abort();
    }
    (a as i64 * b as i64 / c as i64) as i32
}

/// Calls into `FixedDiv16` with a zero divisor that we answered instead of letting crash.
pub static ZERO_DIVISORS_SUPPRESSED: AtomicU32 = AtomicU32::new(0);

/// Divides two 16.16 fixed-point values.
/// The missile guidance code calls this with a zero divisor sometimes, so we suppress it.
#[hook(rva = 0x00002c90)]
unsafe extern "cdecl" fn fixed_div_16(value: u32, divisor: i32) -> u32 {
    if divisor == 0 {
        ZERO_DIVISORS_SUPPRESSED.fetch_add(1, Ordering::Relaxed);
        return 0;
    }
    (((value as i32 as i64) << 16) / divisor as i64) as u32
}

/// Returns a truly pseudorandom number instead of picking from the pregenerated table.
/// This fixes the chance to explode if you're overheating because the pregenerated random
/// numbers had a chance to never return a number < 3 when modulo with a fixed DeltaTime.
///
/// TODO: This could break multiplayer. Look into another solution if it causes desync.
#[hook(rva = 0x000736b3)]
unsafe extern "cdecl" fn random_int_below(max: i32) -> i32 {
    if max <= 0 {
        tracing::error!("random_int_below: max <= 0 (max={max})");
        std::process::abort();
    }
    rand::rng().random_range(0..max)
}

patches! {
    pub(super) static PATCHES = [
        hook mul_div_64,
        hook fixed_div_16,
        hook random_int_below,
    ];
}
