use std::sync::{Arc, atomic::Ordering};

use egui::Vec2b;
use egui::{Context, Response, Ui};

use egui_plot::{Bar, BarChart, Legend, Plot};

use crate::sim::{
    G_DELTA_TIME, PROXIMITY_FUSES_SUPPRESSED, ZERO_DIVISORS_SUPPRESSED, ZERO_LENGTH_FRAMES_SKIPPED,
};

#[cfg(feature = "debug-overlay")]
pub fn show_deltatime_plot(ui: &mut Ui, recent_deltatimes: [Option<i32>; 200]) -> Response {
    let chart = BarChart::new(
        "",
        recent_deltatimes
            .iter()
            .enumerate()
            .map(|(i, &dt)| Bar::new(i as f64, dt.unwrap_or(0) as f64))
            .collect(),
    );

    Plot::new("Delta Time Histogram")
        .legend(Legend::default())
        .show_axes(Vec2b::new(false, true))
        .clamp_grid(true)
        .show(ui, |plot_ui| plot_ui.bar_chart(chart))
        .response
}

pub struct DebugOverlay {
    fonts: egui::FontDefinitions,
    recent_deltatimes: [Option<i32>; 200],
}

impl Default for DebugOverlay {
    fn default() -> Self {
        // Load the Science Gothic font
        let font = egui::FontData::from_static(include_bytes!("../../../ScienceGothic-Reg.ttf"));
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
            fonts,
            recent_deltatimes: [None; 200],
        }
    }
}

impl DebugOverlay {
    pub fn draw(&mut self, ctx: &Context, window_width: f32, window_height: f32) {
        ctx.set_fonts(self.fonts.clone());

        // calculate recent deltatimes
        let delta_time = unsafe { G_DELTA_TIME.get() };
        self.recent_deltatimes.rotate_left(1);
        self.recent_deltatimes[199] = Some(delta_time);

        egui::Window::new("DEBUG")
            .resizable(false)
            .collapsible(false)
            .default_pos(egui::pos2(10.0, 10.0))
            .show(ctx, |ui| {
                ui.label(format!("WINDOW SIZE: {}x{}", window_width, window_height));
                ui.label(format!("DELTATIME: {}", delta_time));
                ui.label(format!(
                    "DELTATIME < 4: {}",
                    self.recent_deltatimes.iter().fold(0, |acc, x| {
                        if let Some(x) = x
                            && *x < 4
                        {
                            acc + 1
                        } else {
                            acc
                        }
                    })
                ));
                ui.label(format!(
                    "DELTATIME == 0: {}",
                    self.recent_deltatimes.iter().fold(0, |acc, x| {
                        if let Some(x) = x
                            && *x == 0
                        {
                            acc + 1
                        } else {
                            acc
                        }
                    })
                ));
                ui.label(format!(
                    "ZERO DIVISORS: {}",
                    ZERO_DIVISORS_SUPPRESSED.load(Ordering::Relaxed)
                ));
                ui.label(format!(
                    "FUSES SUPPRESSED: {}",
                    PROXIMITY_FUSES_SUPPRESSED.load(Ordering::Relaxed)
                ));
                ui.label(format!(
                    "SHOT UPDATES SKIPPED: {}",
                    ZERO_LENGTH_FRAMES_SKIPPED.load(Ordering::Relaxed)
                ));
                show_deltatime_plot(ui, self.recent_deltatimes);
            });
    }
}
