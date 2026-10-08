use crate::settings;

#[derive(Clone, Copy)]
pub enum Resolution {
    /// "MCGA.DLL" or no driver
    Low,
    /// "VESA480.DLL"
    Vesa480,
    /// "VESA768.DLL"
    Vesa768,
}

impl Resolution {
    pub fn from_driver(name: &str) -> Self {
        match name.to_ascii_uppercase().as_str() {
            "VESA480.DLL" => Self::Vesa480,
            "VESA768.DLL" => Self::Vesa768,
            _ => Self::Low,
        }
    }

    pub fn frame_size(self) -> (i32, i32) {
        let widescreen = settings::get().video.widescreen;
        match (self, widescreen) {
            (Self::Low, false) => (320, 240),
            (Self::Low, true) => (427, 240),
            (Self::Vesa480, false) => (640, 480),
            (Self::Vesa480, true) => (854, 480),
            (Self::Vesa768, false) => (1024, 768),
            (Self::Vesa768, true) => (1366, 768),
        }
    }
}
