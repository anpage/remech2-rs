use anyhow::Result;

mod custom_drawmode;
mod hooks;
mod overlay_ui;

pub unsafe fn hook_functions(base_address: usize) -> Result<()> {
    unsafe { hooks::hook_functions(base_address) }
}

pub unsafe fn unhook_functions() {
    unsafe {
        hooks::unhook_functions();
    }
}
