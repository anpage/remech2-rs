use binding::globals;
use mw2_sys::shell;

mod audio_sample;
mod audio_subsystem;
mod interface;
mod midi_sequence;

globals!(
    /// Master SFX volume, 0..=0x10000
    pub(in crate::shell) static G_EFFECTS_VOLUME: i32 = shell::g_soundConfig.m_effectsVolume;
    /// Master MIDI volume, 0..=0x10000
    static G_MIDI_VOLUME: i32 = shell::g_soundConfig.m_midiVolume;
);
