use std::{
    fs,
    io::{BufWriter, Write},
    path::{Path, PathBuf},
    sync::{
        Arc, Mutex,
        mpsc::{self, Receiver, TryRecvError},
    },
};

use anyhow::{Context, Result, bail};
use gamedisc::audio::{AudioSource, FlacSink};
use tracing::warn;

use crate::{
    cd_audio, files,
    launcher::{
        Action, CD_MARKER_FILE, Disc, DiscPick, MusicSource, Stage, centered_window, detect_discs,
        disc_picker_ui,
    },
    settings,
};

enum RipStatus {
    Ripping(String, f32),
    Done,
    Failed(String),
}

enum State {
    Offer(MusicSource),
    Missing,
    Ripping(Arc<Mutex<RipStatus>>),
    Error(String),
}

pub struct MusicCheck {
    state: State,
    missing: Vec<i32>,
    dont_ask: bool,
    picked_image: Option<Receiver<Option<PathBuf>>>,
    discs: Vec<Disc>,
    choosing_disc: bool,
}

impl MusicCheck {
    pub fn new(installed_from: Option<MusicSource>) -> Self {
        let missing = cd_audio::missing_used_tracks();
        let state = match installed_from {
            Some(source) if !missing.is_empty() && source.may_have_music() => State::Offer(source),
            _ => State::Missing,
        };

        Self {
            state,
            missing,
            dont_ask: false,
            picked_image: None,
            discs: detect_discs(),
            choosing_disc: false,
        }
    }

    fn start_rip(&mut self, source: MusicSource) {
        let status = Arc::new(Mutex::new(RipStatus::Ripping("Ripping...".into(), 0.0)));
        self.state = State::Ripping(status.clone());
        std::thread::spawn(move || {
            let report =
                |label, progress| *status.lock().unwrap() = RipStatus::Ripping(label, progress);
            *status.lock().unwrap() = match rip_music(&source, &report) {
                Ok(()) => RipStatus::Done,
                Err(e) => RipStatus::Failed(format!("{e:#}")),
            };
        });
    }

    fn offer_ui(&mut self, ctx: &egui::Context) -> Result<Action> {
        let (mut rip, mut skip) = (false, false);
        centered_window("🎵 Rip the Music?", ctx).show(ctx, |ui| {
            let what = match self.state {
                State::Offer(MusicSource::Disc(_)) => "This disc",
                _ => "This disc image",
            };
            ui.label(format!("{what} contains the game's music. Rip it now?"));
            ui.label("This could take a minute or two.");
            ui.add_space(10.0);
            ui.with_layout(egui::Layout::right_to_left(egui::Align::Min), |ui| {
                skip = ui.button("Skip").clicked();
                rip = ui.button("Rip").clicked();
            });
        });

        if rip && let State::Offer(image) = std::mem::replace(&mut self.state, State::Missing) {
            self.start_rip(image);
        }

        Ok(if skip { Action::Break } else { Action::Nothing })
    }

    fn ripping_ui(&mut self, ctx: &egui::Context) -> Result<Action> {
        let State::Ripping(status) = &self.state else {
            return Ok(Action::Nothing);
        };

        let status = Arc::clone(status);

        let (label, progress) = match &*status.lock().unwrap() {
            RipStatus::Ripping(label, progress) => (label.clone(), *progress),
            RipStatus::Done => {
                self.missing = cd_audio::missing_used_tracks();
                self.state = State::Missing;
                return Ok(Action::Nothing);
            }
            RipStatus::Failed(error) => {
                self.state = State::Error(error.clone());
                return Ok(Action::Nothing);
            }
        };

        centered_window("🎵 Ripping the Music", ctx).show(ctx, |ui| {
            ui.label("Please wait while the music is ripped.");
            ui.add_space(10.0);
            ui.label(label);
            ui.add_space(10.0);
            ui.add(
                egui::ProgressBar::new(progress)
                    .show_percentage()
                    .animate(true),
            );
        });

        Ok(Action::Nothing)
    }

    fn error_ui(&mut self, ctx: &egui::Context) -> Result<Action> {
        let State::Error(error) = &self.state else {
            return Ok(Action::Nothing);
        };

        let mut back = false;

        centered_window("🚫 Error Ripping the Music", ctx).show(ctx, |ui| {
            ui.label("Ripping the music failed:");
            ui.label(error);
            ui.add_space(10.0);
            ui.with_layout(egui::Layout::right_to_left(egui::Align::Min), |ui| {
                back = ui.button("Back").clicked();
            });
        });

        if back {
            self.missing = cd_audio::missing_used_tracks();
            self.discs = detect_discs();
            self.state = State::Missing;
        }

        Ok(Action::Nothing)
    }

    fn pick_image(&mut self) {
        let (sender, receiver) = mpsc::channel();

        std::thread::spawn(move || {
            let file = rfd::FileDialog::new()
                .set_title("Locate a MechWarrior 2 disc image")
                .add_filter("Cue sheets", &["cue"])
                .pick_file();
            let _ = sender.send(file);
        });

        self.picked_image = Some(receiver);
    }

    fn missing_ui(&mut self, ctx: &egui::Context) -> Result<Action> {
        enum Choice {
            Continue,
            Retry,
            OpenFolder,
            Image,
            Disc,
        }

        if let Some(receiver) = &self.picked_image {
            match receiver.try_recv() {
                Ok(Some(image)) => {
                    self.picked_image = None;
                    self.start_rip(MusicSource::Image(image));
                    return Ok(Action::Nothing);
                }
                Ok(None) | Err(TryRecvError::Disconnected) => self.picked_image = None,
                Err(TryRecvError::Empty) => {}
            }
        }

        let picking = self.picked_image.is_some();
        let mut choice = None;
        centered_window("🎵 Missing Music", ctx).show(ctx, |ui| {
            if self.missing.len() == cd_audio::USED_TRACKS.len() {
                ui.label("The in-game music is missing.");
            } else {
                let list = self.missing.iter().map(i32::to_string).collect::<Vec<_>>().join(", ");
                ui.label(
                    format!(
                        "Some of the in-game music is missing ({} {list}).",
                        if self.missing.len() == 1 { "track" } else { "tracks" }
                    )
                );
            }

            ui.add_space(5.0);
            ui.label("The original game played music from CD audio tracks.");
            ui.add_space(5.0);
            ui.label(
                "Rip the music or copy track02 to track27 (FLAC, OGG, MP3 or WAV) into the music folder."
            );
            ui.add_space(5.0);
            ui.colored_label(
                ui.visuals().warn_fg_color,
                "⚠ Continuing without installing the music will result in missions missing music entirely.",
            );
            ui.add_space(10.0);
            ui.checkbox(&mut self.dont_ask, "Don't ask again");
            ui.add_space(10.0);

            if picking {
                ui.disable();
            }

            ui.with_layout(egui::Layout::right_to_left(egui::Align::Min), |ui| {
                if ui.button("Continue").clicked() {
                    choice = Some(Choice::Continue);
                }
                if ui.button("Retry").clicked() {
                    choice = Some(Choice::Retry);
                }
                if ui.button("Open Music Folder").clicked() {
                    choice = Some(Choice::OpenFolder);
                }
                if ui.button("Image…").clicked() {
                    choice = Some(Choice::Image);
                }
                if !self.discs.is_empty() && ui.button("Disc…").clicked() {
                    choice = Some(Choice::Disc);
                }
            });
        });

        match choice {
            Some(Choice::Continue) => {
                if self.dont_ask {
                    settings::update(|s| s.audio.ask_for_music = false);
                }
                Ok(Action::Break)
            }
            Some(Choice::Retry) => {
                self.missing = cd_audio::missing_used_tracks();
                self.discs = detect_discs();
                Ok(Action::Nothing)
            }
            Some(Choice::OpenFolder) => {
                let folder = files::resolve(&settings::get().audio.music_path);
                if let Err(e) = open_folder(&folder) {
                    warn!("couldn't open {}: {e}", folder.display());
                }
                Ok(Action::Nothing)
            }
            Some(Choice::Image) => {
                self.pick_image();
                Ok(Action::Nothing)
            }
            Some(Choice::Disc) => {
                if let [disc] = self.discs.as_slice() {
                    self.start_rip(MusicSource::Disc(disc.clone()));
                } else {
                    self.choosing_disc = true;
                }
                Ok(Action::Nothing)
            }
            None => Ok(Action::Nothing),
        }
    }

    fn disc_picker_ui(&mut self, ctx: &egui::Context) -> Result<Action> {
        match disc_picker_ui(
            ctx,
            &self.discs,
            "Which disc should the music be ripped from?",
        ) {
            Some(DiscPick::Disc(i)) => {
                self.choosing_disc = false;
                self.start_rip(MusicSource::Disc(self.discs[i].clone()));
            }
            Some(DiscPick::Cancel) => self.choosing_disc = false,
            None => {}
        }
        Ok(Action::Nothing)
    }
}

impl Stage for MusicCheck {
    fn ui(&mut self, ctx: &egui::Context) -> Result<Action> {
        match self.state {
            State::Offer(_) => self.offer_ui(ctx),
            State::Ripping(_) => self.ripping_ui(ctx),
            State::Error(_) => self.error_ui(ctx),
            State::Missing if self.missing.is_empty() || !settings::get().audio.ask_for_music => {
                Ok(Action::Break)
            }
            State::Missing if self.choosing_disc => self.disc_picker_ui(ctx),
            State::Missing => self.missing_ui(ctx),
        }
    }
}

fn rip_music(source: &MusicSource, report: &dyn Fn(String, f32)) -> Result<()> {
    let audio = match source {
        MusicSource::Image(path) => {
            if !gamedisc::open_image(path)?.has_file(CD_MARKER_FILE)? {
                bail!("This is not a supported disc image");
            }
            AudioSource::open_image(path)?.context("This image has no audio tracks")?
        }
        MusicSource::Disc(disc) => {
            let audio = AudioSource::open_drive(&disc.volume.device)
                .map_err(|e| with_permission_hint(e.into()))?;
            if audio.audio_tracks().is_empty() {
                bail!("This disc has no audio tracks");
            }
            audio
        }
    };

    let present = cd_audio::present_tracks();
    let tracks: Vec<u8> = audio
        .audio_tracks()
        .into_iter()
        .filter(|n| !present.contains(&i32::from(*n)))
        .collect();

    let music_dir = files::resolve(&settings::get().audio.music_path);
    fs::create_dir_all(&music_dir)?;

    for (i, &number) in tracks.iter().enumerate() {
        let label = format!("Ripping track {number} ({} of {})", i + 1, tracks.len());
        let path = music_dir.join(format!("track{number:02}.flac"));

        let result = (|| -> Result<()> {
            let mut sink = FlacSink::new(BufWriter::new(fs::File::create(&path)?));

            audio.rip(number, &mut sink, &mut |done, total| {
                let within = done as f32 / total as f32;
                report(label.clone(), (i as f32 + within) / tracks.len() as f32);
            })?;

            sink.into_inner().flush()?;

            Ok(())
        })();

        if let Err(e) = result {
            let _ = fs::remove_file(&path);
            return Err(e);
        }
    }

    Ok(())
}

fn with_permission_hint(e: anyhow::Error) -> anyhow::Error {
    let denied = e.chain().any(|cause| {
        cause
            .downcast_ref::<std::io::Error>()
            .is_some_and(|io| io.kind() == std::io::ErrorKind::PermissionDenied)
    });
    if denied && cfg!(target_os = "linux") {
        e.context("You may need to be in the `optical` or `cdrom` group.")
    } else {
        e
    }
}

fn open_folder(path: &Path) -> std::io::Result<()> {
    fs::create_dir_all(path)?;
    let program = if cfg!(windows) {
        "explorer"
    } else {
        "xdg-open"
    };
    let mut child = std::process::Command::new(program).arg(path).spawn()?;
    std::thread::spawn(move || child.wait());
    Ok(())
}
