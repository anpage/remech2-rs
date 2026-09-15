use binding::{data_patches, patches};

use super::MODULE;

data_patches! {
    /// Patch the pause menu's "Flee to Windows" option to be "Flee to Desktop"
    static DESKTOP_LABELS = [
        flee_option: 0x000a1ad0 => b"Desktop",
        flee_title: 0x000a1b08 => b"DESKTOP",
    ];
}

patches!(
    pub(super) static PATCHES = [
        patch DESKTOP_LABELS,
    ];
);
