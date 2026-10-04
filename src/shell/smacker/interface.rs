use std::ffi::{CStr, c_char, c_void};

use tracing::warn;

use super::movie::Movie;

/// Reborrow an open handle the game passed back to us
unsafe fn movie<'a>(smk: *mut c_void) -> Option<&'a mut Movie> {
    if smk.is_null() {
        return None;
    }

    Some(unsafe { &mut *smk.cast::<Movie>() })
}

/// Opens a `.smk` file or returns null.
/// We read the entire file, so we ignore `extrabuf`
#[unsafe(export_name = "SmackOpen")]
pub unsafe extern "C" fn open(name: *const c_char, flags: u32, _extrabuf: u32) -> *mut c_void {
    if name.is_null() {
        return std::ptr::null_mut();
    }

    let Ok(path) = (unsafe { CStr::from_ptr(name) }).to_str() else {
        warn!("smacker: non-UTF-8 movie path");
        return std::ptr::null_mut();
    };

    match Movie::open(path, flags) {
        Ok(movie) => Box::into_raw(movie).cast::<c_void>(),
        Err(e) => {
            warn!("smacker: {e:#}");
            std::ptr::null_mut()
        }
    }
}

/// Closes a handle and frees everything it owned
#[unsafe(export_name = "SmackClose")]
pub unsafe extern "C" fn close(smk: *mut c_void) {
    if smk.is_null() {
        return;
    }

    drop(unsafe { Box::from_raw(smk.cast::<Movie>()) });
}

/// Decodes the current frame into the registered destination
#[unsafe(export_name = "SmackDoFrame")]
pub unsafe extern "C" fn do_frame(smk: *mut c_void) -> u32 {
    let Some(movie) = (unsafe { movie(smk) }) else {
        return 0;
    };

    if let Err(e) = movie.do_frame() {
        warn!("smacker: {e:#}");
    }

    0
}

/// Advances to the next frame and applies its palette record
#[unsafe(export_name = "SmackNextFrame")]
pub unsafe extern "C" fn next_frame(smk: *mut c_void) {
    let Some(movie) = (unsafe { movie(smk) }) else {
        return;
    };

    if let Err(e) = movie.next_frame() {
        warn!("smacker: {e:#}");
    }
}

/// Makes frame `frame - 1` current.
/// 0 re-primes the current one
#[unsafe(export_name = "SmackGoto")]
pub unsafe extern "C" fn goto(smk: *mut c_void, frame: u32) {
    let Some(movie) = (unsafe { movie(smk) }) else {
        return;
    };

    if let Err(e) = movie.goto(frame) {
        warn!("smacker: {e:#}");
    }
}

/// Non-zero until the current frame's display time has elapsed
#[unsafe(export_name = "SmackWait")]
pub unsafe extern "C" fn wait(smk: *mut c_void) -> u32 {
    let Some(movie) = (unsafe { movie(smk) }) else {
        return 0;
    };

    movie.wait()
}

/// Registers the destination for decoded frames
#[unsafe(export_name = "SmackToBuffer")]
pub unsafe extern "C" fn to_buffer(
    smk: *mut c_void,
    left: u32,
    top: u32,
    pitch: u32,
    dest_height: u32,
    buf: *mut c_void,
    flags: u32,
) {
    let Some(movie) = (unsafe { movie(smk) }) else {
        return;
    };

    movie.set_destination(buf, left, top, pitch, dest_height, flags != 0);
}

/// Returns 1 when any track selected by `trackflags` exists
#[unsafe(export_name = "SmackSoundInTrack")]
pub unsafe extern "C" fn sound_in_track(smk: *mut c_void, trackflags: u32) -> u32 {
    let Some(movie) = (unsafe { movie(smk) }) else {
        return 0;
    };

    u32::from(movie.sound_in_track(trackflags))
}

/// Decodes the current frame's audio into `dest`, returning the byte count
#[unsafe(export_name = "SmackGetTrackData")]
pub unsafe extern "C" fn get_track_data(
    smk: *mut c_void,
    dest: *mut c_void,
    trackflags: u32,
) -> u32 {
    let Some(movie) = (unsafe { movie(smk) }) else {
        return 0;
    };

    match unsafe { movie.get_track_data(dest, trackflags) } {
        Ok(n) => n,
        Err(e) => {
            warn!("smacker: {e:#}");
            0
        }
    }
}
