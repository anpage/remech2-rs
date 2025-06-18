use egui::{Context, Frame, LayerId, Margin, Order, TextureId, Vec2, load::SizedTexture};
use windows::Win32::UI::WindowsAndMessaging::{CURSORINFO, GetCursorInfo, ShowCursor};

use crate::shell::drawmode::{
    custom_drawmode::OverlayMouseState, hooks::update_global_mouse_state,
};

pub struct OverlayUi {
    shell_hovered: bool,
    menu_visible: bool,
    show_cursor: bool,
}

impl Default for OverlayUi {
    fn default() -> Self {
        Self {
            shell_hovered: false,
            menu_visible: false,
            show_cursor: true,
        }
    }
}

impl OverlayUi {
    pub fn ui(
        &mut self,
        ctx: &Context,
        texture: TextureId,
        cursor_texture: Option<TextureId>,
        window_width: f32,
        window_height: f32,
        mouse_state: &OverlayMouseState,
    ) {
        // calculate width and height, preserving 4:3 aspect ratio
        let aspect_ratio = const { 4.0 / 3.0 };
        let mut width = window_width;
        let mut height = window_height;
        if width / height > aspect_ratio {
            width = height * aspect_ratio;
        } else {
            height = width / aspect_ratio;
        }

        let mut menu_open = false;

        // ctx.set_pixels_per_point(2.0);

        let response = egui::CentralPanel::default()
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
            })
            .response;

        if response.contains_pointer() {
            self.shell_hovered = true;
        } else {
            self.shell_hovered = false;
        }

        if self.menu_visible {
            egui::Window::new("top_menu")
                .resizable(false)
                .collapsible(false)
                .movable(false)
                .title_bar(false)
                .fixed_pos(egui::pos2(window_width as f32 / 2. - width / 2., 0.0))
                .fixed_size(Vec2::new(width, 30.0))
                .show(ctx, |ui| {
                    egui::containers::menu::Bar::new().ui(ui, |ui| {
                        if ui
                            .menu_button("Clan", |ui| {
                                if ui.button("New Allegiance").clicked() {}
                                if ui.button("Hall of Honor").clicked() {}
                                if ui.button("QuickTips").clicked() {}
                                ui.separator();
                                if ui.button("Flee to Desktop").clicked() {}
                            })
                            .inner
                            .is_some()
                        {
                            menu_open = true;
                        }
                        if ui
                            .menu_button("Options", |ui| {
                                if ui.button("Combat Variables...").clicked() {}
                                if ui.button("Cockpit Controls...").clicked() {}
                                if ui.button("Movie Playback...").clicked() {}
                            })
                            .inner
                            .is_some()
                        {
                            menu_open = true;
                        }
                        if ui
                            .menu_button("Help", |ui| {
                                if ui.button("Codes and Procedures").clicked() {}
                                if ui.button("Technical Help").clicked() {}
                                ui.separator();
                                if ui.button("The Keshik").clicked() {}
                            })
                            .inner
                            .is_some()
                        {
                            menu_open = true;
                        }
                    });
                });
        };
        egui::Window::new("Mouse State")
            .resizable(false)
            .collapsible(false)
            .default_pos(egui::pos2(10.0, 10.0))
            .show(ctx, |ui| {
                ui.label(format!(
                    "Mouse Position: ({}, {})",
                    mouse_state.pos_x, mouse_state.pos_y
                ));
                ui.label(format!("Window Size: {}x{}", window_width, window_height));
                ui.label(format!(
                    "Hovering Shell: {}",
                    if self.shell_hovered { "Yes" } else { "No" }
                ));
                ui.label(format!(
                    "Left Button: {}",
                    if mouse_state.left_down { "Down" } else { "Up" }
                ));
                ui.label(format!(
                    "Right Button: {}",
                    if mouse_state.right_down { "Down" } else { "Up" }
                ));
                ui.label(format!(
                    "Middle Button: {}",
                    if mouse_state.middle_down {
                        "Down"
                    } else {
                        "Up"
                    }
                ));
            });

        if let Some(cursor_texture) = cursor_texture {
            let mut cursor_info = CURSORINFO::default();
            let _ = unsafe { GetCursorInfo(&mut cursor_info) };
            if cursor_info.flags.0 != 0 {
                unsafe { ShowCursor(false) };
            }

            if self.show_cursor {
                let cursor_pos_1 = egui::pos2(mouse_state.pos_x as f32, mouse_state.pos_y as f32);
                let cursor_pos_2 = egui::pos2(
                    mouse_state.pos_x as f32 + 29.0,
                    mouse_state.pos_y as f32 + 25.0,
                );
                let cursor_image = egui::Shape::image(
                    cursor_texture,
                    egui::Rect::from_two_pos(cursor_pos_1, cursor_pos_2),
                    egui::Rect::from_min_max(egui::pos2(0.0, 0.0), egui::pos2(1.0, 1.0)),
                    egui::Color32::WHITE,
                );
                ctx.layer_painter(egui::LayerId::new(
                    Order::Foreground,
                    egui::Id::new("cursor_layer"),
                ))
                .add(cursor_image);
            }
        }

        if mouse_state.pos_y < 30 {
            self.menu_visible = true;
        } else if self.shell_hovered && !menu_open {
            self.menu_visible = false;
        }

        if self.shell_hovered {
            update_global_mouse_state(mouse_state);
        } else {
            update_global_mouse_state(&OverlayMouseState::default());
        }
    }

    pub fn show_cursor(&mut self, show_cursor: bool) {
        self.show_cursor = show_cursor;
    }
}
