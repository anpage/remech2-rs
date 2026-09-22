use std::ffi::c_void;

use binding::{globals, macros::hook, patches};

use crate::sim::{
    RenderTarget,
    window::{G_GAME_WINDOW_GEOMETRY, G_GAME_WINDOW_HEIGHT, G_GAME_WINDOW_WIDTH},
};

use super::MODULE;

/// The camera, which the game calls "Eyepoint"
#[repr(C)]
pub(super) struct Eyepoint {
    position: [i32; 3],
    rotation: [i32; 3],
    /// Horizontal FOV in 16.16, where `tan(fov / 2) == 1 / fov_x`
    pub fov_x: i32,
    unknown1: [i32; 4],
    viewport_left: i32,
    viewport_right: i32,
    viewport_top: i32,
    viewport_bottom: i32,
    hither_clip_plane: i32,
    yon_clip_plane: i32,
    /// `pixel width / pixel height` in 16.16
    pub pixel_aspect_ratio: i32,
}

/// The cockpit layout table passed to LoadCockpitLayout. It runs to at least
/// index 0x22; only the entries the detour needs are named.
#[repr(C)]
pub(super) struct CockpitLayout {
    /// The cockpit's 3D viewport, in normalised 16.16 coordinates
    viewport: *mut RenderTarget,
    unknown1: *mut c_void,
    /// Render target table slot the viewport is copied into
    pub render_target_slot: i32,
}

globals!(
    // static G_PIXEL_ASPECT_RATIO: i32 = 0x000e9610;
    static G_EYEPOINT: *mut Eyepoint = 0x000a6cc0;
    static G_RENDER_TARGET_TABLE: [RenderTarget; 11] = 0x00181a60;
);

const SATELLITE_LAYOUT: i32 = 4;

/// Rewrites eyepoint->fovX every frame from the game's FOV globals.
/// We keep the game's raw value in a shadow so its internal logic always sees the original raw value,
/// then we always apply the Hor+ correction.
#[hook(rva = 0x00011455)]
unsafe extern "cdecl" fn apply_eyepoint_fov(reset: i32) {
    static mut LAST_RAW_FOV: i32 = 0x10000;
    unsafe {
        let cam = G_EYEPOINT.get();
        if cam.is_null() {
            original(reset);
            return;
        }
        let correction = {
            let width = G_GAME_WINDOW_WIDTH.get() as i64;
            let height = G_GAME_WINDOW_HEIGHT.get() as i64;
            if width <= 0 || height <= 0 {
                0x10000
            } else {
                let aspect = (width << 16) / height;
                (((0x15555i64) << 16) / aspect) as i32
            }
        };
        (*cam).fov_x = LAST_RAW_FOV;
        original(reset);
        LAST_RAW_FOV = (*cam).fov_x;
        (*cam).fov_x = ((LAST_RAW_FOV as i64 * correction as i64) >> 16) as i32;
    }
}

/// Recomputes the projection from the eyepoint struct.
/// We force pixel_aspect_ratio to be square each time.
#[hook(rva = 0x0004bc2e)]
unsafe extern "cdecl" fn setup_eyepoint_projection(cam: *mut Eyepoint) {
    unsafe {
        if !cam.is_null() {
            (*cam).pixel_aspect_ratio = 0x10000;
        }
        original(cam);
    }
}

/// Slot `slot` of the game's render target table, if that slot exists
fn render_target(slot: i32) -> Option<&'static mut RenderTarget> {
    let slot = usize::try_from(slot).ok()?;
    unsafe { G_RENDER_TARGET_TABLE.as_mut()?.get_mut(slot) }
}

/// The satellite view's 3D scene slot
static mut SCENE_SLOT: i32 = -1;

/// Copies the cockpit's 3D viewport rect into render-target slot `layout.render_target_slot`.
/// We widen that slot to the full framebuffer so the 3D view covers the whole width behind the centered 4:3 HUD.
#[hook(rva = 0x0003dab0)]
unsafe extern "cdecl" fn load_cockpit_layout(cockpit: i32, layout: *mut CockpitLayout) {
    unsafe {
        original(cockpit, layout);
        if layout.is_null() {
            return;
        }
        let slot = (*layout).render_target_slot;
        if let Some(target) = render_target(slot) {
            target.cover_framebuffer();
        }
        if cockpit == SATELLITE_LAYOUT {
            SCENE_SLOT = slot;
            if let Some(viewport) = (*layout).viewport.as_mut() {
                viewport.cover_framebuffer();
            }
        }
    }
}

/// Copies slot `slot` of the render-target table into StretchBlitSourceRect and the eyepoint.
/// We substitute the full-screen rect for scene slots only, just before they are consumed
#[hook(rva = 0x0000242f)]
unsafe extern "cdecl" fn select_render_target(slot: i32) {
    unsafe {
        if (slot == 0 || slot == SCENE_SLOT)
            && let Some(target) = render_target(slot)
        {
            target.cover_framebuffer();
        }
        original(slot);
    }
}

/// Sets up the map projection for the satellite view.
/// We adjust the world span to match the game window's width, so the map covers the full width.
#[hook(rva = 0x00041fa0)]
unsafe extern "cdecl" fn setup_map_projection(
    eyepoint: *mut c_void,
    slot: i32,
    world_span: i32,
    far_plane: i32,
) {
    unsafe {
        let mut world_span = world_span;
        if slot == SCENE_SLOT
            && let Some(geometry) = G_GAME_WINDOW_GEOMETRY.get().as_ref()
            && geometry.width > 0
        {
            let width = G_GAME_WINDOW_WIDTH.get() as i64;
            world_span = (world_span as i64 * width / geometry.width as i64) as i32;
        }
        original(eyepoint, slot, world_span, far_plane);
    }
}

/// Draws the satellite view's readouts.
/// We widened the satellite view's HUD box to cover the full width,
/// so here we narrow it again for the text in the top left.
#[hook(rva = 0x0003ec35)]
unsafe extern "cdecl" fn draw_map_view_text(layout: *mut CockpitLayout) {
    unsafe {
        if layout.is_null() || (*layout).render_target_slot != SCENE_SLOT {
            original(layout);
            return;
        }
        let viewport = (*layout).viewport;
        if viewport.is_null() {
            original(layout);
            return;
        }

        let scene_rect = (*viewport).clone();
        (*viewport).cover_hud_box();
        original(layout);
        *viewport = scene_rect;
    }
}

patches!(
    pub(super) static PATCHES = [
        hook apply_eyepoint_fov,
        hook setup_eyepoint_projection,
        hook load_cockpit_layout,
        hook select_render_target,
        hook setup_map_projection,
        hook draw_map_view_text,
    ];
);
