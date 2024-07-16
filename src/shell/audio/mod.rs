use anyhow::Result;

mod audio_sample;
mod audio_subsystem;
mod hooks;
mod midi_sequence;

pub unsafe fn hook_functions(base_address: usize) -> Result<()> {
    hooks::hook_functions(base_address)
}

pub unsafe fn unhook_functions() {
    hooks::unhook_functions();
}
