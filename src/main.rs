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

use remech2_sys::shared::{MissionLaunch, MissionReport};

use crate::display::Overlay;

mod about;
mod ailrs;
mod app;
mod cd_audio;
mod display;
mod drawmode;
mod elapsed;
mod files;
mod heap;
mod input;
mod launcher;
mod log;
mod mech_rand;
mod messages;
mod midi_source;
mod resolution;
mod settings;
mod shell;
mod sim;
mod xmi;

fn start_shell(intro_or_sim: &str) -> Result<i32> {
    display::set_overlay(app::with(|app| {
        Overlay::Shell(Box::new(shell::OverlayUi::new(app.egui_ctx())))
    }));
    let result = shell::run(intro_or_sim);
    display::set_overlay(None);
    result
}

fn start_sim(cmd_line: &str, launch: &MissionLaunch, report: &mut MissionReport) -> Result<i32> {
    display::set_overlay(app::with(|app| {
        Overlay::Sim(Box::new(sim::OverlayUi::new(app.egui_ctx())))
    }));
    let result = sim::run(cmd_line, launch, report);
    display::set_overlay(None);
    result
}

fn main() -> Result<()> {
    let loglevel = Level::from(settings::get().debug.log_level);

    let filter = filter::Targets::new().with_target("remech2", loglevel);
    tracing_subscriber::registry()
        .with(tracing_subscriber::fmt::layer())
        .with(filter)
        .init();
    settings::log_load_problems();

    let args: Vec<String> = env::args().collect();

    let mut app = app::App::new()?;

    if !launcher::Launcher::new().run(&mut app)? {
        // The window was closed
        return Ok(());
    }

    app::install(app);

    if args.len() > 1 {
        // launch the sim with the given cmdline
        start_sim(
            &args[1..].join(" "),
            &MissionLaunch::default(),
            &mut MissionReport::default(),
        )?;
        return Ok(());
    }

    let mut result = start_shell("intro")?;

    loop {
        if result == 255 {
            return Ok(());
        }

        let mut report = MissionReport::default();
        result = shell::with_mission_launch(|cmd_line, launch| {
            start_sim(&format!("{cmd_line} /V=5"), launch, &mut report)
        })??;
        shell::set_mission_report(report);

        if result == 255 {
            return Ok(());
        }

        result = start_shell("sim")?;
    }
}
