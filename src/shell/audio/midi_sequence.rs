pub struct MidiSequence {}

impl MidiSequence {
    pub fn new() -> Self {
        Self {}
    }
    pub fn start(&mut self) {}
    pub fn stop(&mut self) {}
    pub fn apply_current_volume(&mut self) {}
    pub fn set_volume(&mut self, _volume: f32) {}
    pub fn set_loop_count(&mut self, _loop_count: i32) {}
    pub fn get_global_active_sequence_count() -> u32 {
        0
    }
}
