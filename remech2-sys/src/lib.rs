#![allow(
    non_snake_case,
    non_camel_case_types,
    non_upper_case_globals,
    dead_code,
    clippy::all
)]

pub mod shared {
    include!(concat!(env!("OUT_DIR"), "/shared.rs"));
}

pub mod sim {
    include!(concat!(env!("OUT_DIR"), "/mw2.rs"));
}

pub mod shell {
    include!(concat!(env!("OUT_DIR"), "/mw2shell.rs"));
}
