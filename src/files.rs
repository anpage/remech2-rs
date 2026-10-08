use std::{
    ffi::{CStr, CString, OsStr, c_char, c_int, c_uint, c_void},
    fs::{self, File, OpenOptions},
    io::{ErrorKind, Read, Seek, SeekFrom, Write},
    path::{Component, MAIN_SEPARATOR_STR, Path, PathBuf},
    ptr, slice,
    sync::{Mutex, OnceLock},
};

use anyhow::{Context, Result};
use directories::ProjectDirs;
use tracing::warn;

use crate::heap::{self, MechHeap};

pub struct Root {
    pub game: PathBuf,
    pub user: PathBuf,
}

/// The game's data and user directories.
///
/// ## Windows
/// - `%LOCALAPPDATA%\ReMech2\data\game`
/// - `%APPDATA%\ReMech2\data\user`
///
/// ## Linux
/// - `~/.local/share/remech2/game`
/// - `~/.local/share/remech2/user`
pub fn root() -> &'static Root {
    static ROOT: OnceLock<Root> = OnceLock::new();
    ROOT.get_or_init(|| {
        let cwd = std::env::current_dir().unwrap_or_default();

        if is_portable(&cwd) {
            return Root {
                game: cwd.clone(),
                user: cwd,
            };
        }

        let dirs = match ProjectDirs::from("", "", "ReMech2") {
            Some(dirs) => Root {
                game: dirs.data_local_dir().join("game"),
                user: dirs.data_dir().join("user"),
            },
            None => Root {
                game: cwd.clone(),
                user: cwd,
            },
        };

        if let Err(e) = fs::create_dir_all(&dirs.game) {
            warn!("files: couldn't create {}: {e}", dirs.game.display());
        }

        dirs
    })
}

fn is_portable(cwd: &Path) -> bool {
    if cwd.join("portable.txt").exists() {
        return true;
    }
    resolve_in(cwd, "MW2.PRJ").is_file()
}

fn bases() -> impl Iterator<Item = &'static Path> {
    let root = root();
    let game = (root.game != root.user).then_some(root.game.as_path());
    [root.user.as_path()].into_iter().chain(game)
}

pub fn resolve(path: &str) -> PathBuf {
    let mut candidates = bases().map(|base| resolve_in(base, path));
    let user = candidates.next().unwrap();
    if user.exists() {
        return user;
    }
    candidates.next().unwrap_or(user)
}

pub fn resolve_user(path: &str) -> PathBuf {
    let user = &root().user;
    if let Err(e) = fs::create_dir_all(user) {
        warn!("files: couldn't create {}: {e}", user.display());
    }
    resolve_in(user, path)
}

/// `path` under `root`, matching each component's case to what is on disk
pub fn resolve_in(root: &Path, path: &str) -> PathBuf {
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

/// The names of the files matching `pattern` in `user/` and `game/`
/// Only the pattern's last component may have wildcards.
pub fn find(pattern: &str) -> Vec<String> {
    let (dir, pattern) = match pattern.rfind(['\\', '/']) {
        Some(at) => (&pattern[..at], &pattern[at + 1..]),
        None => ("", pattern),
    };

    let mut names: Vec<String> = Vec::new();
    for base in bases() {
        let dir = resolve_in(base, dir);
        for name in names_in(&dir) {
            let seen = names
                .iter()
                .any(|seen| seen.to_lowercase() == name.to_lowercase());
            if !seen && matches_wildcards(pattern, &name) && dir.join(&name).is_file() {
                names.push(name);
            }
        }
    }
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

/// Resolve a path the game passed in with `resolve` or `resolve_user`
unsafe fn game_path(path: *const c_char, resolve: fn(&str) -> PathBuf) -> Option<PathBuf> {
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

/// `MechOpen`'s modes
const OPEN_READ: c_int = 0;
const OPEN_READ_WRITE: c_int = 1;
const OPEN_CREATE: c_int = 2;
const OPEN_WRITE: c_int = 3;

const SEEK_SET: c_int = 0;
const SEEK_CUR: c_int = 1;
const SEEK_END: c_int = 2;

/// The files `MechOpen` opened, by handle
static OPEN_FILES: Mutex<Vec<Option<File>>> = Mutex::new(Vec::new());

/// Runs `f` on the open file `handle`, or returns -1 if it isn't one
fn with_file<T: From<i8>>(handle: c_int, f: impl FnOnce(&mut File) -> T) -> T {
    let mut files = OPEN_FILES.lock().unwrap();
    let file = usize::try_from(handle)
        .ok()
        .and_then(|handle| files.get_mut(handle))
        .and_then(Option::as_mut);
    match file {
        Some(file) => f(file),
        None => T::from(-1),
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

#[unsafe(export_name = "MechOpen")]
pub unsafe extern "C" fn mech_open(path: *const c_char, mode: c_int) -> c_int {
    let mut options = OpenOptions::new();
    match mode {
        OPEN_READ => options.read(true),
        OPEN_READ_WRITE => options.read(true).write(true),
        OPEN_CREATE => options.write(true).create(true),
        OPEN_WRITE => options.write(true).create(true).truncate(true),
        _ => return -1,
    };
    let resolver: fn(&str) -> PathBuf = if mode == OPEN_READ {
        resolve
    } else {
        resolve_user
    };
    let Some(path) = (unsafe { game_path(path, resolver) }) else {
        return -1;
    };
    let file = match options.open(&path) {
        Ok(file) => file,
        Err(e) => {
            warn!("files: couldn't open {}: {e}", path.display());
            return -1;
        }
    };

    let mut files = OPEN_FILES.lock().unwrap();
    let handle = match files.iter().position(Option::is_none) {
        Some(free) => free,
        None => {
            files.push(None);
            files.len() - 1
        }
    };
    let Ok(result) = c_int::try_from(handle) else {
        return -1;
    };
    files[handle] = Some(file);
    result
}

#[unsafe(export_name = "MechRead")]
pub unsafe extern "C" fn mech_read(file: c_int, buffer: *mut c_void, count: c_uint) -> c_int {
    if buffer.is_null() {
        return -1;
    }
    let buffer = unsafe { slice::from_raw_parts_mut(buffer.cast::<u8>(), count as usize) };
    with_file(file, |file| {
        let mut done = 0;
        while done < buffer.len() {
            match file.read(&mut buffer[done..]) {
                Ok(0) => break,
                Ok(n) => done += n,
                Err(e) if e.kind() == ErrorKind::Interrupted => {}
                Err(_) => return -1,
            }
        }
        c_int::try_from(done).unwrap_or(-1)
    })
}

#[unsafe(export_name = "MechWrite")]
pub unsafe extern "C" fn mech_write(file: c_int, buffer: *const c_void, count: c_uint) -> c_int {
    if buffer.is_null() {
        return -1;
    }
    let buffer = unsafe { slice::from_raw_parts(buffer.cast::<u8>(), count as usize) };
    with_file(file, |file| match file.write_all(buffer) {
        Ok(()) => c_int::try_from(buffer.len()).unwrap_or(-1),
        Err(_) => -1,
    })
}

#[unsafe(export_name = "MechSeek")]
pub extern "C" fn mech_seek(file: c_int, offset: i32, origin: c_int) -> i32 {
    let offset = i64::from(offset);
    let from = match origin {
        SEEK_SET => match u64::try_from(offset) {
            Ok(offset) => SeekFrom::Start(offset),
            Err(_) => return -1,
        },
        SEEK_CUR => SeekFrom::Current(offset),
        SEEK_END => SeekFrom::End(offset),
        _ => return -1,
    };
    with_file(file, |file| match file.seek(from) {
        Ok(position) => i32::try_from(position).unwrap_or(-1),
        Err(_) => -1,
    })
}

#[unsafe(export_name = "MechClose")]
pub extern "C" fn mech_close(file: c_int) -> c_int {
    let mut files = OPEN_FILES.lock().unwrap();
    match usize::try_from(file)
        .ok()
        .and_then(|file| files.get_mut(file))
    {
        Some(slot @ Some(_)) => {
            *slot = None;
            0
        }
        _ => -1,
    }
}

#[unsafe(export_name = "MechFileLength")]
pub extern "C" fn mech_file_length(file: c_int) -> i32 {
    with_file(file, |file| match file.metadata() {
        Ok(metadata) => i32::try_from(metadata.len()).unwrap_or(-1),
        Err(_) => -1,
    })
}

#[unsafe(export_name = "MechRemove")]
pub unsafe extern "C" fn mech_remove(path: *const c_char) -> c_int {
    let Some(path) = (unsafe { game_path(path, resolve_user) }) else {
        return -1;
    };

    status("remove", &path, fs::remove_file(&path))
}

#[unsafe(export_name = "MechRename")]
pub unsafe extern "C" fn mech_rename(from: *const c_char, to: *const c_char) -> c_int {
    let (Some(from), Some(to)) =
        (unsafe { (game_path(from, resolve_user), game_path(to, resolve_user)) })
    else {
        return -1;
    };

    status("rename", &from, fs::rename(&from, &to))
}

#[unsafe(export_name = "MechMakeDir")]
pub unsafe extern "C" fn mech_make_dir(path: *const c_char) -> c_int {
    let Some(path) = (unsafe { game_path(path, resolve_user) }) else {
        return -1;
    };

    status("create", &path, fs::create_dir(&path))
}

#[unsafe(export_name = "MechFileExists")]
pub unsafe extern "C" fn mech_file_exists(path: *const c_char) -> c_int {
    let exists = unsafe { game_path(path, resolve) }.is_some_and(|path| path.exists());
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

#[unsafe(export_name = "MechReadFile")]
pub unsafe extern "C" fn mech_read_file(heap: *mut MechHeap, path: *const c_char) -> *mut c_void {
    let Some(path) = (unsafe { game_path(path, resolve) }) else {
        return ptr::null_mut();
    };

    let read = File::open(&path).and_then(|mut file| {
        let size = usize::try_from(file.metadata()?.len()).map_err(std::io::Error::other)?;
        let block = unsafe { heap::alloc(heap, size) };

        if block.is_null() {
            return Err(ErrorKind::OutOfMemory.into());
        }

        let buffer = unsafe { slice::from_raw_parts_mut(block.cast::<u8>(), size) };

        match file.read_exact(buffer) {
            Ok(()) => Ok(block),
            Err(e) => {
                unsafe { heap::free(heap, block) };
                Err(e)
            }
        }
    });

    match read {
        Ok(block) => block,
        Err(e) => {
            warn!("files: couldn't read {}: {e}", path.display());
            ptr::null_mut()
        }
    }
}

pub fn write_atomically(path: &Path, text: &str) -> Result<()> {
    if let Some(dir) = path.parent() {
        fs::create_dir_all(dir).with_context(|| format!("Couldn't create {}", dir.display()))?;
    }
    let mut temporary = path.as_os_str().to_owned();
    temporary.push(".tmp");
    let temporary = PathBuf::from(temporary);
    fs::write(&temporary, text)
        .with_context(|| format!("Couldn't write {}", temporary.display()))?;
    fs::rename(&temporary, path).with_context(|| format!("Couldn't replace {}", path.display()))
}
