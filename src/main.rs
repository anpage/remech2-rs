#![cfg_attr(not(debug_assertions), windows_subsystem = "windows")]
#![recursion_limit = "256"]

use anyhow::Result;
use std::{
    env,
    fs::File,
    io::{BufReader, Read, Seek, SeekFrom},
};
use tracing::Level;
use tracing_subscriber::{filter, prelude::*};

use crate::{display::Overlay, settings::SETTINGS};

mod about;
mod ailrs;
mod app;
mod cd_audio;
mod common;
mod display;
mod drawmode;
mod files;
mod heap;
mod launcher;
mod log;
mod messages;
mod midi_source;
mod settings;
mod shell;
mod sim;
mod xmi;

fn start_shell(intro_or_sim: &str) -> Result<i32> {
    display::set_overlay(app::with(|app| {
        Overlay::Shell(shell::OverlayUi::new(app.egui_ctx()))
    }));
    let result = shell::run(intro_or_sim);
    display::set_overlay(None);
    result
}

fn start_sim(cmd_line: &str) -> Result<i32> {
    display::set_overlay(app::with(|app| {
        Overlay::Sim(sim::OverlayUi::new(app.egui_ctx()))
    }));
    let result = sim::run(cmd_line);
    display::set_overlay(None);
    result
}

fn str_to_level(loglevel: &str) -> Level {
    match loglevel {
        "trace" => Level::TRACE,
        "debug" => Level::DEBUG,
        "info" => Level::INFO,
        "warn" => Level::WARN,
        "error" => Level::ERROR,
        _ => Level::WARN,
    }
}

fn main() -> Result<()> {
    let loglevel = SETTINGS
        .get(Some("debug"), "loglevel")
        .unwrap_or("warn".to_string())
        .to_ascii_lowercase();

    let loglevel = str_to_level(&loglevel);

    let filter = filter::Targets::new().with_target("remech2", loglevel);
    tracing_subscriber::registry()
        .with(tracing_subscriber::fmt::layer())
        .with(filter)
        .init();

    let args: Vec<String> = env::args().collect();

    let mut app = app::App::new()?;

    if !launcher::Launcher::new().run(&mut app)? {
        // The window was closed
        return Ok(());
    }

    app::install(app);

    if args.len() > 1 {
        // launch the sim with the given cmdline
        start_sim(&args[1..].join(" "))?;
        return Ok(());
    }

    let mut result = start_shell("intro")?;

    loop {
        if result == 255 {
            return Ok(());
        }

        let cmd_line = {
            let mut buffer = vec![];
            let mut file = BufReader::new(File::open("mw2prm.cfg")?);
            file.seek(SeekFrom::Start(280))?;
            for byte in file.bytes() {
                let byte = byte?;
                if byte == 0 {
                    break;
                }
                buffer.push(byte);
            }
            let cmd_line = String::from_utf8_lossy(&buffer).to_string();
            format!("{} {}", cmd_line, "/V=5")
        };

        result = start_sim(&cmd_line)?;

        if result == 255 {
            return Ok(());
        }

        result = start_shell("sim")?;
    }
}
