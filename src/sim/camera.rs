use std::sync::atomic::{AtomicI32, Ordering};

use remech2_sys::sim::{self, GetViewMode, PlaySoundEffect};

use super::window::widescreen;

static RAW_FOV: AtomicI32 = AtomicI32::new(0x10000);

pub fn reset() {
    RAW_FOV.store(0x10000, Ordering::Relaxed);
}

fn hor_plus(fov: i32) -> i32 {
    let (width, height) = unsafe { (sim::g_gameWindowWidth, sim::g_gameWindowHeight) };
    if width <= 0 {
        return fov;
    }
    (i64::from(fov) * 4 * i64::from(height) / (3 * i64::from(width))) as i32
}

#[unsafe(export_name = "ApplyCameraFov")]
pub unsafe extern "C" fn apply_camera_fov(reset: i32) {
    unsafe {
        let Some(eyepoint) = sim::g_eyepoint.as_mut() else {
            return;
        };
        let previous = if widescreen() {
            RAW_FOV.load(Ordering::Relaxed)
        } else {
            eyepoint.m_fovX
        };
        if reset != 0 {
            sim::g_normalFov = 0x10000;
            sim::g_zoomFov = 0x10000;
        }
        let fov = if GetViewMode() == 0 {
            sim::g_normalFov
        } else {
            sim::g_zoomFov
        };
        RAW_FOV.store(fov, Ordering::Relaxed);
        eyepoint.m_fovX = hor_plus(fov);
        if fov != previous {
            sim::g_projectionDirty = 1;
            PlaySoundEffect(0x147, 100, 0x40, 5, 0x50);
        }
    }
}
