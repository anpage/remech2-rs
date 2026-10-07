use std::{
    path::{Path, PathBuf},
    sync::{
        Arc, Mutex,
        mpsc::{self, Receiver, TryRecvError},
    },
};

use anyhow::{Result, bail};

use super::{Action, Stage};
use crate::files::{self};

mod list;

#[derive(Clone, Debug)]
struct CopyError {
    file: String,
    error: String,
}

#[derive(Debug)]
enum CopyStatus {
    Copying((Option<String>, f32)),
    Done,
    Error(CopyError),
}

pub struct FileCheck {
    picked_folder: Option<Receiver<Option<PathBuf>>>,
    missing_files: Vec<MissingFile>,
    copying_files: bool,
    copying_status: Arc<Mutex<CopyStatus>>,
    copying_error: Option<CopyError>,
}

impl FileCheck {
    pub fn new() -> Self {
        Self {
            picked_folder: None,
            missing_files: check_files(),
            copying_files: false,
            copying_status: Arc::new(Mutex::new(CopyStatus::Copying((None, 0.0)))),
            copying_error: None,
        }
    }

    fn pick_folder(&mut self) {
        let (sender, receiver) = mpsc::channel();
        std::thread::spawn(move || {
            let folder = rfd::FileDialog::new()
                .set_title("Locate the MechWarrior 2 files")
                .pick_folder();
            let _ = sender.send(folder);
        });
        self.picked_folder = Some(receiver);
    }

    fn start_copy(&mut self, source: PathBuf) {
        self.copying_error = None;
        self.copying_status = Arc::new(Mutex::new(CopyStatus::Copying((None, 0.0))));

        let status = self.copying_status.clone();
        let missing_files = self.missing_files.clone();

        std::fs::create_dir_all(files::resolve("KEATING")).unwrap();
        std::fs::create_dir_all(files::resolve("LAUNCH")).unwrap();
        std::fs::create_dir_all(files::resolve("SMK")).unwrap();

        std::thread::spawn(move || {
            let total_files = missing_files.len() as f32;
            for (i, file) in missing_files.into_iter().enumerate() {
                let progress = (i as f32) / total_files;
                let mut error = Some("File not found in the chosen folder".to_string());

                for cd_path in file.cd_paths {
                    let cd_file = files::resolve_in(&source, cd_path);
                    if cd_file.exists() {
                        if let Err(e) = std::fs::copy(cd_file, &file.path) {
                            tracing::error!(
                                "Error copying file {}: {:?}",
                                &file.path.to_string_lossy(),
                                e
                            );
                            error = Some(format!("{}", e));
                        } else {
                            // fs::copy preserves attributes and everything on a CD is read-only
                            if let Ok(meta) = std::fs::metadata(&file.path) {
                                let mut perms = meta.permissions();
                                #[allow(clippy::permissions_set_readonly_false)]
                                perms.set_readonly(false);
                                let _ = std::fs::set_permissions(&file.path, perms);
                            }
                            error = None;
                            break;
                        }
                    }
                }

                {
                    let mut status = status.lock().unwrap();
                    if let Some(error) = error {
                        *status = CopyStatus::Error(CopyError {
                            file: file.path.to_string_lossy().into(),
                            error,
                        });
                        return;
                    } else {
                        *status = CopyStatus::Copying((
                            Some(file.path.to_string_lossy().into()),
                            progress,
                        ));
                    }
                }
            }
            let mut status = status.lock().unwrap();
            *status = CopyStatus::Done;
        });

        self.copying_files = true;
    }

    fn copy_error_ui(&mut self, ctx: &egui::Context) -> Result<Action> {
        let mut quit = false;
        egui::Window::new("🚫 Error Copying Files")
            .resizable(false)
            .collapsible(false)
            .pivot(egui::Align2::CENTER_CENTER)
            .fixed_pos(ctx.content_rect().center())
            .show(ctx, |ui| {
                ui.label("An error occurred while copying the game files.");
                ui.add_space(10.0);
                ui.label(format!(
                    "Error copying {}:",
                    self.copying_error.as_ref().unwrap().file
                ));
                ui.label(&self.copying_error.as_ref().unwrap().error);
                ui.add_space(10.0);
                ui.with_layout(egui::Layout::right_to_left(egui::Align::Min), |ui| {
                    if ui.button("Quit").clicked() {
                        quit = true;
                    }
                    if ui.button("Back").clicked() {
                        self.copying_files = false;
                        self.copying_error = None;
                        self.missing_files = check_files();
                    }
                });
            });

        if quit {
            bail!("User chose to quit");
        }

        Ok(Action::Nothing)
    }

    fn copy_ui(&mut self, ctx: &egui::Context) -> Result<Action> {
        if self.copying_error.is_some() {
            return self.copy_error_ui(ctx);
        }

        let (file, progress) = {
            let status = self.copying_status.lock().unwrap();
            match *status {
                CopyStatus::Copying(ref progress) => progress.clone(),
                CopyStatus::Done => {
                    self.copying_files = false;
                    self.missing_files = check_files();
                    return Ok(Action::Nothing);
                }
                CopyStatus::Error(ref file) => {
                    self.copying_error = Some(file.clone());
                    return Ok(Action::Nothing);
                }
            }
        };

        egui::Window::new("🗐 Copying Files")
            .resizable(false)
            .collapsible(false)
            .pivot(egui::Align2::CENTER_CENTER)
            .fixed_pos(ctx.content_rect().center())
            .show(ctx, |ui| {
                ui.label("Please wait while the game files are copied.");
                ui.add_space(10.0);
                if let Some(file) = file {
                    ui.label(format!("Copying: {}", file));
                } else {
                    ui.label("Copying...");
                }
                ui.add_space(10.0);
                let progress_bar = egui::ProgressBar::new(progress)
                    .show_percentage()
                    .animate(true);
                ui.add(progress_bar);
            });

        Ok(Action::Nothing)
    }

    fn missing_files_ui(&mut self, ctx: &egui::Context) -> Result<Action> {
        enum Choice {
            Quit,
            Retry,
            Skip,
            Locate,
        }

        if let Some(receiver) = &self.picked_folder {
            match receiver.try_recv() {
                Ok(Some(folder)) => {
                    self.picked_folder = None;
                    self.start_copy(folder);
                    return Ok(Action::Nothing);
                }
                Ok(None) | Err(TryRecvError::Disconnected) => self.picked_folder = None,
                Err(TryRecvError::Empty) => {}
            }
        }

        let picking = self.picked_folder.is_some();
        let mut choice = None;
        egui::Window::new("⚠ Missing Files")
            .resizable(false)
            .collapsible(false)
            .pivot(egui::Align2::CENTER_CENTER)
            .fixed_pos(ctx.content_rect().center())
            .show(ctx, |ui| {
                ui.label("These required game files are missing:");
                ui.add_space(10.0);
                egui::ScrollArea::vertical()
                    .auto_shrink(true)
                    .max_height(100.0)
                    .show(ui, |ui| {
                        ui.allocate_space(egui::Vec2::new(ui.available_width(), 0.0));
                        for file in &self.missing_files {
                            ui.label(file.path.to_string_lossy());
                        }
                    });
                ui.add_space(10.0);
                ui.label("Locate your MechWarrior 2 CD or install folder to copy them from.");
                ui.add_space(5.0);
                ui.label("Continuing without them will likely crash.");
                ui.add_space(10.0);
                if picking {
                    ui.disable();
                }
                ui.with_layout(egui::Layout::right_to_left(egui::Align::Min), |ui| {
                    if ui.button("Quit").clicked() {
                        choice = Some(Choice::Quit);
                    }
                    if ui.button("Retry").clicked() {
                        choice = Some(Choice::Retry);
                    }
                    if ui.button("Continue").clicked() {
                        choice = Some(Choice::Skip);
                    }
                    if ui.button("Locate…").clicked() {
                        choice = Some(Choice::Locate);
                    }
                });
            });

        match choice {
            Some(Choice::Quit) => bail!("User chose to quit"),
            Some(Choice::Retry) => {
                self.missing_files = check_files();
                Ok(Action::Nothing)
            }
            Some(Choice::Skip) => Ok(Action::Break),
            Some(Choice::Locate) => {
                self.pick_folder();
                Ok(Action::Nothing)
            }
            None => Ok(Action::Nothing),
        }
    }
}

impl Stage for FileCheck {
    fn ui(&mut self, ctx: &egui::Context) -> Result<Action> {
        if self.copying_files {
            return self.copy_ui(ctx);
        }

        if self.missing_files.is_empty() {
            return Ok(Action::Break);
        }

        self.missing_files_ui(ctx)
    }
}

#[derive(Copy, Clone, Debug)]
struct GameFile {
    path: &'static str,
    cd_paths: &'static [&'static str],
}

#[derive(Clone, Debug)]
struct MissingFile {
    path: PathBuf,
    cd_paths: &'static [&'static str],
}

fn check_folder<P: AsRef<Path>>(path: P, files: &[GameFile], missing_files: &mut Vec<MissingFile>) {
    for file in files {
        let path = files::resolve_in(path.as_ref(), file.path);
        if !path.exists() {
            missing_files.push(MissingFile {
                path,
                cd_paths: file.cd_paths,
            });
        }
    }
}

fn check_files() -> Vec<MissingFile> {
    let base_path = &files::root().game;
    let mut missing_files = Vec::new();

    check_folder(base_path, list::GAME_FILES, &mut missing_files);
    check_folder(
        files::resolve_in(base_path, "KEATING"),
        list::KEATING_FILES,
        &mut missing_files,
    );
    check_folder(
        files::resolve_in(base_path, "LAUNCH"),
        list::LAUNCH_FILES,
        &mut missing_files,
    );
    check_folder(
        files::resolve_in(base_path, "SMK"),
        list::SMK_FILES,
        &mut missing_files,
    );

    missing_files
}
