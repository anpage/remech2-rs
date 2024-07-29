use std::path::{Path, PathBuf};

use anyhow::{bail, Result};

mod list;

pub struct FileCheck {
    missing_files: Vec<PathBuf>,
}

impl FileCheck {
    pub fn new<P: AsRef<Path>>(_cd_drive_path: P) -> Self {
        let missing_files = check_files(".");
        Self { missing_files }
    }
}

impl super::Stage for FileCheck {
    fn ui(&mut self, ctx: &egui::Context) -> Result<super::Action> {
        if self.missing_files.is_empty() {
            return Ok(super::Action::Continue(Box::new(
                super::dll_check::DllCheck,
            )));
        }

        enum Choice {
            Quit,
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
                    .auto_shrink(false)
                    .max_height(100.0)
                    .show(ui, |ui| {
                        for file in &self.missing_files {
                            ui.label(file.to_string_lossy());
                        }
                    });
                ui.add_space(10.0);
                ui.label("Would you like to install them from CD?");
                ui.add_space(10.0);
                ui.with_layout(egui::Layout::right_to_left(egui::Align::Min), |ui| {
                    if ui.button("Quit").clicked() {
                        choice = Some(Choice::Quit);
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
            Some(Choice::No) => Ok(super::Action::Continue(Box::new(
                super::dll_check::DllCheck,
            ))),
            Some(Choice::Yes) => {
                todo!()
            }
            None => Ok(super::Action::Nothing),
        }
    }
}

#[derive(Copy, Clone, Debug)]
pub struct GameFile {
    path: &'static str,
    cd_paths: &'static [&'static str],
}

fn check_folder<P: AsRef<Path>>(path: P, files: &[GameFile], missing_files: &mut Vec<PathBuf>) {
    for file in files {
        let path = path.as_ref().join(file.path);
        if !path.exists() {
            missing_files.push(path);
        }
    }
}

fn check_files<P: AsRef<Path>>(base_path: P) -> Vec<PathBuf> {
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
