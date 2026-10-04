use std::{sync::OnceLock, thread, time::Duration, time::Instant};

fn start() -> Instant {
    static START: OnceLock<Instant> = OnceLock::new();
    *START.get_or_init(Instant::now)
}

#[unsafe(export_name = "MechMilliseconds")]
pub extern "C" fn milliseconds() -> u32 {
    start().elapsed().as_millis() as u32
}

#[unsafe(export_name = "MechSleep")]
pub extern "C" fn sleep(milliseconds: u32) {
    thread::sleep(Duration::from_millis(milliseconds.into()));
}
