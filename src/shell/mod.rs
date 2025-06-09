use std::{
    ffi::{CString, c_char, c_void},
    fs,
    sync::RwLock,
};

use anyhow::{Context, Result, bail};
use retour::{GenericDetour, RawDetour};
use windows::{
    Win32::{
        Foundation::{FreeLibrary, HMODULE, HWND, WIN32_ERROR},
        Graphics::Gdi::{BitBlt, HDC, SRCCOPY},
        Security::SECURITY_ATTRIBUTES,
        System::{
            LibraryLoader::{GetModuleHandleA, GetProcAddress, LoadLibraryA},
            Registry::{
                HKEY, HKEY_CURRENT_USER, HKEY_LOCAL_MACHINE, REG_CREATE_KEY_DISPOSITION,
                REG_OPEN_CREATE_OPTIONS, REG_SAM_FLAGS, RegCreateKeyExA, RegOpenKeyExA,
            },
        },
    },
    core::{PCSTR, s},
};
use windows_sys::Win32::{
    Foundation::HANDLE,
    Storage::FileSystem::{
        CreateFileA, FILE_CREATION_DISPOSITION, FILE_FLAGS_AND_ATTRIBUTES, FILE_SHARE_MODE,
    },
};

use crate::{
    WindowProc,
    ail::Ail,
    common::{HeapFreeFunc, debug_log, fake_heap_free},
    hooker::hook_function,
};

mod audio;
mod drawmode;

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

static DEBUG_LOG_HOOK: RwLock<Option<RawDetour>> = RwLock::new(None);

type LoadMechVariantListFunc = unsafe extern "cdecl" fn(*const c_char);
static LOAD_MECH_VARIANT_LIST_HOOK: RwLock<Option<GenericDetour<LoadMechVariantListFunc>>> =
    RwLock::new(None);

type GetDbItemLzFunc =
    unsafe extern "fastcall" fn(*mut c_void, *mut c_void, i32, *mut *mut u8, *mut usize) -> i32;
static GET_DB_ITEM_LZ_HOOK: RwLock<Option<GenericDetour<GetDbItemLzFunc>>> = RwLock::new(None);

type ResolutionLabelFunc = unsafe extern "cdecl" fn(*mut SomeSettingsStruct) -> *mut *mut c_void;
static RESOLUTION_LABEL_HOOK: RwLock<Option<GenericDetour<ResolutionLabelFunc>>> =
    RwLock::new(None);

type ResolutionToggleFunc = unsafe extern "cdecl" fn(*mut SomeSettingsStruct);
static RESOLTUTION_TOGGLE_HOOK: RwLock<Option<GenericDetour<ResolutionToggleFunc>>> =
    RwLock::new(None);

type SomeSettingsWeirdFunc =
    unsafe extern "thiscall" fn(*mut c_void, i32, i32, *const c_char, u32) -> *mut *mut c_void;
static G_SOME_SETTINGS_WEIRD_FUNC: RwLock<Option<SomeSettingsWeirdFunc>> = RwLock::new(None);

type LoadFileFromPrjFunc = unsafe extern "thiscall" fn(*mut c_void, *const c_char, i32) -> i32;
static G_LOAD_FILE_FROM_PRJ: RwLock<Option<LoadFileFromPrjFunc>> = RwLock::new(None);

static mut G_MECH_VARIANT_FILENAME: *mut c_char = std::ptr::null_mut();
static mut G_MECH_VARIANT_FILENAMES: *mut [[c_char; 13]; 200] = std::ptr::null_mut();
static mut G_PRJ_OBJECT: *mut c_void = std::ptr::null_mut();

static mut G_HDC: *mut HDC = std::ptr::null_mut();
static mut G_HDC_SRC: *mut HDC = std::ptr::null_mut();
static mut G_BIT_BLIT_WIDTH: *mut i32 = std::ptr::null_mut();
static mut G_BIT_BLIT_HEIGHT: *mut i32 = std::ptr::null_mut();
static mut G_BIT_BLIT_RESULT: *mut i32 = std::ptr::null_mut();

static mut G_DATABASE_MW2: *mut *mut c_void = std::ptr::null_mut();

static mut G_SOME_SETTINGS_WEIRD_GLOBAL: *mut *mut c_void = std::ptr::null_mut();

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

            *G_LOAD_FILE_FROM_PRJ.write().unwrap() = Some(std::mem::transmute::<
                usize,
                LoadFileFromPrjFunc,
            >(base_address + 0x0002e346));

            *G_SOME_SETTINGS_WEIRD_FUNC.write().unwrap() =
                Some(std::mem::transmute::<usize, SomeSettingsWeirdFunc>(
                    base_address + 0x0000544e,
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

            G_SOME_SETTINGS_WEIRD_GLOBAL = (base_address + 0x00071214) as *mut *mut c_void;

            *DEBUG_LOG_HOOK.write().unwrap() = {
                let hook = RawDetour::new(
                    (base_address + 0x00017982) as *const (),
                    debug_log as *const (),
                )?;
                hook.enable()?;
                Some(hook)
            };

            *LOAD_MECH_VARIANT_LIST_HOOK.write().unwrap() = {
                let load_mech_variant_list: LoadMechVariantListFunc =
                    std::mem::transmute(base_address + 0x0000c8b8);
                Some(hook_function(
                    load_mech_variant_list,
                    Self::load_mech_variant_list,
                )?)
            };

            *GET_DB_ITEM_LZ_HOOK.write().unwrap() = {
                let get_db_item_midi: GetDbItemLzFunc =
                    std::mem::transmute(base_address + 0x0004813f);
                Some(hook_function(get_db_item_midi, Self::get_db_item_lz)?)
            };

            *RESOLUTION_LABEL_HOOK.write().unwrap() = {
                let resolution_label: ResolutionLabelFunc =
                    std::mem::transmute(base_address + 0x000435e9);
                Some(hook_function(resolution_label, Self::resolution_label)?)
            };

            *RESOLTUTION_TOGGLE_HOOK.write().unwrap() = {
                let resolution_toggle: ResolutionToggleFunc =
                    std::mem::transmute(base_address + 0x00043703);
                Some(hook_function(resolution_toggle, Self::resolution_toggle)?)
            };

            audio::hook_functions(base_address)?;
            drawmode::hook_functions(base_address)?;

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
        unsafe {
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
        unsafe {
            GET_DB_ITEM_LZ_HOOK.read().unwrap().as_ref().unwrap().call(
                db,
                unused,
                index,
                midi_data,
                midi_data_size,
            )
        }
    }

    /// Loads the list of mech variants from the MW2.PRJ file and any user variants from the filesystem.
    /// Patched to avoid a bug where the game was using an older Win32 API
    unsafe extern "cdecl" fn load_mech_variant_list(mech_type: *const c_char) {
        unsafe {
            // Create "MEK" folder if it doesn't exist
            fs::create_dir_all("MEK").unwrap();

            let mech_type: &str = std::ffi::CStr::from_ptr(mech_type).to_str().unwrap();

            // Clear the list
            (*G_MECH_VARIANT_FILENAMES).fill([0; 13]);

            // Make sure we have at least the default variant
            let default_variant = format!("{mech_type}00std");
            std::ptr::copy_nonoverlapping(
                default_variant.as_ptr(),
                G_MECH_VARIANT_FILENAME.cast(),
                13,
            );
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

                let result = G_LOAD_FILE_FROM_PRJ.read().unwrap().unwrap()(
                    G_PRJ_OBJECT,
                    variant.as_ptr(),
                    6,
                );

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

    unsafe extern "cdecl" fn resolution_label(
        settings: *mut SomeSettingsStruct,
    ) -> *mut *mut c_void {
        unsafe {
            let value = (*(*settings).value)[4];
            let label = if value == 0x34 {
                "~640x480"
            } else if value == 0x37 {
                "~1024x768"
            } else {
                "~320x200"
            };

            let weird_func = G_SOME_SETTINGS_WEIRD_FUNC.read().unwrap().unwrap();
            return weird_func(
                *G_SOME_SETTINGS_WEIRD_GLOBAL,
                (*settings).unknown1 + (*settings).unknown3 / 2,
                (*settings).unknown2,
                CString::new(label).unwrap().as_ptr(),
                0,
            );
        }
    }

    unsafe extern "cdecl" fn resolution_toggle(settings: *mut SomeSettingsStruct) {
        unsafe {
            if (*(*settings).value)[4] == 0x34 {
                std::ptr::copy_nonoverlapping(
                    c"vesa768.dll".as_ptr(),
                    (*(*settings).value).as_mut_ptr(),
                    12,
                );
            } else if (*(*settings).value)[4] == 0x37 {
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
}

impl Drop for Shell {
    fn drop(&mut self) {
        unsafe {
            crate::SHELL_WINDOW_PROC = None;
            DEBUG_LOG_HOOK.write().unwrap().take();
            LOAD_MECH_VARIANT_LIST_HOOK.write().unwrap().take();
            GET_DB_ITEM_LZ_HOOK.write().unwrap().take();
            RESOLUTION_LABEL_HOOK.write().unwrap().take();
            RESOLTUTION_TOGGLE_HOOK.write().unwrap().take();
            audio::unhook_functions();
            drawmode::unhook_functions();
            self.ail.unhook();
            FreeLibrary(self.module).unwrap();
            LOADED = false;
        }
    }
}
