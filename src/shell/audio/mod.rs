use std::ffi::c_void;

use binding::{game_fns, globals};

use crate::shell::MODULE;

mod audio_sample;
mod audio_subsystem;
pub mod hooks;
mod midi_sequence;

// The game's `AudioSample` methods, for the screens that own samples.
//
// We already have hooks for these, but we're calling them through their original addresses for now.
game_fns!(
    /// `(this, subsystem, data, data_size)`
    pub(in crate::shell) static AUDIO_SAMPLE_NEW: unsafe extern "thiscall" fn(
        *mut c_void,
        *mut c_void,
        *mut c_void,
        i32,
    ) -> *mut c_void = 0x0003d419;
    pub(in crate::shell) static AUDIO_SAMPLE_DROP: unsafe extern "thiscall" fn(*mut c_void) =
        0x0003d50f;
    pub(in crate::shell) static AUDIO_SAMPLE_START: unsafe extern "thiscall" fn(*mut c_void) =
        0x0003d6bb;
    pub(in crate::shell) static AUDIO_SAMPLE_ENABLE_LOOP: unsafe extern "thiscall" fn(*mut c_void) =
        0x0003d67f;
    /// `(this, rate, max, start, end)`
    pub(in crate::shell) static AUDIO_SAMPLE_SET_FADE: unsafe extern "thiscall" fn(
        *mut c_void,
        i32,
        i32,
        i32,
        i32,
    ) = 0x0003d561;
    pub(in crate::shell) static AUDIO_SAMPLE_DO_FADE: unsafe extern "thiscall" fn(*mut c_void) =
        0x0003d5ca;
    pub(in crate::shell) static AUDIO_SAMPLE_IS_PLAYING: unsafe extern "thiscall" fn(
        *mut c_void,
    ) -> u32 = 0x0003d77e;
);

globals!(
    /// Master SFX volume, 0..=0x10000
    static G_EFFECTS_VOLUME: i32 = 0x0007167c;
    /// Master MIDI volume, 0..=0x10000
    static G_MIDI_VOLUME: i32 = 0x00071684;
);
