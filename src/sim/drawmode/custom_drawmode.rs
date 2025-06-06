use std::{num::NonZeroIsize, sync::Arc};

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
    sim::drawmode::hooks::{PaletteColor, WINDOW_HEIGHT, WINDOW_WIDTH},
};

pub struct CustomDrawMode {
    ctx: Context,
    painter: Painter,
    texture: TextureHandle,
    palette: [[u8; 3]; 256],
}

impl CustomDrawMode {
    pub fn new(wnd: HWND) -> Result<Self> {
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
        })
    }

    pub fn draw(&mut self, pixel_data: &[u8], width: usize, height: usize) {
        let raw_input = RawInput {
            screen_rect: Some(egui::Rect {
                min: egui::pos2(0.0, 0.0),
                max: egui::pos2(WINDOW_WIDTH as f32, WINDOW_HEIGHT as f32),
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
                size: [width, height],
                pixels,
            },
            Default::default(),
        );

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
                                size: Vec2::new(
                                    WINDOW_HEIGHT as f32 / 3. * 4.,
                                    WINDOW_HEIGHT as f32,
                                ),
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
