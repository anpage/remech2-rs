//! AI-generated tests for finding mounted optical discs

use std::path::PathBuf;

use gamedisc::volumes::{OpticalVolume, optical_volumes, parse_mounts};

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
