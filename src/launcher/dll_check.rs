//! Checks the integrity of certain DLL files which are patched at runtime. If they are missing
//! or have the wrong hash, ReMech2's patching will not work correctly.
//!
//! There should only ever be one version of `WAIL32.DLL` included with any copy of the game,
//! but there are multiple possible versions of `MW2.DLL` and `MW2SHELL.DLL`. ReMech2 deliberately
//! targets the DLL files included with the official 1.1 patch, which means we can download them
//! from the internet and install them if they are missing or invalid.
use std::{
    fs,
    io::{Cursor, Write},
    sync::{Arc, Mutex},
};

use anyhow::{Result, bail};
use futures_util::StreamExt;
use hex_literal::hex;
use sha2::{Digest, Sha256};
use tokio::runtime::Runtime;
use unarc_rs::arj::arj_archive::ArjArchieve as ArjArchive;

use super::{Action, Stage};

const SIM_DLL_HASH: [u8; 32] =
    hex!("6212d542f8f915a594b278ab189f20a27e522e7c08ac57ce68bf47f45b17bbb5");

const SHELL_DLL_HASH: [u8; 32] =
    hex!("1078ccb07fd45388bfd525719a8206ce7a01d6536776d30a5655ccb928900879");

const AIL_DLL_HASH: [u8; 32] =
    hex!("2a2134551ad7cc20f66172571b07d1650703db389b30dc6543fff1da1a761d85");

const MW2_PATCH_HASH: [u8; 32] =
    hex!("7846d3ac5885887e9371cfed7ababf4632b914fbec7469a5e2cc92348e0b5bd8");

const MW2_PATCH_URLS: [&str; 2] = [
    "https://web.archive.org/web/19961025085815/http://www2.activision.com/CustomerSupport/MW2PATCH.EXE",
    "https://archive.org/download/mw2patch/MW2PATCH.EXE",
];

#[derive(Clone, Debug)]
struct DownloadError {
    error: String,
}

#[derive(Debug)]
enum DownloadStatus {
    Downloading(f32),
    Extracting(f32),
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

        let rt = Runtime::new().expect("Unable to create Runtime");
        let _enter = rt.enter();

        std::thread::spawn(move || {
            rt.block_on(async {
                let download_file = async |url: &str| -> Result<Vec<u8>, String> {
                    let Ok(request) = reqwest::get(url).await else {
                        return Err("Failed to download patch".to_string());
                    };

                    let total_size = request.content_length().unwrap_or(1_001_315);
                    let mut downloaded: u64 = 0;

                    let mut stream = request.bytes_stream();
                    let mut archive = Vec::<u8>::with_capacity(total_size as usize);

                    while let Some(item) = stream.next().await {
                        match item {
                            Ok(bytes) => {
                                archive.write_all(&bytes).unwrap();
                                downloaded += bytes.len() as u64;
                                let mut status = status.lock().unwrap();
                                *status = DownloadStatus::Downloading(
                                    downloaded as f32 / total_size as f32,
                                );
                            }
                            Err(e) => {
                                return Err(e.to_string());
                            }
                        }
                    }

                    let mut hasher = Sha256::new();
                    hasher.update(&archive);
                    let file_hash = hasher.finalize();
                    if file_hash.as_slice() != &MW2_PATCH_HASH {
                        return Err("Downloaded file hash mismatch".to_string());
                    }

                    Ok(archive)
                };

                let mut archive: Result<Vec<u8>, String> = Err("No URLs".to_string());
                for url in MW2_PATCH_URLS.iter() {
                    let result = download_file(url).await;
                    archive = result;
                    if archive.is_ok() {
                        break;
                    }
                }

                let archive = match archive {
                    Err(e) => {
                        let mut status = status.lock().unwrap();
                        *status = DownloadStatus::Error(DownloadError {
                            error: format!("Failed to download patch: {e}"),
                        });
                        return;
                    }
                    Ok(archive) => archive,
                };

                {
                    let mut status = status.lock().unwrap();
                    *status = DownloadStatus::Extracting(0.0);
                }

                {
                    let mut archive = ArjArchive::new(Cursor::new(&archive[0x1853..])).unwrap();
                    if let Ok(Some(header)) = archive.get_next_entry() {
                        let buffer = archive.read(&header).unwrap();
                        if let Err(e) = fs::write("MW2SHELL.DLL", buffer) {
                            let mut status = status.lock().unwrap();
                            *status = DownloadStatus::Error(DownloadError {
                                error: format!("Failed to write MW2SHELL.DLL: {e}"),
                            });
                            return;
                        }
                    }
                }

                {
                    let mut status = status.lock().unwrap();
                    *status = DownloadStatus::Extracting(0.5);
                }

                {
                    let mut archive = ArjArchive::new(Cursor::new(&archive[0x9E311..])).unwrap();
                    if let Ok(Some(header)) = archive.get_next_entry() {
                        let buffer = archive.read(&header).unwrap();
                        if let Err(e) = fs::write("MW2.DLL", buffer) {
                            let mut status = status.lock().unwrap();
                            *status = DownloadStatus::Error(DownloadError {
                                error: format!("Failed to write MW2.DLL: {e}"),
                            });
                            return;
                        }
                    }
                }

                {
                    let mut status = status.lock().unwrap();
                    *status = DownloadStatus::Done;
                }
            })
        });

        self.downloading_files = true;
    }

    fn download_error_ui(&mut self, ctx: &egui::Context) -> Result<Action> {
        let mut quit = false;
        egui::Window::new("🚫 Error Installing Patch")
            .resizable(false)
            .collapsible(false)
            .pivot(egui::Align2::CENTER_CENTER)
            .fixed_pos(ctx.screen_rect().center())
            .show(ctx, |ui| {
                ui.label(&self.downloading_error.as_ref().unwrap().error);
                ui.with_layout(egui::Layout::right_to_left(egui::Align::Min), |ui| {
                    if ui.button("Quit").clicked() {
                        quit = true;
                    }
                    if ui.button("Retry").clicked() {
                        self.start_download();
                    }
                });
            });

        if quit {
            bail!("User chose to quit");
        }

        Ok(Action::Nothing)
    }

    fn download_ui(&mut self, ctx: &egui::Context) -> Result<Action> {
        if self.downloading_error.is_some() {
            return self.download_error_ui(ctx);
        }

        let status = self.downloading_status.lock().unwrap();
        match *status {
            DownloadStatus::Downloading(progress) => {
                egui::Window::new("Downloading 1.1 Patch")
                    .resizable(false)
                    .collapsible(false)
                    .pivot(egui::Align2::CENTER_CENTER)
                    .fixed_pos(ctx.screen_rect().center())
                    .show(ctx, |ui| {
                        ui.label("Downloading 1.1 patch...");
                        ui.add_space(10.0);
                        ui.label(format!("{:.0}%", progress * 100.0));
                        ui.add(egui::ProgressBar::new(progress).animate(true));
                    });
            }
            DownloadStatus::Extracting(progress) => {
                egui::Window::new("Extracting DLLs")
                    .resizable(false)
                    .collapsible(false)
                    .pivot(egui::Align2::CENTER_CENTER)
                    .fixed_pos(ctx.screen_rect().center())
                    .show(ctx, |ui| {
                        ui.label("Extracting DLL files...");
                        ui.add_space(10.0);
                        ui.label(format!("{:.0}%", progress * 100.0));
                        ui.add(egui::ProgressBar::new(progress));
                    });
            }
            DownloadStatus::Done => {
                self.downloading_files = false;
                self.missing_files = Self::check_files();
                return Ok(Action::Nothing);
            }
            DownloadStatus::Error(ref error) => {
                self.downloading_error = Some(error.clone());
            }
        }

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
