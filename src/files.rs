use std::{
    ffi::{CStr, CString, OsStr, c_char, c_int, c_void},
    fs,
    path::{Component, MAIN_SEPARATOR_STR, Path, PathBuf},
    ptr,
    sync::OnceLock,
};

use tracing::warn;

/// The game's root directory. For now, this is just the current working directory.
fn root() -> &'static Path {
    static ROOT: OnceLock<PathBuf> = OnceLock::new();
    ROOT.get_or_init(|| std::env::current_dir().unwrap_or_default())
}

pub fn resolve(path: &str) -> PathBuf {
    resolve_in(root(), path)
}

fn resolve_in(root: &Path, path: &str) -> PathBuf {
    let native = path.replace(['\\', '/'], MAIN_SEPARATOR_STR);
    let path = Path::new(&native);

    let mut resolved = if path.is_absolute() {
        PathBuf::new()
    } else {
        root.to_path_buf()
    };
    for component in path.components() {
        match component {
            Component::Normal(name) => resolved = child(&resolved, name),
            Component::CurDir => {}
            other => resolved.push(other),
        }
    }
    resolved
}

/// `name` in `dir`, as it is on disk if something there matches it
fn child(dir: &Path, name: &OsStr) -> PathBuf {
    let exact = dir.join(name);
    if exact.symlink_metadata().is_ok() {
        return exact;
    }

    let Some(wanted) = name.to_str() else {
        return exact;
    };
    match pick(names_in(dir), wanted) {
        Some(found) => dir.join(found),
        None => exact,
    }
}

fn names_in(dir: &Path) -> impl Iterator<Item = String> {
    fs::read_dir(dir)
        .into_iter()
        .flatten()
        .flatten()
        .filter_map(|entry| entry.file_name().into_string().ok())
}

/// The name that matches `wanted` ignoring case.
/// Returns the first in sorted order if several match.
fn pick(names: impl Iterator<Item = String>, wanted: &str) -> Option<String> {
    let wanted = wanted.to_lowercase();
    let mut matches: Vec<String> = names.filter(|name| name.to_lowercase() == wanted).collect();
    matches.sort();

    if let [first, _, ..] = matches.as_slice() {
        warn!("files: several names match {wanted}, using {first}");
    }
    matches.into_iter().next()
}

/// The names of the files matching `pattern`, sorted.
/// Only the pattern's last component may have wildcards.
pub fn find(pattern: &str) -> Vec<String> {
    let (dir, pattern) = match pattern.rfind(['\\', '/']) {
        Some(at) => (resolve(&pattern[..at]), &pattern[at + 1..]),
        None => (root().to_path_buf(), pattern),
    };

    let mut names: Vec<String> = names_in(&dir)
        .filter(|name| matches_wildcards(pattern, name) && dir.join(name).is_file())
        .collect();
    names.sort();
    names
}

/// Whether `name` matches a pattern of `*` and `?` wildcards, ignoring case
fn matches_wildcards(pattern: &str, name: &str) -> bool {
    fn matches(pattern: &[char], name: &[char]) -> bool {
        match pattern {
            [] => name.is_empty(),
            ['*', rest @ ..] => (0..=name.len()).any(|skip| matches(rest, &name[skip..])),
            ['?', rest @ ..] => !name.is_empty() && matches(rest, &name[1..]),
            [c, rest @ ..] => name.first() == Some(c) && matches(rest, &name[1..]),
        }
    }

    // To DOS every name has an extension, if an empty one
    let pattern = if pattern == "*.*" { "*" } else { pattern };
    let pattern: Vec<char> = pattern.to_lowercase().chars().collect();
    let name: Vec<char> = name.to_lowercase().chars().collect();
    matches(&pattern, &name)
}

/// Resolve a path the game passed in
unsafe fn game_path(path: *const c_char) -> Option<PathBuf> {
    if path.is_null() {
        return None;
    }

    match unsafe { CStr::from_ptr(path) }.to_str() {
        Ok(path) => Some(resolve(path)),
        Err(_) => {
            warn!("files: a path isn't UTF-8");
            None
        }
    }
}

#[cfg(windows)]
unsafe extern "C" {
    fn _wfopen(path: *const u16, mode: *const u16) -> *mut c_void;
    fn _wopen(path: *const u16, flags: c_int, ...) -> c_int;
}

#[cfg(not(windows))]
unsafe extern "C" {
    fn fopen(path: *const c_char, mode: *const c_char) -> *mut c_void;
    fn open(path: *const c_char, flags: c_int, ...) -> c_int;
}

#[cfg(windows)]
fn wide(text: &OsStr) -> Vec<u16> {
    use std::os::windows::ffi::OsStrExt;

    text.encode_wide().chain([0]).collect()
}

#[cfg(not(windows))]
fn narrow(path: &Path) -> Option<CString> {
    use std::os::unix::ffi::OsStrExt;

    CString::new(path.as_os_str().as_bytes()).ok()
}

#[cfg(windows)]
unsafe fn c_fopen(path: &Path, mode: &CStr) -> *mut c_void {
    let mode = wide(OsStr::new(&*mode.to_string_lossy()));
    unsafe { _wfopen(wide(path.as_os_str()).as_ptr(), mode.as_ptr()) }
}

#[cfg(not(windows))]
unsafe fn c_fopen(path: &Path, mode: &CStr) -> *mut c_void {
    match narrow(path) {
        Some(path) => unsafe { fopen(path.as_ptr(), mode.as_ptr()) },
        None => ptr::null_mut(),
    }
}

#[cfg(windows)]
unsafe fn c_open(path: &Path, flags: c_int, permissions: c_int) -> c_int {
    unsafe { _wopen(wide(path.as_os_str()).as_ptr(), flags, permissions) }
}

#[cfg(not(windows))]
unsafe fn c_open(path: &Path, flags: c_int, permissions: c_int) -> c_int {
    match narrow(path) {
        Some(path) => unsafe { open(path.as_ptr(), flags, permissions) },
        None => -1,
    }
}

/// 0, or -1 with the failure logged
fn status(what: &str, path: &Path, result: std::io::Result<()>) -> c_int {
    match result {
        Ok(()) => 0,
        Err(e) => {
            warn!("files: couldn't {what} {}: {e}", path.display());
            -1
        }
    }
}

#[unsafe(export_name = "MechFopen")]
pub unsafe extern "C" fn mech_fopen(path: *const c_char, mode: *const c_char) -> *mut c_void {
    let Some(path) = (unsafe { game_path(path) }) else {
        return ptr::null_mut();
    };
    if mode.is_null() {
        return ptr::null_mut();
    }

    unsafe { c_fopen(&path, CStr::from_ptr(mode)) }
}

#[unsafe(export_name = "MechOpen")]
pub unsafe extern "C" fn mech_open(path: *const c_char, flags: c_int, permissions: c_int) -> c_int {
    let Some(path) = (unsafe { game_path(path) }) else {
        return -1;
    };

    unsafe { c_open(&path, flags, permissions) }
}

#[unsafe(export_name = "MechRemove")]
pub unsafe extern "C" fn mech_remove(path: *const c_char) -> c_int {
    let Some(path) = (unsafe { game_path(path) }) else {
        return -1;
    };

    status("remove", &path, fs::remove_file(&path))
}

#[unsafe(export_name = "MechRename")]
pub unsafe extern "C" fn mech_rename(from: *const c_char, to: *const c_char) -> c_int {
    let (Some(from), Some(to)) = (unsafe { (game_path(from), game_path(to)) }) else {
        return -1;
    };

    status("rename", &from, fs::rename(&from, &to))
}

#[unsafe(export_name = "MechMakeDir")]
pub unsafe extern "C" fn mech_make_dir(path: *const c_char) -> c_int {
    let Some(path) = (unsafe { game_path(path) }) else {
        return -1;
    };

    status("create", &path, fs::create_dir(&path))
}

#[unsafe(export_name = "MechFileExists")]
pub unsafe extern "C" fn mech_file_exists(path: *const c_char) -> c_int {
    let exists = unsafe { game_path(path) }.is_some_and(|path| path.exists());
    c_int::from(exists)
}

pub struct FileList {
    names: Vec<CString>,
}

#[unsafe(export_name = "MechFindFiles")]
pub unsafe extern "C" fn mech_find_files(pattern: *const c_char) -> *mut FileList {
    let names = if pattern.is_null() {
        Vec::new()
    } else {
        match unsafe { CStr::from_ptr(pattern) }.to_str() {
            Ok(pattern) => find(pattern),
            Err(_) => Vec::new(),
        }
    };

    let names = names
        .into_iter()
        .filter_map(|name| CString::new(name).ok())
        .collect();
    Box::into_raw(Box::new(FileList { names }))
}

#[unsafe(export_name = "MechFileListCount")]
pub unsafe extern "C" fn mech_file_list_count(list: *const FileList) -> usize {
    unsafe { list.as_ref() }.map_or(0, |list| list.names.len())
}

#[unsafe(export_name = "MechFileListName")]
pub unsafe extern "C" fn mech_file_list_name(list: *const FileList, index: usize) -> *const c_char {
    unsafe { list.as_ref() }
        .and_then(|list| list.names.get(index))
        .map_or(ptr::null(), |name| name.as_ptr())
}

#[unsafe(export_name = "MechFileListFree")]
pub unsafe extern "C" fn mech_file_list_free(list: *mut FileList) {
    if !list.is_null() {
        drop(unsafe { Box::from_raw(list) });
    }
}
