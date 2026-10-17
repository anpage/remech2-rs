//! AI-generated tests for finding mounted optical discs

use std::io::Cursor;
use std::path::PathBuf;

use gamedisc::volumes::{OpticalVolume, label_from_descriptor, optical_volumes, parse_mounts};
use hadris_iso::read::PathSeparator;
use hadris_iso::write::options::{CreationFeatures, IsoFormatOptions};
use hadris_iso::write::{File as IsoFile, InputFiles, IsoImageWriter};

fn volumes(list: &[(&str, &str)]) -> Vec<OpticalVolume> {
    list.iter()
        .map(|&(device, mount_point)| OpticalVolume {
            device: device.to_owned(),
            mount_point: PathBuf::from(mount_point),
        })
        .collect()
}

#[test]
fn optical_filesystems_in_order() {
    let mounts = "\
/dev/nvme0n1p2 / btrfs rw,relatime,ssd 0 0
/dev/sr0 /run/media/alex/MECH2 iso9660 ro,nosuid,nodev,relatime,uid=1000 0 0
tmpfs /tmp tmpfs rw,nosuid,nodev 0 0
/dev/sr1 /media/dvd udf ro,relatime 0 0
/dev/loop0 /mnt/image iso9660 ro,relatime 0 0
";

    assert_eq!(
        parse_mounts(mounts),
        volumes(&[
            ("/dev/sr0", "/run/media/alex/MECH2"),
            ("/dev/sr1", "/media/dvd"),
            ("/dev/loop0", "/mnt/image"),
        ])
    );
}

#[test]
fn other_filesystems_are_ignored() {
    let mounts = "\
fuseiso /home/alex/mnt fuse.fuseiso ro,nosuid,nodev 0 0
/dev/sda1 /boot/efi vfat rw 0 0
proc /proc proc rw 0 0
/dev/sr0 /mnt/iso9660 ext4 rw 0 0
";

    assert_eq!(parse_mounts(mounts), volumes(&[]));
}

#[test]
fn octal_escapes_in_both_columns() {
    // The kernel escapes space, tab, newline and backslash as \ooo
    let mounts = "\
/dev/sr0 /run/media/alex/MW2\\040DISC\\0401 iso9660 ro 0 0
/dev/sr1 /media/tab\\011and\\012newline iso9660 ro 0 0
/dev/disk/by-label/MW2\\134CD /media/back\\134slash udf ro 0 0
";

    assert_eq!(
        parse_mounts(mounts),
        volumes(&[
            ("/dev/sr0", "/run/media/alex/MW2 DISC 1"),
            ("/dev/sr1", "/media/tab\tand\nnewline"),
            ("/dev/disk/by-label/MW2\\CD", "/media/back\\slash"),
        ])
    );
}

#[test]
fn device_is_kept_as_written() {
    // Symlinks are resolved by whoever compares devices, not here
    let mounts = "/dev/cdrom /mnt/cdrom iso9660 ro 0 0\n";

    assert_eq!(
        parse_mounts(mounts),
        volumes(&[("/dev/cdrom", "/mnt/cdrom")])
    );
}

#[test]
fn bind_mounts_of_one_device_appear_once() {
    // As seen inside Distrobox, which bind-mounts the host's /run
    let mounts = "\
/dev/sr0 /run/host/run/media/alex/MECH2 iso9660 ro 0 0
/dev/sr1 /run/host/run/media/alex/OTHER udf ro 0 0
/dev/sr0 /run/media/alex/MECH2 iso9660 ro 0 0
/dev/sr1 /run/media/alex/OTHER udf ro 0 0
";

    assert_eq!(
        parse_mounts(mounts),
        volumes(&[
            ("/dev/sr0", "/run/host/run/media/alex/MECH2"),
            ("/dev/sr1", "/run/host/run/media/alex/OTHER"),
        ])
    );
}

#[test]
fn a_device_counts_once_only_as_an_optical_mount() {
    // A non-optical entry for the same device must not hide the optical one
    let mounts = "\
/dev/sr0 /mnt/odd ext4 rw 0 0
/dev/sr0 /media/cd iso9660 ro 0 0
";

    assert_eq!(parse_mounts(mounts), volumes(&[("/dev/sr0", "/media/cd")]));
}

#[test]
fn malformed_lines_are_skipped() {
    let mounts = "\ngarbage\n/dev/sr0 /mnt/short\n/dev/sr1 /mnt/ok iso9660 ro 0 0\n";

    assert_eq!(parse_mounts(mounts), volumes(&[("/dev/sr1", "/mnt/ok")]));
}

#[test]
fn empty_input() {
    assert_eq!(parse_mounts(""), volumes(&[]));
}

#[test]
fn optical_volumes_runs_on_this_machine() {
    // Whatever is mounted here, detection must not fail or panic
    for volume in optical_volumes() {
        assert!(!volume.device.is_empty());
        assert!(
            volume.mount_point.is_absolute(),
            "{}",
            volume.mount_point.display()
        );
    }
}

// Volume labels

const BLOCK: usize = 2048;

/// A volume descriptor with the given type, signature and label bytes
fn descriptor(kind: u8, signature: &[u8; 5], label: &[u8]) -> Vec<u8> {
    let mut sector = vec![0; BLOCK];
    sector[0] = kind;
    sector[1..6].copy_from_slice(signature);
    sector[6] = 1;
    sector[8..40].fill(b' ');
    sector[40..72].fill(b' ');
    sector[40..40 + label.len()].copy_from_slice(label);
    sector
}

fn primary(label: &[u8]) -> Vec<u8> {
    descriptor(1, b"CD001", label)
}

#[test]
fn label_is_trimmed() {
    assert_eq!(
        label_from_descriptor(&primary(b"MECH2")).as_deref(),
        Some("MECH2")
    );
}

#[test]
fn label_may_fill_the_field() {
    let full = b"ABCDEFGHIJKLMNOPQRSTUVWXYZ_01234";
    assert_eq!(full.len(), 32);
    assert_eq!(
        label_from_descriptor(&primary(full)).as_deref(),
        Some("ABCDEFGHIJKLMNOPQRSTUVWXYZ_01234")
    );
}

#[test]
fn label_padded_with_nuls() {
    // Some mastering tools pad with NUL instead of spaces
    let mut sector = primary(b"MW2");
    sector[43..72].fill(0);
    assert_eq!(label_from_descriptor(&sector).as_deref(), Some("MW2"));
}

#[test]
fn blank_label_is_none() {
    assert_eq!(label_from_descriptor(&primary(b"")), None);

    let mut nuls = primary(b"");
    nuls[40..72].fill(0);
    assert_eq!(label_from_descriptor(&nuls), None);
}

#[test]
fn only_primary_descriptors_have_labels() {
    // Type 2 is a supplementary (Joliet) descriptor, whose label is UCS-2
    assert_eq!(
        label_from_descriptor(&descriptor(2, b"CD001", b"MECH2")),
        None
    );
    assert_eq!(
        label_from_descriptor(&descriptor(1, b"CD002", b"MECH2")),
        None
    );
    assert_eq!(
        label_from_descriptor(&descriptor(1, b"BEA01", b"MECH2")),
        None
    );
}

#[test]
fn short_sectors_are_rejected() {
    let sector = primary(b"MECH2");
    assert_eq!(label_from_descriptor(&sector[..71]), None);
    assert_eq!(label_from_descriptor(&[]), None);
    // The label field itself is enough
    assert_eq!(
        label_from_descriptor(&sector[..72]).as_deref(),
        Some("MECH2")
    );
}

#[test]
fn label_from_a_written_image() {
    let image = iso("MECH2");
    assert_eq!(
        label_from_descriptor(&image[16 * BLOCK..17 * BLOCK]).as_deref(),
        Some("MECH2")
    );
}

fn iso(volume_name: &str) -> Vec<u8> {
    let options = IsoFormatOptions {
        volume_name: volume_name.to_owned(),
        system_id: None,
        volume_set_id: None,
        publisher_id: None,
        preparer_id: None,
        application_id: None,
        sector_size: BLOCK,
        path_separator: PathSeparator::ForwardSlash,
        features: CreationFeatures::default(),
        strict_charset: false,
    };
    let input = InputFiles {
        path_separator: PathSeparator::ForwardSlash,
        files: vec![IsoFile::File {
            name: std::sync::Arc::new("OLD_HERC.DRV".to_owned()),
            contents: b"hercules".to_vec(),
        }],
    };
    let mut buffer = Cursor::new(vec![]);
    IsoImageWriter::create(&mut buffer, input, options).expect("failed to write ISO");
    buffer.into_inner()
}

/// `OpticalVolume::label` reads the device directly on Unix, so an image file can stand in for a drive
#[cfg(unix)]
mod device {
    use std::fs;
    use std::path::{Path, PathBuf};

    use gamedisc::volumes::OpticalVolume;

    use super::{BLOCK, iso};

    fn scratch(name: &str) -> PathBuf {
        let dir = Path::new(env!("CARGO_TARGET_TMPDIR"))
            .join("volumes")
            .join(name);
        let _ = fs::remove_dir_all(&dir);
        fs::create_dir_all(&dir).unwrap();
        dir
    }

    fn volume(device: &Path) -> OpticalVolume {
        OpticalVolume {
            device: device.to_string_lossy().into_owned(),
            mount_point: PathBuf::from("/mnt/unused"),
        }
    }

    #[test]
    fn label_reads_the_device() {
        let dir = scratch("label_reads_the_device");
        let device = dir.join("sr0");
        fs::write(&device, iso("MECH2")).unwrap();

        assert_eq!(volume(&device).label().as_deref(), Some("MECH2"));
    }

    #[test]
    fn label_without_a_readable_descriptor() {
        let dir = scratch("label_without_a_readable_descriptor");

        assert_eq!(volume(&dir.join("missing")).label(), None);

        let short = dir.join("short");
        fs::write(&short, vec![0; 16 * BLOCK + 10]).unwrap();
        assert_eq!(volume(&short).label(), None);

        let blank = dir.join("blank");
        fs::write(&blank, vec![0; 20 * BLOCK]).unwrap();
        assert_eq!(volume(&blank).label(), None);
    }
}
