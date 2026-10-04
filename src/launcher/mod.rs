use anyhow::Result;

use crate::app::App;

mod dll_check;
mod file_check;

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
