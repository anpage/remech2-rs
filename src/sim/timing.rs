use std::{ffi::c_void, sync::RwLock, time::Instant};

use binding::macros::{game_fns, globals, hook, patches};
use windows::Win32::Foundation::TRUE;

use crate::{
    settings::SETTINGS,
    sim::{
        G_CURRENT_DRAW_MODE, RenderTarget,
        drawmode::hooks::{G_CURRENT_DRAW_MODE_EXTENSION, PixelBuffer},
        window::G_WINDOW_ACTIVE,
    },
};

use super::MODULE;

/// Ticks per frame at the ideal 45 FPS, which is what every sim system seems to be tuned for.
pub(super) const TICKS_PER_IDEAL_FRAME: i32 = 4;

game_fns!(
    /// Fills the target's rect with a palette index.
    static FILL_RENDER_TARGET_RECT: unsafe extern "cdecl" fn(*mut RenderTarget, u8) -> i32 =
        0x000630b9;
    /// Draws one frame of an RLE shape into the target at (x, y).
    static DRAW_SHAPE_FRAME: unsafe extern "cdecl" fn(
        *mut RenderTarget,
        *mut c_void,
        i32,
        i32,
        i32,
    ) -> i32 = 0x00061228;
);

globals!(
    /// Set once InitDisplayGeometry has run, cleared when the display is torn down.
    static G_DISPLAY_READY: i32 = 0x000a2464;
    /// The loading screen's backdrop, loaded by StartSupAnim.
    static G_SUP_ANIM_BACKDROP: *mut c_void = 0x000a0154;
    /// The dropship's animation frames.
    static G_SUP_ANIM_SHAPE: *mut c_void = 0x000a0158;
    static G_SUP_ANIM_FRAME_COUNT: i32 = 0x000a015c;
    static G_SUP_ANIM_FRAME: i32 = 0x000a0198;
    static G_SUP_ANIM_X: i32 = 0x000bcd54;
    static G_SUP_ANIM_Y: i32 = 0x000bcd50;
    /// The whole framebuffer, which the animation draws into.
    static G_SUP_ANIM_TARGET: RenderTarget = 0x000bcd58;
    /// StartSupAnim's copy of the main buffer, with its size fields held as
    /// maxima rather than extents, the way the shape drawing code wants them.
    static G_SUP_ANIM_PIXEL_BUFFER: PixelBuffer = 0x000bcd70;
    static G_MAIN_PIXEL_BUFFER: PixelBuffer = 0x00176ef0;
    static G_TICKS_CHECK: u32 = 0x000ad008;
    static G_TICKS_1: u32 = 0x000ad20c;
    static G_TICKS_2: u32 = 0x000ad210;
    pub(super) static G_DELTA_TIME: i32 = 0x000ba550;
);

/// AIL's 181Hz tick, which the game uses as its clock.
#[hook(rva = 0x00067ed8)]
unsafe extern "stdcall" fn game_tick_timer_callback(_user: u32) {
    unsafe {
        if G_TICKS_CHECK.get() & 0x200 == 0 {
            G_TICKS_1.set(G_TICKS_1.get() + 1);
        }
        if G_TICKS_CHECK.get() & 0x100 == 0 {
            G_TICKS_2.set(G_TICKS_2.get() + 1);
        }
    }
}

/// The dropship loading screen ("sup anim"), driven by an AIL timer at 330ms.
#[hook(rva = 0x00003f3d)]
unsafe extern "stdcall" fn sup_anim_timer_callback(_user: u32) {
    unsafe {
        if G_DISPLAY_READY.get() == 0
            || G_SUP_ANIM_BACKDROP.get().is_null()
            || G_SUP_ANIM_SHAPE.get().is_null()
            || G_SUP_ANIM_FRAME_COUNT.get() <= 1
        {
            return;
        }

        // Nothing to draw into unless the window is up and the buffer locks.
        if G_WINDOW_ACTIVE.get() != TRUE {
            return;
        }
        let extension = G_CURRENT_DRAW_MODE_EXTENSION.get();
        if ((*extension).lock_display_buffer_func)() != 0 {
            return;
        }

        // The main buffer flips, so the copy's data pointer is a tick stale.
        (*G_SUP_ANIM_PIXEL_BUFFER.ptr()).data = (*G_MAIN_PIXEL_BUFFER.ptr()).data;

        let target = G_SUP_ANIM_TARGET.ptr();
        (FILL_RENDER_TARGET_RECT.get())(target, 0);
        (DRAW_SHAPE_FRAME.get())(target, G_SUP_ANIM_BACKDROP.get(), 0, 0, 0);
        (DRAW_SHAPE_FRAME.get())(
            target,
            G_SUP_ANIM_SHAPE.get(),
            G_SUP_ANIM_FRAME.get(),
            G_SUP_ANIM_X.get(),
            G_SUP_ANIM_Y.get(),
        );

        // The game re-reads this here rather than reusing the check above, and
        // it can have changed: this runs on the timer thread.
        if G_WINDOW_ACTIVE.get() == TRUE {
            ((*G_CURRENT_DRAW_MODE.get()).blit_flip_func)();
        }

        G_SUP_ANIM_FRAME.set((G_SUP_ANIM_FRAME.get() + 1) % G_SUP_ANIM_FRAME_COUNT.get());
    }
}

/// We hook this in order to limit the framerate to the configured value.
#[hook(rva = 0x0007ce2c)]
unsafe extern "stdcall" fn next_clock() {
    let framerate_limit = SETTINGS.get_int("video", "framerate_limit", 45);

    if framerate_limit > 0 {
        static LAST_INSTANT: RwLock<Option<Instant>> = RwLock::new(None);
        let frame_time = 1.0 / framerate_limit as f64;
        let mut last_instant = LAST_INSTANT.write().unwrap();
        let last = *last_instant.get_or_insert_with(Instant::now);
        while last.elapsed().as_secs_f64() < frame_time {
            std::thread::yield_now();
        }
        *last_instant = Some(Instant::now());
    }

    unsafe {
        original();
    }
}

patches!(
    pub(super) static PATCHES = [
        hook game_tick_timer_callback,
        hook next_clock,
        hook sup_anim_timer_callback,
    ];
);
