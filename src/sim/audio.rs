use binding::{patches, thunks};

use crate::ailrs::interface::{
    allocate_file_sample, allocate_sample_handle, end_sample, init_sample, load_sample_buffer,
    register_eos_callback, release_sample_handle, resume_sample, sample_buffer_ready,
    sample_user_data, serve, set_preference, set_sample_loop_count, set_sample_pan,
    set_sample_playback_rate, set_sample_type, set_sample_user_data, set_sample_volume,
    start_sample, stop_sample, wave_out_open,
};

use super::MODULE;

thunks! {
    /// The sim's Miles Sound System imports, redirected to `ailrs`.
    static AIL_THUNKS = [
        0x001836e8 => allocate_file_sample,
        0x00183654 => allocate_sample_handle,
        0x00183674 => end_sample,
        0x001836dc => init_sample,
        0x00183690 => load_sample_buffer,
        0x001836d8 => register_eos_callback,
        0x001836ec => release_sample_handle,
        0x0018366c => resume_sample,
        0x00183650 => sample_buffer_ready,
        0x00183664 => sample_user_data,
        0x00183698 => set_preference,
        0x001836e4 => set_sample_loop_count,
        0x0018368c => set_sample_pan,
        0x001836d0 => set_sample_playback_rate,
        0x001836d4 => set_sample_type,
        0x00183678 => set_sample_user_data,
        0x00183668 => set_sample_volume,
        0x001836e0 => start_sample,
        0x00183670 => stop_sample,
        0x001836b0 => wave_out_open,
        0x001836b4 => serve,
    ];
}

patches!(
    pub(super) static PATCHES = [
        patch AIL_THUNKS,
    ];
);
