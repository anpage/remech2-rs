use serde::{Deserialize, Serialize};

use crate::settings;

#[derive(Clone, Copy, PartialEq, Eq, Debug, Default, Serialize, Deserialize)]
#[serde(try_from = "u32", into = "u32")]
pub enum RenderResolution {
    R240,
    R480,
    #[default]
    R768,
}

impl RenderResolution {
    pub fn next(self) -> Self {
        match self {
            Self::R240 => Self::R480,
            Self::R480 => Self::R768,
            Self::R768 => Self::R240,
        }
    }

    pub fn frame_size(self) -> (i32, i32) {
        let widescreen = settings::get().video.widescreen;
        match (self, widescreen) {
            (Self::R240, false) => (320, 240),
            (Self::R240, true) => (427, 240),
            (Self::R480, false) => (640, 480),
            (Self::R480, true) => (854, 480),
            (Self::R768, false) => (1024, 768),
            (Self::R768, true) => (1366, 768),
        }
    }
}

impl TryFrom<u32> for RenderResolution {
    type Error = String;

    fn try_from(height: u32) -> Result<Self, Self::Error> {
        match height {
            240 => Ok(Self::R240),
            480 => Ok(Self::R480),
            768 => Ok(Self::R768),
            _ => Err(format!("{height} isn't 240, 480 or 768")),
        }
    }
}

impl From<RenderResolution> for u32 {
    fn from(resolution: RenderResolution) -> Self {
        match resolution {
            RenderResolution::R240 => 240,
            RenderResolution::R480 => 480,
            RenderResolution::R768 => 768,
        }
    }
}
