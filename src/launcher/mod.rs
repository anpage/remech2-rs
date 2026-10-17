use std::path::{Path, PathBuf};

use anyhow::Result;
use gamedisc::{
    audio::AudioSource,
    volumes::{self, OpticalVolume},
};

use crate::{app::App, files};

mod file_check;
mod music_check;

const CD_MARKER_FILE: &str = "OLD_HERC.DRV";

enum Action {
    /// Stay on current stage
    Nothing,
    /// Move on to another stage
    Continue(Box<dyn Stage>),
    /// Exit the launcher and run the game
    Break,
}

trait Stage {
    fn ui(&mut self, ctx: &egui::Context) -> Result<Action>;
}

pub struct Launcher {
    current_stage: Box<dyn Stage>,
}

impl Launcher {
    pub fn new() -> Self {
        Launcher {
            current_stage: Box::new(file_check::FileCheck::new()),
        }
    }

    pub fn run(&mut self, app: &mut App) -> Result<bool> {
        loop {
            if app.pump() {
                return Ok(false);
            }

            let mut finished = Ok(false);
            app.present(None, |ctx| finished = self.ui(ctx));
            if finished? {
                return Ok(true);
            }
        }
    }

    fn ui(&mut self, ctx: &egui::Context) -> Result<bool> {
        match self.current_stage.ui(ctx)? {
            Action::Nothing => Ok(false),
            Action::Continue(stage) => {
                self.current_stage = stage;
                Ok(false)
            }
            Action::Break => Ok(true),
        }
    }
}

#[derive(Clone)]
enum MusicSource {
    Image(PathBuf),
    Disc(Disc),
}

impl MusicSource {
    fn may_have_music(&self) -> bool {
        match self {
            Self::Image(path) => has_audio(path),
            Self::Disc(_) => true,
        }
    }
}

fn has_audio(image: &Path) -> bool {
    AudioSource::open_image(image)
        .ok()
        .flatten()
        .is_some_and(|source| !source.audio_tracks().is_empty())
}

#[derive(Clone)]
struct Disc {
    volume: OpticalVolume,
    label: Option<String>,
}

fn detect_discs() -> Vec<Disc> {
    volumes::optical_volumes()
        .into_iter()
        .filter(|v| files::resolve_in(&v.mount_point, CD_MARKER_FILE).exists())
        .map(|volume| Disc {
            label: volume.label(),
            volume,
        })
        .collect()
}

enum DiscPick {
    Disc(usize),
    Cancel,
}

fn disc_picker_ui(ctx: &egui::Context, discs: &[Disc], question: &str) -> Option<DiscPick> {
    let mut pick = None;

    centered_window("💿 Choose a Disc", ctx).show(ctx, |ui| {
        ui.label(question);
        ui.add_space(10.0);
        for (i, disc) in discs.iter().enumerate() {
            if ui
                .button(match &disc.label {
                    Some(label) => format!("{label}: {}", disc.volume.mount_point.display()),
                    None => disc.volume.mount_point.display().to_string(),
                })
                .clicked()
            {
                pick = Some(DiscPick::Disc(i));
            }
        }
        ui.add_space(10.0);
        ui.with_layout(egui::Layout::right_to_left(egui::Align::Min), |ui| {
            if ui.button("Cancel").clicked() {
                pick = Some(DiscPick::Cancel);
            }
        });
    });

    pick
}

fn centered_window(title: &str, ctx: &egui::Context) -> egui::Window<'static> {
    egui::Window::new(title)
        .resizable(false)
        .collapsible(false)
        .pivot(egui::Align2::CENTER_CENTER)
        .fixed_pos(ctx.content_rect().center())
}
