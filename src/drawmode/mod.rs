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

pub fn frame_rect(window_size: [f32; 2], frame_size: [usize; 2]) -> Option<([f32; 2], [f32; 2])> {
    if frame_size[0] == 0 || frame_size[1] == 0 {
        return None;
    }
    let aspect_ratio = frame_size[0] as f32 / frame_size[1] as f32;
    let size = fit_to_window(window_size[0], window_size[1], aspect_ratio).round();
    if size.x < 1.0 || size.y < 1.0 {
        return None;
    }
    let origin = [
        ((window_size[0] - size.x) / 2.0).floor(),
        ((window_size[1] - size.y) / 2.0).floor(),
    ];
    Some((origin, size.into()))
}
