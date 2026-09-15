use std::ffi::{c_char, c_void};

use binding::{game_fns, globals, macros::hook, patches};
use windows::{Win32::Foundation::TRUE, core::BOOL};

use super::MODULE;

type ResolutionLabelFunc = unsafe extern "cdecl" fn(*mut SomeSettingsStruct) -> *mut *mut c_void;
type ResolutionToggleFunc = unsafe extern "cdecl" fn(*mut SomeSettingsStruct);

#[repr(C)]
struct SomeSettingsStruct {
    unknown1: i32,
    unknown2: i32,
    unknown3: i32,
    unknown4: i32,
    unknown5: [u8; 12],
    label_func: ResolutionLabelFunc,
    toggle_func: ResolutionToggleFunc,
    value: *mut [c_char; 15],
}

game_fns!(
    static G_SOME_SETTINGS_WEIRD_FUNC: unsafe extern "thiscall" fn(
        *mut c_void,
        i32,
        i32,
        *const c_char,
        u32,
    ) -> *mut *mut c_void = 0x0000544e;
);

globals!(
    static G_SOME_SETTINGS_WEIRD_GLOBAL: *mut c_void = 0x00071214;
);

#[hook(rva = 0x000435e9)]
unsafe extern "cdecl" fn resolution_label(settings: *mut SomeSettingsStruct) -> *mut *mut c_void {
    unsafe {
        let value = (*(*settings).value)[4];
        let label = if value == b'4' as i8 {
            c"~640x480"
        } else if value == b'7' as i8 {
            c"~1024x768"
        } else {
            c"~320x200"
        };

        (G_SOME_SETTINGS_WEIRD_FUNC.get())(
            G_SOME_SETTINGS_WEIRD_GLOBAL.get(),
            (*settings).unknown1 + (*settings).unknown3 / 2,
            (*settings).unknown2,
            label.as_ptr(),
            0,
        )
    }
}

#[hook(rva = 0x00043703)]
unsafe extern "cdecl" fn resolution_toggle(settings: *mut SomeSettingsStruct) {
    unsafe {
        if (*(*settings).value)[4] == b'4' as i8 {
            std::ptr::copy_nonoverlapping(
                c"vesa768.dll".as_ptr(),
                (*(*settings).value).as_mut_ptr(),
                12,
            );
        } else if (*(*settings).value)[4] == b'7' as i8 {
            (*(*settings).value).fill(0);
        } else {
            std::ptr::copy_nonoverlapping(
                c"vesa480.dll".as_ptr(),
                (*(*settings).value).as_mut_ptr(),
                12,
            );
        }
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
        hook resolution_label,
        hook resolution_toggle,
        hook load_settings_from_registry,
    ];
);
