use std::mem::size_of;

use remech2_sys::{
    shared::{MechHeapAllocZeroed, PANE, WINDOW},
    sim::{self, CockpitLayout, FixedMul16, GameWindowGeometry, Point, Rect, g_artResolutionSizes},
};

use crate::settings;

/// The satellite view's index in `g_cockpitLayouts`
const SATELLITE_LAYOUT: i32 = 4;

/// Sizes the game's frame for the render resolution.
/// When widescreen is enabled, the frame is 16:9 and the HUD stays in a 4:3 box centred within it.
#[unsafe(export_name = "SetGameResolution")]
pub extern "C" fn set_game_resolution() {
    let video = settings::get().video.clone();
    let (width, height) = (video.render_width, video.render_height);
    unsafe {
        sim::g_gameWindowWidth = width as i32;
        sim::g_gameWindowHeight = height as i32;
    }
}

#[unsafe(export_name = "InitGameWindowGeometry")]
pub unsafe extern "C" fn init_game_window_geometry() -> i32 {
    unsafe {
        let geometry = MechHeapAllocZeroed(sim::g_primaryHeap, size_of::<GameWindowGeometry>())
            .cast::<GameWindowGeometry>();
        sim::g_gameWindowGeometry = geometry;
        let Some(geometry) = geometry.as_mut() else {
            return 0;
        };

        let (width, height) = (sim::g_gameWindowWidth, sim::g_gameWindowHeight);
        geometry.m_width = (height * 4 / 3).min(width);
        geometry.m_height = height;
        geometry.m_unk0x08 = 1;
        geometry.m_numColors = 0x100;
        geometry.m_unk0x10 = 1;
        geometry.m_unk0x14 = 0;
        sim::g_screenPixelCount = height * width;
        sim::g_screenWidth = width;
        sim::g_screenHeight = height;
        sim::g_screenWidthMinus1 = geometry.m_width - 1;
        sim::g_screenHeightMinus1 = geometry.m_height - 1;
        sim::g_screenHalfWidth = width / 2;
        sim::g_screenHalfHeight = height / 2;
        crate::sim::camera::reset();
        1
    }
}

#[unsafe(export_name = "SetPixelAspect")]
pub unsafe extern "C" fn set_pixel_aspect(geometry: *mut GameWindowGeometry) {
    let Some(geometry) = (unsafe { geometry.as_ref() }) else {
        return;
    };
    let aspect = super::math::mul_div_64(geometry.m_height << 16, 0x15555, geometry.m_width << 16);
    unsafe {
        sim::g_pixelAspect = aspect;
        if let Some(eyepoint) = sim::g_eyepoint.as_mut() {
            eyepoint.m_pixelAspect = aspect;
        }
    }
}

/// Picks the art resolution closest to the window's size.
#[unsafe(export_name = "ChooseArtResolution")]
pub unsafe extern "C" fn choose_art_resolution(geometry: *mut GameWindowGeometry) {
    let Some(geometry) = (unsafe { geometry.as_ref() }) else {
        return;
    };
    let sizes = &raw const g_artResolutionSizes;
    let distance = |i: usize| {
        let (x, y) = unsafe { ((*sizes)[i].m_x, (*sizes)[i].m_y) };
        (x - (geometry.m_width - 1)).abs() + (y - (geometry.m_height - 1)).abs()
    };
    if let Some(closest) = (0..3).min_by_key(|&i| distance(i)) {
        unsafe { sim::g_artResolution = closest as i32 };
    }
}

fn hud_origin() -> (i32, i32) {
    unsafe {
        let Some(geometry) = sim::g_gameWindowGeometry.as_ref() else {
            return (0, 0);
        };
        (
            (sim::g_gameWindowWidth - geometry.m_width) / 2,
            (sim::g_gameWindowHeight - geometry.m_height) / 2,
        )
    }
}

pub fn widescreen() -> bool {
    hud_origin() != (0, 0)
}

fn scale_to_screen(x: i32, y: i32) -> (i32, i32) {
    let (x0, y0) = hud_origin();
    unsafe {
        (
            x0 + FixedMul16(sim::g_screenWidthMinus1, x),
            y0 + FixedMul16(sim::g_screenHeightMinus1, y),
        )
    }
}

#[unsafe(export_name = "ScaleRectToScreen")]
pub unsafe extern "C" fn scale_rect_to_screen(
    _buffer: *mut WINDOW,
    src: *mut PANE,
    dst: *mut PANE,
) -> *mut PANE {
    let (Some(src), Some(out)) = (unsafe { src.as_ref() }, unsafe { dst.as_mut() }) else {
        return dst;
    };
    let ((x0, y0), (x1, y1)) = (
        scale_to_screen(src.m_x0, src.m_y0),
        scale_to_screen(src.m_x1, src.m_y1),
    );
    (out.m_x0, out.m_y0, out.m_x1, out.m_y1) = (x0, y0, x1, y1);
    dst
}

#[unsafe(export_name = "ScaleBoundsToScreen")]
pub unsafe extern "C" fn scale_bounds_to_screen(
    _buffer: *mut WINDOW,
    src: *mut Rect,
    dst: *mut Rect,
) -> *mut Rect {
    let (Some(src), Some(out)) = (unsafe { src.as_ref() }, unsafe { dst.as_mut() }) else {
        return dst;
    };
    let ((left, top), (right, bottom)) = (
        scale_to_screen(src.m_left, src.m_top),
        scale_to_screen(src.m_right, src.m_bottom),
    );
    (out.m_left, out.m_top, out.m_right, out.m_bottom) = (left, top, right, bottom);
    dst
}

#[unsafe(export_name = "ScalePointToScreen")]
pub unsafe extern "C" fn scale_point_to_screen(
    _buffer: *mut WINDOW,
    src: *mut Point,
    dst: *mut Point,
) -> *mut Point {
    let (Some(src), Some(out)) = (unsafe { src.as_ref() }, unsafe { dst.as_mut() }) else {
        return dst;
    };
    (out.m_x, out.m_y) = scale_to_screen(src.m_x, src.m_y);
    dst
}

#[unsafe(export_name = "CenterRectOnScreen")]
pub unsafe extern "C" fn center_rect_on_screen(
    _buffer: *mut WINDOW,
    src: *mut PANE,
    dst: *mut PANE,
) -> *mut PANE {
    let (Some(src), Some(out)) = (unsafe { src.as_ref() }, unsafe { dst.as_mut() }) else {
        return dst;
    };
    let (width, height) = (src.m_x1 - src.m_x0 + 1, src.m_y1 - src.m_y0 + 1);
    let (x0, y0) = hud_origin();
    let (left, top) = unsafe {
        (
            x0 + (sim::g_screenWidthMinus1 - width - 1) / 2,
            y0 + (sim::g_screenHeightMinus1 - height - 1) / 2,
        )
    };
    (out.m_x0, out.m_y0) = (left, top);
    (out.m_x1, out.m_y1) = (left + width - 1, top + height - 1);
    dst
}

fn satellite_slot() -> Option<i32> {
    unsafe {
        let layouts = &raw const sim::g_cockpitLayouts;
        (*layouts)[SATELLITE_LAYOUT as usize]
            .as_ref()
            .map(|layout| layout.m_paneSlot)
    }
}

fn cover_frame(pane: &mut PANE) {
    unsafe {
        pane.m_x0 = 0;
        pane.m_y0 = 0;
        pane.m_x1 = sim::g_gameWindowWidth - 1;
        pane.m_y1 = sim::g_gameWindowHeight - 1;
    }
}

#[unsafe(export_name = "SelectPane")]
pub unsafe extern "C" fn select_pane(index: i32) {
    unsafe {
        let panes = &raw mut sim::g_panes;
        let Some(target) = usize::try_from(index)
            .ok()
            .and_then(|i| (*panes).get_mut(i))
        else {
            return;
        };
        if widescreen() && (index == 0 || Some(index) == satellite_slot()) {
            cover_frame(target);
        }
        if index == sim::g_paneIndex {
            return;
        }
        if let Some(eyepoint) = sim::g_eyepoint.as_mut() {
            eyepoint.m_viewLeft = 0;
            eyepoint.m_viewTop = 0;
            eyepoint.m_viewRight = target.m_x1 - target.m_x0;
            eyepoint.m_viewBottom = target.m_y1 - target.m_y0;
            eyepoint.m_offsetX = 0;
            eyepoint.m_offsetY = 0;
        }
        sim::g_currentPane = *target;
        sim::g_paneIndex = index;
        sim::g_projectionDirty = 1;
    }
}

#[unsafe(export_name = "MechWidenCockpitLayout")]
pub unsafe extern "C" fn widen_cockpit_layout(cockpit: i32, layout: *mut CockpitLayout) {
    if cockpit != SATELLITE_LAYOUT || !widescreen() {
        return;
    }
    let Some(layout) = (unsafe { layout.as_mut() }) else {
        return;
    };
    unsafe {
        if let Some(viewport) = layout.m_viewport.as_mut() {
            cover_frame(viewport);
        }
        let panes = &raw mut sim::g_panes;
        if let Some(pane) = usize::try_from(layout.m_paneSlot)
            .ok()
            .and_then(|i| (*panes).get_mut(i))
        {
            cover_frame(pane);
        }
    }
}

#[unsafe(export_name = "MechWidenMapSpan")]
pub unsafe extern "C" fn widen_map_span(slot: i32, span: i32) -> i32 {
    if !widescreen() || Some(slot) != satellite_slot() {
        return span;
    }
    unsafe {
        match sim::g_gameWindowGeometry.as_ref() {
            Some(geometry) if geometry.m_width > 0 => {
                (i64::from(span) * i64::from(sim::g_gameWindowWidth) / i64::from(geometry.m_width))
                    as i32
            }
            _ => span,
        }
    }
}

#[unsafe(export_name = "MechHudPane")]
pub unsafe extern "C" fn hud_pane(pane: *mut PANE) {
    let Some(pane) = (unsafe { pane.as_mut() }) else {
        return;
    };
    let (x0, y0) = hud_origin();
    unsafe {
        let Some(geometry) = sim::g_gameWindowGeometry.as_ref() else {
            return;
        };
        pane.m_x0 = x0;
        pane.m_y0 = y0;
        pane.m_x1 = x0 + geometry.m_width - 1;
        pane.m_y1 = y0 + geometry.m_height - 1;
    }
}
