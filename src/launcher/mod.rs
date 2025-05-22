use std::num::NonZeroIsize;

use anyhow::{Result, bail};
use egui::{Modifiers, MouseWheelUnit, RawInput};
use egui_wgpu::{WgpuConfiguration, WgpuSetupCreateNew};
use painter::Painter;
use wgpu::InstanceDescriptor;
use windows::Win32::{
    Foundation::{HINSTANCE, HWND},
    UI::WindowsAndMessaging::{
        DispatchMessageA, MSG, PM_REMOVE, PeekMessageA, TranslateMessage, WM_LBUTTONDOWN,
        WM_LBUTTONUP, WM_MOUSEMOVE, WM_MOUSEWHEEL, WM_QUIT,
    },
};

mod cd_check;
mod dll_check;
mod file_check;
mod painter;

type Window = raw_window_handle::Win32WindowHandle;

enum Action {
    /// Stay on current stage
    Nothing,
    /// Move on to another stage
    Continue(Box<dyn Stage>),
    /// Exit the launcher and run the game
    Break,
}

trait Stage {
    fn ui(&mut self, ctx: &egui::Context) -> Result<Action>;
}

pub struct Launcher {
    ctx: egui::Context,
    painter: Painter,
    window: HWND,
    current_stage: Box<dyn Stage>,
}

impl Launcher {
    pub fn new(wnd: HWND, instance: HINSTANCE) -> Result<Self> {
        let window = {
            let mut wnd = raw_window_handle::Win32WindowHandle::new(
                NonZeroIsize::new(wnd.0 as isize).unwrap(),
            );
            wnd.hinstance = Some(NonZeroIsize::new(instance.0 as isize).unwrap());
            wnd
        };
        let ctx = egui::Context::default();
        let config = WgpuConfiguration {
            wgpu_setup: WgpuSetupCreateNew {
                instance_descriptor: InstanceDescriptor {
                    backends: wgpu::Backends::GL,
                    ..Default::default()
                },
                ..Default::default()
            }
            .into(),
            ..Default::default()
        };
        let mut painter = pollster::block_on(Painter::new(config, 2, None, false, false));
        unsafe {
            pollster::block_on(painter.set_window(ctx.viewport_id(), Some(&window)))?;
        }
        Ok(Launcher {
            painter,
            ctx,
            window: wnd,
            current_stage: Box::new(cd_check::CdCheck::new()),
        })
    }

    pub fn launch(&mut self) -> Result<()> {
        let mut lpmsg = MSG::default();
        loop {
            let message_received: bool =
                unsafe { PeekMessageA(&mut lpmsg, Some(self.window), 0, 0, PM_REMOVE).into() };

            let mut raw_input = RawInput {
                screen_rect: Some(egui::Rect {
                    min: egui::pos2(0.0, 0.0),
                    max: egui::pos2(640.0, 480.0),
                }),
                ..Default::default()
            };

            if message_received {
                if lpmsg.message == WM_QUIT {
                    bail!("WM_QUIT received");
                }

                if lpmsg.message == WM_MOUSEMOVE {
                    let x = lpmsg.lParam.0 & 0xFFFF;
                    let y = lpmsg.lParam.0 >> 16;

                    raw_input
                        .events
                        .push(egui::Event::PointerMoved(egui::pos2(x as f32, y as f32)));
                }

                if lpmsg.message == WM_LBUTTONDOWN {
                    let x = lpmsg.lParam.0 & 0xFFFF;
                    let y = lpmsg.lParam.0 >> 16;

                    raw_input.events.push(egui::Event::PointerButton {
                        pos: egui::pos2(x as f32, y as f32),
                        button: egui::PointerButton::Primary,
                        pressed: true,
                        modifiers: Modifiers::default(),
                    });
                }

                if lpmsg.message == WM_LBUTTONUP {
                    let x = lpmsg.lParam.0 & 0xFFFF;
                    let y = lpmsg.lParam.0 >> 16;

                    raw_input.events.push(egui::Event::PointerButton {
                        pos: egui::pos2(x as f32, y as f32),
                        button: egui::PointerButton::Primary,
                        pressed: false,
                        modifiers: Modifiers::default(),
                    });
                }

                if lpmsg.message == WM_MOUSEWHEEL {
                    let distance = (lpmsg.wParam.0 >> 16) as i16;

                    raw_input.events.push(egui::Event::MouseWheel {
                        unit: MouseWheelUnit::Point,
                        delta: egui::vec2(0.0, distance as f32),
                        modifiers: Modifiers::default(),
                    });
                }

                unsafe {
                    let _ = TranslateMessage(&lpmsg);
                    DispatchMessageA(&lpmsg);
                }
            }

            let mut result = Ok(Action::Nothing);
            let full_output = self.ctx.run(raw_input, |ctx| {
                result = self.current_stage.ui(ctx);
            });

            match result {
                Ok(Action::Continue(stage)) => {
                    self.current_stage = stage;
                }
                Ok(Action::Break) => {
                    break;
                }
                Err(e) => {
                    return Err(e);
                }
                _ => {}
            }

            let clipped_primitives = self
                .ctx
                .tessellate(full_output.shapes, full_output.pixels_per_point);

            self.painter.paint_and_update_textures(
                self.ctx.viewport_id(),
                full_output.pixels_per_point,
                [0.0, 0.0, 0.0, 1.0],
                &clipped_primitives,
                &full_output.textures_delta,
            );
        }
        Ok(())
    }
}
