use std::ffi::{CStr, c_char};

use tracing::{debug, error};

fn text(text: *const c_char) -> Option<String> {
    if text.is_null() {
        return None;
    }
    let text = unsafe { CStr::from_ptr(text) }.to_string_lossy();
    Some(text.trim_end().to_owned())
}

#[unsafe(export_name = "MechLogError")]
pub unsafe extern "C" fn log_error(message: *const c_char) {
    if let Some(message) = text(message) {
        error!("{message}");
    }
}

#[unsafe(export_name = "MechLogDebug")]
pub unsafe extern "C" fn log_debug(message: *const c_char) {
    if let Some(message) = text(message) {
        debug!("{message}");
    }
}
