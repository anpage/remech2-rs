use std::{fs::File, path::PathBuf};

use anyhow::{bail, Result};
use windows::{
    core::PCSTR,
    Win32::{
        Storage::FileSystem::{GetDriveTypeA, GetLogicalDriveStringsA},
        System::WindowsProgramming::DRIVE_CDROM,
    },
};

use super::{Action, Stage};

pub struct CdCheck {
    cd_path: Option<PathBuf>,
}

impl CdCheck {
    pub fn new() -> Self {
        Self {
            cd_path: Self::cd_check(),
        }
    }

    fn cd_check() -> Option<PathBuf> {
        let mut drive_strings = [0u8; 128];
        unsafe {
            GetLogicalDriveStringsA(Some(&mut drive_strings));
        }

        for drive in drive_strings.split(|&c| c == 0) {
            if drive.is_empty() {
                continue;
            }

            let drive_type = unsafe { GetDriveTypeA(PCSTR(drive.as_ptr())) };

            if drive_type != DRIVE_CDROM {
                continue;
            }

            let drive_letter = *drive.first().unwrap() as char;
            let path = format!("{}:\\OLD_HERC.DRV", drive_letter);
            if File::open(&path).is_ok() {
                let drive = format!("{}:\\", drive_letter);
                return Some(PathBuf::from(drive));
            }
        }

        None
    }
}

impl Stage for CdCheck {
    fn ui(&mut self, ctx: &egui::Context) -> Result<Action> {
        if let Some(path) = &self.cd_path {
            return Ok(Action::Continue(Box::new(
                super::file_check::FileCheck::new(path),
            )));
        }
        let mut should_bail = false;
        egui::Window::new("💿 Please Insert CD")
            .resizable(false)
            .collapsible(false)
            .pivot(egui::Align2::CENTER_CENTER)
            .fixed_pos(ctx.screen_rect().center())
            .show(ctx, |ui| {
                ui.label("You must insert the game's CD into your CD-ROM drive.");
                ui.add_space(10.0);
                ui.with_layout(egui::Layout::right_to_left(egui::Align::Min), |ui| {
                    if ui.button("Quit").clicked() {
                        should_bail = true;
                    }
                    if ui.button("Retry").clicked() {
                        self.cd_path = Self::cd_check();
                    }
                });
            });
        if should_bail {
            bail!("CD not found");
        }
        Ok(Action::Nothing)
    }
}
