use std::{
    collections::{BTreeMap, BTreeSet, VecDeque},
    sync::{LazyLock, Mutex},
};

use super::KeyCode;

#[derive(Clone, Copy, Debug, PartialEq, Eq, Hash)]
pub enum MouseButton {
    Left,
    Right,
    Middle,
    X1,
    X2,
}

impl MouseButton {
    pub const ALL: [MouseButton; 5] = [
        MouseButton::Left,
        MouseButton::Right,
        MouseButton::Middle,
        MouseButton::X1,
        MouseButton::X2,
    ];

    pub fn name(self) -> &'static str {
        match self {
            MouseButton::Left => "Left",
            MouseButton::Right => "Right",
            MouseButton::Middle => "Middle",
            MouseButton::X1 => "X1",
            MouseButton::X2 => "X2",
        }
    }

    pub fn from_name(name: &str) -> Option<Self> {
        Self::ALL.into_iter().find(|button| button.name() == name)
    }
}

#[derive(Clone, Copy, Debug, PartialEq, Eq, Hash)]
pub enum WheelDirection {
    Up,
    Down,
    Left,
    Right,
}

impl WheelDirection {
    pub const ALL: [WheelDirection; 4] = [
        WheelDirection::Up,
        WheelDirection::Down,
        WheelDirection::Left,
        WheelDirection::Right,
    ];

    pub fn name(self) -> &'static str {
        match self {
            WheelDirection::Up => "Up",
            WheelDirection::Down => "Down",
            WheelDirection::Left => "Left",
            WheelDirection::Right => "Right",
        }
    }

    pub fn from_name(name: &str) -> Option<Self> {
        Self::ALL
            .into_iter()
            .find(|direction| direction.name() == name)
    }
}

#[derive(Clone, Copy, Debug, PartialEq, Eq)]
pub enum KbmEvent {
    Key {
        key: KeyCode,
        pressed: bool,
    },
    MouseButton {
        button: MouseButton,
        pressed: bool,
    },
    /// One detent of a wheel
    WheelNotch(WheelDirection),
    /// Relative mouse motion in device units
    MouseMotion {
        dx: i32,
        dy: i32,
    },
    Focus(bool),
    Text(char),
}

#[derive(Clone, Debug, Default)]
pub struct KbmState {
    pub focused: bool,
    pub keys_held: BTreeSet<KeyCode>,
    key_presses: BTreeMap<KeyCode, u32>,
    buttons_held: [bool; 5],
    button_presses: [u32; 5],
    wheel_notches: [u32; 4],
    pub motion_total: (i64, i64),
}

impl KbmState {
    pub(super) fn apply(&mut self, event: KbmEvent) {
        match event {
            KbmEvent::Key { key, pressed: true } => {
                if self.keys_held.insert(key) {
                    let presses = self.key_presses.entry(key).or_default();
                    *presses = presses.wrapping_add(1);
                }
            }
            KbmEvent::Key {
                key,
                pressed: false,
            } => {
                self.keys_held.remove(&key);
            }
            KbmEvent::MouseButton { button, pressed } => {
                let held = &mut self.buttons_held[button as usize];
                if !std::mem::replace(held, pressed) && pressed {
                    let presses = &mut self.button_presses[button as usize];
                    *presses = presses.wrapping_add(1);
                }
            }
            KbmEvent::WheelNotch(direction) => {
                self.wheel_notches[direction as usize] += 1;
            }
            KbmEvent::MouseMotion { dx, dy } => {
                self.motion_total.0 += i64::from(dx);
                self.motion_total.1 += i64::from(dy);
            }
            KbmEvent::Focus(focused) => {
                if !focused {
                    self.keys_held.clear();
                    self.buttons_held = [false; 5];
                }
                self.focused = focused;
            }
            KbmEvent::Text(_) => {}
        }
    }

    pub fn key_presses(&self, key: KeyCode) -> u32 {
        self.key_presses.get(&key).copied().unwrap_or(0)
    }

    pub fn all_key_presses(&self) -> impl Iterator<Item = (KeyCode, u32)> {
        self.key_presses
            .iter()
            .map(|(&key, &presses)| (key, presses))
    }

    pub fn button_held(&self, button: MouseButton) -> bool {
        self.buttons_held[button as usize]
    }

    pub fn button_presses(&self, button: MouseButton) -> u32 {
        self.button_presses[button as usize]
    }

    pub fn wheel_notches(&self, direction: WheelDirection) -> u32 {
        self.wheel_notches[direction as usize]
    }
}

static KBM: LazyLock<Mutex<KbmState>> = LazyLock::new(Default::default);

const UI_EVENTS_KEPT: usize = 256;

static UI_EVENTS: Mutex<VecDeque<KbmEvent>> = Mutex::new(VecDeque::new());

/// Applies events from the platform backend.
pub fn push(events: impl IntoIterator<Item = KbmEvent>) {
    let mut state = KBM.lock().unwrap();
    let mut ui_events = UI_EVENTS.lock().unwrap();
    for event in events {
        state.apply(event);
        if matches!(event, KbmEvent::MouseMotion { .. }) {
            continue;
        }
        if ui_events.len() == UI_EVENTS_KEPT {
            ui_events.pop_front();
        }
        ui_events.push_back(event);
    }
}

/// Takes the events that arrived since the last call, oldest first
pub fn take_ui_events() -> Vec<KbmEvent> {
    UI_EVENTS.lock().unwrap().drain(..).collect()
}

pub fn snapshot() -> KbmState {
    KBM.lock().unwrap().clone()
}
