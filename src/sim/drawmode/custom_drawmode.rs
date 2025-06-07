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

use crate::{launcher::painter::Painter, sim::drawmode::hooks::PaletteColor};

pub struct CustomDrawMode {
    ctx: Context,
    painter: Painter,
    texture: TextureHandle,
    palette: [[u8; 3]; 256],
    cached_width: i32,
    cached_height: i32,
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

        let image = Arc::new(ColorImage::new([1024, 768], Color32::BLACK));
        let texture = ctx.load_texture("sim-framebuffer", Arc::clone(&image), Default::default());

        Ok(Self {
            painter,
            ctx,
            texture,
            palette: [[0; 3]; 256],
            cached_width: window_width,
            cached_height: window_height,
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

        let raw_input = RawInput {
            screen_rect: Some(egui::Rect {
                min: egui::pos2(0.0, 0.0),
                max: egui::pos2(window_width as f32, window_height as f32),
            }),
            ..Default::default()
        };

        let pixels = pixel_data
            .iter()
            .map(|&p| {
                let color = self.palette[p as usize];
                Color32::from_rgb(color[0] * 4, color[1] * 4, color[2] * 4)
            })
            .collect::<Vec<_>>();

        // Update the texture with the current pixel data
        self.texture.set(
            ColorImage {
                size: [game_width, game_height],
                pixels,
            },
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

        let full_output = self.ctx.run(raw_input, |ctx| {
            egui::CentralPanel::default()
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
                            });
                        },
                    )
                });
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
}
