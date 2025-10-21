use std::{process::exit, sync::Arc};

use egui::{
    Context, FontFamily, Frame, Margin, Order, RichText, TextStyle, TextureId, Vec2,
    load::SizedTexture,
};
use windows::Win32::{
    Foundation::{HWND, LPARAM, WPARAM},
    UI::WindowsAndMessaging::{CURSORINFO, GetCursorInfo, SendMessageA, ShowCursor, WM_COMMAND},
};

use crate::shell::drawmode::{
    custom_drawmode::OverlayMouseState, hooks::update_global_mouse_state,
};

pub struct OverlayUi {
    shell_hovered: bool,
    menu_visible: bool,
    show_cursor: bool,
    fonts: egui::FontDefinitions,
    exit_dialog_open: bool,
}

impl Default for OverlayUi {
    fn default() -> Self {
        // Load the Science Gothic font
        let font = egui::FontData::from_static(include_bytes!("../../../ScienceGothic-Md.ttf"));
        let mut fonts = egui::FontDefinitions::default();
        fonts
            .font_data
            .insert("ScienceGothic".to_owned(), Arc::new(font));
        fonts
            .families
            .get_mut(&egui::FontFamily::Proportional)
            .unwrap()
            .insert(0, "ScienceGothic".to_owned());

        Self {
            shell_hovered: false,
            menu_visible: false,
            show_cursor: true,
            fonts,
            exit_dialog_open: false,
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
        hwnd: HWND,
    ) {
        // calculate width and height, preserving 4:3 aspect ratio
        let aspect_ratio = const { 4.0 / 3.0 };
        let mut width = window_width;
        let mut height = window_height;
        let scale_factor;
        if width / height > aspect_ratio {
            width = height * aspect_ratio;
            scale_factor = width / 640.0;
        } else {
            height = width / aspect_ratio;
            scale_factor = height / 480.0;
        }

        let mut menu_open = false;

        // ctx.set_pixels_per_point(2.0);
        ctx.set_fonts(self.fonts.clone());

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
            let handle_menu_button = |id: u16| {
                let w_param: usize = id.into();
                unsafe {
                    SendMessageA(hwnd, WM_COMMAND, WPARAM(w_param), LPARAM(0));
                }
            };

            egui::Window::new("top_menu")
                .resizable(false)
                .collapsible(false)
                .movable(false)
                .title_bar(false)
                .fixed_pos(egui::pos2(window_width as f32 / 2. - width / 2., 0.0))
                .fixed_size(Vec2::new(width, 30.0))
                .show(ctx, |ui| {
                    egui::containers::menu::Bar::new().ui(ui, |ui| {
                        let set_font_size = |ui: &mut egui::Ui| {
                            let style = ui.style_mut();
                            style.text_styles.insert(
                                egui::TextStyle::Button,
                                egui::FontId::new(scale_factor * 10.0, FontFamily::Proportional),
                            );
                            style.override_text_style = Some(TextStyle::Button);
                            style.spacing.item_spacing = Vec2::new(8.0 * scale_factor, 0.0);
                        };
                        set_font_size(ui);
                        if ui
                            .menu_button("CLAN", |ui| {
                                set_font_size(ui);
                                if ui.button("NEW ALLEGIANCE").clicked() {
                                    handle_menu_button(40001);
                                }
                                if ui.button("HALL OF HONOR").clicked() {
                                    handle_menu_button(40002);
                                }
                                if ui.button("QUICKTIPS").clicked() {
                                    handle_menu_button(40050);
                                }
                                ui.separator();
                                if ui.button("FLEE TO DESKTOP").clicked() {
                                    self.exit_dialog_open = true;
                                }
                            })
                            .inner
                            .is_some()
                        {
                            menu_open = true;
                        }
                        if ui
                            .menu_button("OPTIONS", |ui| {
                                set_font_size(ui);
                                if ui.button("COMBAT VARIABLES...").clicked() {
                                    handle_menu_button(40084);
                                }
                                if ui.button("COCKPIT CONTROLS...").clicked() {
                                    handle_menu_button(40011);
                                }
                                if ui.button("MOVIE PLAYBACK...").clicked() {
                                    handle_menu_button(40086);
                                }
                            })
                            .inner
                            .is_some()
                        {
                            menu_open = true;
                        }
                        if ui
                            .menu_button("HELP", |ui| {
                                set_font_size(ui);
                                if ui.button("CODES AND PROCEDURES").clicked() {
                                    handle_menu_button(40012);
                                }
                                if ui.button("TECHNICAL HELP").clicked() {
                                    handle_menu_button(40085);
                                }
                                ui.separator();
                                if ui.button("THE KESHIK").clicked() {
                                    handle_menu_button(40082);
                                }
                            })
                            .inner
                            .is_some()
                        {
                            menu_open = true;
                        }
                    });
                });
        };

        if self.exit_dialog_open {
            egui::Window::new("Flee to Desktop")
                .resizable(false)
                .collapsible(false)
                .pivot(egui::Align2::CENTER_CENTER)
                .fixed_pos(ctx.screen_rect().center())
                .show(ctx, |ui| {
                    ui.label(RichText::new("Embrace Cowardice?").size(20.0));
                    ui.add_space(10.0);
                    ui.with_layout(egui::Layout::right_to_left(egui::Align::Min), |ui| {
                        if ui.button(RichText::new("Yes").size(20.0)).clicked() {
                            exit(0);
                        }
                        if ui.button(RichText::new("No").size(20.0)).clicked() {
                            self.exit_dialog_open = false;
                        }
                    });
                });
        }

        if false {
            egui::Window::new("DEBUG")
                .resizable(false)
                .collapsible(false)
                .default_pos(egui::pos2(10.0, 10.0))
                .show(ctx, |ui| {
                    ui.label(format!(
                        "MOUSE POSITION: ({}, {})",
                        mouse_state.pos_x, mouse_state.pos_y
                    ));
                    ui.label(format!("WINDOW SIZE: {}x{}", window_width, window_height));
                    ui.label(format!(
                        "HOVERING SHELL: {}",
                        if self.shell_hovered { "YES" } else { "NO" }
                    ));
                    ui.label(format!(
                        "LEFT BUTTON: {}",
                        if mouse_state.left_down { "DOWN" } else { "UP" }
                    ));
                    ui.label(format!(
                        "RIGHT BUTTON: {}",
                        if mouse_state.right_down { "DOWN" } else { "UP" }
                    ));
                    ui.label(format!(
                        "MIDDLE BUTTON: {}",
                        if mouse_state.middle_down {
                            "DOWN"
                        } else {
                            "UP"
                        }
                    ));
                });
        }

        if let Some(cursor_texture) = cursor_texture {
            let mut cursor_info = CURSORINFO::default();
            let _ = unsafe { GetCursorInfo(&mut cursor_info) };
            if cursor_info.flags.0 != 0 {
                unsafe { ShowCursor(false) };
            }

            if self.show_cursor {
                let cursor_pos_1 = egui::pos2(mouse_state.pos_x as f32, mouse_state.pos_y as f32);
                let cursor_pos_2 = egui::pos2(
                    mouse_state.pos_x as f32 + 29.0 * 1.5,
                    mouse_state.pos_y as f32 + 25.0 * 1.5,
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

        if (mouse_state.pos_y as f32) < 30.0 * scale_factor {
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
