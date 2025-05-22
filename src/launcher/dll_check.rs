//! Checks the integrity of certain DLL files which are patched at runtime. If they are missing
//! or have the wrong hash, ReMech2's patching will not work correctly.
//!
//! There should only ever be one version of `WAIL32.DLL` included with any copy of the game,
//! but there are multiple possible versions of `MW2.DLL` and `MW2SHELL.DLL`. ReMech2 deliberately
//! targets the DLL files included with the official 1.1 patch, which means we can download them
//! from the internet and install them if they are missing or invalid.
use std::sync::{Arc, Mutex};

use anyhow::{Result, bail};
use hex_literal::hex;
use sha2::{Digest, Sha256};

use super::{Action, Stage};

const SIM_DLL_HASH: [u8; 32] =
    hex!("6212d542f8f915a594b278ab189f20a27e522e7c08ac57ce68bf47f45b17bbb5");

const SHELL_DLL_HASH: [u8; 32] =
    hex!("1078ccb07fd45388bfd525719a8206ce7a01d6536776d30a5655ccb928900879");

const AIL_DLL_HASH: [u8; 32] =
    hex!("2a2134551ad7cc20f66172571b07d1650703db389b30dc6543fff1da1a761d85");

#[derive(Clone, Debug)]
struct DownloadError {
    error: String,
}

#[derive(Debug)]
enum DownloadStatus {
    Downloading(f32),
    Extracting(f32),
    Copying((Option<String>, f32)),
    Done,
    Error(DownloadError),
}

pub struct DllCheck {
    missing_files: Vec<String>,
    downloading_files: bool,
    downloading_status: Arc<Mutex<DownloadStatus>>,
    downloading_error: Option<DownloadError>,
}

impl DllCheck {
    pub fn new() -> Self {
        Self {
            missing_files: Self::check_files(),
            downloading_files: false,
            downloading_status: Arc::new(Mutex::new(DownloadStatus::Downloading(0.0))),
            downloading_error: None,
        }
    }

    fn check_file(file: &str, hash: &[u8; 32]) -> Result<()> {
        if !std::path::Path::new(file).exists() {
            bail!("{} is missing", file);
        }

        let file_hash = {
            let mut hasher = Sha256::new();
            let file = std::fs::read(file)?;
            hasher.update(&file);
            hasher.finalize()
        };

        if file_hash.as_slice() != hash {
            bail!("{} hash mismatch", file);
        }

        Ok(())
    }

    fn check_files() -> Vec<String> {
        let mut missing_files = Vec::new();

        if Self::check_file("MW2.DLL", &SIM_DLL_HASH).is_err() {
            missing_files.push("MW2.DLL".to_string());
        }

        if Self::check_file("MW2SHELL.DLL", &SHELL_DLL_HASH).is_err() {
            missing_files.push("MW2SHELL.DLL".to_string());
        }

        if Self::check_file("WAIL32.DLL", &AIL_DLL_HASH).is_err() {
            missing_files.push("WAIL32.DLL".to_string());
        }

        missing_files
    }

    fn start_download(&mut self) {
        self.downloading_error = None;
        self.downloading_status = Arc::new(Mutex::new(DownloadStatus::Downloading(0.0)));

        let status = self.downloading_status.clone();
        let missing_files = self.missing_files.clone();

        std::thread::spawn(move || {
            // TODO: Download the files
            let mut status = status.lock().unwrap();
            *status = DownloadStatus::Done;
        });

        self.downloading_files = true;
    }

    fn download_ui(&mut self, ctx: &egui::Context) -> Result<Action> {
        Ok(Action::Nothing)
    }
}

impl Stage for DllCheck {
    fn ui(&mut self, ctx: &egui::Context) -> Result<Action> {
        if self.downloading_files {
            return self.download_ui(ctx);
        }

        if self.missing_files.is_empty() {
            return Ok(Action::Break);
        }

        enum Choice {
            Quit,
            Retry,
            No,
            Yes,
        }

        let mut choice = None;
        egui::Window::new("⚠ Incorrect DLL Files")
            .resizable(false)
            .collapsible(false)
            .pivot(egui::Align2::CENTER_CENTER)
            .fixed_pos(ctx.screen_rect().center())
            .show(ctx, |ui| {
                ui.label("These required game files are missing:");
                ui.add_space(10.0);
                egui::ScrollArea::vertical()
                    .auto_shrink(true)
                    .max_height(100.0)
                    .show(ui, |ui| {
                        ui.allocate_space(egui::Vec2::new(ui.available_width(), 0.0));
                        for file in &self.missing_files {
                            ui.label(file);
                        }
                    });
                ui.add_space(10.0);
                ui.label("Would you like to download them?");
                ui.add_space(10.0);
                ui.with_layout(egui::Layout::right_to_left(egui::Align::Min), |ui| {
                    if ui.button("Quit").clicked() {
                        choice = Some(Choice::Quit);
                    }
                    if ui.button("Retry").clicked() {
                        choice = Some(Choice::Retry);
                    }
                    if ui.button("No").clicked() {
                        choice = Some(Choice::No);
                    }
                    if ui.button("Yes").clicked() {
                        choice = Some(Choice::Yes);
                    }
                });
            });

        match choice {
            Some(Choice::Quit) => bail!("User chose to quit"),
            Some(Choice::Retry) => {
                self.missing_files = Self::check_files();
                Ok(Action::Nothing)
            }
            Some(Choice::No) => Ok(Action::Break),
            Some(Choice::Yes) => {
                self.start_download();
                Ok(super::Action::Nothing)
            }
            None => Ok(super::Action::Nothing),
        }
    }
}
