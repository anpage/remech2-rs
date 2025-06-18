use std::{
    num::{NonZero, NonZeroIsize},
    sync::Arc,
};

use anyhow::Result;
use egui::{Color32, ColorImage, Context, RawInput, TextureHandle};
use egui_wgpu::{WgpuConfiguration, WgpuSetupCreateNew};
use wgpu::InstanceDescriptor;
use windows::Win32::{
    Foundation::{HINSTANCE, HWND},
    System::LibraryLoader::GetModuleHandleA,
};

use crate::{
    launcher::painter::Painter,
    shell::drawmode::{
        hooks::{G_CURSOR_GRAPHIC, get_mouse_state},
        overlay_ui::OverlayUi,
    },
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

pub struct CustomDrawMode {
    ctx: Context,
    painter: Painter,
    texture: TextureHandle,
    cursor_texture: Option<TextureHandle>,
    palette: [[u8; 3]; 256],
    cached_width: i32,
    cached_height: i32,
    cached_mouse_state: OverlayMouseState,
    overlay_ui: OverlayUi,
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
            cursor_texture: None,
            palette: [[0; 3]; 256],
            cached_width: window_width,
            cached_height: window_height,
            cached_mouse_state: Default::default(),
            overlay_ui: Default::default(),
        })
    }

    pub fn load_cursor_texture(&mut self) {
        if unsafe { G_CURSOR_GRAPHIC.is_null() } {
            return;
        }

        let cursor_data = unsafe { **G_CURSOR_GRAPHIC };
        let cursor_data = &cursor_data[0x28..0x1A7];
        const WIDTH: usize = 29;
        const HEIGHT: usize = 25;

        const PALETTE: [u8; 15] = [
            0, 255, 238, 221, 204, 187, 170, 153, 127, 102, 85, 68, 51, 34, 17,
        ];

        const NUM_PIXELS: usize = WIDTH * HEIGHT;
        let mut pixels = vec![Color32::TRANSPARENT; NUM_PIXELS];
        let mut position = 0usize; // Linear position in the pixel buffer
        let mut data = cursor_data.iter();

        while position < NUM_PIXELS {
            if let Some(&ctrl) = data.next() {
                match ctrl {
                    0 => {
                        // Move to next line
                        let current_y = position / WIDTH;
                        position = (current_y + 1) * WIDTH;
                    }
                    1 => {
                        // Skip pixels
                        if let Some(&skip_count) = data.next() {
                            position = (position + skip_count as usize).min(NUM_PIXELS);
                        }
                    }
                    ctrl if ctrl & 1 == 0 => {
                        // Write same pixel value multiple times
                        if let Some(&pixel_value) = data.next() {
                            let count = (ctrl >> 1) as usize;
                            let color_value = PALETTE.get(pixel_value as usize).unwrap();
                            let color = Color32::from_gray(*color_value);
                            (0..count).for_each(|_| {
                                if position >= NUM_PIXELS {
                                    return;
                                }
                                pixels[position] = color;
                                position += 1;
                            });
                        }
                    }
                    ctrl => {
                        // Write multiple different pixel values
                        let count = (ctrl >> 1) as usize;
                        data.by_ref().take(count).for_each(|&pixel_value| {
                            if position >= NUM_PIXELS {
                                return;
                            }
                            let color_value = PALETTE.get(pixel_value as usize).unwrap();
                            pixels[position] = Color32::from_gray(*color_value);
                            if position % WIDTH < WIDTH - 1 {
                                position += 1;
                            }
                        });
                    }
                }
            } else {
                break;
            }
        }

        let cursor_image = Arc::new(ColorImage::new([WIDTH, HEIGHT], pixels));
        let cursor_texture =
            self.ctx
                .load_texture("cursor", Arc::clone(&cursor_image), Default::default());

        self.cursor_texture = Some(cursor_texture);
    }

    pub fn draw(
        &mut self,
        pixel_data: &[u8],
        game_width: usize,
        game_height: usize,
        window_width: i32,
        window_height: i32,
    ) {
        if self.cursor_texture.is_none() {
            self.load_cursor_texture();
        }

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

        let full_output = self.ctx.run(raw_input, |ctx| {
            self.overlay_ui.ui(
                ctx,
                self.texture.id(),
                self.cursor_texture.as_ref().map(|t| t.id()),
                window_width as f32,
                window_height as f32,
                &self.cached_mouse_state,
            );
        });

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

    pub fn show_cursor(&mut self, show: bool) {
        self.overlay_ui.show_cursor(show);
    }
}
