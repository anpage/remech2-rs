use windows::{
    Win32::{
        Foundation::{HANDLE, INVALID_HANDLE_VALUE},
        Security::SECURITY_ATTRIBUTES,
        Storage::FileSystem::{
            CreateFileA, FILE_CREATION_DISPOSITION, FILE_FLAGS_AND_ATTRIBUTES, FILE_SHARE_MODE,
        },
    },
    core::PCSTR,
};

use binding::{data_patches, patches, thunks};

use super::SMACK_MODULE;

thunks! {
    static SMACK_THUNKS in SMACK_MODULE = [
        0x0000e150 => create_file,
    ];
}

data_patches! {
    /// Make Smacker skip DirectSound and use its waveOut path instead.
    /// DirectSound was causing FMV audio to go missing on Windows.
    static SMACK_DATA_PATCHES in SMACK_MODULE = [
        use_direct_sound: 0x0000c610 => &[0u8; 4],
    ];
}

/// Patched to avoid a bug where Smacker would infinite loop as it failed to read the video file.
/// It seems like reading a file without buffering has stricter requirements in modern WIndows.
unsafe extern "system" fn create_file(
    lpfilename: PCSTR,
    dwdesiredaccess: u32,
    dwsharemode: FILE_SHARE_MODE,
    lpsecurityattributes: *const SECURITY_ATTRIBUTES,
    dwcreationdisposition: FILE_CREATION_DISPOSITION,
    dwflagsandattributes: FILE_FLAGS_AND_ATTRIBUTES,
    htemplatefile: HANDLE,
) -> HANDLE {
    unsafe {
        let lpsecurityattributes = if lpsecurityattributes.is_null() {
            None
        } else {
            Some(lpsecurityattributes)
        };

        let htemplatefile = if htemplatefile.is_invalid() {
            None
        } else {
            Some(htemplatefile)
        };

        // Remove FILE_FLAG_NO_BUFFERING
        let dwflagsandattributes = dwflagsandattributes.0 & !0x2000_0000;

        if let Ok(handle) = CreateFileA(
            lpfilename,
            dwdesiredaccess,
            dwsharemode,
            lpsecurityattributes,
            dwcreationdisposition,
            FILE_FLAGS_AND_ATTRIBUTES(dwflagsandattributes),
            htemplatefile,
        ) {
            handle
        } else {
            INVALID_HANDLE_VALUE
        }
    }
}

patches!(
    pub(super) static PATCHES = [
        patch SMACK_THUNKS,
        patch SMACK_DATA_PATCHES,
    ];
);
