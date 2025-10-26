use std::sync::Arc;

use egui::{Context, Frame, Margin, TextureId, Vec2, load::SizedTexture};

use crate::sim::{
    G_WINDOW_ACTIVE,
    drawmode::hooks::{G_MOUSE_CAPTURED, G_MOUSE_NEEDS_CENTERING},
};

pub struct OverlayUi {
    fonts: egui::FontDefinitions,
}

impl Default for OverlayUi {
    fn default() -> Self {
        // Load the Science Gothic font
        let font = egui::FontData::from_static(include_bytes!("../../../ScienceGothic-Reg.ttf"));
        let mut fonts = egui::FontDefinitions::default();
        fonts
            .font_data
            .insert("ScienceGothic".to_owned(), Arc::new(font));
        fonts
            .families
            .get_mut(&egui::FontFamily::Proportional)
            .unwrap()
            .insert(0, "ScienceGothic".to_owned());

        Self { fonts }
    }
}

impl OverlayUi {
    pub fn ui(&mut self, ctx: &Context, texture: TextureId, window_width: f32, window_height: f32) {
        // calculate width and height, preserving 4:3 aspect ratio
        let aspect_ratio = const { 4.0 / 3.0 };
        let mut width = window_width;
        let mut height = window_height;
        if width / height > aspect_ratio {
            width = height * aspect_ratio;
        } else {
            height = width / aspect_ratio;
        }

        // ctx.set_pixels_per_point(2.0);
        ctx.set_fonts(self.fonts.clone());

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
                            id: texture,
                            size: Vec2::new(width, height),
                        })
                    },
                )
            });

        if cfg!(debug_assertions) {
            egui::Window::new("DEBUG")
                .resizable(false)
                .collapsible(false)
                .default_pos(egui::pos2(10.0, 10.0))
                .show(ctx, |ui| {
                    ui.label(format!("WINDOW SIZE: {}x{}", window_width, window_height));
                    ui.label(format!("WINDOW ACTIVE: {}", unsafe {
                        (*G_WINDOW_ACTIVE).0
                    }));
                    ui.label(format!("MOUSE CAPTURED: {}", unsafe {
                        (*G_MOUSE_CAPTURED).0
                    }));
                    ui.label(format!("MOUSE NEEDS CENTERING: {}", unsafe {
                        (*G_MOUSE_NEEDS_CENTERING).0
                    }));
                });
        }
    }
}
