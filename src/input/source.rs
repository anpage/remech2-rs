use std::{fmt, str::FromStr};

use serde::{Deserialize, Deserializer, Serialize, Serializer, de};

use super::{
    KeyCode,
    kbm::{MouseButton, WheelDirection},
    pad::{AxisKind, HatDirection},
};

#[derive(Clone, Copy, Debug, PartialEq, Eq, Hash)]
pub enum Modifier {
    Shift,
    Control,
    Alt,
    Meta,
}

impl Modifier {
    pub const ALL: [Modifier; 4] = [
        Modifier::Shift,
        Modifier::Control,
        Modifier::Alt,
        Modifier::Meta,
    ];

    pub fn name(self) -> &'static str {
        match self {
            Modifier::Shift => "Shift",
            Modifier::Control => "Control",
            Modifier::Alt => "Alt",
            Modifier::Meta => "Meta",
        }
    }

    pub fn from_name(name: &str) -> Option<Self> {
        Self::ALL
            .into_iter()
            .find(|modifier| modifier.name() == name)
    }

    pub fn keys(self) -> [KeyCode; 2] {
        match self {
            Modifier::Shift => [KeyCode::ShiftLeft, KeyCode::ShiftRight],
            Modifier::Control => [KeyCode::ControlLeft, KeyCode::ControlRight],
            Modifier::Alt => [KeyCode::AltLeft, KeyCode::AltRight],
            Modifier::Meta => [KeyCode::MetaLeft, KeyCode::MetaRight],
        }
    }
}

#[derive(Clone, Copy, Debug, PartialEq, Eq, Hash)]
pub enum MouseAxis {
    X,
    Y,
}

impl MouseAxis {
    pub fn name(self) -> &'static str {
        match self {
            MouseAxis::X => "X",
            MouseAxis::Y => "Y",
        }
    }

    pub fn from_name(name: &str) -> Option<Self> {
        match name {
            "X" => Some(MouseAxis::X),
            "Y" => Some(MouseAxis::Y),
            _ => None,
        }
    }
}

#[derive(Clone, Copy, Debug, PartialEq)]
pub enum Threshold {
    Above(f64),
    Below(f64),
}

impl Threshold {
    pub fn reached(self, value: f64) -> bool {
        match self {
            Threshold::Above(threshold) => value >= threshold,
            Threshold::Below(threshold) => value <= threshold,
        }
    }
}

#[derive(Clone, Debug, PartialEq)]
pub enum Source {
    Key(KeyCode),
    Modifier(Modifier),
    MouseButton(MouseButton),
    Wheel(WheelDirection),
    MouseAxis(MouseAxis),
    Button {
        device: String,
        index: u16,
    },
    Hat {
        device: String,
        index: u8,
        direction: HatDirection,
    },
    Axis {
        device: String,
        axis: AxisKind,
    },
    AxisThreshold {
        device: String,
        axis: AxisKind,
        threshold: Threshold,
    },
}

#[derive(Clone, Copy, Debug, PartialEq, Eq)]
pub enum SourceKind {
    Digital,
    Analog,
    Delta,
}

impl Source {
    pub fn kind(&self) -> SourceKind {
        match self {
            Source::Axis { .. } => SourceKind::Analog,
            Source::MouseAxis(_) => SourceKind::Delta,
            _ => SourceKind::Digital,
        }
    }

    pub fn device(&self) -> Option<&str> {
        match self {
            Source::Button { device, .. }
            | Source::Hat { device, .. }
            | Source::Axis { device, .. }
            | Source::AxisThreshold { device, .. } => Some(device),
            // Keyboard and mouse sources have no device
            _ => None,
        }
    }
}

pub fn valid_device_id(id: &str) -> bool {
    !id.is_empty()
        && id != "key"
        && id != "mouse"
        && id
            .chars()
            .all(|c| c.is_ascii_alphanumeric() || c == '-' || c == '_')
}

impl fmt::Display for Source {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        match self {
            Source::Key(key) => write!(f, "key/{key}"),
            Source::Modifier(modifier) => write!(f, "key/{}", modifier.name()),
            Source::MouseButton(button) => write!(f, "mouse/button/{}", button.name()),
            Source::Wheel(direction) => write!(f, "mouse/wheel/{}", direction.name()),
            Source::MouseAxis(axis) => write!(f, "mouse/axis/{}", axis.name()),
            Source::Button { device, index } => write!(f, "{device}/button/{index}"),
            Source::Hat {
                device,
                index,
                direction,
            } => write!(f, "{device}/hat/{index}/{}", direction.name()),
            Source::Axis { device, axis } => write!(f, "{device}/axis/{}", axis.name()),
            Source::AxisThreshold {
                device,
                axis,
                threshold,
            } => {
                let (side, value) = match threshold {
                    Threshold::Above(value) => ("above", value),
                    Threshold::Below(value) => ("below", value),
                };
                write!(f, "{device}/axis/{}/{side}/{value}", axis.name())
            }
        }
    }
}

impl FromStr for Source {
    type Err = String;

    fn from_str(s: &str) -> Result<Self, Self::Err> {
        let parts: Vec<&str> = s.split('/').collect();
        let bad = || format!("unrecognised input {s:?}");
        let name = |part: &str, what: &str| format!("unknown {what} {part:?} in {s:?}");

        Ok(match parts.as_slice() {
            ["key", key] => match KeyCode::from_name(key) {
                Some(key) => Source::Key(key),
                None => Source::Modifier(Modifier::from_name(key).ok_or_else(|| name(key, "key"))?),
            },
            ["mouse", "button", button] => Source::MouseButton(
                MouseButton::from_name(button).ok_or_else(|| name(button, "mouse button"))?,
            ),
            ["mouse", "wheel", direction] => Source::Wheel(
                WheelDirection::from_name(direction)
                    .ok_or_else(|| name(direction, "wheel direction"))?,
            ),
            ["mouse", "axis", axis] => Source::MouseAxis(
                MouseAxis::from_name(axis).ok_or_else(|| name(axis, "mouse axis"))?,
            ),
            [device, rest @ ..] if valid_device_id(device) => {
                let device = device.to_string();
                match rest {
                    ["button", index] => Source::Button {
                        device,
                        index: index.parse().map_err(|_| bad())?,
                    },
                    ["hat", index, direction] => Source::Hat {
                        device,
                        index: index.parse().map_err(|_| bad())?,
                        direction: HatDirection::from_name(direction)
                            .ok_or_else(|| name(direction, "hat direction"))?,
                    },
                    ["axis", axis] => Source::Axis {
                        device,
                        axis: AxisKind::from_name(axis).ok_or_else(|| name(axis, "axis"))?,
                    },
                    ["axis", axis, side, value] => {
                        let value: f64 = value.parse().map_err(|_| bad())?;
                        if !(-1.0..=1.0).contains(&value) {
                            return Err(format!("threshold out of range in {s:?}"));
                        }
                        Source::AxisThreshold {
                            device,
                            axis: AxisKind::from_name(axis).ok_or_else(|| name(axis, "axis"))?,
                            threshold: match *side {
                                "above" => Threshold::Above(value),
                                "below" => Threshold::Below(value),
                                _ => return Err(bad()),
                            },
                        }
                    }
                    _ => return Err(bad()),
                }
            }
            _ => return Err(bad()),
        })
    }
}

impl Serialize for Source {
    fn serialize<S: Serializer>(&self, serializer: S) -> Result<S::Ok, S::Error> {
        serializer.collect_str(self)
    }
}

impl<'de> Deserialize<'de> for Source {
    fn deserialize<D: Deserializer<'de>>(deserializer: D) -> Result<Self, D::Error> {
        String::deserialize(deserializer)?
            .parse()
            .map_err(de::Error::custom)
    }
}
