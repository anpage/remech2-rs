use std::sync::atomic::AtomicU32;

/// Proximity fuses that armed on a frame the 45 FPS sim would never have sampled.
pub static PROXIMITY_FUSES_SUPPRESSED: AtomicU32 = AtomicU32::new(0);

/// Frames with `DeltaTime == 0` that we skipped the shot updater on.
pub static ZERO_LENGTH_FRAMES_SKIPPED: AtomicU32 = AtomicU32::new(0);

/// Calls into `FixedDiv16` with a zero divisor that we answered instead of letting crash.
pub static ZERO_DIVISORS_SUPPRESSED: AtomicU32 = AtomicU32::new(0);
