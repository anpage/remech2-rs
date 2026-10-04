use std::{ffi::c_void, ptr, slice};

use tracing::warn;

use super::{
    audio_sample::AudioSample, audio_subsystem::AudioSubsystem, midi_sequence::MidiSequence,
};

/// Borrow the sound file the game passed in
unsafe fn file<'a>(data: *const c_void, size: u32) -> Option<&'a [u8]> {
    if data.is_null() {
        return None;
    }

    Some(unsafe { slice::from_raw_parts(data.cast::<u8>(), size as usize) })
}

#[unsafe(export_name = "ShellAudioSubsystem_New")]
pub extern "C" fn audio_subsystem_new() -> *mut AudioSubsystem {
    Box::into_raw(Box::new(AudioSubsystem::new()))
}

#[unsafe(export_name = "ShellAudioSubsystem_Delete")]
pub unsafe extern "C" fn audio_subsystem_delete(subsystem: *mut AudioSubsystem) {
    if !subsystem.is_null() {
        drop(unsafe { Box::from_raw(subsystem) });
    }
}

#[unsafe(export_name = "ShellAudioSubsystem_CloseDigitalDriver")]
pub unsafe extern "C" fn audio_subsystem_close_digital_driver(subsystem: *mut AudioSubsystem) {
    if let Some(subsystem) = unsafe { subsystem.as_mut() } {
        subsystem.close_digital_driver();
    }
}

#[unsafe(export_name = "ShellAudioSubsystem_ApplyMidiVolume")]
pub unsafe extern "C" fn audio_subsystem_apply_midi_volume(subsystem: *mut AudioSubsystem) {
    if let Some(subsystem) = unsafe { subsystem.as_mut() } {
        subsystem.apply_midi_volume();
    }
}

#[unsafe(export_name = "ShellMidiSequence_New")]
pub unsafe extern "C" fn midi_sequence_new(
    subsystem: *mut AudioSubsystem,
    data: *const c_void,
    size: u32,
) -> *mut MidiSequence {
    let Some(data) = (unsafe { file(data, size) }) else {
        return ptr::null_mut();
    };
    if subsystem.is_null() {
        return ptr::null_mut();
    }

    match MidiSequence::new(subsystem, data) {
        Ok(sequence) => Box::into_raw(Box::new(sequence)),
        Err(e) => {
            warn!("shell audio: couldn't load a MIDI sequence: {e:#}");
            ptr::null_mut()
        }
    }
}

#[unsafe(export_name = "ShellMidiSequence_Delete")]
pub unsafe extern "C" fn midi_sequence_delete(sequence: *mut MidiSequence) {
    if !sequence.is_null() {
        drop(unsafe { Box::from_raw(sequence) });
    }
}

#[unsafe(export_name = "ShellMidiSequence_Start")]
pub unsafe extern "C" fn midi_sequence_start(sequence: *mut MidiSequence) {
    if let Some(sequence) = unsafe { sequence.as_mut() } {
        sequence.start();
    }
}

#[unsafe(export_name = "ShellMidiSequence_Stop")]
pub unsafe extern "C" fn midi_sequence_stop(sequence: *mut MidiSequence) {
    if let Some(sequence) = unsafe { sequence.as_mut() } {
        sequence.stop();
    }
}

#[unsafe(export_name = "ShellMidiSequence_SetVolume")]
pub unsafe extern "C" fn midi_sequence_set_volume(sequence: *mut MidiSequence, volume: i32) {
    if let Some(sequence) = unsafe { sequence.as_mut() } {
        sequence.set_volume(volume);
    }
}

#[unsafe(export_name = "ShellAudioSample_New")]
pub unsafe extern "C" fn audio_sample_new(
    subsystem: *mut AudioSubsystem,
    data: *const c_void,
    size: u32,
) -> *mut AudioSample {
    let Some(data) = (unsafe { file(data, size) }) else {
        return ptr::null_mut();
    };
    if subsystem.is_null() {
        return ptr::null_mut();
    }

    match AudioSample::new(subsystem, data) {
        Ok(sample) => Box::into_raw(Box::new(sample)),
        Err(e) => {
            warn!("shell audio: couldn't load a sample: {e:#}");
            ptr::null_mut()
        }
    }
}

#[unsafe(export_name = "ShellAudioSample_Delete")]
pub unsafe extern "C" fn audio_sample_delete(sample: *mut AudioSample) {
    if !sample.is_null() {
        drop(unsafe { Box::from_raw(sample) });
    }
}

#[unsafe(export_name = "ShellAudioSample_SetFade")]
pub unsafe extern "C" fn audio_sample_set_fade(
    sample: *mut AudioSample,
    rate: i32,
    max: i32,
    start: i32,
    end: i32,
) {
    if let Some(sample) = unsafe { sample.as_mut() } {
        sample.set_fade(rate, max, start, end);
    }
}

#[unsafe(export_name = "ShellAudioSample_DoFade")]
pub unsafe extern "C" fn audio_sample_do_fade(sample: *mut AudioSample) {
    if let Some(sample) = unsafe { sample.as_mut() } {
        sample.do_fade();
    }
}

#[unsafe(export_name = "ShellAudioSample_EnableLoop")]
pub unsafe extern "C" fn audio_sample_enable_loop(sample: *mut AudioSample) {
    if let Some(sample) = unsafe { sample.as_mut() } {
        sample.enable_loop();
    }
}

#[unsafe(export_name = "ShellAudioSample_Start")]
pub unsafe extern "C" fn audio_sample_start(sample: *mut AudioSample) {
    if let Some(sample) = unsafe { sample.as_mut() } {
        sample.start();
    }
}

#[unsafe(export_name = "ShellAudioSample_Stop")]
pub unsafe extern "C" fn audio_sample_stop(sample: *mut AudioSample) {
    if let Some(sample) = unsafe { sample.as_mut() } {
        sample.stop();
    }
}

#[unsafe(export_name = "ShellAudioSample_IsPlaying")]
pub unsafe extern "C" fn audio_sample_is_playing(sample: *mut AudioSample) -> i32 {
    unsafe { sample.as_ref() }.is_some_and(AudioSample::is_playing) as i32
}

#[unsafe(export_name = "ShellAudioSample_SetVolume")]
pub unsafe extern "C" fn audio_sample_set_volume(sample: *mut AudioSample, volume: i32) {
    if let Some(sample) = unsafe { sample.as_mut() } {
        sample.set_volume(volume);
    }
}
