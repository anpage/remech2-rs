mod interface;
mod movie;
mod sound;

use binding::{patches, thunks};

use super::MODULE;

thunks! {
    // Covers all of the SMACKW32.DLL functions imported by the shell
    static SMACKER_THUNK = [
        0x00099664 => interface::open,
        0x0009965c => interface::close,
        0x00099658 => interface::do_frame,
        0x00099650 => interface::next_frame,
        0x00099654 => interface::goto,
        0x00099660 => interface::wait,
        0x00099668 => interface::to_buffer,
        0x0009966c => interface::sound_in_track,
        0x00099670 => interface::get_track_data,
    ];
}

patches!(
    pub(in crate::shell) static PATCHES = [
        patch SMACKER_THUNK,
    ];
);
