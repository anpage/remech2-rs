use std::collections::HashMap;

use gilrs_core::{EvCode, EventType, Gamepad, Gilrs};

use super::{Axis, AxisKind, Backend, Button, Device, Hat, HatDirection, PadState};

enum AxisCode {
    Axis(AxisKind),
    Slider,
    Hat { number: u32, y: bool },
}

#[cfg(target_os = "linux")]
fn classify(code: EvCode) -> AxisCode {
    const ABS_HAT0X: u32 = 0x10;
    const ABS_HAT3Y: u32 = 0x17;
    match code.into_u32() & 0xffff {
        0 => AxisCode::Axis(AxisKind::X),
        1 => AxisCode::Axis(AxisKind::Y),
        2 => AxisCode::Axis(AxisKind::Z),
        3 => AxisCode::Axis(AxisKind::Rx),
        4 => AxisCode::Axis(AxisKind::Ry),
        5 => AxisCode::Axis(AxisKind::Rz),
        abs @ ABS_HAT0X..=ABS_HAT3Y => AxisCode::Hat {
            number: (abs - ABS_HAT0X) / 2,
            y: abs % 2 == 1,
        },
        _ => AxisCode::Slider,
    }
}

#[cfg(not(target_os = "linux"))]
fn classify(code: EvCode) -> AxisCode {
    const SWITCH: u32 = 2;
    const KINDS: [AxisKind; 6] = [
        AxisKind::X,
        AxisKind::Y,
        AxisKind::Z,
        AxisKind::Rx,
        AxisKind::Ry,
        AxisKind::Rz,
    ];
    let (kind, index) = (code.into_u32() >> 16, code.into_u32() & 0xffff);
    if kind == SWITCH {
        AxisCode::Hat {
            number: index / 2,
            y: index % 2 == 1,
        }
    } else {
        KINDS
            .get(index as usize)
            .map_or(AxisCode::Slider, |&kind| AxisCode::Axis(kind))
    }
}

#[cfg(target_os = "linux")]
fn identity(gamepad: &Gamepad) -> (Option<String>, Option<String>) {
    use gilrs_core::LinuxGamepadExt;

    let Some(node) = gamepad.devpath().file_name() else {
        return (None, None);
    };
    let device = std::path::Path::new("/sys/class/input")
        .join(node)
        .join("device");
    let read = |name| {
        std::fs::read_to_string(device.join(name))
            .ok()
            .map(|text| text.trim().to_owned())
            .filter(|text| !text.is_empty())
    };
    (read("phys"), read("uniq"))
}

#[cfg(not(target_os = "linux"))]
fn identity(_gamepad: &Gamepad) -> (Option<String>, Option<String>) {
    (None, None)
}

#[derive(Clone, Copy)]
enum Control {
    Button(usize),
    Axis(usize),
    Hat { index: usize, y: bool },
}

struct Known {
    id: usize,
    controls: HashMap<u32, Control>,
    hats: Vec<(i32, i32)>,
}

fn describe(gamepad: &Gamepad) -> (Device, HashMap<u32, Control>) {
    let mut controls = HashMap::new();

    let mut buttons = Vec::new();
    for &code in gamepad.buttons() {
        controls.insert(code.into_u32(), Control::Button(buttons.len()));
        buttons.push(Button::new(format!("Button {}", buttons.len() + 1)));
    }

    let mut axes = Vec::new();
    let mut hats = Vec::new();
    let mut hat_numbers = Vec::new();
    let mut sliders = 0;
    for &code in gamepad.axes() {
        let kind = match classify(code) {
            AxisCode::Axis(kind) => kind,
            AxisCode::Slider => {
                sliders += 1;
                AxisKind::Slider(sliders - 1)
            }
            AxisCode::Hat { number, y } => {
                let index = hat_numbers
                    .iter()
                    .position(|&n| n == number)
                    .unwrap_or_else(|| {
                        hat_numbers.push(number);
                        hats.push(Hat::new(format!("Hat {}", hats.len() + 1)));
                        hats.len() - 1
                    });
                controls.insert(code.into_u32(), Control::Hat { index, y });
                continue;
            }
        };
        let (min, max) = gamepad
            .axis_info(code)
            .map_or((i16::MIN.into(), i16::MAX.into()), |info| {
                (info.min, info.max)
            });
        controls.insert(code.into_u32(), Control::Axis(axes.len()));
        axes.push(Axis::new(kind, kind.to_string(), min, max));
    }

    let (persistent_id, serial_number) = identity(gamepad);
    let device = Device {
        id: 0,
        name: gamepad.name().to_owned(),
        vendor_id: gamepad.vendor_id(),
        product_id: gamepad.product_id(),
        persistent_id,
        serial_number,
        connected: true,
        axes,
        buttons,
        hats,
    };
    (device, controls)
}

pub(super) struct GilrsBackend {
    gilrs: Gilrs,
    known: HashMap<usize, Known>,
    scanned: bool,
}

impl GilrsBackend {
    pub(super) fn new() -> Result<Self, gilrs_core::Error> {
        Ok(Self {
            gilrs: Gilrs::new()?,
            known: HashMap::new(),
            scanned: false,
        })
    }

    fn connect(&mut self, pads: &mut PadState, gilrs_id: usize) {
        let Some(gamepad) = self.gilrs.gamepad(gilrs_id) else {
            return;
        };
        if let Some(known) = self.known.get(&gilrs_id) {
            if let Some(device) = pads.devices.get_mut(known.id) {
                (device.persistent_id, device.serial_number) = identity(gamepad);
            }
            pads.set_connected(known.id, true);
            return;
        }
        let (device, controls) = describe(gamepad);
        let hats = vec![(0, 0); device.hats.len()];
        let id = pads.add_device(device);
        self.known.insert(gilrs_id, Known { id, controls, hats });
    }
}

impl Backend for GilrsBackend {
    fn poll(&mut self, pads: &mut PadState) {
        if !std::mem::replace(&mut self.scanned, true) {
            for gilrs_id in 0..self.gilrs.last_gamepad_hint() {
                if self
                    .gilrs
                    .gamepad(gilrs_id)
                    .is_some_and(Gamepad::is_connected)
                {
                    self.connect(pads, gilrs_id);
                }
            }
        }

        while let Some(event) = self.gilrs.next_event() {
            if let EventType::Connected = event.event {
                self.connect(pads, event.id);
                continue;
            }
            let Some(known) = self.known.get_mut(&event.id) else {
                continue;
            };
            let (code, value) = match event.event {
                EventType::Disconnected => {
                    known.hats.fill((0, 0));
                    pads.set_connected(known.id, false);
                    continue;
                }
                EventType::ButtonPressed(code) => (code, 1),
                EventType::ButtonReleased(code) => (code, 0),
                EventType::AxisValueChanged(value, code) => (code, value),
                _ => continue,
            };
            let Some(device) = pads.devices.get_mut(known.id) else {
                continue;
            };
            match known.controls.get(&code.into_u32()) {
                Some(&Control::Button(index)) => device.set_button(index, value != 0),
                Some(&Control::Axis(index)) => device.set_axis(index, value),
                Some(&Control::Hat { index, y }) => {
                    let hat = &mut known.hats[index];
                    if y {
                        hat.1 = value;
                    } else {
                        hat.0 = value;
                    }
                    device.set_hat(index, HatDirection::from_xy(hat.0, hat.1));
                }
                None => {}
            }
        }
    }
}
