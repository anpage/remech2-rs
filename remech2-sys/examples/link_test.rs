use std::hint::black_box;

use remech2_sys::{shell, sim};

fn main() {
    black_box(sim::SimMain as *const ());
    black_box(shell::ShellMain as *const ());
}
