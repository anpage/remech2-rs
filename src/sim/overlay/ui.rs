use egui::Context;

#[cfg(feature = "debug-overlay")]
use crate::sim::overlay::debug_overlay::DebugOverlay;

pub struct OverlayUi {
    #[cfg(feature = "debug-overlay")]
    debug_overlay: DebugOverlay,
}

impl OverlayUi {
    #[cfg_attr(not(feature = "debug-overlay"), expect(unused_variables))]
    pub fn new(ctx: &Context) -> Self {
        Self {
            #[cfg(feature = "debug-overlay")]
            debug_overlay: DebugOverlay::new(ctx),
        }
    }

    #[cfg_attr(not(feature = "debug-overlay"), expect(unused_variables))]
    pub fn ui(&mut self, ctx: &Context) {
        #[cfg(feature = "debug-overlay")]
        {
            let window_size = ctx.content_rect().size();
            self.debug_overlay.draw(ctx, window_size.x, window_size.y);
        }
    }
}
