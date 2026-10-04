use std::ffi::{CStr, c_char, c_int};
use std::sync::Mutex;

use tracing::warn;
use windows::Win32::Foundation::WPARAM;

use crate::messages;
use crate::shell::overlay::confirm;
use crate::shell::screens::ShellMsg;

const DECLINED: i32 = 1;

struct Message {
    lines: Vec<String>,
    buttons: usize,
}

/// Splits `line1|line2#Ok` into lines and button count.
/// A message with no `#` has no buttons, which the original renders with the `Ok` template.
fn parse(message: &str) -> Message {
    let (lines, buttons) = match message.split_once('#') {
        Some((lines, buttons)) => (lines, buttons.split('|').count()),
        None => (message, 0),
    };
    Message {
        lines: if lines.is_empty() {
            Vec::new()
        } else {
            lines.split('|').map(str::to_owned).collect()
        },
        buttons,
    }
}

#[unsafe(export_name = "ShowDialog")]
pub unsafe extern "C" fn show_dialog(message: *const c_char, _confirm: c_int) -> c_int {
    if message.is_null() {
        return 0;
    }
    let raw = unsafe { CStr::from_ptr(message) }
        .to_string_lossy()
        .into_owned();
    let message = parse(&raw);
    let lines: Vec<&str> = message.lines.iter().map(String::as_str).collect();

    if message.buttons == 2 {
        return if confirm::run(&lines) { 0 } else { DECLINED };
    }

    confirm::notify(&lines);
    0
}

/// A video transition blocked while a prompt is up
static PARKED: Mutex<Option<(u32, usize)>> = Mutex::new(None);

/// Blocks the landing and finale transitions while a prompt is on screen
pub fn park_transition(message: u32, wparam: WPARAM) -> bool {
    let message = ShellMsg(message);
    if (message != ShellMsg::LANDING && message != ShellMsg::FINALE) || !confirm::is_open() {
        return false;
    }

    let mut parked = PARKED.lock().unwrap();
    if let Some((blocked, _)) = *parked {
        warn!(
            "dropping blocked transition {blocked:#x} for {:#x}",
            message.0
        );
    }
    *parked = Some((message.0, wparam.0));
    true
}

/// Re-posts a blocked transition once the prompt is gone
pub fn replay_transition() {
    if confirm::is_open() {
        return;
    }
    let Some((message, wparam)) = PARKED.lock().unwrap().take() else {
        return;
    };
    messages::post(message, wparam, 0);
}
