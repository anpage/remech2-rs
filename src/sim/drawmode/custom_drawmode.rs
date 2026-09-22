use anyhow::Result;
use windows::Win32::Foundation::HWND;

use crate::{
    drawmode::{Framebuffer, PaletteColor},
    sim::drawmode::overlay_ui::OverlayUi,
};

pub struct CustomDrawMode {
    framebuffer: Framebuffer,
    overlay_ui: OverlayUi,
}

impl CustomDrawMode {
    pub fn new(wnd: HWND, window_width: i32, window_height: i32) -> Result<Self> {
        let framebuffer = Framebuffer::new(wnd, window_width, window_height)?;
        let overlay_ui = OverlayUi::new(framebuffer.ctx());

        Ok(Self {
            framebuffer,
            overlay_ui,
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
        let Self {
            framebuffer,
            overlay_ui,
        } = self;

        framebuffer.draw(
            pixel_data,
            [game_width, game_height],
            [window_width, window_height],
            Vec::new(),
            |ctx| {
                overlay_ui.ui(
                    ctx,
                    [game_width as f32, game_height as f32],
                    window_width as f32,
                    window_height as f32,
                );
            },
        );
    }

    pub fn set_palette(&mut self, palette_data: &[PaletteColor; 256]) {
        self.framebuffer.set_palette_6bit(palette_data);
    }
}
