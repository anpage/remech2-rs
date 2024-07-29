use std::{
    ffi::{c_char, c_void, CString},
    fs,
};

use anyhow::{bail, Context, Result};
use retour::{GenericDetour, RawDetour};
use windows::{
    core::{s, PCSTR},
    Win32::{
        Foundation::{FreeLibrary, HMODULE, HWND, WIN32_ERROR},
        Graphics::Gdi::{BitBlt, HDC, SRCCOPY},
        Security::SECURITY_ATTRIBUTES,
        System::{
            LibraryLoader::{GetModuleHandleA, GetProcAddress, LoadLibraryA},
            Registry::{
                RegCreateKeyExA, RegOpenKeyExA, HKEY, HKEY_CURRENT_USER, HKEY_LOCAL_MACHINE,
                REG_CREATE_KEY_DISPOSITION, REG_OPEN_CREATE_OPTIONS, REG_SAM_FLAGS,
            },
        },
    },
};
use windows_sys::Win32::{
    Foundation::HANDLE,
    Storage::FileSystem::{
        CreateFileA, FILE_CREATION_DISPOSITION, FILE_FLAGS_AND_ATTRIBUTES, FILE_SHARE_MODE,
    },
};

use crate::{
    ail::Ail,
    common::{debug_log, fake_heap_free, HeapFreeFunc},
    hooker::hook_function,
    WindowProc,
};

mod audio;

type ShellMainProc = unsafe extern "stdcall" fn(HMODULE, i32, *const c_char, i32, HWND) -> i32;

type RegCreateKeyExAFunc = unsafe extern "system" fn(
    HKEY,
    PCSTR,
    u32,
    PCSTR,
    REG_OPEN_CREATE_OPTIONS,
    REG_SAM_FLAGS,
    *const SECURITY_ATTRIBUTES,
    *mut HKEY,
    *mut REG_CREATE_KEY_DISPOSITION,
) -> WIN32_ERROR;
type RegOpenKeyExAFunc =
    unsafe extern "system" fn(HKEY, PCSTR, u32, REG_SAM_FLAGS, *mut HKEY) -> WIN32_ERROR;

type CreateFileFunc = unsafe extern "system" fn(
    lpfilename: windows_sys::core::PCSTR,
    dwdesiredaccess: u32,
    dwsharemode: FILE_SHARE_MODE,
    lpsecurityattributes: *const windows_sys::Win32::Security::SECURITY_ATTRIBUTES,
    dwcreationdisposition: FILE_CREATION_DISPOSITION,
    dwflagsandattributes: FILE_FLAGS_AND_ATTRIBUTES,
    htemplatefile: HANDLE,
) -> HANDLE;

static mut DEBUG_LOG_HOOK: Option<RawDetour> = None;

type LoadMechVariantListFunc = unsafe extern "cdecl" fn(*const c_char);
static mut LOAD_MECH_VARIANT_LIST_HOOK: Option<GenericDetour<LoadMechVariantListFunc>> = None;

type CallsBitBlitFunc = unsafe extern "stdcall" fn() -> i32;
static mut CALLS_BIT_BLIT_HOOK: Option<GenericDetour<CallsBitBlitFunc>> = None;

type GetDbItemLzFunc =
    unsafe extern "fastcall" fn(*mut c_void, *mut c_void, i32, *mut *mut u8, *mut usize) -> i32;
static mut GET_DB_ITEM_LZ_HOOK: Option<GenericDetour<GetDbItemLzFunc>> = None;

type LoadFileFromPrjFunc = unsafe extern "thiscall" fn(*mut c_void, *const c_char, i32) -> i32;
static mut G_LOAD_FILE_FROM_PRJ: Option<LoadFileFromPrjFunc> = None;

static mut G_MECH_VARIANT_FILENAME: *mut c_char = std::ptr::null_mut();
static mut G_MECH_VARIANT_FILENAMES: *mut [[c_char; 13]; 200] = std::ptr::null_mut();
static mut G_PRJ_OBJECT: *mut c_void = std::ptr::null_mut();

static mut G_HDC: *mut HDC = std::ptr::null_mut();
static mut G_HDC_SRC: *mut HDC = std::ptr::null_mut();
static mut G_BIT_BLIT_WIDTH: *mut i32 = std::ptr::null_mut();
static mut G_BIT_BLIT_HEIGHT: *mut i32 = std::ptr::null_mut();
static mut G_BIT_BLIT_RESULT: *mut i32 = std::ptr::null_mut();

static mut G_DATABASE_MW2: *mut *mut c_void = std::ptr::null_mut();

static mut LOADED: bool = false;

pub struct Shell {
    ail: Ail,
    module: HMODULE,
}

impl Shell {
    pub fn new() -> Result<Self> {
        if unsafe { LOADED } {
            bail!("Can't load shell more than once");
        }

        let module = unsafe { LoadLibraryA(s!("MW2SHELL.DLL"))? };
        let base_address = module.0 as usize;

        let smack_module = unsafe { GetModuleHandleA(s!("SMACKW32.DLL"))? };
        let smack_base_address = smack_module.0 as usize;

        unsafe {
            let heap_free_thunk = (base_address + 0x0009952c) as *mut HeapFreeFunc;
            *heap_free_thunk = fake_heap_free;

            let reg_create_key_ex_a_thunk = (base_address + 0x000993f0) as *mut RegCreateKeyExAFunc;
            *reg_create_key_ex_a_thunk = Self::reg_create_key_ex_a;

            let reg_open_key_ex_a_thunk = (base_address + 0x000993e8) as *mut RegOpenKeyExAFunc;
            *reg_open_key_ex_a_thunk = Self::reg_open_key_ex_a;

            let create_file_thunk = (smack_base_address + 0x0000e150) as *mut CreateFileFunc;
            *create_file_thunk = Self::create_file;

            G_LOAD_FILE_FROM_PRJ = Some(std::mem::transmute::<usize, LoadFileFromPrjFunc>(
                base_address + 0x0002e346,
            ));

            G_MECH_VARIANT_FILENAME = (base_address + 0x0007a800) as *mut c_char;
            G_MECH_VARIANT_FILENAMES = (base_address + 0x00079d80) as *mut [[c_char; 13]; 200];
            G_PRJ_OBJECT = (base_address + 0x00071230) as *mut c_void;

            G_HDC = (base_address + 0x00066df8) as *mut HDC;
            G_HDC_SRC = (base_address + 0x00067204) as *mut HDC;
            G_BIT_BLIT_WIDTH = (base_address + 0x00096e8c) as *mut i32;
            G_BIT_BLIT_HEIGHT = (base_address + 0x00096e90) as *mut i32;
            G_BIT_BLIT_RESULT = (base_address + 0x000965f4) as *mut i32;

            G_DATABASE_MW2 = (base_address + 0x0007122c) as *mut *mut c_void;

            DEBUG_LOG_HOOK = {
                let hook = RawDetour::new(
                    (base_address + 0x00017982) as *const (),
                    debug_log as *const (),
                )?;
                hook.enable()?;
                Some(hook)
            };

            LOAD_MECH_VARIANT_LIST_HOOK = {
                let load_mech_variant_list: LoadMechVariantListFunc =
                    std::mem::transmute(base_address + 0x0000c8b8);
                Some(hook_function(
                    load_mech_variant_list,
                    Self::load_mech_variant_list,
                )?)
            };

            CALLS_BIT_BLIT_HOOK = {
                let calls_bit_blit: CallsBitBlitFunc =
                    std::mem::transmute(base_address + 0x00030ef9);
                Some(hook_function(calls_bit_blit, Self::calls_bit_blit)?)
            };

            GET_DB_ITEM_LZ_HOOK = {
                let get_db_item_midi: GetDbItemLzFunc =
                    std::mem::transmute(base_address + 0x0004813f);
                Some(hook_function(get_db_item_midi, Self::get_db_item_lz)?)
            };

            audio::hook_functions(base_address)?;

            let ail = Ail::new()?;

            LOADED = true;

            Ok(Self { ail, module })
        }
    }

    pub fn shell_main(&self, intro_or_sim: &str, window: HWND) -> Result<i32> {
        let shell_main = unsafe {
            let shell_main =
                GetProcAddress(self.module, s!("ShellMain")).context("Couldn't find ShellMain")?;
            std::mem::transmute::<*const (), ShellMainProc>(shell_main as *const ())
        };

        let intro_or_sim = CString::new(intro_or_sim).context("CString::new failed")?;
        let result = unsafe { shell_main(self.module, 0, intro_or_sim.as_ptr(), 1, window) };

        if result == -1 {
            bail!("REMECH 2 is unable to locate necessary program components.");
        }

        Ok(result)
    }

    pub fn window_proc(&self) -> Result<WindowProc> {
        unsafe {
            let window_proc = GetProcAddress(self.module, s!("ShellWindowProc"))
                .context("Couldn't find ShellWindowProc")?;
            Ok(std::mem::transmute::<
                unsafe extern "system" fn() -> isize,
                WindowProc,
            >(window_proc))
        }
    }

    /// Patched to avoid a bug where Smacker would infinite loop as it failed to read the video file.
    /// It seems like reading a file without buffering has stricter requirements in modern WIndows.
    unsafe extern "system" fn create_file(
        lpfilename: windows_sys::core::PCSTR,
        dwdesiredaccess: u32,
        dwsharemode: FILE_SHARE_MODE,
        lpsecurityattributes: *const windows_sys::Win32::Security::SECURITY_ATTRIBUTES,
        dwcreationdisposition: FILE_CREATION_DISPOSITION,
        dwflagsandattributes: FILE_FLAGS_AND_ATTRIBUTES,
        htemplatefile: HANDLE,
    ) -> HANDLE {
        // Remove FILE_FLAG_NO_BUFFERING
        let dwflagsandattributes = dwflagsandattributes & !0x2000_0000;
        CreateFileA(
            lpfilename,
            dwdesiredaccess,
            dwsharemode,
            lpsecurityattributes,
            dwcreationdisposition,
            dwflagsandattributes,
            htemplatefile,
        )
    }

    /// Called by the game to load a file from DATABASE.MW2 and LZ decompress it.
    ///
    /// Hooking it for now to allow for reimplementation later.
    unsafe extern "fastcall" fn get_db_item_lz(
        db: *mut c_void,
        unused: *mut c_void,
        index: i32,
        midi_data: *mut *mut u8,
        midi_data_size: *mut usize,
    ) -> i32 {
        GET_DB_ITEM_LZ_HOOK
            .as_ref()
            .unwrap()
            .call(db, unused, index, midi_data, midi_data_size)
    }

    /// Loads the list of mech variants from the MW2.PRJ file and any user variants from the filesystem.
    /// Patched to avoid a bug where the game was using an older Win32 API
    unsafe extern "cdecl" fn load_mech_variant_list(mech_type: *const c_char) {
        // Create "MEK" folder if it doesn't exist
        fs::create_dir_all("MEK").unwrap();

        let mech_type: &str = std::ffi::CStr::from_ptr(mech_type).to_str().unwrap();

        // Clear the list
        (*G_MECH_VARIANT_FILENAMES).fill([0; 13]);

        // Make sure we have at least the default variant
        let default_variant = format!("{mech_type}00std");
        std::ptr::copy_nonoverlapping(default_variant.as_ptr(), G_MECH_VARIANT_FILENAME.cast(), 13);
        std::ptr::copy_nonoverlapping(
            default_variant.as_ptr(),
            (*G_MECH_VARIANT_FILENAMES)[0].as_mut_ptr().cast(),
            13,
        );

        // Load the built-in mech variants from the MW2.PRJ file into the next 99 indices
        for i in 1..100 {
            let variant = format!("{mech_type}{i:02}std");
            std::ptr::copy_nonoverlapping(variant.as_ptr(), G_MECH_VARIANT_FILENAME.cast(), 13);

            let variant = CString::new(variant).unwrap();

            let result = G_LOAD_FILE_FROM_PRJ.unwrap()(G_PRJ_OBJECT, variant.as_ptr(), 6);

            if result > -1 {
                std::ptr::copy_nonoverlapping(
                    variant.as_ptr(),
                    (*G_MECH_VARIANT_FILENAMES)[i].as_mut_ptr(),
                    13,
                );
            }
        }

        // Find all user-defined mech variants from the filesystem and load their names into index 100 and higher
        let files = fs::read_dir("mek").unwrap();
        for file in files {
            let path = file.unwrap().path();
            let filename = path.file_name().unwrap().to_str().unwrap();
            if &filename[..3] == mech_type && &filename[5..] == "usr.mek" {
                let variant = CString::new(filename[..8].to_string()).unwrap();
                let variant = variant.as_ptr();
                let i = 100 + filename[3..5].parse::<usize>().unwrap();

                std::ptr::copy_nonoverlapping(variant, G_MECH_VARIANT_FILENAME, 13);

                if (*G_MECH_VARIANT_FILENAMES)[i][0] == 0 {
                    std::ptr::copy_nonoverlapping(variant, G_MECH_VARIANT_FILENAME, 13);
                    break;
                }
            }
        }

        dbg!(&(*G_MECH_VARIANT_FILENAMES));
    }

    /// This is the function that the shell uses to draw to the window with GDI's BitBlt function.
    /// The original function incorrectly failed if the call didn't return the number of lines blitted.
    unsafe extern "stdcall" fn calls_bit_blit() -> i32 {
        unsafe {
            let result = BitBlt(
                *G_HDC,
                0,
                0,
                *G_BIT_BLIT_WIDTH,
                *G_BIT_BLIT_HEIGHT,
                *G_HDC_SRC,
                0,
                0,
                SRCCOPY,
            );

            if let Ok(()) = result {
                *G_BIT_BLIT_RESULT = 1;
                0
            } else {
                *G_BIT_BLIT_RESULT = 0;
                -1
            }
        }
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
            reserved,
            class,
            options,
            sam,
            security_attributes,
            result,
            disposition,
        )
    }

    unsafe extern "system" fn reg_open_key_ex_a(
        h_key: HKEY,
        sub_key: PCSTR,
        reserved: u32,
        sam: REG_SAM_FLAGS,
        result: *mut HKEY,
    ) -> WIN32_ERROR {
        let h_key = if h_key == HKEY_LOCAL_MACHINE {
            HKEY_CURRENT_USER
        } else {
            h_key
        };

        RegOpenKeyExA(h_key, sub_key, reserved, sam, result)
    }
}

impl Drop for Shell {
    fn drop(&mut self) {
        unsafe {
            crate::SHELL_WINDOW_PROC = None;
            DEBUG_LOG_HOOK = None;
            LOAD_MECH_VARIANT_LIST_HOOK = None;
            CALLS_BIT_BLIT_HOOK = None;
            GET_DB_ITEM_LZ_HOOK = None;
            audio::unhook_functions();
            self.ail.unhook();
            FreeLibrary(self.module).unwrap();
            LOADED = false;
        }
    }
}
