use std::{process::exit, sync::Arc};

use egui::{
    Context, FontFamily, Frame, Margin, Order, TextStyle, TextureId, Vec2, load::SizedTexture,
};
use tracing::error;
use windows::Win32::{
    Foundation::{HWND, LPARAM, WPARAM},
    UI::WindowsAndMessaging::{
        CURSOR_SHOWING, CURSORINFO, GetCursorInfo, PostMessageA, ShowCursor, WM_COMMAND,
    },
};

use crate::shell::dialog;
use crate::shell::drawmode::{
    confirm,
    custom_drawmode::OverlayMouseState,
    hooks::{get_mouse_state, update_global_mouse_state},
    menu,
};
use crate::{about, shell::screens};

pub struct OverlayUi {
    shell_hovered: bool,
    menu_visible: bool,
    show_cursor: bool,
    fonts: egui::FontDefinitions,
    exit_dialog_open: bool,
    about_dialog_open: bool,
}

impl Default for OverlayUi {
    fn default() -> Self {
        // Load the Squarish Sans font
        let font =
            egui::FontData::from_static(include_bytes!("../../../Squarish_Sans_CT_Regular_SC.ttf"));
        let mut fonts = egui::FontDefinitions::default();
        fonts
            .font_data
            .insert("SquarishSans".to_owned(), Arc::new(font));
        fonts
            .families
            .get_mut(&egui::FontFamily::Proportional)
            .unwrap()
            .insert(0, "SquarishSans".to_owned());

        Self {
            shell_hovered: false,
            menu_visible: false,
            show_cursor: true,
            fonts,
            exit_dialog_open: false,
            about_dialog_open: false,
        }
    }
}

impl OverlayUi {
    pub fn ui(
        &mut self,
        ctx: &Context,
        texture: TextureId,
        cursor_texture: Option<TextureId>,
        window_size: (f32, f32),
        mouse_state: &OverlayMouseState,
        hwnd: HWND,
    ) {
        // calculate width and height, preserving 4:3 aspect ratio
        let aspect_ratio = const { 4.0 / 3.0 };
        let (mut width, mut height) = window_size;

        let scale_factor = if width / height > aspect_ratio {
            width = height * aspect_ratio;
            width / 640.0
        } else {
            height = width / aspect_ratio;
            height / 480.0
        };

        let mut menu_open = false;

        let menu_locked = menu::locked();
        if menu_locked {
            self.menu_visible = false;
        }

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

        self.shell_hovered = response.contains_pointer();

        if self.menu_visible {
            let handle_menu_button = |id: u16| {
                let w_param: usize = id.into();
                unsafe {
                    if let Err(e) = PostMessageA(Some(hwnd), WM_COMMAND, WPARAM(w_param), LPARAM(0))
                    {
                        error!("menu command {id} failed: {e}");
                    }
                }
            };

            egui::Window::new("top_menu")
                .resizable(false)
                .collapsible(false)
                .movable(false)
                .title_bar(false)
                .fixed_pos(egui::pos2(window_size.0 / 2. - width / 2., 0.0))
                .fixed_size(Vec2::new(width, 30.0))
                .show(ctx, |ui| {
                    egui::containers::menu::MenuBar::new().ui(ui, |ui| {
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
                            .menu_button("Clan", |ui| {
                                set_font_size(ui);
                                if ui.button("New Alliance").clicked() {
                                    handle_menu_button(40001);
                                }
                                if ui.button("Hall of Honor").clicked() {
                                    handle_menu_button(40002);
                                }
                                ui.separator();
                                if ui.button("Flee to Desktop").clicked() {
                                    // Originally menu item 40003
                                    self.exit_dialog_open = true;
                                }
                            })
                            .inner
                            .is_some()
                        {
                            menu_open = true;
                        }
                        if ui
                            .menu_button("Options", |ui| {
                                set_font_size(ui);
                                if ui.button("Combat Variables...").clicked() {
                                    handle_menu_button(40084);
                                }
                                if ui.button("Cockpit Controls...").clicked() {
                                    handle_menu_button(40011);
                                }
                            })
                            .inner
                            .is_some()
                        {
                            menu_open = true;
                        }
                        if ui
                            .menu_button("Help", |ui| {
                                set_font_size(ui);
                                if ui.button("The Keshik").clicked() {
                                    handle_menu_button(40082);
                                }
                                ui.separator();
                                if ui.button("About ReMech 2").clicked() {
                                    self.about_dialog_open = true;
                                }
                            })
                            .inner
                            .is_some()
                        {
                            menu_open = true;
                        }
                        if cfg!(debug_assertions)
                            && ui
                                .menu_button("Debug", |ui| {
                                    set_font_size(ui);
                                    screens::debug::menu(ui);
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
            match confirm::dialog(ctx, &["Embrace Cowardice?"], scale_factor) {
                Some(true) => exit(0),
                Some(false) => self.exit_dialog_open = false,
                None => {}
            }
        }

        about::window(ctx, &mut self.about_dialog_open, scale_factor);
        confirm::window(ctx, scale_factor);
        dialog::replay_transition(hwnd);

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
                    ui.label(format!("WINDOW SIZE: {}x{}", window_size.0, window_size.1));
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
            let mut cursor_info = CURSORINFO {
                cbSize: size_of::<CURSORINFO>() as u32,
                ..Default::default()
            };
            let showing = unsafe { GetCursorInfo(&mut cursor_info) }.is_ok()
                && cursor_info.flags.0 & CURSOR_SHOWING.0 != 0;
            if showing {
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

        if !menu_locked {
            if (mouse_state.pos_y as f32) < 30.0 * scale_factor {
                self.menu_visible = true;
            } else if self.shell_hovered && !menu_open {
                self.menu_visible = false;
            }
        }

        if self.shell_hovered && !confirm::is_open() {
            update_global_mouse_state(mouse_state);
        } else {
            update_global_mouse_state(&OverlayMouseState::default());
        }
    }

    pub fn show_cursor(&mut self, show_cursor: bool) {
        self.show_cursor = show_cursor;
    }

    pub fn update_mouse_state(&self) {
        if self.shell_hovered && !confirm::is_open() {
            let mouse_state = unsafe { get_mouse_state() };
            update_global_mouse_state(&mouse_state);
        } else {
            update_global_mouse_state(&OverlayMouseState::default());
        }
    }
}
