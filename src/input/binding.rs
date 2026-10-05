use serde::{Deserialize, Serialize};

use super::source::Source;

#[derive(Clone, Copy, Debug, PartialEq, Eq, Serialize, Deserialize)]
#[serde(rename_all = "snake_case")]
pub enum Direction {
    Increase,
    Decrease,
}

impl Direction {
    pub fn sign(self) -> f64 {
        match self {
            Direction::Increase => 1.0,
            Direction::Decrease => -1.0,
        }
    }
}

#[derive(Clone, Copy, Debug, Default, PartialEq, Eq, Serialize, Deserialize)]
#[serde(rename_all = "snake_case")]
pub enum AxisMode {
    /// Deflection sets the position
    #[default]
    Absolute,
    /// Deflection sets the speed the position moves at
    Relative,
}

#[derive(Clone, Debug, PartialEq, Serialize, Deserialize)]
#[serde(default)]
pub struct AxisTuning {
    #[serde(skip_serializing_if = "is_false")]
    pub invert: bool,
    /// Travel around the center that reads as centered, as a fraction of each half
    #[serde(skip_serializing_if = "is_zero")]
    pub deadzone: f64,
    /// Travel at each end that reads as fully deflected, as a fraction of each half
    #[serde(skip_serializing_if = "is_zero")]
    pub saturation: f64,
    /// Response curve exponent
    #[serde(skip_serializing_if = "is_one")]
    pub curve: f64,
    /// Sensitivity
    #[serde(skip_serializing_if = "is_one")]
    pub scale: f64,
    #[serde(skip_serializing_if = "is_absolute")]
    pub mode: AxisMode,
    /// Where an absolute axis's ends land on the action, like `[0.0, 1.0]`
    #[serde(skip_serializing_if = "Option::is_none")]
    pub range: Option<[f64; 2]>,
}

impl Default for AxisTuning {
    fn default() -> Self {
        Self {
            invert: false,
            deadzone: 0.0,
            saturation: 0.0,
            curve: 1.0,
            scale: 1.0,
            mode: AxisMode::Absolute,
            range: None,
        }
    }
}

impl AxisTuning {
    pub fn shape(&self, raw: f64) -> f64 {
        let value = if self.invert { -raw } else { raw };
        let deadzone = self.deadzone.clamp(0.0, 0.99);
        let saturation = self.saturation.clamp(0.0, 0.99 - deadzone);
        let live = 1.0 - deadzone - saturation;
        let t = ((value.abs() - deadzone) / live).clamp(0.0, 1.0);
        t.powf(self.curve.max(0.1)).copysign(value)
    }

    pub fn absolute(&self, shaped: f64) -> f64 {
        let value = (shaped * self.scale).clamp(-1.0, 1.0);
        match self.range {
            Some([low, high]) => low + (value + 1.0) / 2.0 * (high - low),
            None => value,
        }
    }
}

#[derive(Clone, Debug, PartialEq, Serialize, Deserialize)]
pub struct Binding {
    /// The action's stable name
    pub action: String,
    pub source: Source,
    #[serde(default, skip_serializing_if = "Vec::is_empty")]
    pub modifiers: Vec<Source>,
    /// Whether each press turns the action on or off
    #[serde(default, skip_serializing_if = "is_false")]
    pub toggle: bool,
    #[serde(default, skip_serializing_if = "Option::is_none")]
    pub direction: Option<Direction>,
    /// Move this far per press instead of ramping
    #[serde(default, skip_serializing_if = "Option::is_none")]
    pub step: Option<f64>,
    #[serde(default, skip_serializing_if = "Option::is_none")]
    pub set_to: Option<f64>,
    #[serde(flatten)]
    pub tuning: AxisTuning,
}

impl Binding {
    pub fn new(action: &str, source: Source) -> Self {
        Self {
            action: action.to_owned(),
            source,
            modifiers: Vec::new(),
            toggle: false,
            direction: None,
            step: None,
            set_to: None,
            tuning: AxisTuning::default(),
        }
    }

    pub fn sources(&self) -> impl Iterator<Item = &Source> {
        std::iter::once(&self.source).chain(&self.modifiers)
    }
}

fn is_false(value: &bool) -> bool {
    !value
}

fn is_zero(value: &f64) -> bool {
    *value == 0.0
}

fn is_one(value: &f64) -> bool {
    *value == 1.0
}

fn is_absolute(mode: &AxisMode) -> bool {
    *mode == AxisMode::Absolute
}
