use std::sync::{Arc, Mutex};

use crate::ailrs::{
    interface::SampleHandle,
    storage::{DriverKey, get_driver},
    voice::{Status, Voice, VoiceState},
};

#[derive(Clone)]
pub struct Sample(Arc<Mutex<VoiceState>>);

impl Sample {
    pub fn new(driver: DriverKey) -> Option<Self> {
        let driver = get_driver(driver)?;
        let state = Arc::new(Mutex::new(VoiceState::new(driver.is_mono())));
        driver.mixer().add(Voice::new(state.clone()));
        Some(Self(state))
    }

    pub fn init(&self) {
        self.0.lock().unwrap().init();
    }

    pub fn load_buffer(&self, slot: usize, data: &[u8]) {
        self.0.lock().unwrap().load_buffer(slot, data);
    }

    pub fn buffer_ready(&self) -> i32 {
        self.0.lock().unwrap().buffer_ready()
    }

    pub fn start(&self) {
        let mut s = self.0.lock().unwrap();
        s.status = Status::Playing;
    }

    pub fn stop(&self) {
        let mut s = self.0.lock().unwrap();
        if s.status == Status::Playing {
            s.status = Status::Stopped;
        }
    }

    pub fn resume(&self) {
        let mut s = self.0.lock().unwrap();
        if s.status == Status::Stopped {
            s.status = Status::Playing;
        }
    }

    pub fn end(&self) {
        let mut s = self.0.lock().unwrap();
        s.status = Status::Done;
    }

    pub fn release(&self) {
        self.0.lock().unwrap().alive = false;
    }

    pub fn user_data(&self, index: u32) -> i32 {
        let s = self.0.lock().unwrap();
        s.user_data.get(index as usize).copied().unwrap_or(0)
    }

    pub fn set_user_data(&self, index: u32, value: i32) {
        let mut s = self.0.lock().unwrap();
        if let Some(slot) = s.user_data.get_mut(index as usize) {
            *slot = value;
        }
    }

    pub fn set_loop_count(&self, loop_count: u32) {
        self.0.lock().unwrap().loop_count = loop_count;
    }

    pub fn set_volume(&self, volume: i32) {
        self.0.lock().unwrap().volume = volume.clamp(0, 127) as u8;
    }

    pub fn set_pan(&self, pan: i32) {
        self.0.lock().unwrap().pan = pan.clamp(0, 127) as u8;
    }

    pub fn set_playback_rate(&self, rate: i32) {
        self.0.lock().unwrap().rate = rate.max(1) as u32;
    }

    pub fn set_type(&self, format: i32, _flags: u32) {
        // Mech2 only ever passes `DIG_F_MONO_8` (0)
        if format != 0 {
            tracing::error!("set_sample_type: unsupported format {format}, expected DIG_F_MONO_8");
            panic!("unsupported sample format {format}");
        }
    }

    pub fn register_eos_callback(
        &self,
        callback: Option<unsafe extern "stdcall" fn(SampleHandle)>,
    ) -> Option<unsafe extern "stdcall" fn(SampleHandle)> {
        let mut s = self.0.lock().unwrap();
        std::mem::replace(&mut s.eos_callback, callback)
    }

    pub fn take_pending_eos(&self) -> Option<unsafe extern "stdcall" fn(SampleHandle)> {
        let mut s = self.0.lock().unwrap();
        if s.pending_eos {
            s.pending_eos = false;
            s.eos_callback
        } else {
            None
        }
    }
}
