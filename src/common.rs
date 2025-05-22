use std::ffi::{CStr, c_char, c_int, c_void};

use windows::Win32::{
    Foundation::{BOOL, HANDLE, TRUE},
    System::Memory::HEAP_FLAGS,
};

unsafe extern "C" {
    fn sprintf(s: *mut c_char, format: *const c_char, ...) -> c_int;
}

pub type HeapFreeFunc = unsafe extern "system" fn(HANDLE, HEAP_FLAGS, *const c_void) -> BOOL;
pub unsafe extern "system" fn fake_heap_free(
    _h_heap: HANDLE,
    _dw_flags: HEAP_FLAGS,
    _lp_mem: *const c_void,
) -> BOOL {
    TRUE
}

pub unsafe extern "C" fn debug_log(format: *const c_char, mut args: ...) { unsafe {
    let mut buffer = [0; 256];
    sprintf(buffer.as_mut_ptr(), format, args.as_va_list());
    let buffer = CStr::from_ptr(buffer.as_ptr());
    print!("{}", buffer.to_str().unwrap());
}}
