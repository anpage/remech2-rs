use windows::{
    Win32::{
        Foundation::{TRUE, WIN32_ERROR},
        Security::SECURITY_ATTRIBUTES,
        System::Registry::{
            HKEY, HKEY_CURRENT_USER, HKEY_LOCAL_MACHINE, REG_CREATE_KEY_DISPOSITION,
            REG_OPEN_CREATE_OPTIONS, REG_SAM_FLAGS, RegCreateKeyExA, RegOpenKeyExA,
        },
    },
    core::{BOOL, PCSTR},
};

use binding::{macros::hook, patches, thunks};

use super::MODULE;

thunks! {
    /// Both redirect HKLM to HKCU.
    static REGISTRY_THUNKS = [
        0x000993f0 => reg_create_key_ex_a,
        0x000993e8 => reg_open_key_ex_a,
    ];
}

unsafe extern "system" fn reg_create_key_ex_a(
    h_key: HKEY,
    sub_key: PCSTR,
    reserved: u32,
    class: PCSTR,
    options: REG_OPEN_CREATE_OPTIONS,
    sam: REG_SAM_FLAGS,
    security_attributes: *const SECURITY_ATTRIBUTES,
    result: *mut HKEY,
    disposition: *mut REG_CREATE_KEY_DISPOSITION,
) -> WIN32_ERROR {
    unsafe {
        let h_key = if h_key == HKEY_LOCAL_MACHINE {
            HKEY_CURRENT_USER
        } else {
            h_key
        };

        let security_attributes = if security_attributes.is_null() {
            None
        } else {
            Some(security_attributes)
        };

        let disposition = if disposition.is_null() {
            None
        } else {
            Some(disposition)
        };

        RegCreateKeyExA(
            h_key,
            sub_key,
            Some(reserved),
            class,
            options,
            sam,
            security_attributes,
            result,
            disposition,
        )
    }
}

unsafe extern "system" fn reg_open_key_ex_a(
    h_key: HKEY,
    sub_key: PCSTR,
    reserved: u32,
    sam: REG_SAM_FLAGS,
    result: *mut HKEY,
) -> WIN32_ERROR {
    unsafe {
        let h_key = if h_key == HKEY_LOCAL_MACHINE {
            HKEY_CURRENT_USER
        } else {
            h_key
        };

        RegOpenKeyExA(h_key, sub_key, Some(reserved), sam, result)
    }
}

#[hook(rva = 0x000103e2)]
unsafe extern "cdecl" fn load_settings_from_registry(
    quick_tips: *mut i32,
    show_dialog: *mut i32,
    little_movies: *mut i32,
) -> BOOL {
    unsafe {
        *quick_tips = 0;
        *show_dialog = 0;
        *little_movies = 0;
    }
    TRUE
}

patches!(
    pub(super) static PATCHES = [
        patch REGISTRY_THUNKS,
        hook load_settings_from_registry,
    ];
);
