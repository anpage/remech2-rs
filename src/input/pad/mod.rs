use std::{
    fmt,
    sync::{LazyLock, Mutex, MutexGuard, PoisonError},
};

#[derive(Clone, Copy, Debug, PartialEq, Eq, Hash)]
pub enum AxisKind {
    X,
    Y,
    Z,
    Rx,
    Ry,
    Rz,
    Slider(u8),
}

impl AxisKind {
    pub fn name(self) -> String {
        match self {
            AxisKind::Slider(n) => format!("Slider{n}"),
            kind => kind.to_string(),
        }
    }

    pub fn from_name(name: &str) -> Option<Self> {
        Some(match name {
            "X" => AxisKind::X,
            "Y" => AxisKind::Y,
            "Z" => AxisKind::Z,
            "Rx" => AxisKind::Rx,
            "Ry" => AxisKind::Ry,
            "Rz" => AxisKind::Rz,
            _ => AxisKind::Slider(name.strip_prefix("Slider")?.parse().ok()?),
        })
    }
}

impl fmt::Display for AxisKind {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        match self {
            AxisKind::X => f.write_str("X"),
            AxisKind::Y => f.write_str("Y"),
            AxisKind::Z => f.write_str("Z"),
            AxisKind::Rx => f.write_str("Rx"),
            AxisKind::Ry => f.write_str("Ry"),
            AxisKind::Rz => f.write_str("Rz"),
            AxisKind::Slider(n) => write!(f, "Slider {n}"),
        }
    }
}

#[derive(Clone, Copy, Debug, PartialEq, Eq, Hash)]
pub enum HatDirection {
    Up,
    UpRight,
    Right,
    DownRight,
    Down,
    DownLeft,
    Left,
    UpLeft,
}

impl HatDirection {
    /// Clockwise from up
    pub const ALL: [HatDirection; 8] = [
        HatDirection::Up,
        HatDirection::UpRight,
        HatDirection::Right,
        HatDirection::DownRight,
        HatDirection::Down,
        HatDirection::DownLeft,
        HatDirection::Left,
        HatDirection::UpLeft,
    ];

    pub fn from_degrees(degrees: f32) -> Self {
        let sector = (degrees.rem_euclid(360.0) / 45.0).round() as usize % 8;
        Self::ALL[sector]
    }

    pub fn from_name(name: &str) -> Option<Self> {
        Self::ALL
            .into_iter()
            .find(|direction| direction.name() == name)
    }

    pub fn includes(self, other: HatDirection) -> bool {
        if self == other {
            return true;
        }
        let is_cardinal = (other as usize).is_multiple_of(2);
        let distance = (self as usize).abs_diff(other as usize);
        is_cardinal && (distance == 1 || distance == 7)
    }

    pub fn name(self) -> &'static str {
        match self {
            HatDirection::Up => "Up",
            HatDirection::UpRight => "UpRight",
            HatDirection::Right => "Right",
            HatDirection::DownRight => "DownRight",
            HatDirection::Down => "Down",
            HatDirection::DownLeft => "DownLeft",
            HatDirection::Left => "Left",
            HatDirection::UpLeft => "UpLeft",
        }
    }
}

#[derive(Clone, Debug)]
pub struct Axis {
    pub kind: AxisKind,
    pub name: String,
    pub min: i32,
    pub max: i32,
    pub raw: i32,
}

impl Axis {
    pub fn new(kind: AxisKind, name: String, min: i32, max: i32) -> Self {
        Self {
            kind,
            name,
            min,
            max,
            raw: midpoint(min, max),
        }
    }

    pub fn value(&self) -> f32 {
        normalize(self.raw, self.min, self.max)
    }
}

fn midpoint(min: i32, max: i32) -> i32 {
    ((i64::from(min) + i64::from(max)) / 2) as i32
}

fn normalize(value: i32, min: i32, max: i32) -> f32 {
    if max <= min {
        return 0.0;
    }
    let t = (f64::from(value) - f64::from(min)) / (f64::from(max) - f64::from(min));
    (t * 2.0 - 1.0).clamp(-1.0, 1.0) as f32
}

#[derive(Clone, Debug)]
pub struct Button {
    pub name: String,
    pub pressed: bool,
    pub press_count: u32,
}

impl Button {
    pub fn new(name: String) -> Self {
        Self {
            name,
            pressed: false,
            press_count: 0,
        }
    }
}

#[derive(Clone, Debug)]
pub struct Hat {
    pub name: String,
    pub direction: Option<HatDirection>,
}

impl Hat {
    pub fn new(name: String) -> Self {
        Self {
            name,
            direction: None,
        }
    }
}

#[derive(Clone, Debug)]
pub struct Device {
    pub id: usize,
    pub name: String,
    pub vendor_id: Option<u16>,
    pub product_id: Option<u16>,
    pub persistent_id: Option<String>,
    pub serial_number: Option<String>,
    pub connected: bool,
    pub axes: Vec<Axis>,
    pub buttons: Vec<Button>,
    pub hats: Vec<Hat>,
}

impl Device {
    pub fn set_axis(&mut self, index: usize, raw: i32) {
        if let Some(axis) = self.axes.get_mut(index) {
            axis.raw = raw;
        }
    }

    pub fn set_button(&mut self, index: usize, pressed: bool) {
        let Some(button) = self.buttons.get_mut(index) else {
            return;
        };
        if button.pressed == pressed {
            return;
        }
        button.pressed = pressed;
        if pressed {
            button.press_count = button.press_count.wrapping_add(1);
        }
    }

    pub fn set_hat(&mut self, index: usize, direction: Option<HatDirection>) {
        if let Some(hat) = self.hats.get_mut(index) {
            hat.direction = direction;
        }
    }

    fn release_all(&mut self) {
        for index in 0..self.buttons.len() {
            self.set_button(index, false);
        }
        for index in 0..self.hats.len() {
            self.set_hat(index, None);
        }
    }
}

#[derive(Clone, Debug, Default)]
pub enum BackendStatus {
    #[default]
    Starting,
    Running,
    Failed(String),
}

#[derive(Clone, Debug, Default)]
pub struct PadState {
    pub status: BackendStatus,
    pub devices: Vec<Device>,
}

impl PadState {
    fn add_device(&mut self, mut device: Device) -> usize {
        let id = self.devices.len();
        device.id = id;
        tracing::info!(
            "Controller {id} connected: {:?}, {:04x?}:{:04x?}, {} axes, {} buttons, {} hats, path {:?}",
            device.name,
            device.vendor_id,
            device.product_id,
            device.axes.len(),
            device.buttons.len(),
            device.hats.len(),
            device.persistent_id,
        );
        self.devices.push(device);
        id
    }

    fn set_connected(&mut self, id: usize, connected: bool) {
        let Some(device) = self.devices.get_mut(id) else {
            return;
        };
        if device.connected == connected {
            return;
        }
        if !connected {
            device.release_all();
        }
        device.connected = connected;
        tracing::info!(
            "Controller {id} {}",
            if connected {
                "reconnected"
            } else {
                "disconnected"
            }
        );
    }
}

static PADS: LazyLock<Mutex<PadState>> = LazyLock::new(Default::default);

fn lock_pads() -> MutexGuard<'static, PadState> {
    PADS.lock().unwrap_or_else(PoisonError::into_inner)
}

pub fn snapshot() -> PadState {
    lock_pads().clone()
}
