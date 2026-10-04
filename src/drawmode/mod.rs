pub mod scaler;

pub use scaler::ScalingMode;

use egui::Vec2;

/// The largest rect with the given aspect ratio that fits in the window
pub fn fit_to_window(window_width: f32, window_height: f32, aspect_ratio: f32) -> Vec2 {
    if window_width / window_height > aspect_ratio {
        Vec2::new(window_height * aspect_ratio, window_height)
    } else {
        Vec2::new(window_width, window_width / aspect_ratio)
    }
}
