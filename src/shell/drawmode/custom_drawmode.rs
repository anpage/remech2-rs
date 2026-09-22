use std::sync::Arc;

use anyhow::Result;
use egui::{Color32, ColorImage, TextureHandle};
use windows::Win32::Foundation::HWND;

use crate::{
    drawmode::{Framebuffer, PaletteColor},
    shell::drawmode::{
        hooks::{G_CURSOR_GRAPHIC, get_mouse_state},
        overlay_ui::OverlayUi,
    },
};

#[derive(Clone, Debug, Default)]
pub struct OverlayMouseState {
    pub pos_x: i32,
    pub pos_y: i32,
    pub left_down: bool,
    pub right_down: bool,
    pub middle_down: bool,
}

pub struct CustomDrawMode {
    framebuffer: Framebuffer,
    cursor_texture: Option<TextureHandle>,
    cached_mouse_state: OverlayMouseState,
    overlay_ui: OverlayUi,
}

impl CustomDrawMode {
    pub fn new(wnd: HWND, window_width: i32, window_height: i32) -> Result<Self> {
        let framebuffer = Framebuffer::new(wnd, window_width, window_height)?;
        let overlay_ui = OverlayUi::new(framebuffer.ctx());

        Ok(Self {
            framebuffer,
            cursor_texture: None,
            cached_mouse_state: Default::default(),
            overlay_ui,
        })
    }

    pub fn load_cursor_texture(&mut self) {
        let cursor_graphic = G_CURSOR_GRAPHIC.ptr();
        if cursor_graphic.is_null() || unsafe { (*cursor_graphic).is_null() } {
            return;
        }

        let cursor_data = unsafe { **cursor_graphic };
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
        let cursor_texture = self.framebuffer.ctx().load_texture(
            "cursor",
            Arc::clone(&cursor_image),
            Default::default(),
        );

        self.cursor_texture = Some(cursor_texture);
    }

    pub fn draw(
        &mut self,
        pixel_data: &[u8],
        game_width: usize,
        game_height: usize,
        window_width: i32,
        window_height: i32,
        hwnd: HWND,
    ) {
        if self.cursor_texture.is_none() {
            self.load_cursor_texture();
        }

        let mut events = Vec::new();
        let mouse_state = unsafe { get_mouse_state() };

        // Mouse moved
        if mouse_state.pos_x != self.cached_mouse_state.pos_x
            || mouse_state.pos_y != self.cached_mouse_state.pos_y
        {
            events.push(egui::Event::PointerMoved(egui::pos2(
                mouse_state.pos_x as f32,
                mouse_state.pos_y as f32,
            )));
        }

        // Mouse button pressed
        if mouse_state.left_down && !self.cached_mouse_state.left_down {
            events.push(egui::Event::PointerButton {
                pos: egui::pos2(mouse_state.pos_x as f32, mouse_state.pos_y as f32),
                button: egui::PointerButton::Primary,
                pressed: true,
                modifiers: Default::default(),
            });
        }

        if mouse_state.right_down && !self.cached_mouse_state.right_down {
            events.push(egui::Event::PointerButton {
                pos: egui::pos2(mouse_state.pos_x as f32, mouse_state.pos_y as f32),
                button: egui::PointerButton::Secondary,
                pressed: true,
                modifiers: Default::default(),
            });
        }

        if mouse_state.middle_down && !self.cached_mouse_state.middle_down {
            events.push(egui::Event::PointerButton {
                pos: egui::pos2(mouse_state.pos_x as f32, mouse_state.pos_y as f32),
                button: egui::PointerButton::Middle,
                pressed: true,
                modifiers: Default::default(),
            });
        }

        // Mouse button released
        if !mouse_state.left_down && self.cached_mouse_state.left_down {
            events.push(egui::Event::PointerButton {
                pos: egui::pos2(mouse_state.pos_x as f32, mouse_state.pos_y as f32),
                button: egui::PointerButton::Primary,
                pressed: false,
                modifiers: Default::default(),
            });
        }

        if !mouse_state.right_down && self.cached_mouse_state.right_down {
            events.push(egui::Event::PointerButton {
                pos: egui::pos2(mouse_state.pos_x as f32, mouse_state.pos_y as f32),
                button: egui::PointerButton::Secondary,
                pressed: false,
                modifiers: Default::default(),
            });
        }

        if !mouse_state.middle_down && self.cached_mouse_state.middle_down {
            events.push(egui::Event::PointerButton {
                pos: egui::pos2(mouse_state.pos_x as f32, mouse_state.pos_y as f32),
                button: egui::PointerButton::Middle,
                pressed: false,
                modifiers: Default::default(),
            });
        }

        self.cached_mouse_state = mouse_state;

        let Self {
            framebuffer,
            cursor_texture,
            cached_mouse_state,
            overlay_ui,
        } = self;

        framebuffer.draw(
            pixel_data,
            [game_width, game_height],
            [window_width, window_height],
            events,
            |ctx| {
                overlay_ui.ui(
                    ctx,
                    [game_width as f32, game_height as f32],
                    cursor_texture.as_ref().map(|t| t.id()),
                    (window_width as f32, window_height as f32),
                    cached_mouse_state,
                    hwnd,
                );
            },
        );
    }

    /// Pre-scale a 6-bit palette to 8-bit
    pub fn set_palette_6bit(&mut self, palette_data: &[PaletteColor; 256]) {
        self.framebuffer.set_palette_6bit(palette_data);
    }

    pub fn set_palette(&mut self, palette_data: &[PaletteColor; 256]) {
        self.framebuffer.set_palette(palette_data);
    }

    pub fn show_cursor(&mut self, show: bool) {
        self.overlay_ui.show_cursor(show);
    }

    pub fn update_mouse_state(&self) {
        self.overlay_ui.update_mouse_state();
    }
}
