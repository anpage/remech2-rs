use anyhow::Result;
use retour::GenericDetour;

use std::ffi::c_void;

use crate::hooker::hook_function;

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

type AudioSubsystemConstructorFunc =
    unsafe extern "fastcall" fn(*mut AudioSubsystemProxy) -> *mut AudioSubsystemProxy;
static mut AUDIO_SUBSYSTEM_CONSTRUCTOR_HOOK: Option<GenericDetour<AudioSubsystemConstructorFunc>> =
    None;

unsafe extern "fastcall" fn audio_subsystem_constructor(
    this: *mut AudioSubsystemProxy,
) -> *mut AudioSubsystemProxy {
    unsafe {
        let audio_subsystem = Box::new(AudioSubsystem::new());
        (*this).audio_subsystem = Box::into_raw(audio_subsystem);
        this
    }
}

type AudioSubsystemDestructorFunc = unsafe extern "fastcall" fn(*mut AudioSubsystemProxy);
static mut AUDIO_SUBSYSTEM_DESTRUCTOR_HOOK: Option<GenericDetour<AudioSubsystemDestructorFunc>> =
    None;

unsafe extern "fastcall" fn audio_subsystem_destructor(this: *mut AudioSubsystemProxy) {
    unsafe {
        let audio_subsystem = Box::from_raw((*this).audio_subsystem);
        drop(audio_subsystem);
    }
}

type AudioSubsystemGetDigitalDriverFunc =
    unsafe extern "fastcall" fn(*mut AudioSubsystemProxy) -> *mut c_void;
static mut AUDIO_SUBSYSTEM_GET_DIGITAL_DRIVER_HOOK: Option<
    GenericDetour<AudioSubsystemGetDigitalDriverFunc>,
> = None;

unsafe extern "fastcall" fn audio_subsystem_get_digital_driver(
    _this: *mut AudioSubsystemProxy,
) -> *mut c_void {
    unreachable!()
}

type AudioSubsystemCloseDigitalDriverFunc = unsafe extern "fastcall" fn(*mut AudioSubsystemProxy);
static mut AUDIO_SUBSYSTEM_CLOSE_DIGITAL_DRIVER_HOOK: Option<
    GenericDetour<AudioSubsystemCloseDigitalDriverFunc>,
> = None;

unsafe extern "fastcall" fn audio_subsystem_close_digital_driver(this: *mut AudioSubsystemProxy) {
    unsafe {
        let mut audio_subsystem = Box::from_raw((*this).audio_subsystem);
        audio_subsystem.close_digital_driver();
        let _ = Box::into_raw(audio_subsystem);
    }
}

type AudioSubsystemApplyMidiVolumeFunc = unsafe extern "fastcall" fn(*mut AudioSubsystemProxy);
static mut AUDIO_SUBSYSTEM_APPLY_MIDI_VOLUME_HOOK: Option<
    GenericDetour<AudioSubsystemApplyMidiVolumeFunc>,
> = None;

unsafe extern "fastcall" fn audio_subsystem_apply_midi_volume(this: *mut AudioSubsystemProxy) {
    unsafe {
        let mut audio_subsystem = Box::from_raw((*this).audio_subsystem);
        audio_subsystem.apply_midi_volume();
        let _ = Box::into_raw(audio_subsystem);
    }
}

type AudioSubsystemGetActiveSequenceCountFunc =
    unsafe extern "fastcall" fn(*mut AudioSubsystemProxy) -> u32;
static mut AUDIO_SUBSYSTEM_GET_ACTIVE_SEQUENCE_COUNT_HOOK: Option<
    GenericDetour<AudioSubsystemGetActiveSequenceCountFunc>,
> = None;

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

pub type AudioSampleConstructorFunc = unsafe extern "fastcall" fn(
    *mut AudioSampleProxy,
    *mut c_void,
    *mut AudioSubsystemProxy,
    *const u8,
    i32,
) -> *mut AudioSampleProxy;
static mut AUDIO_SAMPLE_CONSTRUCTOR_HOOK: Option<GenericDetour<AudioSampleConstructorFunc>> = None;

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

pub type AudioSampleDestructorFunc = unsafe extern "fastcall" fn(*mut AudioSampleProxy);
static mut AUDIO_SAMPLE_DESTRUCTOR_HOOK: Option<GenericDetour<AudioSampleDestructorFunc>> = None;

unsafe extern "fastcall" fn audio_sample_destructor(this: *mut AudioSampleProxy) {
    unsafe {
        let audio_sample = Box::from_raw((*this).audio_sample);
        drop(audio_sample);
    }
}

pub type AudioSampleStartFunc = unsafe extern "fastcall" fn(*mut AudioSampleProxy);
static mut AUDIO_SAMPLE_START_HOOK: Option<GenericDetour<AudioSampleStartFunc>> = None;

unsafe extern "fastcall" fn audio_sample_start(this: *mut AudioSampleProxy) {
    unsafe {
        let mut audio_sample = Box::from_raw((*this).audio_sample);
        audio_sample.start();
        let _ = Box::into_raw(audio_sample);
    }
}

pub type AudioSampleGetIsPlayingFunc = unsafe extern "fastcall" fn(*mut AudioSampleProxy) -> u32;
static mut AUDIO_SAMPLE_GET_IS_PLAYING_HOOK: Option<GenericDetour<AudioSampleGetIsPlayingFunc>> =
    None;

unsafe extern "fastcall" fn audio_sample_get_is_playing(this: *mut AudioSampleProxy) -> u32 {
    unsafe {
        let audio_sample = Box::from_raw((*this).audio_sample);
        let is_playing = audio_sample.is_playing() as u32;
        let _ = Box::into_raw(audio_sample);
        is_playing
    }
}

pub type AudioSampleSetFadeFunc =
    unsafe extern "fastcall" fn(*mut AudioSampleProxy, *mut c_void, i32, i32, i32, i32);
static mut AUDIO_SAMPLE_SET_FADE_HOOK: Option<GenericDetour<AudioSampleSetFadeFunc>> = None;

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

pub type AudioSampleDoFadeFunc = unsafe extern "fastcall" fn(*mut AudioSampleProxy);
static mut AUDIO_SAMPLE_DO_FADE_HOOK: Option<GenericDetour<AudioSampleDoFadeFunc>> = None;

unsafe extern "fastcall" fn audio_sample_do_fade(this: *mut AudioSampleProxy) {
    unsafe {
        let mut audio_sample = Box::from_raw((*this).audio_sample);
        audio_sample.do_fade();
        let _ = Box::into_raw(audio_sample);
    }
}

pub type AudioSampleEnableLoopFunc = unsafe extern "fastcall" fn(*mut AudioSampleProxy);
static mut AUDIO_SAMPLE_ENABLE_LOOP_HOOK: Option<GenericDetour<AudioSampleEnableLoopFunc>> = None;

unsafe extern "fastcall" fn audio_sample_enable_loop(this: *mut AudioSampleProxy) {
    unsafe {
        let mut audio_sample = Box::from_raw((*this).audio_sample);
        audio_sample.enable_loop();
        let _ = Box::into_raw(audio_sample);
    }
}

pub type AudioSampleSetLoopCountFunc =
    unsafe extern "fastcall" fn(*mut AudioSampleProxy, *mut c_void, i32);
static mut AUDIO_SAMPLE_SET_LOOP_COUNT_HOOK: Option<GenericDetour<AudioSampleSetLoopCountFunc>> =
    None;

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

pub type AudioSampleSetVolumeFunc =
    unsafe extern "fastcall" fn(*mut AudioSampleProxy, *mut c_void, i32);
static mut AUDIO_SAMPLE_SET_VOLUME_HOOK: Option<GenericDetour<AudioSampleSetVolumeFunc>> = None;

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

pub type MidiSequenceConstructorFunc = unsafe extern "fastcall" fn(
    *mut MidiSequenceProxy,
    *mut c_void,
    *mut AudioSubsystemProxy,
    *mut c_void,
    i32,
) -> *mut MidiSequenceProxy;
static mut MIDI_SEQUENCE_CONSTRUCTOR_HOOK: Option<GenericDetour<MidiSequenceConstructorFunc>> =
    None;

unsafe extern "fastcall" fn midi_sequence_constructor(
    this: *mut MidiSequenceProxy,
    _: *mut c_void,
    audio_subsystem_proxy: *mut AudioSubsystemProxy,
    _data: *mut c_void,
    _data_size: i32,
) -> *mut MidiSequenceProxy {
    unsafe {
        let audio_subsystem = (*audio_subsystem_proxy).audio_subsystem;
        let data = std::slice::from_raw_parts(_data as *const u8, _data_size as usize);
        let midi_sequence = Box::new(MidiSequence::new(audio_subsystem, data));
        (*this).midi_sequence = Box::into_raw(midi_sequence);
        this
    }
}

pub type MidiSequenceDestructorFunc = unsafe extern "fastcall" fn(*mut MidiSequenceProxy);
static mut MIDI_SEQUENCE_DESTRUCTOR_HOOK: Option<GenericDetour<MidiSequenceDestructorFunc>> = None;

unsafe extern "fastcall" fn midi_sequence_destructor(_this: *mut MidiSequenceProxy) {
    unsafe {
        let midi_sequence = Box::from_raw((*_this).midi_sequence);
        drop(midi_sequence);
    }
}

pub type MidiSequenceStartFunc = unsafe extern "fastcall" fn(*mut MidiSequenceProxy);
static mut MIDI_SEQUENCE_START_HOOK: Option<GenericDetour<MidiSequenceStartFunc>> = None;

unsafe extern "fastcall" fn midi_sequence_start(_this: *mut MidiSequenceProxy) {
    unsafe {
        let mut midi_sequence = Box::from_raw((*_this).midi_sequence);
        midi_sequence.start();
        let _ = Box::into_raw(midi_sequence);
    }
}

pub type MidiSequenceStopFunc = unsafe extern "fastcall" fn(*mut MidiSequenceProxy);
static mut MIDI_SEQUENCE_STOP_HOOK: Option<GenericDetour<MidiSequenceStopFunc>> = None;

unsafe extern "fastcall" fn midi_sequence_stop(_this: *mut MidiSequenceProxy) {
    unsafe {
        let mut midi_sequence = Box::from_raw((*_this).midi_sequence);
        midi_sequence.stop();
        let _ = Box::into_raw(midi_sequence);
    }
}

pub type MidiSequenceApplyCurrentVolumeFunc = unsafe extern "fastcall" fn(*mut MidiSequenceProxy);
static mut MIDI_SEQUENCE_APPLY_CURRENT_VOLUME_HOOK: Option<
    GenericDetour<MidiSequenceApplyCurrentVolumeFunc>,
> = None;

unsafe extern "fastcall" fn midi_sequence_apply_current_volume(_this: *mut MidiSequenceProxy) {
    unsafe {
        let mut midi_sequence = Box::from_raw((*_this).midi_sequence);
        midi_sequence.apply_current_volume();
        let _ = Box::into_raw(midi_sequence);
    }
}

pub type MidiSequenceSetVolumeFunc =
    unsafe extern "fastcall" fn(*mut MidiSequenceProxy, *mut c_void, i32);
static mut MIDI_SEQUENCE_SET_VOLUME_HOOK: Option<GenericDetour<MidiSequenceSetVolumeFunc>> = None;

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

pub type MidiSequenceSetLoopCountFunc =
    unsafe extern "fastcall" fn(*mut MidiSequenceProxy, *mut c_void, i32);
static mut MIDI_SEQUENCE_SET_LOOP_COUNT_HOOK: Option<GenericDetour<MidiSequenceSetLoopCountFunc>> =
    None;

unsafe extern "fastcall" fn midi_sequence_set_loop_count(
    _this: *mut MidiSequenceProxy,
    _: *mut c_void,
    _loop_count: i32,
) {
    // Do nothing. We're always looping.
}

pub type MidiSequenceGetGlobalActiveSequenceCountFunc =
    unsafe extern "fastcall" fn(*mut MidiSequenceProxy) -> u32;
static mut MIDI_SEQUENCE_GET_GLOBAL_ACTIVE_SEQUENCE_COUNT_HOOK: Option<
    GenericDetour<MidiSequenceGetGlobalActiveSequenceCountFunc>,
> = None;

unsafe extern "fastcall" fn midi_sequence_get_global_active_sequence_count(
    _this: *mut MidiSequenceProxy,
) -> u32 {
    // Just return 1 because if this object exists, there's at least 1 playing.
    1
}

pub unsafe fn hook_functions(base_address: usize) -> Result<()> {
    unsafe {
        AUDIO_SUBSYSTEM_CONSTRUCTOR_HOOK = {
            let target: AudioSubsystemConstructorFunc =
                std::mem::transmute(base_address + 0x0003ceb0);
            Some(hook_function(target, audio_subsystem_constructor)?)
        };
        AUDIO_SUBSYSTEM_DESTRUCTOR_HOOK = {
            let target: AudioSubsystemDestructorFunc =
                std::mem::transmute(base_address + 0x0003cf5d);
            Some(hook_function(target, audio_subsystem_destructor)?)
        };
        AUDIO_SUBSYSTEM_GET_DIGITAL_DRIVER_HOOK = {
            let target: AudioSubsystemGetDigitalDriverFunc =
                std::mem::transmute(base_address + 0x0003cf89);
            Some(hook_function(target, audio_subsystem_get_digital_driver)?)
        };
        AUDIO_SUBSYSTEM_CLOSE_DIGITAL_DRIVER_HOOK = {
            let target: AudioSubsystemCloseDigitalDriverFunc =
                std::mem::transmute(base_address + 0x0003d032);
            Some(hook_function(target, audio_subsystem_close_digital_driver)?)
        };
        AUDIO_SUBSYSTEM_APPLY_MIDI_VOLUME_HOOK = {
            let target: AudioSubsystemApplyMidiVolumeFunc =
                std::mem::transmute(base_address + 0x0003d0fc);
            Some(hook_function(target, audio_subsystem_apply_midi_volume)?)
        };
        AUDIO_SUBSYSTEM_GET_ACTIVE_SEQUENCE_COUNT_HOOK = {
            let target: AudioSubsystemGetActiveSequenceCountFunc =
                std::mem::transmute(base_address + 0x0003d0b4);
            Some(hook_function(
                target,
                audio_subsystem_get_active_sequence_count,
            )?)
        };

        AUDIO_SAMPLE_CONSTRUCTOR_HOOK = {
            let target: AudioSampleConstructorFunc = std::mem::transmute(base_address + 0x0003d419);
            Some(hook_function(target, audio_sample_constructor)?)
        };
        AUDIO_SAMPLE_DESTRUCTOR_HOOK = {
            let target: AudioSampleDestructorFunc = std::mem::transmute(base_address + 0x0003d50f);
            Some(hook_function(target, audio_sample_destructor)?)
        };
        AUDIO_SAMPLE_START_HOOK = {
            let target: AudioSampleStartFunc = std::mem::transmute(base_address + 0x0003d6bb);
            Some(hook_function(target, audio_sample_start)?)
        };
        AUDIO_SAMPLE_GET_IS_PLAYING_HOOK = {
            let target: AudioSampleGetIsPlayingFunc =
                std::mem::transmute(base_address + 0x0003d77e);
            Some(hook_function(target, audio_sample_get_is_playing)?)
        };
        AUDIO_SAMPLE_SET_FADE_HOOK = {
            let target: AudioSampleSetFadeFunc = std::mem::transmute(base_address + 0x0003d561);
            Some(hook_function(target, audio_sample_set_fade)?)
        };
        AUDIO_SAMPLE_DO_FADE_HOOK = {
            let target: AudioSampleDoFadeFunc = std::mem::transmute(base_address + 0x0003d5ca);
            Some(hook_function(target, audio_sample_do_fade)?)
        };
        AUDIO_SAMPLE_ENABLE_LOOP_HOOK = {
            let target: AudioSampleEnableLoopFunc = std::mem::transmute(base_address + 0x0003d67f);
            Some(hook_function(target, audio_sample_enable_loop)?)
        };
        AUDIO_SAMPLE_SET_LOOP_COUNT_HOOK = {
            let target: AudioSampleSetLoopCountFunc =
                std::mem::transmute(base_address + 0x0003d842);
            Some(hook_function(target, audio_sample_set_loop_count)?)
        };
        AUDIO_SAMPLE_SET_VOLUME_HOOK = {
            let target: AudioSampleSetVolumeFunc = std::mem::transmute(base_address + 0x0003d7c0);
            Some(hook_function(target, audio_sample_set_volume)?)
        };

        MIDI_SEQUENCE_CONSTRUCTOR_HOOK = {
            let target: MidiSequenceConstructorFunc =
                std::mem::transmute(base_address + 0x0003d198);
            Some(hook_function(target, midi_sequence_constructor)?)
        };
        MIDI_SEQUENCE_DESTRUCTOR_HOOK = {
            let target: MidiSequenceDestructorFunc = std::mem::transmute(base_address + 0x0003d29e);
            Some(hook_function(target, midi_sequence_destructor)?)
        };
        MIDI_SEQUENCE_START_HOOK = {
            let target: MidiSequenceStartFunc = std::mem::transmute(base_address + 0x0003d2f7);
            Some(hook_function(target, midi_sequence_start)?)
        };
        MIDI_SEQUENCE_STOP_HOOK = {
            let target: MidiSequenceStopFunc = std::mem::transmute(base_address + 0x0003d341);
            Some(hook_function(target, midi_sequence_stop)?)
        };
        MIDI_SEQUENCE_APPLY_CURRENT_VOLUME_HOOK = {
            let target: MidiSequenceApplyCurrentVolumeFunc =
                std::mem::transmute(base_address + 0x0003d3f4);
            Some(hook_function(target, midi_sequence_apply_current_volume)?)
        };
        MIDI_SEQUENCE_SET_VOLUME_HOOK = {
            let target: MidiSequenceSetVolumeFunc = std::mem::transmute(base_address + 0x0003d371);
            Some(hook_function(target, midi_sequence_set_volume)?)
        };
        MIDI_SEQUENCE_SET_LOOP_COUNT_HOOK = {
            let target: MidiSequenceSetLoopCountFunc =
                std::mem::transmute(base_address + 0x0003d25c);
            Some(hook_function(target, midi_sequence_set_loop_count)?)
        };
        MIDI_SEQUENCE_GET_GLOBAL_ACTIVE_SEQUENCE_COUNT_HOOK = {
            let target: MidiSequenceGetGlobalActiveSequenceCountFunc =
                std::mem::transmute(base_address + 0x0003d3d4);
            Some(hook_function(
                target,
                midi_sequence_get_global_active_sequence_count,
            )?)
        };

        Ok(())
    }
}

pub unsafe fn unhook_functions() {
    unsafe {
        AUDIO_SUBSYSTEM_CONSTRUCTOR_HOOK = None;
        AUDIO_SUBSYSTEM_DESTRUCTOR_HOOK = None;
        AUDIO_SUBSYSTEM_GET_DIGITAL_DRIVER_HOOK = None;
        AUDIO_SUBSYSTEM_CLOSE_DIGITAL_DRIVER_HOOK = None;
        AUDIO_SUBSYSTEM_APPLY_MIDI_VOLUME_HOOK = None;
        AUDIO_SUBSYSTEM_GET_ACTIVE_SEQUENCE_COUNT_HOOK = None;

        AUDIO_SAMPLE_CONSTRUCTOR_HOOK = None;
        AUDIO_SAMPLE_DESTRUCTOR_HOOK = None;
        AUDIO_SAMPLE_START_HOOK = None;
        AUDIO_SAMPLE_GET_IS_PLAYING_HOOK = None;
        AUDIO_SAMPLE_SET_FADE_HOOK = None;
        AUDIO_SAMPLE_DO_FADE_HOOK = None;
        AUDIO_SAMPLE_ENABLE_LOOP_HOOK = None;
        AUDIO_SAMPLE_SET_LOOP_COUNT_HOOK = None;
        AUDIO_SAMPLE_SET_VOLUME_HOOK = None;

        MIDI_SEQUENCE_CONSTRUCTOR_HOOK = None;
        MIDI_SEQUENCE_DESTRUCTOR_HOOK = None;
        MIDI_SEQUENCE_START_HOOK = None;
        MIDI_SEQUENCE_STOP_HOOK = None;
        MIDI_SEQUENCE_APPLY_CURRENT_VOLUME_HOOK = None;
        MIDI_SEQUENCE_SET_VOLUME_HOOK = None;
        MIDI_SEQUENCE_SET_LOOP_COUNT_HOOK = None;
        MIDI_SEQUENCE_GET_GLOBAL_ACTIVE_SEQUENCE_COUNT_HOOK = None;
    }
}
