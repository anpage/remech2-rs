use std::hint::black_box;

use mw2_sys::{shell, sim};

fn main() {
    black_box(sim::SimMain as *const ());
    black_box(shell::ShellMain as *const ());
}
