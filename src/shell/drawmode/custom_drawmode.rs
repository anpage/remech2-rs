use std::{
    num::{NonZero, NonZeroIsize},
    sync::Arc,
};

use anyhow::Result;
use egui::{
    Color32, ColorImage, Context, Frame, Margin, RawInput, TextureHandle, Vec2, load::SizedTexture,
};
use egui_wgpu::{WgpuConfiguration, WgpuSetupCreateNew};
use wgpu::InstanceDescriptor;
use windows::Win32::{
    Foundation::{HINSTANCE, HWND},
    System::LibraryLoader::GetModuleHandleA,
};

use crate::{
    launcher::painter::Painter,
    shell::drawmode::hooks::{get_mouse_state, update_global_mouse_state},
};

#[repr(C)]
#[derive(Clone, Copy, Debug)]
pub struct PaletteColor {
    pub red: u8,
    pub green: u8,
    pub blue: u8,
}

#[derive(Clone, Debug)]
pub struct OverlayMouseState {
    pub pos_x: i32,
    pub pos_y: i32,
    pub left_down: bool,
    pub right_down: bool,
    pub middle_down: bool,
}

impl Default for OverlayMouseState {
    fn default() -> Self {
        Self {
            pos_x: 0,
            pos_y: 0,
            left_down: false,
            right_down: false,
            middle_down: false,
        }
    }
}

struct UiState {
    shell_hovered: bool,
    menu_visible: bool,
}

impl Default for UiState {
    fn default() -> Self {
        Self {
            shell_hovered: false,
            menu_visible: false,
        }
    }
}

pub struct CustomDrawMode {
    ctx: Context,
    painter: Painter,
    texture: TextureHandle,
    palette: [[u8; 3]; 256],
    cached_width: i32,
    cached_height: i32,
    cached_mouse_state: OverlayMouseState,
    ui_state: UiState,
}

impl CustomDrawMode {
    pub fn new(wnd: HWND, window_width: i32, window_height: i32) -> Result<Self> {
        let instance: HINSTANCE = unsafe { GetModuleHandleA(None)?.into() };

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

        let image = Arc::new(ColorImage::new([1, 1], vec![Color32::BLACK]));
        let texture = ctx.load_texture("sim-framebuffer", Arc::clone(&image), Default::default());

        Ok(Self {
            painter,
            ctx,
            texture,
            palette: [[0; 3]; 256],
            cached_width: window_width,
            cached_height: window_height,
            cached_mouse_state: Default::default(),
            ui_state: Default::default(),
        })
    }

    pub fn draw(
        &mut self,
        pixel_data: &[u8],
        game_width: usize,
        game_height: usize,
        window_width: i32,
        window_height: i32,
    ) {
        if self.cached_width != window_width || self.cached_height != window_height {
            self.painter.on_window_resized(
                self.ctx.viewport_id(),
                NonZero::new(window_width as u32).unwrap(),
                NonZero::new(window_height as u32).unwrap(),
            );
            self.cached_width = window_width;
            self.cached_height = window_height;
        }

        let mut raw_input = RawInput {
            screen_rect: Some(egui::Rect {
                min: egui::pos2(0.0, 0.0),
                max: egui::pos2(window_width as f32, window_height as f32),
            }),
            ..Default::default()
        };

        let mouse_state = unsafe { get_mouse_state() };

        // Mouse moved
        if mouse_state.pos_x != self.cached_mouse_state.pos_x
            || mouse_state.pos_y != self.cached_mouse_state.pos_y
        {
            raw_input.events.push(egui::Event::PointerMoved(egui::pos2(
                mouse_state.pos_x as f32,
                mouse_state.pos_y as f32,
            )));
        }

        // Mouse button pressed
        if mouse_state.left_down && !self.cached_mouse_state.left_down {
            raw_input.events.push(egui::Event::PointerButton {
                pos: egui::pos2(mouse_state.pos_x as f32, mouse_state.pos_y as f32),
                button: egui::PointerButton::Primary,
                pressed: true,
                modifiers: Default::default(),
            });
        }

        if mouse_state.right_down && !self.cached_mouse_state.right_down {
            raw_input.events.push(egui::Event::PointerButton {
                pos: egui::pos2(mouse_state.pos_x as f32, mouse_state.pos_y as f32),
                button: egui::PointerButton::Secondary,
                pressed: true,
                modifiers: Default::default(),
            });
        }

        if mouse_state.middle_down && !self.cached_mouse_state.middle_down {
            raw_input.events.push(egui::Event::PointerButton {
                pos: egui::pos2(mouse_state.pos_x as f32, mouse_state.pos_y as f32),
                button: egui::PointerButton::Middle,
                pressed: true,
                modifiers: Default::default(),
            });
        }

        // Mouse button released
        if !mouse_state.left_down && self.cached_mouse_state.left_down {
            raw_input.events.push(egui::Event::PointerButton {
                pos: egui::pos2(mouse_state.pos_x as f32, mouse_state.pos_y as f32),
                button: egui::PointerButton::Primary,
                pressed: false,
                modifiers: Default::default(),
            });
        }

        if !mouse_state.right_down && self.cached_mouse_state.right_down {
            raw_input.events.push(egui::Event::PointerButton {
                pos: egui::pos2(mouse_state.pos_x as f32, mouse_state.pos_y as f32),
                button: egui::PointerButton::Secondary,
                pressed: false,
                modifiers: Default::default(),
            });
        }

        if !mouse_state.middle_down && self.cached_mouse_state.middle_down {
            raw_input.events.push(egui::Event::PointerButton {
                pos: egui::pos2(mouse_state.pos_x as f32, mouse_state.pos_y as f32),
                button: egui::PointerButton::Middle,
                pressed: false,
                modifiers: Default::default(),
            });
        }

        self.cached_mouse_state = mouse_state;

        let pixels = pixel_data
            .iter()
            .map(|&p| {
                let color = self.palette[p as usize];
                Color32::from_rgb(color[0] * 4, color[1] * 4, color[2] * 4)
            })
            .collect::<Vec<_>>();

        // Update the texture with the current pixel data
        self.texture.set(
            ColorImage::new([game_width, game_height], pixels),
            Default::default(),
        );

        // calculate width and height, preserving 4:3 aspect ratio
        let aspect_ratio = 4.0 / 3.0;
        let mut width = window_width as f32;
        let mut height = window_height as f32;
        if width / height > aspect_ratio {
            width = height * aspect_ratio;
        } else {
            height = width / aspect_ratio;
        }

        let mut menu_open = false;

        let full_output = self.ctx.run(raw_input, |ctx| {
            // ctx.set_pixels_per_point(2.0);

            let response = egui::CentralPanel::default()
                .frame(Frame {
                    inner_margin: Margin::same(0),
                    ..Default::default()
                })
                .show(ctx, |ui| {
                    ui.with_layout(
                        egui::Layout::centered_and_justified(egui::Direction::LeftToRight),
                        |ui| {
                            ui.image(SizedTexture {
                                id: self.texture.id(),
                                size: Vec2::new(width, height),
                            })
                        },
                    )
                })
                .response;

            if response.contains_pointer() {
                self.ui_state.shell_hovered = true;
            } else {
                self.ui_state.shell_hovered = false;
            }

            if self.ui_state.menu_visible {
                egui::Window::new("top_menu")
                    .resizable(false)
                    .collapsible(false)
                    .movable(false)
                    .title_bar(false)
                    .fixed_pos(egui::pos2(window_width as f32 / 2. - width / 2., 0.0))
                    .fixed_size(Vec2::new(width, 30.0))
                    .show(ctx, |ui| {
                        egui::containers::menu::Bar::new().ui(ui, |ui| {
                            if ui
                                .menu_button("Clan", |ui| {
                                    if ui.button("New Allegiance").clicked() {}
                                    if ui.button("Hall of Honor").clicked() {}
                                    if ui.button("QuickTips").clicked() {}
                                    ui.separator();
                                    if ui.button("Flee to Desktop").clicked() {}
                                })
                                .inner
                                .is_some()
                            {
                                menu_open = true;
                            }
                            if ui
                                .menu_button("Options", |ui| {
                                    if ui.button("Combat Variables...").clicked() {}
                                    if ui.button("Cockpit Controls...").clicked() {}
                                    if ui.button("Movie Playback...").clicked() {}
                                })
                                .inner
                                .is_some()
                            {
                                menu_open = true;
                            }
                            if ui
                                .menu_button("Help", |ui| {
                                    if ui.button("Codes and Procedures").clicked() {}
                                    if ui.button("Technical Help").clicked() {}
                                    ui.separator();
                                    if ui.button("The Keshik").clicked() {}
                                })
                                .inner
                                .is_some()
                            {
                                menu_open = true;
                            }
                        });
                    });
            };
            egui::Window::new("Mouse State")
                .resizable(false)
                .collapsible(false)
                .default_pos(egui::pos2(10.0, 10.0))
                .show(ctx, |ui| {
                    ui.label(format!(
                        "Mouse Position: ({}, {})",
                        self.cached_mouse_state.pos_x, self.cached_mouse_state.pos_y
                    ));
                    ui.label(format!(
                        "Window Size: {}x{}",
                        self.cached_width, self.cached_height
                    ));
                    ui.label(format!(
                        "Hovering Shell: {}",
                        if self.ui_state.shell_hovered {
                            "Yes"
                        } else {
                            "No"
                        }
                    ));
                    ui.label(format!(
                        "Left Button: {}",
                        if self.cached_mouse_state.left_down {
                            "Down"
                        } else {
                            "Up"
                        }
                    ));
                    ui.label(format!(
                        "Right Button: {}",
                        if self.cached_mouse_state.right_down {
                            "Down"
                        } else {
                            "Up"
                        }
                    ));
                    ui.label(format!(
                        "Middle Button: {}",
                        if self.cached_mouse_state.middle_down {
                            "Down"
                        } else {
                            "Up"
                        }
                    ));
                });
        });

        if self.cached_mouse_state.pos_y < 30 {
            self.ui_state.menu_visible = true;
        } else if self.ui_state.shell_hovered && !menu_open {
            self.ui_state.menu_visible = false;
        }

        if self.ui_state.shell_hovered {
            update_global_mouse_state(&self.cached_mouse_state);
        } else {
            update_global_mouse_state(&OverlayMouseState::default());
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

    pub fn set_palette(&mut self, palette_data: &[PaletteColor; 256]) {
        if palette_data.len() != 256 {
            panic!("Palette data must be exactly 256 colors");
        }

        for i in 0..256 {
            let color = palette_data[i];
            self.palette[i] = [color.red, color.green, color.blue];
        }
    }
}
