use std::ffi::{CStr, VaList, c_char, c_int, c_void};

use windows::{
    Win32::{
        Foundation::{HANDLE, HWND, TRUE},
        System::Memory::HEAP_FLAGS,
    },
    core::BOOL,
};

unsafe extern "C" {
    fn vsprintf(s: *mut c_char, format: *const c_char, ap: VaList) -> c_int;
}

pub unsafe extern "system" fn fake_heap_free(
    _h_heap: HANDLE,
    _dw_flags: HEAP_FLAGS,
    _lp_mem: *const c_void,
) -> BOOL {
    TRUE
}

pub unsafe extern "system" fn fake_set_menu(_hwnd: HWND, _h_menu: *mut c_void) -> BOOL {
    TRUE
}

pub unsafe extern "C" fn debug_log(format: *const c_char, args: ...) {
    unsafe {
        if format.is_null() {
            return;
        }
        let mut buffer = [0i8; 1024];
        vsprintf(buffer.as_mut_ptr(), format, args);
        let buffer = CStr::from_ptr(buffer.as_ptr());
        print!("{}", buffer.to_str().unwrap());
    }
}
