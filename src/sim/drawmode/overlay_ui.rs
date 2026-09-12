use egui::{Context, Frame, Margin, TextureId, Vec2, load::SizedTexture};

use crate::settings::SETTINGS;
#[cfg(feature = "debug-overlay")]
use crate::sim::drawmode::debug_overlay::DebugOverlay;

pub struct OverlayUi {
    widescreen: bool,
    #[cfg(feature = "debug-overlay")]
    debug_overlay: DebugOverlay,
}

impl Default for OverlayUi {
    fn default() -> Self {
        let widescreen = SETTINGS.get_bool("video", "widescreen", false);
        Self {
            widescreen,
            #[cfg(feature = "debug-overlay")]
            debug_overlay: Default::default(),
        }
    }
}

impl OverlayUi {
    pub fn ui(&mut self, ctx: &Context, texture: TextureId, window_width: f32, window_height: f32) {
        // calculate width and height, preserving aspect ratio
        let aspect_ratio = if self.widescreen {
            const { 16.0 / 9.0 }
        } else {
            const { 4.0 / 3.0 }
        };
        let mut width = window_width;
        let mut height = window_height;
        if width / height > aspect_ratio {
            width = height * aspect_ratio;
        } else {
            height = width / aspect_ratio;
        }

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

        #[cfg(feature = "debug-overlay")]
        self.debug_overlay.draw(ctx, window_width, window_height);
    }
}
