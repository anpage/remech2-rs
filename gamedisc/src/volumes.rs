use std::{collections::HashSet, path::PathBuf};

/// A mounted optical disc
#[derive(Clone, Debug, PartialEq, Eq)]
pub struct OpticalVolume {
    /// A device path like `/dev/sr0` on Linux or `\\.\D:` on Windows
    pub device: String,
    /// Where its filesystem is mounted, eg. `/run/media/user/MECH2` or `D:\`
    pub mount_point: PathBuf,
}

impl OpticalVolume {
    /// The volume label
    pub fn label(&self) -> Option<String> {
        platform_volume_label(self)
    }
}

/// Mounted optical discs
pub fn optical_volumes() -> Vec<OpticalVolume> {
    platform_optical_volumes()
}

#[cfg(target_os = "linux")]
fn platform_optical_volumes() -> Vec<OpticalVolume> {
    match std::fs::read_to_string("/proc/mounts") {
        Ok(text) => parse_mounts(&text),
        Err(e) => {
            tracing::warn!("couldn't read /proc/mounts: {e}");
            vec![]
        }
    }
}

#[cfg(windows)]
fn platform_optical_volumes() -> Vec<OpticalVolume> {
    use windows_sys::Win32::Storage::FileSystem::{GetDriveTypeW, GetLogicalDrives};
    use windows_sys::Win32::System::WindowsProgramming::DRIVE_CDROM;

    let mask = unsafe { GetLogicalDrives() };
    (0..26u8)
        .filter(|i| mask & (1 << i) != 0)
        .map(|i| char::from(b'A' + i))
        .filter(|letter| {
            let root: Vec<u16> = format!(r"{letter}:\").encode_utf16().chain([0]).collect();
            unsafe { GetDriveTypeW(root.as_ptr()) == DRIVE_CDROM }
        })
        .map(|letter| OpticalVolume {
            device: format!(r"\\.\{letter}:"),
            mount_point: PathBuf::from(format!(r"{letter}:\")),
        })
        .collect()
}

/// The iso9660 and udf entries in /proc/mounts text
pub fn parse_mounts(text: &str) -> Vec<OpticalVolume> {
    let mut seen = HashSet::new();
    text.lines()
        .filter_map(|line| {
            let mut fields = line.split(' ');
            let (device, mount_point, fs_type) = (fields.next()?, fields.next()?, fields.next()?);
            if !matches!(fs_type, "iso9660" | "udf") {
                return None;
            }
            let device = unescape(device);
            seen.insert(device.clone()).then(|| OpticalVolume {
                device,
                mount_point: PathBuf::from(unescape(mount_point)),
            })
        })
        .collect()
}

/// Needed to undo the characters escaped by the kernel
fn unescape(field: &str) -> String {
    let mut out = String::with_capacity(field.len());
    let mut rest = field;

    while let Some(i) = rest.find('\\') {
        out.push_str(&rest[..i]);

        match rest
            .get(i + 1..i + 4)
            .and_then(|o| u8::from_str_radix(o, 8).ok())
        {
            Some(byte) => {
                out.push(char::from(byte));
                rest = &rest[i + 4..];
            }
            None => {
                out.push('\\');
                rest = &rest[i + 1..];
            }
        }
    }

    out.push_str(rest);
    out
}

/// The trimmed volume identifier from an ISO 9660 primary volume descriptor
pub fn label_from_descriptor(sector: &[u8]) -> Option<String> {
    let field = sector.get(40..72)?;

    if sector[0] != 1 || &sector[1..6] != b"CD001" {
        return None;
    }

    let label = String::from_utf8_lossy(field);
    let label = label.trim_end_matches([' ', '\0']);

    (!label.is_empty()).then(|| label.to_owned())
}

#[cfg(target_os = "linux")]
fn platform_volume_label(volume: &OpticalVolume) -> Option<String> {
    use std::io::{Read, Seek, SeekFrom};

    let mut device = std::fs::File::open(&volume.device).ok()?;
    device.seek(SeekFrom::Start(16 * 2048)).ok()?;

    let mut sector = vec![0; 2048];
    device.read_exact(&mut sector).ok()?;

    label_from_descriptor(&sector)
}

#[cfg(windows)]
fn platform_volume_label(volume: &OpticalVolume) -> Option<String> {
    use std::os::windows::ffi::OsStrExt;
    use windows_sys::Win32::Storage::FileSystem::GetVolumeInformationW;

    let root: Vec<u16> = volume
        .mount_point
        .as_os_str()
        .encode_wide()
        .chain([0])
        .collect();

    let mut name = [0u16; 261];

    let ok = unsafe {
        GetVolumeInformationW(
            root.as_ptr(),
            name.as_mut_ptr(),
            name.len() as u32,
            std::ptr::null_mut(),
            std::ptr::null_mut(),
            std::ptr::null_mut(),
            std::ptr::null_mut(),
            0,
        )
    };

    if ok == 0 {
        return None;
    }

    let len = name.iter().position(|&c| c == 0).unwrap_or(name.len());
    let label = String::from_utf16_lossy(&name[..len]);

    (!label.is_empty()).then_some(label)
}
