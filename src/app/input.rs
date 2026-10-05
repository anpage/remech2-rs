use winit::{
    event::{ElementState, MouseButton, MouseScrollDelta, WindowEvent},
    keyboard::PhysicalKey,
};

use crate::input::{
    KeyCode,
    kbm::{self, KbmEvent, WheelDirection},
};

/// Touchpad scrolling to wheel detents
const PIXELS_PER_NOTCH: f64 = 50.0;

#[derive(Default)]
pub struct Feed {
    wheel: [f64; 2],
    motion: [f64; 2],
}

impl Feed {
    pub fn window_event(&mut self, event: &WindowEvent) {
        match event {
            WindowEvent::KeyboardInput { event, .. } => {
                let PhysicalKey::Code(code) = event.physical_key else {
                    return;
                };
                if let Some(key) = KeyCode::from_winit(code) {
                    kbm::push(KbmEvent::Key {
                        key,
                        pressed: event.state == ElementState::Pressed,
                    });
                }
            }
            WindowEvent::MouseInput { state, button, .. } => {
                let button = match button {
                    MouseButton::Left => kbm::MouseButton::Left,
                    MouseButton::Right => kbm::MouseButton::Right,
                    MouseButton::Middle => kbm::MouseButton::Middle,
                    MouseButton::Back => kbm::MouseButton::X1,
                    MouseButton::Forward => kbm::MouseButton::X2,
                    MouseButton::Other(_) => return,
                };
                kbm::push(KbmEvent::MouseButton {
                    button,
                    pressed: *state == ElementState::Pressed,
                });
            }
            WindowEvent::MouseWheel { delta, .. } => {
                let (x, y) = match *delta {
                    MouseScrollDelta::LineDelta(x, y) => (f64::from(x), f64::from(y)),
                    MouseScrollDelta::PixelDelta(position) => {
                        (position.x / PIXELS_PER_NOTCH, position.y / PIXELS_PER_NOTCH)
                    }
                };
                self.wheel(0, x, WheelDirection::Left, WheelDirection::Right);
                self.wheel(1, y, WheelDirection::Up, WheelDirection::Down);
            }
            WindowEvent::Focused(focused) => kbm::push(KbmEvent::Focus(*focused)),
            _ => {}
        }
    }

    fn wheel(
        &mut self,
        axis: usize,
        delta: f64,
        positive: WheelDirection,
        negative: WheelDirection,
    ) {
        let travel = &mut self.wheel[axis];
        *travel += delta;
        while *travel >= 1.0 {
            kbm::push(KbmEvent::WheelNotch(positive));
            *travel -= 1.0;
        }
        while *travel <= -1.0 {
            kbm::push(KbmEvent::WheelNotch(negative));
            *travel += 1.0;
        }
    }

    pub fn motion(&mut self, delta: (f64, f64)) {
        let [x, y] = &mut self.motion;
        *x += delta.0;
        *y += delta.1;
        let (dx, dy) = (x.trunc(), y.trunc());
        if dx != 0.0 || dy != 0.0 {
            *x -= dx;
            *y -= dy;
            kbm::push(KbmEvent::MouseMotion {
                dx: dx as i32,
                dy: dy as i32,
            });
        }
    }
}
