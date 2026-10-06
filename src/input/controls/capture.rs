use std::collections::{HashMap, HashSet};

use crate::input::{
    KeyCode,
    kbm::{KbmState, MouseButton, WheelDirection},
    pad::{AxisKind, Device, HatDirection},
    source::Threshold,
};

/// How far an axis has to move from where it started to be captured
const AXIS_CAPTURE_DISTANCE: f64 = 0.5;

const THRESHOLD_SHARE: f64 = 0.75;

#[derive(Clone, Debug, PartialEq)]
pub enum Input {
    Key(KeyCode),
    MouseButton(MouseButton),
    Wheel(WheelDirection),
    Button {
        device: usize,
        index: u16,
    },
    Hat {
        device: usize,
        index: u8,
        direction: HatDirection,
    },
    Axis {
        device: usize,
        axis: AxisKind,
    },
    Threshold {
        device: usize,
        axis: AxisKind,
        threshold: Threshold,
    },
}

#[derive(Clone, Debug, PartialEq)]
pub struct Captured {
    pub input: Input,
    pub modifiers: Vec<Input>,
}

#[derive(Clone, Copy, Debug)]
pub struct Wanted {
    pub axes: bool,
    pub ignore_mouse: bool,
}

#[derive(Default)]
struct HatPush {
    frames: HashMap<HatDirection, u32>,
}

impl HatPush {
    fn direction(&self) -> Option<HatDirection> {
        HatDirection::ALL
            .into_iter()
            .filter_map(|direction| Some((self.frames.get(&direction).copied()?, direction)))
            .max_by_key(|&(frames, direction)| (frames, std::cmp::Reverse(direction as usize)))
            .map(|(_, direction)| direction)
    }
}

pub struct Capture {
    last_kbm: KbmState,
    last_devices: Vec<Device>,
    axes_at_start: HashMap<usize, Vec<f64>>,
    held: Vec<Input>,
    hats: HashMap<(usize, u8), HatPush>,
    hats_at_start: HashSet<(usize, u8)>,
}

fn axis_values(device: &Device) -> Vec<f64> {
    device
        .axes
        .iter()
        .map(|axis| f64::from(axis.value()))
        .collect()
}

impl Capture {
    pub fn new(kbm: &KbmState, devices: &[Device]) -> Self {
        Self {
            last_kbm: kbm.clone(),
            last_devices: devices.to_vec(),
            axes_at_start: devices
                .iter()
                .map(|device| (device.id, axis_values(device)))
                .collect(),
            held: Vec::new(),
            hats: HashMap::new(),
            hats_at_start: devices
                .iter()
                .flat_map(|device| {
                    device
                        .hats
                        .iter()
                        .enumerate()
                        .filter(|(_, hat)| hat.direction.is_some())
                        .map(|(index, _)| (device.id, index as u8))
                })
                .collect(),
        }
    }

    pub fn update(
        &mut self,
        kbm: &KbmState,
        devices: &[Device],
        wanted: Wanted,
    ) -> Option<Captured> {
        let outcome = self.look(kbm, devices, wanted);
        self.last_kbm = kbm.clone();
        self.last_devices = devices.to_vec();
        outcome
    }

    fn look(&mut self, kbm: &KbmState, devices: &[Device], wanted: Wanted) -> Option<Captured> {
        let mut tapped = None;
        let mut press = |held: &mut Vec<Input>, input: Input, still_held: bool| {
            if still_held {
                if !held.contains(&input) {
                    held.push(input);
                }
            } else if tapped.is_none() {
                tapped = Some(input);
            }
        };

        for (key, presses) in kbm.all_key_presses() {
            if presses != self.last_kbm.key_presses(key) {
                press(
                    &mut self.held,
                    Input::Key(key),
                    kbm.keys_held.contains(&key),
                );
            }
        }
        if !wanted.ignore_mouse {
            for button in MouseButton::ALL {
                if kbm.button_presses(button) != self.last_kbm.button_presses(button) {
                    press(
                        &mut self.held,
                        Input::MouseButton(button),
                        kbm.button_held(button),
                    );
                }
            }
        }
        for device in devices.iter().filter(|device| device.connected) {
            let last = self.last_devices.iter().find(|last| last.id == device.id);
            for (index, button) in device.buttons.iter().enumerate() {
                let before = last
                    .and_then(|last| last.buttons.get(index))
                    .map_or(button.press_count, |last| last.press_count);
                if button.press_count != before {
                    let input = Input::Button {
                        device: device.id,
                        index: index as u16,
                    };
                    press(&mut self.held, input, button.pressed);
                }
            }
            for (index, hat) in device.hats.iter().enumerate() {
                let key = (device.id, index as u8);
                match hat.direction {
                    None => {
                        self.hats_at_start.remove(&key);
                    }
                    Some(_) if self.hats_at_start.contains(&key) => {}
                    Some(direction) => {
                        let push = self.hats.entry(key).or_default();
                        *push.frames.entry(direction).or_default() += 1;
                    }
                }
            }
        }

        if let Some(input) = tapped {
            return Some(self.finish(input, kbm, devices, wanted));
        }

        for direction in WheelDirection::ALL {
            if kbm.wheel_notches(direction) != self.last_kbm.wheel_notches(direction) {
                return Some(self.finish(Input::Wheel(direction), kbm, devices, wanted));
            }
        }

        let centered = self.hats.iter().find_map(|(&(device, index), push)| {
            let now = devices
                .iter()
                .find(|d| d.id == device && d.connected)
                .and_then(|d| d.hats.get(usize::from(index)))
                .and_then(|hat| hat.direction);
            if now.is_some() {
                return None;
            }
            Some(Input::Hat {
                device,
                index,
                direction: push.direction()?,
            })
        });
        if let Some(input) = centered {
            return Some(self.finish(input, kbm, devices, wanted));
        }
        if self
            .held
            .iter()
            .any(|input| !is_held(input, kbm, devices, wanted))
        {
            let input = self.held.pop()?;
            let modifiers = std::mem::take(&mut self.held);
            return Some(Captured { input, modifiers });
        }

        for device in devices.iter().filter(|device| device.connected) {
            let start = self
                .axes_at_start
                .entry(device.id)
                .or_insert_with(|| axis_values(device));
            for (index, axis) in device.axes.iter().enumerate() {
                let Some(&from) = start.get(index) else {
                    continue;
                };
                let to = f64::from(axis.value());
                if (to - from).abs() < AXIS_CAPTURE_DISTANCE {
                    continue;
                }
                let input = if wanted.axes {
                    Input::Axis {
                        device: device.id,
                        axis: axis.kind,
                    }
                } else {
                    Input::Threshold {
                        device: device.id,
                        axis: axis.kind,
                        threshold: threshold_between(from, to),
                    }
                };
                return Some(self.finish(input, kbm, devices, wanted));
            }
        }

        None
    }

    fn finish(&self, input: Input, kbm: &KbmState, devices: &[Device], wanted: Wanted) -> Captured {
        let mut modifiers: Vec<Input> = self
            .held
            .iter()
            .filter(|&held| *held != input && is_held(held, kbm, devices, wanted))
            .cloned()
            .collect();
        for (&(device, index), push) in &self.hats {
            let pushed = devices
                .iter()
                .find(|d| d.id == device && d.connected)
                .and_then(|d| d.hats.get(usize::from(index))?.direction);
            if let (Some(_), Some(direction)) = (pushed, push.direction()) {
                let hat = Input::Hat {
                    device,
                    index,
                    direction,
                };
                if hat != input {
                    modifiers.push(hat);
                }
            }
        }
        Captured { input, modifiers }
    }
}

fn is_held(input: &Input, kbm: &KbmState, devices: &[Device], wanted: Wanted) -> bool {
    let device = |id: usize| devices.iter().find(|d| d.id == id && d.connected);
    match *input {
        Input::Key(key) => kbm.keys_held.contains(&key),
        Input::MouseButton(button) => kbm.button_held(button) || wanted.ignore_mouse,
        Input::Button { device: id, index } => device(id)
            .and_then(|d| d.buttons.get(usize::from(index)))
            .is_some_and(|button| button.pressed),
        Input::Hat {
            device: id,
            index,
            direction,
        } => device(id)
            .and_then(|d| d.hats.get(usize::from(index))?.direction)
            .is_some_and(|pushed| pushed.includes(direction)),
        Input::Wheel(_) | Input::Axis { .. } | Input::Threshold { .. } => false,
    }
}

fn threshold_between(from: f64, to: f64) -> Threshold {
    let value = ((from + (to - from) * THRESHOLD_SHARE) * 20.0).round() / 20.0;
    let value = value.clamp(-1.0, 1.0);
    if to > from {
        Threshold::Above(value)
    } else {
        Threshold::Below(value)
    }
}

pub fn device_pressed(before: &[Device], now: &[Device]) -> Option<usize> {
    now.iter()
        .filter(|device| device.connected)
        .find(|device| {
            let Some(last) = before.iter().find(|last| last.id == device.id) else {
                return false;
            };
            let buttons = device
                .buttons
                .iter()
                .zip(&last.buttons)
                .any(|(button, last)| button.press_count != last.press_count);
            let hats = device
                .hats
                .iter()
                .zip(&last.hats)
                .any(|(hat, last)| hat.direction.is_some() && hat.direction != last.direction);
            buttons || hats
        })
        .map(|device| device.id)
}
