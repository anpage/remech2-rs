use std::ffi::{CStr, VaList, c_char};

use printf_compat::{
    argument::{Argument, Specifier},
    output,
};
use tracing::{debug, error, trace};

fn text(text: *const c_char) -> Option<String> {
    if text.is_null() {
        return None;
    }
    let text = unsafe { CStr::from_ptr(text) }.to_string_lossy();
    Some(text.trim_end().to_owned())
}

fn formatted_text(format: *const c_char, args: VaList<'_>) -> Option<String> {
    if format.is_null() {
        return None;
    }

    let mut message = String::new();
    let mut write = output::fmt_write(&mut message);
    let written = unsafe {
        printf_compat::format(format, args, move |argument: Argument<'_>| {
            let bytes = match argument.specifier {
                Specifier::String(text) => text.to_bytes(),
                Specifier::Bytes(bytes) => bytes,
                _ => return write(argument),
            };
            let text = String::from_utf8_lossy(bytes);
            write(Argument {
                specifier: Specifier::Bytes(text.as_bytes()),
                ..argument
            })
        })
    };

    if written < 0 {
        return None;
    }

    Some(message.trim_end().to_owned())
}

#[unsafe(export_name = "MechLogError")]
pub unsafe extern "C" fn log_error(message: *const c_char) {
    if let Some(message) = text(message) {
        error!("{message}");
    }
}

#[unsafe(export_name = "MechLogErrorf")]
pub unsafe extern "C" fn log_errorf(format: *const c_char, args: ...) {
    if let Some(message) = formatted_text(format, args) {
        error!("{message}");
    }
}

#[unsafe(export_name = "MechLogDebug")]
pub unsafe extern "C" fn log_debug(message: *const c_char) {
    if let Some(message) = text(message) {
        debug!("{message}");
    }
}

#[unsafe(export_name = "MechLogDebugf")]
pub unsafe extern "C" fn log_debugf(format: *const c_char, args: ...) {
    if let Some(message) = formatted_text(format, args) {
        debug!("{message}");
    }
}

#[unsafe(export_name = "MechLogTrace")]
pub unsafe extern "C" fn log_trace(message: *const c_char) {
    if let Some(message) = text(message) {
        trace!("{message}");
    }
}

#[unsafe(export_name = "MechLogTracef")]
pub unsafe extern "C" fn log_tracef(format: *const c_char, args: ...) {
    if let Some(message) = formatted_text(format, args) {
        trace!("{message}");
    }
}
