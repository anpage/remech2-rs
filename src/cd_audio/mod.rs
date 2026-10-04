use std::{fs, path::PathBuf};

use anyhow::{Result, bail};
use rodio::{Decoder, OutputStream, OutputStreamBuilder, Sink};
use tracing::{error, info, warn};

use crate::settings::SETTINGS;

const MAX_CD_VOLUME: i32 = 65535;
const MAX_TRACK: i32 = 99;

#[repr(C)]
#[derive(Clone, Copy, Debug, Default, Eq, PartialEq)]
pub enum AudioCdStatus {
    #[default]
    Stopped = 2,
    Playing = 3,
    Paused = 4,
    Error = 5,
}

pub struct TrackInfo {
    pub number: i32,
    pub path: PathBuf,
}

pub struct CdAudioPlayer {
    stream: OutputStream,
    sink: Sink,
    tracks: Vec<TrackInfo>,
    state: AudioCdStatus,
    volume: i32,
}

impl CdAudioPlayer {
    pub fn new() -> Result<Self> {
        let stream = OutputStreamBuilder::from_default_device()
            .and_then(|b| b.with_channels(2).open_stream())
            .inspect_err(|e| error!("failed to open audio stream for bg music: {e}"))?;
        let sink = Sink::connect_new(stream.mixer());
        let tracks = Self::scan_for_tracks()
            .inspect_err(|e| error!("failed to scan for music tracks: {e}"))?;
        Ok(Self {
            stream,
            sink,
            tracks,
            state: Default::default(),
            volume: MAX_CD_VOLUME,
        })
    }

    fn scan_for_tracks() -> Result<Vec<TrackInfo>> {
        let mut tracks = vec![];

        let music_dir = PathBuf::from(
            SETTINGS
                .get(Some("audio"), "music_path")
                .unwrap_or_else(|| "Music".to_owned()),
        );

        if !music_dir.is_dir() {
            bail!("no music directory found");
        }

        for entry in std::fs::read_dir(&music_dir)? {
            let entry = entry?;
            let path = entry.path();
            if !path.is_file() {
                continue;
            }

            let extension = path
                .extension()
                .and_then(|e| e.to_str())
                .map(|e| e.to_ascii_lowercase())
                .unwrap_or_default();
            if extension != "wav" && extension != "ogg" && extension != "mp3" {
                continue;
            }

            let track_number: Option<i32> =
                path.file_stem().and_then(|s| s.to_str()).and_then(|stem| {
                    stem.get(..5)
                        .filter(|prefix| prefix.eq_ignore_ascii_case("track"))
                        .and_then(|_| stem.get(5..))
                        .and_then(|num| num.parse().ok())
                });
            let Some(track_number) = track_number else {
                continue;
            };

            if !(2..=MAX_TRACK).contains(&track_number) {
                warn!("skipping track {}: {}", track_number, path.display());
                continue;
            }

            if tracks.iter().any(|t: &TrackInfo| t.number == track_number) {
                warn!(
                    "skipping duplicate track {}: {}",
                    track_number,
                    path.display()
                );
                continue;
            }

            info!("track {} found: {}", track_number, path.display());
            tracks.push(TrackInfo {
                number: track_number,
                path,
            });
        }

        if tracks.is_empty() {
            bail!("no music tracks found")
        }

        tracks.sort_unstable_by_key(|t| t.number);

        Ok(tracks)
    }

    pub fn has_track(&self, track_number: i32) -> bool {
        self.tracks.iter().any(|t| t.number == track_number)
    }

    /// Plays a track from start to finish
    pub fn play(&mut self, track_number: i32) -> Result<()> {
        let Some(track) = self.tracks.iter().find(|t| t.number == track_number) else {
            bail!("track not found: {}", track_number);
        };

        let decoder = Decoder::try_from(fs::File::open(&track.path)?)?;

        self.sink.stop();
        self.sink = Sink::connect_new(self.stream.mixer());
        self.sink.set_volume(self.sink_volume());
        self.sink.append(decoder);
        self.sink.play();
        self.state = AudioCdStatus::Playing;

        Ok(())
    }

    pub fn pause(&mut self) {
        if self.sink.empty() {
            return;
        }
        self.sink.pause();
        self.state = AudioCdStatus::Paused;
    }

    pub fn resume(&mut self) {
        if self.sink.empty() {
            return;
        }
        self.sink.play();
        self.state = AudioCdStatus::Playing;
    }

    pub fn set_volume(&mut self, volume: i32) {
        self.volume = volume.clamp(0, MAX_CD_VOLUME);
        self.sink.set_volume(self.sink_volume());
    }

    fn sink_volume(&self) -> f32 {
        let linear = (self.volume as f32 / MAX_CD_VOLUME as f32).clamp(0.0, 1.0);
        linear * linear
    }

    pub fn state(&mut self) -> AudioCdStatus {
        if self.sink.empty() {
            self.state = AudioCdStatus::Stopped;
        }
        self.state
    }

    pub fn stop(&mut self) {
        self.sink.stop();
        self.state = AudioCdStatus::Stopped;
    }
}
