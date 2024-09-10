use std::{
    path::{Path, PathBuf},
    sync::{Arc, Mutex},
};

use anyhow::{bail, Result};

use super::{dll_check, Action, Stage};

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
    cd_drive_path: PathBuf,
    missing_files: Vec<MissingFile>,
    copying_files: bool,
    copying_status: Arc<Mutex<CopyStatus>>,
    copying_error: Option<CopyError>,
}

impl FileCheck {
    pub fn new<P: AsRef<Path>>(cd_drive_path: P) -> Self {
        let missing_files = check_files(".");
        Self {
            cd_drive_path: cd_drive_path.as_ref().to_path_buf(),
            missing_files,
            copying_files: false,
            copying_status: Arc::new(Mutex::new(CopyStatus::Copying((None, 0.0)))),
            copying_error: None,
        }
    }

    fn start_copy(&mut self) {
        self.copying_error = None;
        self.copying_status = Arc::new(Mutex::new(CopyStatus::Copying((None, 0.0))));

        let cd_drive_path = self.cd_drive_path.clone();
        let status = self.copying_status.clone();
        let missing_files = self.missing_files.clone();

        std::fs::create_dir_all("GIDDI").unwrap();
        std::fs::create_dir_all("KEATING").unwrap();
        std::fs::create_dir_all("LAUNCH").unwrap();
        std::fs::create_dir_all("SMK").unwrap();

        std::thread::spawn(move || {
            let total_files = missing_files.len() as f32;
            for (i, file) in missing_files.into_iter().enumerate() {
                let progress = (i as f32) / total_files;
                let mut error = Some("File not found on CD".to_string());

                for cd_path in file.cd_paths {
                    let cd_file = cd_drive_path.join(Path::new(cd_path));
                    if cd_file.exists() {
                        if let Err(e) = std::fs::copy(cd_file, &file.path) {
                            tracing::error!(
                                "Error copying file {}: {:?}",
                                &file.path.to_string_lossy(),
                                e
                            );
                            error = Some(format!("{}", e));
                        } else {
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
            .fixed_pos(ctx.screen_rect().center())
            .show(ctx, |ui| {
                ui.label("An error occurred while copying the game files.");
                ui.add_space(10.0);
                ui.label(format!(
                    "Error copying {}:",
                    &self.copying_error.as_ref().unwrap().file
                ));
                ui.label(&self.copying_error.as_ref().unwrap().error);
                ui.add_space(10.0);
                ui.with_layout(egui::Layout::right_to_left(egui::Align::Min), |ui| {
                    if ui.button("Quit").clicked() {
                        quit = true;
                    }
                    if ui.button("Retry").clicked() {
                        self.start_copy();
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
                    self.missing_files = check_files(".");
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
            .fixed_pos(ctx.screen_rect().center())
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
}

impl Stage for FileCheck {
    fn ui(&mut self, ctx: &egui::Context) -> Result<Action> {
        if self.copying_files {
            return self.copy_ui(ctx);
        }

        if self.missing_files.is_empty() {
            return Ok(Action::Continue(Box::new(dll_check::DllCheck)));
        }

        enum Choice {
            Quit,
            Retry,
            No,
            Yes,
        }

        let mut choice = None;
        egui::Window::new("⚠ Missing Files")
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
                            ui.label(file.path.to_string_lossy());
                        }
                    });
                ui.add_space(10.0);
                ui.label("Would you like to install them from CD?");
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
                self.missing_files = check_files(".");
                Ok(Action::Nothing)
            }
            Some(Choice::No) => Ok(Action::Continue(Box::new(dll_check::DllCheck))),
            Some(Choice::Yes) => {
                self.start_copy();
                Ok(super::Action::Nothing)
            }
            None => Ok(super::Action::Nothing),
        }
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
        let path = path.as_ref().join(file.path);
        if !path.exists() {
            missing_files.push(MissingFile {
                path,
                cd_paths: file.cd_paths,
            });
        }
    }
}

fn check_files<P: AsRef<Path>>(base_path: P) -> Vec<MissingFile> {
    let mut missing_files = Vec::new();

    check_folder(base_path.as_ref(), list::GAME_FILES, &mut missing_files);
    check_folder(
        base_path.as_ref().join("GIDDI"),
        list::GIDDI_FILES,
        &mut missing_files,
    );
    check_folder(
        base_path.as_ref().join("KEATING"),
        list::KEATING_FILES,
        &mut missing_files,
    );
    check_folder(
        base_path.as_ref().join("LAUNCH"),
        list::LAUNCH_FILES,
        &mut missing_files,
    );
    check_folder(
        base_path.as_ref().join("SMK"),
        list::SMK_FILES,
        &mut missing_files,
    );

    missing_files
}
