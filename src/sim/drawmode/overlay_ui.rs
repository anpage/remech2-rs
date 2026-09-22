use egui::Context;

use crate::drawmode::{ScalingMode, fit_to_window, show_framebuffer};
use crate::settings::SETTINGS;
#[cfg(feature = "debug-overlay")]
use crate::sim::drawmode::debug_overlay::DebugOverlay;

pub struct OverlayUi {
    widescreen: bool,
    scaling: ScalingMode,
    #[cfg(feature = "debug-overlay")]
    debug_overlay: DebugOverlay,
}

impl OverlayUi {
    pub fn new(ctx: &Context) -> Self {
        let widescreen = SETTINGS.get_bool("video", "widescreen", false);
        Self {
            widescreen,
            scaling: ScalingMode::from_settings(),
            #[cfg(feature = "debug-overlay")]
            debug_overlay: DebugOverlay::new(ctx),
        }
    }

    pub fn ui(
        &mut self,
        ctx: &Context,
        source_size: [f32; 2],
        window_width: f32,
        window_height: f32,
    ) {
        let aspect_ratio = if self.widescreen {
            const { 16.0 / 9.0 }
        } else {
            const { 4.0 / 3.0 }
        };
        let size = fit_to_window(window_width, window_height, aspect_ratio);

        show_framebuffer(ctx, source_size, size, self.scaling);

        #[cfg(feature = "debug-overlay")]
        self.debug_overlay.draw(ctx, window_width, window_height);
    }
}
