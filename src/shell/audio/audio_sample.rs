use rodio::{Decoder, Sink, Source};

use super::audio_subsystem::AudioSubsystem;

pub struct AudioSample {
    data: Vec<u8>,
    sink: Sink,

    /// Volume in the range 0..128
    volume: i32,

    initial_fade_rate: i32,
    fade_rate: i32,
    max_fade: i32,
    start_volume: i32,
    end_volume: i32,
}

impl AudioSample {
    pub fn new(subsystem: *mut AudioSubsystem, data: &[u8]) -> Self {
        let source = Decoder::new(std::io::Cursor::new(data.to_vec())).unwrap();
        let driver = unsafe { (*subsystem).get_digital_driver().unwrap() };
        let sink = Sink::try_new(&driver).unwrap();
        sink.pause();
        sink.append(source);

        Self {
            data: data.to_vec(),
            sink,
            volume: 0,
            initial_fade_rate: 0,
            fade_rate: 0,
            max_fade: 0,
            start_volume: 0,
            end_volume: 0,
        }
    }

    pub fn start(&mut self) {
        self.apply_volume();
        self.sink.play();
    }

    pub fn is_playing(&self) -> bool {
        !self.sink.is_paused() && !self.sink.empty()
    }

    pub fn set_fade(&mut self, rate: i32, max: i32, start: i32, end: i32) {
        self.initial_fade_rate = rate;
        self.fade_rate = rate;
        self.max_fade = max;
        self.start_volume = start;
        self.end_volume = end;
        self.set_volume(start);
    }

    pub fn do_fade(&mut self) {
        if self.max_fade <= 0 {
            return;
        }

        self.fade_rate -= 1;

        if self.fade_rate != 0 {
            return;
        }

        self.max_fade -= 1;
        self.fade_rate = self.initial_fade_rate;

        if self.end_volume == self.start_volume {
            return;
        }

        if self.end_volume > self.start_volume {
            self.set_volume(self.volume + 1);
        } else {
            self.set_volume(self.volume - 1);
        };
    }

    pub fn set_volume(&mut self, volume: i32) {
        let volume = volume.clamp(0, 127);
        self.volume = volume;
        self.sink.set_volume(volume as f32 / 127.0);
    }

    pub fn apply_volume(&mut self) {
        self.set_volume(self.volume);
    }

    pub fn enable_loop(&mut self) {
        self.sink.clear();
        let source = Decoder::new(std::io::Cursor::new(self.data.clone())).unwrap();
        self.sink.append(source.repeat_infinite());
    }
}
