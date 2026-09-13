use std::sync::Mutex;

use binding::{globals, macros::hook, patches};
use windows::{
    Win32::Media::Multimedia::{
        MCI_FORMAT_TMSF, MCI_FROM, MCI_MODE_OPEN, MCI_MODE_PAUSE, MCI_MODE_PLAY, MCI_MODE_STOP,
        MCI_OPEN, MCI_OPEN_PARMSA, MCI_OPEN_TYPE, MCI_PLAY, MCI_PLAY_PARMS, MCI_SET, MCI_SET_PARMS,
        MCI_SET_TIME_FORMAT, MCI_STATUS, MCI_STATUS_ITEM, MCI_STATUS_MODE, MCI_STATUS_PARMS,
        MCI_TO, mciSendCommandA,
    },
    core::s,
};

use crate::{
    cd_audio::{AudioCdStatus, CdAudioPlayer, MAX_TRACK, source::CdSource, tmsf::CdAudioPosition},
    settings::SETTINGS,
};

use super::MODULE;

#[repr(C)]
struct CdAudioTracks {
    first_track: u32,
    number_of_tracks: u32,
    track_positions: *mut u32,
}

globals!(
    static G_CD_AUDIO_DEVICE: u32 = 0x000aa278;
    static G_CD_AUDIO_AUX_DEVICE: i32 = 0x000aa27c;
    static G_CD_AUDIO_GLOBAL_1: u32 = 0x000beca8;
    static G_CD_AUDIO_GLOBAL_2: u32 = 0x000becac;
    static G_CD_AUDIO_INITIALIZED: u32 = 0x000aa28c;
    static G_AUDIO_CD_STATUS: AudioCdStatus = 0x000becc0;
    static G_CD_AUDIO_TRACK_DATA: CdAudioTracks = 0x000aa280;
    static G_PAUSED_CD_AUDIO_POSITION: CdAudioPosition = 0x000becb0;
    static G_CD_AUDIO_VOLUME: i32 = 0x000a14a4;
);

/// Cache the CD audio device to reuse between sim launches.
/// Windows 11 crashes if we try to close the CD audio device.
static mut CD_AUDIO_DEVICE: u32 = u32::MAX;

/// Our own CD audio player that actually plays tracks from files.
static CD_AUDIO_PLAYER: Mutex<Option<CdAudioPlayer>> = Mutex::new(None);

/// This function initializes the CD audio device for the game's background music.
/// We hook it to work around bugs in modern Windows' MCI implementation.
#[hook(rva = 0x0005a8b5)]
unsafe extern "stdcall" fn init_cd_audio() -> u32 {
    unsafe {
        let source =
            CdSource::from_str(&SETTINGS.get(Some("audio"), "cd_source").unwrap_or_default());

        if source != CdSource::Mci {
            let have_player = {
                let mut guard = CD_AUDIO_PLAYER.lock().unwrap();
                if guard.is_none() {
                    match CdAudioPlayer::new() {
                        Ok(p) => *guard = Some(p),
                        Err(e) => {
                            if source == CdSource::Files {
                                tracing::error!("cd_source=files but player init failed: {e}");
                            } else {
                                tracing::warn!("CD audio files unavailable, using MCI: {e}");
                            }
                        }
                    }
                }
                guard.is_some()
            };

            if have_player {
                G_CD_AUDIO_DEVICE.set(1);
                G_CD_AUDIO_AUX_DEVICE.set(get_cd_audio_aux_device());
                G_CD_AUDIO_INITIALIZED.set(1);
                return 0;
            }
            if source == CdSource::Files {
                // files were explicitly requested but unavailable
                return 1;
            }
        }

        if CD_AUDIO_DEVICE != u32::MAX {
            G_CD_AUDIO_DEVICE.set(CD_AUDIO_DEVICE);
            G_CD_AUDIO_INITIALIZED.set(1);
            return 0;
        }

        let mut mci_open_parms = MCI_OPEN_PARMSA {
            lpstrDeviceType: s!("cdaudio"),
            ..Default::default()
        };
        let mci_open_error = mciSendCommandA(
            0,
            MCI_OPEN,
            Some(MCI_OPEN_TYPE as usize),
            Some(&mut mci_open_parms as *mut _ as usize),
        );
        if mci_open_error != 0 {
            return 1;
        }

        G_CD_AUDIO_DEVICE.set(mci_open_parms.wDeviceID);
        CD_AUDIO_DEVICE = G_CD_AUDIO_DEVICE.get();

        let mut mci_set_parms = MCI_SET_PARMS {
            dwTimeFormat: MCI_FORMAT_TMSF,
            ..Default::default()
        };
        let mci_set_error = mciSendCommandA(
            G_CD_AUDIO_DEVICE.get(),
            MCI_SET,
            Some(MCI_SET_TIME_FORMAT as usize),
            Some(&mut mci_set_parms as *mut _ as usize),
        );
        if mci_set_error != 0 {
            return 1;
        }

        G_CD_AUDIO_AUX_DEVICE.set(get_cd_audio_aux_device());
        0
    }
}

#[hook(rva = 0x0005a7a0)]
unsafe extern "stdcall" fn get_cd_audio_aux_device() -> i32 {
    unsafe {
        if CD_AUDIO_PLAYER.lock().unwrap().is_some() {
            return 0;
        }

        original()
    }
}

/// Windows 11 was throwing an error if the CD device was closed.
/// Now we just cache the device and re-use it between sim launches.
/// This doesn't apply if we're playing music from files.
#[hook(rva = 0x0005aa0a)]
unsafe extern "stdcall" fn close_cd_audio() -> i32 {
    if let Some(player) = CD_AUDIO_PLAYER.lock().unwrap().as_mut() {
        player.stop();
    }
    0
}

#[hook(rva = 0x0005aa94)]
unsafe extern "stdcall" fn stop_cd_audio() {
    unsafe {
        if let Some(player) = CD_AUDIO_PLAYER.lock().unwrap().as_mut() {
            player.stop();
            return;
        }
        original();
    }
}

#[hook(rva = 0x0005aabe)]
unsafe extern "cdecl" fn play_cd_audio(from: u32, to: u32) {
    unsafe {
        if let Some(player) = CD_AUDIO_PLAYER.lock().unwrap().as_mut() {
            if let Err(e) = player.play_tmsf(from, to) {
                tracing::error!("play_tmsf failed: {e}");
            }
            return;
        }

        let mut flags = MCI_FROM;

        let mut mci_play_parms = MCI_PLAY_PARMS {
            dwFrom: from,
            ..Default::default()
        };
        if to != 0 {
            flags = MCI_FROM | MCI_TO;
            mci_play_parms.dwTo = to;
        }
        let _ = mciSendCommandA(
            G_CD_AUDIO_DEVICE.get(),
            MCI_PLAY,
            Some(flags as usize),
            Some(&mut mci_play_parms as *mut _ as usize),
        );
    }
}

#[hook(rva = 0x0005aa40)]
unsafe extern "stdcall" fn pause_cd_audio() {
    unsafe {
        if let Some(player) = CD_AUDIO_PLAYER.lock().unwrap().as_mut() {
            player.pause();
            return;
        }
        original();
    }
}

#[hook(rva = 0x0005aa6a)]
unsafe extern "stdcall" fn resume_cd_audio() {
    unsafe {
        if let Some(player) = CD_AUDIO_PLAYER.lock().unwrap().as_mut() {
            player.resume();
            return;
        }
        original();
    }
}

#[hook(rva = 0x0005ab0b)]
unsafe extern "stdcall" fn start_cd_audio() -> i32 {
    unsafe {
        G_CD_AUDIO_GLOBAL_1.set(0);
        G_CD_AUDIO_GLOBAL_2.set(0);

        let init_cd_audio_result = init_cd_audio();
        if init_cd_audio_result != 0 {
            return 0;
        }

        G_CD_AUDIO_INITIALIZED.set(1);

        G_AUDIO_CD_STATUS.set(get_cd_status());

        match G_AUDIO_CD_STATUS.get() {
            AudioCdStatus::_Open => {
                G_CD_AUDIO_INITIALIZED.set(0);
            }
            AudioCdStatus::Stopped => {
                get_cd_audio_tracks(G_CD_AUDIO_TRACK_DATA.ptr());
            }
            AudioCdStatus::Playing => {
                get_cd_audio_tracks(G_CD_AUDIO_TRACK_DATA.ptr());
            }
            AudioCdStatus::Paused => {
                get_cd_audio_tracks(G_CD_AUDIO_TRACK_DATA.ptr());
                get_cd_audio_position(G_PAUSED_CD_AUDIO_POSITION.ptr());
            }
            AudioCdStatus::Error | AudioCdStatus::_Unknown => {}
        }

        set_cd_audio_volume(G_CD_AUDIO_VOLUME.get());
        1
    }
}

#[hook(rva = 0x0005ac33)]
unsafe extern "cdecl" fn get_cd_status() -> AudioCdStatus {
    unsafe {
        if let Some(player) = CD_AUDIO_PLAYER.lock().unwrap().as_mut() {
            return player.state();
        }

        if G_CD_AUDIO_INITIALIZED.get() == 0 {
            return AudioCdStatus::Error;
        }

        let mut mci_status_parms = MCI_STATUS_PARMS {
            dwItem: MCI_STATUS_MODE as u32,
            ..Default::default()
        };
        let mci_status_error = mciSendCommandA(
            G_CD_AUDIO_DEVICE.get(),
            MCI_STATUS,
            Some(MCI_STATUS_ITEM as usize),
            Some(&mut mci_status_parms as *mut _ as usize),
        );
        if mci_status_error != 0 {
            deinit_cd_audio();
            return AudioCdStatus::Error;
        }

        match mci_status_parms.dwReturn as u32 {
            // Some CD emulation software reports MCI_MODE_OPEN when stopped
            MCI_MODE_OPEN | MCI_MODE_STOP => AudioCdStatus::Stopped,
            MCI_MODE_PLAY => AudioCdStatus::Playing,
            MCI_MODE_PAUSE => AudioCdStatus::Paused,
            _ => AudioCdStatus::Error,
        }
    }
}

#[hook(rva = 0x0005b49e)]
unsafe extern "cdecl" fn get_cd_audio_tracks(cd_audio_tracks: *mut CdAudioTracks) -> i32 {
    static mut TRACK_POSITIONS: [u32; MAX_TRACK as usize + 1] = [0; MAX_TRACK as usize + 1];

    unsafe {
        if let Some(player) = CD_AUDIO_PLAYER.lock().unwrap().as_mut() {
            let out = &mut *cd_audio_tracks;
            out.first_track = 1;
            out.number_of_tracks = player.tracks().last().map_or_default(|t| t.number);

            for track in out.first_track..=out.number_of_tracks + 1 {
                let pos = CdAudioPosition {
                    track,
                    ..Default::default()
                };
                (*std::ptr::addr_of_mut!(TRACK_POSITIONS))[(track - out.first_track) as usize] =
                    pos.into();
            }

            out.track_positions = std::ptr::addr_of_mut!(TRACK_POSITIONS) as *mut u32;
            return 0;
        }

        original(cd_audio_tracks)
    }
}

#[hook(rva = 0x0005b61d)]
unsafe extern "cdecl" fn get_cd_audio_position(cd_audio_position: *mut CdAudioPosition) {
    unsafe {
        if let Some(player) = CD_AUDIO_PLAYER.lock().unwrap().as_mut() {
            if !cd_audio_position.is_null() {
                *cd_audio_position = player.cd_position();
            }
            return;
        }

        original(cd_audio_position)
    }
}

#[hook(rva = 0x0005b6e5)]
unsafe extern "cdecl" fn get_cd_audio_volume() -> i32 {
    unsafe {
        if let Some(player) = CD_AUDIO_PLAYER.lock().unwrap().as_ref() {
            return player.volume();
        }

        original()
    }
}

#[hook(rva = 0x0005b734)]
unsafe extern "cdecl" fn set_cd_audio_volume(volume: i32) -> i32 {
    unsafe {
        if let Some(player) = CD_AUDIO_PLAYER.lock().unwrap().as_mut() {
            player.set_volume(volume);
            return 1;
        }

        original(volume)
    }
}

#[hook(rva = 0x0005abff)]
unsafe extern "stdcall" fn deinit_cd_audio() {
    unsafe {
        if CD_AUDIO_PLAYER.lock().unwrap().take().is_some() {
            G_CD_AUDIO_INITIALIZED.set(0);
            return;
        }

        original();
    }
}

#[hook(rva = 0x0005af07)]
unsafe extern "cdecl" fn update_cd_audio_position(position: *mut CdAudioPosition) {
    unsafe {
        if G_CD_AUDIO_INITIALIZED.get() == 0 {
            return;
        }

        let cd_status = get_cd_status();
        match cd_status {
            AudioCdStatus::_Open | AudioCdStatus::Stopped => {
                *position = CdAudioPosition::default();
            }
            AudioCdStatus::Playing => {
                get_cd_audio_position(position);
            }
            AudioCdStatus::Paused => {
                *position = G_PAUSED_CD_AUDIO_POSITION.get();
            }
            AudioCdStatus::Error | AudioCdStatus::_Unknown => {}
        }
    }
}

/// The game would reset to the first track of the CD when you unpause.
/// This starts playback again from the saved pause position instead;
#[hook(rva = 0x0005ad5e)]
unsafe extern "stdcall" fn cd_audio_toggle_paused() {
    unsafe {
        if G_CD_AUDIO_INITIALIZED.get() == 0 {
            return;
        }

        let cd_status = get_cd_status();
        match cd_status {
            AudioCdStatus::_Open => {}
            AudioCdStatus::Stopped => {
                let position = (G_PAUSED_CD_AUDIO_POSITION.get()).into();
                play_cd_audio(position, 0);
            }
            AudioCdStatus::Playing => {
                update_cd_audio_position(G_PAUSED_CD_AUDIO_POSITION.ptr());
                pause_cd_audio();
            }
            AudioCdStatus::Paused => {
                resume_cd_audio();
            }
            AudioCdStatus::Error | AudioCdStatus::_Unknown => {}
        }
    }
}

patches!(
    pub(super) static PATCHES = [
        hook init_cd_audio,
        hook get_cd_audio_aux_device,
        hook close_cd_audio,
        hook stop_cd_audio,
        hook play_cd_audio,
        hook pause_cd_audio,
        hook resume_cd_audio,
        hook start_cd_audio,
        hook get_cd_status,
        hook get_cd_audio_tracks,
        hook get_cd_audio_position,
        hook get_cd_audio_volume,
        hook set_cd_audio_volume,
        hook deinit_cd_audio,
        hook update_cd_audio_position,
        hook cd_audio_toggle_paused,
    ];
);
