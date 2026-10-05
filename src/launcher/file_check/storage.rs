use std::path::PathBuf;

/// The roots of the drives, which may not all exist
#[cfg(windows)]
pub fn roots() -> Vec<PathBuf> {
    ('A'..='Z')
        .map(|letter| PathBuf::from(format!("{letter}:\\")))
        .collect()
}

/// The mount points
#[cfg(target_os = "linux")]
pub fn roots() -> Vec<PathBuf> {
    let mounts = std::fs::read_to_string("/proc/self/mounts").unwrap_or_default();
    mounts
        .lines()
        .filter_map(|line| line.split(' ').nth(1))
        .map(unescape_mount_point)
        .collect()
}

#[cfg(not(any(windows, target_os = "linux")))]
pub fn roots() -> Vec<PathBuf> {
    Vec::new()
}

/// `/proc/self/mounts` writes space, tab, newline and backslash as octal escapes (`\040`)
#[cfg(target_os = "linux")]
fn unescape_mount_point(field: &str) -> PathBuf {
    use std::{ffi::OsString, os::unix::ffi::OsStringExt};

    let mut bytes = Vec::with_capacity(field.len());
    let mut rest = field.as_bytes();
    while let [first, tail @ ..] = rest {
        let escaped = match tail {
            [a, b, c, ..] if *first == b'\\' => std::str::from_utf8(&[*a, *b, *c])
                .ok()
                .and_then(|digits| u8::from_str_radix(digits, 8).ok()),
            _ => None,
        };
        match escaped {
            Some(byte) => {
                bytes.push(byte);
                rest = &tail[3..];
            }
            None => {
                bytes.push(*first);
                rest = tail;
            }
        }
    }
    PathBuf::from(OsString::from_vec(bytes))
}
