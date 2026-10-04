pub mod scaler;

pub use scaler::ScalingMode;

use egui::{Context, Event, Frame, Margin, Rect, Response, Sense, Vec2, pos2, vec2};

use crate::drawmode::scaler::{PaletteData, SharpBilinear};

#[repr(C)]
#[derive(Clone, Copy, Debug)]
pub struct PaletteColor {
    pub red: u8,
    pub green: u8,
    pub blue: u8,
}

pub struct Framebuffer {
    ctx: Context,
    palette: PaletteData,
}

impl Framebuffer {
    pub fn new() -> Self {
        Self {
            ctx: egui::Context::default(),
            palette: [[0.0, 0.0, 0.0, 1.0]; 256],
        }
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
    }

    pub fn set_palette(&mut self, palette_data: &[PaletteColor; 256]) {
        for (i, color) in palette_data.iter().enumerate() {
            self.palette[i] = to_gpu_color(color.red, color.green, color.blue);
        }
    }

    pub fn draw(
        &mut self,
        _pixel_data: &[u8],
        _game_size: [usize; 2],
        window_size: [i32; 2],
        events: Vec<Event>,
        ui: impl FnMut(&Context),
    ) {
        let [window_width, window_height] = window_size;

        let raw_input = egui::RawInput {
            screen_rect: Some(Rect {
                min: pos2(0.0, 0.0),
                max: pos2(window_width as f32, window_height as f32),
            }),
            events,
            ..Default::default()
        };

        let _ = self.ctx.run(raw_input, ui);
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
