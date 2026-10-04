use std::{
    cell::{Cell, RefCell},
    collections::VecDeque,
    ffi::c_int,
};

use mw2_sys::shared::{MechMessage, MechMessageHandler};

thread_local! {
    static QUEUE: RefCell<VecDeque<MechMessage>> = const { RefCell::new(VecDeque::new()) };
    static HANDLER: Cell<MechMessageHandler> = const { Cell::new(None) };
}

/// Adds a message to the back of the queue if the game is there to handle it
pub fn post_to_game(message: u32, wparam: usize, lparam: isize) {
    if HANDLER.get().is_some() {
        post(message, wparam, lparam);
    }
}

/// Adds a message to the back of the queue
pub fn post(message: u32, wparam: usize, lparam: isize) {
    QUEUE.with_borrow_mut(|queue| {
        queue.push_back(MechMessage {
            m_message: message,
            m_wParam: wparam,
            m_lParam: lparam,
        })
    });
}

pub fn discard(first: u32, last: u32) {
    QUEUE.with_borrow_mut(|queue| {
        queue.retain(|queued| !(first..=last).contains(&queued.m_message));
    });
}

#[unsafe(export_name = "MechSetMessageHandler")]
pub extern "C" fn set_handler(handler: MechMessageHandler) -> MechMessageHandler {
    HANDLER.replace(handler)
}

#[unsafe(export_name = "MechPostMessage")]
pub extern "C" fn post_message(message: u32, wparam: usize, lparam: isize) {
    post(message, wparam, lparam);
}

#[unsafe(export_name = "MechSendMessage")]
pub extern "C" fn send_message(message: u32, wparam: usize, lparam: isize) -> isize {
    match HANDLER.get() {
        Some(handler) => unsafe { handler(message, wparam, lparam) },
        None => 0,
    }
}

#[unsafe(export_name = "MechPeekMessage")]
pub unsafe extern "C" fn peek_message(
    message: *mut MechMessage,
    first: u32,
    last: u32,
    remove: c_int,
) -> c_int {
    let any = first == 0 && last == 0;
    let found = QUEUE.with_borrow_mut(|queue| {
        let index = queue
            .iter()
            .position(|queued| any || (first..=last).contains(&queued.m_message))?;
        if remove != 0 {
            queue.remove(index)
        } else {
            queue.get(index).copied()
        }
    });

    match found {
        Some(found) => {
            if let Some(message) = unsafe { message.as_mut() } {
                *message = found;
            }
            1
        }
        None => 0,
    }
}
