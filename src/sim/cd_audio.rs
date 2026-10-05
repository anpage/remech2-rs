use std::sync::Mutex;

use remech2_sys::sim;
use tracing::error;

use crate::cd_audio::{AudioCdStatus, CdAudioPlayer};

struct Drive {
    player: Option<CdAudioPlayer>,
    /// The status the game last remembered
    status: AudioCdStatus,
}

impl Drive {
    fn status(&mut self) -> AudioCdStatus {
        self.player
            .as_mut()
            .map_or(AudioCdStatus::Error, CdAudioPlayer::state)
    }

    fn apply_volume(&mut self) {
        if let Some(player) = self.player.as_mut() {
            player.set_volume(unsafe { sim::g_soundConfig.m_midiVolume });
        }
    }
}

static DRIVE: Mutex<Drive> = Mutex::new(Drive {
    player: None,
    status: AudioCdStatus::Error,
});

#[unsafe(export_name = "StartCdAudio")]
pub extern "C" fn start() -> i32 {
    let mut drive = DRIVE.lock().unwrap();
    if drive.player.is_none() {
        // The player logs why there is no music
        drive.player = CdAudioPlayer::new().ok();
    }

    drive.status = drive.status();
    drive.apply_volume();
    i32::from(drive.player.is_some())
}

#[unsafe(export_name = "DeInitCdAudio")]
pub extern "C" fn deinit() {
    DRIVE.lock().unwrap().player = None;
}

#[unsafe(export_name = "IsCdAudioInitialized")]
pub extern "C" fn is_initialized() -> i32 {
    i32::from(DRIVE.lock().unwrap().player.is_some())
}

#[unsafe(export_name = "IsCdTrackOnDisc")]
pub extern "C" fn is_track_on_disc(track: i32) -> i32 {
    let drive = DRIVE.lock().unwrap();
    let on_disc = drive
        .player
        .as_ref()
        .is_some_and(|player| player.has_track(track));
    i32::from(on_disc)
}

#[unsafe(export_name = "GetCdStatus")]
pub extern "C" fn status() -> i32 {
    DRIVE.lock().unwrap().status() as i32
}

#[unsafe(export_name = "RefreshCdStatus")]
pub extern "C" fn refresh_status() -> i32 {
    let mut drive = DRIVE.lock().unwrap();
    drive.status = drive.status();
    drive.status as i32
}

#[unsafe(export_name = "PollCdDrive")]
pub extern "C" fn poll() -> i32 {
    let mut drive = DRIVE.lock().unwrap();
    if drive.player.is_none() {
        return 0;
    }

    let status = drive.status();
    let changed = status != drive.status;
    drive.status = status;
    i32::from(changed)
}

#[unsafe(export_name = "PlayCdTrack")]
pub extern "C" fn play_track(track: i32) {
    if let Some(player) = DRIVE.lock().unwrap().player.as_mut()
        && let Err(e) = player.play(track)
    {
        error!("couldn't play music track {track}: {e:#}");
    }
}

#[unsafe(export_name = "CdAudioTogglePaused")]
pub extern "C" fn toggle_paused() {
    if let Some(player) = DRIVE.lock().unwrap().player.as_mut() {
        match player.state() {
            AudioCdStatus::Playing => player.pause(),
            AudioCdStatus::Paused => player.resume(),
            AudioCdStatus::Stopped | AudioCdStatus::Error => {}
        }
    }
}

#[unsafe(export_name = "StopCdAudioAndWait")]
pub extern "C" fn stop() {
    if let Some(player) = DRIVE.lock().unwrap().player.as_mut() {
        player.stop();
    }
}

#[unsafe(export_name = "ApplyCdAudioVolume")]
pub extern "C" fn apply_volume() {
    DRIVE.lock().unwrap().apply_volume();
}
