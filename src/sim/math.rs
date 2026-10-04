use std::sync::atomic::Ordering;

use rand::Rng;

use crate::sim::stats::ZERO_DIVISORS_SUPPRESSED;

/// This function is used all over the game to perform ((a * b) / c).
/// It would sometimes overflow and sometimes divide by zero, especially when the FPS is too high.
#[unsafe(export_name = "MulDiv64")]
pub extern "C" fn mul_div_64(a: i32, b: i32, c: i32) -> i32 {
    if c == 0 {
        tracing::error!("mul_div_64: division by zero (a={a}, b={b})");
        std::process::abort();
    }
    (a as i64 * b as i64 / c as i64) as i32
}

/// Divides two 16.16 fixed-point values.
/// The missile guidance code calls this with a zero divisor sometimes, so we suppress it.
#[unsafe(export_name = "FixedDiv16")]
pub extern "C" fn fixed_div_16(value: i32, divisor: i32) -> i32 {
    if divisor == 0 {
        ZERO_DIVISORS_SUPPRESSED.fetch_add(1, Ordering::Relaxed);
        return 0;
    }
    (((value as i64) << 16) / divisor as i64) as i32
}

/// Returns a truly pseudorandom number instead of picking from the pregenerated table.
/// This fixes the chance to explode if you're overheating because the pregenerated random
/// numbers had a chance to never return a number < 3 when modulo with a fixed DeltaTime.
///
/// TODO: This could break multiplayer. Look into another solution if it causes desync.
#[unsafe(export_name = "RandomIntBelow")]
pub extern "C" fn random_int_below(max: i32) -> i32 {
    if max <= 0 {
        tracing::error!("random_int_below: max <= 0 (max={max})");
        std::process::abort();
    }
    rand::rng().random_range(0..max)
}
