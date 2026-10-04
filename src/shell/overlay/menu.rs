use std::sync::atomic::{AtomicUsize, Ordering};

static LOCKS: AtomicUsize = AtomicUsize::new(0);

/// Hides the overlay's menu bar until the guard drops
#[must_use = "the bar is only hidden while the guard is alive"]
pub fn lock() -> MenuLock {
    LOCKS.fetch_add(1, Ordering::Relaxed);
    MenuLock
}

pub(super) fn locked() -> bool {
    LOCKS.load(Ordering::Relaxed) != 0
}

pub struct MenuLock;

impl Drop for MenuLock {
    fn drop(&mut self) {
        LOCKS.fetch_sub(1, Ordering::Relaxed);
    }
}
