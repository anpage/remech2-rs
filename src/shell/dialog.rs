use std::ffi::{CStr, c_char};
use std::sync::Mutex;

use binding::{macros::hook, patches};
use tracing::{error, warn};
use windows::Win32::{
    Foundation::{HWND, LPARAM, WPARAM},
    UI::WindowsAndMessaging::PostMessageA,
};

use super::MODULE;
use crate::shell::drawmode::confirm;
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

#[hook(rva = 0x00043e25)]
unsafe extern "cdecl" fn show_dialog(message: *const c_char, _confirm: i32) -> i32 {
    let raw = unsafe { CStr::from_ptr(message) }
        .to_string_lossy()
        .into_owned();
    let message = parse(&raw);

    // We've hooked all the places that show yes/no dialogs.
    // If this gets called with two buttons, we missed something.
    if message.buttons >= 2 {
        error!("confirmation reached the ShowDialog hook: {raw:?}");
        debug_assert!(false);
        return DECLINED;
    }

    let lines: Vec<&str> = message.lines.iter().map(String::as_str).collect();
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
pub fn replay_transition(window: HWND) {
    if confirm::is_open() {
        return;
    }
    let Some((message, wparam)) = PARKED.lock().unwrap().take() else {
        return;
    };
    if let Err(e) = unsafe { PostMessageA(Some(window), message, WPARAM(wparam), LPARAM(0)) } {
        error!("re-posting blocked transition {message:#x} failed: {e}");
    }
}

patches!(
    pub(super) static PATCHES = [
        hook show_dialog,
    ];
);
