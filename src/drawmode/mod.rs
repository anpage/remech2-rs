mod scaler;

pub use scaler::ScalingMode;

use std::num::{NonZero, NonZeroIsize};

use anyhow::Result;
use egui::{Context, Event, Frame, Margin, Rect, Response, Sense, Vec2, pos2, vec2};
use egui_wgpu::{RendererOptions, WgpuConfiguration};
use windows::Win32::{
    Foundation::{HINSTANCE, HWND},
    System::LibraryLoader::GetModuleHandleA,
};

use crate::{
    drawmode::scaler::{PaletteData, Scaler, SharpBilinear},
    launcher::painter::Painter,
};

#[repr(C)]
#[derive(Clone, Copy, Debug)]
pub struct PaletteColor {
    pub red: u8,
    pub green: u8,
    pub blue: u8,
}

pub struct Framebuffer {
    ctx: Context,
    painter: Painter,
    palette: PaletteData,
    palette_dirty: bool,
    cached_width: i32,
    cached_height: i32,
}

impl Framebuffer {
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
        let mut painter = pollster::block_on(Painter::new(
            WgpuConfiguration::default(),
            false,
            RendererOptions {
                msaa_samples: 0,
                depth_stencil_format: None,
                dithering: false,
                predictable_texture_filtering: false,
            },
        ));
        unsafe {
            pollster::block_on(painter.set_window(ctx.viewport_id(), Some(&window)))?;
        }

        Ok(Self {
            ctx,
            painter,
            palette: [[0.0, 0.0, 0.0, 1.0]; 256],
            palette_dirty: true,
            cached_width: window_width,
            cached_height: window_height,
        })
    }

    pub fn ctx(&self) -> &Context {
        &self.ctx
    }

    /// Pre-scale a 6-bit palette to 8-bit
    pub fn set_palette_6bit(&mut self, palette_data: &[PaletteColor; 256]) {
        let scale = |v: u8| (v.min(63) << 2) | (v.min(63) >> 4);
        for (i, color) in palette_data.iter().enumerate() {
            self.palette[i] = to_gpu_color(scale(color.red), scale(color.green), scale(color.blue));
        }
        self.palette_dirty = true;
    }

    pub fn set_palette(&mut self, palette_data: &[PaletteColor; 256]) {
        for (i, color) in palette_data.iter().enumerate() {
            self.palette[i] = to_gpu_color(color.red, color.green, color.blue);
        }
        self.palette_dirty = true;
    }

    pub fn draw(
        &mut self,
        pixel_data: &[u8],
        game_size: [usize; 2],
        window_size: [i32; 2],
        events: Vec<Event>,
        ui: impl FnMut(&Context),
    ) {
        let [window_width, window_height] = window_size;

        if self.cached_width != window_width || self.cached_height != window_height {
            self.painter.on_window_resized(
                self.ctx.viewport_id(),
                NonZero::new(window_width as u32).unwrap(),
                NonZero::new(window_height as u32).unwrap(),
            );
            self.cached_width = window_width;
            self.cached_height = window_height;
        }

        let raw_input = egui::RawInput {
            screen_rect: Some(Rect {
                min: pos2(0.0, 0.0),
                max: pos2(window_width as f32, window_height as f32),
            }),
            events,
            ..Default::default()
        };

        if let Some(render_state) = self.painter.render_state() {
            let mut renderer = render_state.renderer.write();
            let scaler = renderer
                .callback_resources
                .entry::<Scaler>()
                .or_insert_with(|| Scaler::new(render_state));
            if self.palette_dirty {
                scaler.upload_palette(render_state, &self.palette);
                self.palette_dirty = false;
            }
            scaler.upload(render_state, pixel_data, game_size);
        }

        let full_output = self.ctx.run(raw_input, ui);

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
}

fn to_gpu_color(red: u8, green: u8, blue: u8) -> [f32; 4] {
    [
        red as f32 / 255.0,
        green as f32 / 255.0,
        blue as f32 / 255.0,
        1.0,
    ]
}

/// The largest rect with the given aspect ratio that fits in the window
pub fn fit_to_window(window_width: f32, window_height: f32, aspect_ratio: f32) -> Vec2 {
    if window_width / window_height > aspect_ratio {
        Vec2::new(window_height * aspect_ratio, window_height)
    } else {
        Vec2::new(window_width, window_width / aspect_ratio)
    }
}

/// Snap a rect to whole physical pixels
fn round_to_pixels(rect: Rect, pixels_per_point: f32) -> Rect {
    let round = |v: f32| (v * pixels_per_point).round() / pixels_per_point;
    Rect::from_min_size(
        pos2(round(rect.min.x), round(rect.min.y)),
        vec2(round(rect.width()), round(rect.height())),
    )
}

/// Paints the framebuffer centered in a full-window panel, returning the panel's response
pub fn show_framebuffer(
    ctx: &Context,
    source_size: [f32; 2],
    size: Vec2,
    mode: ScalingMode,
) -> Response {
    egui::CentralPanel::default()
        .frame(Frame {
            inner_margin: Margin::same(0),
            ..Default::default()
        })
        .show(ctx, |ui| {
            ui.with_layout(
                egui::Layout::centered_and_justified(egui::Direction::LeftToRight),
                |ui| {
                    let (rect, response) = ui.allocate_exact_size(size, Sense::hover());
                    let pixels_per_point = ui.pixels_per_point();
                    let rect = round_to_pixels(rect, pixels_per_point);
                    ui.painter().add(egui_wgpu::Callback::new_paint_callback(
                        rect,
                        SharpBilinear {
                            source_size,
                            output_size: [
                                rect.width() * pixels_per_point,
                                rect.height() * pixels_per_point,
                            ],
                            mode,
                        },
                    ));
                    response
                },
            )
        })
        .response
}
