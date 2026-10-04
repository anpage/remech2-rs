use std::{
    process::exit,
    sync::{
        Arc,
        atomic::{AtomicBool, Ordering},
    },
};

use egui::{
    Color32, ColorImage, Context, CursorIcon, FontFamily, Order, PointerButton, Rect, TextStyle,
    TextureHandle, Vec2,
};
use tracing::error;
use windows::Win32::{
    Foundation::{LPARAM, WPARAM},
    UI::WindowsAndMessaging::{PostMessageA, WM_COMMAND},
};

use crate::drawmode::fit_to_window;
use crate::shell::dialog;
use crate::shell::drawmode::{
    confirm, menu,
    mouse::{G_CURSOR_GRAPHIC, G_WINDOW, OverlayMouseState, update_global_mouse_state},
};
use crate::{about, shell::screens};

static SHOW_CURSOR: AtomicBool = AtomicBool::new(true);

pub(super) fn show_cursor(show: bool) {
    SHOW_CURSOR.store(show, Ordering::Relaxed);
}

pub struct OverlayUi {
    shell_hovered: bool,
    menu_visible: bool,
    cursor_texture: Option<TextureHandle>,
    exit_dialog_open: bool,
    about_dialog_open: bool,
}

impl OverlayUi {
    pub fn new(ctx: &Context) -> Self {
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

        ctx.set_fonts(fonts);

        Self {
            shell_hovered: false,
            menu_visible: false,
            cursor_texture: None,
            exit_dialog_open: false,
            about_dialog_open: false,
        }
    }

    fn load_cursor_texture(&mut self, ctx: &Context) {
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
        self.cursor_texture = Some(ctx.load_texture("cursor", cursor_image, Default::default()));
    }

    pub fn ui(&mut self, ctx: &Context) {
        if self.cursor_texture.is_none() {
            self.load_cursor_texture(ctx);
        }

        let hwnd = unsafe { G_WINDOW.get() };
        let window_rect = ctx.content_rect();
        let window_size = (window_rect.width(), window_rect.height());
        let mouse_state = &ctx.input(|input| {
            let pos = input.pointer.latest_pos().unwrap_or(egui::pos2(-1.0, -1.0));
            OverlayMouseState {
                pos_x: pos.x,
                pos_y: pos.y,
                left_down: input.pointer.button_down(PointerButton::Primary),
                right_down: input.pointer.button_down(PointerButton::Secondary),
                middle_down: input.pointer.button_down(PointerButton::Middle),
            }
        });

        let size = fit_to_window(window_size.0, window_size.1, const { 4.0 / 3.0 });
        let scale_factor = size.x / 640.0;

        let mut menu_open = false;

        let menu_locked = menu::locked();
        if menu_locked {
            self.menu_visible = false;
        }

        let shell_rect = Rect::from_center_size(window_rect.center(), size);
        self.shell_hovered = shell_rect.contains(egui::pos2(mouse_state.pos_x, mouse_state.pos_y))
            && !ctx.is_pointer_over_area();

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
                .fixed_pos(egui::pos2(window_size.0 / 2. - size.x / 2., 0.0))
                .fixed_size(Vec2::new(size.x, 30.0))
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

        if let Some(cursor_texture) = &self.cursor_texture {
            ctx.set_cursor_icon(CursorIcon::None);

            if SHOW_CURSOR.load(Ordering::Relaxed) {
                let cursor_pos_1 = egui::pos2(mouse_state.pos_x, mouse_state.pos_y);
                let cursor_pos_2 = egui::pos2(
                    mouse_state.pos_x + 29.0 * scale_factor,
                    mouse_state.pos_y + 25.0 * scale_factor,
                );
                let cursor_image = egui::Shape::image(
                    cursor_texture.id(),
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
            if mouse_state.pos_y < 30.0 * scale_factor {
                self.menu_visible = true;
            } else if self.shell_hovered && !menu_open {
                self.menu_visible = false;
            }
        }

        let window_size = [window_size.0, window_size.1];
        if self.shell_hovered && !confirm::is_open() {
            update_global_mouse_state(mouse_state, window_size);
        } else {
            update_global_mouse_state(&OverlayMouseState::default(), window_size);
        }
    }
}
