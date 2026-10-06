use std::{cell::RefCell, ffi::c_int, ptr, slice};

use crate::{
    app::{self, Frame},
    drawmode::scaler::PaletteData,
    shell, sim,
};

struct Display {
    pixels: Box<[u8]>,
    size: [usize; 2],
}

pub enum Overlay {
    Shell(Box<shell::OverlayUi>),
    Sim(Box<sim::OverlayUi>),
}

thread_local! {
    static OVERLAY: RefCell<Option<Overlay>> = const { RefCell::new(None) };
    static DISPLAY: RefCell<Option<Display>> = const { RefCell::new(None) };
    static PALETTE: RefCell<PaletteData> = const { RefCell::new([[0.0, 0.0, 0.0, 1.0]; 256]) };
}

pub fn set_overlay(overlay: Option<Overlay>) {
    app::capture_pointer(false);
    OVERLAY.set(overlay);
}

fn present(source: Option<[usize; 4]>) {
    DISPLAY.with_borrow(|display| {
        let Some(display) = display else {
            return;
        };
        PALETTE.with_borrow(|palette| {
            let frame = Frame {
                pixels: &display.pixels,
                size: display.size,
                source,
                palette,
            };
            app::with(|app| {
                app.present(Some(frame), |ctx| {
                    OVERLAY.with_borrow_mut(|overlay| match overlay {
                        Some(Overlay::Shell(overlay)) => overlay.ui(ctx),
                        Some(Overlay::Sim(overlay)) => overlay.ui(ctx),
                        None => {}
                    });
                })
            });
        });
    });
}

#[unsafe(export_name = "MechDisplayBegin")]
pub extern "C" fn begin(width: c_int, height: c_int) -> *mut u8 {
    let (Ok(width), Ok(height)) = (usize::try_from(width), usize::try_from(height)) else {
        return ptr::null_mut();
    };
    let Some(len) = width.checked_mul(height) else {
        return ptr::null_mut();
    };
    let mut pixels = Vec::new();
    if pixels.try_reserve_exact(len).is_err() {
        return ptr::null_mut();
    }
    pixels.resize(len, 0);

    let mut pixels = pixels.into_boxed_slice();
    let frame = pixels.as_mut_ptr();
    DISPLAY.set(Some(Display {
        pixels,
        size: [width, height],
    }));
    frame
}

#[unsafe(export_name = "MechDisplayEnd")]
pub extern "C" fn end() {
    DISPLAY.set(None);
}

#[unsafe(export_name = "MechDisplaySetPalette")]
pub unsafe extern "C" fn set_palette(palette: *const u8) {
    if palette.is_null() {
        return;
    }
    let colors = unsafe { slice::from_raw_parts(palette.cast::<[u8; 3]>(), 256) };

    let scale = |v: u8| {
        let v = v.min(63);
        ((v << 2) | (v >> 4)) as f32 / 255.0
    };
    PALETTE.with_borrow_mut(|palette| {
        for (color, &[red, green, blue]) in palette.iter_mut().zip(colors) {
            *color = [scale(red), scale(green), scale(blue), 1.0];
        }
    });
}

#[unsafe(export_name = "MechDisplayPresent")]
pub extern "C" fn present_frame() {
    present(None);
}

#[unsafe(export_name = "MechDisplayPresentRect")]
pub extern "C" fn present_rect(left: c_int, top: c_int, right: c_int, bottom: c_int) {
    let Some([width, height]) = DISPLAY.with_borrow(|display| display.as_ref().map(|d| d.size))
    else {
        return;
    };

    let left = (left.max(0) as usize).min(width);
    let top = (top.max(0) as usize).min(height);
    let right = ((right + 1).max(0) as usize).clamp(left, width);
    let bottom = ((bottom + 1).max(0) as usize).clamp(top, height);
    if right == left || bottom == top {
        return;
    }

    present(Some([left, top, right - left, bottom - top]));
}
