use std::ffi::c_void;

use binding::{macros::hook, patches};

use crate::shell::MODULE;

use super::{
    audio_sample::AudioSample, audio_subsystem::AudioSubsystem, midi_sequence::MidiSequence,
};

#[repr(C)]
pub struct AudioSubsystemProxy {
    audio_subsystem: *mut AudioSubsystem,
}

#[repr(C)]
pub struct AudioSampleProxy {
    audio_sample: *mut AudioSample,
}

#[repr(C)]
pub struct MidiSequenceProxy {
    midi_sequence: *mut MidiSequence,
}

#[hook(rva = 0x0003ceb0)]
unsafe extern "fastcall" fn audio_subsystem_constructor(
    this: *mut AudioSubsystemProxy,
) -> *mut AudioSubsystemProxy {
    unsafe {
        let audio_subsystem = Box::new(AudioSubsystem::new());
        (*this).audio_subsystem = Box::into_raw(audio_subsystem);
        this
    }
}

#[hook(rva = 0x0003cf5d)]
unsafe extern "fastcall" fn audio_subsystem_destructor(this: *mut AudioSubsystemProxy) {
    unsafe {
        let audio_subsystem = Box::from_raw((*this).audio_subsystem);
        drop(audio_subsystem);
    }
}

#[hook(rva = 0x0003cf89)]
unsafe extern "fastcall" fn audio_subsystem_get_digital_driver(
    _this: *mut AudioSubsystemProxy,
) -> *mut c_void {
    unreachable!()
}

#[hook(rva = 0x0003d032)]
unsafe extern "fastcall" fn audio_subsystem_close_digital_driver(this: *mut AudioSubsystemProxy) {
    unsafe {
        let mut audio_subsystem = Box::from_raw((*this).audio_subsystem);
        audio_subsystem.close_digital_driver();
        let _ = Box::into_raw(audio_subsystem);
    }
}

#[hook(rva = 0x0003d0fc)]
unsafe extern "fastcall" fn audio_subsystem_apply_midi_volume(this: *mut AudioSubsystemProxy) {
    unsafe {
        let mut audio_subsystem = Box::from_raw((*this).audio_subsystem);
        audio_subsystem.apply_midi_volume();
        let _ = Box::into_raw(audio_subsystem);
    }
}

#[hook(rva = 0x0003d0b4)]
unsafe extern "fastcall" fn audio_subsystem_get_active_sequence_count(
    this: *mut AudioSubsystemProxy,
) -> u32 {
    unsafe {
        let audio_subsystem = Box::from_raw((*this).audio_subsystem);
        let count = audio_subsystem.active_sequence_count();
        let _ = Box::into_raw(audio_subsystem);
        count
    }
}

#[hook(rva = 0x0003d419)]
unsafe extern "fastcall" fn audio_sample_constructor(
    this: *mut AudioSampleProxy,
    _: *mut c_void,
    audio_subsystem_proxy: *mut AudioSubsystemProxy,
    data: *const u8,
    data_size: i32,
) -> *mut AudioSampleProxy {
    unsafe {
        let audio_subsystem = (*audio_subsystem_proxy).audio_subsystem;
        let data = std::slice::from_raw_parts(data, data_size as usize);
        let audio_sample = Box::new(AudioSample::new(audio_subsystem, data));
        (*this).audio_sample = Box::into_raw(audio_sample);
        this
    }
}

#[hook(rva = 0x0003d50f)]
unsafe extern "fastcall" fn audio_sample_destructor(this: *mut AudioSampleProxy) {
    unsafe {
        let audio_sample = Box::from_raw((*this).audio_sample);
        drop(audio_sample);
    }
}

#[hook(rva = 0x0003d6bb)]
unsafe extern "fastcall" fn audio_sample_start(this: *mut AudioSampleProxy) {
    unsafe {
        let mut audio_sample = Box::from_raw((*this).audio_sample);
        audio_sample.start();
        let _ = Box::into_raw(audio_sample);
    }
}

#[hook(rva = 0x0003d77e)]
unsafe extern "fastcall" fn audio_sample_get_is_playing(this: *mut AudioSampleProxy) -> u32 {
    unsafe {
        let audio_sample = Box::from_raw((*this).audio_sample);
        let is_playing = audio_sample.is_playing() as u32;
        let _ = Box::into_raw(audio_sample);
        is_playing
    }
}

#[hook(rva = 0x0003d561)]
unsafe extern "fastcall" fn audio_sample_set_fade(
    this: *mut AudioSampleProxy,
    _: *mut c_void,
    rate: i32,
    max: i32,
    start: i32,
    end: i32,
) {
    unsafe {
        let mut audio_sample = Box::from_raw((*this).audio_sample);
        audio_sample.set_fade(rate, max, start, end);
        let _ = Box::into_raw(audio_sample);
    }
}

#[hook(rva = 0x0003d5ca)]
unsafe extern "fastcall" fn audio_sample_do_fade(this: *mut AudioSampleProxy) {
    unsafe {
        let mut audio_sample = Box::from_raw((*this).audio_sample);
        audio_sample.do_fade();
        let _ = Box::into_raw(audio_sample);
    }
}

#[hook(rva = 0x0003d67f)]
unsafe extern "fastcall" fn audio_sample_enable_loop(this: *mut AudioSampleProxy) {
    unsafe {
        let mut audio_sample = Box::from_raw((*this).audio_sample);
        audio_sample.enable_loop();
        let _ = Box::into_raw(audio_sample);
    }
}

#[hook(rva = 0x0003d842)]
unsafe extern "fastcall" fn audio_sample_set_loop_count(
    this: *mut AudioSampleProxy,
    _: *mut c_void,
    loop_count: i32,
) {
    unsafe {
        let mut audio_sample = Box::from_raw((*this).audio_sample);
        if loop_count == 0 {
            audio_sample.enable_loop();
        }
        let _ = Box::into_raw(audio_sample);
    }
}

#[hook(rva = 0x0003d7c0)]
unsafe extern "fastcall" fn audio_sample_set_volume(
    this: *mut AudioSampleProxy,
    _: *mut c_void,
    volume: i32,
) {
    unsafe {
        let mut audio_sample = Box::from_raw((*this).audio_sample);
        audio_sample.set_volume(volume);
        let _ = Box::into_raw(audio_sample);
    }
}

#[hook(rva = 0x0003d198)]
unsafe extern "fastcall" fn midi_sequence_constructor(
    this: *mut MidiSequenceProxy,
    _: *mut c_void,
    audio_subsystem_proxy: *mut AudioSubsystemProxy,
    data: *mut c_void,
    data_size: i32,
) -> *mut MidiSequenceProxy {
    unsafe {
        let audio_subsystem = (*audio_subsystem_proxy).audio_subsystem;
        let data = std::slice::from_raw_parts(data as *const u8, data_size as usize);
        let midi_sequence = Box::new(MidiSequence::new(audio_subsystem, data));
        (*this).midi_sequence = Box::into_raw(midi_sequence);
        this
    }
}

#[hook(rva = 0x0003d29e)]
unsafe extern "fastcall" fn midi_sequence_destructor(this: *mut MidiSequenceProxy) {
    unsafe {
        let midi_sequence = Box::from_raw((*this).midi_sequence);
        drop(midi_sequence);
    }
}

#[hook(rva = 0x0003d2f7)]
unsafe extern "fastcall" fn midi_sequence_start(this: *mut MidiSequenceProxy) {
    unsafe {
        let mut midi_sequence = Box::from_raw((*this).midi_sequence);
        midi_sequence.start();
        let _ = Box::into_raw(midi_sequence);
    }
}

#[hook(rva = 0x0003d341)]
unsafe extern "fastcall" fn midi_sequence_stop(this: *mut MidiSequenceProxy) {
    unsafe {
        let mut midi_sequence = Box::from_raw((*this).midi_sequence);
        midi_sequence.stop();
        let _ = Box::into_raw(midi_sequence);
    }
}

#[hook(rva = 0x0003d3f4)]
unsafe extern "fastcall" fn midi_sequence_apply_current_volume(this: *mut MidiSequenceProxy) {
    unsafe {
        let mut midi_sequence = Box::from_raw((*this).midi_sequence);
        midi_sequence.apply_current_volume();
        let _ = Box::into_raw(midi_sequence);
    }
}

#[hook(rva = 0x0003d371)]
unsafe extern "fastcall" fn midi_sequence_set_volume(
    this: *mut MidiSequenceProxy,
    _: *mut c_void,
    volume: i32,
) {
    unsafe {
        let mut midi_sequence = Box::from_raw((*this).midi_sequence);
        midi_sequence.set_volume(volume);
        let _ = Box::into_raw(midi_sequence);
    }
}

#[hook(rva = 0x0003d25c)]
unsafe extern "fastcall" fn midi_sequence_set_loop_count(
    _this: *mut MidiSequenceProxy,
    _: *mut c_void,
    _loop_count: i32,
) {
    // Do nothing. We're always looping.
}

#[hook(rva = 0x0003d3d4)]
unsafe extern "fastcall" fn midi_sequence_get_global_active_sequence_count(
    _this: *mut MidiSequenceProxy,
) -> u32 {
    // Just return 1 because if this object exists, there's at least 1 playing.
    1
}

patches!(
    pub(in crate::shell) static PATCHES = [
        hook audio_subsystem_constructor,
        hook audio_subsystem_destructor,
        hook audio_subsystem_get_digital_driver,
        hook audio_subsystem_close_digital_driver,
        hook audio_subsystem_apply_midi_volume,
        hook audio_subsystem_get_active_sequence_count,
        hook audio_sample_constructor,
        hook audio_sample_destructor,
        hook audio_sample_start,
        hook audio_sample_get_is_playing,
        hook audio_sample_set_fade,
        hook audio_sample_do_fade,
        hook audio_sample_enable_loop,
        hook audio_sample_set_loop_count,
        hook audio_sample_set_volume,
        hook midi_sequence_constructor,
        hook midi_sequence_destructor,
        hook midi_sequence_start,
        hook midi_sequence_stop,
        hook midi_sequence_apply_current_volume,
        hook midi_sequence_set_volume,
        hook midi_sequence_set_loop_count,
        hook midi_sequence_get_global_active_sequence_count,
    ];
);
