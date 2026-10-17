use std::path::PathBuf;

use anyhow::Result;

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

fn detect_discs() -> Vec<PathBuf> {
    gamedisc::volumes::optical_volumes()
        .into_iter()
        .map(|volume| volume.mount_point)
        .filter(|mount_point| files::resolve_in(mount_point, CD_MARKER_FILE).exists())
        .collect()
}

fn centered_window(title: &str, ctx: &egui::Context) -> egui::Window<'static> {
    egui::Window::new(title)
        .resizable(false)
        .collapsible(false)
        .pivot(egui::Align2::CENTER_CENTER)
        .fixed_pos(ctx.content_rect().center())
}
