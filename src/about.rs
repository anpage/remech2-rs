use egui::{Context, FontFamily, FontId, RichText, ScrollArea};

/// The version for a tagged release, otherwise a description of the commit.
/// Set by `build.rs`.
const VERSION: &str = env!("REMECH2_VERSION");

/// Everything whose license has to ship with the binary, in display order.
const LICENSES: &[(&str, &str)] = &[
    ("REMECH 2", include_str!("../LICENSE.md")),
    ("GENERALUSER GS", include_str!("../LICENSE-GUGS.txt")),
    (
        "SCIENCE GOTHIC",
        include_str!("../LICENSE-ScienceGothic.txt"),
    ),
];

pub fn window(ctx: &Context, open: &mut bool, scale_factor: f32) {
    egui::Window::new("ABOUT")
        .open(open)
        .resizable(false)
        .collapsible(false)
        .pivot(egui::Align2::CENTER_CENTER)
        .default_pos(ctx.content_rect().center())
        .show(ctx, |ui| {
            ui.set_width(240.0 * scale_factor);

            ui.vertical_centered(|ui| {
                ui.label(RichText::new("ReMech 2").size(16.0 * scale_factor).strong());
                ui.label(RichText::new(VERSION).size(6.0 * scale_factor));
                ui.add_space(8.0 * scale_factor);
                ui.label(
                    RichText::new(
                        "An unofficial open-source project.\n\
                        In no way associated with or endorsed by Activision Blizzard, Inc.",
                    )
                    .size(8.0 * scale_factor),
                );
            });

            ui.add_space(8.0 * scale_factor);

            ScrollArea::vertical()
                .max_height(1000.0 * scale_factor)
                .auto_shrink([false, false])
                .show(ui, |ui| {
                    for (index, (name, text)) in LICENSES.iter().enumerate() {
                        if index > 0 {
                            ui.add_space(12.0 * scale_factor);
                        }
                        ui.label(RichText::new(*name).size(6.0 * scale_factor).strong());
                        ui.separator();
                        ui.label(
                            RichText::new(*text)
                                .font(FontId::new(5.0 * scale_factor, FontFamily::Monospace)),
                        );
                    }
                });
        });
}
